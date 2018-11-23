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

#if defined(DSM_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define DSM_BIND_CHANNEL 0x0d //13 This can be any odd channel

//During binding we will send BIND_COUNT/2 packets
//One packet each 10msec
#define DSM_BIND_COUNT 300

enum {
	DSM_BIND_WRITE=0,
	DSM_BIND_CHECK,
	DSM_BIND_READ,
	DSM_CHANSEL,
	DSM_CH1_WRITE_A,
	DSM_CH1_CHECK_A,
	DSM_CH2_WRITE_A,
	DSM_CH2_CHECK_A,
	DSM_CH2_READ_A,
	DSM_CH1_WRITE_B,
	DSM_CH1_CHECK_B,
	DSM_CH2_WRITE_B,
	DSM_CH2_CHECK_B,
	DSM_CH2_READ_B,
};

//
uint8_t sop_col;
uint8_t DSM_num_ch=0;
uint8_t ch_map[14];
const uint8_t PROGMEM DSM_ch_map_progmem[][14] = {
//22+11ms for 4..7 channels
	{1, 0, 2, 3, 0xff, 0xff, 0xff, 1,    0,    2,    3, 0xff, 0xff,    0xff}, //4ch  - Guess
	{1, 0, 2, 3, 4,    0xff, 0xff, 1,    0,    2,    3,    4, 0xff,    0xff}, //5ch  - Guess
	{1, 5, 2, 3, 0,    4,    0xff, 1,    5,    2,    3,    0,    4,    0xff}, //6ch  - HP6DSM
	{1, 5, 2, 4, 3,    6,    0,    1,    5,    2,    4,    3,    6,    0   }, //7ch  - DX6i
//22ms for 8..12 channels
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    0xff, 0xff, 0xff, 0xff}, //8ch  - DX8/DX7
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    8,    0xff, 0xff, 0xff}, //9ch  - Guess
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    8,    9,    0xff, 0xff}, //10ch - Guess
	{1, 5, 2, 3, 6,    10,   0xff, 4,    0,    7,    8,    9,    0xff, 0xff}, //11ch - Guess
	{1, 5, 2, 4, 6,    10,   0xff, 0,    7,    3,    8,    9   , 11  , 0xff}, //12ch - DX18
//11ms for 8..12 channels
	{1, 5, 2, 3, 6,    7,    0xff, 1,    5,    2,    4,    0,    0xff, 0xff}, //8ch  - DX7
	{1, 5, 2, 3, 6,    7,    0xff, 1,    5,    2,    4,    0,    8,    0xff}, //9ch  - Guess
	{1, 5, 2, 3, 4,    8,    9,    1,    5,    2,    3,     0,   7,    6   }, //10ch - DX18
};

