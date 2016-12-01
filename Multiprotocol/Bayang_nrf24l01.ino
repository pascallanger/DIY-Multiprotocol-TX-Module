/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
// Compatible with EAchine H8 mini, H10, BayangToys X6/X7/X9, JJRC JJ850 ...
// Last sync with hexfet new_protocols/bayang_nrf24l01.c dated 2015-12-22

#if defined(BAYANG_NRF24L01_INO)
#include "iface_nrf24l01.h"

#ifdef ENABLE_BAYANG_TELEMETRY
	#define BAYANG_PACKET_PERIOD_TELEM    3200
	uint32_t bayang_telemetry_last_rx = 0;
#endif


#define BAYANG_BIND_COUNT		1000
#define BAYANG_PACKET_PERIOD	2000
#define BAYANG_INITIAL_WAIT		500
#define BAYANG_PACKET_SIZE		15
#define BAYANG_RF_NUM_CHANNELS	4
#define BAYANG_RF_BIND_CHANNEL	0
#define BAYANG_ADDRESS_LENGTH	5

enum BAYANG_FLAGS {
	// flags going to packet[2]
	BAYANG_FLAG_RTH			= 0x01,
	BAYANG_FLAG_HEADLESS	= 0x02,
#ifdef ENABLE_BAYANG_TELEMETRY
	BAYANG_FLAG_TELEMETRY	= 0x04,
#endif
	BAYANG_FLAG_FLIP		= 0x08,
	BAYANG_FLAG_VIDEO		= 0x10, 
	BAYANG_FLAG_PICTURE		= 0x20, 
	// flags going to packet[3]
	BAYANG_FLAG_INVERTED	= 0x80, // inverted flight on Floureon H101
#ifdef ENABLE_BAYANG_TELEMETRY
	BAYANG_FLAG_FLIGHT_MODE0 = 0x01,
	BAYANG_FLAG_FLIGHT_MODE1 = 0x02,
	BAYANG_FLAG_DATA_SELECT0 = 0x04,
	BAYANG_FLAG_DATA_SELECT1 = 0x08,
	BAYANG_FLAG_DATA_SELECT2 = 0x10,
	BAYANG_FLAG_DATA_ADJUST0 = 0x20,
	BAYANG_FLAG_DATA_ADJUST1 = 0x40,
#endif

};

