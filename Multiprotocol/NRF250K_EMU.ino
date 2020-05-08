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
#ifdef NRF24L01_INSTALLED
#include "iface_nrf250k.h"

static void __attribute__((unused)) XN297L_Init()
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		debugln("Using NRF");
		PE1_on;							//NRF24L01 antenna RF3 by default
		PE2_off;						//NRF24L01 antenna RF3 by default
		NRF24L01_Initialize();
		NRF24L01_SetTxRxMode(TX_EN);
		NRF24L01_FlushTx();
		NRF24L01_FlushRx();
		NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);	// Clear data ready, data sent, and retransmit
		NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);		// No Auto Acknowldgement on all data pipes
		NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);	// Enable data pipe 0 only
		NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
		NRF24L01_SetPower();
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		debugln("Using CC2500");
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
		xn297_scramble_enabled=XN297_SCRAMBLED;	//enabled by default
	#endif
}

static void __attribute__((unused)) XN297L_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		XN297_SetTXAddr(addr,len);
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		if (len > 5) len = 5;
		if (len < 3) len = 3;
		xn297_addr_len = len;
		memcpy(xn297_tx_addr, addr, len);
	#endif
}

static void __attribute__((unused)) XN297L_WritePayload(uint8_t* msg, uint8_t len)
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
		NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
		NRF24L01_FlushTx();
		XN297_WritePayload(msg, len);
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		uint8_t buf[32];
		uint8_t last = 0;
		uint8_t i;
		static const uint16_t initial = 0xb5d2;

		// address
		for (i = 0; i < xn297_addr_len; ++i)
		{
			buf[last] = xn297_tx_addr[xn297_addr_len - i - 1];
			if(xn297_scramble_enabled)
				buf[last] ^=  xn297_scramble[i];
			last++;
		}

		// payload
		for (i = 0; i < len; ++i) {
			// bit-reverse bytes in packet
			buf[last] = bit_reverse(msg[i]);
			if(xn297_scramble_enabled)
				buf[last] ^= xn297_scramble[xn297_addr_len+i];
			last++;
		}

		// crc
		uint16_t crc = initial;
		for (uint8_t i = 0; i < last; ++i)
			crc = crc16_update(crc, buf[i], 8);
		if(xn297_scramble_enabled)
			crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
		else
			crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
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
	#endif
}

static void __attribute__((unused)) XN297L_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
		NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
		NRF24L01_FlushTx();
		XN297_WriteEnhancedPayload(msg, len, noack);
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		uint8_t buf[32];
		uint8_t scramble_index=0;
		uint8_t last = 0;
		static uint8_t pid=0;

		// address
		if (xn297_addr_len < 4)
		{
			// If address length (which is defined by receive address length)
			// is less than 4 the TX address can't fit the preamble, so the last
			// byte goes here
			buf[last++] = 0x55;
		}
		for (uint8_t i = 0; i < xn297_addr_len; ++i)
		{
			buf[last] = xn297_tx_addr[xn297_addr_len-i-1];
			if(xn297_scramble_enabled)
				buf[last] ^= xn297_scramble[scramble_index++];
			last++;
		}

		// pcf
		buf[last] = (len << 1) | (pid>>1);
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[scramble_index++];
		last++;
		buf[last] = (pid << 7) | (noack << 6);

		// payload
		buf[last]|= bit_reverse(msg[0]) >> 2; // first 6 bit of payload
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[scramble_index++];

		for (uint8_t i = 0; i < len-1; ++i)
		{
			last++;
			buf[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
			if(xn297_scramble_enabled)
				buf[last] ^= xn297_scramble[scramble_index++];
		}

		last++;
		buf[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[scramble_index++] & 0xc0;

		// crc
		//if (xn297_crc)
		{
			uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
			uint16_t crc = 0xb5d2;
			for (uint8_t i = offset; i < last; ++i)
				crc = crc16_update(crc, buf[i], 8);
			crc = crc16_update(crc, buf[last] & 0xc0, 2);
			if (xn297_scramble_enabled)
				crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
			//else
			//	crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);

			buf[last++] |= (crc >> 8) >> 2;
			buf[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
			buf[last++] = (crc & 0xff) << 6;
		}

		pid++;
		pid &= 3;

		// stop TX/RX
		CC2500_Strobe(CC2500_SIDLE);
		// flush tx FIFO
		CC2500_Strobe(CC2500_SFTX);
		// packet length
		CC2500_WriteReg(CC2500_3F_TXFIFO, last + 3);
		// xn297L preamble
		CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (uint8_t*)"\x71\x0F\x55", 3);
		// xn297 packet
		CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, last);
		// transmit
		CC2500_Strobe(CC2500_STX);
	#endif
}