const uint8_t PROGMEM DSM_pncodes[5][8][8] = {
	/* Note these are in order transmitted (LSB 1st) */
	{ /* Row 0 */
		/* Col 0 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8},
		/* Col 1 */ {0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6},
		/* Col 2 */ {0xF1, 0x94, 0x30, 0x21, 0xA1, 0x1C, 0x88, 0xA9},
		/* Col 3 */ {0xD0, 0xD2, 0x8E, 0xBC, 0x82, 0x2F, 0xE3, 0xB4},
		/* Col 4 */ {0x8C, 0xFA, 0x47, 0x9B, 0x83, 0xA5, 0x66, 0xD0},
		/* Col 5 */ {0x07, 0xBD, 0x9F, 0x26, 0xC8, 0x31, 0x0F, 0xB8},
		/* Col 6 */ {0xEF, 0x03, 0x95, 0x89, 0xB4, 0x71, 0x61, 0x9D},
		/* Col 7 */ {0x40, 0xBA, 0x97, 0xD5, 0x86, 0x4F, 0xCC, 0xD1},
		/* Col 8    {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}*/
	},
	{ /* Row 1 */
		/* Col 0 */ {0x83, 0xF7, 0xA8, 0x2D, 0x7A, 0x44, 0x64, 0xD3},
		/* Col 1 */ {0x3F, 0x2C, 0x4E, 0xAA, 0x71, 0x48, 0x7A, 0xC9},
		/* Col 2 */ {0x17, 0xFF, 0x9E, 0x21, 0x36, 0x90, 0xC7, 0x82},
		/* Col 3 */ {0xBC, 0x5D, 0x9A, 0x5B, 0xEE, 0x7F, 0x42, 0xEB},
		/* Col 4 */ {0x24, 0xF5, 0xDD, 0xF8, 0x7A, 0x77, 0x74, 0xE7},
		/* Col 5 */ {0x3D, 0x70, 0x7C, 0x94, 0xDC, 0x84, 0xAD, 0x95},
		/* Col 6 */ {0x1E, 0x6A, 0xF0, 0x37, 0x52, 0x7B, 0x11, 0xD4},
		/* Col 7 */ {0x62, 0xF5, 0x2B, 0xAA, 0xFC, 0x33, 0xBF, 0xAF},
		/* Col 8    {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97} */
	},
	{ /* Row 2 */
		/* Col 0 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97},
		/* Col 1 */ {0x8E, 0x4A, 0xD0, 0xA9, 0xA7, 0xFF, 0x20, 0xCA},
		/* Col 2 */ {0x4C, 0x97, 0x9D, 0xBF, 0xB8, 0x3D, 0xB5, 0xBE},
		/* Col 3 */ {0x0C, 0x5D, 0x24, 0x30, 0x9F, 0xCA, 0x6D, 0xBD},
		/* Col 4 */ {0x50, 0x14, 0x33, 0xDE, 0xF1, 0x78, 0x95, 0xAD},
		/* Col 5 */ {0x0C, 0x3C, 0xFA, 0xF9, 0xF0, 0xF2, 0x10, 0xC9},
		/* Col 6 */ {0xF4, 0xDA, 0x06, 0xDB, 0xBF, 0x4E, 0x6F, 0xB3},
		/* Col 7 */ {0x9E, 0x08, 0xD1, 0xAE, 0x59, 0x5E, 0xE8, 0xF0},
		/* Col 8    {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E} */
	},
	{ /* Row 3 */
		/* Col 0 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E},
		/* Col 1 */ {0x80, 0x69, 0x26, 0x80, 0x08, 0xF8, 0x49, 0xE7},
		/* Col 2 */ {0x7D, 0x2D, 0x49, 0x54, 0xD0, 0x80, 0x40, 0xC1},
		/* Col 3 */ {0xB6, 0xF2, 0xE6, 0x1B, 0x80, 0x5A, 0x36, 0xB4},
		/* Col 4 */ {0x42, 0xAE, 0x9C, 0x1C, 0xDA, 0x67, 0x05, 0xF6},
		/* Col 5 */ {0x9B, 0x75, 0xF7, 0xE0, 0x14, 0x8D, 0xB5, 0x80},
		/* Col 6 */ {0xBF, 0x54, 0x98, 0xB9, 0xB7, 0x30, 0x5A, 0x88},
		/* Col 7 */ {0x35, 0xD1, 0xFC, 0x97, 0x23, 0xD4, 0xC9, 0x88},
		/* Col 8    {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93} */
// Wrong values used by Orange TX/RX
//		/* Col 8 */ {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
	},
	{ /* Row 4 */
		/* Col 0 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93},
		/* Col 1 */ {0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C},
		/* Col 2 */ {0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA},
		/* Col 3 */ {0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC},
		/* Col 4 */ {0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84},
		/* Col 5 */ {0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7},
		/* Col 6 */ {0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0},
		/* Col 7 */ {0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1},
		/* Col 8    {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8} */
	},
};

static void __attribute__((unused)) DSM_read_code(uint8_t *buf, uint8_t row, uint8_t col, uint8_t len)
{
	for(uint8_t i=0;i<len;i++)
		buf[i]=pgm_read_byte_near( &DSM_pncodes[row][col][i] );
}

static uint8_t __attribute__((unused)) DSM_get_pn_row(uint8_t channel)
{
	return ((sub_protocol == DSMX_11 || sub_protocol == DSMX_22 )? (channel - 2) % 5 : channel % 5);	
}

