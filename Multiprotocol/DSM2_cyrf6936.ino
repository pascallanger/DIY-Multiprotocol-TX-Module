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

#define DSM2_NUM_CHANNELS 7
#define RANDOM_CHANNELS  0		// disabled
//#define RANDOM_CHANNELS  1	// enabled
#define BIND_CHANNEL 0x0d //13 This can be any odd channel
#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

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


const uint8_t pncodes[5][9][8] = {
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
		/* Col 8 */ {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
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

//
uint8_t chidx;
uint8_t sop_col;
uint8_t data_col;
uint16_t cyrf_state;
uint8_t crcidx;
uint8_t binding;
uint16_t crc;

/*
#ifdef USE_FIXED_MFGID
const uint8_t cyrfmfg_id[6] = {0x5e, 0x28, 0xa3, 0x1b, 0x00, 0x00}; //dx8
const uint8_t cyrfmfg_id[6] = {0xd4, 0x62, 0xd6, 0xad, 0xd3, 0xff}; //dx6i
#else
//uint8_t cyrfmfg_id[6];
#endif
*/

static void __attribute__((unused)) build_bind_packet()
{
	uint8_t i;
	uint16_t sum = 384 - 0x10;//
	packet[0] = crc >> 8;
	packet[1] = crc & 0xff;
	packet[2] = 0xff ^ cyrfmfg_id[2];
	packet[3] = (0xff ^ cyrfmfg_id[3]) + RX_num;
	packet[4] = packet[0];
	packet[5] = packet[1];
	packet[6] = packet[2];
	packet[7] = packet[3];
	for(i = 0; i < 8; i++)
		sum += packet[i];
	packet[8] = sum >> 8;
	packet[9] = sum & 0xff;
	packet[10] = 0x01; //???
	packet[11] = DSM2_NUM_CHANNELS;
	if(sub_protocol==DSMX)	//DSMX type
		packet[12] = 0xb2;	// Telemetry off: packet[12] = num_channels < 8 && Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_OFF ? 0xa2 : 0xb2;
	else
#if DSM2_NUM_CHANNELS < 8
		packet[12] = 0x01;
#else
		packet[12] = 0x02;
#endif
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
#if DSM2_NUM_CHANNELS==4
	const uint8_t ch_map[] = {0, 1, 2, 3, 0xff, 0xff, 0xff};    //Guess
#elif DSM2_NUM_CHANNELS==5
	const uint8_t ch_map[] = {0, 1, 2, 3, 4,    0xff, 0xff}; //Guess
#elif DSM2_NUM_CHANNELS==6
	const uint8_t ch_map[] = {1, 5, 2, 3, 0,    4,    0xff}; //HP6DSM
#elif DSM2_NUM_CHANNELS==7
	const uint8_t ch_map[] = {1, 5, 2, 4, 3,    6,    0}; //DX6i
#elif DSM2_NUM_CHANNELS==8
	const uint8_t ch_map[] = {1, 5, 2, 3, 6,    0xff, 0xff, 4, 0, 7,    0xff, 0xff, 0xff, 0xff}; //DX8
#elif DSM2_NUM_CHANNELS==9
	const uint8_t ch_map[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 0xff, 0xff, 0xff, 0xff, 0xff}; //DM9
#elif DSM2_NUM_CHANNELS==10
	const uint8_t ch_map[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 0xff, 0xff, 0xff, 0xff};
#elif DSM2_NUM_CHANNELS==11
	const uint8_t ch_map[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 10, 0xff, 0xff, 0xff};
#elif DSM2_NUM_CHANNELS==12
	const uint8_t ch_map[] = {3, 2, 1, 5, 0,    4,    6,    7, 8, 9, 10, 11, 0xff, 0xff};
#endif

	uint8_t i;
	uint8_t bits;
	//
	if( binding && PROTOCOL_SticksMoved(0) )
	{
		//BIND_DONE;
		binding = 0;
	}
	if (sub_protocol==DSMX)
	{
		packet[0] = cyrfmfg_id[2];
		packet[1] = cyrfmfg_id[3] + RX_num;
		bits=11;
	}
	else
	{
		packet[0] = (0xff ^ cyrfmfg_id[2]);
		packet[1] = (0xff ^ cyrfmfg_id[3]) + RX_num;
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
	{CYRF_02_TX_CTRL, 0x00},
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
	const uint8_t pn_bind[] = { 0xc6,0x94,0x22,0xfe,0x48,0xe6,0x57,0x4e };
	uint8_t data_code[32];
	CYRF_ConfigRFChannel(BIND_CHANNEL); //This seems to be random?
	uint8_t pn_row = get_pn_row(BIND_CHANNEL);
	//printf("Ch: %d Row: %d SOP: %d Data: %d\n", BIND_CHANNEL, pn_row, sop_col, data_col);
	CYRF_ConfigCRCSeed(crc);
	CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
	memcpy(data_code, pncodes[pn_row][data_col], 16);
	memcpy(data_code + 16, pncodes[0][8], 8);
	memcpy(data_code + 24, pn_bind, 8);
	CYRF_ConfigDataCode(data_code, 32);
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
	uint8_t pn_row = get_pn_row(hopping_frequency[chidx]);
	//printf("Ch: %d Row: %d SOP: %d Data: %d\n", ch[chidx], pn_row, sop_col, data_col);
	CYRF_ConfigRFChannel(hopping_frequency[chidx]);
	CYRF_ConfigCRCSeed(crcidx ? ~crc : crc);
	CYRF_ConfigSOPCode(pncodes[pn_row][sop_col]);
	CYRF_ConfigDataCode(pncodes[pn_row][data_col], 16);
	if(sub_protocol == DSMX)
		chidx = (chidx + 1) % 23;
	else
		chidx = (chidx + 1) % 2;
	crcidx = !crcidx;
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
		if (((next_ch ^ id) & 0x01 )== 0)
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
#define CH1_CH2_DELAY 4010  // Time between write of channel 1 and channel 2
#define WRITE_DELAY   1650  // 1550 original, Time after write to verify write complete
#define READ_DELAY     400  // Time before write to check read state, and switch channels
	uint8_t i = 0;

	switch(cyrf_state)
	{
		default:
			//Binding
			cyrf_state++;
			if(cyrf_state & 1)
			{
				//Send packet on even states
				//Note state has already incremented,
				// so this is actually 'even' state
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
			chidx = 0;
			crcidx = 0;
			cyrf_state = DSM2_CH1_WRITE_A;	// in fact cyrf_state++
			set_sop_data_crc();
			return 10000;
		case DSM2_CH1_WRITE_A:
		case DSM2_CH1_WRITE_B:
			build_data_packet(cyrf_state == DSM2_CH1_WRITE_B);//compare state and DSM2_CH1_WRITE_B return 0 or 1
		case DSM2_CH2_WRITE_A:
		case DSM2_CH2_WRITE_B:
			CYRF_WriteDataPacket(packet);
			cyrf_state++;				// change from WRITE to CHECK mode
			return WRITE_DELAY;
		case DSM2_CH1_CHECK_A:
		case DSM2_CH1_CHECK_B:
			while (! (CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02))
				if(++i > NUM_WAIT_LOOPS)
					break;
			set_sop_data_crc();
			cyrf_state++;			// change from CH1_CHECK to CH2_WRITE
			return CH1_CH2_DELAY - WRITE_DELAY;
		case DSM2_CH2_CHECK_A:
		case DSM2_CH2_CHECK_B:
			while (! (CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02))
				if(++i > NUM_WAIT_LOOPS)
					break;
			if (cyrf_state == DSM2_CH2_CHECK_A)
				CYRF_SetPower(0x28);	//Keep transmit power in sync
			// No telemetry...
			set_sop_data_crc();
			if (cyrf_state == DSM2_CH2_CHECK_A)
			{
	#if DSM2_NUM_CHANNELS < 8
				cyrf_state = DSM2_CH1_WRITE_A;		// change from CH2_CHECK_A to CH1_WRITE_A (ie no upper)
				return 11000 - CH1_CH2_DELAY - WRITE_DELAY ;	// Original is 22000 from deviation but it works better this way
	#else
				cyrf_state = DSM2_CH1_WRITE_B;		// change from CH2_CHECK_A to CH1_WRITE_A (to transmit upper)
	#endif
			}
			else
				cyrf_state = DSM2_CH1_WRITE_A;		// change from CH2_CHECK_B to CH1_WRITE_A (upper already transmitted so transmit lower)
			return 11000 - CH1_CH2_DELAY - WRITE_DELAY;
	}
	return 0;		
}

uint16_t initDsm2()
{ 
	CYRF_Reset();
	CYRF_GetMfgData(cyrfmfg_id);//

	cyrf_config();

	if (sub_protocol ==DSMX)
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
	
	///}
	crc = ~((cyrfmfg_id[0] << 8) + cyrfmfg_id[1]); //The crc for channel 'a' is NOT(mfgid[1] << 8 + mfgid[0])
	crcidx = 0;//The crc for channel 'b' is (mfgid[1] << 8 + mfgid[0])
	//
	sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;//Ok
	data_col = 7 - sop_col;//ok

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
