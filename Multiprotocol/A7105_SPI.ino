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
//A7105 SPI routines
//-------------------------------
//-------------------------------
#include "iface_a7105.h"

void A7105_WriteData(uint8_t len, uint8_t channel)
{
	uint8_t i;
	CS_off;
	A7105_Write(A7105_RST_WRPTR);
	A7105_Write(0x05);
	for (i = 0; i < len; i++)
		A7105_Write(packet[i]);
	CS_on;
	A7105_WriteReg(0x0F, channel);
	A7105_Strobe(A7105_TX);
}

void A7105_ReadData() {
	uint8_t i;
	A7105_Strobe(0xF0); //A7105_RST_RDPTR
	CS_off;
	A7105_Write(0x45);
	for (i=0;i<16;i++)
		packet[i]=A7105_Read();
	CS_on;
}

void A7105_WriteReg(uint8_t address, uint8_t data) {
	CS_off;
	A7105_Write(address); 
	NOP();
	A7105_Write(data);  
	CS_on;
} 

uint8_t A7105_ReadReg(uint8_t address) { 
	uint8_t result;
	CS_off;
	A7105_Write(address |=0x40);		//bit 6 =1 for reading
	result = A7105_Read();  
	CS_on;
	return(result); 
} 

void A7105_Write(uint8_t command) {  
	uint8_t n=8; 
 
	SCK_off;//SCK start low
	SDI_off;
	while(n--) {
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

uint8_t A7105_Read(void) {
	uint8_t result=0;
	uint8_t i;

	SDI_SET_INPUT;
	for(i=0;i<8;i++) {                    
		if(SDI_1)  ///if SDIO =1 
			result=(result<<1)|0x01;
		else
			result=result<<1;
		SCK_on;
		NOP();
		SCK_off;
		NOP();
	}
	SDI_SET_OUTPUT;
	return result;
}   

//------------------------
void A7105_SetTxRxMode(uint8_t mode)
{
	if(mode == TX_EN) {
		A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
		A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x31);
	} else if (mode == RX_EN) {
		A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x31);
		A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
	} else {
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
	
	A7105_WriteReg(0x00, 0x00);
	_delay_us(1000);
	A7105_SetTxRxMode(TXRX_OFF);		//Set both GPIO as output and low
	result=A7105_ReadReg(0x10) == 0x9E;	//check if is reset.
	A7105_Strobe(A7105_STANDBY);
	return result;
}

void A7105_WriteID(uint32_t ida) {
	CS_off;
	A7105_Write(0x06);//ex id=0x5475c52a ;txid3txid2txid1txid0
	A7105_Write((ida>>24)&0xff);//53 
	A7105_Write((ida>>16)&0xff);//75
	A7105_Write((ida>>8)&0xff);//c5
	A7105_Write((ida>>0)&0xff);//2a
	CS_on;
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
	if(IS_BIND_DONE_on)
		power=IS_POWER_FLAG_on?A7105_HIGH_POWER:A7105_LOW_POWER;
	if(IS_RANGE_FLAG_on)
		power=A7105_RANGE_POWER;
	A7105_WriteReg(0x28, power);
}

void A7105_Strobe(uint8_t address) {
	CS_off;
	A7105_Write(address);
	CS_on;
}

const uint8_t PROGMEM HUBSAN_A7105_regs[] = {
	0xFF, 0x63, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF ,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x04, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2B, 0xFF, 0xFF, 0x62, 0x80, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
	0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF
};
const uint8_t PROGMEM FLYSKY_A7105_regs[] = {
	0xff, 0x42, 0x00, 0x14, 0x00, 0xff, 0xff ,0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,
	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,
	0x13, 0xc3, 0x00, 0xff, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,
	0x01, 0x0f, 0xff
};
#define ID_NORMAL 0x55201041
#define ID_PLUS   0xAA201041
void A7105_Init(uint8_t protocol)
{
	void *A7105_Regs;
	
	if(protocol==INIT_FLYSKY)
	{
		A7105_WriteID(0x5475c52A);//0x2Ac57554
		A7105_Regs=(void *)FLYSKY_A7105_regs;
	}
	else
	{
		A7105_WriteID(ID_NORMAL);
		A7105_Regs=(void *)HUBSAN_A7105_regs;
	}
	for (uint8_t i = 0; i < 0x33; i++){
		if( pgm_read_byte_near((uint16_t)(A7105_Regs)+i) != 0xFF)
			A7105_WriteReg(i, pgm_read_byte_near((uint16_t)(A7105_Regs)+i));
	}
	A7105_Strobe(A7105_STANDBY);

	//IF Filter Bank Calibration
	A7105_WriteReg(A7105_02_CALC,1);
	while(A7105_ReadReg(A7105_02_CALC));	// Wait for calibration to end
//	A7105_ReadReg(A7105_22_IF_CALIB_I);
//	A7105_ReadReg(A7105_24_VCO_CURCAL);

	if(protocol==INIT_FLYSKY)
	{
		//VCO Current Calibration
		A7105_WriteReg(A7105_24_VCO_CURCAL,0x13); //Recommended calibration from A7105 Datasheet
		//VCO Bank Calibration
		A7105_WriteReg(A7105_26_VCO_SBCAL_II,0x3b); //Recommended calibration from A7105 Datasheet
	}

	//VCO Bank Calibrate channel 0
	A7105_WriteReg(A7105_0F_CHANNEL, 0);
	A7105_WriteReg(A7105_02_CALC,2);
	while(A7105_ReadReg(A7105_02_CALC));	// Wait for calibration to end
//	A7105_ReadReg(A7105_25_VCO_SBCAL_I);
	
	//VCO Bank Calibrate channel A0
	A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
	A7105_WriteReg(A7105_02_CALC, 2);
	while(A7105_ReadReg(A7105_02_CALC));	// Wait for calibration to end
//	A7105_ReadReg(A7105_25_VCO_SBCAL_I);

	//Reset VCO Band calibration
	if(protocol==INIT_FLYSKY)
		A7105_WriteReg(A7105_25_VCO_SBCAL_I,0x08);

	A7105_SetTxRxMode(TX_EN);
	A7105_SetPower();

	A7105_Strobe(A7105_STANDBY);
}
