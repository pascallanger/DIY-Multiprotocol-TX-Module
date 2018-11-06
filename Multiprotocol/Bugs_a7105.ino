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
 
#ifdef BUGS_A7105_INO

//////////// rxid -> radioid algorithm //////////////////////////////
// Hex digit 1 is periodic with length 2, and hex digit 2 is periodic
// with length 16. However, storing the byte of those 2 digits
// instead of manipulating bits results simpler code and smaller binary.
const uint8_t PROGMEM BUGS_most_popular_67_cycle[]= {
	0x34, 0xc5, 0x6a, 0xb4, 0x29, 0xd5, 0x2c, 0xd3, 0x91, 0xb3, 0x6c, 0x49,
	0x52, 0x9c, 0x4d, 0x65, 0xc3, 0x4a, 0x5b, 0xd6, 0x92, 0x6d, 0x94, 0xa6,
	0x55, 0xcd, 0x2b, 0x9a, 0x36, 0x95, 0x4b, 0xd4, 0x35, 0x8d, 0x96, 0xb2,
	0xa3 };

static uint8_t __attribute__((unused)) BUGS_most_popular_67(uint8_t i)
{
	uint8_t ii;
	if (i == 0)
		return 0xd2;
	else if (i == 1)
		return 0xda;
	else if (i % 16 < 2)
	{
		ii = 2 * (i / 16) + i % 16 - 2;
		if (ii % 2 == 0)
			ii += 7;
	}
	else
		ii=2 * (i / 16) + (i % 16 - 2) % 7;
	return pgm_read_byte_near( &BUGS_most_popular_67_cycle[ii]);
}

static uint8_t __attribute__((unused)) BUGS_most_popular_45(uint8_t i)
{
	if (i == 0)
		return 0xa3;
	else if (i == 1)
		return 0x86;
	else
	{
		if (i % 8 == 1)
			i -= 8;
		else
			i--;
		return BUGS_most_popular_67(i);
	}
}

static uint8_t __attribute__((unused)) BUGS_most_popular_23(uint8_t i)
{
	if (i == 0)
		return 0xb2;
	else if (i == 1)
		return 0xcb;
	else
	{
		if (i % 8 == 1)
			i -= 8;
		else
			i--;
		return BUGS_most_popular_45(i);
	}
}

const uint8_t PROGMEM BUGS_most_popular_01[] = {
	0x52, 0xac, 0x59, 0xa4, 0x53, 0xab, 0x57, 0xa9,
    0x56, 0xa5, 0x5b, 0xa7, 0x5d, 0xa6, 0x58, 0xad};

static uint32_t __attribute__((unused)) BUGS_most_popular(uint8_t i)
{
	i += !(i <= 127);
	uint8_t mp01=pgm_read_byte_near( &BUGS_most_popular_01[i % 16] );
	return (uint32_t) mp01 << 24 |
		(uint32_t) BUGS_most_popular_23(i) << 16 |
		(uint32_t) BUGS_most_popular_45(i) << 8 |
		BUGS_most_popular_67(i);
}

static uint32_t __attribute__((unused)) BUGS_second_most_popular(uint8_t i)
{
	if (i < 127)
		return BUGS_most_popular(i + 1);
	else if (i > 128)
		return BUGS_most_popular(i - 1);
	else
		return 0x52d6926d;
}

// The 22 irregular values do not match the above periodicities. They might be
// errors from the readout, but let us try them here as long as it is not
// proven.
#define BUGS_NBR_IRREGULAR 22
const uint16_t PROGMEM BUGS_irregular_keys[BUGS_NBR_IRREGULAR] = {
	1131, 1287, 2842, 4668, 5311, 11594, 13122, 13813,
	20655, 22975, 25007, 25068, 28252, 33309, 35364, 35765,
	37731, 40296, 43668, 46540, 49868, 65535 };

const uint32_t PROGMEM BUGS_irregular_values[BUGS_NBR_IRREGULAR] = {
	0x52d6926d, 0xa586da34, 0x5329d52c, 0xa66c4952,
	0x536c4952, 0x524a5bd6, 0x534d65c3, 0xa9d391b3,
	0x5249529c, 0xa555cd2b, 0xac9a3695, 0x58d391b3,
	0xa791b36c, 0x53926d94, 0xa7926d94, 0xa72cd391,
	0xa9b429d5, 0x5629d52c, 0xad2b9a36, 0xa74d65c3,
	0x526d94a6, 0xad96b2a3 };

