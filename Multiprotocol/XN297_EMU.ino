#if defined(CC2500_INSTALLED) || defined(NRF24L01_INSTALLED)

#include "iface_xn297.h"

bool xn297_scramble_enabled, xn297_crc, xn297_bitrate, xn297_rf;
uint8_t xn297_addr_len, xn297_rx_packet_len;
uint8_t xn297_tx_addr[5], xn297_rx_addr[5];

// XN297 whitening: x^7 + x^3 + 1, seed 0x71, MSB first (E3 B1 4B EA 85 ...).
// Generated a byte at a time with (x^7 + x^3 + 1)^4 = x^28 + x^12 + 1: s[i] = s[i-28] ^ s[i-12].
// xn297_lfsr holds the next 28 bits in bits 31..4, 0 when scrambling is off.
uint32_t xn297_lfsr;

static uint8_t __attribute__((unused)) XN297_scramble_byte()
{
	uint8_t s = xn297_lfsr >> 24;
	xn297_lfsr = (xn297_lfsr << 8) | (uint8_t)(s ^ (xn297_lfsr >> 8)) << 4;
	return s;
}

static void __attribute__((unused)) XN297_scramble_init()
{
	xn297_lfsr = xn297_scramble_enabled ? XN297_SCRAMBLE_SEED : 0;
}

// XN297 CRC: CRC-16/CCITT (init 0xFFFF, xorout 0xFFFF) over the unscrambled
// address, pcf and payload, sent scrambled: XORed with the scramble bits that follow,
// (xn297_lfsr >> 16) once the last byte has been scrambled.
// Returns the scrambled byte, crc updated with the unscrambled one.
static uint8_t __attribute__((unused)) XN297_crc_scramble(uint8_t b)
{
	crc16_update(b, 8);
	return b ^ XN297_scramble_byte();
}

// Returns the unscrambled byte, crc updated with it.
static uint8_t __attribute__((unused)) XN297_unscramble_crc(uint8_t b)
{
	b ^= XN297_scramble_byte();
	crc16_update(b, 8);
	return b;
}

#ifdef XN297DUMP_NRF24L01_INO
// XN297Dump computes the CRC from 0xB5D2 over the bits as sent: the xorout is the
// difference for a CRC over 'bits' bits, the CRC of the scramble bits plus the
// scramble of the CRC bits.
static uint16_t __attribute__((unused)) XN297_crc_xorout(bool scramble, uint16_t bits)
{
	uint16_t xorout = 0xFFFF ^ 0xB5D2;
	uint8_t lfsr = 0x71, s;					// scramble bits: 1110001, then s[i] = s[i-7] ^ s[i-3]
	for(uint16_t i = 0; i < bits + 16; i++)
	{
		s = scramble ? lfsr >> 6 : 0;
		lfsr = ((lfsr << 1) | (((lfsr >> 6) ^ (lfsr >> 2)) & 1)) & 0x7F;
		if(i < bits)
		{
			if((xorout >> 15) ^ s)
				xorout = (xorout << 1) ^ 0x1021;
			else
				xorout <<= 1;
		}
		else
			xorout ^= (uint16_t)s << (bits + 15 - i);
	}
	return ~xorout;
}
#endif

static bool __attribute__((unused)) XN297_Configure(bool crc_en, bool scramble_en, bool bitrate, bool force_nrf)
{
	xn297_crc = crc_en;
	xn297_scramble_enabled = scramble_en;
	xn297_bitrate = bitrate;
	xn297_rf = XN297_NRF;
	
	#if defined(NRF24L01_INSTALLED) and defined(CC2500_INSTALLED)
		if(bitrate == XN297_1M || force_nrf)
			xn297_rf = XN297_NRF;		// Use NRF24L01
		else
			xn297_rf = XN297_CC2500;	// Use CC2500
	#elif defined(NRF24L01_INSTALLED) and not defined(CC2500_INSTALLED)
		xn297_rf = XN297_NRF;			// Use NRF24L01
	#else //CC2500 only
		xn297_rf = XN297_CC2500;		// Use CC2500
		if(bitrate == XN297_1M)
		{
			xn297_rf = XN297_NRF;		// Use NRF24L01 which does not exist, nothing will happen...
			SUB_PROTO_INVALID;
			return false;				// Can't do...
		}
	#endif

	#if defined(NRF24L01_INSTALLED)
		if(xn297_rf == XN297_NRF)
		{
			debugln("Using NRF");
			rf_switch(SW_NRF);
			NRF24L01_Initialize();
			if(bitrate == XN297_250K)
				NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
		}
	#endif
	#if defined(CC2500_INSTALLED)
		if(xn297_rf == XN297_CC2500)
		{
			debugln("Using CC2500");
			rf_switch(SW_CC2500);
			CC2500_250K_Init();
			option_override = 2;	// OPTION_RFTUNE
		}
	#endif
	return true;
}

