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

#if defined(V2X2_NRF24L01_INO) || defined(V2X2_RX_NRF24L01_INO)

enum {
	V2X2_FLAG_CAMERA = 0x01, // also automatic Missile Launcher and Hoist in one direction
	V2X2_FLAG_VIDEO = 0x02, // also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir.
	V2X2_FLAG_FLIP = 0x04,
	V2X2_FLAG_UNK9 = 0x08,
	V2X2_FLAG_LIGHT = 0x10,
	V2X2_FLAG_UNK10 = 0x20,
	V2X2_FLAG_BIND = 0xC0,
	// flags going to byte 10
	V2X2_FLAG_HEADLESS = 0x02,
	V2X2_FLAG_MAG_CAL_X = 0x08,
	V2X2_FLAG_MAG_CAL_Y = 0x20,
	V2X2_FLAG_EMERGENCY = 0x80,	// JXD-506
	// flags going to byte 11 (JXD-506)
	V2X2_FLAG_START_STOP = 0x40,
	V2X2_FLAG_CAMERA_UP = 0x01,
	V2X2_FLAG_CAMERA_DN = 0x02,
};

// This is frequency hopping table for V202 protocol
// The table is the first 4 rows of 32 frequency hopping
// patterns, all other rows are derived from the first 4.
// For some reason the protocol avoids channels, dividing
// by 16 and replaces them by subtracting 3 from the channel
// number in this case.
// The pattern is defined by 5 least significant bits of
// sum of 3 bytes comprising TX id
const uint8_t PROGMEM v2x2_freq_hopping[][16] = {
	{ 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
		0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
	{ 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
		0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
	{ 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
		0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
	{ 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
		0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

static void V2X2_set_tx_id(void)
{
	uint8_t sum;
	sum = rx_tx_addr[1] + rx_tx_addr[2] + rx_tx_addr[3];
	// Higher 3 bits define increment to corresponding row
	uint8_t increment = (sum & 0x1e) >> 2;
	// Base row is defined by lowest 2 bits
	sum &= 0x03;
	for (uint8_t i = 0; i < 16; ++i) {
		uint8_t val = pgm_read_byte_near(&v2x2_freq_hopping[sum][i]) + increment;
		// Strange avoidance of channels divisible by 16
		hopping_frequency[i] = (val & 0x0f) ? val : val - 3;
	}
}

#endif
