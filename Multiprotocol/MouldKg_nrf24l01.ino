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
#if defined(MOULDKG_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_MOULDKG_ORIGINAL_ID

#define MOULDKG_PACKET_PERIOD		5000
#define MOULDKG_BIND_PACKET_PERIOD	12000
#define MOULDKG_TX_BIND_CHANNEL		11
#define MOULDKG_RX_BIND_CHANNEL		76
#define MOULDKG_PAYLOAD_SIZE		5
#define MOULDKG_BIND_PAYLOAD_SIZE	7
#define MOULDKG_BIND_COUNT			300
#define MOULDKG_RF_NUM_CHANNELS		4

enum {
    MOULDKG_BINDTX=0,
    MOULDKG_BINDRX,
    MOULDKG_DATA,
};

static void __attribute__((unused)) MOULDKG_send_packet()
{
	memcpy(&packet[1],rx_tx_addr,3);
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xC0;
		memset(&packet[4], 0x00, 3);
	}
	else
	{
		XN297_RFChannel(hopping_frequency[(packet_count>>1)&0x03]);

		uint8_t val=0;
		if(packet_count&1)
		{
			packet[0] = 0x31;
			//Button B
			if(Channel_data[CH2]>CHANNEL_MAX_COMMAND) val |= 0x40;
			else if(Channel_data[CH2]<CHANNEL_MIN_COMMAND) val |= 0x80;
			//Button C
			if(Channel_data[CH3]>CHANNEL_MAX_COMMAND) val |= 0x10;
			else if(Channel_data[CH3]<CHANNEL_MIN_COMMAND) val |= 0x20;
		}
		else
		{
			packet[0] = 0x30;
			val = 0x60
				| GET_FLAG(CH5_SW, 0x80)	// Button E
				| GET_FLAG(CH6_SW, 0x10);	// Button F
		}
		//Button A
		if(Channel_data[CH1]>CHANNEL_MAX_COMMAND) val |= 0x01;
		else if(Channel_data[CH1]<CHANNEL_MIN_COMMAND) val |= 0x02;
		//Button D
		if(Channel_data[CH4]>CHANNEL_MAX_COMMAND) val |= 0x04;
		else if(Channel_data[CH4]<CHANNEL_MIN_COMMAND) val |= 0x08;
		packet[4]= val;

		packet_count++;
	}

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, IS_BIND_IN_PROGRESS?MOULDKG_BIND_PAYLOAD_SIZE:MOULDKG_PAYLOAD_SIZE);
	#if 0
		uint8_t len = IS_BIND_IN_PROGRESS?MOULDKG_BIND_PAYLOAD_SIZE:MOULDKG_PAYLOAD_SIZE;
		for(uint8_t i=0; i < len; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) MOULDKG_initialize_txid()
{
	rx_tx_addr[0]=rx_tx_addr[3];	// Use RX_num;
	#ifdef FORCE_MOULDKG_ORIGINAL_ID
		rx_tx_addr[0]=0x57;
		rx_tx_addr[1]=0x1B;
		rx_tx_addr[2]=0xF8;
	#endif
	//Are the frequencies constant??? If not where are they coming from???
	memcpy(hopping_frequency,(uint8_t*)"\x0F\x1C\x39\x3C", MOULDKG_RF_NUM_CHANNELS);	// 15,28,57,60
}

static void __attribute__((unused)) MOULDKG_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"KDH", 3);
	XN297_SetRXAddr((uint8_t*)"KDH", MOULDKG_BIND_PAYLOAD_SIZE);
}

uint16_t MOULDKG_callback()
{
	switch(phase)
	{
		case MOULDKG_BINDTX:
			if(XN297_IsRX())
			{
				//Example:	TX: C=11 S=Y A= 4B 44 48 P(7)= C0 37 02 4F 00 00 00
				//			RX: C=76 S=Y A= 4B 44 48 P(7)= 5A 37 02 4F 03 0D 8E
				XN297_ReadPayload(packet_in, MOULDKG_BIND_PAYLOAD_SIZE);
				for(uint8_t i=0; i < MOULDKG_BIND_PAYLOAD_SIZE; i++)
					debug("%02X ", packet_in[i]);
				debugln();
				//Not sure if I should test packet_in[0]
				if(memcmp(&packet_in[1],&packet[1],3)==0)
				{//TX ID match, use RX ID to transmit normal packets
					XN297_SetTXAddr(&packet_in[4], 3);
					XN297_SetTxRxMode(TXRX_OFF);
					BIND_DONE;
				}
			}
			XN297_RFChannel(MOULDKG_TX_BIND_CHANNEL);	// Set TX bind channel
			XN297_SetTxRxMode(TXRX_OFF);
			MOULDKG_send_packet();
			phase++;
			return 500;
		case MOULDKG_BINDRX:
			//Wait for the packet transmission to finish
			while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_RFChannel(MOULDKG_RX_BIND_CHANNEL);	// Set RX bind channel
			XN297_SetTxRxMode(RX_EN);
			phase = MOULDKG_BINDTX;
			return MOULDKG_BIND_PACKET_PERIOD-500;
		case MOULDKG_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(MOULDKG_PACKET_PERIOD);
			#endif
			MOULDKG_send_packet();
			break;
	}
	return MOULDKG_PACKET_PERIOD;
}

void MOULDKG_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	MOULDKG_initialize_txid();
	MOULDKG_RF_init();
	bind_counter = MOULDKG_BIND_COUNT;
	packet_count = 0;
}

#endif