static uint32_t __attribute__((unused)) BUGS_is_irregular(uint16_t i)
{
	for (uint8_t j = 0; j < BUGS_NBR_IRREGULAR; ++j)
		if (pgm_read_word_near( &BUGS_irregular_keys[j]) == i)
			return pgm_read_dword_near( &BUGS_irregular_values[j]);
	return 0;
}

static uint32_t __attribute__((unused)) BUGS_rxid_to_radioid(uint16_t rxid)
{
	uint8_t block = rxid / 256;
	uint8_t second_seq_size;
	bool use_most_popular;

	if (rxid < 32768)
	{
		second_seq_size = 128 - block;
		use_most_popular = rxid % 256 >= second_seq_size;
	}
	else
	{
		second_seq_size = block - 127;
		use_most_popular = 255 - rxid % 256 >= second_seq_size;
	}
	uint32_t v = BUGS_is_irregular(rxid);
	if (!v)
	{
		if (use_most_popular)
			v = BUGS_most_popular(rxid % 255);
		else
			v = BUGS_second_most_popular(rxid % 255);
	}
	return v;
}
//////////// rxid -> radioid algorithm //////////////////////////////

// For code readability
#define BUGS_CH_SW_ARM		CH5_SW
#define BUGS_CH_SW_ANGLE	CH6_SW
#define BUGS_CH_SW_FLIP		CH7_SW
#define BUGS_CH_SW_PICTURE	CH8_SW
#define BUGS_CH_SW_VIDEO	CH9_SW
#define BUGS_CH_SW_LED		CH10_SW

// flags packet byte 4
#define BUGS_FLAG_FLIP		0x08    // automatic flip
#define BUGS_FLAG_MODE		0x04    // low/high speed select (set is high speed)
#define BUGS_FLAG_VIDEO		0x02    // toggle video
#define BUGS_FLAG_PICTURE	0x01    // toggle picture

// flags packet byte 5
#define BUGS_FLAG_LED		0x80    // enable LEDs
#define BUGS_FLAG_ARM		0x40    // arm (toggle to turn on motors)
#define BUGS_FLAG_DISARM	0x20    // disarm (toggle to turn off motors)
#define BUGS_FLAG_ANGLE		0x04    // angle/acro mode (set is angle mode)

#define BUGS_PACKET_SIZE	22
#define BUGS_NUM_RFCHAN		16

enum {
	BUGS_BIND_1,
	BUGS_BIND_2,
	BUGS_BIND_3,
	BUGS_DATA_1,
	BUGS_DATA_2,
	BUGS_DATA_3,
};

static void __attribute__((unused)) BUGS_check_arming()
{
	uint8_t arm_channel = BUGS_CH_SW_ARM;

	if (arm_channel != arm_channel_previous)
	{
		arm_channel_previous = arm_channel;
		if (arm_channel)
		{
			armed = 1;
			arm_flags ^= BUGS_FLAG_ARM;
		}
		else
		{
			armed = 0;
			arm_flags ^= BUGS_FLAG_DISARM;
		}
	}
}

static void __attribute__((unused)) BUGS_build_packet(uint8_t bind)
{
	uint8_t force_values = bind | !armed;
	uint8_t change_channel = ((packet_count & 0x1) << 6);
	uint16_t aileron  = convert_channel_16b_limit(AILERON,800,0);
	uint16_t elevator = convert_channel_16b_limit(ELEVATOR,800,0);
	uint16_t throttle = convert_channel_16b_limit(THROTTLE,0,800);
	uint16_t rudder   = convert_channel_16b_limit(RUDDER,800,0);

	memset(packet, 0, BUGS_PACKET_SIZE);
	packet[1] = 0x76;		// txid (rx uses to know hopping frequencies)
	packet[2] = 0x71;
	packet[3] = 0x94;

	BUGS_check_arming();	// sets globals arm_flags and armed
	if(bind)
	{
		packet[4] = change_channel | 0x80;
		packet[5] = 0x02 | arm_flags
		| GET_FLAG(BUGS_CH_SW_ANGLE, BUGS_FLAG_ANGLE);
	}
	else
	{
		packet[4] = change_channel | BUGS_FLAG_MODE
		| GET_FLAG(BUGS_CH_SW_FLIP, BUGS_FLAG_FLIP)
		| GET_FLAG(BUGS_CH_SW_PICTURE, BUGS_FLAG_PICTURE)
		| GET_FLAG(BUGS_CH_SW_VIDEO, BUGS_FLAG_VIDEO);
		packet[5] = 0x02 | arm_flags
		| GET_FLAG(BUGS_CH_SW_ANGLE, BUGS_FLAG_ANGLE)
		| GET_FLAG(BUGS_CH_SW_LED, BUGS_FLAG_LED);
	}

	packet[6] = force_values ? 100 : (aileron  >> 2);
	packet[7] = force_values ? 100 : (elevator >> 2);
	packet[8] = force_values ?   0 : (throttle >> 2);
	packet[9] = force_values ? 100 : (rudder   >> 2);
	packet[10] = 100;
	packet[11] = 100;
	packet[12] = 100;
	packet[13] = 100;

	packet[14] = ((aileron  << 6) & 0xc0)
	| ((elevator << 4) & 0x30)
	| ((throttle << 2) & 0x0c)
	| ((rudder       ) & 0x03);

	//    packet[15] = 0;

	// driven trims
	packet[16] = aileron / 8 + 14;
	packet[17] = elevator / 8 + 14;
	packet[18] = 64;
	packet[19] = rudder / 8 + 14;

	//    packet[20] = 0;
	//    packet[21] = 0;

    uint8_t check = 0x6d;
    for (uint8_t i=1; i < BUGS_PACKET_SIZE; i++)
        check ^= packet[i];
	packet[0] = check;
}

