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
// Compatible with JJRC ZSX-280 plane.

#if defined(ZSX_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_ZSX_ORIGINAL_ID

#define ZSX_INITIAL_WAIT	500
#define ZSX_PACKET_PERIOD	10093
#define ZSX_RF_BIND_CHANNEL	7
#define ZSX_PAYLOAD_SIZE	6
#define ZSX_BIND_COUNT		50
#define ZSX_RF_NUM_CHANNELS	1

static void __attribute__((unused)) ZSX_send_packet()
{
	memcpy(&packet[1],rx_tx_addr,3);
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xAA;
		packet[4] = 0x00;
		packet[5] = 0x00;
	}
	else
	{
		packet[0]= 0x55;
		packet[4]= 0xFF-convert_channel_8b(RUDDER);		// FF..80..01
		packet[5]= convert_channel_8b(THROTTLE)>>1		// 0..7F
				| GET_FLAG(CH5_SW, 0x80);				// Light
	}

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, ZSX_PAYLOAD_SIZE);
}

static void __attribute__((unused)) ZSX_initialize_txid()
{
	rx_tx_addr[0]=rx_tx_addr[3];	// Use RX_num;
	#ifdef FORCE_ZSX_ORIGINAL_ID
		//TX1
		rx_tx_addr[0]=0x03;
		rx_tx_addr[1]=0x01;
		rx_tx_addr[2]=0xC3;
	#endif
}

static void __attribute__((unused)) ZSX_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\xc1\xc2\xc3", 3);
	XN297_RFChannel(ZSX_RF_BIND_CHANNEL);	// Set bind channel
}

uint16_t ZSX_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(ZSX_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 3);
			XN297_RFChannel(0x00);
		}
	ZSX_send_packet();
	return ZSX_PACKET_PERIOD;
}

void ZSX_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	ZSX_initialize_txid();
	ZSX_RF_init();
	bind_counter=ZSX_BIND_COUNT;
}

#endif

// XN297 speed 1Mb, scrambled
// Bind
//   channel 7
//   address: C1 C2 C3
//   P(6)= AA 03 01 C3 00 00
//   03 01 C3 <- normal address
// Normal
//   channel 0 and seems to be fixed
//   address: 03 01 C3
//   P(6)= 55 03 01 C3 80 00
//   03 01 C3 <- normal address
//   80 <- rudder FF..80..01
//   00 <- throttle 00..7F, light flag 0x80