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

uint8_t chanrow;
uint8_t chancol;
uint8_t chanoffset;

static void __attribute__((unused)) flysky_apply_extension_flags()
{
	const uint8_t V912_X17_SEQ[10] =  { 0x14, 0x31, 0x40, 0x49, 0x49,    // sometime first byte is 0x15 ?
										0x49, 0x49, 0x49, 0x49, 0x49, }; 
	static uint8_t seq_counter;
	switch(sub_protocol)
	{
		case V9X9:
			if(Servo_AUX1)
				packet[12] |= FLAG_V9X9_FLIP;
			if(Servo_AUX2)
				packet[12] |= FLAG_V9X9_LED;
			if(Servo_AUX3)
				packet[10] |= FLAG_V9X9_CAMERA;
			if(Servo_AUX4)
				packet[10] |= FLAG_V9X9_VIDEO;
			break;
			
		case V6X6:
			packet[13] = 0x03; // 3 = 100% rate (0=40%, 1=60%, 2=80%)
			packet[14] = 0x00;
			if(Servo_AUX1) 
				packet[14] |= FLAG_V6X6_FLIP;
			if(Servo_AUX2) 
				packet[14] |= FLAG_V6X6_LED;
			if(Servo_AUX3) 
				packet[14] |= FLAG_V6X6_CAMERA;
			if(Servo_AUX4) 
				packet[14] |= FLAG_V6X6_VIDEO;
			if(Servo_AUX5)
			{ 
				packet[13] |= FLAG_V6X6_HLESS1;
				packet[14] |= FLAG_V6X6_HLESS2;
			}
			if(Servo_AUX6) //use option to manipulate these bytes
				packet[14] |= FLAG_V6X6_RTH;
			if(Servo_AUX7) 
				packet[14] |= FLAG_V6X6_XCAL;
			if(Servo_AUX8) 
				packet[14] |= FLAG_V6X6_YCAL;
			packet[15] = 0x10; // unknown
			packet[16] = 0x10; // unknown
			packet[17] = 0xAA; // unknown
			packet[18] = 0xAA; // unknown
			packet[19] = 0x60; // unknown, changes at irregular interval in stock TX
			packet[20] = 0x02; // unknown
			break;
			
		case V912:
			seq_counter++;
			if( seq_counter > 9)
				seq_counter = 0;
			packet[12] |= 0x20; // bit 6 is always set ?
			packet[13] = 0x00;  // unknown
			packet[14] = 0x00;
			if(Servo_AUX1)
				packet[14]  = FLAG_V912_BTMBTN;
			if(Servo_AUX2)
				packet[14] |= FLAG_V912_TOPBTN;
			packet[15] = 0x27; // [15] and [16] apparently hold an analog channel with a value lower than 1000
			packet[16] = 0x03; // maybe it's there for a pitch channel for a CP copter ?
			packet[17] = V912_X17_SEQ[seq_counter]; // not sure what [17] & [18] are for
			if(seq_counter == 0)                    // V912 Rx does not even read those bytes... [17-20]
				packet[18] = 0x02;
			else
				packet[18] = 0x00;
			packet[19] = 0x00; // unknown
			packet[20] = 0x00; // unknown
			break;
			
		default:
			break; 
	}
}

static void __attribute__((unused)) flysky_build_packet(uint8_t init)
{
    uint8_t i;
	//servodata timing range for flysky.
	////-100% =~ 0x03e8//=1000us(min)
	//+100% =~ 0x07ca//=1994us(max)
	//Center = 0x5d9//=1497us(center)
	//channel order AIL;ELE;THR;RUD;AUX1;AUX2;AUX3;AUX4
    packet[0] = init ? 0xaa : 0x55;
    packet[1] = rx_tx_addr[3];
    packet[2] = rx_tx_addr[2];
    packet[3] = rx_tx_addr[1];
    packet[4] = rx_tx_addr[0];
	const uint8_t ch[]={AILERON, ELEVATOR, THROTTLE, RUDDER, AUX1, AUX2, AUX3, AUX4};
	for(i = 0; i < 8; i++)
	{
		packet[5 + i*2]=Servo_data[ch[i]]&0xFF;		//low byte of servo timing(1000-2000us)
		packet[6 + i*2]=(Servo_data[ch[i]]>>8)&0xFF;	//high byte of servo timing(1000-2000us)
	}
    flysky_apply_extension_flags();
}

