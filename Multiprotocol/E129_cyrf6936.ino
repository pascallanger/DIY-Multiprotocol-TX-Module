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

#if defined(E129_CYRF6936_INO)

#include "iface_rf2500.h"

//#define E129_FORCE_ID

#define E129_BIND_CH		0x2D	//45
#define E129_PAYLOAD_SIZE	16

static void __attribute__((unused)) E129_build_data_packet()
{
	//Build the packet
	memset(packet,0,E129_PAYLOAD_SIZE);
	packet[ 0] = 0x0F;									// Packet length
	if(IS_BIND_IN_PROGRESS)
	{
		packet[ 1] = 0xA4;
		packet[ 2] = bit_reverse(rx_tx_addr[2]);
		packet[ 3] = bit_reverse(rx_tx_addr[3]);
		packet[ 4] = bit_reverse(rx_tx_addr[0]);
		packet[ 5] = bit_reverse(rx_tx_addr[1]);
		for(uint8_t i=0; i<4; i++)
			packet[6+i]=hopping_frequency[i]-2;
	}
	else
	{
		packet[ 1] = 0xA6;
		packet[ 2] = 0xF7;								// High rate 0xF7, low rate 0xF4
		//packet[ 3] = 0x00;							// Mode: short press=0x20->0x00->0x20->..., long press=0x10->0x30->0x10->...
		packet[ 4] = GET_FLAG(CH5_SW, 0x20)				// Take off/Land 0x20
				| GET_FLAG(CH6_SW, 0x04);				// Emergency stop 0x04
		
		uint16_t val = convert_channel_10b(AILERON,false);
		uint8_t trim = convert_channel_8b(CH7) & 0xFC;
		packet[ 5] = trim | (val >>8);					// Trim (0x00..0x1F..0x3E) << 2 | channel >> 8
		packet[ 6] = val;								// channel (0x000...0x200...0x3FF)
		val = convert_channel_10b(ELEVATOR,false);
		trim = convert_channel_8b(CH8) & 0xFC;
		packet[ 7] = trim | (val >>8);					// Trim (0x00..0x1F..0x3E) << 2 | channel >> 8
		packet[ 8] = val;								// channel (0x000...0x200...0x3FF)
		if(packet_count>200)
			val = convert_channel_10b(THROTTLE,false);
		else
		{//Allow bind to complete with throttle not centered
			packet_count++;
			val=0x200;
		}
		packet[ 9] = (0x1F<<2) | (val >>8);				// Trim (0x00..0x1F..0x3E) << 2 | channel >> 8
		packet[10] = val;								// channel (0x000...0x200...0x3FF)
		val = convert_channel_10b(RUDDER,false);
		trim = convert_channel_8b(CH9) & 0xFC;
		packet[11] = trim | (val >>8);					// Trim (0x00..0x1F..0x3E) << 2 | channel >> 8
		packet[12] = val;								// channel (0x000...0x200...0x3FF)
	}
	packet[14] = 0x00;									// Check
	for(uint8_t i=0;i<14;i++)
		packet[14] += packet[i];
	
	RF2500_BuildPayload(packet);
}

uint16_t E129_callback()
{
	//Set RF channel
	if(phase==0)
		RF2500_RFChannel(IS_BIND_IN_PROGRESS ? E129_BIND_CH : hopping_frequency[hopping_frequency_no]);

	//Send packet
	RF2500_SendPayload();
	
	//Send twice on same channel
	if(phase==0)
	{
		phase++;
		return 1260;
	}

	//Bind
	if(bind_counter)
		if(--bind_counter==0)
		{
			BIND_DONE;
			RF2500_SetTXAddr(rx_tx_addr);	// 4 bytes of address
		}

	//Build packet
	E129_build_data_packet();
	
	//Set power
	RF2500_SetPower();
	
	//Hopp
	hopping_frequency_no++;
	hopping_frequency_no &= 3;

	phase=0;
	return 5200-1260;
}

void E129_init()
{
	BIND_IN_PROGRESS;						// Autobind protocol
    bind_counter = 384;						// ~2sec

    //RF2500 emu init
	RF2500_Init(E129_PAYLOAD_SIZE, true);	// 16 bytes, Scrambled
   
	//Freq hopping
	calc_fh_channels(4);
	for(uint8_t i=0; i<4; i++)
		if(hopping_frequency[i]==E129_BIND_CH)
			hopping_frequency[i]++;
	
    #ifdef E129_FORCE_ID
        rx_tx_addr[0]=0xC1;
        rx_tx_addr[1]=0x22;
        rx_tx_addr[2]=0x05;
        rx_tx_addr[3]=0xA3;
        hopping_frequency[0]=0x3C;			//60
        hopping_frequency[1]=0x49;			//73
        hopping_frequency[2]=0x4B;			//75
        hopping_frequency[3]=0x41;			//65
    #endif

	RF2500_SetTXAddr((uint8_t*)"\xE2\x32\xE0\xC8");	// 4 bytes of bind address

	E129_build_data_packet();
	hopping_frequency_no=0;
	packet_count=0;
	phase=0;
}

#endif
