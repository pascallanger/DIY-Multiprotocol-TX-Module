
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
#ifdef CC2500_INSTALLED
#include "iface_cc2500.h"

//----------------------------
void CC2500_WriteReg(uint8_t address, uint8_t data)
{
	CC25_CSN_off;
	SPI_Write(address); 
	NOP();
	SPI_Write(data);
	CC25_CSN_on;
} 

//----------------------
static void CC2500_ReadRegisterMulti(uint8_t address, uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	SPI_Write(CC2500_READ_BURST | address);
	for(uint8_t i = 0; i < length; i++)
		data[i] = SPI_Read();
	CC25_CSN_on;
}

//--------------------------------------------
static uint8_t CC2500_ReadReg(uint8_t address)
{ 
	uint8_t result;
	CC25_CSN_off;
	SPI_Write(CC2500_READ_SINGLE | address);
	result = SPI_Read();  
	CC25_CSN_on;
	return(result); 
} 

//------------------------
void CC2500_ReadData(uint8_t *dpbuffer, uint8_t len)
{
	CC2500_ReadRegisterMulti(CC2500_3F_RXFIFO, dpbuffer, len);
}

//*********************************************
void CC2500_Strobe(uint8_t state)
{
	CC25_CSN_off;
	SPI_Write(state);
	CC25_CSN_on;
}

static void CC2500_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	SPI_Write(CC2500_WRITE_BURST | address);
	for(uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);
	CC25_CSN_on;
}

void CC2500_WriteData(uint8_t *dpbuffer, uint8_t len)
{
	CC2500_Strobe(CC2500_SFTX);
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, dpbuffer, len);
	CC2500_Strobe(CC2500_STX);
}

void CC2500_SetTxRxMode(uint8_t mode)
{
	if(mode == TX_EN)
	{//from deviation firmware
		CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
		CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F | 0x40);
	}
	else
		if (mode == RX_EN)
		{
			CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
			CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F | 0x40);
		}
		else
		{
			CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
			CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
		}
}

//------------------------
/*static void cc2500_resetChip(void)
{
	// Toggle chip select signal
	CC25_CSN_on;
	delayMicroseconds(30);
	CC25_CSN_off;
	delayMicroseconds(30);
	CC25_CSN_on;
	delayMicroseconds(45);
	CC2500_Strobe(CC2500_SRES);
	_delay_ms(100);
}
*/
uint8_t CC2500_Reset()
{
	CC2500_Strobe(CC2500_SRES);
	delayMilliseconds(1);
	CC2500_SetTxRxMode(TXRX_OFF);
	return CC2500_ReadReg(CC2500_0E_FREQ1) == 0xC4;//check if reset
}
/*
static void CC2500_SetPower_Value(uint8_t power)
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
	CC2500_WriteReg(CC2500_3E_PATABLE,  patable[power]);
}
*/
void CC2500_SetPower()
{
	uint8_t power=CC2500_BIND_POWER;
	if(IS_BIND_DONE)
		#ifdef CC2500_ENABLE_LOW_POWER
			power=IS_POWER_FLAG_on?CC2500_HIGH_POWER:CC2500_LOW_POWER;
		#else
			power=CC2500_HIGH_POWER;
		#endif
	if(IS_LBT_POWER_on)
	{
		power=CC2500_LBT_POWER;
		LBT_POWER_off;			// Only accept once
	}
	if(IS_RANGE_FLAG_on)
		power=CC2500_RANGE_POWER;
	if(prev_power != power)
	{
		CC2500_WriteReg(CC2500_3E_PATABLE, power);
		prev_power=power;
	}
}

void __attribute__((unused)) CC2500_SetFreqOffset()
{
	if(prev_option != option)
	{
		prev_option = option;
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	}
}