const uint8_t PROGMEM BUGS_hop []= {
		0x1d, 0x3b, 0x4d, 0x29, 0x11, 0x2d, 0x0b, 0x3d, 0x59, 0x48, 0x17, 0x41, 0x23, 0x4e, 0x2a, 0x63,	// bind phase ID=0xac59a453
		0x4b, 0x19, 0x35, 0x1e, 0x63, 0x0f, 0x45, 0x21, 0x51, 0x3a, 0x5d, 0x25, 0x0a, 0x44, 0x61, 0x27,	// data phase ID=0xA4C56AB4 for txid 767194 if rx responds C6 BB 57 7F 00 00 00 00 00 00 FF 87 40 00 00 00
	};

static void  __attribute__((unused))BUGS_set_radio_data()
{	// captured radio data for bugs rx/tx version A2
	// it appears that the hopping frequencies are determined by the txid
	// and the data phase radio id is determined by the first 2 bytes of the
	// rx bind packet
	uint8_t offset=0;
	uint32_t radio_id=0xac59a453;	// bind phase ID=0xac59a453

	if(IS_BIND_DONE)
	{
		offset=BUGS_NUM_RFCHAN;
		// Read radio_id from EEPROM
		radio_id=0;
		uint8_t base_adr=BUGS_EEPROM_OFFSET+RX_num*4;
		for(uint8_t i=0; i<4; i++)
			radio_id|=eeprom_read_byte((EE_ADDR)(base_adr+i))<<(i*8);
	}
	A7105_WriteID(radio_id);

	for(uint8_t i=0; i<BUGS_NUM_RFCHAN;i++)
		hopping_frequency[i]=pgm_read_byte_near( &BUGS_hop[i+offset] );
}

static void __attribute__((unused)) BUGS_increment_counts()
{	// this logic works with the use of packet_count in BUGS_build_packet
	// to properly indicate channel changes to rx
	packet_count += 1;
	if ((packet_count & 1) == 0)
	{
		hopping_frequency_no += 1;
		hopping_frequency_no %= BUGS_NUM_RFCHAN;
	}
}

#define BUGS_PACKET_PERIOD   6100
#define BUGS_DELAY_TX        2000
#define BUGS_DELAY_POST_RX   1500
#define BUGS_DELAY_BIND_RST   200

