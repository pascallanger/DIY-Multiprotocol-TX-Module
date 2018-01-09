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
// Last sync with hexfet new_protocols/flysky_a7105.c dated 2015-09-28

#if defined(FLYSKY_A7105_INO)

#include "iface_a7105.h"

//FlySky constants & variables
#define FLYSKY_BIND_COUNT 2500

enum {
	// flags going to byte 10
	FLAG_V9X9_VIDEO = 0x40,
	FLAG_V9X9_CAMERA= 0x80,
	// flags going to byte 12
	FLAG_V9X9_FLIP   = 0x10,
	FLAG_V9X9_LED   = 0x20,
};

enum {
	// flags going to byte 13
	FLAG_V6X6_HLESS1= 0x80,
	// flags going to byte 14
	FLAG_V6X6_VIDEO = 0x01,
	FLAG_V6X6_YCAL  = 0x02,
	FLAG_V6X6_XCAL  = 0x04,
	FLAG_V6X6_RTH   = 0x08,
	FLAG_V6X6_CAMERA= 0x10,
	FLAG_V6X6_HLESS2= 0x20,
	FLAG_V6X6_LED   = 0x40,
	FLAG_V6X6_FLIP  = 0x80,
};

enum {
	// flags going to byte 14
	FLAG_V912_TOPBTN= 0x40,
	FLAG_V912_BTMBTN= 0x80,
};

const uint8_t PROGMEM V912_X17_SEQ[10] =  { 0x14, 0x31, 0x40, 0x49, 0x49,    // sometime first byte is 0x15 ?
											0x49, 0x49, 0x49, 0x49, 0x49, }; 

static void __attribute__((unused)) flysky_apply_extension_flags()
{
	switch(sub_protocol)
	{
		case V9X9:
			if(CH5_SW)
				packet[12] |= FLAG_V9X9_FLIP;
			if(CH6_SW)
				packet[12] |= FLAG_V9X9_LED;
			if(CH7_SW)
				packet[10] |= FLAG_V9X9_CAMERA;
			if(CH8_SW)
				packet[10] |= FLAG_V9X9_VIDEO;
			break;
			
		case V6X6:
			packet[13] = 0x03; // 3 = 100% rate (0=40%, 1=60%, 2=80%)
			packet[14] = 0x00;
			if(CH5_SW) 
				packet[14] |= FLAG_V6X6_FLIP;
			if(CH6_SW) 
				packet[14] |= FLAG_V6X6_LED;
			if(CH7_SW) 
				packet[14] |= FLAG_V6X6_CAMERA;
			if(CH8_SW) 
				packet[14] |= FLAG_V6X6_VIDEO;
			if(CH9_SW)
			{ 
				packet[13] |= FLAG_V6X6_HLESS1;
				packet[14] |= FLAG_V6X6_HLESS2;
			}
			if(CH10_SW)
				packet[14] |= FLAG_V6X6_RTH;
			if(CH11_SW) 
				packet[14] |= FLAG_V6X6_XCAL;
			if(CH12_SW) 
				packet[14] |= FLAG_V6X6_YCAL;
			packet[15] = 0x10; // unknown
			packet[16] = 0x10; // unknown
			packet[17] = 0xAA; // unknown
			packet[18] = 0xAA; // unknown
			packet[19] = 0x60; // unknown, changes at irregular interval in stock TX
			packet[20] = 0x02; // unknown
			break;
			
		case V912:
			packet_count++;
			if( packet_count > 9)
				packet_count = 0;
			packet[12] |= 0x20; // bit 6 is always set ?
			packet[13] = 0x00;  // unknown
			packet[14] = 0x00;
			if(CH5_SW)
				packet[14]  = FLAG_V912_BTMBTN;
			if(CH6_SW)
				packet[14] |= FLAG_V912_TOPBTN;
			packet[15] = 0x27; // [15] and [16] apparently hold an analog channel with a value lower than 1000
			packet[16] = 0x03; // maybe it's there for a pitch channel for a CP copter ?
			packet[17] = pgm_read_byte( &V912_X17_SEQ[packet_count] ) ; // not sure what [17] & [18] are for
			if(packet_count == 0)                    // V912 Rx does not even read those bytes... [17-20]
				packet[18] = 0x02;
			else
				packet[18] = 0x00;
			packet[19] = 0x00; // unknown
			packet[20] = 0x00; // unknown
			break;
			
		case CX20:
			packet[19] = 0x00; // unknown
			packet[20] = (hopping_frequency_no<<4)|0x0A;
			break;
		default:
			break; 
	}
}

