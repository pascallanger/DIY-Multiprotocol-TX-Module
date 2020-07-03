#ifdef SX1276_INSTALLED
#include "iface_sx1276.h"

bool SX1276_Mode_LoRa=false;

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
	SPI_Write(address & 0x7F);
	uint8_t result = SPI_Read();
	SPI_CSN_on;
	return result;
}

void SX1276_WriteRegisterMulti(uint8_t address, const uint8_t* data, uint8_t length)
{
	SPI_CSN_off;
	SPI_Write(address | 0x80); // MSB 1 = write

	for(uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);

	SPI_CSN_on;
}

void SX1276_ReadRegisterMulti(uint8_t address, uint8_t* data, uint8_t length)
{ 
	SPI_CSN_off;
	SPI_Write(address & 0x7F);

	for(uint8_t i = 0; i < length; i++)
		data[i]=SPI_Read();

	SPI_CSN_on;
}

uint8_t SX1276_Reset()
{
	//TODO when pin is not wired
	#ifdef SX1276_RST_pin
		SX1276_RST_off;
		delayMicroseconds(200);
		SX1276_RST_on;
	#endif
    return 0;
}

bool SX1276_DetectChip() //to be called after reset, verfies the chip has been detected
{
	#define SX1276_Detect_MaxAttempts 5
	uint8_t i = 0;
	bool chipFound = false;
	while ((i < SX1276_Detect_MaxAttempts) && !chipFound)
	{
		uint8_t ChipVersion = SX1276_ReadReg(SX1276_42_VERSION);
		if (ChipVersion == 0x12)
		{
			debugln("SX1276 reg version=%d", ChipVersion);
			chipFound = true;
		}
		else
		{
			debug("SX1276 not found! attempts: %d", i);
			debug(" of ");
			debugln("%d SX1276 reg version=%d", SX1276_Detect_MaxAttempts, ChipVersion);
			i++;
		}
	}
	if (!chipFound)
	{
		debugln("SX1276 not detected!!!");
		return false;
	}
	else
	{
		debugln("Found SX1276 Device!");
		return true;
	}
}

void SX1276_SetTxRxMode(uint8_t mode)
{
	#ifdef SX1276_TXEN_pin
		if(mode == TX_EN)
			SX1276_TXEN_on;
		else
			SX1276_RXEN_on;
	#endif
}

void SX1276_SetFrequency(uint32_t frequency)
{
	uint32_t f = frequency / 61;
	uint8_t data[3];
	data[0] = f >> 16;
	data[1] = f >> 8;
	data[2] = f;
	
	SX1276_WriteRegisterMulti(SX1276_06_FRFMSB, data, 3);
}

void SX1276_SetMode(bool lora, bool low_freq_mode, uint8_t mode)
{
	uint8_t data = 0x00;

	SX1276_Mode_LoRa=lora;

	if(lora)
	{
		data = data | (1 << 7);
		data = data & ~(1 << 6);
	}
	else
	{
		data = data & ~(1 << 7);
		data = data | (1 << 6);
	}

	if(low_freq_mode)
		data = data | (1 << 3);
	
	data = data | mode;

	SX1276_WriteReg(SX1276_01_OPMODE, data);
}

void SX1276_SetDetectOptimize(bool auto_if, uint8_t detect_optimize)
{
	uint8_t data = SX1276_ReadReg(SX1276_31_DETECTOPTIMIZE);
	data = (data & 0b01111000) | detect_optimize;
	data = data | (auto_if << 7);

	SX1276_WriteReg(SX1276_31_DETECTOPTIMIZE, data);
}

void SX1276_ConfigModem1(uint8_t bandwidth, uint8_t coding_rate, bool implicit_header_mode)
{
	uint8_t data = 0x00;
	data = data | (bandwidth << 4);
	data = data | (coding_rate << 1);
	data = data | implicit_header_mode;

	SX1276_WriteReg(SX1276_1D_MODEMCONFIG1, data);

	if (bandwidth == SX1276_MODEM_CONFIG1_BW_500KHZ) //datasheet errata reconmendation http://caxapa.ru/thumbs/972894/SX1276_77_8_ErrataNote_1.1_STD.pdf
	{
		SX1276_WriteReg(SX1276_36_LORA_REGHIGHBWOPTIMIZE1, 0x02);
		SX1276_WriteReg(SX1276_3A_LORA_REGHIGHBWOPTIMIZE2, 0x64);
	}
	else
		SX1276_WriteReg(SX1276_36_LORA_REGHIGHBWOPTIMIZE1, 0x03);
}

void SX1276_ConfigModem2(uint8_t spreading_factor, bool tx_continuous_mode, bool rx_payload_crc_on)
{
	uint8_t data = SX1276_ReadReg(SX1276_1E_MODEMCONFIG2);
	data = data & 0b11; // preserve the last 2 bits
	data = data | (spreading_factor << 4);
	data = data | (tx_continuous_mode << 3);
	data = data | (rx_payload_crc_on << 2);

	SX1276_WriteReg(SX1276_1E_MODEMCONFIG2, data);
}

void SX1276_ConfigModem3(bool low_data_rate_optimize, bool agc_auto_on)
{
	uint8_t data = SX1276_ReadReg(SX1276_26_MODEMCONFIG3);
	data = data & 0b11; // preserve the last 2 bits
	data = data | (low_data_rate_optimize << 3);
	data = data | (agc_auto_on << 2);
	
	SX1276_WriteReg(SX1276_26_MODEMCONFIG3, data);
}

void SX1276_SetPreambleLength(uint16_t length)
{
	uint8_t data[2];
	data[0] = (length >> 8) & 0xFF; // high byte
	data[1] = length & 0xFF; // low byte
	
	SX1276_WriteRegisterMulti(SX1276_20_PREAMBLEMSB, data, 2);
}

void SX1276_SetDetectionThreshold(uint8_t threshold)
{
	SX1276_WriteReg(SX1276_37_DETECTIONTHRESHOLD, threshold);
}

void SX1276_SetLna(uint8_t gain, bool high_freq_lna_boost)
{
	uint8_t data = SX1276_ReadReg(SX1276_0C_LNA);
	data = data & 0b100; // preserve the third bit
	data = data | (gain << 5);
	
	if(high_freq_lna_boost)
		data = data | 0b11;
	
	SX1276_WriteReg(SX1276_0C_LNA, data);
}

void SX1276_SetHopPeriod(uint8_t freq_hop_period)
{
	SX1276_WriteReg(SX1276_24_HOPPERIOD, freq_hop_period);
}

void SX1276_SetPaDac(bool on)
{
	uint8_t data = SX1276_ReadReg(SX1276_4D_PADAC);
	data = data & 0b11111000; // preserve the upper 5 bits

	if(on)
		data = data | 0x07;
	else
		data = data | 0x04;
	
	SX1276_WriteReg(SX1276_4D_PADAC, data);
}

void SX1276_SetPaConfig(bool pa_boost_pin, uint8_t max_power, uint8_t output_power)
{
	uint8_t data = 0x00;
	data = data | (pa_boost_pin << 7);
	data = data | (max_power << 4);
	data = data | output_power;

	SX1276_WriteReg(SX1276_09_PACONFIG, data);
}

void SX1276_WritePayloadToFifo(uint8_t* payload, uint8_t length)
{
	SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, length);
	SX1276_WriteReg(SX1276_0E_FIFOTXBASEADDR, 0x00);
	SX1276_WriteReg(SX1276_0D_FIFOADDRPTR, 0x00);
	SX1276_WriteRegisterMulti(SX1276_00_FIFO, payload, length);
}

#endif