static void __attribute__((unused)) BAYANG_send_packet(uint8_t bind)
{
	uint8_t i;
	if (bind)
	{
		packet[0]= 0xA4;
		for(i=0;i<5;i++)
			packet[i+1]=rx_tx_addr[i];
		for(i=0;i<4;i++)
			packet[i+6]=hopping_frequency[i];
		packet[10] = rx_tx_addr[0];	// txid[0]
		packet[11] = rx_tx_addr[1];	// txid[1]
	}
	else
	{

		int telem_enabled = 0;
#ifdef ENABLE_BAYANG_TELEMETRY
		telem_enabled = (sub_protocol == BAYANG_TELEM);
		// telem_enabled &= Servo_AUX8; // enable telem with a switch
#endif
		uint16_t val;
		packet[0] = 0xA5;
		packet[1] = 0xFA;		// normal mode is 0xf7, expert 0xfa

		//Flags packet[2]
		packet[2] = 0x00;
		if(Servo_AUX1)
			packet[2] = BAYANG_FLAG_FLIP;
		if(Servo_AUX2)
			packet[2] |= BAYANG_FLAG_RTH;
		if(Servo_AUX3)
			packet[2] |= BAYANG_FLAG_PICTURE;
		if(Servo_AUX4)
			packet[2] |= BAYANG_FLAG_VIDEO;
		if(Servo_AUX5)
			packet[2] |= BAYANG_FLAG_HEADLESS;

#ifdef ENABLE_BAYANG_TELEMETRY
		if (telem_enabled)
		{
			packet[2] |= BAYANG_FLAG_TELEMETRY;
		}
#endif
	  

		//Flags packet[3]
		packet[3] = 0x00;
		if(Servo_AUX6)
			packet[3] = BAYANG_FLAG_INVERTED;

#ifdef ENABLE_BAYANG_TELEMETRY
		if (telem_enabled)
		{
			static uint8_t dataselect = 2;
			uint8_t dataselect_old = dataselect;
			uint16_t partitions[4] ={1200,1400,1600,1800}; // 5 options (previous data set, data 1, data 2, data 3, next data set)
			for (uint8_t i = 0; i < 4; ++i)
			{
				int16_t hysteresis = 0;
				if (dataselect_old*2 == (i*2+1) - 1)
				{
					hysteresis = 25;
				}
				else if (dataselect_old*2 == (i*2+1) + 1)
				{
					hysteresis = -25;
				}
				
				if (Servo_data[AUX7] <= partitions[i] + hysteresis)
				{
					dataselect = i;
					break;
				}
				else
				{
					dataselect = i+1;
				}
			}
			
			// data adjust 1333  1666 - aux 6
			static uint8_t dataadjust = 1;
			uint8_t dataadjust_old = dataadjust;
			partitions[0] = 1333; // three options (decreaes, do nothing, increase)
			partitions[1] = 1666;
			for (uint8_t i = 0; i < 2; ++i)
			{
				int16_t hysteresis = 0;
				if (dataadjust_old*2 == (i*2+1) - 1)
				{
					hysteresis = 25;
				}
				else if (dataadjust_old*2 == (i*2+1) + 1)
				{
					hysteresis = -25;
				}
				
				if (Servo_data[AUX8] <= partitions[i] + hysteresis)
				{
					dataadjust = i;
					break;
				}
				else
				{
					dataadjust = i+1;
				}
			}
			
			static uint8_t flightmode = 0;
			// flight mode 1250 1500 1750 - aux 1
			uint8_t flightmode_old = flightmode;
			partitions[0] = 1250;  // 4 flight modes
			partitions[1] = 1500;
			partitions[2] = 1750;
			for (uint8_t i = 0; i < 3; ++i)
			{
				int16_t hysteresis = 0;
				if (flightmode_old*2 == (i*2+1) - 1)
				{
					hysteresis = 25;
				}
				else if (flightmode_old*2 == (i*2+1) + 1)
				{
					hysteresis = -25;
				}
				
				if (Servo_data[AUX9] <= partitions[i] + hysteresis)
				{
					flightmode = i;
					break;
				}
				else
				{
					flightmode = i+1;
				}
			}
			packet[3] |= (flightmode & 0x3);
			packet[3] |= (dataselect & 0x7) << 2;
			packet[3] |= (dataadjust & 0x3) << 5;
		}
#endif		

		//Aileron
		val = convert_channel_10b(AILERON);
		packet[4] = (val>>8) + ((val>>2) & 0xFC);
		packet[5] = val & 0xFF;
		//Elevator
		val = convert_channel_10b(ELEVATOR);
		packet[6] = (val>>8) + ((val>>2) & 0xFC);
		packet[7] = val & 0xFF;
		//Throttle
		val = convert_channel_10b(THROTTLE);
		packet[8] = (val>>8) + 0x7C;
		packet[9] = val & 0xFF;
		//Rudder
		val = convert_channel_10b(RUDDER);
		packet[10] = (val>>8) + (val>>2 & 0xFC);
		packet[11] = val & 0xFF;
	}
	packet[12] = rx_tx_addr[2];	// txid[2]
	packet[13] = 0x0A;
	packet[14] = 0;
	for (uint8_t i=0; i < BAYANG_PACKET_SIZE-1; i++)
		packet[14] += packet[i];

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? BAYANG_RF_BIND_CHANNEL:hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no%=BAYANG_RF_NUM_CHANNELS;
	
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) BAYANG_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", BAYANG_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE); // rx pipe 0
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
	NRF24L01_SetPower();

}

uint16_t BAYANG_callback()
{
	if(IS_BIND_DONE_on)
	{
#ifdef ENABLE_BAYANG_TELEMETRY
		if(sub_protocol == BAYANG_TELEM)
		{
			Bayang_process();
			// Return 0 here so that Bayang_process is called as often as possible
			// to check for the incomming telemetry data. When telemetry is enabled
			// the packet period is handled in the Bayang_process function.
			return 0; 
		}
		else
#endif
		{
			BAYANG_send_packet(0);
		}
	}
	else
	{
		if (bind_counter == 0)
		{
			XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
			XN297_SetRXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);

			BIND_DONE;
		}
		else
		{
			BAYANG_send_packet(1);
			bind_counter--;
		}
	}
	return BAYANG_PACKET_PERIOD;
}

static void __attribute__((unused)) BAYANG_initialize_txid()
{
	//Could be using txid[0..2] but using rx_tx_addr everywhere instead...
	hopping_frequency[0]=0;
	hopping_frequency[1]=(rx_tx_addr[0]&0x1F)+0x10;
	hopping_frequency[2]=hopping_frequency[1]+0x20;
	hopping_frequency[3]=hopping_frequency[2]+0x20;
	hopping_frequency_no=0;
}

uint16_t initBAYANG(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	bind_counter = BAYANG_BIND_COUNT;
	BAYANG_initialize_txid();
	BAYANG_init();
	return BAYANG_INITIAL_WAIT+BAYANG_PACKET_PERIOD;
}