static void __attribute__((unused)) XN297L_HoppingCalib(uint8_t num_freq)
{	//calibrate hopping frequencies
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
		return;		//NRF
	#ifdef CC2500_INSTALLED
		for (uint8_t i = 0; i < num_freq; i++)
		{
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[i]*3);
			CC2500_Strobe(CC2500_SCAL);
			delayMicroseconds(900);
			calData[i]=CC2500_ReadReg(CC2500_25_FSCAL1);
		}
	#endif
}

static void __attribute__((unused)) XN297L_Hopping(uint8_t index)
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[index]);
		return;
	}
	#ifdef CC2500_INSTALLED
		// spacing is 333.25 kHz, must multiply xn297 channel by 3
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[index] * 3);
		// set PLL calibration
		CC2500_WriteReg(CC2500_25_FSCAL1, calData[index]);
	#endif
}

static void __attribute__((unused)) XN297L_RFChannel(uint8_t number)
{	//change channel
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, number);
		return;
	}
	#ifdef CC2500_INSTALLED
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, number*3);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
	#endif
}

static void __attribute__((unused)) XN297L_SetPower()
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		NRF24L01_SetPower();
		return;
	}
	#ifdef CC2500_INSTALLED
		CC2500_SetPower();
	#endif
}

static void __attribute__((unused)) XN297L_SetFreqOffset()
{	// Frequency offset
	#ifdef CC2500_INSTALLED
	if(option==0 && prev_option==0)
	#endif
		return;		//NRF
	#ifdef CC2500_INSTALLED
		if (prev_option != option)
		{
			if(prev_option==0 || option==0)
				CHANGE_PROTOCOL_FLAG_on;
			prev_option = option;
			CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		}
	#endif
}

static void __attribute__((unused)) NRF250K_SetTXAddr(uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, len);
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		xn297_addr_len = len;
		memcpy(xn297_tx_addr, addr, len);
	#endif
}

static void __attribute__((unused)) NRF250K_WritePayload(uint8_t* msg, uint8_t len)
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		NRF24L01_FlushTx();
		NRF24L01_WriteReg(NRF24L01_07_STATUS, _BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_RX_DR) | _BV(NRF24L01_07_MAX_RT));
		NRF24L01_WritePayload(msg, len);
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		#if defined(ESKY150V2_CC2500_INO)
			uint8_t buf[158];
		#else
			uint8_t buf[35];
		#endif
		uint8_t last = 0;
		uint8_t i;

		//nrf preamble
		if(xn297_tx_addr[xn297_addr_len - 1] & 0x80)
			buf[0]=0xAA;
		else
			buf[0]=0x55;
		last++;
		// address
		for (i = 0; i < xn297_addr_len; ++i)
			buf[last++] = xn297_tx_addr[xn297_addr_len - i - 1];
		// payload
		for (i = 0; i < len; ++i)
			buf[last++] = msg[i];

		// crc
		uint16_t crc = 0xffff;
		for (uint8_t i = 1; i < last; ++i)
			crc = crc16_update(crc, buf[i], 8);
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
		buf[last++] = 0;

		//for(uint8_t i=0;i<last;i++)
		//	debug("%02X ",buf[i]);
		//debugln("");
		// stop TX/RX
		CC2500_Strobe(CC2500_SIDLE);
		// flush tx FIFO
		CC2500_Strobe(CC2500_SFTX);
		// packet length
		CC2500_WriteReg(CC2500_3F_TXFIFO, last);
		// transmit nrf packet
		uint8_t *buff=buf;
		uint8_t status;
		if(last>63)
		{
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 63);
			CC2500_Strobe(CC2500_STX);
			last-=63;
			buff+=63;
			while(last)
			{//Loop until all the data is sent
				do
				{// Wait for the FIFO to become available
					status=CC2500_ReadReg(CC2500_3A_TXBYTES | CC2500_READ_BURST);
				}
				while((status&0x7F)>31 && (status&0x80)==0);
				if(last>31)
				{//Send 31 bytes
					CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 31);
					last-=31;
					buff+=31;
				}
				else
				{//Send last bytes
					CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
					last=0;
				}
			}
		}
		else
		{//Send packet
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
			CC2500_Strobe(CC2500_STX);
		}
	#endif
}

static boolean __attribute__((unused)) NRF250K_IsPacketSent()
{
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{	//NRF
		return NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS);
	}
	return true;	// don't know on the CC2500 how to detect if the packet has been transmitted...
}

#endif