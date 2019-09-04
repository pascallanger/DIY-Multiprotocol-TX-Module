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

#if defined(SCANNER_CC2500_INO)

// Ported from DeviationTX frequency scanner

#include "iface_cc2500.h"

struct Scanner {
	uint8_t rssi[255];
	uint8_t chan_min;
	uint8_t chan_max;
	uint16_t averaging;
} Scanner;

#define MIN_RADIOCHANNEL    0x00
#define MAX_RADIOCHANNEL    255 //0x62
#define CHANNEL_LOCK_TIME   300  // slow scan_channel requires 270 usec for synthesizer to settle
#define INTERNAL_AVERAGE    1
#define AVERAGE_INTVL       50

static int scan_averages, scan_channel, scan_state;
static uint32_t rssi_sum;
static uint8_t calibration[MAX_RADIOCHANNEL];
static uint8_t calibration_fscal2, calibration_fscal3;

enum ScanStates {
	SCAN_CHANNEL_CHANGE = 0,
	SCAN_GET_RSSI = 1,
};

static void __attribute__((unused)) Scanner_cc2500_init()
{
	/* Initialize CC2500 chip */
	/*CC2500_WriteReg(0x30, 0x3d); // soft reset
	CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x00);  // packet automation control
	CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x00);  // packet automation control
	CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);  // scan_channel number
	CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x08);  // frequency synthesizer control
	CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00);  // frequency synthesizer control
	CC2500_WriteReg(CC2500_0D_FREQ2, 0x5C);  // frequency control word, high byte
	CC2500_WriteReg(CC2500_0E_FREQ1, 0x4E);  // frequency control word, middle byte
	CC2500_WriteReg(CC2500_0F_FREQ0, 0xDE);  // frequency control word, low byte
	CC2500_WriteReg(CC2500_10_MDMCFG4, 0x86);  // modem configuration
	CC2500_WriteReg(CC2500_11_MDMCFG3, 0x83);  // modem configuration
	CC2500_WriteReg(CC2500_12_MDMCFG2, 0x00);  // modem configuration FSK for better sensitivity
	CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);  // modem configuration
	CC2500_WriteReg(CC2500_14_MDMCFG0, 0xA4);  // modem configuration
	CC2500_WriteReg(CC2500_15_DEVIATN, 0x44);  // modem deviation setting 38.085938
	CC2500_WriteReg(CC2500_17_MCSM1, 0x0F);  // always stay in RX mode
	CC2500_WriteReg(CC2500_18_MCSM0, 0x08);  // disable auto-calibration
	CC2500_WriteReg(CC2500_19_FOCCFG, 0x16);  // frequency offset compensation configuration
	CC2500_WriteReg(CC2500_1A_BSCFG, 0x6C);  // bit synchronization configuration
	CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x03);  // agc control
	CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x40);  // agc control
	CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0x91);  // agc control
	CC2500_WriteReg(CC2500_21_FREND1, 0x56);  // front end rx configuration
	CC2500_WriteReg(CC2500_22_FREND0, 0x10);  // front end tx configuration
	CC2500_WriteReg(CC2500_23_FSCAL3, 0xA9);  // frequency synthesizer calibration
	CC2500_WriteReg(CC2500_24_FSCAL2, 0x0A);  // frequency synthesizer calibration
	CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);  // frequency synthesizer calibration
	CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);  // frequency synthesizer calibration
	CC2500_WriteReg(CC2500_2C_TEST2, 0x88);  // various test settings
	CC2500_WriteReg(CC2500_2D_TEST1, 0x31);  // various test settings
	CC2500_WriteReg(CC2500_2E_TEST0, 0x0B);  // various test settings
	CC2500_WriteReg(CC2500_3E_PATABLE, 0xfe);
	CC2500_SetTxRxMode(RX_EN);  // Receive mode
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_Strobe(CC2500_SRX);*/

	CC2500_WriteReg(0x30, 0x3D);   // software reset
	CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x0F);   // Frequency Synthesizer Control (0x0F)
	CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x12);   // Packet Automation Control (0x12)
	CC2500_WriteReg(CC2500_0D_FREQ2, 0x5C);   // Frequency control word, high byte
	CC2500_WriteReg(CC2500_0E_FREQ1, 0x4E);   // Frequency control word, middle byte
	CC2500_WriteReg(CC2500_0F_FREQ0, 0xDE);   // Frequency control word, low byte
	CC2500_WriteReg(CC2500_10_MDMCFG4, 0x0D);   // Modem Configuration
	CC2500_WriteReg(CC2500_11_MDMCFG3, 0x3B);   // Modem Configuration (0x3B)
	CC2500_WriteReg(CC2500_12_MDMCFG2, 0x00);   // Modem Configuration 0x30 - OOK modulation, 0x00 - FSK modulation (better sensitivity)
	CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);   // Modem Configuration
	CC2500_WriteReg(CC2500_14_MDMCFG0, 0xFF);   // Modem Configuration (0xFF)
	CC2500_WriteReg(CC2500_17_MCSM1, 0x0F);   // Always stay in RX mode
	CC2500_WriteReg(CC2500_18_MCSM0, 0x08);   // Main Radio Control State Machine Configuration (0x04)
	CC2500_WriteReg(CC2500_19_FOCCFG, 0x15);   // Frequency Offset Compensation configuration
	CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x83);   // AGC Control (0x83)
	CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
	CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0x91);   // AGC Control
	CC2500_WriteReg(CC2500_23_FSCAL3, 0xEA);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_24_FSCAL2, 0x0A);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);   // Frequency Synthesizer Calibration
	CC2500_SetTxRxMode(RX_EN);
	delayMicroseconds(1000);  // wait for RX to activate
}

