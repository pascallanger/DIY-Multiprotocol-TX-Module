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

const uint8_t PROGMEM RF2500_init_vals[][2] = {
	{CYRF_02_TX_CTRL, 0x00},		// transmit err & complete interrupts disabled
	{CYRF_05_RX_CTRL, 0x00},		// receive err & complete interrupts disabled
	{CYRF_28_CLK_EN, 0x02},			// Force Receive Clock Enable, MUST be set
	{CYRF_32_AUTO_CAL_TIME, 0x3c},	// must be set to 3C
	{CYRF_35_AUTOCAL_OFFSET, 0x14},	// must be  set to 14
	{CYRF_06_RX_CFG, 0x48},			// LNA manual control, Rx Fast Turn Mode Enable
	{CYRF_1B_TX_OFFSET_LSB, 0x00},	// Tx frequency offset LSB
	{CYRF_1C_TX_OFFSET_MSB, 0x00},	// Tx frequency offset MSB
	{CYRF_0F_XACT_CFG, 0x24},		// Force End State, transaction end state = idle
	{CYRF_03_TX_CFG, 0x00},			// GFSK mode
	{CYRF_12_DATA64_THOLD, 0x0a},	// 64 Chip Data PN Code Correlator Threshold = 10
	{CYRF_0F_XACT_CFG, 0x04},		// Transaction End State = idle
	{CYRF_39_ANALOG_CTRL, 0x01},	// synth setting time for all channels is the same as for slow channels
	{CYRF_0F_XACT_CFG, 0x24},		//Force IDLE
	{CYRF_29_RX_ABORT, 0x00},		//Clear RX abort
	{CYRF_12_DATA64_THOLD, 0x0a},	//set pn correlation threshold
	{CYRF_10_FRAMING_CFG, 0x4a},	//set sop len and threshold
	{CYRF_29_RX_ABORT, 0x0f},		//Clear RX abort?
	{CYRF_03_TX_CFG, 0x00},			// GFSK mode
	{CYRF_10_FRAMING_CFG, 0x4a},	// 0b11000000 //set sop len and threshold
	{CYRF_1F_TX_OVERRIDE, 0x04},	//disable tx CRC
	{CYRF_1E_RX_OVERRIDE, 0x14},	//disable rx crc
	{CYRF_14_EOP_CTRL, 0x00},		//set EOP sync == 0
};

uint8_t RF2500_payload_length, RF2500_tx_addr[4], RF2500_buf[80];
bool RF2500_scramble_enabled;

#define RF2500_ADDR_LENGTH 4

static void __attribute__((unused)) RF2500_Init(uint8_t payload_length, bool scramble)
{
	for(uint8_t i = 0; i < sizeof(RF2500_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&RF2500_init_vals[i][0]), pgm_read_byte_near(&RF2500_init_vals[i][1]));

	RF2500_payload_length=payload_length;
	
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, RF2500_ADDR_LENGTH + 2 + (payload_length+2)*4 );		// full payload length with CRC + address + 5 + FEC
	RF2500_scramble_enabled=scramble;
	
	CYRF_WritePreamble(0xAAAA02);
	CYRF_SetTxRxMode(TX_EN);
	RF2500_SetPower();
}

static void __attribute__((unused)) RF2500_SetTXAddr(const uint8_t* addr)
{
	memcpy(RF2500_tx_addr, addr, RF2500_ADDR_LENGTH);
}

static void __attribute__((unused)) RF2500_BuildPayload(uint8_t* buffer)
{
	const uint8_t RF2500_scramble[] = { 0xD0, 0x9E, 0x53, 0x33, 0xD8, 0xBA, 0x98, 0x08, 0x24, 0xCB, 0x3B, 0xFC, 0x71, 0xA3, 0xF4, 0x55 };
	const uint16_t RF2500_crc_xorout_scramble = 0xAEE4;

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
	uint8_t *buffer=RF2500_buf;
	uint8_t len=4 + 2 + (RF2500_payload_length+2)*4;					// Full payload length with CRC + address + 5 + FEC

	uint8_t send=len>16 ? 16 : len;
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x40);
	CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, send);			// Fill the buffer with 16 bytes max
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x80);							// Start send
	buffer += send;
	len -= send;

	while(len>8)
	{
		while((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)&0x10) == 0);	// Wait that half of the buffer is empty
		CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, 8);			// Add 8 bytes to the buffer
		buffer+=8;
		len-=8;
	}

	if(len)
	{
		while((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)&0x10) == 0);	// Wait that half of the buffer is empty
		CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, len);		// Add the remaining bytes to the buffer
	}
}

static void __attribute__((unused)) RF2500_RFChannel(uint8_t channel)
{
	CYRF_ConfigRFChannel(channel);
}

static void __attribute__((unused)) RF2500_SetPower()
{
	CYRF_SetPower(0x00);
}

#endif