const uint8_t PROGMEM tx_channels[16][16] = {
  {0x0a, 0x5a, 0x14, 0x64, 0x1e, 0x6e, 0x28, 0x78, 0x32, 0x82, 0x3c, 0x8c, 0x46, 0x96, 0x50, 0xa0},
  {0xa0, 0x50, 0x96, 0x46, 0x8c, 0x3c, 0x82, 0x32, 0x78, 0x28, 0x6e, 0x1e, 0x64, 0x14, 0x5a, 0x0a},
  {0x0a, 0x5a, 0x50, 0xa0, 0x14, 0x64, 0x46, 0x96, 0x1e, 0x6e, 0x3c, 0x8c, 0x28, 0x78, 0x32, 0x82},
  {0x82, 0x32, 0x78, 0x28, 0x8c, 0x3c, 0x6e, 0x1e, 0x96, 0x46, 0x64, 0x14, 0xa0, 0x50, 0x5a, 0x0a},
  {0x28, 0x78, 0x0a, 0x5a, 0x50, 0xa0, 0x14, 0x64, 0x1e, 0x6e, 0x3c, 0x8c, 0x32, 0x82, 0x46, 0x96},
  {0x96, 0x46, 0x82, 0x32, 0x8c, 0x3c, 0x6e, 0x1e, 0x64, 0x14, 0xa0, 0x50, 0x5a, 0x0a, 0x78, 0x28},
  {0x50, 0xa0, 0x28, 0x78, 0x0a, 0x5a, 0x1e, 0x6e, 0x3c, 0x8c, 0x32, 0x82, 0x46, 0x96, 0x14, 0x64},
  {0x64, 0x14, 0x96, 0x46, 0x82, 0x32, 0x8c, 0x3c, 0x6e, 0x1e, 0x5a, 0x0a, 0x78, 0x28, 0xa0, 0x50},
  {0x50, 0xa0, 0x46, 0x96, 0x3c, 0x8c, 0x28, 0x78, 0x0a, 0x5a, 0x32, 0x82, 0x1e, 0x6e, 0x14, 0x64},
  {0x64, 0x14, 0x6e, 0x1e, 0x82, 0x32, 0x5a, 0x0a, 0x78, 0x28, 0x8c, 0x3c, 0x96, 0x46, 0xa0, 0x50},
  {0x46, 0x96, 0x3c, 0x8c, 0x50, 0xa0, 0x28, 0x78, 0x0a, 0x5a, 0x1e, 0x6e, 0x32, 0x82, 0x14, 0x64},
  {0x64, 0x14, 0x82, 0x32, 0x6e, 0x1e, 0x5a, 0x0a, 0x78, 0x28, 0xa0, 0x50, 0x8c, 0x3c, 0x96, 0x46},
  {0x46, 0x96, 0x0a, 0x5a, 0x3c, 0x8c, 0x14, 0x64, 0x50, 0xa0, 0x28, 0x78, 0x1e, 0x6e, 0x32, 0x82},
  {0x82, 0x32, 0x6e, 0x1e, 0x78, 0x28, 0xa0, 0x50, 0x64, 0x14, 0x8c, 0x3c, 0x5a, 0x0a, 0x96, 0x46},
  {0x46, 0x96, 0x0a, 0x5a, 0x50, 0xa0, 0x3c, 0x8c, 0x28, 0x78, 0x1e, 0x6e, 0x32, 0x82, 0x14, 0x64},
  {0x64, 0x14, 0x82, 0x32, 0x6e, 0x1e, 0x78, 0x28, 0x8c, 0x3c, 0xa0, 0x50, 0x5a, 0x0a, 0x96, 0x46},
};

uint16_t ReadFlySky()
{
    if (bind_counter)
	{
        flysky_build_packet(1);
        A7105_WriteData(21, 1);
        bind_counter--;
        if (! bind_counter)
            BIND_DONE;
    }
	else
	{
		flysky_build_packet(0);
        A7105_WriteData(21, pgm_read_byte_near(&tx_channels[chanrow][chancol])-chanoffset);
        chancol = (chancol + 1) % 16;
        if (! chancol) //Keep transmit power updated
			A7105_SetPower();
    }
	return 1510;	//1460 on deviation but not working with the latest V911 bricks... Turnigy 9X v2 is 1533, Flysky TX for 9XR/9XR Pro is 1510, V911 TX is 1490.
}

uint16_t initFlySky() {
	A7105_Init(INIT_FLYSKY);	//flysky_init();
	
	if ((rx_tx_addr[3]&0xF0) > 0x90) // limit offset to 9 as higher values don't work with some RX (ie V912)
		rx_tx_addr[3]=rx_tx_addr[3]-0x70;
	chanrow=rx_tx_addr[3] & 0x0F;
	chancol=0;
	chanoffset=rx_tx_addr[3]/16;
	
	if(IS_AUTOBIND_FLAG_on)
		bind_counter = FLYSKY_BIND_COUNT;
	else
		bind_counter = 0;
	return 2400;
}

#endif