// FIFO config is one less than desired value
#define BUGS_FIFO_SIZE_RX      15
#define BUGS_FIFO_SIZE_TX      21
uint16_t ReadBUGS(void)
{
	uint8_t mode, base_adr;
	uint16_t rxid;
	uint32_t radio_id;
	uint16_t start;

	// keep frequency tuning updated
	#ifndef FORCE_FLYSKY_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif

	switch(phase)
	{
		case BUGS_BIND_1:
			BUGS_build_packet(1);
			A7105_Strobe(A7105_STANDBY);
			A7105_WriteReg(A7105_03_FIFOI, BUGS_FIFO_SIZE_TX);
			A7105_WriteData(BUGS_PACKET_SIZE, hopping_frequency[hopping_frequency_no]);
			phase = BUGS_BIND_2;
			packet_period = BUGS_DELAY_TX;
			break;

		case BUGS_BIND_2:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)micros()-start < 500)			// Wait max 500µs, using serial+telemetry exit in about 60µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetTxRxMode(RX_EN);
			A7105_WriteReg(A7105_0F_PLL_I, hopping_frequency[hopping_frequency_no] - 2);
			A7105_WriteReg(A7105_03_FIFOI, BUGS_FIFO_SIZE_RX);
			A7105_Strobe(A7105_RX);

			BUGS_increment_counts();
			phase = BUGS_BIND_3;
			packet_period = BUGS_PACKET_PERIOD-BUGS_DELAY_TX-BUGS_DELAY_POST_RX;
			break;

		case BUGS_BIND_3:
			mode = A7105_ReadReg(A7105_00_MODE);
			A7105_Strobe(A7105_STANDBY);
			A7105_SetTxRxMode(TX_EN);
			if (mode & 0x01)
			{
				phase = BUGS_BIND_1;
				packet_period = BUGS_DELAY_BIND_RST;         // No received data so restart binding procedure.
				break;
			}
			A7105_ReadData(16);
			if ((packet[0] + packet[1] + packet[2] + packet[3]) == 0)
			{
				phase = BUGS_BIND_1;
				packet_period = BUGS_DELAY_BIND_RST;         // No received data so restart binding procedure.
				break;
			}
			A7105_Strobe(A7105_STANDBY);
			BIND_DONE;
			// set radio_id
			rxid = (packet[1] << 8) + packet[2];
			radio_id = BUGS_rxid_to_radioid(rxid);
			base_adr=BUGS_EEPROM_OFFSET+RX_num*4;
			for(uint8_t i=0; i<4; i++)
				eeprom_write_byte((EE_ADDR)(base_adr+i),radio_id>>(i*8));	// Save radio_id in EEPROM
			BUGS_set_radio_data();
			phase = BUGS_DATA_1;
			packet_count = 0;
			hopping_frequency_no = 0;
			packet_period = BUGS_DELAY_POST_RX;
			break;

		case BUGS_DATA_1:
			A7105_SetPower();
			BUGS_build_packet(0);
			A7105_WriteReg(A7105_03_FIFOI, BUGS_FIFO_SIZE_TX);
			A7105_WriteData(BUGS_PACKET_SIZE, hopping_frequency[hopping_frequency_no]);
			phase = BUGS_DATA_2;
			packet_period = BUGS_DELAY_TX;
			break;

		case BUGS_DATA_2:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)micros()-start < 500)			// Wait max 500µs, using serial+telemetry exit in about 60µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetTxRxMode(RX_EN);
			A7105_WriteReg(A7105_0F_PLL_I, hopping_frequency[hopping_frequency_no] - 2);
			A7105_WriteReg(A7105_03_FIFOI, BUGS_FIFO_SIZE_RX);
			A7105_Strobe(A7105_RX);

			BUGS_increment_counts();
			phase = BUGS_DATA_3;
			packet_period = BUGS_PACKET_PERIOD-BUGS_DELAY_TX-BUGS_DELAY_POST_RX;
			break;

		case BUGS_DATA_3:
			mode = A7105_ReadReg(A7105_00_MODE);
			A7105_Strobe(A7105_STANDBY);
			A7105_SetTxRxMode(TX_EN);
			if (!(mode & 0x01))
			{
				A7105_ReadData(16);
				#if defined(BUGS_HUB_TELEMETRY)
					v_lipo1=packet[10] == 0xff ? 0xff : 0x00;					// Voltage in this case is only an alert on level good or bad.
					RX_RSSI=packet[3];
					// Read TX RSSI
					int16_t temp=256-(A7105_ReadReg(A7105_1D_RSSI_THOLD)*8)/5;	// Value from A7105 is between 8 for maximum signal strength to 160 or less
					if(temp<0) temp=0;
					else if(temp>255) temp=255;
					TX_RSSI=temp;
					telemetry_link=1;
				#endif
			}
			phase = BUGS_DATA_1;
			packet_period = BUGS_DELAY_POST_RX;
			break;
	}
	return packet_period;
}

uint16_t initBUGS(void)
{
	uint32_t radio_id=0;
	uint8_t base_adr=BUGS_EEPROM_OFFSET+RX_num*4;
	for(uint8_t i=0; i<4; i++)
		radio_id|=eeprom_read_byte((EE_ADDR)(base_adr+i))<<(i*8);
	if(radio_id==0xffffffff)
		BIND_IN_PROGRESS;

	BUGS_set_radio_data();
	if (IS_BIND_IN_PROGRESS)
		phase = BUGS_BIND_1;
	else
		phase = BUGS_DATA_1;

	A7105_Init();

	hopping_frequency_no = 0;
	packet_count = 0;
	armed = 0;
	arm_flags = BUGS_FLAG_DISARM;		// initial value from captures
	arm_channel_previous = BUGS_CH_SW_ARM;
	#ifdef BUGS_HUB_TELEMETRY
		init_frskyd_link_telemetry();
	#endif

	return 10000;
}

#endif
