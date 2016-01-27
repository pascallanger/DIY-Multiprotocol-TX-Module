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
#define PKTS_PER_CHANNEL 4
#define DEVO_BIND_COUNT 0x1388
//#define TELEMETRY_ENABLE 0x30
#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us
//
//#define TELEM_ON 0
//#define TELEM_OFF 1

enum Devo_PhaseState
{
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

const uint8_t sopcodes[][8] = {
	/* Note these are in order transmitted (LSB 1st) */
	/* 0 */ {0x3C,0x37,0xCC,0x91,0xE2,0xF8,0xCC,0x91}, //0x91CCF8E291CC373C
	/* 1 */ {0x9B,0xC5,0xA1,0x0F,0xAD,0x39,0xA2,0x0F}, //0x0FA239AD0FA1C59B
	/* 2 */ {0xEF,0x64,0xB0,0x2A,0xD2,0x8F,0xB1,0x2A}, //0x2AB18FD22AB064EF
	/* 3 */ {0x66,0xCD,0x7C,0x50,0xDD,0x26,0x7C,0x50}, //0x507C26DD507CCD66
	/* 4 */ {0x5C,0xE1,0xF6,0x44,0xAD,0x16,0xF6,0x44}, //0x44F616AD44F6E15C
	/* 5 */ {0x5A,0xCC,0xAE,0x46,0xB6,0x31,0xAE,0x46}, //0x46AE31B646AECC5A
	/* 6 */ {0xA1,0x78,0xDC,0x3C,0x9E,0x82,0xDC,0x3C}, //0x3CDC829E3CDC78A1
	/* 7 */ {0xB9,0x8E,0x19,0x74,0x6F,0x65,0x18,0x74}, //0x7418656F74198EB9
	/* 8 */ {0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49}, //0x49C1DF6249C0B1DF
	/* 9 */ {0x97,0xE5,0x14,0x72,0x7F,0x1A,0x14,0x72}, //0x72141A7F7214E597
};

uint8_t txState;
uint8_t pkt_num;
uint8_t ch_idx;
uint8_t use_fixed_id;
uint8_t failsafe_pkt;

static void __attribute__((unused)) scramble_pkt()
{
#ifdef NO_SCRAMBLE
	return;
#else
	uint8_t i;
	for(i = 0; i < 15; i++)
		packet[i + 1] ^= cyrfmfg_id[i % 4];
#endif
}

static void __attribute__((unused)) add_pkt_suffix()
{
	uint8_t bind_state;
	if (use_fixed_id)
	{
		if (bind_counter > 0)
			bind_state = 0xc0;
		else
			bind_state = 0x80;
	}
	else
		bind_state = 0x00;
	packet[10] = bind_state | (PKTS_PER_CHANNEL - pkt_num - 1);
	packet[11] = *(hopping_frequency_ptr + 1);
	packet[12] = *(hopping_frequency_ptr + 2);
	packet[13] = fixed_id  & 0xff;
	packet[14] = (fixed_id >> 8) & 0xff;
	packet[15] = (fixed_id >> 16) & 0xff;
}

static void __attribute__((unused)) build_beacon_pkt(uint8_t upper)
{
	packet[0] = ((DEVO_NUM_CHANNELS << 4) | 0x07);
//	uint8_t enable = 0;
	uint8_t max = 8;
//	int offset = 0;
	if (upper)
	{
		packet[0] += 1;
		max = 4;
//		offset = 8;
	}
	for(uint8_t i = 0; i < max; i++)
		packet[i+1] = 0;
//	packet[9] = enable;
	packet[9] = 0;
	add_pkt_suffix();
}

static void __attribute__((unused)) build_bind_pkt()
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
	add_pkt_suffix();
	//The fixed-id portion is scrambled in the bind packet
	//I assume it is ignored
	packet[13] ^= cyrfmfg_id[0];
	packet[14] ^= cyrfmfg_id[1];
	packet[15] ^= cyrfmfg_id[2];
}

static void __attribute__((unused)) build_data_pkt()
{
	uint8_t i;
	packet[0] = (DEVO_NUM_CHANNELS << 4) | (0x0b + ch_idx);
	uint8_t sign = 0x0b;
	for (i = 0; i < 4; i++)
	{
		//
		int16_t value= map(Servo_data[ch_idx * 4 + i],PPM_MIN,PPM_MAX,-1600,1600);//range -1600...+1600
		//s32 value = (s32)Channels[ch_idx * 4 + i] * 0x640 / CHAN_MAX_VALUE;//10000
		if(value < 0)
		{
			value = -value;
			sign |= 1 << (7 - i);
		}
		packet[2 * i + 1] = value & 0xff;
		packet[2 * i + 2] = (value >> 8) & 0xff;
	}
	packet[9] = sign;
	ch_idx = ch_idx + 1;
	if (ch_idx * 4 >= DEVO_NUM_CHANNELS)
		ch_idx = 0;
	add_pkt_suffix();
}

static void __attribute__((unused)) cyrf_set_bound_sop_code()
{
	/* crc == 0 isn't allowed, so use 1 if the math results in 0 */
	uint8_t crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
	if(! crc)
		crc = 1;
	uint8_t sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
	CYRF_SetTxRxMode(TX_EN);
	CYRF_ConfigCRCSeed((crc << 8) + crc);
	CYRF_ConfigSOPCode(sopcodes[sopidx]);
	CYRF_SetPower(0x08);
}

