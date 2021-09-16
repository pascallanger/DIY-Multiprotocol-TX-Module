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

#if defined(DEVO_CYRF6936_INO) 
 
#include "iface_cyrf6936.h"

//For Debug
//#define NO_SCRAMBLE

#define DEVO_PKTS_PER_CHANNEL	4
#define DEVO_BIND_COUNT			0x1388

#define DEVO_NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

enum {
	DEVO_BIND,
	DEVO_BIND_SENDCH,
	DEVO_BOUND,
	DEVO_BOUND_1,
	DEVO_BOUND_2,
	DEVO_BOUND_3,
	DEVO_BOUND_4,
	DEVO_BOUND_5,
	DEVO_BOUND_6,
	DEVO_BOUND_7,
	DEVO_BOUND_8,
	DEVO_BOUND_9,
	DEVO_BOUND_10,
};

static void __attribute__((unused)) DEVO_scramble_pkt()
{
#ifdef NO_SCRAMBLE
	return;
#else
	for(uint8_t i = 0; i < 15; i++)
		packet[i + 1] ^= cyrfmfg_id[i % 4];
#endif
}

static void __attribute__((unused)) DEVO_add_pkt_suffix()
{
    uint8_t bind_state;
	#ifdef ENABLE_PPM
	if(mode_select && (option&0x01)==0 && IS_BIND_DONE) 			//PPM mode and option not already set and bind is finished
	{
		BIND_SET_INPUT;
		BIND_SET_PULLUP;										// set pullup
		if(IS_BIND_BUTTON_on)
		{
			eeprom_write_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num),0x01);	// Set fixed id mode for the current model
			option |= 0x01;
		}
		BIND_SET_OUTPUT;
	}
	#endif //ENABLE_PPM
    if(prev_option!=option && IS_BIND_DONE)
	{
		MProtocol_id = RX_num + MProtocol_id_master;
		bind_counter=DEVO_BIND_COUNT;
	}
	if (option&0x01)
	{
        if (bind_counter > 0)
            bind_state = 0xc0;
        else
            bind_state = 0x80;
    }
	else
        bind_state = 0x00;
	packet[10] = bind_state | (DEVO_PKTS_PER_CHANNEL - packet_count - 1);
	packet[11] = *(hopping_frequency_ptr + 1);
	packet[12] = *(hopping_frequency_ptr + 2);
	packet[13] = MProtocol_id  & 0xff;
	packet[14] = (MProtocol_id >> 8) & 0xff;
	packet[15] = (MProtocol_id >> 16) & 0xff;
}

static void __attribute__((unused)) DEVO_build_beacon_pkt(uint8_t upper)
{
	packet[0] = (num_ch << 4) | 0x07;
	uint8_t max = 8, offset = 0, enable = 0;
	if (upper)
	{
		packet[0] += 1;
		max = 4;
		offset = 8;
	}
	for(uint8_t i = 0; i < max; i++)
	{
		#ifdef FAILSAFE_ENABLE
			uint16_t failsafe=Failsafe_data[CH_EATR[i+offset]];
			if(i + offset < num_ch && failsafe!=FAILSAFE_CHANNEL_HOLD && IS_FAILSAFE_VALUES_on)
			{
				enable |= 0x80 >> i;
				packet[i+1] = ((failsafe*25)>>8)-100;
			}
			else
		#else
			(void)offset;
		#endif
				packet[i+1] = 0;
	}
	packet[9] = enable;
	DEVO_add_pkt_suffix();
}

static void __attribute__((unused)) DEVO_build_bind_pkt()
{
	packet[0] = (num_ch << 4) | 0x0a;
	packet[1] = bind_counter & 0xff;
	packet[2] = (bind_counter >> 8);
	packet[3] = *hopping_frequency_ptr;
	packet[4] = *(hopping_frequency_ptr + 1);
	packet[5] = *(hopping_frequency_ptr + 2);
	packet[6] = cyrfmfg_id[0];
	packet[7] = cyrfmfg_id[1];
	packet[8] = cyrfmfg_id[2];
	packet[9] = cyrfmfg_id[3];
	DEVO_add_pkt_suffix();
	//The fixed-id portion is scrambled in the bind packet
	//I assume it is ignored
	packet[13] ^= cyrfmfg_id[0];
	packet[14] ^= cyrfmfg_id[1];
	packet[15] ^= cyrfmfg_id[2];
}