static void __attribute__((unused)) flysky_build_packet(uint8_t init)
{
    uint8_t i;
	//servodata timing range for flysky.
	//-100% =~ 0x03e8//=1000us(min)
	//+100% =~ 0x07ca//=1994us(max)
	//Center = 0x5d9//=1497us(center)
	//channel order AIL;ELE;THR;RUD;CH5;CH6;CH7;CH8
    packet[0] = init ? 0xaa : 0x55;
    packet[1] = rx_tx_addr[3];
    packet[2] = rx_tx_addr[2];
    packet[3] = rx_tx_addr[1];
    packet[4] = rx_tx_addr[0];
	for(i = 0; i < 8; i++)
	{
		uint16_t temp=convert_channel_ppm(CH_AETR[i]);
		if(sub_protocol == CX20 && CH_AETR[i]==ELEVATOR)
			temp=3000-temp;
		packet[5 + i*2]=temp&0xFF;		//low byte of servo timing(1000-2000us)
		packet[6 + i*2]=(temp>>8)&0xFF;	//high byte of servo timing(1000-2000us)
	}
    flysky_apply_extension_flags();
}

uint16_t ReadFlySky()
{
	#ifndef FORCE_FLYSKY_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		flysky_build_packet(1);
		A7105_WriteData(21, 1);
		bind_counter--;
		if (bind_counter==0)
			BIND_DONE;
	}
	else
	{
		flysky_build_packet(0);
		A7105_WriteData(21, hopping_frequency[hopping_frequency_no & 0x0F]);
		A7105_SetPower();
	}
	hopping_frequency_no++;

	if(sub_protocol==CX20)
		return 3984;
	else
		return 1510;	//1460 on deviation but not working with the latest V911 bricks... Turnigy 9X v2 is 1533, Flysky TX for 9XR/9XR Pro is 1510, V911 TX is 1490.
}

const uint8_t PROGMEM tx_channels[8][4] = {
	{ 0x12, 0x34, 0x56, 0x78},
	{ 0x18, 0x27, 0x36, 0x45},
	{ 0x41, 0x82, 0x36, 0x57},
	{ 0x84, 0x13, 0x65, 0x72},
	{ 0x87, 0x64, 0x15, 0x32},
	{ 0x76, 0x84, 0x13, 0x52},
	{ 0x71, 0x62, 0x84, 0x35},
	{ 0x71, 0x86, 0x43, 0x52}
};

uint16_t initFlySky()
{
	uint8_t chanrow;
	uint8_t chanoffset;
	uint8_t temp;

	A7105_Init();
	
	// limit offset to 9 as higher values don't work with some RX (ie V912)
	// limit offset to 9 as CX20 repeats the same channels after that
	if ((rx_tx_addr[3]&0xF0) > 0x90)
		rx_tx_addr[3]=rx_tx_addr[3]-0x70;

	// Build frequency hop table
	chanrow=rx_tx_addr[3] & 0x0F;
	chanoffset=rx_tx_addr[3]/16;
	for(uint8_t i=0;i<16;i++)
	{
		temp=pgm_read_byte_near(&tx_channels[chanrow>>1][i>>2]);
		if(i&0x02)
			temp&=0x0F;
		else
			temp>>=4;
		temp*=0x0A;
		if(i&0x01)
			temp+=0x50;
		if(sub_protocol==CX20)
		{
			if(temp==0x0A)
				temp+=0x37;
			if(temp==0xA0)
			{
				if (chanoffset<4)
					temp=0x37;
				else if (chanoffset<9)
					temp=0x2D;
				else
					temp=0x29;
			}
		}
		hopping_frequency[((chanrow&1)?15-i:i)]=temp-chanoffset;
	}
	hopping_frequency_no=0;
	packet_count=0;
	if(IS_BIND_IN_PROGRESS)
		bind_counter = FLYSKY_BIND_COUNT;
	else
		bind_counter = 0;
	return 2400;
}
#endif
