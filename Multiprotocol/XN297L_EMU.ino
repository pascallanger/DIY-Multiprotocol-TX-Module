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
#include "iface_xn297l.h"

#if defined (XN297L_CC2500_EMU)
static void __attribute__((unused)) XN297L_Init()
{
	PE1_off; // antenna RF2
	PE2_on;
	CC2500_Reset();
	CC2500_Strobe(CC2500_SIDLE);

	// Address Config = No address check
	// Base Frequency = 2400
	// CRC Autoflush = false
	// CRC Enable = false
	// Channel Spacing = 333.251953
	// Data Format = Normal mode
	// Data Rate = 249.939
	// Deviation = 126.953125
	// Device Address = 0
	// Manchester Enable = false
	// Modulated = true
	// Modulation Format = GFSK
	// Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
	// RX Filter BW = 203.125000
	// Sync Word Qualifier Mode = No preamble/sync
	// TX Power = 0
	// Whitening = false
	// Fast Frequency Hopping - no PLL auto calibration

	CC2500_WriteReg(CC2500_08_PKTCTRL0,	0x01);   // Packet Automation Control
	CC2500_WriteReg(CC2500_0B_FSCTRL1,	0x0A);   // Frequency Synthesizer Control
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);  // Frequency offset hack 
	CC2500_WriteReg(CC2500_0D_FREQ2,	0x5C);   // Frequency Control Word, High Byte
	CC2500_WriteReg(CC2500_0E_FREQ1,	0x4E);   // Frequency Control Word, Middle Byte
	CC2500_WriteReg(CC2500_0F_FREQ0,	0xC3);   // Frequency Control Word, Low Byte
	CC2500_WriteReg(CC2500_10_MDMCFG4,	0x8D);   // Modem Configuration
	CC2500_WriteReg(CC2500_11_MDMCFG3,	0x3B);   // Modem Configuration
	CC2500_WriteReg(CC2500_12_MDMCFG2,	0x10);   // Modem Configuration
	CC2500_WriteReg(CC2500_13_MDMCFG1,	0x23);   // Modem Configuration
	CC2500_WriteReg(CC2500_14_MDMCFG0,	0xA4);   // Modem Configuration
	CC2500_WriteReg(CC2500_15_DEVIATN,	0x62);   // Modem Deviation Setting
	CC2500_WriteReg(CC2500_18_MCSM0,	0x08);   // Main Radio Control State Machine Configuration
	CC2500_WriteReg(CC2500_19_FOCCFG,	0x1D);   // Frequency Offset Compensation Configuration
	CC2500_WriteReg(CC2500_1A_BSCFG,	0x1C);   // Bit Synchronization Configuration
	CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);   // AGC Control
	CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
	CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xB0);   // AGC Control
	CC2500_WriteReg(CC2500_21_FREND1,	0xB6);   // Front End RX Configuration
	CC2500_WriteReg(CC2500_23_FSCAL3,	0xEA);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_25_FSCAL1,	0x00);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_26_FSCAL0,	0x11);   // Frequency Synthesizer Calibration

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

static void __attribute__((unused)) XN297L_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	xn297_addr_len = len;
	memcpy(xn297_tx_addr, addr, len);
}

static void __attribute__((unused)) XN297L_WritePayload(uint8_t* msg, uint8_t len)
{
	uint8_t buf[32];
	uint8_t last = 0;
	uint8_t i;
	static const uint16_t initial = 0xb5d2;

	// address
	for (i = 0; i < xn297_addr_len; ++i) {
		buf[last++] = xn297_tx_addr[xn297_addr_len - i - 1] ^ xn297_scramble[i];
	}

	// payload
	for (i = 0; i < len; ++i) {
		// bit-reverse bytes in packet
		uint8_t b_out = bit_reverse(msg[i]);
		buf[last++] = b_out ^ xn297_scramble[xn297_addr_len + i];
	}

	// crc
	uint16_t crc = initial;
	for (uint8_t i = 0; i < last; ++i)
		crc = crc16_update(crc, buf[i], 8);
	crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
	buf[last++] = crc >> 8;
	buf[last++] = crc & 0xff;

	// stop TX/RX
	CC2500_Strobe(CC2500_SIDLE);
	// flush tx FIFO
	CC2500_Strobe(CC2500_SFTX);
	// packet length
	CC2500_WriteReg(CC2500_3F_TXFIFO, last + 3);
	// xn297L preamble
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (uint8_t*)"\x71\x0f\x55", 3);
	// xn297 packet
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, last);
	// transmit
	CC2500_Strobe(CC2500_STX);
}

static void __attribute__((unused)) XN297L_HoppingCalib(uint8_t num_freq)
{	//calibrate hopping frequencies
	for (uint8_t i = 0; i < num_freq; i++)
	{
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[i]*3);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[i]=CC2500_ReadReg(CC2500_25_FSCAL1);
	}
}

static void __attribute__((unused)) XN297L_Hopping(uint8_t index)
{
	// spacing is 333.25 kHz, must multiply xn297 channel by 3
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[index] * 3);
	// set PLL calibration
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[index]);
}

static void __attribute__((unused)) XN297L_RFChannel(uint8_t number)
{	//change channel
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, number*3);
	CC2500_Strobe(CC2500_SCAL);
	delayMicroseconds(900);
}

static void __attribute__((unused)) XN297L_SetPower()
{
	CC2500_SetPower();
}

static void __attribute__((unused)) XN297L_SetFreqOffset()
{	// Frequency offset
	if (prev_option != option)
	{
		prev_option = option;
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	}
}

#elif defined (NRF24L01_INSTALLED)

static void __attribute__((unused)) XN297L_Init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);		// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);	// Enable data pipe 0 only
	NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
	NRF24L01_SetPower();
}

static void __attribute__((unused)) XN297L_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	XN297_SetTXAddr(addr,len);
}

static void __attribute__((unused)) XN297L_WritePayload(uint8_t* msg, uint8_t len)
{
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(msg, len);
}

static void __attribute__((unused)) XN297L_HoppingCalib(__attribute__((unused)) uint8_t num_freq)
{	//calibrate hopping frequencies
}

static void __attribute__((unused)) XN297L_Hopping(uint8_t index)
{
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[index]);
}

static void __attribute__((unused)) XN297L_RFChannel(uint8_t number)
{	//change channel
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, number);
}

static void __attribute__((unused)) XN297L_SetPower()
{
	NRF24L01_SetPower();
}

static void __attribute__((unused)) XN297L_SetFreqOffset()
{	// Frequency offset
}

#endif