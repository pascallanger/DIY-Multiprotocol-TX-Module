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

#if defined(WK2x01_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define WK_BIND_COUNT 2980
#define WK_NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us

enum {
	WK_BIND=0,
	WK_BOUND_1,
	WK_BOUND_2,
	WK_BOUND_3,
	WK_BOUND_4,
	WK_BOUND_5,
	WK_BOUND_6,
	WK_BOUND_7,
	WK_BOUND_8,
};

static const uint8_t WK_sopcodes[8] = {
    /* Note these are in order transmitted (LSB 1st) */
    0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49 //0x49C1DF6249C0B1DF
};
static const uint8_t init_2801[] = {0xc5, 0x34, 0x60, 0x00, 0x25};
static const uint8_t init_2601[] = {0xb9, 0x45, 0xb0, 0xf1, 0x3a};
static const uint8_t init_2401[] = {0xa5, 0x23, 0xd0, 0xf0, 0x00};

uint8_t WK_last_beacon;

static void __attribute__((unused)) WK_add_pkt_crc(uint8_t init)
{
	uint8_t add = init;
	uint8_t xou = init;
	for (uint8_t i = 0; i < 14; i++)
	{
		add += packet[i];
		xou ^= packet[i];
	}
	packet[14] = xou;
	packet[15] = add;
}

static void __attribute__((unused)) WK_build_bind_pkt(const uint8_t *init)
{
	packet[0] = init[0];
	packet[1] = init[1];
	packet[2] = hopping_frequency[0];
	packet[3] = hopping_frequency[1];
	packet[4] = init[2];
	packet[5] = hopping_frequency[2];
	packet[6] = 0xff;
	packet[7] = 0x00;
	packet[8] = 0x00;
	packet[9] = 0x32;
	if (sub_protocol == WK2401)
		packet[10]  = 0x10 | (rx_tx_addr[0]  & 0x0e);
	else
		packet[10]  = rx_tx_addr[0];
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2] | packet_count;
	packet[13] = init[3];
	WK_add_pkt_crc(init[4]);
}

static int16_t __attribute__((unused)) WK_get_channel(uint8_t ch, int32_t scale, int16_t center, int16_t range)
{
	int16_t value = convert_channel_16b_nolimit(CH_AETR[ch],-scale,scale)+center;
	if (value < center - range) value = center - range;
	if (value > center + range) value = center + range;
	return value;
}

static void __attribute__((unused)) WK_build_data_pkt_2401()
{
	uint16_t msb = 0;
	uint8_t offset = 0;
	for (uint8_t i = 0; i < 4; i++)
	{
		if (i == 2)
			offset = 1;
		int16_t value = WK_get_channel(i, 0x800, 0, 0xA00);	//12 bits, allow value to go to 125%
		uint16_t base = abs(value) >> 2;					//10 bits is the base value
		uint16_t trim = abs(value) & 0x03;					//lowest 2 bits represent trim
		if (base >= 0x200)
		{  //if value is > 100%, remainder goes to trim
			trim = 4 *(base - 0x200);
			base = 0x1ff;
		}
		base = (value >= 0) ? 0x200 + base : 0x200 - base;
		trim = (value >= 0) ? 0x200 + trim : 0x200 - trim;

		packet[2*i+offset]   = base & 0xff;
		packet[2*i+offset+1] = trim & 0xff;
		msb = (msb << 4) | ((base >> 6) & 0x0c) | ((trim >> 8) & 0x03);
	}
	packet[4] = msb >> 8; //Ele/Ail MSB
	packet[9] = msb & 0xff; //Thr/Rud MSB
	packet[10]  = 0xe0 | (rx_tx_addr[0]  & 0x0e);
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2] | packet_count;
	packet[13] = 0xf0; //FIXME - What is this?
	WK_add_pkt_crc(0x00);
}

#define PCT(pct, max) (((int32_t)(max) * (int32_t)(pct) + 1L) / 1000L)
#define MAXTHR 426 //Measured to provide equal value at +/-0
static void __attribute__((unused)) WK_channels_6plus1_2601(uint8_t frame, int16_t *_v1, int16_t *_v2)
{
	int16_t thr = WK_get_channel(2, 1000, 0, 1000);
	int16_t v1;
	uint8_t thr_rev = 0, pitch_rev = 0;
	if(thr > 0)
	{
		if(thr >= 780)
		{ //78%
			v1 = 0; //thr = 60% * (x - 78%) / 22% + 40%
			thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
		}
		else
		{
			v1 = 1023 - 1023 * thr / 780;
			thr = PCT(MAXTHR, 512); //40%
		}
	}
	else
	{
		thr = -thr;
		thr_rev = 1;
		if(thr >= 780)
		{ //78%
			v1 = 1023; //thr = 60% * (x - 78%) / 22% + 40%
			thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
		}
		else
		{
			v1 = 1023 * thr / 780;
			thr = PCT(MAXTHR, 512); //40%
		}
	}
	if (thr >= 512)
		thr = 511;
	packet[2] = thr & 0xff;
	packet[4] = (packet[4] & 0xF3) | ((thr >> 6) & 0x04);

	int16_t pitch= WK_get_channel(5, 0x400, 0, 0x400);
	if (pitch < 0)
	{
		pitch_rev = 1;
		pitch = -pitch;
	}
	if (frame == 1)
	{
		//Pitch curve and range
		if (thr > PCT(MAXTHR, 512))
			*_v2 = pitch - pitch * 16 * (thr - PCT(MAXTHR, 512)) / PCT(1000 - MAXTHR, 512) / 100;
		else
			*_v2 = pitch;
		*_v1 = 0;
	}
	else
		if (frame == 2)
		{
			//Throttle curve & Expo
			*_v1 = v1;
			*_v2 = 512;
		}
	packet[7] = (thr_rev << 5) | (pitch_rev << 2); //reverse bits
	packet[8] = 0;
}

