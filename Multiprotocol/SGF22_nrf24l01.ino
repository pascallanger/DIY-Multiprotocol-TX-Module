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
// Compatible with SGF22 R11

#if defined(SGF22_NRF24L01_INO)

#include "iface_xn297.h"

#define FORCE_SGF22_ORIGINAL_ID

#define SGF22_PACKET_PERIOD		11950 //10240
#define SGF22_BIND_RF_CHANNEL	78
#define SGF22_PAYLOAD_SIZE		12
#define SGF22_BIND_COUNT		50
#define SGF22_RF_NUM_CHANNELS	4

//packet[8]
#define SGF22_FLAG_3D			0x00
#define SGF22_FLAG_ROLL			0x08
#define SGF22_FLAG_LIGHT		0x04
#define SGF22_FLAG_VIDEO		0x10
#define SGF22_FLAG_6G			0x40
#define SGF22_FLAG_VERTICAL		0xC0
//packet[9]
#define SGF22_FLAG_PHOTO		0x40

static void __attribute__((unused)) SGF22_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		packet[ 0] = 0x5B;
		packet[ 8] = 0x00;								// ??? do they have to be 0 for bind to succeed ?
		packet[ 9] = 0x00;								// ??? do they have to be 0 for bind to succeed ?
		packet[10] = 0xAA;
		packet[11] = 0x55;
	}
	else
	{
		//hop
		XN297_Hopping(packet_sent & 0x03);				// ??? from the dumps I can't really say how hop and seq are sync, there could be an offset (0,1,2,3)...
		if( (packet_sent & 0x03) == 0x02)
			packet_count = packet_sent;
		packet_sent++;
		if(packet_sent > 0x7B)
			packet_sent = 0;
		//packet
		packet[0] = 0x1B;
		packet[8] = SGF22_FLAG_3D						// default
				| GET_FLAG(CH6_SW, SGF22_FLAG_ROLL)		// roll
				| GET_FLAG(CH7_SW, SGF22_FLAG_LIGHT)	// push up throttle trim for light
				| GET_FLAG(CH9_SW, SGF22_FLAG_VIDEO);   // push down throttle trim for video
		if(Channel_data[CH5] > CHANNEL_MIN_COMMAND)
			packet[8] |= SGF22_FLAG_6G;					// mode 1 - 6g
		if(Channel_data[CH5] > CHANNEL_MAX_COMMAND)
			packet[8] |= SGF22_FLAG_VERTICAL;			// mode 0 - vertical
		packet[9] = GET_FLAG(CH8_SW, SGF22_FLAG_PHOTO);	// press in throttle trim for photo
		packet[10] = 0x42;								// no fine tune
		packet[11] = 0x10;								// no fine tune
	}
	packet[1] = packet_count;							// sequence
	packet[2] = rx_tx_addr[2];
	packet[3] = rx_tx_addr[3];
	packet[4] = convert_channel_8b(THROTTLE);
	packet[5] = convert_channel_8b(RUDDER);
	packet[6] = convert_channel_8b(ELEVATOR);
	packet[7] = convert_channel_8b(AILERON);

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, SGF22_PAYLOAD_SIZE,0);
	#if 0
		debug_time("");
		for(uint8_t i=0; i<SGF22_PAYLOAD_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
}

static void __attribute__((unused)) SGF22_initialize_txid()
{
	#ifdef FORCE_SGF22_ORIGINAL_ID
		rx_tx_addr[2] = 0x1F;
		rx_tx_addr[3] = 0x61;
		memcpy(hopping_frequency,"\x15\x34\x24\x44", SGF22_RF_NUM_CHANNELS);    //Original dump=>21=0x15,52=0x34,36=0x24,68=0x44
	#endif
}

static void __attribute__((unused)) SGF22_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\xC7\x95\x3C\xBB\xA5", 5);
	XN297_RFChannel(SGF22_BIND_RF_CHANNEL);			// Set bind channel
}

uint16_t SGF22_callback()
{
	if(phase == 0)
	{
		phase++;
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(SGF22_PACKET_PERIOD);
		#endif
		SGF22_send_packet();
		if(IS_BIND_IN_PROGRESS)
		{
			if(--bind_counter==0)
				BIND_DONE;
		}
	}
	else
	{//send 3 times in total the same packet
		NRF24L01_Strobe(REUSE_TX_PL);
		phase++;
		if(phase > 2)
		{
			phase = 0;
			return SGF22_PACKET_PERIOD - 2*1550;
		}
	}
	return 1550;
}

void SGF22_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	SGF22_initialize_txid();
	SGF22_RF_init();
	bind_counter=SGF22_BIND_COUNT;
	packet_sent = packet_count = 0x26;
	phase = 0;
}

#endif
