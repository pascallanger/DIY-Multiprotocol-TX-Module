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

#if defined(JOYSWAY_A7105_INO)

#include "iface_a7105.h"

//#define JOYSWAY_FORCE_ID

static void __attribute__((unused)) JOYSWAY_send_packet()
{
	static uint8_t next_ch = 0x30;
	
	//RF frequency
	if (packet_count == 254)
	{
		packet_count = 0;
		A7105_WriteID(0x5475c52a);
		rf_ch_num = 0x0a;
	}
	else if (packet_count == 2)
	{
		A7105_WriteID(MProtocol_id);
		rf_ch_num = 0x30;
	}
	else
	{
		if (packet_count & 0x01)
			rf_ch_num = 0x30;
		else
			rf_ch_num = next_ch;
	}
	if (! (packet_count & 0x01))
	{
		next_ch++;
		if (next_ch >= 0x45)
			next_ch = 0x30;
	}

	//Payload
	packet[0] = packet_count == 0 ? 0xdd : 0xff;
	//ID
	packet[1] = rx_tx_addr[0];
	packet[2] = rx_tx_addr[1];
	packet[3] = rx_tx_addr[2];
	packet[4] = rx_tx_addr[3];
	packet[5] = 0x00;
	//Channels
	for (uint8_t i = 0; i < 4; i++)
		packet[ 6 + (i & 0x01) + ((i & 0x02)<<1)] = convert_channel_16b_limit(i, 0x00, 0xCC);
	packet[8] = 0x64;
	packet[9] = 0x64;
	packet[12] = 0x64;
	packet[13] = 0x64;
	packet[14] = packet_count == 0 ? 0x30 : 0xaa;
	//Check
	uint8_t value = 0;
	for (uint8_t i = 0; i < 15; i++)
		value += packet[i];
	packet[15] = value;

	//Send
	#if 0
		debug("ch=%02X P=",rf_ch_num);
		for(uint8_t i=0; i<16; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif
	A7105_WriteData(16, rf_ch_num);
	A7105_SetPower();
	packet_count++;
}

uint16_t JOYSWAY_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(6000);
	#endif
	#ifndef FORCE_JOYSWAY_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif

	JOYSWAY_send_packet();
	return 6000;
}

void JOYSWAY_init()
{
	BIND_DONE;		// not a bind protocol

	MProtocol_id &= 0x00FFFFFF;
	MProtocol_id |= 0xF8000000;
	#ifdef JOYSWAY_FORCE_ID
		MProtocol_id = 0xf82dcaa0;
	#endif

	set_rx_tx_addr(MProtocol_id);

	A7105_Init();
	
	packet_count = 2;
}
#endif
