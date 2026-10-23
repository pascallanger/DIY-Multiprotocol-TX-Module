#if defined(CC2500_INSTALLED) || defined(NRF24L01_INSTALLED)

#include "iface_xn297.h"

bool xn297_scramble_enabled, xn297_crc, xn297_bitrate, xn297_rf;
uint8_t xn297_addr_len, xn297_rx_packet_len;
uint8_t xn297_tx_addr[5], xn297_rx_addr[5];

// xn297 address / pcf / payload scramble table
const uint8_t xn297_scramble[] = {
    0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66,
    0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69, 0xEE, 0x1F,
    0xC7, 0x62, 0x97, 0xD5, 0x0B, 0x79, 0xCA, 0xCC,
    0x1B, 0x5D, 0x19, 0x10, 0x24, 0xD3, 0xDC, 0x3F,
    0x8E, 0xC5, 0x2F, 0xAA, 0x16, 0xF3, 0x95 };

// XN297 CRC: CRC-16/CCITT (init 0xFFFF, xorout 0xFFFF) over the unscrambled
// address, pcf and payload, sent scrambled. The emulation computes it from 0xB5D2
// over the bits as sent: the xorout is the difference for a CRC over 'bits' bits,
// the CRC of the scramble bits plus the scramble of the CRC bits.
static uint16_t __attribute__((unused)) XN297_crc_xorout(bool scramble, uint16_t bits)
{
	uint16_t xorout = 0xFFFF ^ 0xB5D2;
	uint8_t lfsr = 0x71, s;					// scramble bits (xn297_scramble[]): 1110001, then s[i] = s[i-7] ^ s[i-3]
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
	//Scramble address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		xn297_rx_addr[i] = addr[i];
		if(xn297_scramble_enabled)
			xn297_rx_addr[i] ^= xn297_scramble[xn297_addr_len-i-1];
	}

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
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		buf[last] = xn297_tx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			buf[last] ^=  xn297_scramble[i];
		last++;
	}

	// payload
	for (uint8_t i = 0; i < len; ++i)
	{
		// bit-reverse bytes in packet
		buf[last] = bit_reverse(msg[i]);
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[xn297_addr_len+i];
		last++;
	}

	// crc
	if (xn297_crc)
	{
		uint8_t offset = (xn297_addr_len < 4  && xn297_rf == XN297_NRF) ? 1 : 0;
		crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc16_update( buf[i], 8);
		crc ^= XN297_crc_xorout(xn297_scramble_enabled, 8 * (xn297_addr_len + len));
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
	}

	// send packet
	XN297_SendPayload(buf, last);
}

static void __attribute__((unused)) XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
	uint8_t buf[32];
	uint8_t scramble_index=0;
	uint8_t last = 0;
	static uint8_t pid=0;

	if (xn297_rf == XN297_NRF && xn297_addr_len < 4)
	{ // If address length (which is defined by receiver address length) is less than 4 the TX address can't fit the preamble, so the last byte goes here
		buf[last++] = 0x55;
	}

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
	if(len)
	{
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
	}
	if(xn297_scramble_enabled)
		buf[last] ^= xn297_scramble[scramble_index++] & 0xc0;

	// crc
	if (xn297_crc)
	{
		uint8_t offset = (xn297_addr_len < 4 && xn297_rf == XN297_NRF) ? 1 : 0;
		crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc16_update( buf[i], 8);
		crc16_update( buf[last] & 0xc0, 2);
		if (xn297_scramble_enabled)
			crc ^= XN297_crc_xorout(true, 8 * (xn297_addr_len + len) + 10);
		//else
		//	crc ^= XN297_crc_xorout(false, 8 * (xn297_addr_len + len) + 10);

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

	// Decode payload
	for(uint8_t i=0; i<len; i++)
	{
		uint8_t b_in=buf[i];
		if(xn297_scramble_enabled)
			b_in ^= xn297_scramble[i+xn297_addr_len];
		msg[i] = bit_reverse(b_in);
	}

	if (!xn297_crc)
		return true;	// No CRC so OK by default...

	// Calculate CRC
	crc = 0xb5d2;
	//process address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		crc16_update( xn297_rx_addr[xn297_addr_len-i-1], 8);
	//process payload
	for (uint8_t i = 0; i < len; ++i)
		crc16_update( buf[i], 8);
	//xorout
	crc ^= XN297_crc_xorout(xn297_scramble_enabled, 8 * (xn297_addr_len + len));
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
	// Decode payload
	pcf_size = buffer[0];
	if(xn297_scramble_enabled)
		pcf_size ^= xn297_scramble[xn297_addr_len];
	pcf_size = pcf_size >> 1;
	if(pcf_size>32)
		return 255;								// Error
	for(uint8_t i=0; i< pcf_size; i++)
	{
		msg[i] = bit_reverse((buffer[i+1] << 2) | (buffer[i+2] >> 6));
		if(xn297_scramble_enabled)
			msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) | 
									(xn297_scramble[xn297_addr_len+i+2] >> 6));
	}

	if (!xn297_crc)
		return pcf_size;						// No CRC so OK by default...

	// Calculate CRC
	crc = 0xb5d2;
	//process address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
		crc16_update( xn297_rx_addr[xn297_addr_len-i-1], 8);
	//process payload
	for (uint8_t i = 0; i < pcf_size+1; ++i)
		crc16_update( buffer[i], 8);
	crc16_update( buffer[pcf_size+1] & 0xc0, 2);
	//xorout
	if (xn297_scramble_enabled)
		crc ^= XN297_crc_xorout(true, 8 * (xn297_addr_len + pcf_size) + 10);
#ifdef XN297DUMP_NRF24L01_INO
	else
		crc ^= XN297_crc_xorout(false, 8 * (xn297_addr_len + pcf_size) + 10);
#endif
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
