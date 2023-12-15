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
// Compatible with CADET PRO V4 TX

#if defined(PELIKAN_A7105_INO)

#include "iface_a7105.h"

//#define PELIKAN_FORCE_ID
//#define PELIKAN_LITE_FORCE_ID
#define PELIKAN_LITE_FORCE_HOP				// hop sequence creation is unknown
//#define PELIKAN_SCX24_FORCE_ID
//#define PELIKAN_SCX24_FORCE_HOP				// hop sequence creation is unknown

#define PELIKAN_BIND_COUNT		400			// 3sec
#define PELIKAN_BIND_RF			0x3C
#define PELIKAN_NUM_RF_CHAN 	0x1D
#define PELIKAN_PACKET_PERIOD	7980
#define PELIKAN_LITE_PACKET_PERIOD 18000
#define PELIKAN_SCX24_PACKET_PERIOD 15069
#define PELIKAN_SCX_HOP_LIMIT 90

static void __attribute__((unused)) pelikan_build_packet()
{
	static boolean upper=false;
	uint8_t sum;
	uint16_t channel;

	if(sub_protocol == PELIKAN_SCX24)
		packet[0] = 0x11;
	else //PELIKAN_PRO & PELIKAN_LITE
		packet[0] = 0x15;
    if(IS_BIND_IN_PROGRESS)
	{
		packet[2] = rx_tx_addr[0];
		packet[3] = rx_tx_addr[1];
		packet[4] = rx_tx_addr[2];
		packet[5] = rx_tx_addr[3];

		if(sub_protocol == PELIKAN_SCX24)
		{
			packet[1] = 0x65;				//??
			packet[6] = 0x55;				//??
			packet[7] = 0xAA;				//??
		}
		else
		{//PELIKAN_PRO & PELIKAN_LITE
			packet[1] = 0x04;				//version??
			if(sub_protocol==PELIKAN_PRO)
				packet[6] = 0x05;			//sub version??
			else //PELIKAN_LITE
				packet[6] = 0x03;			//sub version??
			packet[7] = 0x00;				//??
		}
		packet[8] = 0x55;					//??
		packet_length = 10;
	}
	else
	{
		//ID
		packet[1]  = rx_tx_addr[0];
		if(sub_protocol == PELIKAN_SCX24)
		{
			//ID
			packet[4]  = rx_tx_addr[1];
			//Channels
			channel = Channel_data[0];		//STEERING: 1B1..23B..2C5 ???
			packet[2]  = channel >> 9;
			packet[3]  = channel >> 1;
			channel = Channel_data[1];		//THROTTLE: 0DB..1FF..30E
			packet[5]  = channel >> 9;
			packet[6]  = channel >> 1;
			channel = Channel_data[2];		//CH3: 055..3AA
			packet[7]  = channel >> 9;
			packet[8]  = channel >> 1;
			//Hopping counters
			if(++packet_count>2)
			{
				packet_count=0;
				if(++hopping_frequency_no>=PELIKAN_NUM_RF_CHAN)
					hopping_frequency_no=0;
			}
			//Length
			packet_length = 14;
		}
		else
		{//PELIKAN_PRO & PELIKAN_LITE
			//ID
			packet[7]  = rx_tx_addr[1];
			//Channels
			uint8_t offset=upper?4:0;
			channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871,false);
			uint8_t top=(channel>>2) & 0xC0;
			packet[2]  = channel;
			channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871,false);
			top|=(channel>>4) & 0x30;
			packet[3]  = channel;
			channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871,false);
			top|=(channel>>6) & 0x0C;
			packet[4]  = channel;
			channel=convert_channel_16b_nolimit(CH_AETR[offset], 153, 871,false);
			top|=(channel>>8) & 0x03;
			packet[5]  = channel;
			packet[6]  = top;
			//Check
			sum=0x00;
			for(uint8_t i=0;i<8;i++)
				sum+=packet[i];
			packet[8]=sum;
			//Low/Up channel flag
			packet[9]=upper?0xAA:0x00;
			upper=!upper;
			//Hopping counters
			if(sub_protocol==PELIKAN_LITE || ++packet_count>4)
			{
				packet_count=0;
				if(++hopping_frequency_no>=PELIKAN_NUM_RF_CHAN)
					hopping_frequency_no=0;
			}
			//Length
			packet_length = 15;
		}
		//Hopping
		packet[packet_length-5] = hopping_frequency_no;
		packet[packet_length-4] = packet_count;
		//ID
		packet[packet_length-3] = rx_tx_addr[2];
		packet[packet_length-2] = rx_tx_addr[3];
	}

	//Check
	sum=0x00;
	for(uint8_t i=0; i<packet_length-1 ;i++)
		sum+=packet[i];
	packet[packet_length-1] = sum;

	//Send
	#ifdef DEBUG_SERIAL
		debug("C: %02X P(%d):",IS_BIND_IN_PROGRESS?PELIKAN_BIND_RF:hopping_frequency[hopping_frequency_no],packet_length);
		for(uint8_t i=0;i<packet_length;i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
	A7105_WriteData(packet_length, IS_BIND_IN_PROGRESS?PELIKAN_BIND_RF:hopping_frequency[hopping_frequency_no]);
	A7105_SetPower();
}

