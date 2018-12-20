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
/** A7105 routines **/
/********************/
#ifdef A7105_INSTALLED
#include "iface_a7105.h"

void A7105_WriteData(uint8_t len, uint8_t channel)
{
	uint8_t i;
	A7105_CSN_off;
	SPI_Write(A7105_RST_WRPTR);
	SPI_Write(A7105_05_FIFO_DATA);
	for (i = 0; i < len; i++)
		SPI_Write(packet[i]);
	A7105_CSN_on;
	if(protocol!=PROTO_FLYSKY)
	{
		A7105_Strobe(A7105_STANDBY);	//Force standby mode, ie cancel any TX or RX...
		A7105_SetTxRxMode(TX_EN);		//Switch to PA
	}
	A7105_WriteReg(A7105_0F_PLL_I, channel);
	A7105_Strobe(A7105_TX);
}

void A7105_ReadData(uint8_t len)
{
	uint8_t i;
	A7105_Strobe(A7105_RST_RDPTR);
	A7105_CSN_off;
	SPI_Write(0x40 | A7105_05_FIFO_DATA);	//bit 6 =1 for reading
	for (i=0;i<len;i++)
		packet[i]=SPI_SDI_Read();
	A7105_CSN_on;
}

void A7105_WriteReg(uint8_t address, uint8_t data) {
	A7105_CSN_off;
	SPI_Write(address); 
	NOP();
	SPI_Write(data);  
	A7105_CSN_on;
} 

uint8_t A7105_ReadReg(uint8_t address)
{ 
	uint8_t result;
	A7105_CSN_off;
	SPI_Write(address |=0x40);				//bit 6 =1 for reading
	result = SPI_SDI_Read();  
	A7105_CSN_on;
	return(result); 
} 

//------------------------
void A7105_SetTxRxMode(uint8_t mode)
{
	if(mode == TX_EN)
	{
		A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
		A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x31);
	}
	else
		if (mode == RX_EN)
		{
			A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x31);
			A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
		}
		else
		{
			//The A7105 seems to some with a cross-wired power-amp (A7700)
			//On the XL7105-D03, TX_EN -> RXSW and RX_EN -> TXSW
			//This means that sleep mode is wired as RX_EN = 1 and TX_EN = 1
			//If there are other amps in use, we'll need to fix this
			A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
			A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
		}
}

//------------------------
uint8_t A7105_Reset()
{
	uint8_t result;
	
	A7105_WriteReg(A7105_00_MODE, 0x00);
	delayMilliseconds(1);
	A7105_SetTxRxMode(TXRX_OFF);			//Set both GPIO as output and low
	result=A7105_ReadReg(A7105_10_PLL_II) == 0x9E;	//check if is reset.
	A7105_Strobe(A7105_STANDBY);
	return result;
}

void A7105_WriteID(uint32_t ida)
{
	A7105_CSN_off;
	SPI_Write(A7105_06_ID_DATA);			//ex id=0x5475c52a ;txid3txid2txid1txid0
	SPI_Write((ida>>24)&0xff);				//53 
	SPI_Write((ida>>16)&0xff);				//75
	SPI_Write((ida>>8)&0xff);				//c5
	SPI_Write((ida>>0)&0xff);				//2a
	A7105_CSN_on;
}

/*
static void A7105_SetPower_Value(int power)
{
	//Power amp is ~+16dBm so:
	//TXPOWER_100uW  = -23dBm == PAC=0 TBG=0
	//TXPOWER_300uW  = -20dBm == PAC=0 TBG=1
	//TXPOWER_1mW    = -16dBm == PAC=0 TBG=2
	//TXPOWER_3mW    = -11dBm == PAC=0 TBG=4
	//TXPOWER_10mW   = -6dBm  == PAC=1 TBG=5
	//TXPOWER_30mW   = 0dBm   == PAC=2 TBG=7
	//TXPOWER_100mW  = 1dBm   == PAC=3 TBG=7
	//TXPOWER_150mW  = 1dBm   == PAC=3 TBG=7
	uint8_t pac, tbg;
	switch(power) {
		case 0: pac = 0; tbg = 0; break;
		case 1: pac = 0; tbg = 1; break;
		case 2: pac = 0; tbg = 2; break;
		case 3: pac = 0; tbg = 4; break;
		case 4: pac = 1; tbg = 5; break;
		case 5: pac = 2; tbg = 7; break;
		case 6: pac = 3; tbg = 7; break;
		case 7: pac = 3; tbg = 7; break;
		default: pac = 0; tbg = 0; break;
	};
	A7105_WriteReg(0x28, (pac << 3) | tbg);
}
*/

