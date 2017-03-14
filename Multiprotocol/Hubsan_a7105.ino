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
// compatible with Hubsan H102D, H107/L/C/D and H107P/C+/D+
// Last sync with hexfet new_protocols/hubsan_a7105.c dated 2015-12-11

#if defined(HUBSAN_A7105_INO)

#include "iface_a7105.h"

enum{
    // flags going to packet[9] (Normal)
    HUBSAN_FLAG_VIDEO= 0x01,   // record video
    HUBSAN_FLAG_FLIP = 0x08,   // enable flips
    HUBSAN_FLAG_LED  = 0x04    // enable LEDs
};

enum{
    // flags going to packet[9] (Plus series)
    HUBSAN_FLAG_HEADLESS = 0x08, // headless mode
};

enum{
    // flags going to packet[13] (Plus series)
    HUBSAN_FLAG_SNAPSHOT  = 0x01,
    HUBSAN_FLAG_FLIP_PLUS = 0x80,
};

uint32_t sessionid,id_data;

enum {
	BIND_1,
	BIND_2,
	BIND_3,
	BIND_4,
	BIND_5,
	BIND_6,
	BIND_7,
	BIND_8,
	DATA_1,
	DATA_2,
	DATA_3,
	DATA_4,
	DATA_5,
};
#define WAIT_WRITE 0x80

static void __attribute__((unused)) hubsan_update_crc()
{
	uint8_t sum = 0;
	for(uint8_t i = 0; i < 15; i++)
		sum += packet[i];
	packet[15] = (256 - (sum % 256)) & 0xFF;
}

static void __attribute__((unused)) hubsan_build_bind_packet(uint8_t bind_state)
{
	static uint8_t handshake_counter;
	if(phase < BIND_7)
		handshake_counter = 0;
	memset(packet, 0, 16);
	packet[0] = bind_state;
	packet[1] = channel;
	packet[2] = (sessionid >> 24) & 0xFF;
	packet[3] = (sessionid >> 16) & 0xFF;
	packet[4] = (sessionid >>  8) & 0xFF;
	packet[5] = (sessionid >>  0) & 0xFF;
	if(id_data == ID_NORMAL)
	{
		packet[6] = 0x08;
		packet[7] = 0xe4;
		packet[8] = 0xea;
		packet[9] = 0x9e;
		packet[10] = 0x50;
		//const uint32_t txid = 0xdb042679; 
		packet[11] = 0xDB;
		packet[12] = 0x04;
		packet[13] = 0x26;
		packet[14] = 0x79;
	}
	else
	{ //ID_PLUS
		if(phase >= BIND_3)
		{
			packet[7] = 0x62;
			packet[8] = 0x16;
		}
		if(phase == BIND_7)
			packet[2] = handshake_counter++;
	}
	hubsan_update_crc();
}