static void __attribute__((unused)) DEVO_build_data_pkt()
{
	static uint8_t ch_idx=0;

	packet[0] = (num_ch << 4) | (0x0b + ch_idx);
	uint8_t sign = 0x0b;
	for (uint8_t i = 0; i < 4; i++)
	{
		int16_t value=convert_channel_16b_nolimit(CH_EATR[ch_idx * 4 + i],-1600,1600,false);//range -1600..+1600
		if(value < 0)
		{
			value = -value;
			sign |= 1 << (7 - i);
		}
		packet[2 * i + 1] = value & 0xff;
		packet[2 * i + 2] = (value >> 8) & 0xff;
	}
	packet[9] = sign;
	ch_idx++;
	if (ch_idx * 4 >= num_ch)
		ch_idx = 0;
	DEVO_add_pkt_suffix();
}

#if defined DEVO_HUB_TELEMETRY
static uint32_t __attribute__((unused)) DEVO_text_to_int(uint8_t *ptr, uint8_t len)
{
	uint32_t value = 0;
	for(uint8_t i = 0; i < len; i++)
		value = value * 10 + (ptr[i] - '0');
	return value;
}

static void __attribute__((unused)) DEVO_float_to_ints(uint8_t *ptr, uint16_t *value, uint16_t *decimal)
{
	bool seen_decimal = false;
	*value = 0;
	*decimal = 0;
	for(uint8_t i = 0; i < 7; i++)
	{
		if(ptr[i] == '.')
		{
			seen_decimal = true;
			continue;
		}
		if(ptr[i] < '0' || ptr[i] > '9')
		{
			if(*value != 0 || seen_decimal)
				return;
		}
		else
		{
			if(seen_decimal)
				*decimal = *decimal * 10 + (ptr[i] - '0');
			else
				*value = *value * 10 + (ptr[i] - '0');
		}
	}
}

