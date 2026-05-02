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

// LT8910/LT8900 register definitions
// SPI convention: bit7=0 WRITE, bit7=1 READ

#ifndef _IFACE_LT8910_H_
#define _IFACE_LT8910_H_

// Register addresses
#define LT8910_REG_CHIP_ID			0x00
#define LT8910_REG_01				0x01
#define LT8910_REG_02				0x02
#define LT8910_REG_RSSI			0x04
#define LT8910_REG_05				0x05
#define LT8910_REG_CHANNEL			0x07	// bits[6:0]=channel, bit[8]=TX enable
#define LT8910_REG_08				0x08
#define LT8910_REG_CURRENT			0x09
#define LT8910_REG_0A				0x0A
#define LT8910_REG_0B				0x0B
#define LT8910_REG_0C				0x0C
#define LT8910_REG_0D				0x0D
#define LT8910_REG_RX_PLL			0x16
#define LT8910_REG_CRC_POLY		0x17
#define LT8910_REG_PREAMBLE		0x18
#define LT8910_REG_19				0x19
#define LT8910_REG_1A				0x1A
#define LT8910_REG_1B				0x1B
#define LT8910_REG_1C				0x1C
#define LT8910_REG_SYNCWORD1		0x20
#define LT8910_REG_SYNCWORD2		0x21
#define LT8910_REG_SYNCWORD3		0x22
#define LT8910_REG_PACKET_CONFIG	0x23	// bit9=CRC_INITIAL_DATA_EN, bit8=CRC_ON
#define LT8910_REG_SYNCWORD4		0x24
#define LT8910_REG_SYNCWORD5		0x25
#define LT8910_REG_SYNCWORD6		0x26
#define LT8910_REG_SYNCWORD7		0x27
#define LT8910_REG_CRC_SEED		0x28
#define LT8910_REG_TX_POWER		0x29
#define LT8910_REG_2A				0x2A
#define LT8910_REG_2B				0x2B
#define LT8910_REG_FIFO			0x32
#define LT8910_REG_FIFO_CTRL		0x34

// Channel register bit masks
#define LT8910_TX_EN				0x0100	// bit 8 in channel register
#define LT8910_CHIP_RST			0x8000	// bit 15: software reset (auto-clear)

// Chip IDs
#define LT8900_CHIP_ID				0x6FE0
#define LT8910_CHIP_ID				0x6FF0

#endif
