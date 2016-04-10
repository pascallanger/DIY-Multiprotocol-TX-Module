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

#if defined(DSM2_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define RANDOM_CHANNELS  0		// disabled
//#define RANDOM_CHANNELS  1	// enabled
#define BIND_CHANNEL 0x0d //13 This can be any odd channel

//During binding we will send BIND_COUNT/2 packets
//One packet each 10msec
#define BIND_COUNT1 600

enum {
	DSM2_BIND = 0,
	DSM2_CHANSEL     = BIND_COUNT1 + 0,
	DSM2_CH1_WRITE_A = BIND_COUNT1 + 1,
	DSM2_CH1_CHECK_A = BIND_COUNT1 + 2,
	DSM2_CH2_WRITE_A = BIND_COUNT1 + 3,
	DSM2_CH2_CHECK_A = BIND_COUNT1 + 4,
	DSM2_CH2_READ_A  = BIND_COUNT1 + 5,
	DSM2_CH1_WRITE_B = BIND_COUNT1 + 6,
	DSM2_CH1_CHECK_B = BIND_COUNT1 + 7,
	DSM2_CH2_WRITE_B = BIND_COUNT1 + 8,
	DSM2_CH2_CHECK_B = BIND_COUNT1 + 9,
	DSM2_CH2_READ_B  = BIND_COUNT1 + 10,
};

