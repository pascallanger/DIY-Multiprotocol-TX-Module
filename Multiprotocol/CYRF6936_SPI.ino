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
#include "iface_cyrf6936.h"

void CYRF_WriteRegister(uint8_t address, uint8_t data)
{
	CYRF_CSN_off;
	SPI_Write(0x80 | address);
	SPI_Write(data);
	CYRF_CSN_on;
}

static void CYRF_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t length)
{
	uint8_t i;

	CYRF_CSN_off;
	SPI_Write(0x80 | address);
	for(i = 0; i < length; i++)
		SPI_Write(data[i]);
	CYRF_CSN_on;
}

static void CYRF_ReadRegisterMulti(uint8_t address, uint8_t data[], uint8_t length)
{
	uint8_t i;

	CYRF_CSN_off;
	SPI_Write(address);
	for(i = 0; i < length; i++)
		data[i] = SPI_Read();
	CYRF_CSN_on;
}

uint8_t CYRF_ReadRegister(uint8_t address)
{
	uint8_t data;
	CYRF_CSN_off;
	SPI_Write(address);
	data = SPI_Read();
	CYRF_CSN_on;
	return data;
}
//

uint8_t CYRF_Reset()
{
#ifdef CYRF_RST_HI
	CYRF_RST_HI;										//Hardware reset
	delayMicroseconds(100);
	CYRF_RST_LO;
	delayMicroseconds(100);		  
#endif
	CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x01);	//Software reset
	delayMicroseconds(200);
	CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);		//Enable XOUT as GPIO
	CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);			//Enable PACTL as GPIO
	CYRF_SetTxRxMode(TXRX_OFF);
	//Verify the CYRF chip is responding
	return (CYRF_ReadRegister(CYRF_10_FRAMING_CFG) == 0xa5);
}

/*
*
*/
void CYRF_GetMfgData(uint8_t data[])
{
#ifndef FORCE_CYRF_ID
	/* Fuses power on */
	CYRF_WriteRegister(CYRF_25_MFG_ID, 0xFF);

	CYRF_ReadRegisterMulti(CYRF_25_MFG_ID, data, 6);

	/* Fuses power off */
	CYRF_WriteRegister(CYRF_25_MFG_ID, 0x00); 
#else
	memcpy(data,FORCE_CYRF_ID,6);
#endif
}

/*
* 1 - Tx else Rx
*/
void CYRF_SetTxRxMode(uint8_t mode)
{
	if(mode==TXRX_OFF)
	{
		if(protocol!=PROTO_WFLY)
			CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24); // 4=IDLE, 8=TX, C=RX
		CYRF_WriteRegister(CYRF_0E_GPIO_CTRL,0x00); // XOUT=0 PACTL=0
	}
	else
	{
		//Set the post tx/rx state
		if(protocol!=PROTO_WFLY)
			CYRF_WriteRegister(CYRF_0F_XACT_CFG, mode == TX_EN ? 0x28 : 0x2C); // 4=IDLE, 8=TX, C=RX
		if(mode == TX_EN)
#ifdef ORANGE_TX_BLUE
			CYRF_WriteRegister(CYRF_0E_GPIO_CTRL,0x20); // XOUT=1, PACTL=0
		else
			CYRF_WriteRegister(CYRF_0E_GPIO_CTRL,0x80);	// XOUT=0, PACTL=1
#else
			CYRF_WriteRegister(CYRF_0E_GPIO_CTRL,0x80); // XOUT=1, PACTL=0
		else
			CYRF_WriteRegister(CYRF_0E_GPIO_CTRL,0x20);	// XOUT=0, PACTL=1
#endif
	}
}
/*
*
*/
void CYRF_ConfigRFChannel(uint8_t ch)
{
	CYRF_WriteRegister(CYRF_00_CHANNEL,ch);
}

/*
static void CYRF_SetPower_Value(uint8_t power)
{
	uint8_t val = CYRF_ReadRegister(CYRF_03_TX_CFG) & 0xF8;
	CYRF_WriteRegister(CYRF_03_TX_CFG, val | (power & 0x07));
}
*/