static void __attribute__((unused)) cyrf_init()
{
	/* Initialise CYRF chip */
	CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x39);
	CYRF_SetPower(0x08);
	CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
	CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
	CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);
	CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xA4);
	CYRF_WriteRegister(CYRF_11_DATA32_THOLD, 0x05);
	CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0E);
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
	CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
	CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);
	CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x10);
	CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
	CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x28);
}

static void __attribute__((unused)) set_radio_channels()
{
	//int i;
	CYRF_FindBestChannels(hopping_frequency, 3, 4, 4, 80);
	//printf("Radio Channels:");
	// for (i = 0; i < 3; i++) {
	//     printf(" %02x", radio_ch[i]);

	//Serial.print(radio_ch[i]);
	// }
	// printf("\n");
	//Makes code a little easier to duplicate these here
	hopping_frequency[3] = hopping_frequency[0];
	hopping_frequency[4] = hopping_frequency[1];
}

static void __attribute__((unused)) DEVO_BuildPacket()
{
	switch(phase)
	{
		case DEVO_BIND:
			if(bind_counter>0)
				bind_counter--;
			build_bind_pkt();
			phase = DEVO_BIND_SENDCH;
			break;
		case DEVO_BIND_SENDCH:
			if(bind_counter>0)
				bind_counter--;
			build_data_pkt();
			scramble_pkt();
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
			build_data_pkt();
			scramble_pkt();
			phase++;
			if (bind_counter > 0)
			{
				bind_counter--;
				if (bind_counter == 0)
					BIND_DONE;
			}
			break;
		case DEVO_BOUND_10:
			build_beacon_pkt(DEVO_NUM_CHANNELS > 8 ? failsafe_pkt : 0);
			failsafe_pkt = failsafe_pkt ? 0 : 1;
			scramble_pkt();
			phase = DEVO_BOUND_1;
			break;
	}
	pkt_num++;
	if(pkt_num == PKTS_PER_CHANNEL)
		pkt_num = 0;
}

uint16_t devo_callback()
{
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
		if(++i > NUM_WAIT_LOOPS)
			return 1200;
	if (phase == DEVO_BOUND)
	{
		/* exit binding state */
		phase = DEVO_BOUND_3;
		cyrf_set_bound_sop_code();
	}   
	if(pkt_num == 0)
	{
		//Keep tx power updated
		CYRF_SetPower(0x08);
		hopping_frequency_ptr = hopping_frequency_ptr == &hopping_frequency[2] ? hopping_frequency : hopping_frequency_ptr + 1;
		CYRF_ConfigRFChannel(*hopping_frequency_ptr);
	}
	return 1200;
}

/*static void __attribute__((unused)) devo_bind()
{
	fixed_id = Model_fixed_id;
	bind_counter = DEVO_BIND_COUNT;
	use_fixed_id = 1;
	//PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs 12000ms
}


static void __attribute__((unused)) generate_fixed_id_bind(){
if(BIND_0){
//randomSeed((uint32_t)analogRead(A6)<<10|analogRead(A7));//seed
uint8_t txid[4];
//Model_fixed_id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
Model_fixed_id=0x332211;
txid[0]=  (id &0xFF);
txid[1] = ((id >> 8) & 0xFF);
txid[2] = ((id >> 16) & 0xFF);
//txid[3] = ((id >> 24) & 0xFF);
eeprom_write_block((const void*)txid,(void*)40,3);
devo_bind();
}
}
*/


uint16_t DevoInit()
{	
	CYRF_Reset();
	cyrf_init();
	CYRF_GetMfgData(cyrfmfg_id);
	CYRF_SetTxRxMode(TX_EN);
	CYRF_ConfigCRCSeed(0x0000);
	CYRF_ConfigSOPCode(sopcodes[0]);
	set_radio_channels();
	use_fixed_id = 0;
	failsafe_pkt = 0;
	hopping_frequency_ptr = hopping_frequency;
	//
	CYRF_ConfigRFChannel(*hopping_frequency_ptr);
	//FIXME: Properly setnumber of channels;
	pkt_num = 0;
	ch_idx = 0;
	txState = 0;
	//uint8_t txid[4];
	//
	
	/*	
if(BIND_0){
Model_fixed_id=0;
eeprom_write_block((const void*)0,(void*)40,4);
while(1){
LED_ON;
delay(100);
LED_OFF;
delay(100);
}
}
else{
eeprom_read_block((void*)txid,(const void*)40,3);
Model_fixed_id=(txid[0] | ((uint32_t)txid[1]<<8) | ((uint32_t)txid[2]<<16));
}	
*/	
	
	if(! Model_fixed_id)
	{//model fixed ID =0
		fixed_id = ((uint32_t)(hopping_frequency[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
		| ((uint32_t)(hopping_frequency[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
		| ((uint32_t)(hopping_frequency[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);		
		fixed_id = fixed_id % 1000000;
		bind_counter = DEVO_BIND_COUNT;
		phase = DEVO_BIND;
		//PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs
	}
	else
	{
		fixed_id = Model_fixed_id;
		use_fixed_id = 1;
		phase = DEVO_BOUND_1;
		bind_counter = 0;
		cyrf_set_bound_sop_code();
	}  

	return 2400;
}

#endif
