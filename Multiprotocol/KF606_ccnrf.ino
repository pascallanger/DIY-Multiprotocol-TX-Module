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
// Compatible with KF606 plane.

#if defined(KF606_CCNRF_INO)

#include "iface_xn297.h"

//#define FORCE_KF606_ORIGINAL_ID
//#define FORCE_MIG320_ORIGINAL_ID
//#define FORCE_ZCZ50_ORIGINAL_ID

#define KF606_INITIAL_WAIT    500
#define KF606_PACKET_PERIOD   3000
#define KF606_RF_BIND_CHANNEL 7
#define KF606_PAYLOAD_SIZE    4
#define KF606_BIND_COUNT	  857	//3sec
#define KF606_RF_NUM_CHANNELS 2

static void __attribute__((unused)) KF606_send_packet()
{
	uint8_t len = KF606_PAYLOAD_SIZE;
	if(IS_BIND_IN_PROGRESS)
	{
		if(sub_protocol != KF606_ZCZ50)
		{
			packet[0] = 0xAA;
			memcpy(&packet[1],rx_tx_addr,3);
		}
		else
			memcpy(packet,rx_tx_addr,4);
	}
	else
	{
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no ^= 1;			// 2 RF channels

		packet[0] = 0x55;
		if(sub_protocol == KF606_ZCZ50) len--;
		packet[len-3] = convert_channel_8b(THROTTLE);									// 0..255
		// Deadband is needed on aileron, 40 gives +-6%
		packet[len-2] = convert_channel_8b_limit_deadband(AILERON,0x00,0x80,0xFF,40);	// Aileron: High rate:2B..80..DA
		uint8_t p3;
		p3 = convert_channel_8b(CH5)>>3;												// Aileron trim must be on a separated channel 01..10..1F
		p3 += (packet[2]-0x80)>>3;														// Drive trims for more aileron authority
		if(p3 > 0x80)
			p3 = 0x01;
		else if(p3 > 0x1F)
			p3 = 0x1F;
		p3 |= GET_FLAG(CH6_SW, 0xC0);													// 0xC0 and 0xE0 are both turning the LED off, not sure if there is another hidden feature
		packet[len-1] = p3;
	}

	if(sub_protocol == KF606_MIG320)
	{
		len++;
		packet[4] = 0;	// additional channel?
	}

	#if 0
		for(uint8_t i=0; i<len; i++)
			debug("%02X ",packet[i]);
		debugln("");
	#endif

	// Send
	XN297_SetPower();
	XN297_SetFreqOffset();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, len);
}

static void __attribute__((unused)) KF606_initialize_txid()
{
	rx_tx_addr[0]=rx_tx_addr[3];	// Use RX_num;
	hopping_frequency[0]=(rx_tx_addr[0]&0x3F)+9;
	hopping_frequency[1]=hopping_frequency[0]+3;
	#ifdef FORCE_KF606_ORIGINAL_ID
		//TX1
		rx_tx_addr[0]=0x57;
		rx_tx_addr[1]=0x02;
		rx_tx_addr[2]=0x00;
		hopping_frequency[0]=0x20;
		hopping_frequency[1]=0x23;
		//TX2
		rx_tx_addr[0]=0x25;
		rx_tx_addr[1]=0x04;
		rx_tx_addr[2]=0x00;
		hopping_frequency[0]=0x2E;
		hopping_frequency[1]=0x31;
	#endif
	#ifdef FORCE_MIG320_ORIGINAL_ID
		rx_tx_addr[0]=0xBB;
		rx_tx_addr[1]=0x13;
		rx_tx_addr[2]=0x00;
		hopping_frequency[0]=68;
		hopping_frequency[1]=71;
	#endif
	if(sub_protocol == KF606_ZCZ50)
	{
		rx_tx_addr[1] = rx_tx_addr[0];
		rx_tx_addr[0]=0xAA;
	}
	#ifdef FORCE_ZCZ50_ORIGINAL_ID
		rx_tx_addr[0]=0xAA;
		rx_tx_addr[1]=0x67;
		rx_tx_addr[2]=0x64;
		rx_tx_addr[3]=0x01;
		hopping_frequency[0]=48;
		hopping_frequency[1]=51;
	#endif
}

static void __attribute__((unused)) KF606_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	XN297_SetTXAddr((uint8_t*)"\xe7\xe7\xe7\xe7\xe7", 5);
	XN297_HoppingCalib(KF606_RF_NUM_CHANNELS);					// Calibrate all channels
	XN297_RFChannel(KF606_RF_BIND_CHANNEL);						// Set bind channel
}

uint16_t KF606_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(KF606_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, sub_protocol != KF606_ZCZ50 ? 3 : 4);
		}
	KF606_send_packet();
	return KF606_PACKET_PERIOD;
}

void KF606_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	KF606_initialize_txid();
	KF606_RF_init();
	hopping_frequency_no = 0;
	bind_counter=KF606_BIND_COUNT;
}

#endif

// MIG320 protocol
// Bind
// 250K C=7 S=Y A= E7 E7 E7 E7 E7 P(5)= AA BB 13 00 00
// 3ms on ch7
// Normal
// 250K C=68 S=Y A= BB 13 00 P(5)= 55 00 80 10 00
// P[1] = THR 00..FF
// P[2] = AIL 2B..80..DA
// P[3] = TRIM 01..10..1F
// channels 68=BB&3F+9 and 71


// ZCZ50v2 protocol (with fake front propeller)
// Bind
// 250K C=7 S=Y A= E7 E7 E7 E7 E7 P(4)= AA 67 64 01
// 3ms on ch7
// Normal
// 250K C=48 S=Y A= AA 67 64 01 P(3)= 00 80 10
// P[0] = THR 0x00..0xFF
// P[1] = AIL low rate 0x52..0x80..0xB1, high rate: 0x41..0x80..0xC3
// P[2] = TRIM 0x01..0x10..0x1F + UNKNOWN 0x00 or 0xC0