#ifdef ENABLE_BAYANG_TELEMETRY
	extern float    telemetry_voltage;
	extern uint8_t  telemetry_rx_rssi;
	extern uint8_t  telemetry_tx_rssi;
	extern uint8_t  telemetry_datamode;
	extern float    telemetry_data[3]; // pids
	extern uint8_t  telemetry_dataitem;
	extern uint16_t telemetry_uptime;
	extern uint16_t telemetry_flighttime;
	extern uint8_t  telemetry_flightmode;

	uint8_t telemetry_tx_rssi_count;
	uint8_t telemetry_tx_rssi_next;


	uint8_t BAYANG_recv_packet() {
		uint8_t received = 0;
		if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
			int sum = 0;
			uint16_t roll, pitch, yaw, throttle;

			XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
			
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			NRF24L01_FlushRx();
			
			if (packet[0] == 0xA9) {
				//serialDebug.print("data packet");
				for (int i = 0; i < 14; i++) {
					sum += packet[i];
				}
				if ( (sum & 0xFF) == packet[14] ) {
					typedef union {
						uint16_t v;
						uint8_t  bytes[2];
					} val;
					val v;
					
					v.bytes[0] = packet[1];
					v.bytes[1] = packet[2];
					telemetry_voltage = v.v / 100.f;

					telemetry_rx_rssi = packet[3];
					

					v.bytes[0] = packet[4];
					v.bytes[1] = (packet[6] & 0x0F); // 12 bit # shared with flighttime
					telemetry_uptime = v.v;
					
					v.bytes[0] = packet[5];
					v.bytes[1] = (packet[6] >> 4);
					telemetry_flighttime = v.v;
					
					telemetry_flightmode = packet[7] & 0x3; // 0 = level, 1 = acro,
					telemetry_datamode   = (packet[7] >> 2) & 0xF;  // (0=acro yaw, 1=acro roll/acro pitch, 2=level roll/pitch)
					telemetry_dataitem   = (packet[7] >> 6) & 0x3;  // (0=acro yaw, 1=acro roll/acro pitch, 2=level roll/pitch)
					
					v.bytes[0] = packet[8];
					v.bytes[1] = packet[9];
					telemetry_data[0] = v.v/1000.f;
					
					v.bytes[0] = packet[10];
					v.bytes[1] = packet[11];
					telemetry_data[1] = v.v/1000.f;
					
					v.bytes[0] = packet[12];
					v.bytes[1] = packet[13];
					telemetry_data[2] = v.v/1000.f;
				}
			}
			received = 1;
		}
		return received;
	}


	typedef enum
	{
		BAYANG_STATE_IDLE,
		BAYANG_STATE_TRANSMITTING,
		BAYANG_STATE_RECEIEVING,
	} BayangState;

	BayangState Bayang_state = BAYANG_STATE_IDLE;
	uint32_t Bayang_next_send = 0;

	void Bayang_process() {
		uint32_t time_micros = micros();
		
		if (BAYANG_STATE_TRANSMITTING == Bayang_state) {
			if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS)) {
				// send finished, switch to rx to receive telemetry
				XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
				NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
				NRF24L01_FlushTx();
				NRF24L01_FlushRx();
				
				telemetry_tx_rssi_count++;
				
				Bayang_state = BAYANG_STATE_RECEIEVING;
			}
		}
		else if (BAYANG_STATE_RECEIEVING == Bayang_state) {
			// 250us is about the time it takes to read a packet over spi
			if (time_micros > (Bayang_next_send-250)) {
				// didnt receive telemetry in time
				Bayang_state = BAYANG_STATE_IDLE;

				// if it's been a while since receiving telemetry data,
				// stop sending the telemetry data to the transmitter
				if (millis() - bayang_telemetry_last_rx > 1000)
					telemetry_lost = 1;
			}
			else if (BAYANG_recv_packet()) {
				telemetry_tx_rssi_next++;
				// received telemetry packet
				telemetry_lost = 0;
				bayang_telemetry_last_rx = millis();
			}
			else
			{
				return;
			}
		
			if (100 == telemetry_tx_rssi_count)
			{
				telemetry_tx_rssi = telemetry_tx_rssi_next;
				telemetry_tx_rssi_next = 0;
				telemetry_tx_rssi_count = 0;
			}
			
			Bayang_state = BAYANG_STATE_IDLE;
			XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
			NRF24L01_FlushRx();
		}
		else if (time_micros > Bayang_next_send) {
			BAYANG_send_packet(0);
			Bayang_state = BAYANG_STATE_TRANSMITTING;
			Bayang_next_send = time_micros + BAYANG_PACKET_PERIOD_TELEM;
		}
	}
#endif //ENABLE_BAYANG_TELEMETRY
#endif
