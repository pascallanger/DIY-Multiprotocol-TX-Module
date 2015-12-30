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
//-------------------------------
//-------------------------------
//CC2500 SPI routines
//-------------------------------
//-------------------------------
#include "iface_cc2500.h"

void cc2500_readFifo(uint8_t *dpbuffer, uint8_t len)
{
	ReadRegisterMulti(CC2500_3F_RXFIFO | CC2500_READ_BURST, dpbuffer, len);
}

//----------------------
static void ReadRegisterMulti(uint8_t address, uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	cc2500_spi_write(address);
	for(uint8_t i = 0; i < length; i++)
		data[i] = cc2500_spi_read();
	CC25_CSN_on;
}

//*********************************************

void CC2500_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	cc2500_spi_write(CC2500_WRITE_BURST | address);
	for(uint8_t i = 0; i < length; i++)
		cc2500_spi_write(data[i]);
	CC25_CSN_on;
}

void cc2500_writeFifo(uint8_t *dpbuffer, uint8_t len)
{
	cc2500_strobe(CC2500_SFTX);//0x3B
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, dpbuffer, len);
	cc2500_strobe(CC2500_STX);//0x35
}

//-------------------------------------- 
void cc2500_spi_write(uint8_t command) {
	uint8_t n=8; 

	SCK_off;//SCK start low
	SDI_off;
	while(n--)
	{
		if(command&0x80)
			SDI_on;
		else 
			SDI_off;
		SCK_on;
		NOP();
		SCK_off;
		command = command << 1;
	}
	SDI_on;
}
 
//----------------------------
void cc2500_writeReg(uint8_t address, uint8_t data) {//same as 7105
	CC25_CSN_off;
	cc2500_spi_write(address); 
	NOP();
	cc2500_spi_write(data);  
	CC25_CSN_on;
} 

uint8_t cc2500_spi_read(void)
{
	uint8_t result;
	uint8_t i;
	result=0;
	for(i=0;i<8;i++)
	{
		if(SDO_1)  ///
			result=(result<<1)|0x01;
		else
			result=result<<1;
		SCK_on;
		NOP();
		SCK_off;
		NOP();
	}
	return result;
}   
  
//--------------------------------------------
uint8_t cc2500_readReg(uint8_t address)
{ 
	uint8_t result;
	CC25_CSN_off;
	address |=0x80; //bit 7 =1 for reading
	cc2500_spi_write(address);
	result = cc2500_spi_read();  
	CC25_CSN_on;
	return(result); 
} 
//------------------------
void cc2500_strobe(uint8_t address)
{
	CC25_CSN_off;
	cc2500_spi_write(address);
	CC25_CSN_on;
}
//------------------------
void cc2500_resetChip(void)
{
	// Toggle chip select signal
	CC25_CSN_on;
	_delay_us(30);
	CC25_CSN_off;
	_delay_us(30);
	CC25_CSN_on;
	_delay_us(45);
	cc2500_strobe(CC2500_SRES);
	_delay_ms(100);
}

uint8_t CC2500_Reset()
{
	cc2500_strobe(CC2500_SRES);
	_delay_us(1000);
	CC2500_SetTxRxMode(TXRX_OFF);
	return cc2500_readReg(CC2500_0E_FREQ1) == 0xC4;//check if reset
}

void CC2500_SetPower_Value(uint8_t power)
{
	const unsigned char patable[8]=	{
		0xC5,  // -12dbm
		0x97, // -10dbm
		0x6E, // -8dbm
		0x7F, // -6dbm
		0xA9, // -4dbm
		0xBB, // -2dbm
		0xFE, // 0dbm
		0xFF // 1.5dbm
	};
	if (power > 7)
		power = 7;
	cc2500_writeReg(CC2500_3E_PATABLE,  patable[power]);
}

void CC2500_SetPower()
{
	uint8_t power=CC2500_BIND_POWER;
	if(IS_BIND_DONE_on)
		power=IS_POWER_FLAG_on?CC2500_HIGH_POWER:CC2500_LOW_POWER;
	else
		if(IS_RANGE_FLAG_on)
			power=CC2500_POWER_0;
	cc2500_writeReg(CC2500_3E_PATABLE, power);
}

void CC2500_SetTxRxMode(uint8_t mode)
{
	if(mode == TX_EN)
	{//from deviation firmware
		cc2500_writeReg(CC2500_02_IOCFG0, 0x2F | 0x40);
		cc2500_writeReg(CC2500_00_IOCFG2, 0x2F);
	}
	else
		if (mode == RX_EN)
		{
			cc2500_writeReg(CC2500_02_IOCFG0, 0x2F);
			cc2500_writeReg(CC2500_00_IOCFG2, 0x2F | 0x40);
		}
		else
		{
			cc2500_writeReg(CC2500_02_IOCFG0, 0x2F);
			cc2500_writeReg(CC2500_00_IOCFG2, 0x2F);
		}
}