static void __attribute__((unused)) XN297_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	xn297_addr_len = len;
	memcpy(xn297_tx_addr, addr, len);
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
		{
			uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 };			// bytes for XN297 preamble 0xC710F55 (28 bit)
			NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, xn297_addr_len == 3 ? buf+1 : buf, 5);
		}
	#endif
};

static void __attribute__((unused)) XN297_SetRXAddr(const uint8_t* addr, uint8_t rx_packet_len)
{
	//Scramble address in transmission order
	XN297_scramble_init();
	for (uint8_t i = xn297_addr_len; i--; )
		xn297_rx_addr[i] = addr[i] ^ XN297_scramble_byte();

	if(xn297_crc)
		rx_packet_len += 2;															// Include CRC
	rx_packet_len += 2;																// Include pcf, will this be a problem timing wise even if not enhanced?

	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
		{
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, xn297_rx_addr, xn297_addr_len);
			if(rx_packet_len > 32)
				rx_packet_len = 32;
			NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, rx_packet_len);
		}
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
		{// TX: Sync1, Sync0, Address
			CC2500_WriteReg(CC2500_04_SYNC1, xn297_rx_addr[xn297_addr_len-1]);		// Sync word, high byte
			CC2500_WriteReg(CC2500_05_SYNC0, xn297_rx_addr[xn297_addr_len-2]);		// Sync word, low byte
			CC2500_WriteReg(CC2500_09_ADDR,  xn297_rx_addr[xn297_addr_len-3]);		// Address
			rx_packet_len += 1 + xn297_addr_len - 3;								// The Address field above will be in the payload then the end of the XN297 address
		}
	#endif
	xn297_rx_packet_len = rx_packet_len;
}

static void __attribute__((unused)) XN297_SetTxRxMode(enum TXRX_State mode)
{
	static enum TXRX_State cur_mode=TXRX_OFF;
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
		{
			
			NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)			//reset the flag(s)
												| (1 << NRF24L01_07_TX_DS)
												| (1 << NRF24L01_07_MAX_RT));
			if(mode==TXRX_OFF)
			{
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0);							//PowerDown
				NRF_CE_off;
				return;
			}
			NRF_CE_off;
			if(mode == TX_EN)
			{
				NRF24L01_FlushTx();
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, 1 << NRF24L01_00_PWR_UP);
			}
			else
			{
				NRF24L01_FlushRx();
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_PWR_UP)
													| (1 << NRF24L01_00_PRIM_RX));	// RX
			}
			if(mode != cur_mode)
			{
				//delayMicroseconds(130);
				cur_mode=mode;
			}			
			NRF_CE_on;
		}
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
		{
			if(mode != cur_mode)
			{
				CC2500_SetTxRxMode(mode);
				if(mode == RX_EN)
				{
					CC2500_WriteReg(CC2500_12_MDMCFG2,  0x12);   						// Modem Configuration, GFSK, 16/16 Sync Word TX&RX
					CC2500_WriteReg(CC2500_06_PKTLEN, xn297_rx_packet_len);				// Packet len
					CC2500_Strobe(CC2500_SFRX);
					CC2500_Strobe(CC2500_SRX);
				}
				else
					CC2500_WriteReg(CC2500_12_MDMCFG2,	0x10);   						// Modem Configuration, GFSK, no preambule and no sync word
				cur_mode=mode;
			}
		}
	#endif
}

#ifdef CC2500_INSTALLED
uint8_t XN297_Buffer[32];
uint8_t XN297_Buffer_Len = 0;

