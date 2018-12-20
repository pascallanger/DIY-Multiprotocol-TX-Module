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
// compatible with Cheerson CX-10 blue & newer red pcb, CX-10A, CX11, CX-10 green pcb, DM007, Floureon FX-10, JXD 509 (Q282), Q222, Q242 and Q282
// Last sync with hexfet new_protocols/cx10_nrf24l01.c dated 2015-11-26

#if defined(CX10_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CX10_BIND_COUNT		4360   // 6 seconds
#define CX10_PACKET_SIZE	15
#define CX10A_PACKET_SIZE	19       // CX10 blue board packets have 19-byte payload
#define Q2X2_PACKET_SIZE	21
#define CX10_PACKET_PERIOD	1316  // Timeout for callback in uSec
#define CX10A_PACKET_PERIOD	6000

#define CX10_INITIAL_WAIT     500

// flags
#define CX10_FLAG_FLIP       0x10 // goes to rudder channel
#define CX10_FLAG_MODE_MASK  0x03
#define CX10_FLAG_HEADLESS   0x04
// flags2
#define CX10_FLAG_VIDEO      0x02
#define CX10_FLAG_SNAPSHOT   0x04

// frequency channel management
#define CX10_RF_BIND_CHANNEL 0x02
#define CX10_NUM_RF_CHANNELS    4

enum {
    CX10_BIND1 = 0,
    CX10_BIND2,
    CX10_DATA
};

static void __attribute__((unused)) CX10_Write_Packet(uint8_t bind)
{
	uint8_t offset = 0;
	if(sub_protocol == CX10_BLUE)
		offset = 4;
	packet[0] = bind ? 0xAA : 0x55;
	packet[1] = rx_tx_addr[0];
	packet[2] = rx_tx_addr[1];
	packet[3] = rx_tx_addr[2];
	packet[4] = rx_tx_addr[3];
	// packet[5] to [8] (aircraft id) is filled during bind for blue board
	uint16_t aileron= convert_channel_16b_limit(AILERON ,1000,2000);
	uint16_t elevator=convert_channel_16b_limit(ELEVATOR,2000,1000);
	uint16_t throttle=convert_channel_16b_limit(THROTTLE,1000,2000);
	uint16_t rudder=  convert_channel_16b_limit(RUDDER  ,2000,1000);
    // Channel 5 - flip flag
	packet[12+offset] = GET_FLAG(CH5_SW,CX10_FLAG_FLIP); // flip flag applied on rudder

	// Channel 6 - rate mode is 2 lsb of packet 13
	if(CH6_SW)		// rate 3 / headless on CX-10A
		flags = 0x02;
	else
		if(Channel_data[CH6] < CHANNEL_MIN_COMMAND)
			flags = 0x00;			// rate 1
		else
			flags = 0x01;			// rate 2
	uint8_t flags2=0;	// packet 14

	uint8_t video_state=packet[14] & 0x21;
	switch(sub_protocol)
	{
		case CX10_BLUE:
			flags |= GET_FLAG(!CH7_SW, 0x10)	// Channel 7 - picture
					|GET_FLAG( CH8_SW, 0x08);	// Channel 8 - video
			break;
		case F_Q282:
		case F_Q242:
		case F_Q222:
			memcpy(&packet[15], "\x10\x10\xaa\xaa\x00\x00", 6);
			//FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL
			flags2 = GET_FLAG(CH5_SW, 0x80)		// Channel 5 - FLIP
					|GET_FLAG(!CH6_SW, 0x40)	// Channel 6 - LED
					|GET_FLAG(CH9_SW, 0x08)		// Channel 9 - HEADLESS
					|GET_FLAG(CH11_SW, 0x04)		// Channel 11 - XCAL
					|GET_FLAG(CH12_SW, 0x02);	// Channel 12 - YCAL or Start/Stop motors on JXD 509
	
			if(sub_protocol==F_Q242)
			{
				flags=2;
				flags2|= GET_FLAG(CH7_SW,0x01)	// Channel 7 - picture
						|GET_FLAG(CH8_SW,0x10);	// Channel 8 - video
				packet[17]=0x00;
				packet[18]=0x00;
			}
			else
			{ // F_Q282 & F_Q222
				flags=3;							// expert
				if(CH8_SW)						// Channel 8 - F_Q282 video / F_Q222 Module 1
				{
					if (!(video_state & 0x20)) video_state ^= 0x21;
				}
				else
					if (video_state & 0x20) video_state &= 0x01;
				flags2 |= video_state
						|GET_FLAG(CH7_SW,0x10);	// Channel 7 - F_Q282 picture / F_Q222 Module 2
			}
			if(CH10_SW)	flags |=0x80;			// Channel 10 - RTH
			break;
		case DM007:
			aileron = 3000 - aileron;
			//FLIP|MODE|PICTURE|VIDEO|HEADLESS
			flags2=  GET_FLAG(CH7_SW,CX10_FLAG_SNAPSHOT)	// Channel 7 - picture
					|GET_FLAG(CH8_SW,CX10_FLAG_VIDEO);		// Channel 8 - video
			if(CH9_SW)	flags |= CX10_FLAG_HEADLESS;		// Channel 9 - headless
			break;
		case JC3015_2:
			aileron = 3000 - aileron;
			elevator = 3000 - elevator;
			//FLIP|MODE|LED|DFLIP
			if(CH8_SW)	packet[12] &= ~CX10_FLAG_FLIP;
		case JC3015_1:
			//FLIP|MODE|PICTURE|VIDEO
			flags2=	 GET_FLAG(CH7_SW,_BV(3))	// Channel 7
					|GET_FLAG(CH8_SW,_BV(4));	// Channel 8
			break;
		case MK33041:
			elevator = 3000 - elevator;
			//FLIP|MODE|PICTURE|VIDEO|HEADLESS|RTH
			flags|=GET_FLAG(CH7_SW,_BV(7))	// Channel 7 - picture
				  |GET_FLAG(CH10_SW,_BV(2));	// Channel 10 - rth
			flags2=GET_FLAG(CH8_SW,_BV(0))	// Channel 8 - video
				  |GET_FLAG(CH9_SW,_BV(5));	// Channel 9 - headless
			break;
	}
	packet[5+offset] = lowByte(aileron);
	packet[6+offset] = highByte(aileron);
	packet[7+offset] = lowByte(elevator);
	packet[8+offset] = highByte(elevator);
	packet[9+offset] = lowByte(throttle);
	packet[10+offset]= highByte(throttle);
	packet[11+offset]= lowByte(rudder);
	packet[12+offset]|= highByte(rudder);
	packet[13+offset]=flags;
	packet[14+offset]=flags2;
	
	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, CX10_RF_BIND_CHANNEL);
	else
	{
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
		hopping_frequency_no %= CX10_NUM_RF_CHANNELS;
	}
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	XN297_WritePayload(packet, packet_length);
	NRF24L01_SetPower();
}