uint16_t PELIKAN_callback()
{
	if(phase==0)
	{
		#ifndef FORCE_PELIKAN_TUNING
			A7105_AdjustLOBaseFreq(1);
		#endif
		if(IS_BIND_IN_PROGRESS)
		{
			bind_counter--;
			if (bind_counter==0)
			{
				BIND_DONE;
				A7105_Strobe(A7105_STANDBY);
				if(sub_protocol==PELIKAN_PRO)
					A7105_WriteReg(A7105_03_FIFOI,0x28); //????
				else if(sub_protocol==PELIKAN_SCX24)
					A7105_WriteReg(A7105_03_FIFOI,0x0D);
				else//PELIKAN_LITE
					A7105_WriteID(MProtocol_id);
			}
		}
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(packet_period);
		#endif
		pelikan_build_packet();
		if(IS_BIND_IN_PROGRESS || sub_protocol != PELIKAN_LITE)
			return packet_period;
		//PELIKAN_LITE
		phase++;
		return 942;
	}
	//PELIKAN_LITE
	A7105_Strobe(A7105_TX);
	phase++;
	if(phase==1)
		return 942;
	phase=0;
	return PELIKAN_LITE_PACKET_PERIOD-942-942;
}

static uint8_t pelikan_firstCh(uint8_t u, uint8_t l)
{
	int16_t i;
	i = u * 10 + l - 23;
	do
	{
		if (i > 24)
			i -= 24;
		if (i <= 0)
			return 10;
		else if ((i > 0) && (i < 13))
			return 10 + 12 + (i * 4);
		else if ((i > 12) && (i < 24))
			return 10 - 2 + ((i - 12) * 4);
	}
	while (i > 24);
	return 0;
}

static uint8_t pelikan_firstCh_scx(uint8_t i, uint8_t j)
{
	uint8_t ch;
	switch (j) {
		case 0:
			ch = 30;
			break;
		case 1:
			ch = (i * 4) + 42;
			break;
		case 2:
			ch = (i * 4) + 42;
			break;
		case 3:
			ch = (i * 2) + 36;
			break;
		case 4:
			ch = (i * 8) + 54;
			break;
		case 5:
			ch = 30;
			break;
	}

	if (ch > PELIKAN_SCX_HOP_LIMIT)
	{
		do
		{
			ch -= 62;
		} while (ch > PELIKAN_SCX_HOP_LIMIT);
	}

	switch (ch) {
		case 48:
			if (j == 3)
				ch += 18;
			else if (j == 4)
				ch += 20;
			else
				ch += 40;
			break;
		case 40:
			if (j == 4)
				ch += 18;
			break;
		case 52:
			if (j < 3)
				ch -= 20;
			else if (j == 4)
				ch -= 10;
			break;
		case 66:
			if  (j < 3)
				ch += 18;
			else if (j == 4)
				ch -= 22;
			break;
		case 72:
			if (j < 3)
				ch -= 10;
			else if (j ==3)
				ch -= 20;
			else if (j == 4)
				ch -= 36;
			break;
		case 74:
			if (j == 4)
				ch -= 20;
			break;
		case 86:
			if (j == 4)
				ch -= 48;
			break;
	}

	return ch;
}