void A7105_SetPower()
{
	uint8_t power=A7105_BIND_POWER;
	if(IS_BIND_DONE)
		#ifdef A7105_ENABLE_LOW_POWER
			power=IS_POWER_FLAG_on?A7105_HIGH_POWER:A7105_LOW_POWER;
		#else
			power=A7105_HIGH_POWER;
		#endif
	if(IS_RANGE_FLAG_on)
		power=A7105_RANGE_POWER;
	if(prev_power != power)
	{
		A7105_WriteReg(A7105_28_TX_TEST, power);
		prev_power=power;
	}
}

void A7105_Strobe(uint8_t address) {
	A7105_CSN_off;
	SPI_Write(address);
	A7105_CSN_on;
}

// Fine tune A7105 LO base frequency
// this is required for some A7105 modules and/or RXs with inaccurate crystal oscillator
void A7105_AdjustLOBaseFreq(uint8_t cmd)
{
	static int16_t old_offset=2048;
	int16_t offset=1024;
	if(cmd==0)
	{	// Called at init of the A7105
		old_offset=2048;
		switch(protocol)
		{
			case PROTO_HUBSAN:
				#ifdef FORCE_HUBSAN_TUNING
					offset=(int16_t)FORCE_HUBSAN_TUNING;
				#endif
				break;
			case PROTO_BUGS:
				#ifdef FORCE_HUBSAN_TUNING
					offset=(int16_t)FORCE_HUBSAN_TUNING;
				#endif
				break;
			case PROTO_FLYSKY:
				#ifdef FORCE_FLYSKY_TUNING
					offset=(int16_t)FORCE_FLYSKY_TUNING;
				#endif
				break;
			case PROTO_AFHDS2A:
				#ifdef FORCE_AFHDS2A_TUNING
					offset=(int16_t)FORCE_AFHDS2A_TUNING;
				#endif
				break;
		}
	}
	if(offset==1024)	// Use channel 15 as an input
		offset=convert_channel_16b_nolimit(CH15,-300,300);

	if(old_offset==offset)	// offset is the same as before...
			return;
	old_offset=offset;

	// LO base frequency = 32e6*(bip+(bfp/(2^16)))
	uint8_t bip;	// LO base frequency integer part
	uint16_t bfp;	// LO base frequency fractional part
	offset++;		// as per datasheet, not sure why recommended, but that's a +1kHz drift only ...
	offset<<=1;
	if(offset < 0)
	{
		bip = 0x4a;	// 2368 MHz
		bfp = 0xffff + offset;
	}
	else
	{
		bip = 0x4b;	// 2400 MHz (default)
		bfp = offset;
	}
	A7105_WriteReg( A7105_11_PLL_III, bip);
	A7105_WriteReg( A7105_12_PLL_IV, (bfp >> 8) & 0xff);
	A7105_WriteReg( A7105_13_PLL_V, bfp & 0xff);
	//debugln("Channel: %d, offset: %d, bip: %2x, bfp: %4x", Channel_data[14], offset, bip, bfp);
}

static void __attribute__((unused)) A7105_SetVCOBand(uint8_t vb1, uint8_t vb2)
{	// Set calibration band value to best match
	uint8_t diff1, diff2;

	if (vb1 >= 4)
		diff1 = vb1 - 4;
	else
		diff1 = 4 - vb1;

	if (vb2 >= 4)
		diff2 = vb2 - 4;
	else
		diff2 = 4 - vb2;

	if (diff1 == diff2 || diff1 > diff2)
		A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb1 | 0x08);
	else
		A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb2 | 0x08);
}

