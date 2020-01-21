#ifdef SX1276_INSTALLED

void SX1276_WriteReg(uint8_t address, uint8_t data)
{
	SPI_CSN_off;
	SPI_Write(address | 0x80); // MSB 1 = write
	NOP();
	SPI_Write(data);
	SPI_CSN_on;
}

uint8_t SX1276_ReadReg(uint8_t address)
{ 
	SPI_CSN_off;
	SPI_Write(address & 0b01111111);
	uint8_t result = SPI_Read();  
	SPI_CSN_on;

	return result;
}

void SX1276_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t length)
{
	SPI_CSN_off;
	SPI_Write(address | 0x80); // MSB 1 = write
	for(uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);
	SPI_CSN_on;
}

uint8_t SX1276_Reset()
{
	//TODO

    return 0;
}

#endif