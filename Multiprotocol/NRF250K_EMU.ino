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
	prev_option = option;
	#ifdef CC2500_INSTALLED
	if(option==0)
	#endif
	{//NRF
		debugln("Using NRF");
		PE1_on;							//NRF24L01 antenna RF3 by default
		PE2_off;						//NRF24L01 antenna RF3 by default
		NRF24L01_Initialize();
		NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
		return;
	}
	//CC2500
	#ifdef CC2500_INSTALLED
		debugln("Using CC2500");
		xn297_scramble_enabled=XN297_SCRAMBLED;	//enabled by default
		PE1_off; // antenna RF2
		PE2_on;
		CC2500_250K_Init();
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
		crc = 0xb5d2;
		for (uint8_t i = 0; i < last; ++i)
			crc16_update( buf[i], 8);
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
			crc = 0xb5d2;
			for (uint8_t i = 0; i < last; ++i)
				crc16_update( buf[i], 8);
			crc16_update( buf[last] & 0xc0, 2);
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
		CC2500_250K_HoppingCalib(num_freq);
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
		CC2500_250K_Hopping(index);
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
		CC2500_250K_RFChannel(number);
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
				CHANGE_PROTOCOL_FLAG_on;	// switch from NRF <-> CC2500
			CC2500_SetFreqOffset();
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
		CC2500_250K_NRF_SetTXAddr(addr, len);
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
		CC2500_250K_NRF_WritePayload(msg, len);
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