static void __attribute__((unused)) DEVO_parse_telemetry_packet()
{ // Telemetry packets every 2.4ms
	DEVO_scramble_pkt(); //This will unscramble the packet
	
	debugln("RX");
	if ((((uint32_t)packet[15] << 16) | ((uint32_t)packet[14] << 8) | packet[13]) != (MProtocol_id & 0x00ffffff))
		return;	// ID does not match
		
	if((telemetry_link & 3) != 0)
	{
		debugln("S%d",telemetry_link);
		return;	// Previous telemetry not sent yet...
	}

	//Debug telem RX
	//for(uint8_t i=0;i<12;i++)
	//	debug("%02X ",packet[i]);
	//debugln("");
	
	#if defined HUB_TELEMETRY
	//Telemetry https://github.com/DeviationTX/deviation/blob/5efb6a28bea697af9a61b5a0ed2528cc8d203f90/src/protocol/devo_cyrf6936.c#L232
		uint16_t val, dec;
		uint32_t val32;
		switch(packet[0])
		{
			case 0x30:	// Volt and RPM packet
				//RSSI and voltage
				TX_RSSI = CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
				TX_RSSI = (TX_RSSI << 1) + TX_RSSI;
				RX_RSSI = TX_RSSI;
				telemetry_link |= 1;
				v_lipo1 = packet[1] << 1;
				v_lipo2 = packet[3] << 1;
				//packet[5] = 127;																					// 12.7V
				if(packet[5] != 0)
				{
					val  = (packet[5]*11)/21;																		// OpenTX strange transformation??
					dec  = val;
					val /= 10;
					dec -= val*10;
					frsky_send_user_frame(0x3A, val, 0x00);															// volt3
					frsky_send_user_frame(0x3B, dec, 0x00);															// volt3
				}
				val = packet[7] * 120;																				// change to RPM
				frsky_send_user_frame(0x03, val, val>>8);															// RPM
				break;
			case 0x31:	// Temperature packet
				//memcpy(&packet[1],"\x29\x2A\x2B\x00\x00\x00\x00\x00\x00\x00\x00\x00",12);							// 21°, 22°, 23°
				for(uint8_t i=0; i<2;i++)
					if(packet[i+1]!=0xff)
					{
						val = packet[i+1];
						val -= 20;
						frsky_send_user_frame(0x02 + i*3, val, val>>8);												// temp 1 & 2
					}
				break;
			// GPS Data
			case 0x32: // Longitude
				//memcpy(&packet[1],"\x30\x33\x30\x32\x30\x2e\x38\x32\x37\x30\x45\xfb",12);							// 030°20.8270E in ddmm.mmmm
				//memcpy(&packet[1],"\x31\x31\x37\x31\x31\x2e\x35\x39\x34\x37\x57\xfb",12);							// RX705 sends 117°11.5947W which should be 11706.95685W in ddmm.mmmm
				val = DEVO_text_to_int(&packet[1], 3)*100;															// dd00
				val32 = DEVO_text_to_int(&packet[4], 2) * 10000 + DEVO_text_to_int(&packet[7], 4);					// mmmmmm
				if(option&0x02)																						// if RX705 GPS format
					val32 = (val32*3)/5;																			// then * 6/10 correction
				dec = val32/10000;
				val = val + dec;																					// dddmm
				frsky_send_user_frame(0x12  , val, val>>8);
				val = val32 - dec*10000;																			// .mmmm
				frsky_send_user_frame(0x12+8, val, val>>8);
				frsky_send_user_frame(0x1A+8, packet[11], 0x00);													// 'E'/'W'
				break;
			case 0x33: // Latitude
				//memcpy(&packet[1],"\x35\x39\x35\x34\x2e\x37\x37\x37\x36\x4e\x07\x00",12);							// 59°54.776N in ddmm.mmmm
				//memcpy(&packet[1],"\x31\x37\x31\x31\x2e\x35\x39\x34\x37\x4e\xfb\x00",12);							// RX705 sends 17°11.5947N which should be 1706.95685N in ddmm.mmmm
				val = DEVO_text_to_int(&packet[1], 2)*100;															// dd00
				val32 = DEVO_text_to_int(&packet[3], 2) * 10000 + DEVO_text_to_int(&packet[6], 4);					// mmmmmm
				if(option&0x02)																						// if RX705 GPS format
					val32 = (val32*3)/5;																			// then * 6/10 correction
				dec = val32/10000;
				val = val + dec;																					// dddmm
				frsky_send_user_frame(0x13  , val, val>>8);
				val = val32 - dec*10000;																			// .mmmm
				frsky_send_user_frame(0x13+8, val, val>>8);
				frsky_send_user_frame(0x1B+8, packet[10], 0x00);													// 'N'/'S'
				break;
			case 0x34: // Altitude
				//memcpy(&packet[1],"\x31\x32\x2e\x38\x00\x00\x00\x4d\x4d\x4e\x45\xfb",12);							// 12.8 MMNE
				DEVO_float_to_ints(&packet[1], &val, &dec);
				frsky_send_user_frame(0x10, val, val>>8);
				frsky_send_user_frame(0x21, dec, dec>>8);
				break;
			case 0x35: // Speed
				//memcpy(&packet[1],"\x00\x00\x00\x00\x00\x00\x30\x2e\x30\x30\x00\x00",12);							// 0.0
				DEVO_float_to_ints(&packet[1], &val, &dec);
				frsky_send_user_frame(0x11  , val, val>>8);
				frsky_send_user_frame(0x11+8, dec, dec>>8);
				break;
			case 0x36: // Time
				//memcpy(&packet[1],"\x31\x38\x32\x35\x35\x32\x31\x35\x31\x30\x31\x32",12);							// "182552151012" = 2012-10-15 18:25:52 (UTC)
				if(packet[1]!=0)
				{
					frsky_send_user_frame(0x15, DEVO_text_to_int(&packet[9], 2), DEVO_text_to_int(&packet[7], 2));	// month, day
					val = 2000 + DEVO_text_to_int(&packet[11], 2);													// year
					frsky_send_user_frame(0x16, val, val>>8);
					frsky_send_user_frame(0x17, DEVO_text_to_int(&packet[1], 2), DEVO_text_to_int(&packet[3], 2));	// hour, min
					frsky_send_user_frame(0x18, DEVO_text_to_int(&packet[5], 2), 0x00);								// second
				}
				break;
		}
	#else
		if(packet[0] == 0x30)
		{
			TX_RSSI = CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
			TX_RSSI = (TX_RSSI << 1) + TX_RSSI;
			RX_RSSI = TX_RSSI;
			telemetry_link |= 1;
			v_lipo1 = packet[1] << 1;
			v_lipo2 = packet[3] << 1;
		}
	#endif
}
#endif

static void __attribute__((unused)) DEVO_cyrf_set_bound_sop_code()
{
	/* crc == 0 isn't allowed, so use 1 if the math results in 0 */
	uint8_t crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
	if(! crc)
		crc = 1;
	uint8_t sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
	CYRF_SetTxRxMode(TX_EN);
	CYRF_ConfigCRCSeed((crc << 8) + crc);
	CYRF_PROGMEM_ConfigSOPCode(DEVO_j6pro_sopcodes[sopidx]);
	CYRF_SetPower(0x08);
}

