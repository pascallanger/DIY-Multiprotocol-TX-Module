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
// compatible with JJRC345

#if defined(JJRC345_NRF24L01_INO)

#include "iface_nrf24l01.h"

//#define JJRC345_FORCE_ID

#define JJRC345_PACKET_PERIOD		7450 // Timeout for callback in uSec
#define JJRC345_INITIAL_WAIT		500
#define JJRC345_PACKET_SIZE			16
#define JJRC345_RF_BIND_CHANNEL		5
#define JJRC345_BIND_COUNT			500
#define JJRC345_NUM_CHANNELS		4


enum JJRC345_FLAGS {
    // flags going to packet[8]
	JJRC345_FLAG_HEADLESS	= 0x40,
	JJRC345_FLAG_RTH		= 0x80,
};

static uint8_t __attribute__((unused)) JJRC345_convert_channel(uint8_t num)
{
	uint8_t val=convert_channel_8b(num);
	// 7E..60..41..01, 80 center, 81..C1..E0..FE
	if(val<0x80)
	{
		val=0x80-val;	// 80..01
		if(val>0x7E)
			val=0x7E;	// 7E..01
	}
	else if(val>0xFE)
		val=0xFE;			// 81..FE
	return val;
}

static void __attribute__((unused)) JJRC345_send_packet()
{
	packet[0] = 0x00;
	packet[2] = 0x00;
	if (IS_BIND_IN_PROGRESS)
	{ //00 05 00 0A 46 4A 41 47 00 00 40 46 A5 4A F1 18
		packet[1]  = JJRC345_RF_BIND_CHANNEL;
		packet[4]  = hopping_frequency[0];
		packet[5]  = hopping_frequency[1];
		packet[6]  = hopping_frequency[2];
		packet[7]  = hopping_frequency[3];
		packet[12] = 0xa5;
	}
	else
	{ //00 41 00 0A 00 80 80 80 00 00 40 46 00 49 F1 18
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
		hopping_frequency_no++;
		hopping_frequency_no %= JJRC345_NUM_CHANNELS;
		packet[1]  = hopping_frequency[hopping_frequency_no];	// next packet will be sent on this channel

		packet[4]  = convert_channel_8b(THROTTLE);				// throttle: 00..FF
		packet[5]  = JJRC345_convert_channel(RUDDER);			// rudder: 70..60..41..01, 80 center, 81..C1..E0..F0
		packet[6]  = JJRC345_convert_channel(ELEVATOR);			// elevator: 70..60..41..01, 80 center, 81..C1..E0..F0
		packet[7]  = JJRC345_convert_channel(AILERON);			// aileron: 70..60..41..01, 80 center, 81..C1..E0..F0

		if(CH5_SW)	//Flip
		{
			if(packet[6]>0xF0)
				packet[6]=0xFF;
			else if(packet[6]<0x80 && packet[6]>0x70)
				packet[6]=0x7F;
			if(packet[7]>0xF0)
				packet[7]=0xFF;
			else if(packet[7]<0x80 && packet[7]>0x70)
				packet[7]=0x7F;
		}
		
		packet[12] = 0x02;										// Rate: 00-01-02
	}

	packet[3] = 0x00;											// Checksum upper bits

	packet[8] = 0x00											// Rudder trim, 00 when not used, 01..1F when trimmed left, 20..3F
				| GET_FLAG(CH6_SW,JJRC345_FLAG_HEADLESS)		// Headless mode: 00 normal, 40 headless
				| GET_FLAG(CH7_SW,JJRC345_FLAG_RTH);			// RTH: 80 active
	packet[9] = 0;												// Elevator trim, 00 when not used, 20..25 when trimmed up, 0..1F when trimmed down
	packet[10] = 0x40;											// Aileron trim, 40 when not used, 40..5F when trimmed left, 61..7F when trimmed right

	packet[11] = hopping_frequency[0];							// First hopping frequency

	// Checksum
	uint16_t sum=2;
	for (uint8_t i = 0; i < 13; i++)
		sum += packet[i];
	packet[13]=sum;
	packet[3]=((sum>>8)<<2)+2;
	
	// TX ID
	packet[14] = rx_tx_addr[2];
	packet[15] = rx_tx_addr[3];

	// Power on, TX mode
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, JJRC345_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) JJRC345_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", 5);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, JJRC345_RF_BIND_CHANNEL);	// Bind channel
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);					// Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);						// No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);					// Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_1M);							// 1 Mbps
    NRF24L01_SetPower();
}

uint16_t JJRC345_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(JJRC345_PACKET_PERIOD);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		if (bind_counter)
			bind_counter--;
		else
			BIND_DONE;
	}
	JJRC345_send_packet();
	return	JJRC345_PACKET_PERIOD;
}

static void __attribute__((unused)) JJRC345_initialize_txid()
{
	calc_fh_channels(JJRC345_NUM_CHANNELS);
	
	#ifdef JJRC345_FORCE_ID
		//TX 1
		rx_tx_addr[2]=0x1B;
		rx_tx_addr[3]=0x12;
        hopping_frequency[0] = 0x3f;
        hopping_frequency[1] = 0x49;
        hopping_frequency[2] = 0x47;
        hopping_frequency[3] = 0x47;
		//TX 2
		rx_tx_addr[2]=0xF1;
		rx_tx_addr[3]=0x18;
        hopping_frequency[0] = 0x46;
        hopping_frequency[1] = 0x4A;
        hopping_frequency[2] = 0x41;
        hopping_frequency[3] = 0x47;
	#endif
}

uint16_t initJJRC345(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = JJRC345_BIND_COUNT;
	JJRC345_initialize_txid();
	JJRC345_init();
	return	JJRC345_INITIAL_WAIT;
}

#endif
