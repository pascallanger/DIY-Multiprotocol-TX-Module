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

/******************************/
/**  FrSky D and X routines  **/
/******************************/

#if defined(FRSKYX_CC2500_INO) || defined(FRSKYL_CC2500_INO) || defined(FRSKY_RX_CC2500_INO) || defined(FRSKYR9_SX1276_INO)
//**CRC**
const uint16_t PROGMEM FrSkyX_CRC_Short[]={
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7 };
static uint16_t __attribute__((unused)) FrSkyX_CRCTable(uint8_t val)
{
	uint16_t word ;
	word = pgm_read_word(&FrSkyX_CRC_Short[val&0x0F]) ;
	val /= 16 ;
	return word ^ (0x1081 * val) ;
}
uint16_t FrSkyX_crc(uint8_t *data, uint8_t len, uint8_t init=0)
{
	uint16_t crc = init;
	for(uint8_t i=0; i < len; i++)
		crc = (crc<<8) ^ FrSkyX_CRCTable((uint8_t)(crc>>8) ^ *data++);
	return crc;
}
#endif


#if defined(FRSKYD_CC2500_INO) || defined(FRSKYX_CC2500_INO) || defined(FRSKYX_CC2500_INO) || defined(FRSKYL_CC2500_INO) || defined(FRSKY_RX_CC2500_INO)
enum {
	FRSKY_BIND		= 0,
	FRSKY_BIND_DONE	= 1000,
	FRSKY_DATA1,
	FRSKY_DATA2,
	FRSKY_DATA3,
	FRSKY_DATA4,
	FRSKY_DATA5,
};

uint8_t FrSkyFormat=0;

void Frsky_init_hop(void)
{
	uint8_t val;
	uint8_t channel = rx_tx_addr[0]&0x07;
	uint8_t channel_spacing = rx_tx_addr[1];
	//Filter bad tables
	if(channel_spacing<0x02) channel_spacing+=0x02;
	if(channel_spacing>0xE9) channel_spacing-=0xE7;
	if(channel_spacing%0x2F==0) channel_spacing++;
		
	hopping_frequency[0]=channel;
	for(uint8_t i=1;i<50;i++)
	{
		channel=(channel+channel_spacing) % 0xEB;
		val=channel;
		if((val==0x00) || (val==0x5A) || (val==0xDC))
			val++;
		hopping_frequency[i]=i>46?0:val;
	}
}

void FrSkyX2_init_hop(void)
{
	uint16_t id=(rx_tx_addr[2]<<8) + rx_tx_addr[3];
	//Increment
	uint8_t inc = (id % 46) + 1;
	if( inc == 12 || inc ==35 ) inc++;							//Exception list from dumps
	//Start offset
	uint8_t offset = id % 5;

	debug("hop: ");
	uint8_t channel;
	for(uint8_t i=0; i<47; i++)
	{
		channel = 5 * (uint16_t(inc * i) % 47) + offset;
		//Exception list from dumps
		if(FrSkyFormat & 2 )// LBT or FCC
		{//LBT
			if( channel <=1 || channel == 43 || channel == 44 || channel == 87 || channel == 88 || channel == 129 || channel == 130 || channel == 173 || channel == 174)
				channel += 2;
			else if( channel == 216 || channel == 217 || channel == 218)
				channel += 3;
		}
		else //FCC
			if ( channel == 3 || channel == 4 || channel == 46 || channel == 47 || channel == 90 || channel == 91  || channel == 133 || channel == 134 || channel == 176 || channel == 177 || channel == 220 || channel == 221 )
				channel += 2;
		//Store
		hopping_frequency[i] = channel;
		debug(" %02X",channel);
	}
	debugln("");
	hopping_frequency[47] = 0;									//Bind freq
}

