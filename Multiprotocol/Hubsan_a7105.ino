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

#if defined(HUBSAN_A7105_INO)

#include "iface_a7105.h"

enum{
	HUBSAN_FLAG_VIDEO = 0x01,   // record video
	HUBSAN_FLAG_FLIP  = 0x08,
	HUBSAN_FLAG_LIGHT = 0x04
};

uint32_t sessionid;
const uint32_t txid = 0xdb042679; 

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

void update_crc()
{
	uint8_t sum = 0;
	for(uint8_t i = 0; i < 15; i++)
		sum += packet[i];
	packet[15] = (256 - (sum % 256)) & 0xFF;
}

void hubsan_build_bind_packet(uint8_t state)
{
	packet[0] = state;
	packet[1] = channel;
	packet[2] = (sessionid >> 24) & 0xFF;
	packet[3] = (sessionid >> 16) & 0xFF;
	packet[4] = (sessionid >>  8) & 0xFF;
	packet[5] = (sessionid >>  0) & 0xFF;
	packet[6] = 0x08;
	packet[7] = 0xe4;
	packet[8] = 0xea;
	packet[9] = 0x9e;
	packet[10] = 0x50;
	packet[11] = (txid >> 24) & 0xFF;
	packet[12] = (txid >> 16) & 0xFF;
	packet[13] = (txid >>  8) & 0xFF;
	packet[14] = (txid >>  0) & 0xFF;
	update_crc();
}

//cc : throttle  observed range: 0x00 - 0xFF (smaller is down)
//ee : rudder    observed range: 0x34 - 0xcc (smaller is right)52-204-60%
//gg : elevator  observed range: 0x3e - 0xbc (smaller is up)62-188 -50%
//ii : aileron   observed range: 0x45 - 0xc3 (smaller is right)69-195-50%
void hubsan_build_packet()
{
	static uint8_t vtx_freq = 0; 
	memset(packet, 0, 16);
	if(vtx_freq != option || packet_count==100) // set vTX frequency (H107D)
	{
		vtx_freq = option;
		packet[0] = 0x40;
		packet[1] = (option>0xF2)?0x17:0x16;
		packet[2] = option+0x0D;	// 5645 - 5900 MHz
		packet[3] = 0x82;
		packet_count++;      
	}
	else //20 00 00 00 80 00 7d 00 84 02 64 db 04 26 79 7b
	{
		packet[0] = 0x20;
		packet[2] = convert_channel_8b(THROTTLE);//throtle
	}
	packet[4] = 0xFF - convert_channel_8b(RUDDER);//Rudder is reversed
	packet[6] = 0xFF - convert_channel_8b(ELEVATOR); //Elevator is reversed
	packet[8] = convert_channel_8b(AILERON);//aileron
	if( packet_count < 100) {
		packet[9] = 0x02 | HUBSAN_FLAG_LIGHT | HUBSAN_FLAG_FLIP;
		packet_count++;
	}
	else
	{
		packet[9] = 0x02;
        // Channel 5
		if( Servo_data[AUX1] >= PPM_SWITCH)
			packet[9] |= HUBSAN_FLAG_FLIP;
        // Channel 6
		if( Servo_data[AUX2] >= PPM_SWITCH)
			packet[9] |= HUBSAN_FLAG_LIGHT;
        // Channel 8
		if( Servo_data[AUX4] > PPM_SWITCH)
			packet[9] |= HUBSAN_FLAG_VIDEO;
	}
	packet[10] = 0x64;
	packet[11] = (txid >> 24) & 0xFF;
	packet[12] = (txid >> 16) & 0xFF;
	packet[13] = (txid >>  8) & 0xFF;
	packet[14] = (txid >>  0) & 0xFF;
	update_crc();
}

uint8_t hubsan_check_integrity() 
{
    uint8_t sum = 0;
    for(int i = 0; i < 15; i++)
        sum += packet[i];
    return packet[15] == ((256 - (sum % 256)) & 0xFF);
}

uint16_t ReadHubsan() 
{
	static uint8_t txState=0;
	static uint8_t rfMode=0;
	uint16_t delay;
	uint8_t i;

	switch(phase) {
		case BIND_1:
		case BIND_3:
		case BIND_5:
		case BIND_7:
			hubsan_build_bind_packet(phase == BIND_7 ? 9 : (phase == BIND_5 ? 1 : phase + 1 - BIND_1));
			A7105_Strobe(A7105_STANDBY);
			A7105_WriteData(16, channel);
			phase |= WAIT_WRITE;
			return 3000;
		case BIND_1 | WAIT_WRITE:
		case BIND_3 | WAIT_WRITE:
		case BIND_5 | WAIT_WRITE:
		case BIND_7 | WAIT_WRITE:
			//wait for completion
			for(i = 0; i< 20; i++) {
				if(! (A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			}
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			phase &= ~WAIT_WRITE;
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
			A7105_ReadData();
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
			A7105_ReadData();
			if(packet[1] == 9) {
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
				rfMode = A7105_TX;
				if( phase == DATA_1)
						A7105_SetPower(); //Keep transmit power in sync
				hubsan_build_packet();
				A7105_Strobe(A7105_STANDBY);
				A7105_WriteData(16, phase == DATA_5 ? channel + 0x23 : channel);
				if (phase == DATA_5)
					phase = DATA_1;
				else
					phase++;
				delay=3000;
			}
			else {
#if defined(TELEMETRY)
				if( rfMode == A7105_TX) {// switch to rx mode 3ms after packet sent
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
				if( rfMode == A7105_RX) { // check for telemetry frame
					for( i=0; i<10; i++) {
						if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)) { // data received
							A7105_ReadData();
							if( !(A7105_ReadReg(A7105_00_MODE) & 0x01)){ // data received
								v_lipo=packet[13];// hubsan lipo voltage 8bits the real value is h_lipo/10(0x2A=42-4.2V)
								telemetry_link=1;
							}	
							A7105_Strobe(A7105_RX);
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
	A7105_Init(INIT_HUBSAN);	//hubsan_init();

	randomSeed((uint32_t)analogRead(A0) << 10 | analogRead(A4));
	sessionid = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
	channel = allowed_ch[random(0xfefefefe) % sizeof(allowed_ch)];

	BIND_IN_PROGRESS;	// autobind protocol
	phase = BIND_1;
	packet_count=0;
	return 10000;
}

#endif
