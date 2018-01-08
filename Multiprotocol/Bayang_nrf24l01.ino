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

#define BAYANG_BIND_COUNT		1000
#define BAYANG_PACKET_PERIOD	1000
#define BAYANG_INITIAL_WAIT		500
#define BAYANG_PACKET_SIZE		15
#define BAYANG_RF_NUM_CHANNELS	4
#define BAYANG_RF_BIND_CHANNEL	0
#define BAYANG_RF_BIND_CHANNEL_X16_AH 10
#define BAYANG_ADDRESS_LENGTH	5

enum BAYANG_FLAGS {
	// flags going to packet[2]
	BAYANG_FLAG_RTH			= 0x01,
	BAYANG_FLAG_HEADLESS	= 0x02, 
	BAYANG_FLAG_FLIP		= 0x08,
	BAYANG_FLAG_VIDEO		= 0x10, 
	BAYANG_FLAG_PICTURE		= 0x20, 
	// flags going to packet[3]
	BAYANG_FLAG_INVERTED	= 0x80, // inverted flight on Floureon H101
	BAYANG_FLAG_TAKE_OFF	= 0x20, // take off / landing on X16 AH
	BAYANG_FLAG_EMG_STOP	= 0x04,
};

static void __attribute__((unused)) BAYANG_send_packet(uint8_t bind)
{
	uint8_t i;
	if (bind)
	{
	#ifdef BAYANG_HUB_TELEMETRY
		if(option)
			packet[0]= 0xA3;	// telemetry is enabled
		else
	#endif
			packet[0]= 0xA4;
		for(i=0;i<5;i++)
			packet[i+1]=rx_tx_addr[i];
		for(i=0;i<4;i++)
			packet[i+6]=hopping_frequency[i];
		switch (sub_protocol)
		{
			case X16_AH:
				packet[10] = 0x00;
				packet[11] = 0x00;
				break;
			case IRDRONE:
				packet[10] = 0x30;
				packet[11] = 0x01;
				break;
			default:
				packet[10] = rx_tx_addr[0];	// txid[0]
				packet[11] = rx_tx_addr[1];	// txid[1]
				break;
		}
	}
	else
	{
		uint16_t val;
		uint8_t dyntrim = 1;
		switch (sub_protocol)
		{
			case X16_AH:
			case IRDRONE:
				packet[0] = 0xA6;
				break;
			default:
				packet[0] = 0xA5;
				break;
		}
		packet[1] = 0xFA;		// normal mode is 0xf7, expert 0xfa

		//Flags packet[2]
		packet[2] = 0x00;
		if(CH5_SW)
			packet[2] = BAYANG_FLAG_FLIP;
		if(CH6_SW)
			packet[2] |= BAYANG_FLAG_RTH;
		if(CH7_SW)
			packet[2] |= BAYANG_FLAG_PICTURE;
		if(CH8_SW)
			packet[2] |= BAYANG_FLAG_VIDEO;
		if(CH9_SW)
		{
			packet[2] |= BAYANG_FLAG_HEADLESS;
			dyntrim = 0;
		}
		//Flags packet[3]
		packet[3] = 0x00;
		if(CH10_SW)
			packet[3] = BAYANG_FLAG_INVERTED;
		if(CH11_SW)
			dyntrim = 0;
		if(CH12_SW)
			packet[3] |= BAYANG_FLAG_TAKE_OFF;
		if(CH13_SW)
			packet[3] |= BAYANG_FLAG_EMG_STOP;
		//Aileron
		val = convert_channel_10b(AILERON);
		packet[4] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
		packet[5] = val & 0xFF;
		//Elevator
		val = convert_channel_10b(ELEVATOR);
		packet[6] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
		packet[7] = val & 0xFF;
		//Throttle
		val = convert_channel_10b(THROTTLE);
		packet[8] = (val>>8) + 0x7C;
		packet[9] = val & 0xFF;
		//Rudder
		val = convert_channel_10b(RUDDER);
		packet[10] = (val>>8) + (dyntrim ? ((val>>2) & 0xFC) : 0x7C);
		packet[11] = val & 0xFF;
	}
	switch (sub_protocol)
	{
		case H8S3D:
			packet[12] = rx_tx_addr[2];	// txid[2]
			packet[13] = 0x34;
			break;
		case X16_AH:
			packet[12] = 0;
			packet[13] = 0;
			break;
		case IRDRONE:
			packet[12] = 0xE0;
			packet[13] = 0x2E;
			break;
		default:
			packet[12] = rx_tx_addr[2];	// txid[2]
			packet[13] = 0x0A;
			break;
	}
	packet[14] = 0;
	for (uint8_t i=0; i < BAYANG_PACKET_SIZE-1; i++)
		packet[14] += packet[i];

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? rf_ch_num:hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no%=BAYANG_RF_NUM_CHANNELS;

	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_SetTxRxMode(TX_EN);

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

	#ifdef BAYANG_HUB_TELEMETRY
	if (option)
	{	// switch radio to rx as soon as packet is sent
		while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS)));
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);
    }
	#endif

	NRF24L01_SetPower();	// Set tx_power
}

