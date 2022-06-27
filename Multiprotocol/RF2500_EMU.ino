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
#include "iface_rf2500.h"

#define RF2500_ADDR_LENGTH 4

uint8_t RF2500_payload_length, RF2500_tx_addr[RF2500_ADDR_LENGTH], RF2500_buf[100];
bool RF2500_scramble_enabled;

static void __attribute__((unused)) RF2500_Init(uint8_t payload_length, bool scramble)
{
	CYRF_GFSK1M_Init( RF2500_ADDR_LENGTH + 2 + (payload_length+2)*4, 2 );		// full payload length with CRC + address + 5 + FEC

	RF2500_payload_length=payload_length;
	RF2500_scramble_enabled=scramble;
}

static void __attribute__((unused)) RF2500_SetTXAddr(const uint8_t* addr)
{
	memcpy(RF2500_tx_addr, addr, RF2500_ADDR_LENGTH);
}

static void __attribute__((unused)) RF2500_BuildPayload(uint8_t* buffer)
{
	const uint8_t RF2500_scramble[] = { 0xD0, 0x9E, 0x53, 0x33, 0xD8, 0xBA, 0x98, 0x08, 0x24, 0xCB, 0x3B, 0xFC, 0x71, 0xA3, 0xF4, 0x55, 0x68, 0x4F, 0xA9 }; //0x4F: unsure
	uint16_t RF2500_crc_xorout_scramble;
	if(RF2500_payload_length == 16)
		RF2500_crc_xorout_scramble = 0xAEE4;
	else //19
		RF2500_crc_xorout_scramble = 0xE7C5;

	#if 0
		for(uint8_t i=0; i<RF2500_payload_length; i++)
			debug("%02X ", buffer[i]);
	#endif
	//Scramble the incoming buffer
	if(RF2500_scramble_enabled)
		for(uint8_t i=0; i<RF2500_payload_length; i++)
			buffer[i] ^= RF2500_scramble[i];

	//Add CRC to the buffer
	crc=0x0000;
	for(uint8_t i=0;i<RF2500_payload_length;i++)
		crc16_update(bit_reverse(buffer[i]),8);
	buffer[RF2500_payload_length  ] = bit_reverse(crc>>8);
	buffer[RF2500_payload_length+1] = bit_reverse(crc);

	#if 0
		debugln("C:%02X %02X",buffer[RF2500_payload_length  ], buffer[RF2500_payload_length+1]);
	#endif

	if(RF2500_scramble_enabled)
	{
		buffer[RF2500_payload_length  ] ^= RF2500_crc_xorout_scramble>>8;
		buffer[RF2500_payload_length+1] ^= RF2500_crc_xorout_scramble;
	}

	#if 0
		debug("B:");
		for(uint8_t i=0; i<RF2500_payload_length+2; i++)
			debug(" %02X",buffer[i]);
		debugln("");
	#endif

	memcpy(RF2500_buf,RF2500_tx_addr,RF2500_ADDR_LENGTH);		// Address
	
	uint8_t pos = RF2500_ADDR_LENGTH;

	RF2500_buf[pos++]=0xC3;RF2500_buf[pos++]=0xC3;				// 5 FEC encoded
	memset(&RF2500_buf[pos],0x00,(RF2500_payload_length+2)*4);	// + CRC) * 4 FEC bytes per byte
	
	//FEC encode
	for(uint8_t i=0; i<RF2500_payload_length+2; i++)			// Include CRC
	{
		for(uint8_t j=0;j<8;j++)
		{
			uint8_t offset=pos + (i<<2) + (j>>1);
			RF2500_buf[offset] <<= 4;
			if( (buffer[i]>>j) & 0x01 )
				RF2500_buf[offset] |= 0x0C;
			else
				RF2500_buf[offset] |= 0x03;
		}
	}

	#if 0
		debug("E:");
		for(uint8_t i=0; i<RF2500_ADDR_LENGTH+2+(RF2500_payload_length+2)*4; i++)
			debug(" %02X",RF2500_buf[i]);
		debugln("");
	#endif

	//CYRF wants LSB first
	for(uint8_t i=0; i<RF2500_ADDR_LENGTH+2+(RF2500_payload_length+2)*4; i++)
		RF2500_buf[i]=bit_reverse(RF2500_buf[i]);
}

static void __attribute__((unused)) RF2500_SendPayload()
{
	CYRF_GFSK1M_SendPayload(RF2500_buf, RF2500_ADDR_LENGTH + 2 + (RF2500_payload_length+2)*4 );
}

#endif