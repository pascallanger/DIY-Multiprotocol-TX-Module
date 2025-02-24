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
#if defined(SHENQI2_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_SHENQI2_ORIGINAL_ID

#define SHENQI2_PAYLOAD_SIZE		8
#define SHENQI2_RF_NUM_CHANNELS		16
#define SHENQI2_BIND_COUNT			2000
#define SHENQI2_WRITE_TIME			650
#define SHENQI2_BIND_CHANNEL		44
#define SHENQI2_PACKET_PERIOD		8210


enum {
	SHENQI2_BIND = 0,
	SHENQI2_BIND_RX,
	SHENQI2_DATA,
};

static void __attribute__((unused)) SHENQI2_send_packet()
{
	if(bind_counter)
	{
		bind_counter--;
		if(!bind_counter)
			BIND_DONE;
	}

	memset(packet, 0x00, SHENQI2_PAYLOAD_SIZE);

	packet_count &= 0x0F;
	packet[0] = packet_count;
	
	memcpy(&packet[1],rx_tx_addr,5);

	if(IS_BIND_DONE)
	{//Normal
		uint8_t val = convert_channel_8b(CH2);
		if(val < 0x70)
			val = 0x30;
		else if(val < 0x80)
			val = 0x00;
		else
		{
			val &= 0x7F;
			val >>= 2;
		}
		if(Channel_data[CH1] > 1024+50)
			val |= 0x40;
		else if(Channel_data[CH1] < 1024-50)
			val |= 0x80;
		packet[6] = val;
		//packet[7] = 0x00;		// ??
	}
	else
		packet[0] |= 0x30;

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, SHENQI2_PAYLOAD_SIZE, false);
	#ifdef DEBUG_SERIAL
		for(uint8_t i=0; i < SHENQI2_PAYLOAD_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) SHENQI2_initialize_txid()
{
	#ifdef FORCE_SHENQI2_ORIGINAL_ID
		//TXID
		rx_tx_addr[0] = 0x51;
		rx_tx_addr[1] = 0x70;
		rx_tx_addr[2] = 0x02;
		//RXID
		rx_tx_addr[3] = 0x46;
		rx_tx_addr[4] = 0xBE;
	#endif
	rx_tx_addr[3] = 0x00;
	rx_tx_addr[4] = 0x00;
	//Freq
	memcpy(hopping_frequency,(uint8_t*)"\x05\x09\x0E\x0F\x17\x1C\x21\x27\x2A\x2C\x33\x39\x3D\x42\x48\x4C",16);
}

static void __attribute__((unused)) SHENQI2_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	//Address
	XN297_SetTXAddr((uint8_t*)"\x74\xD1\x3A\xF5\x6C", 5);
	XN297_SetRXAddr((uint8_t*)"\x74\xD1\x3A\xF5\x6C", SHENQI2_PAYLOAD_SIZE);
	XN297_RFChannel(SHENQI2_BIND_CHANNEL);
}

uint16_t SHENQI2_callback()
{
	static bool rx=false;
	
	switch(phase)
	{
		case SHENQI2_BIND:
			rx = XN297_IsRX();
			XN297_SetTxRxMode(TXRX_OFF);
			SHENQI2_send_packet();
			packet_count++;
			if(rx)
			{
				uint8_t val=XN297_ReadEnhancedPayload(packet_in, SHENQI2_PAYLOAD_SIZE);
				if(val == SHENQI2_PAYLOAD_SIZE)
				{
					if(memcmp(rx_tx_addr, packet_in, 3) == 0)
					{//Good packet with our TXID
						BIND_DONE;
						rx_tx_addr[3] = packet_in[3];
						rx_tx_addr[4] = packet_in[4];
						packet_count = 0;
						phase = SHENQI2_DATA;
					}
					#ifdef DEBUG_SERIAL
						for(uint8_t i=0; i < SHENQI2_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
						debugln();
					#endif
				}
			}
			phase++;
			return SHENQI2_WRITE_TIME;
		case SHENQI2_BIND_RX:
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = SHENQI2_BIND;
			return SHENQI2_PACKET_PERIOD - SHENQI2_WRITE_TIME;
		default: //SHENQI2_DATA
			//Since I don't know the order of the channels, I'm hopping on all the channels quickly
			//Refresh rate from the motorcycle perspective is 32ms instead of 8ms...
			XN297_Hopping(hopping_frequency_no);
			hopping_frequency_no++;
			hopping_frequency_no &= 0x0F;
			SHENQI2_send_packet();
			if(hopping_frequency_no%4 == 0)
				packet_count++;
			return SHENQI2_PACKET_PERIOD/4;
	}
	return 0;
}

void SHENQI2_init()
{
	BIND_IN_PROGRESS;
	SHENQI2_initialize_txid();
	SHENQI2_RF_init();

	bind_counter = SHENQI2_BIND_COUNT;
	phase = SHENQI2_BIND;
	hopping_frequency_no = 0;
}

#endif
/*
XN297 1Mb Enhanced,Acked,Scrambled

Bind
---
RX on channel: 44, Time:  2890us P: 34 51 70 02 00 00 00 00
RX on channel: 44, Time:  1780us P: 34 51 70 02 00 00 00 00
RX on channel: 44, Time:  1773us P: 34 51 70 02 00 00 00 00
RX on channel: 44, Time:  1772us P: 34 51 70 02 00 00 00 00
RX on channel: 44, Time:  2889us P: 35 51 70 02 00 00 00 00
RX on channel: 44, Time:  1769us P: 35 51 70 02 00 00 00 00
RX on channel: 44, Time:  1774us P: 35 51 70 02 00 00 00 00
RX on channel: 44, Time:  1771us P: 35 51 70 02 00 00 00 00
RX on channel: 44, Time:  2894us P: 36 51 70 02 00 00 00 00

A= 74 D1 3A F5 6C
RF:44
Timing: 1772µs between the same 4 packets, 2892µs to the next packet, 8208µs between 2 packets
Request ack, if no ack repeat the packet 4 times

P[0] = counter 00..0F | 30 bind
P[1] = TXID[0]
P[2] = TXID[1]
P[3] = TXID[2]
P[4] = RXID[0]
P[5] = RXID[1]
P[6] = TH 00..1F, Break 30, 40 ST_right, 80 ST_left
P[7] = 00?

Answer from motorcycle:
P: 51 70 02 46 BE 00 00 00
P[0] = TXID[0]
P[1] = TXID[1]
P[2] = TXID[2]
P[3] = RXID[0]
P[4] = RXID[1]
P[5] = 00
P[6] = 00
P[7] = 00

Normal packets
---
A= 74 D1 3A F5 6C
RF:5,9,14,15,23,28,33,39,42,44,51,57,61,66,72,76
RF:\x05\x09\x0E\x0F\x17\x1C\x21\x27\x2A\x2C\x33\x39\x3D\x42\x48\x4C
- order of the channels is unknown and vary over time
- send 16 times on each channel and switch (counter 00..0F)
Timing:1772µs between the same 4 packets, 2892µs to the next packet, 8208µs between 2 packets
Timing:8208 between packets if acked
P: 00 51 70 02 46 BE 00 00
P[0] = counter 00..0F
P[1] = TXID[0]
P[2] = TXID[1]
P[3] = TXID[2]
P[4] = RXID[0]
P[5] = RXID[1]
P[6] = TH 00..1F, Break 30, 40 ST_right, 80 ST_left
P[7] = 00?
*/