const uint8_t PROGMEM DSM_init_vals[][2] = {
	{CYRF_02_TX_CTRL, 0x00},				// All TX interrupt disabled
	{CYRF_05_RX_CTRL, 0x00},				// All RX interrupt disabled
	{CYRF_28_CLK_EN, 0x02},					// Force receive clock enable
	{CYRF_32_AUTO_CAL_TIME, 0x3c},			// Default init value
	{CYRF_35_AUTOCAL_OFFSET, 0x14},			// Default init value
	{CYRF_06_RX_CFG, 0x4A},					// LNA enabled, RX override enabled, Fast turn mode enabled, RX is 1MHz below TX
	{CYRF_1B_TX_OFFSET_LSB, 0x55},			// Default init value
	{CYRF_1C_TX_OFFSET_MSB, 0x05},			// Default init value
	{CYRF_39_ANALOG_CTRL, 0x01},			// All slow for synth setting time
	{CYRF_01_TX_LENGTH, 0x10},				// 16 bytes packet
	{CYRF_14_EOP_CTRL, 0x02},				// Set EOP Symbol Count to 2
	{CYRF_12_DATA64_THOLD, 0x0a},			// 64 Chip Data PN corelator threshold, default datasheet value is 0x0E
	//Below is for bind only
	{CYRF_03_TX_CFG, 0x38 | CYRF_BIND_POWER}, //64 chip codes, SDR mode
	{CYRF_10_FRAMING_CFG, 0x4a},			// SOP disabled, no LEN field and SOP correlator of 0x0a but since SOP is disabled...
	{CYRF_1F_TX_OVERRIDE, 0x04},			// Disable TX CRC, no ACK, use TX synthesizer
	{CYRF_1E_RX_OVERRIDE, 0x14},			// Disable RX CRC, Force receive data rate, use RX synthesizer
};

const uint8_t PROGMEM DSM_data_vals[][2] = {
	{CYRF_29_RX_ABORT, 0x20},				// Abort RX operation in case we are coming from bind
	{CYRF_0F_XACT_CFG, 0x24},				// Force Idle
	{CYRF_29_RX_ABORT, 0x00},				// Clear abort RX
	{CYRF_03_TX_CFG, 0x28 | CYRF_HIGH_POWER}, // 64 chip codes, 8DR mode
	{CYRF_10_FRAMING_CFG, 0xea},			// SOP enabled, SOP_CODE_ADR 64 chips, Packet len enabled, SOP correlator 0x0A
	{CYRF_1F_TX_OVERRIDE, 0x00},			// CRC16 enabled, no ACK
	{CYRF_1E_RX_OVERRIDE, 0x00},			// CRC16 enabled, no ACK
};

static void __attribute__((unused)) DSM_cyrf_config()
{
	for(uint8_t i = 0; i < sizeof(DSM_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&DSM_init_vals[i][0]), pgm_read_byte_near(&DSM_init_vals[i][1]));
	CYRF_WritePreamble(0x333304);
	CYRF_ConfigRFChannel(0x61);
}

static void __attribute__((unused)) DSM_build_bind_packet()
{
	uint8_t i;
	uint16_t sum = 384 - 0x10;//
	packet[0] = 0xff ^ cyrfmfg_id[0];
	packet[1] = 0xff ^ cyrfmfg_id[1];
	packet[2] = 0xff ^ cyrfmfg_id[2];
	packet[3] = 0xff ^ cyrfmfg_id[3];
	packet[4] = packet[0];
	packet[5] = packet[1];
	packet[6] = packet[2];
	packet[7] = packet[3];
	for(i = 0; i < 8; i++)
		sum += packet[i];
	packet[8] = sum >> 8;
	packet[9] = sum & 0xff;
	packet[10] = 0x01; //???
	packet[11] = DSM_num_ch;

	if (sub_protocol==DSM2_22)
		packet[12]=DSM_num_ch<8?0x01:0x02;	// DSM2/1024 1 or 2 packets depending on the number of channels
	if(sub_protocol==DSM2_11)
		packet[12]=0x12;					// DSM2/2048 2 packets
	if(sub_protocol==DSMX_22)
		#if defined DSM_TELEMETRY
			packet[12] = 0xb2;				// DSMX/2048 2 packets
		#else
			packet[12] = DSM_num_ch<8? 0xa2 : 0xb2;	// DSMX/2048 1 or 2 packets depending on the number of channels
		#endif
	if(sub_protocol==DSMX_11 || sub_protocol==DSM_AUTO) // Force DSMX/1024 in mode Auto
		packet[12]=0xb2;					// DSMX/1024 2 packets
	
	packet[13] = 0x00; //???
	for(i = 8; i < 14; i++)
		sum += packet[i];
	packet[14] = sum >> 8;
	packet[15] = sum & 0xff;
}