static void __attribute__((unused)) XN297_SendCC2500Payload()
{
	// stop TX/RX
	CC2500_Strobe(CC2500_SIDLE);
	// flush tx FIFO
	CC2500_Strobe(CC2500_SFTX);
	// packet length
	CC2500_WriteReg(CC2500_06_PKTLEN, XN297_Buffer_Len + 4);  // Packet len, fix packet len
	// xn297L preamble
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (uint8_t*)"\x0C\x71\x0F\x55", 4);
	// xn297 packet
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, XN297_Buffer, XN297_Buffer_Len);
	// transmit
	CC2500_Strobe(CC2500_STX);
}
#endif

static void __attribute__((unused)) XN297_SendPayload(uint8_t* msg, uint8_t len)
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
		{
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			NRF24L01_FlushTx();
			NRF24L01_WritePayload(msg, len);
		}
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
		{
			memcpy(XN297_Buffer, msg, len);
			XN297_Buffer_Len = len;
			XN297_SendCC2500Payload();
		}
	#endif
}

static void __attribute__((unused)) XN297_ReSendPayload()
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			NRF24L01_Strobe(NRF24L01_E3_REUSE_TX_PL);
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			XN297_SendCC2500Payload();
	#endif
}

static void __attribute__((unused)) XN297_WritePayload(uint8_t* msg, uint8_t len)
{
	uint8_t buf[32];
	uint8_t last = 0;

	if (xn297_rf == XN297_NRF && xn297_addr_len < 4 && xn297_rf == XN297_NRF)
	{ // If address length (which is defined by receiver address length) is less than 4 the TX address can't fit the preamble, so the last byte goes here
		buf[last++] = 0x55;
	}

	// address
	crc = 0xFFFF;
	XN297_scramble_init();
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		buf[last++] = XN297_crc_scramble(xn297_tx_addr[xn297_addr_len-i-1]);

	// payload
	for (uint8_t i = 0; i < len; ++i)
		buf[last++] = XN297_crc_scramble(bit_reverse(msg[i]));	// bit-reverse bytes in packet

	// crc
	if (xn297_crc)
	{
		crc = ~crc ^ (xn297_lfsr >> 16);
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
	}

	// send packet
	XN297_SendPayload(buf, last);
}

static void __attribute__((unused)) XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
	uint8_t buf[32];
	uint8_t last = 0;
	static uint8_t pid=0;

	if (xn297_rf == XN297_NRF && xn297_addr_len < 4)
	{ // If address length (which is defined by receiver address length) is less than 4 the TX address can't fit the preamble, so the last byte goes here
		buf[last++] = 0x55;
	}

	// address
	crc = xn297_scramble_enabled ? 0xFFFF : 0xB5D2;	// unscrambled: no xorout
	XN297_scramble_init();
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		buf[last++] = XN297_crc_scramble(xn297_tx_addr[xn297_addr_len-i-1]);

	// pcf
	buf[last++] = XN297_crc_scramble((len << 1) | (pid>>1));
	buf[last] = (pid << 7) | (noack << 6);

	// payload
	if(len)
	{
		buf[last] = XN297_crc_scramble(buf[last] | (bit_reverse(msg[0]) >> 2)); // first 6 bit of payload

		for (uint8_t i = 0; i < len-1; ++i)
		{
			last++;
			buf[last] = XN297_crc_scramble((bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2));
		}

		last++;
		buf[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
	}
	crc16_update( buf[last], 2);
	buf[last] ^= (xn297_lfsr >> 24) & 0xc0;

	// crc
	if (xn297_crc)
	{
		if (xn297_scramble_enabled)
			crc = ~crc ^ (xn297_lfsr >> 14);

		buf[last++] |= (crc >> 8) >> 2;
		buf[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
		buf[last++] = (crc & 0xff) << 6;
	}
	pid++;
	pid &= 0x03;

	// send packet
	XN297_SendPayload(buf, last);
}

static bool __attribute__((unused)) XN297_IsRX()
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			return (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR));
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
		{
			if((CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F) != xn297_rx_packet_len + 2) // 2 = RSSI + LQI
				return false; 										// Buffer does not contain the expected number of bytes
			// Check the address
			uint8_t buf[3];
			CC2500_ReadData(buf, xn297_addr_len-3 + 1);
			for(uint8_t i=0; i < xn297_addr_len-3 + 1; i++)
				if(buf[i] != xn297_rx_addr[xn297_addr_len-3 - i])
					return false;									// Bad address
			return true;											// Address is correct
		}
	#endif
	return false;
}

