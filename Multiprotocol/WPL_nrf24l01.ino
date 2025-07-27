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
// Compatible with WPL "Basic" TX models D12, D12KM, D22, D32, D42, D14

#if defined(WPL_NRF24L01_INO)

#include "iface_xn297.h"

#define FORCE_WPL_ORIGINAL_ID

#define WPL_PACKET_PERIOD   9875
#define WPL_RF_NUM_CHANNELS 4
#define WPL_PAYLOAD_SIZE    16
#define WPL_BIND_COUNT		303	//3sec

static void __attribute__((unused)) WPL_send_packet()
{
	#if 0
		debug("no:%d, rf:%d, ",hopping_frequency_no + (IS_BIND_IN_PROGRESS?0:4),hopping_frequency[hopping_frequency_no + (IS_BIND_IN_PROGRESS?0:4)]);
	#endif
	XN297_Hopping(hopping_frequency_no + (IS_BIND_IN_PROGRESS?0:4) );
	hopping_frequency_no++;
	hopping_frequency_no &= WPL_RF_NUM_CHANNELS-1;	// 4 RF channels

	memset(&packet[8],0,7);
	packet[0] = 0x94;											//??
	packet[1] = 0x16;											//??
	packet[2] = 0xCC;											//??
	
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(&packet[3],rx_tx_addr,5);
		packet[9] = 0x08;										// ?? Not bound + Headlights on
	}
	else
	{
		packet[3 ] = convert_channel_s8b(CH1);					// Throttle
		packet[4 ] = convert_channel_s8b(CH2);					// Steering
		packet[5 ] = convert_channel_16b_limit(CH3,0x22,0x5E);	// Steering trim
		packet[6 ] = rx_tx_addr[3];								// 0x32??
		packet[7 ] = 0x80; //convert_channel_s8b(CH4);			// Aux
		packet[9 ] = 0x80										// ?? Bound
				   | GET_FLAG(CH5_SW, 0x08)						// Headlights 100%=on
				   | GET_FLAG(CH6_SW, 0x04)						// Throttle rate 100%=high
				   | GET_FLAG(CH7_SW, 0x02);					// Steering rate 100%=high
	}
	uint8_t sum = 0x66;
	for(uint8_t i=0;i<WPL_PAYLOAD_SIZE-1;i++)
		sum += packet[i];
	packet[WPL_PAYLOAD_SIZE-1] = sum;
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, WPL_PAYLOAD_SIZE);
	#if 0
		for(uint8_t i=0; i<WPL_PAYLOAD_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
}

static void __attribute__((unused)) WPL_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\x69\xA5\x37\x4D\x8B", 5);
	XN297_HoppingCalib(WPL_RF_NUM_CHANNELS*2);	// Calibrate bind and normal channels
}

static void __attribute__((unused)) WPL_initialize_txid()
{
	//Bind frequencies
	memcpy(hopping_frequency  ,"\x17\x25\x46\x36", WPL_RF_NUM_CHANNELS);	//23=17, 37=25, 70=46, 54=36
	//Normal frequencies
	memcpy(hopping_frequency+4,"\x0C\x2A\x3D\x1D", WPL_RF_NUM_CHANNELS);	//12=0C, 42=2A, 61=3D, 29=1D
	#ifdef FORCE_WPL_ORIGINAL_ID
		memcpy(rx_tx_addr,"\x96\x2A\xA9\x32\xB4",5);
	#endif
}

uint16_t WPL_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(WPL_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
			BIND_DONE;
	WPL_send_packet();
	return WPL_PACKET_PERIOD;
}

void WPL_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	WPL_initialize_txid();
	WPL_RF_init();
	hopping_frequency_no = 0;
	bind_counter=WPL_BIND_COUNT;
}

#endif
/* https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/issues/1120
Bind packet
-----------
XN297 1Mb Scrambled
Bind address: 69 A5 37 4D 8B
RF channels: 23, 37, 70, 54
Timing: 9875µs
Payload 16 bytes: 94 16 CC 96 2A A9 32 B4 00 08 00 00 00 00 00 33

P[0] = 94 ??
P[1] = 16 ??
P[2] = CC ??
P[3..7] = Normal address
P[8] = 00 ??
P[9] = 08 ?? not bound?, Throttle and Steering rate low, Headlights on
P[10..14] = 00 ??
P[15] = sum(P[0..14])+66 why 66...

Normal packet
-----------
XN297 1Mb Scrambled
Normal address: 96 2A A9 32 B4
RF channels: 12=0C, 42=2A, 61=3D, 29=1D -> no idea where they come from...
Timing: 9875µs
Payload 16 bytes: 94 16 CC 80 80 38 32 80 00 88 00 00 00 00 00 4E
P[0] = 94 ??
P[1] = 16 ??
P[2] = CC ??
P[3] = Throttle, not enough data on dumps... Same coding as Steering?
P[4] = Steering, not enough data on dumps, looks like one side goes from 7F to 00 and the other 80 to FF which would be s8b
P[5] = Steering trim 22..5E, mid gives 40 not 38... Was the trim centered on the other dumps with value 38?
P[6] = 32 ?? Left over from the bind packet TX_ADDR[3]?
P[7] = 80 ?? Additional channel? It moves at the same time as the trim but my guess is that it is an unconnected channel.
P[8] = 00 ??
P[9] = 80 ?? bound?, Throttle and Steering rate low, Headlights off
      |02 -> Steering rate high
      |04 -> Throttle rate high
      |08 -> Headlights on
P[10..14] = 00 ??
P[15] = sum(P[0..14])+66 why 66...
*/
