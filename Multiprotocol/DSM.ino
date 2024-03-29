#if defined(DSM_CYRF6936_INO) || defined(DSM_RX_CYRF6936_INO)

#include "iface_cyrf6936.h"

uint8_t sop_col;

const uint8_t PROGMEM DSM_pncodes[][8] = {
	/* Note these are in order transmitted (LSB 1st) */
	/* Row 1 */
		/* Col 0 */ {0x83, 0xF7, 0xA8, 0x2D, 0x7A, 0x44, 0x64, 0xD3},
		/* Col 1 */ {0x3F, 0x2C, 0x4E, 0xAA, 0x71, 0x48, 0x7A, 0xC9},
		/* Col 2 */ {0x17, 0xFF, 0x9E, 0x21, 0x36, 0x90, 0xC7, 0x82},
		/* Col 3 */ {0xBC, 0x5D, 0x9A, 0x5B, 0xEE, 0x7F, 0x42, 0xEB},
		/* Col 4 */ {0x24, 0xF5, 0xDD, 0xF8, 0x7A, 0x77, 0x74, 0xE7},
		/* Col 5 */ {0x3D, 0x70, 0x7C, 0x94, 0xDC, 0x84, 0xAD, 0x95},
		/* Col 6 */ {0x1E, 0x6A, 0xF0, 0x37, 0x52, 0x7B, 0x11, 0xD4},
		/* Col 7 */ {0x62, 0xF5, 0x2B, 0xAA, 0xFC, 0x33, 0xBF, 0xAF},
	/* Row 2 */
		/* Col 0 */ {0x40, 0x56, 0x32, 0xD9, 0x0F, 0xD9, 0x5D, 0x97},
		/* Col 1 */ {0x8E, 0x4A, 0xD0, 0xA9, 0xA7, 0xFF, 0x20, 0xCA},
		/* Col 2 */ {0x4C, 0x97, 0x9D, 0xBF, 0xB8, 0x3D, 0xB5, 0xBE},
		/* Col 3 */ {0x0C, 0x5D, 0x24, 0x30, 0x9F, 0xCA, 0x6D, 0xBD},
		/* Col 4 */ {0x50, 0x14, 0x33, 0xDE, 0xF1, 0x78, 0x95, 0xAD},
		/* Col 5 */ {0x0C, 0x3C, 0xFA, 0xF9, 0xF0, 0xF2, 0x10, 0xC9},
		/* Col 6 */ {0xF4, 0xDA, 0x06, 0xDB, 0xBF, 0x4E, 0x6F, 0xB3},
		/* Col 7 */ {0x9E, 0x08, 0xD1, 0xAE, 0x59, 0x5E, 0xE8, 0xF0},
	/* Row 3 */
		/* Col 0 */ {0xC0, 0x90, 0x8F, 0xBB, 0x7C, 0x8E, 0x2B, 0x8E},
		/* Col 1 */ {0x80, 0x69, 0x26, 0x80, 0x08, 0xF8, 0x49, 0xE7},
		/* Col 2 */ {0x7D, 0x2D, 0x49, 0x54, 0xD0, 0x80, 0x40, 0xC1},
		/* Col 3 */ {0xB6, 0xF2, 0xE6, 0x1B, 0x80, 0x5A, 0x36, 0xB4},
		/* Col 4 */ {0x42, 0xAE, 0x9C, 0x1C, 0xDA, 0x67, 0x05, 0xF6},
		/* Col 5 */ {0x9B, 0x75, 0xF7, 0xE0, 0x14, 0x8D, 0xB5, 0x80},
		/* Col 6 */ {0xBF, 0x54, 0x98, 0xB9, 0xB7, 0x30, 0x5A, 0x88},
		/* Col 7 */ {0x35, 0xD1, 0xFC, 0x97, 0x23, 0xD4, 0xC9, 0x88},
	/* Row 4 */
		/* Col 0 */ {0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93}, // Wrong values used by Orange TX/RX Row 3 Col 8: {0x88, 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40}
		/* Col 1 */ {0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C},
		/* Col 2 */ {0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA},
		/* Col 3 */ {0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC},
		/* Col 4 */ {0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84},
		/* Col 5 */ {0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7},
		/* Col 6 */ {0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0},
		/* Col 7 */ {0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1},
	/* Row 0 */
		/* Col 0 */ {0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8},
		/* Col 1 */ {0x88, 0x17, 0x13, 0x3B, 0x2D, 0xBF, 0x06, 0xD6},
		/* Col 2 */ {0xF1, 0x94, 0x30, 0x21, 0xA1, 0x1C, 0x88, 0xA9},
		/* Col 3 */ {0xD0, 0xD2, 0x8E, 0xBC, 0x82, 0x2F, 0xE3, 0xB4},
		/* Col 4 */ {0x8C, 0xFA, 0x47, 0x9B, 0x83, 0xA5, 0x66, 0xD0},
		/* Col 5 */ {0x07, 0xBD, 0x9F, 0x26, 0xC8, 0x31, 0x0F, 0xB8},
		/* Col 6 */ {0xEF, 0x03, 0x95, 0x89, 0xB4, 0x71, 0x61, 0x9D},
		/* Col 7 */ {0x40, 0xBA, 0x97, 0xD5, 0x86, 0x4F, 0xCC, 0xD1},
		/* Col 8 */ {0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86}
};