const uint8_t PROGMEM pncodes[5][9][8] = {
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
		/* Col 8 */ {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}
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
		/* Col 8 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97}
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
		/* Col 8 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E}
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
		/* Col 8 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93}
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
		/* Col 8 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8}
	},
};

static void __attribute__((unused)) read_code(uint8_t *buf, uint8_t row, uint8_t col, uint8_t len)
{
	for(uint8_t i=0;i<len;i++)
		buf[i]=pgm_read_byte_near( &pncodes[row][col][i] );
}

//
uint8_t sop_col;
uint8_t data_col;
uint16_t cyrf_state;
uint8_t binding;

static void __attribute__((unused)) build_bind_packet()
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
	packet[11] = option>3?option:option+4;
	if(sub_protocol==DSMX)	//DSMX type
#if defined DSM_TELEMETRY
		packet[12] = 0xb2;	// Telemetry on
#else
		packet[12] = option<8? 0xa2 : 0xb2;	// Telemetry off
#endif
	else
		packet[12] = option<8?0x01:0x02;
	packet[13] = 0x00; //???
	for(i = 8; i < 14; i++)
		sum += packet[i];
	packet[14] = sum >> 8;
	packet[15] = sum & 0xff;
}

static uint8_t __attribute__((unused)) PROTOCOL_SticksMoved(uint8_t init)
{
#define STICK_MOVEMENT 15*(PPM_MAX-PPM_MIN)/100	// defines when the bind dialog should be interrupted (stick movement STICK_MOVEMENT %)
	static uint16_t ele_start, ail_start;
    uint16_t ele = Servo_data[ELEVATOR];//CHAN_ReadInput(MIXER_MapChannel(INP_ELEVATOR));
    uint16_t ail = Servo_data[AILERON];//CHAN_ReadInput(MIXER_MapChannel(INP_AILERON));
    if(init) {
        ele_start = ele;
        ail_start = ail;
        return 0;
    }
    uint16_t ele_diff = ele_start - ele;//abs(ele_start - ele);
    uint16_t ail_diff = ail_start - ail;//abs(ail_start - ail);
    return ((ele_diff + ail_diff) > STICK_MOVEMENT);//
}

static void __attribute__((unused)) build_data_packet(uint8_t upper)//
{
	uint8_t i;
	uint8_t bits;

	uint8_t ch_map[] = {3, 2, 1, 5, 0, 4, 6, 7, 8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};	//9 Channels - DM9 TX
	switch(option>3?option:option+4)										// Create channel map based on number of channels
	{
		case 12:
			ch_map[11]=11;													// 12 channels
		case 11:
			ch_map[10]=10;													// 11 channels
		case 10:
			ch_map[9]=9;													// 10 channels
			break;
		case 8:
			memcpy(ch_map,"\x01\x05\x02\x03\x06\xFF\xFF\x04\x00\x07",10);	// 8 channels	- DX8 TX
			break;
		case 7:
			memcpy(ch_map,"\x01\x05\x02\x04\x03\x06\x00",7);				// 7 channels	- DX6i TX
			break;
		case 6:
			memcpy(ch_map,"\x01\x05\x02\x03\x00\x04\xFF",7);				// 6 channels	- HP6DSM TX
			break;
		case 4:
		case 5:
			memcpy(ch_map,"\x00\x01\x02\x03\xFF\xFF\xFF",7);				// 4 channels	- Guess
			if(option&0x01)
				ch_map[4]=4;												// 5 channels	- Guess
			break;
	}
	//
	if( binding && PROTOCOL_SticksMoved(0) )
		binding = 0;
	if (sub_protocol==DSMX)
	{
		packet[0] = cyrfmfg_id[2];
		packet[1] = cyrfmfg_id[3];
		bits=11;
	}
	else
	{
		packet[0] = (0xff ^ cyrfmfg_id[2]);
		packet[1] = (0xff ^ cyrfmfg_id[3]);
		bits=10;
	}
	//
	uint16_t max = 1 << bits;//max=2048 for DSMX & 1024 for DSM2 less than 8 ch and 2048 otherwise
	//uint16_t pct_100 = (uint32_t)max * 100 / 150;//682  1024*100/150
	//
	for (i = 0; i < 7; i++)
	{	
		uint8_t idx = ch_map[upper * 7 + i];//1,5,2,3,0,4	   
		uint16_t value;	
		if (idx == 0xff)
			value = 0xffff;
		else
		{
			if (binding)
			{ // Failsafe position during binding
				value=max/2;	//all channels to middle
				if(idx==0)
					value=1;	//except throttle
			}
			else
			{
				switch(idx)
				{
					case 0:
						value=Servo_data[THROTTLE];//85.75-938.25=125%//171-853=100%
						break;
					case 1:
						value=Servo_data[AILERON];
						break;
					case 2:
						value=Servo_data[ELEVATOR];
						break;
					case 3:
						value=Servo_data[RUDDER];
						break;
					case 4:
						value=Servo_data[AUX1];
						break;
					case 5:
						value=Servo_data[AUX2];
						break;
					case 6:
						value=Servo_data[AUX3];
						break;
					case 7:
						value=Servo_data[AUX4];
						break;
					case 8:
						value=Servo_data[AUX5];
						break;
					case 9:
						value=Servo_data[AUX6];
						break;
					case 10:
						value=Servo_data[AUX7];
						break;
					case 11:
						value=Servo_data[AUX8];
						break;
				}
				value=map(value,PPM_MIN,PPM_MAX,0,max-1);
			}		   
			value |= (upper && i == 0 ? 0x8000 : 0) | (idx << bits);
		}	  
		packet[i*2+2] = (value >> 8) & 0xff;
		packet[i*2+3] = (value >> 0) & 0xff;
	}
}

static uint8_t __attribute__((unused)) get_pn_row(uint8_t channel)
{
	return (sub_protocol == DSMX ? (channel - 2) % 5 : channel % 5);	
}

const uint8_t init_vals[][2] = {
	{CYRF_02_TX_CTRL, 0x02},	//0x00 in deviation but needed to know when transmit is over
	{CYRF_05_RX_CTRL, 0x00},
	{CYRF_28_CLK_EN, 0x02},
	{CYRF_32_AUTO_CAL_TIME, 0x3c},
	{CYRF_35_AUTOCAL_OFFSET, 0x14},
	{CYRF_06_RX_CFG, 0x4A},
	{CYRF_1B_TX_OFFSET_LSB, 0x55},
	{CYRF_1C_TX_OFFSET_MSB, 0x05},
	{CYRF_0F_XACT_CFG, 0x24}, // Force Idle
	{CYRF_03_TX_CFG, 0x38 | CYRF_BIND_POWER}, //Set 64chip, SDR mode
	{CYRF_12_DATA64_THOLD, 0x0a},
	{CYRF_0F_XACT_CFG, 0x04}, // Idle
	{CYRF_39_ANALOG_CTRL, 0x01},
	{CYRF_0F_XACT_CFG, 0x24}, //Force IDLE
	{CYRF_29_RX_ABORT, 0x00}, //Clear RX abort
	{CYRF_12_DATA64_THOLD, 0x0a}, //set pn correlation threshold
	{CYRF_10_FRAMING_CFG, 0x4a}, //set sop len and threshold
	{CYRF_29_RX_ABORT, 0x0f}, //Clear RX abort?
	{CYRF_03_TX_CFG, 0x38 | CYRF_BIND_POWER}, //Set 64chip, SDR mode
	{CYRF_10_FRAMING_CFG, 0x4a}, //set sop len and threshold
	{CYRF_1F_TX_OVERRIDE, 0x04}, //disable tx CRC
	{CYRF_1E_RX_OVERRIDE, 0x14}, //disable rx crc
	{CYRF_14_EOP_CTRL, 0x02}, //set EOP sync == 2
	{CYRF_01_TX_LENGTH, 0x10}, //16byte packet
};

static void __attribute__((unused)) cyrf_config()
{
	for(uint8_t i = 0; i < sizeof(init_vals) / 2; i++)	
		CYRF_WriteRegister(init_vals[i][0], init_vals[i][1]);
	CYRF_WritePreamble(0x333304);
	CYRF_ConfigRFChannel(0x61);
}

static void __attribute__((unused)) initialize_bind_state()
{
	uint8_t code[32];

	CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
	uint8_t pn_row = get_pn_row(BIND_CHANNEL);
	//printf("Ch: %d Row: %d SOP: %d Data: %d\n", BIND_CHANNEL, pn_row, sop_col, data_col);
	CYRF_ConfigCRCSeed(crc);

	read_code(code,pn_row,sop_col,8);
	CYRF_ConfigSOPCode(code);
	read_code(code,pn_row,data_col,16);
	read_code(code+16,0,8,8);
	memcpy(code + 24, (void *)"\xc6\x94\x22\xfe\x48\xe6\x57\x4e", 8);
	CYRF_ConfigDataCode(code, 32);

	build_bind_packet();
}

const uint8_t data_vals[][2] = {
	{CYRF_05_RX_CTRL, 0x83}, //Initialize for reading RSSI
	{CYRF_29_RX_ABORT, 0x20},
	{CYRF_0F_XACT_CFG, 0x24},
	{CYRF_29_RX_ABORT, 0x00},
	{CYRF_03_TX_CFG, 0x08 | CYRF_HIGH_POWER},
	{CYRF_10_FRAMING_CFG, 0xea},
	{CYRF_1F_TX_OVERRIDE, 0x00},
	{CYRF_1E_RX_OVERRIDE, 0x00},
	{CYRF_03_TX_CFG, 0x28 | CYRF_HIGH_POWER},
	{CYRF_12_DATA64_THOLD, 0x3f},
	{CYRF_10_FRAMING_CFG, 0xff},
	{CYRF_0F_XACT_CFG, 0x24}, //Switch from reading RSSI to Writing
	{CYRF_29_RX_ABORT, 0x00},
	{CYRF_12_DATA64_THOLD, 0x0a},
	{CYRF_10_FRAMING_CFG, 0xea},
};

static void __attribute__((unused)) cyrf_configdata()
{
	for(uint8_t i = 0; i < sizeof(data_vals) / 2; i++)
		CYRF_WriteRegister(data_vals[i][0], data_vals[i][1]);
}

static void __attribute__((unused)) set_sop_data_crc()
{
	uint8_t code[16];
	uint8_t pn_row = get_pn_row(hopping_frequency[hopping_frequency_no]);
	//printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[hopping_frequency_no], pn_row, sop_col, data_col);
	CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
	CYRF_ConfigCRCSeed(crc);
	crc=~crc;

	read_code(code,pn_row,sop_col,8);
	CYRF_ConfigSOPCode(code);
	read_code(code,pn_row,data_col,16);
	CYRF_ConfigDataCode(code, 16);

	if(sub_protocol == DSMX)
		hopping_frequency_no = (hopping_frequency_no + 1) % 23;
	else
		hopping_frequency_no = (hopping_frequency_no + 1) % 2;
}

static void __attribute__((unused)) calc_dsmx_channel()
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

uint16_t ReadDsm2()
{
#define DSM_CH1_CH2_DELAY	4010	// Time between write of channel 1 and channel 2
#define DSM_WRITE_DELAY		1550	// Time after write to verify write complete
#define DSM_READ_DELAY		600		// Time before write to check read state, and switch channels. Was 400 but 500 seems what the 328p needs to read a packet
	uint32_t start;

	switch(cyrf_state)
	{
		default:
			//Binding
			cyrf_state++;
			if(cyrf_state & 1)
			{
				//Send packet on even states
				//Note state has already incremented, so this is actually 'even' state
				CYRF_WriteDataPacket(packet);
				return 8500;
			}
			else
			{
				//Check status on odd states
				CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);
				return 1500;
			}
		case DSM2_CHANSEL:
			BIND_DONE;
			//Select channels and configure for writing data
			//CYRF_FindBestChannels(ch, 2, 10, 1, 79);
			cyrf_configdata();
			CYRF_SetTxRxMode(TX_EN);
			hopping_frequency_no = 0;
			cyrf_state = DSM2_CH1_WRITE_A;				// in fact cyrf_state++
			set_sop_data_crc();
			return 10000;
		case DSM2_CH1_WRITE_A:
		case DSM2_CH1_WRITE_B:
		case DSM2_CH2_WRITE_A:
		case DSM2_CH2_WRITE_B:
			build_data_packet(cyrf_state == DSM2_CH1_WRITE_B);// build lower or upper channels
			CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);	// clear IRQ flags
			CYRF_WriteDataPacket(packet);
			cyrf_state++;								// change from WRITE to CHECK mode
			return DSM_WRITE_DELAY;
		case DSM2_CH1_CHECK_A:
		case DSM2_CH1_CHECK_B:
			start=micros();
			while (micros()-start < 500)				// Wait max 500µs
				if(CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)
					break;
			set_sop_data_crc();
			cyrf_state++;								// change from CH1_CHECK to CH2_WRITE
			return DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
		case DSM2_CH2_CHECK_A:
		case DSM2_CH2_CHECK_B:
			start=micros();
			while (micros()-start < 500)				// Wait max 500µs
				if(CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)
					break;
			if (cyrf_state == DSM2_CH2_CHECK_A)
				CYRF_SetPower(0x28);					//Keep transmit power in sync
#if defined DSM_TELEMETRY
			cyrf_state++;								// change from CH2_CHECK to CH2_READ
			if(option<=3 || option>7)
			{	// disable telemetry for option between 4 and 7 ie 4,5,6,7 channels @11ms since it does not work...
				CYRF_SetTxRxMode(RX_EN);					//Receive mode
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);	//0x80??? //Prepare to receive
			}
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY - DSM_READ_DELAY;
		case DSM2_CH2_READ_A:
		case DSM2_CH2_READ_B:
			//Read telemetry
			uint8_t rx_state = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_state & 0x03) == 0x02)  				// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_state |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_state & 0x07) == 0x02)
			{ // good data (complete with no errors)
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
				uint8_t len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				if(len>MAX_PKT-2)
					len=MAX_PKT-2;
				CYRF_ReadDataPacketLen(pkt+1, len);
				pkt[0]=CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;		// store RSSI of the received telemetry signal
				telemetry_link=1;
			}
			if (cyrf_state == DSM2_CH2_READ_A && option <= 3)		// normal 22ms mode if option<=3 ie 4,5,6,7 channels @22ms
			{
				//Force end read state
				CYRF_WriteRegister(CYRF_0F_XACT_CFG, (CYRF_ReadRegister(CYRF_0F_XACT_CFG) | 0x20));  // Force end state
				start=micros();
				while (micros()-start < 100)	// Wait max 100 µs
					if((CYRF_ReadRegister(CYRF_0F_XACT_CFG) & 0x20) == 0)
						break;
				cyrf_state = DSM2_CH2_READ_B;
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);			//0x80???	//Prepare to receive
				return 11000;
			}
			if (cyrf_state == DSM2_CH2_READ_A && option>7)
				cyrf_state = DSM2_CH1_WRITE_B;				//Transmit upper
			else
				cyrf_state = DSM2_CH1_WRITE_A;				//Force 11ms if option>3 ie 4,5,6,7 channels @11ms
			CYRF_SetTxRxMode(TX_EN);						//Write mode
			set_sop_data_crc();
			return DSM_READ_DELAY;
#else
			// No telemetry
			set_sop_data_crc();
			if (cyrf_state == DSM2_CH2_CHECK_A)
			{
				if(option < 8)
				{
					cyrf_state = DSM2_CH1_WRITE_A;		// change from CH2_CHECK_A to CH1_WRITE_A (ie no upper)
					if(option>3)
						return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;	// force 11ms if option>3 ie 4,5,6,7 channels @11ms
					else
						return 22000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;	// normal 22ms mode if option<=3 ie 4,5,6,7 channels @22ms
				}
				else
					cyrf_state = DSM2_CH1_WRITE_B;		// change from CH2_CHECK_A to CH1_WRITE_A (to transmit upper)
			}
			else
				cyrf_state = DSM2_CH1_WRITE_A;		// change from CH2_CHECK_B to CH1_WRITE_A (upper already transmitted so transmit lower)
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
#endif
	}
	return 0;		
}

uint16_t initDsm2()
{ 
	CYRF_Reset();
	CYRF_GetMfgData(cyrfmfg_id);//
	//Model match
	cyrfmfg_id[3]+=RX_num;
	
	cyrf_config();

	if (sub_protocol == DSMX)
		calc_dsmx_channel();
	else
	{ 
#if RANDOM_CHANNELS == 1
		uint8_t tmpch[10];
		CYRF_FindBestChannels(tmpch, 10, 5, 3, 75);
		//
		randomSeed((uint32_t)analogRead(A6)<<10|analogRead(A7));//seed
		uint8_t idx = random(0xfefefefe) % 10;
		hopping_frequency[0] = tmpch[idx];
		while(1)
		{
			idx = random(0xfefefefe) % 10;
			if (tmpch[idx] != hopping_frequency[0])
				break;
		}
		hopping_frequency[1] = tmpch[idx];
#else
		hopping_frequency[0] = (cyrfmfg_id[0] + cyrfmfg_id[2] + cyrfmfg_id[4]) % 39 + 1;
		hopping_frequency[1] = (cyrfmfg_id[1] + cyrfmfg_id[3] + cyrfmfg_id[5]) % 40 + 40;	
#endif
	}
	
	//The crc for channel '1' is NOT(mfgid[0] << 8 + mfgid[1])
	//The crc for channel '2' is (mfgid[0] << 8 + mfgid[1])
	crc = ~((cyrfmfg_id[0] << 8) + cyrfmfg_id[1]);
	//
	sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
	data_col = 7 - sop_col;

	CYRF_SetTxRxMode(TX_EN);
	//
	if(IS_AUTOBIND_FLAG_on)
	{
		cyrf_state = DSM2_BIND;
		PROTOCOL_SticksMoved(1);  //Initialize Stick position
		initialize_bind_state();		
		binding = 1;
	}
	else
	{
		cyrf_state = DSM2_CHANSEL;//
		binding = 0;
	}
	return 10000;
}

#endif