static uint8_t pelikan_adjust_value(uint8_t value, uint8_t addition, uint8_t limit)
{
	uint8_t i;
	do
	{
		i = 0;
		if (value > limit) {
			value -= 62;
			i++;
		}
		if (value == 24) {
			value += addition;
			i++;
		}
		if (value == 48) {
			value += addition;
			i++;
		}
		if (value == 72) {
			value += addition;
			i++;
		}
	}
	while (i > 0);

	return value;
}

static uint8_t pelikan_add(uint8_t pfrq,uint8_t a, uint8_t limit)
{
	uint8_t nfrq;
	nfrq = pfrq + a;

	nfrq = pelikan_adjust_value(nfrq, a, limit);
	
	return nfrq;
}

static void pelikan_shuffle(uint8_t j)
{
	uint8_t map[4][PELIKAN_NUM_RF_CHAN] = 
		{
			{0,1,2,26,27,28,23,24,25,20,21,22,17,18,19,14,15,16,11,12,13,8,9,10,5,6,7,4,3},
			{0,1,2,28,25,26,27,24,21,22,23,20,17,18,19,16,13,14,15,12,9,10,11,8,5,6,7,3,4},
			{0,1,27,28,25,26,23,24,21,22,19,20,17,18,15,16,13,14,11,12,9,10,7,8,5,6,3,4,2},
			{0,1,28,1,4,2,23,26,22,24,27,25,17,20,16,18,21,19,11,14,10,12,15,13,27,8,6,7,9}
		};

	uint8_t temp[PELIKAN_NUM_RF_CHAN];
	for (uint8_t i = 0; i < PELIKAN_NUM_RF_CHAN; i++)
	{
		temp[i] = hopping_frequency[map[j-1][i]];
	}

	for (uint8_t i = 0; i < PELIKAN_NUM_RF_CHAN; i++)
	{
		hopping_frequency[i] = temp[i];
	}
}

static void __attribute__((unused)) pelikan_init_hop()
{
	#define PELIKAN_HOP_LIMIT 70
	rx_tx_addr[0] = 0;
	rx_tx_addr[1]+= RX_num;
	uint8_t high = (rx_tx_addr[1]>>4) % 3;	// 0..2
	uint8_t low = rx_tx_addr[1] & 0x0F;
	if(high==2)
		low %= 0x04;	// 0..3
	else if(high)
		low %= 0x0E;	// 0..D
	else
		low %= 0x0F;	// 0..E
	rx_tx_addr[1] = (high<<4) + low;
	uint8_t addition = (20 * high)+ (2 * low) + 8;

	uint8_t first_channel = pelikan_firstCh(high, low);
	first_channel = pelikan_adjust_value(first_channel, addition, PELIKAN_HOP_LIMIT);
	hopping_frequency[0] = first_channel;
	debug("%02X", first_channel);
	for (uint8_t i = 1; i < PELIKAN_NUM_RF_CHAN; i++)
	{
		hopping_frequency[i] = pelikan_add(hopping_frequency[i-1], addition, PELIKAN_HOP_LIMIT);
		debug(" %02X", hopping_frequency[i]);
	}
	debugln("");
}

