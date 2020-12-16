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

#if defined(MLINK_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define MLINK_FORCE_ID
#define MLINK_BIND_COUNT	696	// around 20s
#define MLINK_NUM_FREQ		78
#define MLINK_BIND_CHANNEL	0x01
#define MLINK_PACKET_SIZE	8

enum {
	MLINK_BIND_TX=0,
	MLINK_BIND_PREP_RX,
	MLINK_BIND_RX,
	MLINK_PREP_DATA,
	MLINK_RF,
	MLINK_SEND,
};

uint8_t MLINK_Data_Code[16], MLINK_Offset;

const uint8_t PROGMEM MLINK_init_vals[][2] = {
	//Init from dump
	{ CYRF_01_TX_LENGTH,	0x08 },	// Length of packet
	{ CYRF_02_TX_CTRL,		0x40 },	// Clear TX Buffer
	{ CYRF_03_TX_CFG,		0x3C }, //0x3E in normal mode, 0x3C in bind mode: SDR 64 chip codes (=8 bytes data code used)
	{ CYRF_05_RX_CTRL,		0x00 },
	{ CYRF_06_RX_CFG,		0x93 },	// AGC enabled, overwrite enable, valid flag enable
	{ CYRF_0B_PWR_CTRL,		0x00 },
	//{ CYRF_0C_XTAL_CTRL,	0x00 },	// Set to GPIO on reset
	//{ CYRF_0D_IO_CFG,		0x00 },	// Set to GPIO on reset
	//{ CYRF_0E_GPIO_CTRL,	0x00 }, // Set by the CYRF_SetTxRxMode function
	//{ CYRF_0F_XACT_CFG,		0x04 },	// end state idle -> Set by the CYRF_SetTxRxMode function
	{ CYRF_10_FRAMING_CFG,	0x00 },	// SOP disabled
	{ CYRF_11_DATA32_THOLD,	0x05 }, // not used???
	{ CYRF_12_DATA64_THOLD,	0x0F }, // 64 Chip Data PN Code Correlator Threshold
	{ CYRF_14_EOP_CTRL,		0x05 }, // 5 consecutive noncorrelations symbol for EOP
	{ CYRF_15_CRC_SEED_LSB,	0x00 }, // not used???
	{ CYRF_16_CRC_SEED_MSB,	0x00 }, // not used???
	{ CYRF_1B_TX_OFFSET_LSB,0x00 },
	{ CYRF_1C_TX_OFFSET_MSB,0x00 },
	{ CYRF_1D_MODE_OVERRIDE,0x00 },
	{ CYRF_1E_RX_OVERRIDE,	0x14 },	// RX CRC16 is disabled and Force Receive Data Rate
	{ CYRF_1F_TX_OVERRIDE,	0x04 }, // TX CRC16 is disabled
	{ CYRF_26_XTAL_CFG,		0x08 },
	{ CYRF_29_RX_ABORT,		0x00 },
	{ CYRF_32_AUTO_CAL_TIME,0x3C },
	{ CYRF_35_AUTOCAL_OFFSET,0x14 },
	{ CYRF_39_ANALOG_CTRL,	0x03 },	// Receive invert and all slow
};

static void __attribute__((unused)) MLINK_cyrf_config()
{
	for(uint8_t i = 0; i < sizeof(MLINK_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&MLINK_init_vals[i][0]), pgm_read_byte_near(&MLINK_init_vals[i][1]));
	CYRF_WritePreamble(0x333304);
	CYRF_SetTxRxMode(TX_EN);
}

static uint8_t __attribute__((unused)) MLINK_BR(uint8_t byte)
{
	uint8_t result = 0;
	for(uint8_t i=0;i<8;i++)
	{
		result = (result<<1) | (byte & 0x01);
		byte >>= 1;
	}
	return result;
}

static void __attribute__((unused)) MLINK_CRC(uint8_t byte)
{
	crc8 = crc8 ^ MLINK_BR(byte);	// Reflected in
	for ( uint8_t j = 0; j < 8; j++ ) 
		if ( crc8 & 0x80 )
			crc8 = (crc8<<1) ^ 0x31;
		else
			crc8 <<= 1;
}

