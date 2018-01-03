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
/********************/
/**  SPI routines  **/
/********************/
#ifdef STM32_BOARD

#ifdef DEBUG_SERIAL
//	#define DEBUG_SPI
#endif

SPIClass SPI_2(2); 								//Create an instance of the SPI Class called SPI_2 that uses the 2nd SPI Port

void initSPI2()
{
	//SPI_DISABLE();
	SPI_2.end();
	SPI2_BASE->CR1 &= ~SPI_CR1_DFF_8_BIT;		//8 bits format, this bit should be written only when SPI is disabled (SPE = ?0?) for correct operation.
	SPI_2.begin();								//Initialize the SPI_2 port.

	SPI2_BASE->CR1 &= ~SPI_CR1_LSBFIRST;		// Set the SPI_2 bit order MSB first
	SPI2_BASE->CR1 &= ~(SPI_CR1_CPOL|SPI_CR1_CPHA);	// Set the SPI_2 data mode 0: Clock idles low, data captured on rising edge (first transition)
	SPI2_BASE->CR1 &= ~(SPI_CR1_BR);
	SPI2_BASE->CR1 |= SPI_CR1_BR_PCLK_DIV_8;	// Set the speed (36 / 8 = 4.5 MHz SPI_2 speed) SPI_CR1_BR_PCLK_DIV_8
}
	
void SPI_Write(uint8_t command)
{//working OK	
	SPI2_BASE->DR = command;					//Write the first data item to be transmitted into the SPI_DR register (this clears the TXE flag).
	#ifdef DEBUG_SPI
		debug("%02X ",command);
	#endif
	while (!(SPI2_BASE->SR & SPI_SR_RXNE));
	command = SPI2_BASE->DR;					// ... and read the last received data.
}

uint8_t SPI_Read(void)
{
	SPI_Write(0x00);		
	return SPI2_BASE->DR;
}

uint8_t SPI_SDI_Read()
{	
	uint8_t rx=0;
	cli();	//Fix Hubsan droputs??
	while(!(SPI2_BASE->SR & SPI_SR_TXE));
	while((SPI2_BASE->SR & SPI_SR_BSY));	
	//	
	SPI_DISABLE();
	SPI_SET_BIDIRECTIONAL();
	volatile uint8_t x = SPI2_BASE->DR;
	(void)x;
	SPI_ENABLE();
	//
	SPI_DISABLE();				  
	while(!(SPI2_BASE->SR& SPI_SR_RXNE));
	rx=SPI2_BASE->DR;
	SPI_SET_UNIDIRECTIONAL();
	SPI_ENABLE();
	sei();//fix Hubsan dropouts??
	return rx;
}

void SPI_ENABLE()
{
	SPI2_BASE->CR1 |= SPI_CR1_SPE;
}

void SPI_DISABLE()
{
	SPI2_BASE->CR1 &= ~SPI_CR1_SPE;
}

void SPI_SET_BIDIRECTIONAL()
{
	SPI2_BASE->CR1 |= SPI_CR1_BIDIMODE;
	SPI2_BASE->CR1  &= ~ SPI_CR1_BIDIOE;//receive only
}

void SPI_SET_UNIDIRECTIONAL()
{
	SPI2_BASE->CR1 &= ~SPI_CR1_BIDIMODE;
}

#else

#ifdef ORANGE_TX
	#define XNOP() NOP()
#else
	#define XNOP()
#endif

void SPI_Write(uint8_t command)
{
	uint8_t n=8; 

	SCLK_off;//SCK start low
	XNOP();
	SDI_off;
	XNOP();
	do
	{
		if(command&0x80)
			SDI_on;
		else
			SDI_off;
		XNOP();
		SCLK_on;
		XNOP();
		XNOP();
		command = command << 1;
		SCLK_off;
		XNOP();
	}
	while(--n) ;
	SDI_on;
}

uint8_t SPI_Read(void)
{
	uint8_t result=0,i;
	for(i=0;i<8;i++)
	{
		result=result<<1;
		if(SDO_1)
			result |= 0x01;
		SCLK_on;
		XNOP();
		XNOP();
		NOP();
		SCLK_off;
		XNOP();
		XNOP();
	}
	return result;
}

#ifdef A7105_INSTALLED
uint8_t SPI_SDI_Read(void)
{
	uint8_t result=0;
	SDI_input;
	for(uint8_t i=0;i<8;i++)
	{                    
		result=result<<1;
		if(SDI_1)  ///if SDIO =1 
			result |= 0x01;
		SCLK_on;
		NOP();
		SCLK_off;
	}
	SDI_output;
	return result;
}
#endif

#endif//STM32_BOARD