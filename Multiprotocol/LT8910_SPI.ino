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

// LT8910/LT8900 SPI driver for external module via PA15 (SPI_CSN)
// SPI convention: bit7=0 WRITE, bit7=1 READ
// Each register is 16 bits wide, transferred MSB first

#ifdef LT8910_INSTALLED

#include "iface_lt8910.h"

void LT8910_WriteReg(uint8_t address, uint16_t data)
{
	SPI_CSN_off;
	SPI_Write(address & 0x7F);		// bit7=0 for write
	while(SPI2_BASE->SR & SPI_SR_BSY);	// Wait for byte fully clocked out
	SPI_Write(data >> 8);			// MSB first
	while(SPI2_BASE->SR & SPI_SR_BSY);
	SPI_Write(data & 0xFF);		// LSB
	while(SPI2_BASE->SR & SPI_SR_BSY);	// Ensure last byte done before CS rise
	SPI_CSN_on;
}

uint16_t LT8910_ReadReg(uint8_t address)
{
	uint16_t result;
	SPI_CSN_off;
	SPI_Transfer(address | 0x80);		// bit7=1 for read
	uint8_t msb = SPI_Transfer(0xFF);	// Stock TX clocks 0xFF during reads
	uint8_t lsb = SPI_Transfer(0xFF);
	while(SPI2_BASE->SR & SPI_SR_BSY);	// Ensure last byte done before CS rise
	SPI_CSN_on;
	result = ((uint16_t)msb << 8) | lsb;
	return result;
}

void LT8910_WriteFIFO(const uint8_t *data, uint8_t length)
{
	// Stock TX (02b) has ~228µs between each SPI transaction (slow 8-bit MCU).
	// At /256 SPI clock (~140kHz), each LT8910_WriteReg() takes ~196µs on the
	// wire, naturally providing the inter-write timing the LT8910 needs without
	// explicit delays.  Back-to-back calls produce ~200µs start-to-start spacing
	// that closely matches the stock TX's 228µs gap.

	// Clear the FIFO
	LT8910_WriteReg(LT8910_REG_FIFO, 0x0000);
	// Set FIFO control
	LT8910_WriteReg(LT8910_REG_FIFO_CTRL, 0x8080);
	// Write data as 16-bit words, each in a separate SPI transaction
	// (matches stock TX behavior from captures)
	for(uint8_t i = 0; i < length; i += 2)
	{
		uint16_t word = (uint16_t)data[i] << 8;
		if(i + 1 < length)
			word |= data[i + 1];
		LT8910_WriteReg(LT8910_REG_FIFO, word);
	}
}

void LT8910_SetChannel(uint8_t channel)
{
	LT8910_WriteReg(LT8910_REG_CHANNEL, (uint16_t)channel);
}

void LT8910_SetTxOn(uint8_t channel)
{
	LT8910_WriteReg(LT8910_REG_CHANNEL, LT8910_TX_EN | (uint16_t)channel);
}

uint8_t LT8910_Reset()
{
	// Pull CSN high to deselect
	SPI_CSN_on;
	delayMicroseconds(200);
	return 0;
}

bool LT8910_DetectChip()
{
	uint16_t chip_id = LT8910_ReadReg(LT8910_REG_CHIP_ID);
	if(chip_id == LT8910_CHIP_ID || chip_id == LT8900_CHIP_ID)
	{
		debugln("LT8910 chip ID=0x%04X", chip_id);
		return true;
	}
	debugln("LT8910 not detected! ID=0x%04X", chip_id);
	return false;
}

#endif