static void __attribute__((unused)) MLINK_send_bind_data_packet()
{
	uint8_t p_c=packet_count>>1;
	
	memset(packet, p_c<0x16?0x00:0xFF, MLINK_PACKET_SIZE);
	packet[0]=0x0F;	// bind
	packet[1]=p_c;
	switch(p_c)
	{
		case 0x00:
			packet[2]=0x40;	//unknown
			packet[4]=0x01;	//unknown
			packet[5]=0x03;	//unknown
			packet[6]=0xE3;	//unknown
			break;
		case 0x05:
			packet[6]=0x07;	//unknown
			break;
		case 0x06:
			packet[2]=0x3A;	//unknown
			//Start of hopping frequencies
			for(uint8_t i=0;i<4;i++)
				packet[i+3]=hopping_frequency[i];
			break;
		case 0x15:
			packet[6]=0x51;	//unknown
			break;
		case 0x16:
			packet[2]=0x51;	//unknown
			packet[3]=0xEC;	//unknown
			packet[4]=0x05;	//unknown
			break;
		case 0x1A:
			packet[1]=0xFF;
			memset(&packet[2],0x00,5);
			break;
	}
	if(p_c>=0x01 && p_c<=0x04)
	{//DATA_CODE
		uint8_t p_c_5=(p_c-1)*5;
		for(uint8_t i=0;i<5;i++)
			if(i+p_c_5<16)
				packet[i+2]=MLINK_Data_Code[i+p_c_5];
	}
	else
		if(p_c>=0x07 && p_c<=0x15)
		{//Hopping frequencies
			uint8_t p_c_5=5*(p_c-6)-1;
			for(uint8_t i=0;i<5;i++)
				if(i+p_c_5<MLINK_NUM_FREQ)
					packet[i+2]=hopping_frequency[i+p_c_5];
		}

	//Calculate CRC
	crc8=0xFF;	// Init = 0xFF
	for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
		MLINK_CRC(packet[i]);
	packet[7] = MLINK_BR(crc8);			// CRC reflected out

	//Debug
	debug("P(%02d):",p_c);
	for(uint8_t i=0;i<8;i++)
		debug(" %02X",packet[i]);
	debugln("");

	//Send packet
	CYRF_WriteDataPacketLen(packet, MLINK_PACKET_SIZE);
}

static void __attribute__((unused)) MLINK_build_data_packet()
{
	//Channels to be sent
	uint8_t ch[3];
	switch(packet_count%6)
	{
		case 0:
			packet[0] = 0x88;
			ch[0]=4; ch[1]=2; ch[2]=0;
			break;
		case 1:
			packet[0] = 0x80;
			ch[0]=5; ch[1]=3; ch[2]=1;
			break;
		case 2:
			packet[0] = 0x0A;
			ch[0]=16; ch[1]=14; ch[2]=12;
			break;
		case 3:
			packet[0] = 0x09;
			ch[0]=10; ch[1]=8; ch[2]=6;
			break;
		case 4:
			packet[0] = 0x02;
			ch[0]=16; ch[1]=15; ch[2]=13;
			break;
		case 5:
			packet[0] = 0x01;
			ch[0]=11; ch[1]=9; ch[2]=7;
			break;
	}
	//Channels 426..1937..3448
	for(uint8_t i=0;i<3;i++)
	{
		uint16_t tmp=ch[i]<16 ? convert_channel_16b_nolimit(ch[i],426,3448,false) : 0x0000;
		packet[i*2+1]=tmp>>8;
		packet[i*2+2]=tmp;
	}

	//Calculate CRC
	crc8=MLINK_BR(hopping_frequency_no + MLINK_Offset);	// Init = relected freq index + offset
	for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
		MLINK_CRC(packet[i]);
	packet[7] = MLINK_BR(crc8);			// CRC reflected out

/*	//Debug
	debug("P(%02d,%02d):",packet_count,hopping_frequency_no);
	for(uint8_t i=0;i<8;i++)
		debug(" %02X",packet[i]);
	debugln("");*/
}