static void __attribute__((unused)) DSM_initialize_bind_phase()
{
	CYRF_ConfigRFChannel(DSM_BIND_CHANNEL); //This seems to be random?
	//64 SDR Mode is configured so only the 8 first values are needed but need to write 16 values...
	CYRF_ConfigDataCode((const uint8_t*)"\xD7\xA1\x54\xB1\x5E\x89\xAE\x86\xc6\x94\x22\xfe\x48\xe6\x57\x4e", 16);
	DSM_build_bind_packet();
}

static void __attribute__((unused)) DSM_cyrf_configdata()
{
	for(uint8_t i = 0; i < sizeof(DSM_data_vals) / 2; i++)
		CYRF_WriteRegister(pgm_read_byte_near(&DSM_data_vals[i][0]), pgm_read_byte_near(&DSM_data_vals[i][1]));
}

static void __attribute__((unused)) DSM_update_channels()
{
	prev_option=option;
	if(sub_protocol==DSM_AUTO)
		DSM_num_ch=12;						// Force 12 channels in mode Auto
	else
		DSM_num_ch=option;
	if(DSM_num_ch<4 || DSM_num_ch>12)
		DSM_num_ch=6;						// Default to 6 channels if invalid choice...

	// Create channel map based on number of channels and refresh rate
	uint8_t idx=DSM_num_ch-4;
	if(DSM_num_ch>7 && DSM_num_ch<11 && (sub_protocol==DSM2_11 || sub_protocol==DSMX_11))
		idx+=5;								// In 11ms mode change index only for channels 8..10
	for(uint8_t i=0;i<14;i++)
		ch_map[i]=pgm_read_byte_near(&DSM_ch_map_progmem[idx][i]);
}

static void __attribute__((unused)) DSM_build_data_packet(uint8_t upper)
{
	uint8_t bits = 11;

	if(prev_option!=option)
		DSM_update_channels();

	if (sub_protocol==DSMX_11 || sub_protocol==DSMX_22 )
	{
		packet[0] = cyrfmfg_id[2];
		packet[1] = cyrfmfg_id[3];
	}
	else
	{
		packet[0] = (0xff ^ cyrfmfg_id[2]);
		packet[1] = (0xff ^ cyrfmfg_id[3]);
		if(sub_protocol==DSM2_22)
			bits=10;						// Only DSM_22 is using a resolution of 1024
	}
	#ifdef DSM_THROTTLE_KILL_CH
		uint16_t kill_ch=Channel_data[DSM_THROTTLE_KILL_CH-1];
	#endif
	for (uint8_t i = 0; i < 7; i++)
	{	
		uint8_t idx = ch_map[(upper?7:0) + i];//1,5,2,3,0,4	   
		uint16_t value = 0xffff;;	
		if (idx != 0xff)
		{
			/* Spektrum own remotes transmit normal values during bind and actually use this (e.g. Nano CP X) to
			   select the transmitter mode (e.g. computer vs non-computer radio), so always send normal output */
			#ifdef DSM_THROTTLE_KILL_CH
				if(CH_TAER[idx]==THROTTLE && kill_ch<=604)
				{//Activate throttle kill only if DSM_THROTTLE_KILL_CH below -50%
					if(kill_ch<CHANNEL_MIN_100)					// restrict val to 0...400
						kill_ch=0;
					else
						kill_ch-=CHANNEL_MIN_100;
					value=(kill_ch*21)/25;			// kill channel -100%->904us ... -50%->1100us *0x150/400
				}
				else
			#endif
				#ifdef DSM_MAX_THROW
					value=Channel_data[CH_TAER[idx]];								// -100%..+100% => 1024..1976us and -125%..+125% => 904..2096us based on Redcon 6 channel DSM2 RX
				#else
					value=convert_channel_16b_nolimit(CH_TAER[idx],0x150,0x6B0);	// -100%..+100% => 1100..1900us and -125%..+125% => 1000..2000us based on Redcon 6 channel DSM2 RX
				#endif
			if(bits==10) value>>=1;
			value |= (upper && i==0 ? 0x8000 : 0) | (idx << bits);
		}	  
		packet[i*2+2] = (value >> 8) & 0xff;
		packet[i*2+3] = (value >> 0) & 0xff;
	}
}