void Frsky_init_clone(void)
{
	debugln("Clone mode");
	uint16_t temp = FRSKYD_CLONE_EEPROM_OFFSET;
	if(protocol==PROTO_FRSKYX)
		temp=FRSKYX_CLONE_EEPROM_OFFSET;
	else if(protocol==PROTO_FRSKYX2)
		temp=FRSKYX2_CLONE_EEPROM_OFFSET;
	FrSkyFormat=eeprom_read_byte((EE_ADDR)temp++);
	if(protocol==PROTO_FRSKYX)
		FrSkyFormat >>= 1;
	else
		FrSkyFormat >>= 2;
	FrSkyFormat <<= 1;	//FCC_16/LBT_16
	rx_tx_addr[3] = eeprom_read_byte((EE_ADDR)temp++);
	rx_tx_addr[2] = eeprom_read_byte((EE_ADDR)temp++);
	rx_tx_addr[1] = eeprom_read_byte((EE_ADDR)temp++);
	memset(hopping_frequency,0x00,50);
	if(protocol!=PROTO_FRSKYX2)
	{//D8 and D16v1
		for (uint8_t ch = 0; ch < 47; ch++)
			hopping_frequency[ch] = eeprom_read_byte((EE_ADDR)temp++);
	}
	else
		FrSkyX2_init_hop();
}