void CYRF_SetPower(uint8_t val)
{
	uint8_t power=CYRF_BIND_POWER;
	if(IS_BIND_DONE)
		#ifdef CYRF6936_ENABLE_LOW_POWER
			power=IS_POWER_FLAG_on?CYRF_HIGH_POWER:CYRF_LOW_POWER;
		#else
			power=CYRF_HIGH_POWER;
		#endif
	if(IS_RANGE_FLAG_on)
		power=CYRF_RANGE_POWER;
	power|=val;
	if(prev_power != power)
	{
		CYRF_WriteRegister(CYRF_03_TX_CFG,power);
		prev_power=power;
	}
}

/*
*
*/
void CYRF_ConfigCRCSeed(uint16_t crc)
{
	CYRF_WriteRegister(CYRF_15_CRC_SEED_LSB,crc & 0xff);
	CYRF_WriteRegister(CYRF_16_CRC_SEED_MSB,crc >> 8);
}
/*
* these are the recommended sop codes from Cyrpress
* See "WirelessUSB LP/LPstar and PRoC LP/LPstar Technical Reference Manual"
*/
void CYRF_ConfigSOPCode(const uint8_t *sopcodes)
{
	//NOTE: This can also be implemented as:
	//for(i = 0; i < 8; i++) WriteRegister)0x23, sopcodes[i];
	CYRF_WriteRegisterMulti(CYRF_22_SOP_CODE, sopcodes, 8);
}

void CYRF_ConfigDataCode(const uint8_t *datacodes, uint8_t len)
{
	//NOTE: This can also be implemented as:
	//for(i = 0; i < len; i++) WriteRegister)0x23, datacodes[i];
	CYRF_WriteRegisterMulti(CYRF_23_DATA_CODE, datacodes, len);
}

void CYRF_WritePreamble(uint32_t preamble)
{
	CYRF_CSN_off;
	SPI_Write(0x80 | 0x24);
	SPI_Write(preamble & 0xff);
	SPI_Write((preamble >> 8) & 0xff);
	SPI_Write((preamble >> 16) & 0xff);
	CYRF_CSN_on;
}
/*
*
*/
/*static void CYRF_ReadDataPacket(uint8_t dpbuffer[])
{
	CYRF_ReadRegisterMulti(CYRF_21_RX_BUFFER, dpbuffer, 0x10);
}
*/
void CYRF_ReadDataPacketLen(uint8_t dpbuffer[], uint8_t length)
{
    CYRF_ReadRegisterMulti(CYRF_21_RX_BUFFER, dpbuffer, length);
}

static void CYRF_WriteDataPacketLen(const uint8_t dpbuffer[], uint8_t len)
{
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, len);
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x43);	// 0x40
	CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, dpbuffer, len);
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x83);	// 0xBF
}

void CYRF_WriteDataPacket(const uint8_t dpbuffer[])
{
	CYRF_WriteDataPacketLen(dpbuffer, 16);
}

/*static uint8_t CYRF_ReadRSSI(uint8_t dodummyread)
{
	uint8_t result;
	if(dodummyread)
		CYRF_ReadRegister(CYRF_13_RSSI);
	result = CYRF_ReadRegister(CYRF_13_RSSI);
	if(result & 0x80)
		result = CYRF_ReadRegister(CYRF_13_RSSI);
	return (result & 0x0F);
}
*/
//NOTE: This routine will reset the CRC Seed
void CYRF_FindBestChannels(uint8_t *channels, uint8_t len, uint8_t minspace, uint8_t min, uint8_t max)
{
	#define NUM_FREQ 80
	#define FREQ_OFFSET 4
	uint8_t rssi[NUM_FREQ];

	if (min < FREQ_OFFSET)
		min = FREQ_OFFSET;
	if (max > NUM_FREQ)
		max = NUM_FREQ;

	uint8_t i;
	int8_t j;
	memset(channels, 0, sizeof(uint8_t) * len);
	CYRF_ConfigCRCSeed(0x0000);
	CYRF_SetTxRxMode(RX_EN);
	//Wait for pre-amp to switch from send to receive
	delayMilliseconds(1);
	for(i = 0; i < NUM_FREQ; i++)
	{
		CYRF_ConfigRFChannel(i);
		delayMicroseconds(270);					//slow channel require 270usec for synthesizer to settle
        if( !(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80)) {
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive
            delayMicroseconds(15);
            CYRF_ReadRegister(CYRF_13_RSSI);	//dummy read
            delayMicroseconds(15);				//The conversion can occur as often as once every 12us
        }
		rssi[i] = CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;
	}

	for (i = 0; i < len; i++)
	{
		channels[i] = min;
		for (j = min; j < max; j++)
			if (rssi[j] < rssi[channels[i]])
				channels[i] = j;
		for (j = channels[i] - minspace; j < channels[i] + minspace; j++) {
			//Ensure we don't reuse any channels within minspace of the selected channel again
			if (j < 0 || j >= NUM_FREQ)
				continue;
			rssi[j] = 0xff;
		}
	}
	CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Abort RX operation
	CYRF_SetTxRxMode(TX_EN);
	CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Clear abort RX
}