static void __attribute__((unused)) WK_channels_5plus1_2601(uint8_t frame, int16_t *v1, int16_t *v2)
{
	(void)v1;
	//Zero out pitch, provide ail, ele, thr, rud, gyr + gear
	if (frame == 1)
		*v2 = 0;					//Pitch curve and range
	packet[7] = 0;
	packet[8] = 0;
}
static void __attribute__((unused)) WK_channels_heli_2601(uint8_t frame, int16_t *v1, int16_t *v2)
{
	//pitch is controlled by rx
	//we can only control fmode, pit-reverse and pit/thr rate
	uint8_t pit_rev = 0;
	if (sub_protocol==W6_HEL_I)
		pit_rev = 1;
	int16_t pit_rate = WK_get_channel(5, 0x400, 0, 0x400);
	uint8_t fmode = 1;
	if (pit_rate < 0)
	{
		pit_rate = -pit_rate;
		fmode = 0;
	}
	if (frame == 1)
	{
		//Pitch curve and range
		*v1 = pit_rate;
		*v2 = (int16_t)(option) * 0x400 / 100 + 0x400;
	}
	packet[7] = (pit_rev << 2);		//reverse bits
	packet[8] = fmode ? 0x02 : 0x00;
}

static void __attribute__((unused)) WK_build_data_pkt_2601()
{
	uint8_t msb = 0;
	uint8_t frame = (packet_count % 3);
	for (uint8_t i = 0; i < 4; i++)
	{
		int16_t value = WK_get_channel(i, 0x190, 0, 0x1FF);
		uint16_t mag = value < 0 ? -value : value;
		packet[i] = mag & 0xff;
		msb = (msb << 2) | ((mag >> 8) & 0x01) | (value < 0 ? 0x02 : 0x00);
	}
	packet[4] = msb;
	int16_t v1 = 0x200, v2 = 0x200;
	if (frame == 0)
	{
		//Gyro & Rudder mix
		v1 = WK_get_channel(6, 0x200, 0x200, 0x200);
		v2 = 0;
	}
	if (sub_protocol == W6_5_1)
		WK_channels_5plus1_2601(frame, &v1, &v2);
	else if (sub_protocol == W6_6_1)
		WK_channels_6plus1_2601(frame, &v1, &v2);
	else
		WK_channels_heli_2601(frame, &v1, &v2);
	if (v1 > 1023)
		v1 = 1023;
	if (v2 > 1023)
		v2 = 1023;
	packet[5] = v2 & 0xff;
	packet[6] = v1 & 0xff;
	//packet[7] handled by channel code
	packet[8] |= (WK_get_channel(4, 0x190, 0, 0x1FF) > 0 ? 1 : 0);
	packet[9] =  ((v1 >> 4) & 0x30) | ((v2 >> 2) & 0xc0) | 0x04 | frame;
	packet[10]  = rx_tx_addr[0];
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2] | packet_count;
	packet[13] = 0xff;

	WK_add_pkt_crc(0x3A);
}

static void __attribute__((unused)) WK_build_data_pkt_2801()
{
	uint16_t msb = 0;
	uint8_t offset = 0;
	uint8_t sign = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (i == 4) { offset = 1; }
		int16_t value = WK_get_channel(i, 0x1C2, 0, 0x3FF);
		uint16_t mag = value < 0 ? -value : value;
		packet[i+offset] = mag & 0xff;
		msb = (msb << 2) | ((mag >> 8) & 0x03);
		if (value < 0) { sign |= 1 << i; }
	}
	packet[4] = msb >> 8;
	packet[9] = msb  & 0xff;
	packet[10] = rx_tx_addr[0];
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2] | packet_count;
	packet[13] = sign;
	WK_add_pkt_crc(0x25);
}

