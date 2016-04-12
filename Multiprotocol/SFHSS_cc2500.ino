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
// Last sync with main deviation/sfhss_cc2500.c dated 2016-03-23

#if defined(SFHSS_CC2500_INO)

#include "iface_cc2500.h"

//#define SFHSS_USE_TUNE_FREQ
#define SFHSS_COARSE	0

#define SFHSS_PACKET_LEN 13
#define SFHSS_TX_ID_LEN   2

uint8_t	fhss_code; // 0-27

enum {
    SFHSS_START = 0x101,
    SFHSS_CAL   = 0x102,
    SFHSS_TUNE  = 0x103,
    SFHSS_DATA1 = 0x02,
    SFHSS_DATA2 = 0x0b
};

#define SFHSS_FREQ0_VAL 0xC4

// Some important initialization parameters, all others are either default,
// or not important in the context of transmitter
// IOCFG2   2F - GDO2_INV=0 GDO2_CFG=2F - HW0
// IOCFG1   2E - GDO1_INV=0 GDO1_CFG=2E - High Impedance
// IOCFG0   2F - GDO0 same as GDO2, TEMP_SENSOR_ENABLE=off
// FIFOTHR  07 - 33 decimal TX threshold
// SYNC1    D3
// SYNC0    91
// PKTLEN   0D - Packet length, 0D bytes
// PKTCTRL1 04 - APPEND_STATUS on, all other are receive parameters - irrelevant
// PKTCTRL0 0C - No whitening, use FIFO, CC2400 compatibility on, use CRC, fixed packet length
// ADDR     29
// CHANNR   10
// FSCTRL1  06 - IF 152343.75Hz, see page 65
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequency 2399999633Hz for 26MHz crystal, ibid
// FREQ1    4E
// FREQ0    C4
// MDMCFG4  7C - CHANBW_E - 01, CHANBW_M - 03, DRATE_E - 0C. Filter bandwidth = 232142Hz
// MDMCFG3  43 - DRATE_M - 43. Data rate = 128143bps
// MDMCFG2  83 - disable DC blocking, 2-FSK, no Manchester code, 15/16 sync bits detected (irrelevant for TX)
// MDMCFG1  23 - no FEC, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  3B - CHANSPC_M - 3B. Channel spacing = 249938Hz (each 6th channel used, resulting in spacing of 1499628Hz)
// DEVIATN  44 - DEVIATION_E - 04, DEVIATION_M - 04. Deviation = 38085.9Hz
// MCSM2    07 - receive parameters, default, irrelevant
// MCSM1    0C - no CCA (transmit always), when packet received stay in RX, when sent go to IDLE
// MCSM0    08 - no autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D - not interesting, Frequency Offset Compensation
// FREND0   10 - PA_POWER = 0
const PROGMEM uint8_t SFHSS_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, 0x0D, 0x04,
  /* 08 */ 0x0C, 0x29, 0x10, 0x06, 0x00, 0x5C, 0x4E, SFHSS_FREQ0_VAL + SFHSS_COARSE,
  /* 10 */ 0x7C, 0x43, 0x83, 0x23, 0x3B, 0x44, 0x07, 0x0C,
  /* 18 */ 0x08, 0x1D, 0x1C, 0x43, 0x40, 0x91, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x11, 0x11
};

static void __attribute__((unused)) SFHSS_rf_init()
{
	CC2500_Reset();
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&SFHSS_init_values[i]));
	//CC2500_WriteRegisterMulti(CC2500_00_IOCFG2, init_values, sizeof(init_values));
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

static void __attribute__((unused)) SFHSS_tune_chan()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, rf_ch_num*6+16);
	CC2500_Strobe(CC2500_SCAL);
}

static void __attribute__((unused)) SFHSS_tune_chan_fast()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, rf_ch_num*6+16);
	CC2500_WriteRegisterMulti(CC2500_23_FSCAL3, calData[rf_ch_num], 3);
	_delay_us(6);
}

#ifdef SFHSS_USE_TUNE_FREQ
static void __attribute__((unused)) SFHSS_tune_freq() {
// May be we'll need this tuning routine - some receivers are more sensitive to
// frequency impreciseness, and though CC2500 has a procedure to handle it it
// may not be applied in receivers, so we need to compensate for it on TX 
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	CC2500_WriteReg(CC2500_0F_FREQ0, SFHSS_FREQ0_VAL + SFHSS_COARSE);
}
#endif

static void __attribute__((unused)) SFHSS_calc_next_chan()
{
    rf_ch_num += fhss_code + 2;
    if (rf_ch_num > 29)
	{
        if (rf_ch_num < 31)
			rf_ch_num += fhss_code + 2;
        rf_ch_num -= 31;
    }
}

// Channel values are 10-bit values between 86 and 906, 496 is the middle.
static uint16_t __attribute__((unused)) SFHSS_convert_channel(uint8_t num)
{
	return (uint16_t) (map(limit_channel_100(num),PPM_MIN_100,PPM_MAX_100,86,906));
}