static void __attribute__((unused)) CX10_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc",5);
	XN297_SetRXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc",5);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_length);	// rx pipe 0 (used only for blue board)
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, CX10_RF_BIND_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
	NRF24L01_SetPower();
}

uint16_t CX10_callback()
{
	switch (phase) {
		case CX10_BIND1:
			if (bind_counter == 0)
			{
				phase = CX10_DATA;
				BIND_DONE;
			}
			else
			{
				CX10_Write_Packet(1);
				bind_counter--;
			}
			break;
		case CX10_BIND2:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				XN297_ReadPayload(packet, packet_length);
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				if(packet[9] == 1)
				{
					BIND_DONE;
					phase = CX10_DATA;
				}
			}
			else
			{
				// switch to TX mode
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_FlushTx();
				NRF24L01_SetTxRxMode(TX_EN);
				CX10_Write_Packet(1);
				delayMicroseconds(400);
				// switch to RX mode
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_FlushRx();
				NRF24L01_SetTxRxMode(RX_EN);
				XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
			}
			break;
		case CX10_DATA:
			CX10_Write_Packet(0);
			break;
	}
	return packet_period;
}

static void __attribute__((unused)) CX10_initialize_txid()
{
	rx_tx_addr[1]%= 0x30;
	if(sub_protocol&0x08)	//F_Q2X2 protocols
	{
		uint8_t offset=0;	//F_Q282
		if(sub_protocol==F_Q242)
			offset=2;
		if(sub_protocol==F_Q222)
			offset=3;
		for(uint8_t i=0;i<4;i++)
			hopping_frequency[i]=0x46+2*i+offset;
	}
	else
	{
		hopping_frequency[0] = 0x03 + (rx_tx_addr[0] & 0x0F);
		hopping_frequency[1] = 0x16 + (rx_tx_addr[0] >> 4);
		hopping_frequency[2] = 0x2D + (rx_tx_addr[1] & 0x0F);
		hopping_frequency[3] = 0x40 + (rx_tx_addr[1] >> 4);
	}
}

uint16_t initCX10(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	if(sub_protocol==CX10_BLUE)
	{
		packet_length = CX10A_PACKET_SIZE;
		packet_period = CX10A_PACKET_PERIOD;

		phase = CX10_BIND2;

		for(uint8_t i=0; i<4; i++)
			packet[5+i] = 0xff; // clear aircraft id
		packet[9] = 0;
	}
	else
	{
		if(sub_protocol&0x08)	//F_Q2X2 protocols
			packet_length = Q2X2_PACKET_SIZE;
		else
		    packet_length = CX10_PACKET_SIZE;
		packet_period = CX10_PACKET_PERIOD;
		phase = CX10_BIND1;
		bind_counter = CX10_BIND_COUNT;
	}
	CX10_initialize_txid();
	CX10_init();
	return CX10_INITIAL_WAIT+packet_period;
}

#endif