static void __attribute__((unused)) WK_build_beacon_pkt_2801()
{
	WK_last_beacon ^= 1;
	uint8_t en = 0;
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
		set_rx_tx_addr(MProtocol_id);
		rx_tx_addr[2]=rx_tx_addr[3]<<4;		// Make use of RX_num
		bind_counter = WK_BIND_COUNT / 8 + 1;
	}
	if (option)
	{
        if (bind_counter)
            bind_state = 0xe4;
        else
            bind_state = 0x1b;
    }
	else
        bind_state = 0x99;
	
	for (uint8_t i = 0; i < 4; i++)
	{
		#ifdef FAILSAFE_ENABLE
			uint16_t failsafe=Failsafe_data[CH_AETR[i + WK_last_beacon * 4]];
			if(failsafe!=FAILSAFE_CHANNEL_HOLD && IS_FAILSAFE_VALUES_on)
			{
				packet[i+1] = failsafe>>3;	//0..255
				en |= 1 << i;
			}
			else
		#endif
				packet[i+1] = 0;
	}
	packet[0] = en;
	packet[5] = packet[4];
	packet[4] = WK_last_beacon << 6;
	packet[6] = hopping_frequency[0];
	packet[7] = hopping_frequency[1];
	packet[8] = hopping_frequency[2];
	packet[9] = bind_state;
	packet[10] = rx_tx_addr[0];
	packet[11] = rx_tx_addr[1];
	packet[12] = rx_tx_addr[2] | packet_count;
	packet[13] = 0x00; //Does this matter?  in the docs it is the same as the data packet
	WK_add_pkt_crc(0x1C);
}

static void __attribute__((unused)) wk2x01_cyrf_init() {
	/* Initialize CYRF chip */
	CYRF_SetPower(0x28);
	CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
	CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
	CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
	CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xEE);
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
	CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x18);
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x90);
	CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
	CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
	CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
	CYRF_ConfigSOPCode(WK_sopcodes);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x28);
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
	CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
}

static void __attribute__((unused)) WK_BuildPacket_2801()
{
	switch(phase) {
		case WK_BIND:
			bind_counter--;
			WK_build_bind_pkt(init_2801);
			if (bind_counter == 0)
			{
				BIND_DONE;
				phase++;
			}
			break;
		case WK_BOUND_1:
		case WK_BOUND_2:
		case WK_BOUND_3:
		case WK_BOUND_4:
		case WK_BOUND_5:
		case WK_BOUND_6:
		case WK_BOUND_7:
			WK_build_data_pkt_2801();
			phase++;
			break;
		case WK_BOUND_8:
			WK_build_beacon_pkt_2801();
			phase = WK_BOUND_1;
			if (bind_counter)
			{
				bind_counter--;
				if (bind_counter == 0)
					BIND_DONE;
			}
			break;
	}
}

static void __attribute__((unused)) WK_BuildPacket_2601()
{
	if (bind_counter)
	{
		bind_counter--;
		WK_build_bind_pkt(init_2601);
		if (bind_counter == 0)
			BIND_DONE;
	}
	else
		WK_build_data_pkt_2601();
}

static void __attribute__((unused)) WK_BuildPacket_2401()
{
	if (bind_counter)
	{
		bind_counter--;
		WK_build_bind_pkt(init_2401);
		if(bind_counter == 0)
			BIND_DONE;
	}
	else
		WK_build_data_pkt_2401();
}

uint16_t WK_cb()
{
	if (packet_sent == 0)
	{
		packet_sent = 1;
		if(sub_protocol == WK2801)
			WK_BuildPacket_2801();
		else if(sub_protocol == WK2401)
			WK_BuildPacket_2401();
		else
			WK_BuildPacket_2601();
		packet_count = (packet_count + 1) % 12;
		CYRF_WriteDataPacket(packet);
		return 1600;
	}
	packet_sent = 0;
	uint8_t start=micros();
	while ((uint8_t)micros()-start < 100)			// Wait max 100µs
		if(CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x02)
			break;
	if((packet_count & 0x03) == 0)
	{
		hopping_frequency_no++;
		hopping_frequency_no%=3;
		CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
		//Keep transmit power updated
		CYRF_SetPower(0x28);
	}
	return 1200;
}

uint16_t WK_setup()
{
	wk2x01_cyrf_init();
	CYRF_SetTxRxMode(TX_EN);

	hopping_frequency_no=0;
	CYRF_FindBestChannels(hopping_frequency, 3, 4, 4, 80);
	CYRF_ConfigRFChannel(hopping_frequency[0]);

	packet_count = 0;
	packet_sent = 0;
	WK_last_beacon = 0;
	prev_option=option;
	if(sub_protocol!=WK2801 || option==0)
	{
		CYRF_GetMfgData(cyrfmfg_id);
		rx_tx_addr[2]=(hopping_frequency[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3])<<4;
		rx_tx_addr[1]=hopping_frequency[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4];
		rx_tx_addr[0]=hopping_frequency[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5];
		if(sub_protocol == WK2401)
			rx_tx_addr[0] |= 0x01;			//ID must be odd for 2401

		bind_counter = WK_BIND_COUNT;
		phase = WK_BIND;
		BIND_IN_PROGRESS;
	}
	else
	{
		rx_tx_addr[2]=rx_tx_addr[3]<<4;		// Make use of RX_num
		bind_counter = 0;
		phase = WK_BOUND_1;
		BIND_DONE;
	}
	return 2800;
}

#endif
