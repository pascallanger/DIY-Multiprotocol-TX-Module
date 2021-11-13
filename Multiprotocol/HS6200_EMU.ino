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
#ifdef CYRF6936_INSTALLED
#include "iface_HS6200.h"

static bool HS6200_crc;
static uint16_t HS6200_crc_init;
static uint8_t HS6200_address_length, HS6200_tx_addr[5];

static void __attribute__((unused)) HS6200_Init(bool crc_en)
{
	CYRF_GFSK1M_Init(32, 1);	//Dummy number of bytes for now
	HS6200_crc = crc_en;
}

static void __attribute__((unused)) HS6200_SetTXAddr(const uint8_t* addr, uint8_t addr_len)
{
	// precompute address crc
	crc = 0xffff;
	for(uint8_t i=0; i<addr_len; i++)
		crc16_update(addr[addr_len-1-i], 8);
	HS6200_crc_init=crc;
	memcpy(HS6200_tx_addr, addr, addr_len);
	HS6200_address_length = addr_len;
}

static uint16_t __attribute__((unused)) HS6200_calc_crc(uint8_t* msg, uint8_t len)
{
    uint8_t pos;
    
	crc = HS6200_crc_init;
    // pcf + payload
    for(pos=0; pos < len-1; pos++)
        crc16_update(msg[pos], 8);
    // last byte (1 bit only)
    if(len > 0)
        crc16_update(msg[pos+1], 1);
    return crc;
}

static void __attribute__((unused)) HS6200_SendPayload(uint8_t* msg, uint8_t len)
{
	static const uint8_t HS6200_scramble[] = { 0x80,0xf5,0x3b,0x0d,0x6d,0x2a,0xf9,0xbc,0x51,0x8e,0x4c,0xfd,0xc1,0x65,0xd0 }; // todo: find all 32 bytes ...
	uint8_t payload[32];
	const uint8_t no_ack = 1; // never ask for an ack
	static uint8_t pid;
	uint8_t pos = 0;

	if(len > sizeof(HS6200_scramble))
		len = sizeof(HS6200_scramble);

	// address
	for(int8_t i=HS6200_address_length-1; i>=0; i--)
		payload[pos++] = HS6200_tx_addr[i];

	// guard bytes
	payload[pos++] = HS6200_tx_addr[0];
	payload[pos++] = HS6200_tx_addr[0];

	// packet control field
	payload[pos++] = ((len & 0x3f) << 2) | (pid & 0x03);
	payload[pos] = (no_ack & 0x01) << 7;
	pid++;

	// scrambled payload
	if(len > 0)
	{
		payload[pos++] |= (msg[0] ^ HS6200_scramble[0]) >> 1; 
		for(uint8_t i=1; i<len; i++)
			payload[pos++] = ((msg[i-1] ^ HS6200_scramble[i-1]) << 7) | ((msg[i] ^ HS6200_scramble[i]) >> 1);
		payload[pos] = (msg[len-1] ^ HS6200_scramble[len-1]) << 7; 
	}

	// crc
	if(HS6200_crc)
	{
		uint16_t crc = HS6200_calc_crc(&payload[HS6200_address_length+2], len+2);
		uint8_t hcrc = crc >> 8;
		uint8_t lcrc = crc & 0xff;
		payload[pos++] |= (hcrc >> 1);
		payload[pos++] = (hcrc << 7) | (lcrc >> 1);
		payload[pos++] = lcrc << 7;
	}

	#if 0
		debug("E:");
		for(uint8_t i=0; i<pos; i++)
			debug(" %02X",payload[i]);
		debugln("");
	#endif

	//CYRF wants LSB first
	for(uint8_t i=0; i<pos; i++)
		payload[i]=bit_reverse(payload[i]);
	//Send
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, pos);
	CYRF_GFSK1M_SendPayload(payload, pos);
}

#endif