static void __attribute__((unused)) SFHSS_build_data_packet()
{
#define spacer1 0x02 //0b10
#define spacer2 (spacer1 << 4)
    uint8_t ch_offset = state == SFHSS_DATA1 ? 0 : 4;
	const uint8_t ch[]={AILERON, ELEVATOR, THROTTLE, RUDDER, AUX1, AUX2, AUX3, AUX4};

    uint16_t ch1 = SFHSS_convert_channel(ch[ch_offset+0]);
    uint16_t ch2 = SFHSS_convert_channel(ch[ch_offset+1]);
    uint16_t ch3 = SFHSS_convert_channel(ch[ch_offset+2]);
    uint16_t ch4 = SFHSS_convert_channel(ch[ch_offset+3]);
    
    packet[0]  = 0x81; // can be 80, 81, 81 for Orange, only 81 for XK
    packet[1]  = rx_tx_addr[0];
    packet[2]  = rx_tx_addr[1];
    packet[3]  = 0;
    packet[4]  = 0;
    packet[5]  = (rf_ch_num << 3) | spacer1 | ((ch1 >> 9) & 0x01);
    packet[6]  = (ch1 >> 1);
    packet[7]  = (ch1 << 7) | spacer2 | ((ch2 >> 5) & 0x1F /*0b11111*/);
    packet[8]  = (ch2 << 3) | spacer1  | ((ch3 >> 9) & 0x01);
    packet[9]  = (ch3 >> 1);
    packet[10] = (ch3 << 7) | spacer2  | ((ch4 >> 5) & 0x1F /*0b11111*/);
    packet[11] = (ch4 << 3) | ((fhss_code >> 2) & 0x07 /*0b111 */);
    packet[12] = (fhss_code << 6) | state;
}

static void __attribute__((unused)) SFHSS_send_packet()
{
    SFHSS_tune_chan_fast();
    CC2500_WriteData(packet, SFHSS_PACKET_LEN);
}

uint16_t ReadSFHSS()
{
	switch(state)
	{
		case SFHSS_START:
			rf_ch_num = 0;
			SFHSS_tune_chan();
			state = SFHSS_CAL;
			return 2000;
		case SFHSS_CAL:
			CC2500_ReadRegisterMulti(CC2500_23_FSCAL3, calData[rf_ch_num], 3);
			if (++rf_ch_num < 30)
				SFHSS_tune_chan();
			else
			{
				rf_ch_num = 0;
				state = SFHSS_DATA1;
			}
			return 2000;

		/* Work cycle, 6.8ms, second packet 1.65ms after first */
		case SFHSS_DATA1:
			SFHSS_build_data_packet();
			SFHSS_send_packet();
			state = SFHSS_DATA2;
			return 1650;
		case SFHSS_DATA2:
			SFHSS_build_data_packet();
			SFHSS_send_packet();
			SFHSS_calc_next_chan();
			state = SFHSS_TUNE;
			return 2000;
		case SFHSS_TUNE:
			CC2500_SetPower();
			state = SFHSS_DATA1;
			return 3150;
	/*
		case SFHSS_DATA1:
			SFHSS_build_data_packet();
			SFHSS_send_packet();
			state = SFHSS_DATA2;
			return 1650;
		case SFHSS_DATA2:
			SFHSS_build_data_packet();
			SFHSS_send_packet();
			state = SFHSS_CAL2;
			return 500;
		case SFHSS_CAL2:
			SFHSS_tune_freq();
			//        CC2500_SetPower();
			SFHSS_calc_next_chan();
			SFHSS_tune_chan();
			state = SFHSS_DATA1;
			return 4650;
	*/
	}
	return 0;
}

// Generate internal id
static void __attribute__((unused)) SFHSS_get_tx_id()
{
	uint32_t fixed_id;
	// Some receivers (Orange) behaves better if they tuned to id that has
	//  no more than 6 consecutive zeros and ones
	uint8_t run_count = 0;
	// add guard for bit count
	fixed_id = 1 ^ (MProtocol_id & 1);
	for (uint8_t i = 0; i < 16; ++i)
	{
		fixed_id = (fixed_id << 1) | (MProtocol_id & 1);
		MProtocol_id >>= 1;
		// If two LS bits are the same
		if ((fixed_id & 3) == 0 || (fixed_id & 3) == 3)
		{
			if (++run_count > 6)
			{
				fixed_id ^= 1;
				run_count = 0;
			}
		}
		else
			run_count = 0;
	}
	//    fixed_id = 0xBC11;
	rx_tx_addr[0] = fixed_id >> 8;
	rx_tx_addr[1] = fixed_id;
}

uint16_t initSFHSS()
{
	BIND_DONE;	// No bind protocol
	SFHSS_get_tx_id();

	fhss_code=rx_tx_addr[2]%28; // Initialize it to random 0-27 inclusive

	SFHSS_rf_init();
	state = SFHSS_START;

	return 10000;
}

#endif