static void __attribute__((unused)) pelikan_init_hop_scx()
{
	rx_tx_addr[0] = 0x10;
	rx_tx_addr[1] = (rx_tx_addr[1] + RX_num) % 192;
	debugln("TX[0]: %02X TX[1]: %02X", rx_tx_addr[0], rx_tx_addr[1]);
	
	uint8_t high = (rx_tx_addr[1]>>4);
	uint8_t low = rx_tx_addr[1] & 0x0F;
	int16_t i = (high * 10) + low - 23;
	uint8_t j = 0;

	if (i > 0)
		j = 1;
	
	if (i > 24)
	{
		do
		{
			i -= 24;
			j++;
		} while (i > 24);
	}

	debugln("H: %02X L: %02X I: %02X J: %02X", high, low, i, j);

	uint8_t first_channel;
	uint8_t last_channel;
	uint8_t addition;

	first_channel = pelikan_firstCh_scx(i, j);

	if (j == 0)
		last_channel = 42 - (high * 10) - low;
	else
		last_channel = 42 - i + 1;

	if (last_channel == 24)
		last_channel += 9;
	
	if (last_channel == 36)
		last_channel -= 10;

	if (j == 0)
		addition = (2 * i) + 54;
	else if (j == 5)
		addition = (2 * i) + 6;
	else
		addition = 56 - (2 * i);

	hopping_frequency[0] = first_channel;
	for (uint8_t i = 1; i < PELIKAN_NUM_RF_CHAN; i++)
	{
		hopping_frequency[i] = pelikan_add(hopping_frequency[i-1], addition, PELIKAN_SCX_HOP_LIMIT);
	}

	if (j > 0 && j < 5)
		pelikan_shuffle(j);

	if (j == 2)
	{
		hopping_frequency[PELIKAN_NUM_RF_CHAN - 2] = last_channel;
	} else if (j == 4)
	{
		uint8_t t = (2 * i) + 36;
		if (t == 48)
			t += 18;
		if (t == 72)
			t -= 20;

		hopping_frequency[1] = t;
		hopping_frequency[PELIKAN_NUM_RF_CHAN - 5] = last_channel;
	}	
	else
	{
		hopping_frequency[PELIKAN_NUM_RF_CHAN - 1] = last_channel;
	}

	for (uint8_t i = 0; i < PELIKAN_NUM_RF_CHAN; i++)
	{
		debug("%02X ", hopping_frequency[i]);
	}
	debugln("");
}

#ifdef PELIKAN_FORCE_ID
const uint8_t PROGMEM pelikan_hopp[][PELIKAN_NUM_RF_CHAN] = {
	{ 0x5A,0x46,0x32,0x6E,0x6C,0x58,0x44,0x42,0x40,0x6A,0x56,0x54,0x52,0x3E,0x68,0x66,0x64,0x50,0x3C,0x3A,0x38,0x62,0x4E,0x4C,0x5E,0x4A,0x36,0x5C,0x34 }
};
#endif

#ifdef PELIKAN_LITE_FORCE_HOP
const uint8_t PROGMEM pelikan_lite_hopp[][PELIKAN_NUM_RF_CHAN] = {
	{ 0x46,0x2A,0x3E,0x5A,0x5C,0x24,0x4E,0x32,0x54,0x26,0x2C,0x34,0x56,0x1E,0x3A,0x3C,0x50,0x4A,0x2E,0x42,0x20,0x52,0x28,0x22,0x44,0x58,0x36,0x38,0x4C }
};
#endif
#ifdef PELIKAN_SCX24_FORCE_HOP
const uint8_t PROGMEM pelikan_scx24_hopp[][PELIKAN_NUM_RF_CHAN] = {
/*TX1*/	{ 0x1E,0x32,0x46,0x5A,0x44,0x58,0x2E,0x42,0x56,0x2C,0x40,0x54,0x2A,0x3E,0x52,0x28,0x3C,0x50,0x26,0x3A,0x4E,0x24,0x38,0x4C,0x22,0x36,0x4A,0x20,0x1A },
/*TX2*/	{ 0x2C,0x44,0x1E,0x52,0x56,0x22,0x3A,0x3E,0x34,0x4C,0x26,0x5A,0x50,0x2A,0x42,0x38,0x2E,0x46,0x20,0x54,0x4A,0x24,0x3C,0x32,0x28,0x40,0x58,0x1B,0x4E },
/*TX3*/	{ 0x3C,0x4C,0x1E,0x4A,0x5A,0x2C,0x58,0x2A,0x3A,0x56,0x28,0x38,0x26,0x36,0x46,0x34,0x44,0x54,0x42,0x52,0x24,0x50,0x22,0x32,0x4E,0x20,0x40,0x3E,0x17 },
/*TX4*/	{ 0x46,0x32,0x1E,0x58,0x44,0x5A,0x56,0x42,0x2E,0x54,0x40,0x2C,0x52,0x3E,0x2A,0x50,0x3C,0x28,0x4E,0x3A,0x26,0x4C,0x38,0x24,0x4A,0x36,0x22,0x20,0x1A }
};
#endif