const uint8_t PROGMEM DEVO_init_vals[][2] = {
	{ CYRF_1D_MODE_OVERRIDE, 0x38 },
	{ CYRF_03_TX_CFG, 0x08 },
	{ CYRF_06_RX_CFG, 0x4A },
	{ CYRF_0B_PWR_CTRL, 0x00 },
	{ CYRF_10_FRAMING_CFG, 0xA4 },
	{ CYRF_11_DATA32_THOLD, 0x05 },
	{ CYRF_12_DATA64_THOLD, 0x0E },
	{ CYRF_1B_TX_OFFSET_LSB, 0x55 },
	{ CYRF_1C_TX_OFFSET_MSB, 0x05 },
	{ CYRF_32_AUTO_CAL_TIME, 0x3C },
	{ CYRF_35_AUTOCAL_OFFSET, 0x14 },
	{ CYRF_39_ANALOG_CTRL, 0x01 },
	{ CYRF_1E_RX_OVERRIDE, 0x10 },
	{ CYRF_1F_TX_OVERRIDE, 0x00 },
	{ CYRF_01_TX_LENGTH, 0x10 },
	{ CYRF_0F_XACT_CFG, 0x10 },
	{ CYRF_27_CLK_OVERRIDE, 0x02 },
	{ CYRF_28_CLK_EN, 0x02 },
	{ CYRF_0F_XACT_CFG, 0x28 }
};

static void __attribute__((unused)) DEVO_cyrf_init()
{
	/* Initialise CYRF chip */
	for(uint8_t i = 0; i < sizeof(DEVO_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte( &DEVO_init_vals[i][0]), pgm_read_byte( &DEVO_init_vals[i][1]) );
}

static void __attribute__((unused)) DEVO_set_radio_channels()
{
	CYRF_FindBestChannels(hopping_frequency, 3, 4, 4, 80);
	hopping_frequency[3] = hopping_frequency[0];
	hopping_frequency[4] = hopping_frequency[1];
}

static void __attribute__((unused)) DEVO_BuildPacket()
{
	static uint8_t failsafe_pkt=0;
	switch(phase)
	{
		case DEVO_BIND:
			if(bind_counter)
				bind_counter--;
			DEVO_build_bind_pkt();
			phase = DEVO_BIND_SENDCH;
			break;
		case DEVO_BIND_SENDCH:
			if(bind_counter)
				bind_counter--;
			DEVO_build_data_pkt();
			DEVO_scramble_pkt();
			if (bind_counter == 0)
			{
				phase = DEVO_BOUND;
				BIND_DONE;
			}
			else
				phase = DEVO_BIND;
			break;
		case DEVO_BOUND:
		case DEVO_BOUND_1:
		case DEVO_BOUND_2:
		case DEVO_BOUND_3:
		case DEVO_BOUND_4:
		case DEVO_BOUND_5:
		case DEVO_BOUND_6:
		case DEVO_BOUND_7:
		case DEVO_BOUND_8:
		case DEVO_BOUND_9:
			DEVO_build_data_pkt();
			DEVO_scramble_pkt();
			phase++;
			if (bind_counter)
			{
				bind_counter--;
				if (bind_counter == 0)
					BIND_DONE;
			}
			break;
		case DEVO_BOUND_10:
			DEVO_build_beacon_pkt(num_ch > 8 ? failsafe_pkt : 0);
			failsafe_pkt = failsafe_pkt ? 0 : 1;
			DEVO_scramble_pkt();
			phase = DEVO_BOUND_1;
			break;
	}
	packet_count++;
	if(packet_count == DEVO_PKTS_PER_CHANNEL)
		packet_count = 0;
}

uint16_t DEVO_callback()
{
	static uint8_t txState=0;
	
#if defined DEVO_HUB_TELEMETRY
	int delay;
	
	if (txState == 0)
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(2400);
		#endif
		DEVO_BuildPacket();
		CYRF_WriteDataPacket(packet);
		txState = 1;
		return 900;
	}
	if (txState == 1)
	{
		int i = 0;
		uint8_t reg;
		while (! ((reg = CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)) & 0x02))
		{
			if (++i >= DEVO_NUM_WAIT_LOOPS)
				break;
		}
		if (((reg & 0x22) == 0x20) || (CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80))
		{
			CYRF_Reset();
			DEVO_cyrf_init();
			DEVO_cyrf_set_bound_sop_code();
			CYRF_ConfigRFChannel(*hopping_frequency_ptr);
			//printf("Rst CYRF\n");
			delay = 1500;
			txState = 15;
		}
		else
		{
			if (phase == DEVO_BOUND)
			{
				/* exit binding state */
				phase = DEVO_BOUND_3;
				DEVO_cyrf_set_bound_sop_code();
			}
			if((packet_count != 0) && (bind_counter == 0))
			{
				CYRF_SetTxRxMode(RX_EN); //Receive mode
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87); //0x80??? //Prepare to receive
				txState = 2;
				return 1300;
			}
		}
		if(packet_count == 0)
		{
			CYRF_SetPower(0x08);		//Keep tx power updated
			hopping_frequency_ptr = hopping_frequency_ptr == &hopping_frequency[2] ? hopping_frequency : hopping_frequency_ptr + 1;
			CYRF_ConfigRFChannel(*hopping_frequency_ptr);
		}
		delay = 1500;
	}
	if(txState == 2)
	{
		uint8_t rx_state = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
		if((rx_state & 0x03) == 0x02)
		{  // RXC=1, RXE=0 then 2nd check is required (debouncing)
			rx_state |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);   
		}
		if((rx_state & 0x07) == 0x02)
		{ // good data (complete with no errors)
			CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
			CYRF_ReadDataPacketLen(packet, CYRF_ReadRegister(CYRF_09_RX_COUNT));
			DEVO_parse_telemetry_packet();
		}
		CYRF_SetTxRxMode(TX_EN); //Write mode
		delay = 200;
	}
	txState = 0;   
	return delay;