uint16_t ReadMLINK()
{
	uint8_t status;//,len,sum=0,check=0;
	uint8_t start;
	uint16_t sum=0;
	//static uint8_t retry;

	switch(phase)
	{
		case MLINK_BIND_RX:
			//debugln("RX");
			status=CYRF_ReadRegister(CYRF_05_RX_CTRL);
			if( (status&0x80) == 0 )
			{//Packet received
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				debugln("L=%02X",len)
				if( len==8 )
				{
					CYRF_ReadDataPacketLen(packet, len*2);
					debug("RX=");
					for(uint8_t i=0;i<8*2;i++)
						debug(" %02X",packet[i]);
					debugln("");
					//Check CRC
					crc8=0xFF;	// Init = 0xFF
					for(uint8_t i=0;i<MLINK_PACKET_SIZE-1;i++)
						MLINK_CRC(packet[i*2]);
					if(packet[14] == MLINK_BR(crc8))		// CRC is ok
					{
						debugln("CRC ok");
						packet_count=3;
					}
				}
			}
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Enable RX abort
			CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);		// Force end state
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		// Disable RX abort
			phase=MLINK_BIND_TX;							// Retry sending bind packet
			CYRF_SetTxRxMode(TX_EN);						// Transmit mode
			if(packet_count)
				return 18136;
		case MLINK_BIND_TX:
			if(--bind_counter==0 || packet_count>=0x1A*2)
			{ // Switch to normal mode
				BIND_DONE;
				phase=MLINK_PREP_DATA;
			}
			MLINK_send_bind_data_packet();
			if(packet_count == 0)
			{
				phase++;		// MLINK_BIND_PREP_RX
				return 4700;	// Original is 4900
			}
			packet_count++;
			if(packet_count&1)
				return 6000;
			return 22720;
		case MLINK_BIND_PREP_RX:
			start=micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 200)				// Wait max 200µs for TX to finish
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;										// Packet transmission complete
			CYRF_SetTxRxMode(RX_EN);							// Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x82);			// Prepare to receive
			phase++;	//MLINK_BIND_RX
			return 28712-4700;
		case MLINK_PREP_DATA:
			CYRF_ConfigDataCode(MLINK_Data_Code,16);
			hopping_frequency_no = 0x00;
			packet_count = 0;
			MLINK_build_data_packet();
			phase++;
		case MLINK_RF:
			/*sum=0;
			start=micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 200)				// Wait max 200µs for TX to finish
			{
				if(CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)
					break;										// Packet transmission complete
				sum++;
			}
			debugln("S:%d",sum);*/
			//Set RF channel
			CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
			//Set power
			CYRF_SetPower(0x38);
			phase++;	// MLINK_SEND
			return 3100;
		case MLINK_SEND:
			/*sum=0;
			start=micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 200)				// Wait max 200µs for TX to finish
			{
				if(CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)
					break;										// Packet transmission complete
				sum++;
			}
			debugln("SS:%d",sum);*/
			//Send packet
			CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x40);
			CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, packet, MLINK_PACKET_SIZE);
			CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x82);
			//Build next packet
			packet_count++;
			if(packet_count >= 78)
				packet_count=0;
			if(packet_count%3 == 0)
			{//Change RF channel every 3 packets
				phase=MLINK_RF;
				hopping_frequency_no++;
				if(hopping_frequency_no>=MLINK_NUM_FREQ)
					hopping_frequency_no=0;
			}
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(6000);
			#endif
			MLINK_build_data_packet();
			return 6000;
	}
	return 1000;
}

uint16_t initMLINK()
{ 
	MLINK_cyrf_config();
	
	#ifdef MLINK_FORCE_ID
		//Cockpit SX
		memcpy(MLINK_Data_Code,"\x4C\x97\x9D\xBF\xB8\x3D\xB5\xBE",8);
		for(uint8_t i=0;i<8;i++)
			MLINK_Data_Code[i+8]=MLINK_Data_Code[7-i];
		memcpy(hopping_frequency,"\x0D\x41\x09\x43\x17\x2D\x05\x31\x13\x3B\x1B\x3D\x0B\x41\x11\x45\x09\x2B\x17\x4D\x19\x3F\x03\x3F\x0F\x37\x1F\x47\x1B\x49\x07\x35\x27\x2F\x15\x33\x23\x39\x1F\x33\x19\x45\x0D\x2D\x11\x35\x0B\x47\x25\x3D\x21\x37\x1D\x3B\x05\x2F\x21\x39\x23\x4B\x03\x31\x25\x29\x07\x4F\x1D\x4B\x15\x4D\x13\x4F\x0F\x49\x29\x2B\x27\x43",78);
		MLINK_Offset = 0xF4;
	#endif

	debug("ID:")
	for(uint8_t i=0;i<16;i++)
		debug(" %02X", MLINK_Data_Code[i]);
	debugln("");

	debug("RF:")
	for(uint8_t i=0;i<MLINK_NUM_FREQ;i++)
		debug(" %02X", hopping_frequency[i]);
	debugln("");

	if(IS_BIND_IN_PROGRESS)
	{
		packet_count = 0;
		bind_counter = MLINK_BIND_COUNT;
		CYRF_ConfigDataCode((uint8_t*)"\x6F\xBE\x32\x01\xDB\xF1\x2B\x01\xE3\x5C\xFA\x02\x97\x93\xF9\x02",16); //Bind data code
		CYRF_ConfigRFChannel(MLINK_BIND_CHANNEL);
		phase = MLINK_BIND_TX;
	}
	else
		phase = MLINK_PREP_DATA;
	return 10000;
}

#endif