#endif
/******************************/
/**  FrSky V, D and X routines  **/
/******************************/
#if defined(FRSKYV_CC2500_INO) || defined(FRSKYD_CC2500_INO) || defined(FRSKYX_CC2500_INO) || defined(FRSKYL_CC2500_INO)
	const PROGMEM uint8_t FRSKY_common_startreg_cc2500_conf[]= {
		 CC2500_02_IOCFG0 ,		
		 CC2500_00_IOCFG2 ,
		 CC2500_17_MCSM1 ,
		 CC2500_18_MCSM0 ,
		 CC2500_06_PKTLEN ,
		 CC2500_07_PKTCTRL1 ,
		 CC2500_08_PKTCTRL0 ,
		 CC2500_3E_PATABLE ,
		 CC2500_0B_FSCTRL1 ,
		 CC2500_0C_FSCTRL0 ,	// replaced by option value
		 CC2500_0D_FREQ2 ,	
		 CC2500_0E_FREQ1 ,
		 CC2500_0F_FREQ0 ,
		 CC2500_10_MDMCFG4 ,		
		 CC2500_11_MDMCFG3 ,
		 CC2500_12_MDMCFG2 ,
		 CC2500_13_MDMCFG1 ,
		 CC2500_14_MDMCFG0 ,
		 CC2500_15_DEVIATN  };

	#if defined(FRSKYV_CC2500_INO)
		const PROGMEM uint8_t FRSKYV_cc2500_conf[]= {
		/*02_IOCFG0*/  	 0x06 ,		
		/*00_IOCFG2*/  	 0x06 ,
		/*17_MCSM1*/   	 0x0c ,
		/*18_MCSM0*/   	 0x18 ,
		/*06_PKTLEN*/  	 0xff ,
		/*07_PKTCTRL1*/	 0x04 ,
		/*08_PKTCTRL0*/	 0x05 ,
		/*3E_PATABLE*/ 	 0xfe ,
		/*0B_FSCTRL1*/ 	 0x08 ,
		/*0C_FSCTRL0*/ 	 0x00 ,
		/*0D_FREQ2*/   	 0x5c ,	
		/*0E_FREQ1*/   	 0x58 ,
		/*0F_FREQ0*/   	 0x9d ,
		/*10_MDMCFG4*/ 	 0xAA ,		
		/*11_MDMCFG3*/ 	 0x10 ,
		/*12_MDMCFG2*/ 	 0x93 ,
		/*13_MDMCFG1*/ 	 0x23 ,
		/*14_MDMCFG0*/ 	 0x7a ,
		/*15_DEVIATN*/ 	 0x41  };
	#endif

	#if defined(FRSKYD_CC2500_INO)
		const PROGMEM uint8_t FRSKYD_cc2500_conf[]= {
		/*02_IOCFG0*/  	 0x06 ,		
		/*00_IOCFG2*/  	 0x06 ,
		/*17_MCSM1*/   	 0x0c ,
		/*18_MCSM0*/   	 0x18 ,
		/*06_PKTLEN*/  	 0x19 ,
		/*07_PKTCTRL1*/	 0x04 ,
		/*08_PKTCTRL0*/	 0x05 ,
		/*3E_PATABLE*/ 	 0xff ,
		/*0B_FSCTRL1*/ 	 0x08 ,
		/*0C_FSCTRL0*/ 	 0x00 ,
		/*0D_FREQ2*/   	 0x5c ,	
		/*0E_FREQ1*/   	 0x76 ,
		/*0F_FREQ0*/   	 0x27 ,
		/*10_MDMCFG4*/ 	 0xAA ,		
		/*11_MDMCFG3*/ 	 0x39 ,
		/*12_MDMCFG2*/ 	 0x11 ,
		/*13_MDMCFG1*/ 	 0x23 ,
		/*14_MDMCFG0*/ 	 0x7a ,
		/*15_DEVIATN*/ 	 0x42  };
	#endif

	#if defined(FRSKYX_CC2500_INO) || defined(FRSKYL_CC2500_INO)
		const PROGMEM uint8_t FRSKYX_cc2500_conf[]= {
	//FRSKYX
		/*02_IOCFG0*/  	 0x06 ,
		/*00_IOCFG2*/  	 0x06 ,
		/*17_MCSM1*/   	 0x0c ,	//X2->0x0E -> Go/Stay in RX mode
		/*18_MCSM0*/   	 0x18 ,
		/*06_PKTLEN*/  	 0x1E ,
		/*07_PKTCTRL1*/	 0x04 ,
		/*08_PKTCTRL0*/	 0x01 , //X2->0x05 -> CRC enabled
		/*3E_PATABLE*/ 	 0xff ,
		/*0B_FSCTRL1*/ 	 0x0A ,
		/*0C_FSCTRL0*/ 	 0x00 ,
		/*0D_FREQ2*/   	 0x5c ,	
		/*0E_FREQ1*/   	 0x76 ,
		/*0F_FREQ0*/   	 0x27 ,
		/*10_MDMCFG4*/ 	 0x7B ,
		/*11_MDMCFG3*/ 	 0x61 ,	//X2->0x84 -> bitrate 70K->77K
		/*12_MDMCFG2*/ 	 0x13 ,
		/*13_MDMCFG1*/ 	 0x23 ,
		/*14_MDMCFG0*/ 	 0x7a ,
		/*15_DEVIATN*/ 	 0x51  };
		const PROGMEM uint8_t FRSKYXEU_cc2500_conf[]= {
		/*02_IOCFG0*/  	 0x06 ,
		/*00_IOCFG2*/  	 0x06 ,
		/*17_MCSM1*/   	 0x0E ,
		/*18_MCSM0*/   	 0x18 ,
		/*06_PKTLEN*/  	 0x23 ,
		/*07_PKTCTRL1*/	 0x04 ,
		/*08_PKTCTRL0*/	 0x01 , //X2->0x05 -> CRC enabled
		/*3E_PATABLE*/ 	 0xff ,
		/*0B_FSCTRL1*/ 	 0x08 ,
		/*0C_FSCTRL0*/ 	 0x00 ,
		/*0D_FREQ2*/   	 0x5c ,
		/*0E_FREQ1*/   	 0x80 ,
		/*0F_FREQ0*/   	 0x00 ,
		/*10_MDMCFG4*/ 	 0x7B ,
		/*11_MDMCFG3*/ 	 0xF8 ,
		/*12_MDMCFG2*/ 	 0x03 ,
		/*13_MDMCFG1*/ 	 0x23 ,
		/*14_MDMCFG0*/ 	 0x7a ,
		/*15_DEVIATN*/ 	 0x53  };
		const PROGMEM uint8_t FRSKYL_cc2500_conf[]= {
		/*02_IOCFG0*/  	 0x02 ,
		/*00_IOCFG2*/  	 0x02 ,
		/*17_MCSM1*/   	 0x0C ,
		/*18_MCSM0*/   	 0x18 ,
		/*06_PKTLEN*/  	 0xFF ,
		/*07_PKTCTRL1*/	 0x00 ,
		/*08_PKTCTRL0*/	 0x02 ,
		/*3E_PATABLE*/ 	 0xFE ,
		/*0B_FSCTRL1*/ 	 0x0A ,
		/*0C_FSCTRL0*/ 	 0x00 ,
		/*0D_FREQ2*/   	 0x5c ,
		/*0E_FREQ1*/   	 0x76 ,
		/*0F_FREQ0*/   	 0x27 ,
		/*10_MDMCFG4*/ 	 0x5C ,
		/*11_MDMCFG3*/ 	 0x3B ,
		/*12_MDMCFG2*/ 	 0x00 ,
		/*13_MDMCFG1*/ 	 0x03 ,
		/*14_MDMCFG0*/ 	 0x7A ,
		/*15_DEVIATN*/ 	 0x47  };
	#endif

	const PROGMEM uint8_t FRSKY_common_end_cc2500_conf[][2]= {
		{ CC2500_19_FOCCFG,   0x16 },
		{ CC2500_1A_BSCFG,    0x6c },	
		{ CC2500_1B_AGCCTRL2, 0x43 },
		{ CC2500_1C_AGCCTRL1, 0x40 },
		{ CC2500_1D_AGCCTRL0, 0x91 },
		{ CC2500_21_FREND1,   0x56 },
		{ CC2500_22_FREND0,   0x10 },
		{ CC2500_23_FSCAL3,   0xa9 },
		{ CC2500_24_FSCAL2,   0x0A },
		{ CC2500_25_FSCAL1,   0x00 },
		{ CC2500_26_FSCAL0,   0x11 },
		{ CC2500_29_FSTEST,   0x59 },
		{ CC2500_2C_TEST2,    0x88 },
		{ CC2500_2D_TEST1,    0x31 },
		{ CC2500_2E_TEST0,    0x0B },
		{ CC2500_03_FIFOTHR,  0x07 },
		{ CC2500_09_ADDR,     0x00 } };

	void FRSKY_init_cc2500(const uint8_t *ptr)
	{
		for(uint8_t i=0;i<19;i++)
		{
			uint8_t reg=pgm_read_byte_near(&FRSKY_common_startreg_cc2500_conf[i]);
			uint8_t val=pgm_read_byte_near(&ptr[i]);
			if(reg==CC2500_0C_FSCTRL0)
				val=option;
			CC2500_WriteReg(reg,val);
		}
		prev_option = option ;		// Save option to monitor FSCTRL0 change
		for(uint8_t i=0;i<17;i++)
		{
			uint8_t reg=pgm_read_byte_near(&FRSKY_common_end_cc2500_conf[i][0]);
			uint8_t val=pgm_read_byte_near(&FRSKY_common_end_cc2500_conf[i][1]);
			CC2500_WriteReg(reg,val);
		}
		CC2500_SetTxRxMode(TX_EN);
		CC2500_SetPower();
		CC2500_Strobe(CC2500_SIDLE);    // Go to idle...
	}