static void __attribute__((unused)) DSM_set_sop_data_crc()
{
	//The crc for channel '1' is NOT(mfgid[0] << 8 + mfgid[1])
	//The crc for channel '2' is (mfgid[0] << 8 + mfgid[1])
	uint16_t crc = (cyrfmfg_id[0] << 8) + cyrfmfg_id[1];
	if(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B)
		CYRF_ConfigCRCSeed(crc);	//CH2
	else
		CYRF_ConfigCRCSeed(~crc);	//CH1

	uint8_t pn_row = DSM_get_pn_row(hopping_frequency[hopping_frequency_no]);
	uint8_t code[16];
	DSM_read_code(code,pn_row,sop_col,8);					// pn_row between 0 and 4, sop_col between 1 and 7
	CYRF_ConfigSOPCode(code);
	DSM_read_code(code,pn_row,7 - sop_col,8);				// 7-sop_col between 0 and 6
	DSM_read_code(code+8,pn_row,7 - sop_col + 1,8);			// 7-sop_col+1 between 1 and 7
	CYRF_ConfigDataCode(code, 16);

	CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
	hopping_frequency_no++;
	if(sub_protocol == DSMX_11 || sub_protocol == DSMX_22)
		hopping_frequency_no %=23;
	else
		hopping_frequency_no %=2;
}

static void __attribute__((unused)) DSM_calc_dsmx_channel()
{
	uint8_t idx = 0;
	uint32_t id = ~(((uint32_t)cyrfmfg_id[0] << 24) | ((uint32_t)cyrfmfg_id[1] << 16) | ((uint32_t)cyrfmfg_id[2] << 8) | (cyrfmfg_id[3] << 0));
	uint32_t id_tmp = id;
	while(idx < 23)
	{
		uint8_t i;
		uint8_t count_3_27 = 0, count_28_51 = 0, count_52_76 = 0;
		id_tmp = id_tmp * 0x0019660D + 0x3C6EF35F;		// Randomization
		uint8_t next_ch = ((id_tmp >> 8) % 0x49) + 3;	// Use least-significant byte and must be larger than 3
		if ( (next_ch ^ cyrfmfg_id[3]) & 0x01 )
			continue;
		for (i = 0; i < idx; i++)
		{
			if(hopping_frequency[i] == next_ch)
				break;
			if(hopping_frequency[i] <= 27)
				count_3_27++;
			else
				if (hopping_frequency[i] <= 51)
					count_28_51++;
				else
					count_52_76++;
		}
		if (i != idx)
			continue;
		if ((next_ch < 28 && count_3_27 < 8)
			||(next_ch >= 28 && next_ch < 52 && count_28_51 < 7)
			||(next_ch >= 52 && count_52_76 < 8))
			hopping_frequency[idx++] = next_ch;
	}
}

static uint8_t __attribute__((unused)) DSM_Check_RX_packet()
{
	uint8_t result=1;						// assume good packet
	
	uint16_t sum = 384 - 0x10;
	for(uint8_t i = 1; i < 9; i++)
	{
		sum += pkt[i];
		if(i<5)
			if(pkt[i] != (0xff ^ cyrfmfg_id[i-1]))
				result=0; 					// bad packet
	}
	if( pkt[9] != (sum>>8)  && pkt[10] != (uint8_t)sum )
		result=0;
	return result;
}

