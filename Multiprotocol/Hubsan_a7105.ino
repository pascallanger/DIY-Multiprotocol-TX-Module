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
	// flags going to packet[9] (H107)
	HUBSAN_FLAG_VIDEO= 0x01,   // record video
	HUBSAN_FLAG_FLIP = 0x08,   // enable flips
	HUBSAN_FLAG_LED  = 0x04    // enable LEDs
};

enum{
	// flags going to packet[9] (H107 Plus series)
	HUBSAN_FLAG_HEADLESS = 0x08, // headless mode
};

enum{
	// flags going to packet[9] (H301)
	FLAG_H301_VIDEO = 0x01,
	FLAG_H301_STAB  = 0x02,
	FLAG_H301_LED   = 0x10,
	FLAG_H301_RTH   = 0x40,
};

enum{
	// flags going to packet[13] (H107 Plus series)
	HUBSAN_FLAG_SNAPSHOT  = 0x01,
	HUBSAN_FLAG_FLIP_PLUS = 0x80,
};

enum{
	// flags going to packet[9] (H501S)
	FLAG_H501_VIDEO     = 0x01,
	FLAG_H501_LED       = 0x04,
	FLAG_H122D_FLIP     = 0x08,	//H122D
	FLAG_H501_RTH       = 0x20,
	FLAG_H501_HEADLESS1 = 0x40,
	FLAG_H501_GPS_HOLD  = 0x80,
	};

enum{
	// flags going to packet[11] (H122D & H123D)
	FLAG_H123D_FMODES   = 0x03,	//H123D 3 FMODES: Sport mode 1, Sport mode 2, Acro
	FLAG_H122D_OSD	    = 0x20,	//H122D OSD
};

enum{
	// flags going to packet[13] (H501S)
	FLAG_H501_SNAPSHOT  = 0x01,
	FLAG_H501_HEADLESS2 = 0x02,
	FLAG_H501_ALT_HOLD  = 0x08,
};