#ifdef BAYANG_HUB_TELEMETRY
static void __attribute__((unused)) BAYANG_check_rx(void)
{
	if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
	{ // data received from model
		XN297_ReadPayload(packet, BAYANG_PACKET_SIZE);
		NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);

		NRF24L01_FlushRx();
		uint8_t check = packet[0];
		for (uint8_t i=1; i < BAYANG_PACKET_SIZE-1; i++)
			check += packet[i];
		// decode data , check sum is ok as well, since there is no crc
		if (packet[0] == 0x85 && packet[14] == check)
		{
			// uncompensated battery volts*100/2
			v_lipo1 = (packet[3]<<7) + (packet[4]>>2);
			// compensated battery volts*100/2
			v_lipo2 = (packet[5]<<7) + (packet[6]>>2);
			// reception in packets / sec
			RX_RSSI = packet[7];
			//Flags
			//uint8_t flags = packet[3] >> 3;
			// battery low: flags & 1
			telemetry_counter++;
			if(telemetry_lost==0)
				telemetry_link=1;
		}
	}
}
#endif

static void __attribute__((unused)) BAYANG_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", BAYANG_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_PACKET_SIZE);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// No retransmits
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
	
	switch (sub_protocol)
	{
		case X16_AH:
		case IRDRONE:
			rf_ch_num = BAYANG_RF_BIND_CHANNEL_X16_AH;
			break;
		default:
			rf_ch_num = BAYANG_RF_BIND_CHANNEL;
			break;
	}
}

uint16_t BAYANG_callback()
{
	if(IS_BIND_DONE)
	{
		if(packet_count==0)
			BAYANG_send_packet(0);
		packet_count++;
		#ifdef BAYANG_HUB_TELEMETRY
			if (option)
			{	// telemetry is enabled 
				state++;
				if (state > 1000)
				{
					//calculate telemetry reception packet rate - packets per 1000ms
					TX_RSSI = telemetry_counter;
					telemetry_counter = 0;
					state = 0;
					telemetry_lost=0;
				}

				if (packet_count > 1)
					BAYANG_check_rx();

				packet_count %= 5;
			}
			else
		#endif
				packet_count%=2;
	}
	else
	{
		if (bind_counter == 0)
		{
			XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
			#ifdef BAYANG_HUB_TELEMETRY
				XN297_SetRXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
			#endif
			BIND_DONE;
		}
		else
		{
			if(packet_count==0)
				BAYANG_send_packet(1);
			packet_count++;
			packet_count%=4;
			bind_counter--;
		}
	}
	return BAYANG_PACKET_PERIOD;
}

static void __attribute__((unused)) BAYANG_initialize_txid()
{
	//Could be using txid[0..2] but using rx_tx_addr everywhere instead...
	hopping_frequency[0]=0;
	hopping_frequency[1]=(rx_tx_addr[3]&0x1F)+0x10;
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
	packet_count=0;
	#ifdef BAYANG_HUB_TELEMETRY
		init_frskyd_link_telemetry();
		telemetry_lost=1;	// do not send telemetry to TX right away until we have a TX_RSSI value to prevent warning message...
	#endif
	return BAYANG_INITIAL_WAIT+BAYANG_PACKET_PERIOD;
}

#endif