static void __attribute__((unused)) _calibrate()
{
	for (int c = 0; c < MAX_RADIOCHANNEL; c++) {
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, c);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calibration[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
	calibration_fscal3 = CC2500_ReadReg(CC2500_23_FSCAL3);  // only needs to be done once
	calibration_fscal2 = CC2500_ReadReg(CC2500_24_FSCAL2);  // only needs to be done once
	CC2500_Strobe(CC2500_SIDLE);
}

static void __attribute__((unused)) _scan_next()
{
	CC2500_WriteReg(CC2500_0A_CHANNR, Scanner.chan_min + scan_channel);

	CC2500_WriteReg(CC2500_23_FSCAL3, calibration_fscal3);
	CC2500_WriteReg(CC2500_24_FSCAL2, calibration_fscal2);
	CC2500_WriteReg(CC2500_25_FSCAL1, calibration[scan_channel]);
	
	//debugln("_scan_next %d", Scanner.chan_min + scan_channel);
}

static int __attribute__((unused)) _scan_rssi()
{
	uint8_t rssi = CC2500_ReadReg(0x40 | CC2500_34_RSSI);  // 0.5 db/count, RSSI value read from the RSSI status register is a 2’s complement number

	

	uint8_t rssi_rel;
	if (rssi >= 128) {
		rssi_rel = rssi - 128;  // relative power levels 0-127 (equals -137 to -72 dBm)
	}
	else {
		rssi_rel = rssi + 128;  // relativ power levels 128-255 (equals -73 to -10 dBm)
	}
	debugln("rssi %d", rssi_rel);
	return rssi_rel;
}

uint16 Scanner_callback()
{
	int rssi_value;
	switch (scan_state) {
	case SCAN_CHANNEL_CHANGE:
		scan_channel++;
		if (scan_channel >= (Scanner.chan_max - Scanner.chan_min + 1))
			scan_channel = 0;
		_scan_next();
		scan_state = SCAN_GET_RSSI;
		return CHANNEL_LOCK_TIME;
	case SCAN_GET_RSSI:
		rssi_value = _scan_rssi();
		scan_state = SCAN_CHANNEL_CHANGE;

		// debugln("%d\t%d", scan_channel, CC2500_ReadReg(CC2500_34_RSSI));
		// send data to TX
		pkt[0] = scan_channel;  // scan_channel
		pkt[1] = rssi_value; // Scanner.rssi[scan_channel];  // power
		telemetry_link = 1;
	}
	return AVERAGE_INTVL;
}

uint16_t initScanner(void)
{
	Scanner.chan_min = 1;//MIN_RADIOCHANNEL;
	Scanner.chan_max = 1;//MAX_RADIOCHANNEL;

	scan_averages = 0;
	scan_channel = 0;
	scan_state = SCAN_CHANNEL_CHANGE;
	memset(Scanner.rssi, 0, sizeof(Scanner.rssi));  // clear old rssi values
	CC2500_Reset();
	Scanner_cc2500_init();
	CC2500_SetPower();
	CC2500_Strobe(CC2500_SRX);
	_calibrate();
	CC2500_Strobe(CC2500_SFSTXON);
	delayMicroseconds(800);
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_SetTxRxMode(RX_EN);
	CC2500_Strobe(CC2500_SRX);  // Receive mode
	BIND_DONE;
	return 1250;
}

#endif