#if defined(DEVO_CYRF6936_INO) || defined(J6PRO_CYRF6936_INO)
const uint8_t PROGMEM DEVO_j6pro_sopcodes[][8] = {
    /* Note these are in order transmitted (LSB 1st) */
    {0x3C, 0x37, 0xCC, 0x91, 0xE2, 0xF8, 0xCC, 0x91},
    {0x9B, 0xC5, 0xA1, 0x0F, 0xAD, 0x39, 0xA2, 0x0F},
    {0xEF, 0x64, 0xB0, 0x2A, 0xD2, 0x8F, 0xB1, 0x2A},
    {0x66, 0xCD, 0x7C, 0x50, 0xDD, 0x26, 0x7C, 0x50},
    {0x5C, 0xE1, 0xF6, 0x44, 0xAD, 0x16, 0xF6, 0x44},
    {0x5A, 0xCC, 0xAE, 0x46, 0xB6, 0x31, 0xAE, 0x46},
    {0xA1, 0x78, 0xDC, 0x3C, 0x9E, 0x82, 0xDC, 0x3C},
    {0xB9, 0x8E, 0x19, 0x74, 0x6F, 0x65, 0x18, 0x74},
    {0xDF, 0xB1, 0xC0, 0x49, 0x62, 0xDF, 0xC1, 0x49},
    {0x97, 0xE5, 0x14, 0x72, 0x7F, 0x1A, 0x14, 0x72},
#if defined(J6PRO_CYRF6936_INO)
    {0x82, 0xC7, 0x90, 0x36, 0x21, 0x03, 0xFF, 0x17},
    {0xE2, 0xF8, 0xCC, 0x91, 0x3C, 0x37, 0xCC, 0x91}, //Note: the '03' was '9E' in the Cypress recommended table
    {0xAD, 0x39, 0xA2, 0x0F, 0x9B, 0xC5, 0xA1, 0x0F}, //The following are the same as the 1st 8 above,
    {0xD2, 0x8F, 0xB1, 0x2A, 0xEF, 0x64, 0xB0, 0x2A}, //but with the upper and lower word swapped
    {0xDD, 0x26, 0x7C, 0x50, 0x66, 0xCD, 0x7C, 0x50},
    {0xAD, 0x16, 0xF6, 0x44, 0x5C, 0xE1, 0xF6, 0x44},
    {0xB6, 0x31, 0xAE, 0x46, 0x5A, 0xCC, 0xAE, 0x46},
    {0x9E, 0x82, 0xDC, 0x3C, 0xA1, 0x78, 0xDC, 0x3C},
    {0x6F, 0x65, 0x18, 0x74, 0xB9, 0x8E, 0x19, 0x74},
#endif
};
#endif
static void __attribute__((unused)) CYRF_PROGMEM_ConfigSOPCode(const uint8_t *data)
{
	uint8_t code[8];
	for(uint8_t i=0;i<8;i++)
		code[i]=pgm_read_byte_near(&data[i]);
	CYRF_ConfigSOPCode(code);
}
#endif