//cc : throttle  observed range: 0x00 - 0xFF (smaller is down)
//ee : rudder    observed range: 0x34 - 0xcc (smaller is right)52-204-60%
//gg : elevator  observed range: 0x3e - 0xbc (smaller is up)62-188 -50%
//ii : aileron   observed range: 0x45 - 0xc3 (smaller is right)69-195-50%
static void __attribute__((unused)) hubsan_build_packet()
{
	static uint8_t vtx_freq = 0; 
	memset(packet, 0, 16);
	if(vtx_freq != option || packet_count==100) // set vTX frequency (H107D)
	{
		vtx_freq = option;
		packet[0] = 0x40;	// vtx data packet
		packet[1] = (vtx_freq>0xF2)?0x17:0x16;
		packet[2] = vtx_freq+0x0D;	// 5645 - 5900 MHz
		packet[3] = 0x82;
		packet_count++;      
	}
	else //20 00 00 00 80 00 7d 00 84 02 64 db 04 26 79 7b
	{
		packet[0] = 0x20;	// normal data packet
		packet[2] = convert_channel_8b(THROTTLE);		//Throtle
	}
	packet[4] = 0xFF - convert_channel_8b(RUDDER);		//Rudder is reversed
	packet[6] = 0xFF - convert_channel_8b(ELEVATOR);	//Elevator is reversed
	packet[8] = convert_channel_8b(AILERON);			//Aileron
	if(id_data == ID_NORMAL)
	{
		if( packet_count < 100)
		{
			packet[9] = 0x02 | HUBSAN_FLAG_LED | HUBSAN_FLAG_FLIP; // sends default value for the 100 first packets
			packet_count++;
		}
		else
		{
			packet[9] = 0x02;
			// Channel 5
			if(Servo_AUX1)	packet[9] |= HUBSAN_FLAG_FLIP;
			// Channel 6
			if(Servo_AUX2)	packet[9] |= HUBSAN_FLAG_LED;
			// Channel 8
			if(Servo_AUX4)	packet[9] |= HUBSAN_FLAG_VIDEO; // H102D
		}
		packet[10] = 0x64;
		//const uint32_t txid = 0xdb042679; 
		packet[11] = 0xDB;
		packet[12] = 0x04;
		packet[13] = 0x26;
		packet[14] = 0x79;
	}
	else
	{ //ID_PLUS
		packet[3] = 0x64;
		packet[5] = 0x64;
		packet[7] = 0x64;
		packet[9] = 0x06;
		//FLIP|LIGHT|PICTURE|VIDEO|HEADLESS
		if(Servo_AUX4)	packet[9] |= HUBSAN_FLAG_VIDEO;
		if(Servo_AUX5)	packet[9] |= HUBSAN_FLAG_HEADLESS;
		packet[10]= 0x19;
		packet[12]= 0x5C; // ghost channel ?
		packet[13] = 0;
		if(Servo_AUX3)	packet[13]  = HUBSAN_FLAG_SNAPSHOT;
		if(Servo_AUX1)	packet[13] |= HUBSAN_FLAG_FLIP_PLUS;
		packet[14]= 0x49; // ghost channel ?
		if(packet_count < 100)
		{ // set channels to neutral for first 100 packets
			packet[2] = 0x80; // throttle neutral is at mid stick on plus series
			packet[4] = 0x80;
			packet[6] = 0x80;
			packet[8] = 0x80;
			packet[9] = 0x06;
			packet[13]= 0x00;
			packet_count++;
		}
	}
	hubsan_update_crc();
}

#ifdef HUBSAN_HUB_TELEMETRY
static uint8_t __attribute__((unused)) hubsan_check_integrity() 
{
    if( (packet[0]&0xFE) != 0xE0 )
		return 0;
	uint8_t sum = 0;
    for(uint8_t i = 0; i < 15; i++)
        sum += packet[i];
	return ( packet[15] == (uint8_t)(-sum) );
}
#endif

