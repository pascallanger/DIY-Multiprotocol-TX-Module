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
// Compatible with FZ-410 TX

#if defined(FLYZONE_A7105_INO)

#include "iface_a7105.h"

//#define FLYZONE_FORCEID

#define FLYZONE_BIND_COUNT	220		// 5 sec
#define FLYZONE_BIND_CH		0x18	// TX, RX for bind end is 0x17

static void __attribute__((unused)) flyzone_build_packet()
{
    packet[0] = 0xA5;
    packet[1] = rx_tx_addr[2];
    packet[2] = rx_tx_addr[3];
	packet[3] = convert_channel_8b(AILERON);	//00..80..FF
	packet[4] = convert_channel_8b(ELEVATOR);	//00..80..FF
	packet[5] = convert_channel_8b(THROTTLE);	//00..FF
	packet[6] = convert_channel_8b(RUDDER);		//00..80..FF
    packet[7] = convert_channel_8b(CH5);		//00..80..FF
}

uint16_t ReadFlyzone()
{
	#ifndef FORCE_FLYZONE_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0x1B;
		packet[1] = rx_tx_addr[2];
		packet[2] = rx_tx_addr[3];
		A7105_WriteData(3, FLYZONE_BIND_CH);
		if (bind_counter--==0)
			BIND_DONE;
		return 22700;
	}
	else
	{
		if(phase>19)
		{
			phase=0;
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(20*1500);
			#endif
			flyzone_build_packet();
			A7105_WriteData(8, hopping_frequency[0]);
			A7105_SetPower();
		}
		else
		{
			A7105_WriteReg(A7105_0F_PLL_I, hopping_frequency[(phase&0x02)>>1]);
			A7105_Strobe(A7105_TX);
		}
		phase++;
	}
	return 1500;
}

uint16_t initFlyzone()
{
	A7105_Init();

	hopping_frequency[0]=((random(0xfefefefe) & 0x0F)+2)<<2;
	hopping_frequency[1]=hopping_frequency[0]+0x50;
	
	#ifdef FLYZONE_FORCEID
		rx_tx_addr[2]=0x35;
		rx_tx_addr[3]=0xD0;
		hopping_frequency[0]=0x18;
		hopping_frequency[1]=0x68;
	#endif
	
	phase=255;
	bind_counter = FLYZONE_BIND_COUNT;
	return 2400;
}
#endif
// Normal packet is 8 bytes: 0xA5 0xAF 0x59 0x84 0x7A 0x00 0x80 0xFF
// Protocol is using AETR channel order, 1 byte per channel 00..80..FF including trim. Channels are in packet [3,4,5,6].
// packet[0,1,2,7] values are constant in normal mode.
// packet[0]=0xA5 -> normal mode
// packet[1,2] ->ID
// packet[7]=0xFF -> ???
// Channel values are updated every 30ms which is quite slow, slower than PPM... 
// Packets are sent every 1500µs on 2 different channels. 2 times on first channel, 2 times on second channel and restart. The channels are changing between the files 0x08, 0x58 and 0x18, 0x68.
//
// Bind is sending 3 bytes on channel 0x18: 0x1B 0x35 0xD0 every 22.7ms
// packet[0]=0x1B -> bind mode
// packet[1,2] ->ID
// It listens for the model on channel 0x17 and recieves 0x1B 0x35 0xD0 when the plane accepts bind.