static void __attribute__((unused)) DSM_read_code(uint8_t *buf, uint8_t row, uint8_t col)
{
	row--;
	if(row>4)
		row = 4;
	for(uint8_t i=0;i<8;i++)
		buf[i]=pgm_read_byte_near( &DSM_pncodes[row*8+col][i] );
}

const uint8_t PROGMEM DSM_init_vals[][2] = {
	{CYRF_02_TX_CTRL, 0x00},				// All TX interrupt disabled
	{CYRF_05_RX_CTRL, 0x00},				// All RX interrupt disabled
	{CYRF_28_CLK_EN, 0x02},					// Force receive clock enable
	{CYRF_32_AUTO_CAL_TIME, 0x3c},			// Default init value
	{CYRF_35_AUTOCAL_OFFSET, 0x14},			// Default init value
	{CYRF_26_XTAL_CFG, 0x08},				// Start delay
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
}

static void __attribute__((unused)) DSM_cyrf_configdata()
{
	for(uint8_t i = 0; i < sizeof(DSM_data_vals) / 2; i++)
		CYRF_WriteRegister(pgm_read_byte_near(&DSM_data_vals[i][0]), pgm_read_byte_near(&DSM_data_vals[i][1]));
}

static uint8_t __attribute__((unused)) DSM_get_pn_row(uint8_t channel, bool dsmx)
{
	if(protocol == PROTO_DSM && sub_protocol == DSMR)
		return (channel + 2) % 5;
	return (dsmx ? (channel - 2) % 5 : channel % 5);
}

static void __attribute__((unused)) DSM_set_sop_data_crc(bool ch2, bool dsmx)
{
	//The crc for channel '1' is NOT(mfgid[0] << 8 + mfgid[1])
	//The crc for channel '2' is (mfgid[0] << 8 + mfgid[1])
	if(ch2)
		CYRF_ConfigCRCSeed(seed);	//CH2
	else
		CYRF_ConfigCRCSeed(~seed);	//CH1, DSMR only use CH1

	uint8_t pn_row = DSM_get_pn_row(hopping_frequency[hopping_frequency_no], dsmx);
	uint8_t code[16];
	#if 0
		debug_time();
		debug(" crc:%04X,row:%d,col:%d,rf:%02X",(~seed)&0xffff,pn_row,sop_col,hopping_frequency[hopping_frequency_no]);
	#endif
	DSM_read_code(code,pn_row,sop_col);						// pn_row between 0 and 4, sop_col between 0 and 7
	CYRF_ConfigSOPCode(code);
	DSM_read_code(code,pn_row,7 - sop_col);					// 7-sop_col between 0 and 7
	DSM_read_code(code+8,pn_row,7 - sop_col + 1);			// 7-sop_col+1 between 1 and 8
	CYRF_ConfigDataCode(code);

	CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
	hopping_frequency_no++;
	if(dsmx)
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

#endif