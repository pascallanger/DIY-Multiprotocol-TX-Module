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

#if defined(LOSI_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define LOSI_FORCE_ID

const uint8_t PROGMEM LOSI_bind_sop_code[] = {0x62, 0xdf, 0xc1, 0x49, 0xdf, 0xb1, 0xc0, 0x49};
const uint8_t LOSI_data_code[][16] = {
	{ 0xD7, 0xA1, 0x54, 0xB1, 0x5E, 0x89, 0xAE, 0x86, 0xC9, 0x2C, 0x06, 0x93, 0x86, 0xB9, 0x9E, 0xD7 }, //bind
	{ 0xE1, 0xD6, 0x31, 0x26, 0x5F, 0xBD, 0x40, 0x93, 0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C },
	{ 0xDC, 0x68, 0x08, 0x99, 0x97, 0xAE, 0xAF, 0x8C, 0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA },
	{ 0xC3, 0x0E, 0x01, 0x16, 0x0E, 0x32, 0x06, 0xBA, 0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC },
	{ 0xE0, 0x83, 0x01, 0xFA, 0xAB, 0x3E, 0x8F, 0xAC, 0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84 },
	{ 0x5C, 0xD5, 0x9C, 0xB8, 0x46, 0x9C, 0x7D, 0x84, 0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7 },
	{ 0xF1, 0xC6, 0xFE, 0x5C, 0x9D, 0xA5, 0x4F, 0xB7, 0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0 },
	{ 0x58, 0xB5, 0xB3, 0xDD, 0x0E, 0x28, 0xF1, 0xB0, 0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1 },
	{ 0x5F, 0x30, 0x3B, 0x56, 0x96, 0x45, 0xF4, 0xA1, 0x03, 0xBC, 0x6E, 0x8A, 0xEF, 0xBD, 0xFE, 0xF8 }
};

static uint16_t __attribute__((unused)) LOSI_check(uint16_t val)
{
	const uint8_t PROGMEM tab[] = { 0xF1, 0xDA, 0xB6, 0xC8 };
	uint8_t res = 0x0B, tmp;
	uint16_t calc = val>>2; 		// don't care about the 2 first bits
	for(uint8_t i=0; i<5; i++)
	{
		tmp=pgm_read_byte_near(&tab[i&0x03]);
		if(calc&0x0001)
			res ^= tmp>>4;
		calc >>= 1;
		if(calc&0x0001)
			res ^= tmp;
		calc >>= 1;
	}
	return val ^ (res<<12);
}

static void __attribute__((unused)) LOSI_send_packet()
{
	memcpy(packet, rx_tx_addr, 4);
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(&packet[4], rx_tx_addr, 4);
		packet[8] = 0x05;                   // CRC?
		packet[9] = 0x52;                   // CRC?
	}
	else
	{
		for(uint8_t i=0; i<3; i++)
		{
			uint16_t val = LOSI_check(Channel_data[i]<<1);
			packet[4+i*2] = val >> 8;
			packet[5+i*2] = val;
		}
	}

	CYRF_SetPower(0x38);
	CYRF_WriteDataPacketLen(packet, 0x0A);
	#if 0
		for(uint8_t i=0; i < 0x0A; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) LOSI_cyrf_init()
{
	/* Initialise CYRF chip */
	CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
	CYRF_WriteRegister(CYRF_06_RX_CFG, 0x48);
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
	//CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
	CYRF_SetPower(0x38);
	CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0A);
	CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);
	CYRF_WritePreamble(0x333304);
	//CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x00);
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0x4A);
	CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x04);			// No CRC
	//CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x14);
	//CYRF_WriteRegister(CYRF_14_EOP_CTRL, 0x02);
}

uint16_t LOSI_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(19738);
	#endif
	LOSI_send_packet();
	if(bind_counter)
	{
		bind_counter--;
		if(bind_counter==0)
		{
			BIND_DONE;
			CYRF_ConfigDataCode(LOSI_data_code[num_ch], 16);	// Load normal data code
		}
		return 8763;
	}
	return 19738;
}

void LOSI_init()
{
	LOSI_cyrf_init();
	CYRF_FindBestChannels(hopping_frequency, 1, 0, 0x13, 75);	// 75 is unknown since dump stops at 0x27, this routine resets the CRC Seed to 0
	CYRF_ConfigRFChannel(hopping_frequency[0] | 1);				// Only odd channels

    #ifdef LOSI_FORCE_ID
		rx_tx_addr[0] = 0x47;
		rx_tx_addr[1] = 0x52;
		rx_tx_addr[2] =	0xAE;
		rx_tx_addr[3] =	0xAA;
		num_ch = 8;												// It looks that you could choose what you want based on channel business
	#endif

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = 300;
		CYRF_ConfigDataCode(LOSI_data_code[0], 16);				// Load bind data code
	}
	else
	{
		CYRF_ConfigDataCode(LOSI_data_code[num_ch], 16);		// Load normal data code
		bind_counter = 0;
	}
}

#endif
