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

#if defined(SKYARTEC_CC2500_INO)

#include "iface_cc2500.h"

//#define SKYARTEC_FORCE_ID

#define SKYARTEC_COARSE				0x00
#define SKYARTEC_TX_ADDR			rx_tx_addr[1]
#define SKYARTEC_TX_CHANNEL			rx_tx_addr[0]

enum {
    SKYARTEC_PKT1 = 0,
    SKYARTEC_SLEEP1, 
    SKYARTEC_PKT2,
    SKYARTEC_SLEEP2, 
    SKYARTEC_PKT3,
    SKYARTEC_SLEEP3, 
    SKYARTEC_PKT4,
    SKYARTEC_SLEEP4, 
    SKYARTEC_PKT5,
    SKYARTEC_SLEEP5, 
    SKYARTEC_PKT6,
    SKYARTEC_LAST,
};

const PROGMEM uint8_t SKYARTEC_init_values[] = {
  /* 04 */ 0x13, 0x18, 0xFF, 0x05,
  /* 08 */ 0x05, 0x43, 0xCD, 0x09, 0x00, 0x5D, 0x93, 0xB1 + SKYARTEC_COARSE,
  /* 10 */ 0x2D, 0x20, 0x73, 0x22, 0xF8, 0x50, 0x07, 0x30,
  /* 18 */ 0x18, 0x1D, 0x1C, 0xC7, 0x00, 0xB2, 0x87, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x00, 0x11, 0x41,
  /* 28 */ 0x00, 0x59, 0x7F, 0x3F, 0x88, 0x31, 0x0B
};

static void __attribute__((unused)) SKYARTEC_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 4; i <= 0x2E; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&SKYARTEC_init_values[i-4]));
	
	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
	CC2500_Strobe(CC2500_SFTX);
	CC2500_Strobe(CC2500_SFRX);
	CC2500_Strobe(CC2500_SXOFF);
	CC2500_Strobe(CC2500_SIDLE);
}

static void __attribute__((unused)) SKYARTEC_send_data_packet()
{
	//13 c5 01 0259 0168 0000 0259 030c 021a 0489 f3 7e 0a
	//header
	packet[0] = 0x13;				//Length
	packet[1] = SKYARTEC_TX_ADDR;	//Tx Addr?
	packet[2] = 0x01;				//???
	//channels
	for(uint8_t i = 0; i < 7; i++)
	{
		uint16_t value = convert_channel_16b_limit(CH_AETR[i],0x000,0x500);
		packet[3+2*i] = value >> 8;
		packet[4+2*i] = value & 0xff;
	}
	//checks
    uint8_t xor1 = 0;
    for(uint8_t i = 3; i <= 14; i++)
        xor1 ^= packet[i];
    packet[18] = xor1;
    xor1 ^= packet[15];
    xor1 ^= packet[16];
    packet[17] = xor1;
    packet[19] = packet[3] + packet[5] + packet[7] + packet[9] + packet[11] + packet[13];

	CC2500_WriteReg(CC2500_04_SYNC1,	rx_tx_addr[3]);
	CC2500_WriteReg(CC2500_05_SYNC0,	rx_tx_addr[2]);
	CC2500_WriteReg(CC2500_09_ADDR,		SKYARTEC_TX_ADDR);
	CC2500_WriteReg(CC2500_0A_CHANNR,	SKYARTEC_TX_CHANNEL);
	CC2500_WriteData(packet, 20);
}

static void __attribute__((unused)) SKYARTEC_send_bind_packet()
{
	//0b 7d 01 01 b2 c5 4a 2f 00 00 c5 d6
	packet[0] = 0x0b;       //Length
	packet[1] = 0x7d;
	packet[2] = 0x01;
	packet[3] = 0x01;
	packet[4] = rx_tx_addr[0];
	packet[5] = rx_tx_addr[1];
	packet[6] = rx_tx_addr[2];
	packet[7] = rx_tx_addr[3];
	packet[8] = 0x00;
	packet[9] = 0x00;
	packet[10] = SKYARTEC_TX_ADDR;
	uint8_t xor1 = 0;
	for(uint8_t i = 3; i < 11; i++)
		xor1 ^= packet[i];
	packet[11] = xor1;
	CC2500_WriteReg(CC2500_04_SYNC1,	0x7d);
	CC2500_WriteReg(CC2500_05_SYNC0,	0x7d);
	CC2500_WriteReg(CC2500_09_ADDR,		0x7d);
	CC2500_WriteReg(CC2500_0A_CHANNR,	0x7d);
	CC2500_WriteData(packet, 12);
}

uint16_t ReadSKYARTEC()
{
	if (phase & 0x01)
	{
		CC2500_Strobe(CC2500_SIDLE);
		if (phase == SKYARTEC_LAST)
		{
			CC2500_SetPower();
			// Tune frequency if it has been changed
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
				prev_option = option ;
			}
			phase = SKYARTEC_PKT1;
		}
		else
			phase++;
		return 3000;
	}
	if (phase == SKYARTEC_PKT1 && bind_counter)
	{
		SKYARTEC_send_bind_packet();
		bind_counter--;
		if(bind_counter == 0)
			BIND_DONE;
	}
	else
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(6000);
		#endif
		SKYARTEC_send_data_packet();
	}
	phase++;
	return 3000;
}

uint16_t initSKYARTEC()
{
    SKYARTEC_rf_init();

	#ifdef SKYARTEC_FORCE_ID
		memset(rx_tx_addr,0x00,4);
	#endif
	if(rx_tx_addr[0]==0) rx_tx_addr[0]=0xB2;
	if(rx_tx_addr[1]==0) rx_tx_addr[1]=0xC5;
	if(rx_tx_addr[2]==0) rx_tx_addr[2]=0x4A;
	if(rx_tx_addr[3]==0) rx_tx_addr[3]=0x2F;

	bind_counter = 250;
	phase = SKYARTEC_PKT1;
	return 10000;
}

#endif