uint16_t ReadHubsan() 
{
#ifdef HUBSAN_HUB_TELEMETRY
	static uint8_t rfMode=0;
#endif
	static uint8_t txState=0;
	static uint8_t bind_count=0;
	uint16_t delay;
	uint8_t i;

	switch(phase) {
		case BIND_1:
			bind_count++;
			if(bind_count >= 20)
			{
				if(id_data == ID_NORMAL)
					id_data = ID_PLUS;
				else
					id_data = ID_NORMAL;
				A7105_WriteID(id_data);    
				bind_count = 0;
			}
		case BIND_3:
		case BIND_5:
		case BIND_7:
			hubsan_build_bind_packet(phase == BIND_7 ? 9 : (phase == BIND_5 ? 1 : phase + 1 - BIND_1));
			A7105_WriteData(16, channel);
			phase |= WAIT_WRITE;
			return 3000;
		case BIND_1 | WAIT_WRITE:
		case BIND_3 | WAIT_WRITE:
		case BIND_5 | WAIT_WRITE:
		case BIND_7 | WAIT_WRITE:
			//wait for completion
			for(i = 0; i< 20; i++)
				if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			phase &= ~WAIT_WRITE;
			if(id_data == ID_PLUS)
			{
				if(phase == BIND_7 && packet[2] == 9)
				{
					phase = DATA_1;
					A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
					BIND_DONE;
					return 4500;
				}
			}
			phase++;
			return 4500; //7.5msec elapsed since last write
		case BIND_2:
		case BIND_4:
		case BIND_6:
			A7105_SetTxRxMode(TX_EN);
			if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
				phase = BIND_1;
				return 4500; //No signal, restart binding procedure.  12msec elapsed since last write
			}
			A7105_ReadData(16);
			phase++;
			if (phase == BIND_5)
				A7105_WriteID(((uint32_t)packet[2] << 24) | ((uint32_t)packet[3] << 16) | ((uint32_t)packet[4] << 8) | packet[5]);
			return 500;  //8msec elapsed time since last write;
		case BIND_8:
			A7105_SetTxRxMode(TX_EN);
			if(A7105_ReadReg(A7105_00_MODE) & 0x01) {
				phase = BIND_7;
				return 15000; //22.5msec elapsed since last write
			}
			A7105_ReadData(16);
			if(packet[1] == 9 && id_data == ID_NORMAL) {
				phase = DATA_1;
				A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
				BIND_DONE;
				return 28000; //35.5msec elapsed since last write
			} else {
				phase = BIND_7;
				return 15000; //22.5 msec elapsed since last write
			}
		case DATA_1:
		case DATA_2:
		case DATA_3:
		case DATA_4:
		case DATA_5:
			if( txState == 0) { // send packet
#ifdef HUBSAN_HUB_TELEMETRY
				rfMode = A7105_TX;
#endif
				if( phase == DATA_1)
						A7105_SetPower(); //Keep transmit power in sync
				hubsan_build_packet();
				A7105_WriteData(16, phase == DATA_5 && id_data == ID_NORMAL ? channel + 0x23 : channel);
				if (phase == DATA_5)
					phase = DATA_1;
				else
					phase++;
				delay=3000;
			}
			else {
#ifdef HUBSAN_HUB_TELEMETRY
				if( rfMode == A7105_TX)
				{// switch to rx mode 3ms after packet sent
					for( i=0; i<10; i++)
					{
						if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) {// wait for tx completion
							A7105_SetTxRxMode(RX_EN);
							A7105_Strobe(A7105_RX); 
							rfMode = A7105_RX;
							break;
						}
					}
				}
				if( rfMode == A7105_RX)
				{ // check for telemetry frame
					for( i=0; i<10; i++)
					{
						if( !(A7105_ReadReg(A7105_00_MODE) & 0x01))
						{ // data received
							A7105_ReadData(16);
							if( hubsan_check_integrity() )
							{
								v_lipo1=packet[13]*2;// hubsan lipo voltage 8bits the real value is h_lipo/10(0x2A=42 -> 4.2V)
								telemetry_link=1;
							}	
							A7105_Strobe(A7105_RX);
							// Read TX RSSI
							int16_t temp=256-(A7105_ReadReg(A7105_1D_RSSI_THOLD)*8)/5;		// value from A7105 is between 8 for maximum signal strength to 160 or less
							if(temp<0) temp=0;
							else if(temp>255) temp=255;
							TX_RSSI=temp;
							break;
						}
					}
				}
#endif
				delay=1000;
			}
			if (++txState == 8) { // 3ms + 7*1ms
				A7105_SetTxRxMode(TX_EN);
				txState = 0;
			}
			return delay;
	}
	return 0;
}

uint16_t initHubsan() {
	const uint8_t allowed_ch[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};
	A7105_Init();

	sessionid = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
	channel = allowed_ch[random(0xfefefefe) % sizeof(allowed_ch)];

	BIND_IN_PROGRESS;	// autobind protocol
	phase = BIND_1;
	packet_count=0;
	id_data=ID_NORMAL;
#ifdef HUBSAN_HUB_TELEMETRY
	init_frskyd_link_telemetry();
#endif
	return 10000;
}

#endif