void __attribute__((unused)) CC2500_250K_Init()
{
	CC2500_Strobe(CC2500_SIDLE);

	// Address Config = No address check
	// Base Frequency = 2400
	// CRC Autoflush = false
	// CRC Enable = false
	// Channel Spacing = 333.251953
	// Data Format = Normal mode
	// Data Rate = 249.939
	// Deviation = 126.953125
	// Device Address = 0
	// Manchester Enable = false
	// Modulated = true
	// Modulation Format = GFSK
	// Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
	// RX Filter BW = 203.125000
	// Sync Word Qualifier Mode = No preamble/sync
	// TX Power = 0
	// Whitening = false
	// Fast Frequency Hopping - no PLL auto calibration
	CC2500_WriteReg(CC2500_08_PKTCTRL0,	0x01);   // Packet Automation Control
	CC2500_WriteReg(CC2500_0B_FSCTRL1,	0x0A);   // Frequency Synthesizer Control
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);  // Frequency offset hack 
	CC2500_WriteReg(CC2500_0D_FREQ2,	0x5C);   // Frequency Control Word, High Byte
	CC2500_WriteReg(CC2500_0E_FREQ1,	0x4E);   // Frequency Control Word, Middle Byte
	CC2500_WriteReg(CC2500_0F_FREQ0,	0xC3);   // Frequency Control Word, Low Byte
	CC2500_WriteReg(CC2500_10_MDMCFG4,	0x8D);   // Modem Configuration
	CC2500_WriteReg(CC2500_11_MDMCFG3,	0x3B);   // Modem Configuration
	CC2500_WriteReg(CC2500_12_MDMCFG2,	0x10);   // Modem Configuration
	CC2500_WriteReg(CC2500_13_MDMCFG1,	0x23);   // Modem Configuration
	CC2500_WriteReg(CC2500_14_MDMCFG0,	0xA4);   // Modem Configuration
	CC2500_WriteReg(CC2500_15_DEVIATN,	0x62);   // Modem Deviation Setting
	CC2500_WriteReg(CC2500_18_MCSM0,	0x08);   // Main Radio Control State Machine Configuration
	CC2500_WriteReg(CC2500_19_FOCCFG,	0x1D);   // Frequency Offset Compensation Configuration
	CC2500_WriteReg(CC2500_1A_BSCFG,	0x1C);   // Bit Synchronization Configuration
	CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);   // AGC Control
	CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
	CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xB0);   // AGC Control
	CC2500_WriteReg(CC2500_21_FREND1,	0xB6);   // Front End RX Configuration
	CC2500_WriteReg(CC2500_23_FSCAL3,	0xEA);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_25_FSCAL1,	0x00);   // Frequency Synthesizer Calibration
	CC2500_WriteReg(CC2500_26_FSCAL0,	0x11);   // Frequency Synthesizer Calibration

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}
void __attribute__((unused)) CC2500_250K_HoppingCalib(uint8_t num_freq)
{
	for (uint8_t i = 0; i < num_freq; i++)
	{
		CC2500_Strobe(CC2500_SIDLE);
		// spacing is 333.25 kHz, must multiply channel by 3
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[i]*3);
		// calibrate
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[i]=CC2500_ReadReg(CC2500_25_FSCAL1);
	}
}
void __attribute__((unused)) CC2500_250K_Hopping(uint8_t index)
{
	// spacing is 333.25 kHz, must multiply channel by 3
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[index] * 3);
	// set PLL calibration
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[index]);
}
void __attribute__((unused)) CC2500_250K_RFChannel(uint8_t number)
{
	CC2500_Strobe(CC2500_SIDLE);
	// spacing is 333.25 kHz, must multiply channel by 3
	CC2500_WriteReg(CC2500_0A_CHANNR, number*3);
	// calibrate
	CC2500_Strobe(CC2500_SCAL);
	delayMicroseconds(900);
}

//NRF emulation layer with CRC16 enabled
uint8_t cc2500_nrf_tx_addr[5], cc2500_nrf_addr_len;
void __attribute__((unused)) CC2500_250K_NRF_SetTXAddr(uint8_t* addr, uint8_t len)
{
	cc2500_nrf_addr_len = len;
	memcpy(cc2500_nrf_tx_addr, addr, len);
}
void __attribute__((unused)) CC2500_250K_NRF_WritePayload(uint8_t* msg, uint8_t len)
{
	#if defined(ESKY150V2_CC2500_INO)
		uint8_t buf[158];
	#else
		uint8_t buf[35];
	#endif
	uint8_t last = 0;
	uint8_t i;

	//nrf preamble
	if(cc2500_nrf_tx_addr[cc2500_nrf_addr_len - 1] & 0x80)
		buf[0]=0xAA;
	else
		buf[0]=0x55;
	last++;
	// address
	for (i = 0; i < cc2500_nrf_addr_len; ++i)
		buf[last++] = cc2500_nrf_tx_addr[cc2500_nrf_addr_len - i - 1];
	// payload
	for (i = 0; i < len; ++i)
		buf[last++] = msg[i];

	// crc
	crc = 0xffff;
	for (uint8_t i = 1; i < last; ++i)
		crc16_update( buf[i], 8);
	buf[last++] = crc >> 8;
	buf[last++] = crc & 0xff;
	buf[last++] = 0;

	//for(uint8_t i=0;i<last;i++)
	//	debug("%02X ",buf[i]);
	//debugln("");
	// stop TX/RX
	CC2500_Strobe(CC2500_SIDLE);
	// flush tx FIFO
	CC2500_Strobe(CC2500_SFTX);
	// packet length
	CC2500_WriteReg(CC2500_3F_TXFIFO, last);
	// transmit nrf packet
	uint8_t *buff=buf;
	uint8_t status;
	if(last>63)
	{
		CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 63);
		CC2500_Strobe(CC2500_STX);
		last-=63;
		buff+=63;
		while(last)
		{//Loop until all the data is sent
			do
			{// Wait for the FIFO to become available
				status=CC2500_ReadReg(CC2500_3A_TXBYTES | CC2500_READ_BURST);
			}
			while((status&0x7F)>31 && (status&0x80)==0);
			if(last>31)
			{//Send 31 bytes
				CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 31);
				last-=31;
				buff+=31;
			}
			else
			{//Send last bytes
				CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
				last=0;
			}
		}
	}
	else
	{//Send packet
		CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
		CC2500_Strobe(CC2500_STX);
	}
}
#endif