#else
	if (txState == 0)
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(2400);
		#endif
		txState = 1;
		DEVO_BuildPacket();
		CYRF_WriteDataPacket(packet);
		return 1200;
	}
	txState = 0;
	uint8_t i = 0;
	while (! (CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02))
		if(++i > DEVO_NUM_WAIT_LOOPS)
			return 1200;
	if (phase == DEVO_BOUND)
	{
		/* exit binding state */
		phase = DEVO_BOUND_3;
		DEVO_cyrf_set_bound_sop_code();
	}   
	if(packet_count == 0)
	{
		CYRF_SetPower(0x08);		//Keep tx power updated
		hopping_frequency_ptr = hopping_frequency_ptr == &hopping_frequency[2] ? hopping_frequency : hopping_frequency_ptr + 1;
		CYRF_ConfigRFChannel(*hopping_frequency_ptr);
	}
	return 1200;
#endif
}

void DEVO_init()
{	
	#ifdef ENABLE_PPM
		if(mode_select) //PPM mode
		{
			if(IS_BIND_BUTTON_FLAG_on)
			{
				option &= 0xFE;
				eeprom_write_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num),option);	// reset to autobind mode for the current model
			}
			else
			{	
				option=eeprom_read_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num));		// load previous mode: autobind or fixed id
				if(option > 3) option = 0;												// if invalid then it should be autobind
			}
		}
	#endif //ENABLE_PPM
	switch(sub_protocol)
	{
		case 1:
			num_ch=10;
			break;
		case 2:
			num_ch=12;
			break;
		case 3:
			num_ch=6;
			break;
		case 4:
			num_ch=7;
			break;
		default:
			num_ch=8;
			break;
	}
	DEVO_cyrf_init();
	CYRF_GetMfgData(cyrfmfg_id);
	CYRF_SetTxRxMode(TX_EN);
	CYRF_ConfigCRCSeed(0x0000);
	CYRF_PROGMEM_ConfigSOPCode(DEVO_j6pro_sopcodes[0]);
	DEVO_set_radio_channels();

	hopping_frequency_ptr = hopping_frequency;
	CYRF_ConfigRFChannel(*hopping_frequency_ptr);

	packet_count = 0;

	if(option&0x01)
	{
		phase = DEVO_BOUND_1;
		bind_counter = 0;
		DEVO_cyrf_set_bound_sop_code();
	}
	else
	{
		MProtocol_id = ((uint32_t)(hopping_frequency[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
					 | ((uint32_t)(hopping_frequency[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
					 | ((uint32_t)(hopping_frequency[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
		MProtocol_id %= 1000000;
		bind_counter = DEVO_BIND_COUNT;
		phase = DEVO_BIND;
		BIND_IN_PROGRESS;
	}
}

#endif
