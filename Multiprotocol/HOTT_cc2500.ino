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

#if defined(HOTT_CC2500_INO)

#include "iface_cc2500.h"

#define HOTT_COARSE	0
#define HOTT_TX_PACKET_LEN	50
#define HOTT_RX_PACKET_LEN	22
#define HOTT_PACKET_PERIOD	10000

enum {
    HOTT_START = 0x00,
    HOTT_CAL   = 0x01,
    HOTT_DATA1 = 0x02,
    HOTT_RX1   = 0x03,
    HOTT_RX2   = 0x04,
};

#define HOTT_FREQ0_VAL 0x6E

// Some important initialization parameters, all others are either default,
// or not important in the context of transmitter
// FIFOTHR  00
// SYNC1    D3
// SYNC0    91
// PKTLEN   32 - Packet length, 50 bytes
// PKTCTRL1 04 - APPEND_STATUS on=RSSI+LQI, all other are receive parameters - irrelevant
// PKTCTRL0 44 - whitening, use FIFO, use CRC, fixed packet length
// ADDR     00
// CHANNR   10
// FSCTRL1  09 - IF 
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequencyfor 26MHz crystal
// FREQ1    6C
// FREQ0    B9
// MDMCFG4  2D - 
// MDMCFG3  3B - 
// MDMCFG2  73 - disable DC blocking, MSK, no Manchester code, 32 bits sync word
// MDMCFG1  A3 - FEC enable, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  AA - CHANSPC_M - AA
// DEVIATN  47 - 
// MCSM2    07 - 
// MCSM1    00 - always use CCA, go to IDLE when done
// MCSM0    08 - disable autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D
const PROGMEM uint8_t HOTT_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x00, 0xD3, 0x91, 0x32, 0x04,
  /* 08 */ 0x44, 0x00, 0x00, 0x09, 0x00, 0x5C, 0x6C, HOTT_FREQ0_VAL + HOTT_COARSE,
  /* 10 */ 0x2D, 0x3B, 0x73, 0xA3, 0xAA, 0x47, 0x07, 0x00, //original 0x17=0
  /* 18 */ 0x08, 0x1D, 0x1C, 0xC7, 0x09, 0xF0, 0x87, 0x6B,
  /* 20 */ 0xF0, 0xB6, 0x10, 0xEA, 0x0A, 0x00, 0x11
};

static void __attribute__((unused)) HOTT_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&HOTT_init_values[i]));

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

static void __attribute__((unused)) HOTT_tune_chan()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, (rf_ch_num+1)*3);
	CC2500_Strobe(CC2500_SCAL);
}

static void __attribute__((unused)) HOTT_tune_chan_fast()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, (rf_ch_num+1)*3);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[rf_ch_num]);
}

static void __attribute__((unused)) HOTT_tune_freq()
{
	if ( prev_option != option )
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		CC2500_WriteReg(CC2500_0F_FREQ0, HOTT_FREQ0_VAL + HOTT_COARSE);
		prev_option = option ;
		phase = HOTT_START;	// Restart the tune process if option is changed to get good tuned values
	}
}

uint8_t HOTT_hop[75]= { 48, 37, 16, 62, 9, 50, 42, 22, 68, 0, 55, 35, 21, 74, 1, 56, 31, 20, 70, 11, 45, 32, 24, 71, 8, 54, 38, 26, 61, 13, 53, 30, 15, 65, 7, 52, 34, 28, 60, 3, 47, 39, 18, 69, 2, 49, 44, 23, 72, 5, 51, 43, 19, 64, 12, 46, 33, 17, 67, 6, 58, 36, 29, 73, 14, 57, 41, 25, 63, 4, 59, 40, 27, 66, 10 };
uint16_t HOTT_hop_val = 0xC06B;

// Channel values are PPM*2
static void __attribute__((unused)) HOTT_data_packet()
{
	packet[2] = hopping_frequency_no;
	packet[3] = 0x00;	// unknown, may be for additional channels
	for(uint8_t i=4;i<28;i+=2)
	{
		uint16_t val=Channel_data[(i-4)>>1];
		val=(((val<<2)+val)>>2)+860*2;				//value range 860<->2140 *2 <-> -125%<->+125%
		packet[i] = val;
		packet[i+1] = val>>8;
	}

	packet[28] = 0x8C;	// unknown
	packet[29] = 0x02;	// unknown

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
	CC2500_WriteReg(CC2500_06_PKTLEN, 0x32);
	CC2500_WriteData(packet, HOTT_TX_PACKET_LEN);
	#if 0
		debug("P:");
		for(uint8_t i=0;i<HOTT_TX_PACKET_LEN;i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
	hopping_frequency_no++;
	hopping_frequency_no %= 75;
	rf_ch_num=HOTT_hop[hopping_frequency_no];
}

uint16_t ReadHOTT()
{
	switch(phase)
	{
		case HOTT_START:
			rf_ch_num = 0;
			HOTT_tune_chan();
			phase = HOTT_CAL;
			return 2000;
		case HOTT_CAL:
			calData[rf_ch_num]=CC2500_ReadReg(CC2500_25_FSCAL1);
			if (++rf_ch_num < 75)
				HOTT_tune_chan();
			else
			{
				hopping_frequency_no = 0;
				rf_ch_num=HOTT_hop[hopping_frequency_no];
				counter = 0;
				phase = HOTT_DATA1;
			}
			return 2000;

		/* Work cycle: 10ms */
		case HOTT_DATA1:
			//TX
			telemetry_set_input_sync(10000);
			HOTT_tune_freq();
			HOTT_tune_chan_fast();
			HOTT_data_packet();
			phase = HOTT_RX1;
			return 4500;
		case HOTT_RX1:
			//RX
			CC2500_SetTxRxMode(RX_EN);
			CC2500_WriteReg(CC2500_06_PKTLEN, HOTT_RX_PACKET_LEN);
			CC2500_Strobe(CC2500_SRX);
			phase = HOTT_RX2;
			return 4500;
		case HOTT_RX2:
			//Telemetry
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len==HOTT_RX_PACKET_LEN+2)
			{
				CC2500_ReadData(packet_in, len);
				debug("T:");
				for(uint8_t i=0;i<HOTT_RX_PACKET_LEN;i++)
					debug(" %02X", packet_in[i]);
				debugln("");
			}
			CC2500_Strobe(CC2500_SFRX);								//Flush the RXFIFO
			phase=HOTT_DATA1;
			return 1000;
	}
	return 0;
}

static void __attribute__((unused)) HOTT_init()
{
	packet[0] = HOTT_hop_val;
	packet[1] = HOTT_hop_val>>8;
	memcpy(&packet[30],"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x07\x7C\x94\x00\x0C\x50\x4D\xF2\x00\x00\xB1",20); //unknown
}

uint16_t initHOTT()
{
	BIND_DONE;	// Not a TX bind protocol
	HOTT_init();

	HOTT_rf_init();
	phase = HOTT_START;
	return 10000;
}

#endif