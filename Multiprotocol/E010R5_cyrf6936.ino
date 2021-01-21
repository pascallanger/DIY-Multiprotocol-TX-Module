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

#if defined(E010R5_CYRF6936_INO)

#include "iface_rf2500.h"

#define E010R5_FORCE_ID

#define E010R5_BIND_CH		0x2D	//45
#define E010R5_PAYLOAD_SIZE	14


static void __attribute__((unused)) E010R5_build_data_packet()
{
	packet[ 0] = 0x0D;								// Packet length
	packet[ 1] = convert_channel_8b(THROTTLE);
	packet[ 2] = convert_channel_s8b(RUDDER);
	packet[ 3] = convert_channel_s8b(ELEVATOR);
	packet[ 4] = convert_channel_s8b(AILERON);
	packet[ 5] = 0x20;								// Trim Rudder
	packet[ 6] = 0x20;								// Trim Elevator
	packet[ 7] = 0x20;								// Trim Aileron
	packet[ 8] = 0x01								// Flags: high=0x01, low=0x00
			| GET_FLAG(CH5_SW, 0x04)				//        flip=0x04
			| GET_FLAG(CH6_SW, 0x08)				//        led=0x08
			| GET_FLAG(CH8_SW, 0x10)				//		  headless=0x10
			| GET_FLAG(CH9_SW, 0x20);				//		  one key return=0x20
	packet[ 9] = IS_BIND_IN_PROGRESS ? 0x80 : 0x00	// Flags: bind=0x80
			| GET_FLAG(CH7_SW, 0x20)				//        calib=0x20
			| GET_FLAG(CH10_SW, 0x01);				//		  strange effect=0x01=long press on right button
	packet[10] = rx_tx_addr[0];
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2];
	packet[13] = 0x9D;								// Check
	for(uint8_t i=0;i<13;i++)
		packet[13] += packet[i];

	RF2500_BuildPayload(packet);
}

uint16_t ReadE010R5()
{
	//Bind
	if(bind_counter)
	{
		bind_counter--;
		if(bind_counter==0)
			BIND_DONE;
	}

	//Send packet
	RF2500_SendPayload();
	
	//Timing and hopping
	packet_count++;
	switch(packet_count)
	{
		case 1:
		case 2:
		case 4:
		case 5:
			return 1183;
		default:
			hopping_frequency_no++;
			hopping_frequency_no &= 3;
			if(IS_BIND_IN_PROGRESS)
				rf_ch_num = 0x30 + (hopping_frequency_no<<3);
			else
				rf_ch_num = hopping_frequency[hopping_frequency_no];
			RF2500_RFChannel(rf_ch_num);
			RF2500_SetPower();
			packet_count = 0;
		case 3:
			E010R5_build_data_packet();
			return 3400;
	}
	return 0;
}

uint16_t initE010R5()
{
	BIND_IN_PROGRESS;									// Autobind protocol
	bind_counter = 2600;

	//RF2500 emu init
	RF2500_Init(E010R5_PAYLOAD_SIZE, false);			// 14 bytes, not scrambled
	RF2500_SetTXAddr((uint8_t*)"\x0E\x54\x96\xEE");		// Same address for bind and normal packets
	
	#ifdef E010R5_FORCE_ID
		switch(rx_tx_addr[3]%3)
		{
			case 0:
				//TX1
				hopping_frequency[0]=0x35;	//53
				hopping_frequency[1]=0x30;	//48
				rx_tx_addr[1]=0x45;
				rx_tx_addr[2]=0x46;
				break;
			case 1:
				//TX2
				hopping_frequency[0]=0x35;	//53
				hopping_frequency[1]=0x3C;	//60
				rx_tx_addr[1]=0x1B;
				rx_tx_addr[2]=0x9E;
				break;
			default:
				//TX3
				hopping_frequency[0]=0x30;	//48
				hopping_frequency[1]=0x38;	//56
				rx_tx_addr[1]=0x17;
				rx_tx_addr[2]=0x0D;
				break;
		}
    #endif
	rx_tx_addr[0]=0x00;
	// This is the same as the E010 v1...
	hopping_frequency[2]=hopping_frequency[0]+0x10;
	hopping_frequency[3]=hopping_frequency[1]+0x10;

	E010R5_build_data_packet();
	RF2500_RFChannel(hopping_frequency[0]);
	hopping_frequency_no=0;
	packet_count=0;

	return 3400;
}

#endif