#ifdef HUBSAN_A7105_INO
const uint8_t PROGMEM HUBSAN_A7105_regs[] = {
	0xFF, 0x63, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF ,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x04, 0xFF,	// 00 - 0f
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2B, 0xFF, 0xFF, 0x62, 0x80, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,	// 10 - 1f
	0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 20 - 2f
	0xFF, 0xFF // 30 - 31
};
#endif
#ifdef FLYSKY_A7105_INO
const uint8_t PROGMEM FLYSKY_A7105_regs[] = {
	0xff, 0x42, 0x00, 0x14, 0x00, 0xff, 0xff ,0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,	// 00 - 0f
	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,	// 10 - 1f
	0x13, 0xc3, 0x00, 0xff, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	// 20 - 2f
	0x01, 0x0f // 30 - 31
};
#endif
#ifdef AFHDS2A_A7105_INO
const uint8_t PROGMEM AFHDS2A_A7105_regs[] = {
	0xFF, 0x42 | (1<<5), 0x00, 0x25, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3c, 0x05, 0x00, 0x50,	// 00 - 0f
	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80, 0xFF, 0xFF, 0x2a, 0x32, 0xc3, 0x1f,				// 10 - 1f
	0x1e, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,				// 20 - 2f
	0x01, 0x0f // 30 - 31
};
#endif
#ifdef BUGS_A7105_INO
const uint8_t PROGMEM BUGS_A7105_regs[] = {
	0xFF, 0x42, 0x00, 0x15, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0x01, 0x50,	// 00 - 0f
	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x40, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,	// 10 - 1f
	0x16, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x0b, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	// 20 - 2f
	0x01, 0x0f // 30 - 31
};
#endif

#define ID_NORMAL 0x55201041
#define ID_PLUS   0xAA201041
void A7105_Init(void)
{
	uint8_t *A7105_Regs=0;
    uint8_t vco_calibration0, vco_calibration1;
	
	#ifdef BUGS_A7105_INO
		if(protocol==PROTO_BUGS)
			A7105_Regs=(uint8_t*)BUGS_A7105_regs;
		else
	#endif
	#ifdef HUBSAN_A7105_INO
		if(protocol==PROTO_HUBSAN)
		{
			A7105_WriteID(ID_NORMAL);
			A7105_Regs=(uint8_t*)HUBSAN_A7105_regs;
		}
		else
	#endif
		{
			A7105_WriteID(0x5475c52A);//0x2Ac57554
			#ifdef FLYSKY_A7105_INO
				if(protocol==PROTO_FLYSKY)
					A7105_Regs=(uint8_t*)FLYSKY_A7105_regs;
				else
			#endif
				{
					#ifdef AFHDS2A_A7105_INO
						A7105_Regs=(uint8_t*)AFHDS2A_A7105_regs;
					#endif
				}
		}

	for (uint8_t i = 0; i < 0x32; i++)
	{
		uint8_t val=pgm_read_byte_near(&A7105_Regs[i]);
		#ifdef FLYSKY_A7105_INO
			if(protocol==PROTO_FLYSKY && sub_protocol==CX20)
			{
				if(i==0x0E) val=0x01;
				if(i==0x1F) val=0x1F;
				if(i==0x20) val=0x1E;
			}
		#endif
		if( val != 0xFF)
			A7105_WriteReg(i, val);
	}
	A7105_Strobe(A7105_STANDBY);

	//IF Filter Bank Calibration
	A7105_WriteReg(A7105_02_CALC,1);
	while(A7105_ReadReg(A7105_02_CALC));			// Wait for calibration to end
//	A7105_ReadReg(A7105_22_IF_CALIB_I);
//	A7105_ReadReg(A7105_24_VCO_CURCAL);

	if(protocol!=PROTO_HUBSAN)
	{
		//VCO Current Calibration
		A7105_WriteReg(A7105_24_VCO_CURCAL,0x13);	//Recommended calibration from A7105 Datasheet
		//VCO Bank Calibration
		A7105_WriteReg(A7105_26_VCO_SBCAL_II,0x3b);	//Recommended calibration from A7105 Datasheet
	}

	//VCO Bank Calibrate channel 0
	A7105_WriteReg(A7105_0F_CHANNEL, 0);
	A7105_WriteReg(A7105_02_CALC,2);
	while(A7105_ReadReg(A7105_02_CALC));			// Wait for calibration to end
	vco_calibration0 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);
	
	//VCO Bank Calibrate channel A0
	A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
	A7105_WriteReg(A7105_02_CALC, 2);
	while(A7105_ReadReg(A7105_02_CALC));			// Wait for calibration to end
	vco_calibration1 = A7105_ReadReg(A7105_25_VCO_SBCAL_I);

	if(protocol==PROTO_BUGS)
		A7105_SetVCOBand(vco_calibration0 & 0x07, vco_calibration1 & 0x07);	// Set calibration band value to best match
	else
		if(protocol!=PROTO_HUBSAN)
			A7105_WriteReg(A7105_25_VCO_SBCAL_I,protocol==PROTO_FLYSKY?0x08:0x0A);	//Reset VCO Band calibration

	A7105_SetTxRxMode(TX_EN);
	A7105_SetPower();

	A7105_AdjustLOBaseFreq(0);
	
	A7105_Strobe(A7105_STANDBY);
}
#endif