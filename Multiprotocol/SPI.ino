/*SPI routines for STM32
based on arduino maple library
	By Midelic
	on RCGroups
*/


#ifdef STM32_board

SPIClass SPI_2(2); //Create an instance of the SPI Class called SPI_2 that uses the 2nd SPI Port

void initSPI2() {
  //SPI_DISABLE();
  SPI_2.end();
  SPI2_BASE->CR1 &= ~SPI_CR1_DFF_8_BIT;//8 bits format This bit should be written only when SPI is disabled (SPE = ?0?) for correct operation.
  SPI_2.begin(); //Initialize the SPI_2 port.

 SPI_2.setBitOrder(MSBFIRST); // Set the SPI_2 bit order
 SPI_2.setDataMode(SPI_MODE0); //Set the  SPI_2 data mode 0
 SPI_2.setClockDivider(SPI_CLOCK_DIV8);  //// speed (36 / 8 = 4.5 MHz SPI_2 speed)
}

#endif


#if defined STM32_board
	
void spi_write(uint8_t command){//working OK	
SPI2_BASE->DR = command;//Write the first data item to be transmitted into the SPI_DR register (this clears the TXE flag)."
while (!(SPI2_BASE->SR & SPI_SR_RXNE));
command = SPI2_BASE->DR;// "... and read the last received data."
}

uint8_t spi_read(void) {
spi_write(0x00);		
return SPI2_BASE->DR;
}

uint8_t spi_Read(){	
uint8_t rx=0;
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
return rx;
}


void SPI_ENABLE(){
SPI2_BASE->CR1 |= SPI_CR1_SPE;
}

void SPI_DISABLE(){
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

#ifdef XMEGA
#define XNOP() NOP()
#else
#define XNOP()
#endif
void spi_write(uint8_t command)
{
	uint8_t n=8; 
	SCK_off;//SCK start low
	XNOP();
	SDI_off;
	XNOP();
	do
	{
		if(command&0x80)
			SDI_on;
		else 
			SDI_off;
		XNOP() ;
		SCK_on;
		NOP();
		XNOP() ;
		XNOP() ;
		command = command << 1;		
		SCK_off;
                XNOP() ;
	}
	while(n--);
	SDI_on;
} 
   
uint8_t spi_Read(void) {
	uint8_t result=0;
	uint8_t i;

	SDI_SET_INPUT;
	for(i=0;i<8;i++) {
	     result<<=1;
		if(SDI_1)  ///if SDIO =1 
			result |= 0x01;
		SCK_on;
		NOP();
		SCK_off;
		NOP();
	}
	SDI_SET_OUTPUT;
	return result;
}


uint8_t spi_read()
{
	uint8_t result=0,i;
	for(i=0;i<8;i++)
	{                    
		result<<=1;
		if(SDO_1)  ///
			result|=0x01;
		SCK_on;
		XNOP();
		XNOP();
		NOP();
		SCK_off;
		XNOP();
		XNOP();
	}
	return result;
}
#endif
