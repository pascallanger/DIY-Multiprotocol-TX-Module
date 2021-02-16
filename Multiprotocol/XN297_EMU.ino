#if defined(CC2500_INSTALLED) || defined(NRF24L01_INSTALLED)

#include "iface_xn297.h"

#endif

#ifdef NRF24L01_INSTALLED
void XN297_SetTXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	uint8_t buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
	xn297_addr_len = len;
	if (xn297_addr_len < 4)
		for (uint8_t i = 0; i < 4; ++i)
			buf[i] = buf[i+1];
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, buf, 5);
	// Receive address is complicated. We need to use scrambled actual address as a receive address
	// but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
	// first. Also, if the scrambled address begins with 1 nRF24 will look for preamble byte 0xAA
	// instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
	// with receiving signals.
	memcpy(xn297_tx_addr, addr, len);
}

void XN297_SetRXAddr(const uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	uint8_t buf[] = { 0, 0, 0, 0, 0 };
	memcpy(buf, addr, len);
	memcpy(xn297_rx_addr, addr, len);
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		buf[i] = xn297_rx_addr[i];
		if(xn297_scramble_enabled)
			buf[i] ^= xn297_scramble[xn297_addr_len-i-1];
	}
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, buf, 5);
}

void XN297_Configure(uint8_t flags)
{
	xn297_crc = !!(flags & _BV(NRF24L01_00_EN_CRC));
	flags &= ~(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO));
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, flags & 0xFF);
}

void XN297_SetScrambledMode(const uint8_t mode)
{
    xn297_scramble_enabled = mode;
}

void XN297_WritePayload(uint8_t* msg, uint8_t len)
{
	uint8_t buf[32];
	uint8_t last = 0;

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
			buf[last] ^=  xn297_scramble[i];
		last++;
	}
	for (uint8_t i = 0; i < len; ++i)
	{
		// bit-reverse bytes in packet
		buf[last] = bit_reverse(msg[i]);
		if(xn297_scramble_enabled)
			buf[last] ^= xn297_scramble[xn297_addr_len+i];
		last++;
	}
	if (xn297_crc)
	{
		uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
		crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc16_update( buf[i], 8);
		if(xn297_scramble_enabled)
			crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
		else
			crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
	}
	NRF24L01_WritePayload(buf, last);
}

void XN297_WriteEnhancedPayload(uint8_t* msg, uint8_t len, uint8_t noack)
{
	uint8_t packet[32];
	uint8_t scramble_index=0;
	uint8_t last = 0;
	static uint8_t pid=0;

	// address
	if (xn297_addr_len < 4)
	{
		// If address length (which is defined by receive address length)
		// is less than 4 the TX address can't fit the preamble, so the last
		// byte goes here
		packet[last++] = 0x55;
	}
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		packet[last] = xn297_tx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			packet[last] ^= xn297_scramble[scramble_index++];
		last++;
	}

	// pcf
	packet[last] = (len << 1) | (pid>>1);
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++];
	last++;
	packet[last] = (pid << 7) | (noack << 6);

	// payload
	packet[last]|= bit_reverse(msg[0]) >> 2; // first 6 bit of payload
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++];

	for (uint8_t i = 0; i < len-1; ++i)
	{
		last++;
		packet[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
		if(xn297_scramble_enabled)
			packet[last] ^= xn297_scramble[scramble_index++];
	}

	last++;
	packet[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
	if(xn297_scramble_enabled)
		packet[last] ^= xn297_scramble[scramble_index++] & 0xc0;

	// crc
	if (xn297_crc)
	{
		uint8_t offset = xn297_addr_len < 4 ? 1 : 0;
		crc = 0xb5d2;
		for (uint8_t i = offset; i < last; ++i)
			crc16_update( packet[i], 8);
		crc16_update( packet[last] & 0xc0, 2);
		if (xn297_scramble_enabled)
			crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
		//else
		//	crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);

		packet[last++] |= (crc >> 8) >> 2;
		packet[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
		packet[last++] = (crc & 0xff) << 6;
	}
	NRF24L01_WritePayload(packet, last);

	pid++;
	if(pid>3)
		pid=0;
}

boolean XN297_ReadPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget if using CRC to do a +2 on any of the used NRF24L01_11_RX_PW_Px !!!
	uint8_t buf[32];
	if (xn297_crc)
		NRF24L01_ReadPayload(buf, len+2);	// Read payload + CRC 
	else
		NRF24L01_ReadPayload(buf, len);
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
	{
		uint8_t b_in=xn297_rx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			b_in ^=  xn297_scramble[i];
		crc16_update( b_in, 8);
	}
	//process payload
	for (uint8_t i = 0; i < len; ++i)
		crc16_update( buf[i], 8);
	//xorout
	if(xn297_scramble_enabled)
		crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
	else
		crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
	//test
	if( (crc >> 8) == buf[len] && (crc & 0xff) == buf[len+1])
		return true;	// CRC  OK
	return false;		// CRC NOK
}

uint8_t XN297_ReadEnhancedPayload(uint8_t* msg, uint8_t len)
{ //!!! Don't forget do a +2 and if using CRC add +4 on any of the used NRF24L01_11_RX_PW_Px !!!
	uint8_t buffer[32];
	uint8_t pcf_size; // pcf payload size
	if (xn297_crc)
		NRF24L01_ReadPayload(buffer, len+4);	// Read pcf + payload + CRC 
	else
		NRF24L01_ReadPayload(buffer, len+2);	// Read pcf + payload
	pcf_size = buffer[0];
	if(xn297_scramble_enabled)
		pcf_size ^= xn297_scramble[xn297_addr_len];
	pcf_size = pcf_size >> 1;
	for(int i=0; i<len; i++)
	{
		msg[i] = bit_reverse((buffer[i+1] << 2) | (buffer[i+2] >> 6));
		if(xn297_scramble_enabled)
			msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) | 
									(xn297_scramble[xn297_addr_len+i+2] >> 6));
	}

	if (!xn297_crc)
		return pcf_size;	// No CRC so OK by default...

	// Calculate CRC
	crc = 0xb5d2;
	//process address
	for (uint8_t i = 0; i < xn297_addr_len; ++i)
	{
		uint8_t b_in=xn297_rx_addr[xn297_addr_len-i-1];
		if(xn297_scramble_enabled)
			b_in ^=  xn297_scramble[i];
		crc16_update( b_in, 8);
	}
	//process payload
	for (uint8_t i = 0; i < len+1; ++i)
		crc16_update( buffer[i], 8);
	crc16_update( buffer[len+1] & 0xc0, 2);
	//xorout
	if (xn297_scramble_enabled)
		crc ^= pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+len]);
#ifdef XN297DUMP_NRF24L01_INO
	else
		crc ^= pgm_read_word(&xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len]);
#endif
	uint16_t crcxored=(buffer[len+1]<<10)|(buffer[len+2]<<2)|(buffer[len+3]>>6) ;
	if( crc == crcxored)
		return pcf_size;	// CRC  OK
	return 0;				// CRC NOK
}
 
 // End of XN297 emulation

#endif