static void __attribute__((unused)) XN297_ReceivePayload(uint8_t* msg, uint8_t len)
{
	if (xn297_crc)
		len += 2;									// Include CRC 
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			NRF24L01_ReadPayload(msg, len);			// Read payload and CRC 
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_ReadData(msg, len);
	#endif
}

static bool __attribute__((unused)) XN297_ReadPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget if using CRC to do a +2 on the received packet length (NRF24L01_11_RX_PW_Px !!! or CC2500_06_PKTLEN)
	uint8_t buf[32];

	// Read payload
	XN297_ReceivePayload(buf, len);

	// Decode payload, calculate CRC
	crc = 0xFFFF;
	XN297_scramble_init();
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		XN297_unscramble_crc(xn297_rx_addr[xn297_addr_len-i-1]);
	for(uint8_t i=0; i<len; i++)
		msg[i] = bit_reverse(XN297_unscramble_crc(buf[i]));

	if (!xn297_crc)
		return true;	// No CRC so OK by default...

	crc = ~crc ^ (xn297_lfsr >> 16);
	//test
	if( (crc >> 8) == buf[len] && (crc & 0xff) == buf[len+1])
		return true;	// CRC  OK
	return false;		// CRC NOK
}

static uint8_t __attribute__((unused)) XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget do a +2 and if using CRC add +4 on any of the used NRF24L01_11_RX_PW_Px !!!
	uint8_t buffer[32];
	uint8_t pcf_size;							// pcf payload size

	// Read payload
	XN297_ReceivePayload(buffer, len+2);		// Read pcf + payload + CRC
	// Decode payload, calculate CRC
#ifdef XN297DUMP_NRF24L01_INO
	bool xorout = true;
#else
	bool xorout = xn297_scramble_enabled;		// unscrambled: no xorout
#endif
	crc = xorout ? 0xFFFF : 0xB5D2;
	XN297_scramble_init();
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		XN297_unscramble_crc(xn297_rx_addr[xn297_addr_len-i-1]);
	pcf_size = XN297_unscramble_crc(buffer[0]) >> 1;
	if(pcf_size>32)
		return 255;								// Error
	for(uint8_t i=0; i< pcf_size; i++)
	{	// payload is shifted by 2 bits: unscramble with bits 29..22 of the next scramble bits
		msg[i] = bit_reverse(((buffer[i+1] << 2) | (buffer[i+2] >> 6)) ^ (xn297_lfsr >> 22));
		XN297_unscramble_crc(buffer[i+1]);
	}
	crc16_update( (buffer[pcf_size+1] ^ (xn297_lfsr >> 24)) & 0xc0, 2);

	if (!xn297_crc)
		return pcf_size;						// No CRC so OK by default...

	if (xorout)
		crc = ~crc ^ (xn297_lfsr >> 14);
	uint16_t crcxored=(buffer[pcf_size+1]<<10)|(buffer[pcf_size+2]<<2)|(buffer[pcf_size+3]>>6) ;
	if( crc == crcxored)
		return pcf_size;					// CRC  OK
	return 255;								// CRC NOK
}
 
static bool __attribute__((unused)) XN297_IsPacketSent()
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			return (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS));
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			return (CC2500_ReadReg(CC2500_35_MARCSTATE | CC2500_READ_BURST) != 0x13);
	#endif
	return true;	// packet sent to not block
}

static void __attribute__((unused)) XN297_HoppingCalib(uint8_t num_freq)
{	//calibrate hopping frequencies
	#ifdef NRF24L01_INSTALLED
		(void)num_freq;
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_250K_HoppingCalib(num_freq);
	#endif
}

static void __attribute__((unused)) XN297_Hopping(uint8_t index)
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[index]);
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_250K_Hopping(index);
	#endif
}

static void __attribute__((unused)) XN297_RFChannel(uint8_t number)
{	//change channel
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, number);
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_250K_RFChannel(number);
	#endif
}

static void __attribute__((unused)) XN297_SetPower()
{
	#ifdef NRF24L01_INSTALLED
		if(xn297_rf == XN297_NRF)
			NRF24L01_SetPower();
	#endif
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_SetPower();
	#endif
}

static void __attribute__((unused)) XN297_SetFreqOffset()
{	// Frequency offset
	#ifdef CC2500_INSTALLED
		if(xn297_rf == XN297_CC2500)
			CC2500_SetFreqOffset();
	#endif
}

 // End of XN297 emulation

#endif