void PELIKAN_init()
{
	A7105_Init();
	if(IS_BIND_IN_PROGRESS || sub_protocol==PELIKAN_LITE)
		A7105_WriteReg(A7105_03_FIFOI,0x10);

	bind_counter = PELIKAN_BIND_COUNT;

	if(sub_protocol==PELIKAN_PRO)
	{
		pelikan_init_hop();
		//ID from dump
		#if defined(PELIKAN_FORCE_ID)
			rx_tx_addr[0]=0x0D;		// hopping freq
			rx_tx_addr[1]=0xF4;		// hopping freq
			rx_tx_addr[2]=0x50;		// ID
			rx_tx_addr[3]=0x18;		// ID
			// Fill frequency table
			for(uint8_t i=0;i<PELIKAN_NUM_RF_CHAN;i++)
				hopping_frequency[i]=pgm_read_byte_near(&pelikan_hopp[0][i]);
		#endif
		packet_period = PELIKAN_PACKET_PERIOD;
	}
	else
	{
		bind_counter >>= 1;
		if(sub_protocol==PELIKAN_LITE)
		{
			#if defined(PELIKAN_LITE_FORCE_HOP)
				// Hop frequency table
				rx_tx_addr[0]=0x04;		// hopping freq
				rx_tx_addr[1]=0x63;		// hopping freq
				for(uint8_t i=0;i<PELIKAN_NUM_RF_CHAN;i++)
					hopping_frequency[i]=pgm_read_byte_near(&pelikan_lite_hopp[0][i]);
			#endif
			#if defined(PELIKAN_LITE_FORCE_ID)
				// ID
				rx_tx_addr[2]=0x60;
				rx_tx_addr[3]=0x18;
			#endif
			MProtocol_id = ((uint32_t)rx_tx_addr[0]<<24)|((uint32_t)rx_tx_addr[1]<<16)|((uint32_t)rx_tx_addr[2]<<8)|(rx_tx_addr[3]);
			if(IS_BIND_DONE)
				A7105_WriteID(MProtocol_id);
			packet_period = PELIKAN_LITE_PACKET_PERIOD;
		}
		else// if(sub_protocol==PELIKAN_SCX24)
		{
			pelikan_init_hop_scx();
			#if defined(PELIKAN_SCX24_FORCE_HOP)
				// Hop frequency table
				uint8_t num=rx_tx_addr[3] & 0x03;
				switch(num)
				{
					case 1:
						rx_tx_addr[0]=0x10;		// hopping freq TX2
						rx_tx_addr[1]=0x63;		// hopping freq TX2
						break;
					case 2:
						rx_tx_addr[0]=0x81;		// hopping freq TX3
						rx_tx_addr[1]=0x63;		// hopping freq TX3
						break;
					case 3:
						rx_tx_addr[0]=0x36;		// hopping freq TX4
						rx_tx_addr[1]=0x5C;		// hopping freq TX4
						break;
					default:
						rx_tx_addr[0]=0x12;		// hopping freq TX1
						rx_tx_addr[1]=0x46;		// hopping freq TX1
						break;
				}
				for(uint8_t i=0;i<PELIKAN_NUM_RF_CHAN;i++)
					hopping_frequency[i]=pgm_read_byte_near(&pelikan_scx24_hopp[num][i]);
			#endif
			#if defined(PELIKAN_SCX24_FORCE_ID)
				// ID
				rx_tx_addr[2]=0x80;			// TX1
				rx_tx_addr[3]=0x19;			// TX1
				rx_tx_addr[2]=0x80;			// TX2
				rx_tx_addr[3]=0x22;			// TX2
				rx_tx_addr[2]=0x30;			// TX3
				rx_tx_addr[3]=0x18;			// TX3
				rx_tx_addr[2]=0x30;			// TX4
				rx_tx_addr[3]=0x17;			// TX4
			#endif
			A7105_WriteReg(A7105_0E_DATA_RATE,0x03);
			if(IS_BIND_DONE)
				A7105_WriteReg(A7105_03_FIFOI,0x0D);
			packet_period = PELIKAN_SCX24_PACKET_PERIOD;
		}
	}

	hopping_frequency_no = PELIKAN_NUM_RF_CHAN;
	packet_count = 5;
	phase = 0;
	bind_counter = PELIKAN_BIND_COUNT;
}
#endif