uint32_t hubsan_id_data;

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
#define HUBSAN_WAIT_WRITE 0x80

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
	packet[2] = (MProtocol_id >> 24) & 0xFF;
	packet[3] = (MProtocol_id >> 16) & 0xFF;
	packet[4] = (MProtocol_id >>  8) & 0xFF;
	packet[5] = (MProtocol_id >>  0) & 0xFF;
	if(hubsan_id_data == ID_NORMAL && sub_protocol != H501)
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
	static uint8_t vtx_freq = 0, h501_packet = 0; 
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
	if(hubsan_id_data == ID_NORMAL && sub_protocol==H107)
	{// H107/L/C/D, H102D
		if( packet_count < 100)
		{
			packet[9] = 0x02 | HUBSAN_FLAG_LED | HUBSAN_FLAG_FLIP; // sends default value for the 100 first packets
			packet_count++;
		}
		else
		{
			packet[9] = 0x02;
			// Channel 5
			if(CH5_SW)	packet[9] |= HUBSAN_FLAG_FLIP;
			// Channel 6
			if(CH6_SW)	packet[9] |= HUBSAN_FLAG_LED;
			// Channel 8
			if(CH8_SW)	packet[9] |= HUBSAN_FLAG_VIDEO; // H102D
		}
		packet[10] = 0x64;
		//const uint32_t txid = 0xdb042679; 
		packet[11] = 0xDB;
		packet[12] = 0x04;
		packet[13] = 0x26;
		packet[14] = 0x79;
	} else 	if(sub_protocol==H301)
	{// H301
		if( packet_count < 100)
		{
			packet[9] = FLAG_H301_STAB; // sends default value for the 100 first packets
			packet_count++;
		}
		else
		{
            packet[9] = GET_FLAG(CH6_SW, FLAG_H301_LED)
                      | GET_FLAG(CH7_SW, FLAG_H301_STAB)
                      | GET_FLAG(CH8_SW, FLAG_H301_VIDEO)
                      | GET_FLAG(CH5_SW, FLAG_H301_RTH);
		}
		packet[10] = 0x18; // ?
		packet[12] = 0x5c; // ?
		packet[14] = 0xf6; // ?
	}
	else
	{ //ID_PLUS && H501
		packet[3] = sub_protocol==H501 ? 0x00:0x64;
		packet[5] = sub_protocol==H501 ? 0x00:0x64;
		packet[7] = sub_protocol==H501 ? 0x00:0x64;

		if(sub_protocol==H501)
		{ // H501S
			packet[9] = 0x02
					   | GET_FLAG(CH6_SW, FLAG_H501_LED)
					   | GET_FLAG(CH8_SW, FLAG_H501_VIDEO)
					   | GET_FLAG(CH12_SW, FLAG_H122D_FLIP)	// H122D specific -> flip
					   | GET_FLAG(CH5_SW, FLAG_H501_RTH)
					   | GET_FLAG(CH10_SW, FLAG_H501_GPS_HOLD)
					   | GET_FLAG(CH9_SW, FLAG_H501_HEADLESS1);
			//packet[10]= 0x1A;

			//packet[11] content 0x00 is default
			//H123D specific -> Flight modes
			packet[11] = 0x41;	// Sport mode 1
			if(Channel_data[CH13]>CHANNEL_MAX_COMMAND)
				packet[11]=0x43;	// Acro
			else if(Channel_data[CH13]>CHANNEL_MIN_COMMAND)
				packet[11]=0x42;	// Sport mode 2
			//H122D specific -> OSD but useless...
			//packet[11]|= 0x80
			//		  | GET_FLAG(CHXX_SW,FLAG_H122D_OSD); 

			packet[13] = GET_FLAG(CH9_SW, FLAG_H501_HEADLESS2)
					   | GET_FLAG(CH11_SW, FLAG_H501_ALT_HOLD)
					   | GET_FLAG(CH7_SW, FLAG_H501_SNAPSHOT);
		}
		else
		{ // H107P/C+/D+
			packet[9] = 0x06;
			//FLIP|LIGHT|PICTURE|VIDEO|HEADLESS
			if(CH8_SW)	packet[9] |= HUBSAN_FLAG_VIDEO;
			if(CH9_SW)	packet[9] |= HUBSAN_FLAG_HEADLESS;
			packet[10]= 0x19;
			packet[12]= 0x5C; // ghost channel ?
			packet[13] = 0;
			if(CH7_SW)	packet[13]  = HUBSAN_FLAG_SNAPSHOT;
			if(CH5_SW)	packet[13] |= HUBSAN_FLAG_FLIP_PLUS;
			packet[14]= 0x49; // ghost channel ?
		}
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
		if(sub_protocol==H501)
		{ // H501S
			h501_packet++;
			if(h501_packet == 10)
			{
				memset(packet, 0, 16);
				packet[0] = 0xe8;
			}
			else if(h501_packet == 20)
			{
				memset(packet, 0, 16);
				packet[0] = 0xe9;
			}
			if(h501_packet >= 20) h501_packet = 0;
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
	uint16_t delay;
	uint8_t i;

	#ifndef FORCE_HUBSAN_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	switch(phase)
	{
		case BIND_1:
			bind_phase++;
			if(bind_phase >= 20 && sub_protocol != H501)
			{
				if(hubsan_id_data == ID_NORMAL)
					hubsan_id_data = ID_PLUS;
				else
					hubsan_id_data = ID_NORMAL;
				A7105_WriteID(hubsan_id_data);    
				bind_phase = 0;
			}
		case BIND_3:
		case BIND_5:
		case BIND_7:
			hubsan_build_bind_packet(phase == BIND_7 ? 9 : (phase == BIND_5 ? 1 : phase + 1 - BIND_1));
			A7105_Strobe(A7105_STANDBY);
			A7105_WriteData(16, channel);
			phase |= HUBSAN_WAIT_WRITE;
			return 3000;
		case BIND_1 | HUBSAN_WAIT_WRITE:
		case BIND_3 | HUBSAN_WAIT_WRITE:
		case BIND_5 | HUBSAN_WAIT_WRITE:
		case BIND_7 | HUBSAN_WAIT_WRITE:
			//wait for completion
			for(i = 0; i< 20; i++)
				if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			phase &= ~HUBSAN_WAIT_WRITE;
			if(hubsan_id_data == ID_PLUS)
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
			if(packet[1] == 9 && hubsan_id_data == ID_NORMAL) {
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
				A7105_Strobe(A7105_STANDBY);
				uint8_t ch;
				if((phase == DATA_5 && hubsan_id_data == ID_NORMAL) && sub_protocol == H107)
					ch = channel + 0x23;
				else
					ch = channel;
				A7105_WriteData(16, ch);
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

uint16_t initHubsan()
{
	const uint8_t allowed_ch[] = {0x14, 0x1e, 0x28, 0x32, 0x3c, 0x46, 0x50, 0x5a, 0x64, 0x6e, 0x78, 0x82};
	A7105_Init();

	channel = allowed_ch[MProtocol_id % sizeof(allowed_ch)];
	hubsan_id_data=ID_NORMAL;

	if(IS_BIND_IN_PROGRESS || sub_protocol==H107)
	{
		BIND_IN_PROGRESS;	// autobind protocol
		phase = BIND_1;
	}
	else 
	{
		phase = DATA_1;
		A7105_WriteID(MProtocol_id);
		A7105_WriteReg(A7105_1F_CODE_I, 0x0F);
	}
	packet_count=0;
	bind_phase=0;
	#ifdef HUBSAN_HUB_TELEMETRY
		init_frskyd_link_telemetry();
	#endif
	return 10000;
}

#endif