uint16_t ReadDsm()
{
#define DSM_CH1_CH2_DELAY	4010			// Time between write of channel 1 and channel 2
#define DSM_WRITE_DELAY		1950			// Time after write to verify write complete
#define DSM_READ_DELAY		600				// Time before write to check read phase, and switch channels. Was 400 but 600 seems what the 328p needs to read a packet
	#if defined DSM_TELEMETRY
		uint8_t rx_phase;
		uint8_t len;
	#endif
	uint8_t start;
	
	switch(phase)
	{
		case DSM_BIND_WRITE:
			if(bind_counter--==0)
			#if defined DSM_TELEMETRY
				phase=DSM_BIND_CHECK;						//Check RX answer
			#else
				phase=DSM_CHANSEL;							//Switch to normal mode
			#endif
			CYRF_WriteDataPacket(packet);
			return 10000;
	#if defined DSM_TELEMETRY
		case DSM_BIND_CHECK:
			//64 SDR Mode is configured so only the 8 first values are needed but we need to write 16 values...
			CYRF_ConfigDataCode((const uint8_t *)"\x98\x88\x1B\xE4\x30\x79\x03\x84\xC9\x2C\x06\x93\x86\xB9\x9E\xD7", 16);
			CYRF_SetTxRxMode(RX_EN);						//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);		//Prepare to receive
			bind_counter=2*DSM_BIND_COUNT;					//Timeout of 4.2s if no packet received
			phase++;										// change from BIND_CHECK to BIND_READ
			return 2000;
		case DSM_BIND_READ:
			//Read data from RX
			rx_phase = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x03) == 0x02)  					// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_phase |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x07) == 0x02)
			{ // data received with no errors
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				if(len>MAX_PKT-2)
					len=MAX_PKT-2;
				CYRF_ReadDataPacketLen(pkt+1, len);
				if(len==10 && DSM_Check_RX_packet())
				{
					pkt[0]=0x80;
					telemetry_link=1;						// send received data on serial
					phase++;
					return 2000;
				}
			}
			else
				if((rx_phase & 0x02) != 0x02)
				{ // data received with errors
					CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);	// Abort RX operation
					CYRF_SetTxRxMode(RX_EN);					// Force end state read
					CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX operation
					CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);	// Prepare to receive
				}
			if( --bind_counter == 0 )
			{ // Exit if no answer has been received for some time
				phase++;									// DSM_CHANSEL
				return 7000 ;
			}
			return 7000;
	#endif
		case DSM_CHANSEL:
			BIND_DONE;
			DSM_cyrf_configdata();
			CYRF_SetTxRxMode(TX_EN);
			hopping_frequency_no = 0;
			phase = DSM_CH1_WRITE_A;						// in fact phase++
			DSM_set_sop_data_crc();
			return 10000;
		case DSM_CH1_WRITE_A:
		case DSM_CH1_WRITE_B:
		case DSM_CH2_WRITE_A:
		case DSM_CH2_WRITE_B:
			DSM_build_data_packet(phase == DSM_CH1_WRITE_B||phase == DSM_CH2_WRITE_B);	// build lower or upper channels
			CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);		// clear IRQ flags
			CYRF_WriteDataPacket(packet);
			phase++;										// change from WRITE to CHECK mode
			return DSM_WRITE_DELAY;
		case DSM_CH1_CHECK_A:
		case DSM_CH1_CHECK_B:
		case DSM_CH2_CHECK_A:
		case DSM_CH2_CHECK_B:
			start=(uint8_t)micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 100)			// Wait max 100µs, max I've seen is 50µs
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;
			if(phase==DSM_CH1_CHECK_A || phase==DSM_CH1_CHECK_B)
			{
				#if defined DSM_TELEMETRY
					// reset cyrf6936 if freezed after switching from TX to RX
					if (((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x22) == 0x20) || (CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80))
					{
						CYRF_Reset();
						DSM_cyrf_config();
						DSM_cyrf_configdata();
						CYRF_SetTxRxMode(TX_EN);
					}
				#endif
				DSM_set_sop_data_crc();
				phase++;										// change from CH1_CHECK to CH2_WRITE
				return DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
			}
			if (phase == DSM_CH2_CHECK_A)
				CYRF_SetPower(0x28);						//Keep transmit power in sync
