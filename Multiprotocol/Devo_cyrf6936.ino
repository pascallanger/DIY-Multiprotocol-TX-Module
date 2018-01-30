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

#define DEVO_NUM_CHANNELS 8

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
	if(mode_select && option==0 && IS_BIND_DONE) 			//PPM mode and option not already set and bind is finished
	{
		BIND_SET_INPUT;
		BIND_SET_PULLUP;										// set pullup
		if(IS_BIND_BUTTON_on)
		{
			eeprom_write_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num),0x01);	// Set fixed id mode for the current model
			option=1;
		}
		BIND_SET_OUTPUT;
	}
	#endif //ENABLE_PPM
    if(prev_option!=option && IS_BIND_DONE)
	{
		MProtocol_id = RX_num + MProtocol_id_master;
		bind_counter=DEVO_BIND_COUNT;
	}
	if (option)
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
	packet[0] = (DEVO_NUM_CHANNELS << 4) | 0x07;
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
			if(i + offset < DEVO_NUM_CHANNELS && failsafe!=FAILSAFE_CHANNEL_HOLD && IS_FAILSAFE_VALUES_on)
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
	packet[0] = (DEVO_NUM_CHANNELS << 4) | 0x0a;
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

	packet[0] = (DEVO_NUM_CHANNELS << 4) | (0x0b + ch_idx);
	uint8_t sign = 0x0b;
	for (uint8_t i = 0; i < 4; i++)
	{
		int16_t value=convert_channel_16b_nolimit(CH_EATR[ch_idx * 4 + i],-1600,1600);//range -1600..+1600
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
	if (ch_idx * 4 >= DEVO_NUM_CHANNELS)
		ch_idx = 0;
	DEVO_add_pkt_suffix();
}

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
			DEVO_build_beacon_pkt(DEVO_NUM_CHANNELS > 8 ? failsafe_pkt : 0);
			failsafe_pkt = failsafe_pkt ? 0 : 1;
			DEVO_scramble_pkt();
			phase = DEVO_BOUND_1;
			break;
	}
	packet_count++;
	if(packet_count == DEVO_PKTS_PER_CHANNEL)
		packet_count = 0;
}

uint16_t devo_callback()
{
	static uint8_t txState=0;
	if (txState == 0)
	{
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
}

uint16_t DevoInit()
{	
	DEVO_cyrf_init();
	CYRF_GetMfgData(cyrfmfg_id);
	CYRF_SetTxRxMode(TX_EN);
	CYRF_ConfigCRCSeed(0x0000);
	CYRF_PROGMEM_ConfigSOPCode(DEVO_j6pro_sopcodes[0]);
	DEVO_set_radio_channels();

	hopping_frequency_ptr = hopping_frequency;
	CYRF_ConfigRFChannel(*hopping_frequency_ptr);

	packet_count = 0;

	prev_option=option;
	if(option==0)
	{
		MProtocol_id = ((uint32_t)(hopping_frequency[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
					 | ((uint32_t)(hopping_frequency[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
					 | ((uint32_t)(hopping_frequency[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
		MProtocol_id %= 1000000;
		bind_counter = DEVO_BIND_COUNT;
		phase = DEVO_BIND;
		BIND_IN_PROGRESS;
	}
	else
	{
		phase = DEVO_BOUND_1;
		bind_counter = 0;
		DEVO_cyrf_set_bound_sop_code();
	}  
	return 2400;
}

#endif
