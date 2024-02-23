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
#if defined(EAZYRC_NRF24L01_INO)

#include "iface_xn297.h"

#define FORCE_EAZYRC_ORIGINAL_ID

#define EAZYRC_PAYLOAD_SIZE			10
#define EAZYRC_RF_NUM_CHANNELS		4
#define EAZYRC_BIND_CHANNEL			18
#define EAZYRC_PACKET_PERIOD		5000

enum {
	EAZYRC_BINDTX=0,
	EAZYRC_BINDRX,
	EAZYRC_DATA,
};

static void __attribute__((unused)) EAZYRC_send_packet()
{
	//Bind:
	// TX:    C=18 S=Y A= AA BB CC DD EE P(10)= 1A A0 01 00 00 00 1E 00 78 51
	//   packet[0..2]=tx_addr, packet[6]=first rf channel, packet[8]=unk, packet[9]=sum(packet[0..8])
	// RX:    C=18 S=Y A= AA BB CC DD EE P(10)= 41 AD 01 1A A0 01 1E 00 87 4F
	//   packet[0..2]=rx_addr, packet[3..5]=tx_addr, packet[6]=first rf channel, packet[8]=unk but swapped, packet[9]=sum(packet[0..8])
	//Normal: C=30 S=Y A= 1A A0 41 AD 02 P(10)= 7F 7F 1F 19 00 00 1E 00 AB FF
	//   packet[0]=THR, packet[1]=ST, packet[2]=unk, packet[3]=unk, packet[6]=first rf channel, packet[8]=unk, packet[9]=sum(packet[0..8])
	//Bound : C=18 S=Y A= AA BB CC DD EE P(10)= 1A A0 01 41 AD 01 1E 00 79 41
	//   packet[0..2]=tx_addr, packet[3..5]=rx_addr, packet[6]=first rf channel, packet[8]=unk, packet[9]=sum(packet[0..8])
	//   sent every 12 packets in normal mode
	//Packet period around 5ms with a large jitter

	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(&packet,rx_tx_addr,3);
		memset(&packet[3], 0x00, 5);
		packet[6] = hopping_frequency[0];
		packet[8] = 0x78;						//??? packet type?
	}
	else
	{
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= 3;
		
		packet[0] = convert_channel_8b(THROTTLE);
		packet[1] = convert_channel_8b(AILERON);
		packet[2] = 0x1F;						//??? additional channel?
		packet[3] = 0x19;						//??? additional channel?
		memset(&packet[4], 0x00, 4);
		packet[6] = hopping_frequency[0];
		packet[8] = 0xAB;						//??? packet type?
	}

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, EAZYRC_PAYLOAD_SIZE);
	#ifdef DEBUG_SERIAL
		for(uint8_t i=0; i < len; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) EAZYRC_initialize_txid()
{
	rx_tx_addr[1] = rx_tx_addr[3];
	hopping_frequency[0] = (rx_tx_addr[3]%0x1F) + 0x1E;		// Wild guess...
	#ifdef FORCE_EAZYRC_ORIGINAL_ID
		rx_tx_addr[0] = 0x1A;
		rx_tx_addr[1] = 0xA0;
		rx_tx_addr[2] = 0x01;
		hopping_frequency[0] = 0x1E;
	#endif
	rx_tx_addr[2] = 0x01;									// Not sure if this is needed...
	for(uint8_t i=1; i<EAZYRC_RF_NUM_CHANNELS; i++)
		hopping_frequency[i] = hopping_frequency[0] + 10*i;
}

static void __attribute__((unused)) EAZYRC_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\xAA\xBB\xCC\xDD\xEE", 5);
	XN297_SetRXAddr((uint8_t*)"\xAA\xBB\xCC\xDD\xEE", EAZYRC_PAYLOAD_SIZE);
	XN297_RFChannel(EAZYRC_BIND_CHANNEL);
}

uint16_t EAZYRC_callback()
{
	uint8_t rf,n;
	uint16_t addr;
	switch(phase)
	{
		case EAZYRC_BINDTX:
			if(XN297_IsRX())
			{
				//Example:	TX: C=18 S=Y A= AA BB CC DD EE P(10)= 1A A0 01 00 00 00 1E 00 78 51
				// packet[0..2]=tx_addr, packet[6]=first rf channel, packet[8]=unk, packet[9]=sum(packet[0..8])
				//			RX: C=18 S=Y A= AA BB CC DD EE P(10)= 41 AD 01 1A A0 01 1E 00 87 4F
				// packet[0..2]=rx_addr, packet[3..5]=tx_addr, packet[6]=first rf channel, packet[8]=unk but swapped, packet[9]=sum(packet[0..8])
				XN297_ReadPayload(packet_in, EAZYRC_PAYLOAD_SIZE);
				#ifdef DEBUG_SERIAL
					for(uint8_t i=0; i < EAZYRC_PAYLOAD_SIZE; i++)
						debug("%02X ", packet_in[i]);
					debugln();
				#endif
				//could check the validity of the packet by looking at the sum...
				if(memcmp(&packet_in[3],&rx_tx_addr,3)==0)
				{//TX ID match, TX addr to use: 1A A0 41 AD 02
					rx_tx_addr[4] = rx_tx_addr[2] + packet_in[2];	//wild guess
					rx_tx_addr[2] = packet_in[0];
					rx_tx_addr[3] = packet_in[1];
					BIND_DONE;
					XN297_SetTxRxMode(TXRX_OFF);
					XN297_SetTXAddr(rx_tx_addr, 5);
					phase = EAZYRC_DATA;
					return 5000;
				}
			}
			XN297_SetTxRxMode(TXRX_OFF);
			EAZYRC_send_packet();
			phase++;
			return 1000;
		case EAZYRC_BINDRX:
			//Wait for the packet transmission to finish
			while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = EAZYRC_BINDTX;
			return 10000;
		case EAZYRC_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(EAZYRC_PACKET_PERIOD);
			#endif
			EAZYRC_send_packet();
			break;
	}
	return EAZYRC_PACKET_PERIOD;
}

void EAZYRC_init()
{
	BIND_IN_PROGRESS;
	EAZYRC_initialize_txid();
	EAZYRC_RF_init();
	phase = EAZYRC_BINDTX;
	packet_count = 0;
	hopping_frequency_no = 0;
}

#endif