#endif

#if defined(FRSKYX_CC2500_INO) || defined(FRSKYL_CC2500_INO)
uint8_t FrSkyX_chanskip;
uint8_t FrSkyX_TX_Seq, FrSkyX_TX_IN_Seq;
uint8_t FrSkyX_RX_Seq ;

#ifdef SPORT_SEND
	struct t_FrSkyX_TX_Frame
	{
		uint8_t count;
		uint8_t payload[8];
	} ;
	// Store FrskyX telemetry
	struct t_FrSkyX_TX_Frame FrSkyX_TX_Frames[4] ;
#endif

#define FRSKYX_FAILSAFE_TIMEOUT 1032

static void __attribute__((unused)) FrSkyX_set_start(uint8_t ch )
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch]);
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[ch]);
}		

static void __attribute__((unused)) FrSkyX_init()
{
	if(protocol==PROTO_FRSKYL)
		FRSKY_init_cc2500(FRSKYL_cc2500_conf);
	else
		FRSKY_init_cc2500((FrSkyFormat&2)?FRSKYXEU_cc2500_conf:FRSKYX_cc2500_conf); // LBT or FCC
	if(protocol==PROTO_FRSKYX2)
	{
		CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);		// Enable CRC
		if(!(FrSkyFormat&2))
		{ // FCC
			CC2500_WriteReg(CC2500_17_MCSM1, 0x0E);		// Go/Stay in RX mode
			CC2500_WriteReg(CC2500_11_MDMCFG3, 0x84);	// bitrate 70K->77K
		}
	}
	//
	for(uint8_t c=0;c < 48;c++)
	{//calibrate hop channels
		CC2500_Strobe(CC2500_SIDLE);    
		CC2500_WriteReg(CC2500_0A_CHANNR,hopping_frequency[c]);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);//
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
	//#######END INIT########		
}

static void __attribute__((unused)) FrSkyX_initialize_data(uint8_t adr)
{
	CC2500_WriteReg(CC2500_18_MCSM0,    0x8);	
	CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : rx_tx_addr[3]);
	CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);	// check address
}

#endif
