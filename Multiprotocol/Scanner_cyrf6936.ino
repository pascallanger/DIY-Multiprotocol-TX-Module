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

#if defined(SCANNER_CYRF6936_INO)

// Ported from DeviationTX frequency scanner

#include "iface_cyrf6936.h"

struct Scanner {
	uint8_t rssi[255];
	uint8_t chan_min;
	uint8_t chan_max;
	uint8_t attenuator;
	uint16_t averaging;
} Scanner;

#define MIN_RADIOCHANNEL    0x00
#define MAX_RADIOCHANNEL    0x62
#define CHANNEL_LOCK_TIME   300  // slow channel requires 270 usec for synthesizer to settle
#define AVERAGE_INTVL       30

static int scan_averages, scan_channel, scan_state;

enum ScanStates {
	SCAN_CHANNEL_CHANGE = 0,
	SCAN_GET_RSSI = 1,
};

static void __attribute__((unused)) Scanner_cyrf_init()
{
	/* Initialize CYRF chip */
	CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x38);   // FRC SEN (forces the synthesizer to start) + FRC AWAKE (force the oscillator to keep running at all times)
	CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | 7);      // Data Code Length = 32 chip codes + Data Mode = 8DR Mode + max-power(+4 dBm)
	CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);          // LNA + FAST TURN EN + RXOW EN, enable low noise amplifier, fast turning, overwrite enable
	CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);        // Reset power control
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xA4);     // SOP EN + SOP LEN = 32 chips + LEN EN + SOP TH = 04h
	CYRF_WriteRegister(CYRF_11_DATA32_THOLD, 0x05);    // TH32 = 0x05
	CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0E);    // TH64 = 0Eh, set pn correlation threshold
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);   // STRIM LSB = 0x55, typical configuration
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);   // STRIM MSB = 0x05, typical configuration
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);   // AUTO_CAL_TIME = 3Ch, typical configuration
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);  // AUTO_CAL_OFFSET = 14h, typical configuration
	CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);     // ALL SLOW
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);     // FRC RXDR (Force Receive Data Rate)
	CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);     // Reset TX overrides
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);       // TX Length = 16 byte packet
	CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);    // RXF, force receive clock
	CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);          // RXF, force receive clock enable
}

static void __attribute__((unused)) _scan_next()
{
	CYRF_ConfigRFChannel(scan_channel + Scanner.chan_min);
	switch (Scanner.attenuator) {
	case 0: CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A); break;  // LNA on, ATT off
	case 1: CYRF_WriteRegister(CYRF_06_RX_CFG, 0x0A); break;  // LNA off, ATT off
	default:  CYRF_WriteRegister(CYRF_06_RX_CFG, 0x2A); break;  // LNA off, no ATT on
	}
}

static int __attribute__((unused)) _scan_rssi()
{
	if (!(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80)) {
		CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);  // Prepare to receive
		delayMicroseconds(1);
		CYRF_ReadRegister(CYRF_13_RSSI);  // dummy read
		delayMicroseconds(1);
	}
	return CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
}

uint16 Scanner_callback()
{
	int rssi_value;
	switch (scan_state) {
	case SCAN_CHANNEL_CHANGE:
		scan_averages = 0;
		scan_channel++;
		if (scan_channel == (Scanner.chan_max - Scanner.chan_min + 1))
			scan_channel = 0;
		_scan_next();
		scan_state = SCAN_GET_RSSI;
		return CHANNEL_LOCK_TIME;
	case SCAN_GET_RSSI:
		rssi_value = _scan_rssi();
		Scanner.rssi[scan_channel] = (rssi_value + 9 * Scanner.rssi[scan_channel]) / 10;  // fast exponential smoothing with alpha 0.1
		scan_averages++;
		if (scan_averages < Scanner.averaging)
			return AVERAGE_INTVL + random(0xfefefefe) % 10;  // make measurements slightly random in time
		scan_state = SCAN_CHANNEL_CHANGE;

		// send data to TX
		pkt[0] = scan_channel;  // channel
		pkt[1] = Scanner.rssi[scan_channel];  // power
		telemetry_link = 1;

		return AVERAGE_INTVL;
	}
}

uint16_t initScanner(void)
{
	Scanner.chan_min = MIN_RADIOCHANNEL;
	Scanner.chan_max = MAX_RADIOCHANNEL;

	// todo: find optimal values or use user options
	Scanner.averaging = 2;
	Scanner.attenuator = 1;

	scan_averages = 0;
	scan_channel = 0;
	scan_state = SCAN_CHANNEL_CHANGE;
	memset(Scanner.rssi, 0, sizeof(Scanner.rssi));  // clear old rssi values
	CYRF_Reset();
	Scanner_cyrf_init();
	init_frskyd_link_telemetry(); // use FrSkyD HUB to send scanner data
	//telemetry_lost = 1;
	CYRF_SetTxRxMode(RX_EN);  // Receive mode
	BIND_DONE;
	return 1250;
}

#endif
