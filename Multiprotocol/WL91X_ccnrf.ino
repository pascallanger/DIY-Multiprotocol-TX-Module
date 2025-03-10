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

#if defined(WL91X_CCNRF_INO)

#include "iface_xn297.h"

//#define FORCE_WL91X_ORIGINAL_ID

#define WL91X_PAYLOAD_SIZE		9
#define WL91X_RF_NUM_CHANNELS	3
#define WL91X_PACKET_PERIOD		2594

static void __attribute__((unused)) WL91X_send_packet()
{
	uint8_t val;

	//RF freq
	XN297_Hopping(hopping_frequency_no++);
	hopping_frequency_no %= WL91X_RF_NUM_CHANNELS;

	//Sticks
	val = convert_channel_16b_limit(CH2,0x21,0xE0);			//THR forward 00..5F, backward 80..DF
	if(val < 128) val = 127 - val;
	packet[0] = val - 0x80;
	val = convert_channel_s8b(CH1);							//ST right 00..7F, left 80..FF
	packet[1] = val - 0x80;
	//Trims
	val = convert_channel_s8b(CH3);							//ST_Trim centered=80, increment/decrement=4, right 04..7C, left 84..FC
	packet[2] = val - 0x80;
	packet[3] = convert_channel_16b_limit(CH4,0x00,0x70);	//TH_Trim increment/decrement=3, 00..39..6F
	//TX_ID
	memcpy(&packet[4], rx_tx_addr, 4);
	//Checksum
	val = 0;
	for(uint8_t i=0; i<WL91X_PAYLOAD_SIZE-1; i++)
		val += packet[i];
	packet[8] = val;

	//Send
	XN297_SetPower();
	XN297_SetFreqOffset();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, WL91X_PAYLOAD_SIZE);
	#ifdef DEBUG_SERIAL
		for(uint8_t i=0; i < WL91X_PAYLOAD_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) WL91X_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_UNSCRAMBLED, XN297_250K);
	XN297_HoppingCalib(WL91X_RF_NUM_CHANNELS);
	XN297_SetTXAddr((uint8_t*)"\x46\x14\x7B\x08", 4);
}

static void __attribute__((unused)) WL91X_initialize_txid()
{
	#ifdef FORCE_WL91X_ORIGINAL_ID
		memcpy(rx_tx_addr, (uint8_t*)"\x00\x1E\x33\x02",4);
	#endif
	memcpy(hopping_frequency, (uint8_t*)"\x1A\x3B\x3B",3);	//26,59,59
}

uint16_t WL91X_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(WL91X_PACKET_PERIOD);
	#endif
	WL91X_send_packet();
	return WL91X_PACKET_PERIOD;
}

void WL91X_init()
{
	BIND_DONE;	//No bind for this protocol
	WL91X_initialize_txid();
	WL91X_RF_init();
	hopping_frequency_no = 0;
}

#endif
