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

#if defined(FRSKY1_CC2500_INO)

#include "iface_cc2500.h"
const PROGMEM uint8_t FRSKY1_cc2500_conf[][2]={
	{ CC2500_17_MCSM1, 0x0c },
	{ CC2500_18_MCSM0, 0x18 },
	{ CC2500_06_PKTLEN, 0xff },
	{ CC2500_07_PKTCTRL1, 0x04 },
	{ CC2500_08_PKTCTRL0, 0x05 },
	{ CC2500_3E_PATABLE, 0xfe },
	{ CC2500_0B_FSCTRL1, 0x08 },
	{ CC2500_0C_FSCTRL0, 0x00 },
	{ CC2500_0D_FREQ2, 0x5c },
	{ CC2500_0E_FREQ1, 0x58 },
	{ CC2500_0F_FREQ0, 0x9d },
	{ CC2500_10_MDMCFG4, 0xaa },
	{ CC2500_11_MDMCFG3, 0x10 },
	{ CC2500_12_MDMCFG2, 0x93 },
	{ CC2500_13_MDMCFG1, 0x23 },
	{ CC2500_14_MDMCFG0, 0x7a },
	{ CC2500_15_DEVIATN, 0x41 }
};

static void __attribute__((unused)) FRSKY1_init()
{
	for(uint8_t i=0;i<17;i++)
	{
		uint8_t reg=pgm_read_byte_near(&FRSKY1_cc2500_conf[i][0]);
		uint8_t val=pgm_read_byte_near(&FRSKY1_cc2500_conf[i][1]);
		if(reg==CC2500_0C_FSCTRL0)
			val=option;
		CC2500_WriteReg(reg,val);
	}
	prev_option = option ;
	for(uint8_t i=19;i<36;i++)
	{
		uint8_t reg=pgm_read_byte_near(&cc2500_conf[i][0]);
		uint8_t val=pgm_read_byte_near(&cc2500_conf[i][1]);
		CC2500_WriteReg(reg,val);
	}
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();

	CC2500_Strobe(CC2500_SIDLE);    // Go to idle...
}

static uint8_t __attribute__((unused)) FRSKY1_crc8(uint8_t result, uint8_t *data, uint8_t len, uint8_t polynomial)
{
	for(uint8_t i = 0; i < len; i++)
	{
		result = result ^ data[i];
		for(uint8_t j = 0; j < 8; j++)
			if(result & 0x80)
				result = (result << 1) ^ polynomial;
			else
				result = result << 1;
	}
	return result;
}

static uint8_t __attribute__((unused)) FRSKY1_crc8_le(uint8_t init, uint8_t *data, uint8_t len)
{
	uint8_t result = 0;

	for(uint8_t i = 0; i < 8; i++)
	{
		result = (result << 1) | (init & 0x01);
		init >>= 1;
	}
	return FRSKY1_crc8(result,data,len,0x83);
}

static void __attribute__((unused)) FRSKY1_build_bind_packet()
{
    //0e 03 01 57 12 00 06 0b 10 15 1a 00 00 00 61
    packet[0] = 0x0e;                //Length
    packet[1] = 0x03;                //Packet type
    packet[2] = 0x01;                //Packet type
    packet[3] = rx_tx_addr[3];
    packet[4] = rx_tx_addr[2];
    packet[5] = ((state - FRSKY_BIND) % 10) * 5;
    packet[6] = packet[5] * 5 + 6;
    packet[7] = packet[5] * 5 + 11;
    packet[8] = packet[5] * 5 + 16;
    packet[9] = packet[5] * 5 + 21;
    packet[10] = packet[5] * 5 + 26;
    packet[11] = 0x00;
    packet[12] = 0x00;
    packet[13] = 0x00;
    packet[14] = FRSKY1_crc8(0x93, packet, 14, 0x07);
}

static uint8_t __attribute__((unused)) FRSKY1_calc_channel()
{
	uint32_t temp=seed;
	temp = (temp * 0xaa) % 0x7673;
	seed = temp;
	return (seed & 0xff) % 0x32;
}

static void __attribute__((unused)) FRSKY1_build_data_packet()
{
	packet[0] = 0x0e;
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = seed & 0xff;
	packet[4] = seed >> 8;
	if (state == FRSKY_DATA1 || state == FRSKY_DATA3)
		packet[5] = 0x0f;
	else
		if(state == FRSKY_DATA2 || state == FRSKY_DATA4)
			packet[5] = 0xf0;
		else
			packet[5] = 0x00;
	uint8_t idx = 0; //= (state == FRSKY_DATA1) ? 4 : 0;
	for(uint8_t i = 0; i < 4; i++)
	{
		uint16_t value = convert_channel_frsky(i+idx);
		packet[2*i + 6] = value & 0xff;
		packet[2*i + 7] = value >> 8;
	}
	packet[14] = FRSKY1_crc8(crc8, packet, 14, 0x07);
}

static uint16_t ReadFRSKY1()
{
	if (state < FRSKY_BIND_DONE)
	{
		FRSKY1_build_bind_packet();
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
		CC2500_WriteData(packet, packet[0]+1);
		state++;
		return 53460;
	}
	if (state == FRSKY_BIND_DONE)
	{
		state++;
		BIND_DONE;
	}
	if (state >= FRSKY_DATA1)
	{
		CC2500_Strobe(CC2500_SIDLE);
		if (option != prev_option)
		{
			CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
			prev_option=option;
		}
		uint8_t chan = FRSKY1_calc_channel();
		CC2500_WriteReg(CC2500_0A_CHANNR, chan * 5 + 6);
		FRSKY1_build_data_packet();

		CC2500_WriteData(packet, packet[0]+1);
		if (state == FRSKY_DATA5)
		{
			CC2500_SetPower();
			state = FRSKY_DATA1;
		}
		else
			state++;
		return 9006;
	}
	return 0;
}

uint16_t initFRSKY1()
{
	//u8 data[2] = {(fixed_id >> 8) & 0xff, fixed_id & 0xff};
	crc8 = FRSKY1_crc8_le(0x6b, rx_tx_addr+2, 2); // Use rx_tx_addr[2] and rx_tx_addr[3] since we want to use RX_Num
	FRSKY1_init();
	seed = 1;
	if(IS_AUTOBIND_FLAG_on)
		state = FRSKY_BIND;
	else
		state = FRSKY_DATA1;
	return 10000;
}

#endif