#if defined DSM_TELEMETRY
			phase++;										// change from CH2_CHECK to CH2_READ
			CYRF_SetTxRxMode(RX_EN);						//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);		//0x80??? //Prepare to receive
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY - DSM_READ_DELAY;
		case DSM_CH2_READ_A:
		case DSM_CH2_READ_B:
			//Read telemetry
			rx_phase = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x03) == 0x02)  					// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_phase |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x07) == 0x02)
			{ // good data (complete with no errors)
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				if(len>MAX_PKT-2)
					len=MAX_PKT-2;
				CYRF_ReadDataPacketLen(pkt+1, len);
				pkt[0]=CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;// store RSSI of the received telemetry signal
				telemetry_link=1;
			}
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Abort RX operation
			if (phase == DSM_CH2_READ_A && (sub_protocol==DSM2_22 || sub_protocol==DSMX_22) && DSM_num_ch < 8)	// 22ms mode
			{
				CYRF_SetTxRxMode(RX_EN);					// Force end state read
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX operation
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);	//0x80???	//Prepare to receive
				phase = DSM_CH2_READ_B;
				return 11000;
			}
			if (phase == DSM_CH2_READ_A)
				phase = DSM_CH1_WRITE_B;					//Transmit upper
			else
				phase = DSM_CH1_WRITE_A;					//Transmit lower
			CYRF_SetTxRxMode(TX_EN);						//TX mode
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		//Clear abort RX operation
			DSM_set_sop_data_crc();
			return DSM_READ_DELAY;
#else
			// No telemetry
			DSM_set_sop_data_crc();
			if (phase == DSM_CH2_CHECK_A)
			{
				if(DSM_num_ch > 7 || sub_protocol==DSM2_11 || sub_protocol==DSMX_11)
					phase = DSM_CH1_WRITE_B;				//11ms mode or upper to transmit change from CH2_CHECK_A to CH1_WRITE_A
				else										
				{											//Normal mode 22ms
					phase = DSM_CH1_WRITE_A;				// change from CH2_CHECK_A to CH1_WRITE_A (ie no upper)
					return 22000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
				}
			}
			else
				phase = DSM_CH1_WRITE_A;					// change from CH2_CHECK_B to CH1_WRITE_A (upper already transmitted so transmit lower)
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
#endif
	}
	return 0;		
}

uint16_t initDsm()
{ 
	CYRF_GetMfgData(cyrfmfg_id);
	//Model match
	cyrfmfg_id[3]^=RX_num;
	//Calc sop_col
	sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
	//Fix for OrangeRX using wrong DSM_pncodes by preventing access to "Col 8"
	if(sop_col==0)
	{
	   cyrfmfg_id[rx_tx_addr[0]%3]^=0x01;					//Change a bit so sop_col will be different from 0
	   sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
	}
	//Hopping frequencies
	if (sub_protocol == DSMX_11 || sub_protocol == DSMX_22)
		DSM_calc_dsmx_channel();
	else
	{ 
		uint8_t tmpch[10];
		CYRF_FindBestChannels(tmpch, 10, 5, 3, 75);
		//
		uint8_t idx = random(0xfefefefe) % 10;
		hopping_frequency[0] = tmpch[idx];
		while(1)
		{
			idx = random(0xfefefefe) % 10;
			if (tmpch[idx] != hopping_frequency[0])
				break;
		}
		hopping_frequency[1] = tmpch[idx];
	}
	//
	DSM_cyrf_config();
	CYRF_SetTxRxMode(TX_EN);
	//
	DSM_update_channels();
	//
	if(IS_BIND_IN_PROGRESS)
	{
		DSM_initialize_bind_phase();		
		phase = DSM_BIND_WRITE;
		bind_counter=DSM_BIND_COUNT;
	}
	else
		phase = DSM_CHANSEL;//
	return 10000;
}

#endif
