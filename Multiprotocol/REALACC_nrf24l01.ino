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
// Compatible with Realacc R11

#if defined(REALACC_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_REALACC_ORIGINAL_ID

#define REALACC_INITIAL_WAIT		500
#define REALACC_PACKET_PERIOD		2268
#define REALACC_BIND_RF_CHANNEL		80
#define REALACC_BIND_PAYLOAD_SIZE	10
#define REALACC_PAYLOAD_SIZE		13
#define REALACC_BIND_COUNT			50
#define REALACC_RF_NUM_CHANNELS		5

static void __attribute__((unused)) REALACC_send_packet()
{
	packet[ 0]= 0xDC;
	packet[ 1]= convert_channel_8b(AILERON);	// 00..80..FF
	packet[ 2]= convert_channel_8b(ELEVATOR);	// 00..80..FF
	packet[ 3]= convert_channel_8b(THROTTLE);	// 00..FF
	packet[ 4]= convert_channel_8b(RUDDER);		// 00..80..FF
	packet[ 5]= 0x20; 							// Trim
	packet[ 6]= 0x20; 							// Trim
	packet[ 7]= 0x20; 							// Trim
	packet[ 8]= 0x20; 							// Trim
	packet[ 9]= num_ch;							// Change at each power up
	packet[10]= 0x04 							// Flag1
		| 0x02									//   Rate1=0, Rate2=1, Rate3=2
		| GET_FLAG(CH8_SW, 0x20);				//   Headless
	packet[11]= 0x00 							// Flag2
		| GET_FLAG(CH7_SW, 0x01)				//   Calib
		| GET_FLAG(CH9_SW, 0x20)				//   Return
		| GET_FLAG(CH10_SW,0x80);				//   Unknown
	packet[12]= 0x00 							// Flag3
		| GET_FLAG(CH5_SW, 0x01)				//   Flip
		| GET_FLAG(CH6_SW, 0x80);				//   Light

	XN297_Hopping(hopping_frequency_no);
	hopping_frequency_no++;
	hopping_frequency_no %= REALACC_RF_NUM_CHANNELS;
	XN297_WriteEnhancedPayload(packet, REALACC_PAYLOAD_SIZE,0);
}

static void __attribute__((unused)) REALACC_send_bind_packet()
{
	packet[0] = 0xB1;
	memcpy(&packet[1],rx_tx_addr,4);
	memcpy(&packet[5],hopping_frequency,5);

	XN297_WriteEnhancedPayload(packet, REALACC_BIND_PAYLOAD_SIZE,1);
}

static void __attribute__((unused)) REALACC_initialize_txid()
{
	calc_fh_channels(REALACC_RF_NUM_CHANNELS);
	num_ch=random(0xfefefefe);		// 00..FF

	#ifdef FORCE_REALACC_ORIGINAL_ID
		//Dump
		rx_tx_addr[0]=0x99;
		rx_tx_addr[1]=0x06;
		rx_tx_addr[2]=0x00;
		rx_tx_addr[3]=0x00;
		hopping_frequency[0]=0x55;
		hopping_frequency[1]=0x59;
		hopping_frequency[2]=0x5A;
		hopping_frequency[3]=0x5A;
		hopping_frequency[4]=0x62;
		num_ch=0xC5;				// Value in dumps: C5 A2 77 F0 84 58
	#endif
}

static void __attribute__((unused)) REALACC_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"MAIN", 4);
	XN297_RFChannel(REALACC_BIND_RF_CHANNEL);	// Set bind channel
}

uint16_t REALACC_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(REALACC_PACKET_PERIOD);
	#endif
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	if(IS_BIND_IN_PROGRESS)
	{
		REALACC_send_bind_packet();
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 4);
		}
	}
	else
		REALACC_send_packet();
	return REALACC_PACKET_PERIOD;
}

void REALACC_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	REALACC_initialize_txid();
	REALACC_RF_init();
	bind_counter=REALACC_BIND_COUNT;
	hopping_frequency_no=0;
}

#endif

// XN297 speed 1Mb, scrambled, enhanced
// Bind
//   Address = 4D 41 49 4E = 'MAIN'
//   Channel = 80 (most likely from dump)
//   P(10) = B1 99 06 00 00 55 59 5A 5A 62
//     B1 indicates bind packet
//     99 06 00 00 = ID = address of normal packets
//     55 59 5A 5A 62 = 85, 89, 90, 90, 98 = RF channels to be used (kind of match previous dumps)// Normal
// Normal
//   Address = 99 06 00 00
//   Channels = 84, 89, 90, 90, 98 (guess from bind)
//   P(13)= DC 80 80 32 80 20 20 20 20 58 04 00 00
//   DC = normal packet
//   80 80 32 80 : AETR 00..80..FF
//   20 20 20 20 : Trims
//   58 : changing every time the TX restart
//   04 : |0x20=headless, |0x01=rate2, |0x02=rate3
//   00 : |0x01=calib, |0x20=return, |0x80=unknown
//   00 : |0x80=light, |0x01=flip