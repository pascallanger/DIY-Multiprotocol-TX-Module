
/*  **************************
	* By Midelic on RCGroups *
	**************************
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

#if defined(FRSKYX_CC2500_INO)

#include "iface_cc2500.h"

uint8_t chanskip;
uint8_t counter_rst;
uint8_t ctr;
uint8_t seq_last_sent;
uint8_t seq_last_rcvd;

const PROGMEM uint8_t hop_data[]={
	0x02,	0xD4,	0xBB,	0xA2,	0x89,
	0x70,	0x57,	0x3E,	0x25,	0x0C,
	0xDE,	0xC5,	0xAC,	0x93,	0x7A,
	0x61,	0x48,	0x2F,	0x16,	0xE8,
	0xCF,	0xB6,	0x9D,	0x84,	0x6B,
	0x52,	0x39,	0x20,	0x07,	0xD9,
	0xC0,	0xA7,	0x8E,	0x75,	0x5C,
	0x43,	0x2A,	0x11,	0xE3,	0xCA,
	0xB1,	0x98,	0x7F,	0x66,	0x4D,
	0x34,	0x1B,	0x00,	0x1D,	0x03 
};

const uint16_t PROGMEM CRCTable[] =
{
	0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,
	0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
	0x1081,0x0108,0x3393,0x221a,0x56a5,0x472c,0x75b7,0x643e,
	0x9cc9,0x8d40,0xbfdb,0xae52,0xdaed,0xcb64,0xf9ff,0xe876,
	0x2102,0x308b,0x0210,0x1399,0x6726,0x76af,0x4434,0x55bd,
	0xad4a,0xbcc3,0x8e58,0x9fd1,0xeb6e,0xfae7,0xc87c,0xd9f5,
	0x3183,0x200a,0x1291,0x0318,0x77a7,0x662e,0x54b5,0x453c,
	0xbdcb,0xac42,0x9ed9,0x8f50,0xfbef,0xea66,0xd8fd,0xc974,
	0x4204,0x538d,0x6116,0x709f,0x0420,0x15a9,0x2732,0x36bb,
	0xce4c,0xdfc5,0xed5e,0xfcd7,0x8868,0x99e1,0xab7a,0xbaf3,
	0x5285,0x430c,0x7197,0x601e,0x14a1,0x0528,0x37b3,0x263a,
	0xdecd,0xcf44,0xfddf,0xec56,0x98e9,0x8960,0xbbfb,0xaa72,
	0x6306,0x728f,0x4014,0x519d,0x2522,0x34ab,0x0630,0x17b9,
	0xef4e,0xfec7,0xcc5c,0xddd5,0xa96a,0xb8e3,0x8a78,0x9bf1,
	0x7387,0x620e,0x5095,0x411c,0x35a3,0x242a,0x16b1,0x0738,
	0xffcf,0xee46,0xdcdd,0xcd54,0xb9eb,0xa862,0x9af9,0x8b70,
	0x8408,0x9581,0xa71a,0xb693,0xc22c,0xd3a5,0xe13e,0xf0b7,
	0x0840,0x19c9,0x2b52,0x3adb,0x4e64,0x5fed,0x6d76,0x7cff,
	0x9489,0x8500,0xb79b,0xa612,0xd2ad,0xc324,0xf1bf,0xe036,
	0x18c1,0x0948,0x3bd3,0x2a5a,0x5ee5,0x4f6c,0x7df7,0x6c7e,
	0xa50a,0xb483,0x8618,0x9791,0xe32e,0xf2a7,0xc03c,0xd1b5,
	0x2942,0x38cb,0x0a50,0x1bd9,0x6f66,0x7eef,0x4c74,0x5dfd,
	0xb58b,0xa402,0x9699,0x8710,0xf3af,0xe226,0xd0bd,0xc134,
	0x39c3,0x284a,0x1ad1,0x0b58,0x7fe7,0x6e6e,0x5cf5,0x4d7c,
	0xc60c,0xd785,0xe51e,0xf497,0x8028,0x91a1,0xa33a,0xb2b3,
	0x4a44,0x5bcd,0x6956,0x78df,0x0c60,0x1de9,0x2f72,0x3efb,
	0xd68d,0xc704,0xf59f,0xe416,0x90a9,0x8120,0xb3bb,0xa232,
	0x5ac5,0x4b4c,0x79d7,0x685e,0x1ce1,0x0d68,0x3ff3,0x2e7a,
	0xe70e,0xf687,0xc41c,0xd595,0xa12a,0xb0a3,0x8238,0x93b1,
	0x6b46,0x7acf,0x4854,0x59dd,0x2d62,0x3ceb,0x0e70,0x1ff9,
	0xf78f,0xe606,0xd49d,0xc514,0xb1ab,0xa022,0x92b9,0x8330,
	0x7bc7,0x6a4e,0x58d5,0x495c,0x3de3,0x2c6a,0x1ef1,0x0f78
};

static uint8_t __attribute__((unused)) hop(uint8_t byte)
{
	return pgm_read_byte_near(&hop_data[byte]);
}

static void __attribute__((unused)) set_start(uint8_t ch )
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch]);
	CC2500_WriteReg(CC2500_0A_CHANNR, ch==47? 0:hop(ch));
}		

static void __attribute__((unused)) frskyX_init()
{
	for(uint8_t i=0;i<36;i++)
	{
		uint8_t reg=pgm_read_byte_near(&cc2500_conf[i][0]);
		uint8_t val=pgm_read_byte_near(&cc2500_conf[i][1]);
		
		if(reg==CC2500_06_PKTLEN)
			val=0x1E;
		else
		if(reg==CC2500_08_PKTCTRL0)
			val=0x01;
		else
		if(reg==CC2500_0B_FSCTRL1)
			val=0x0A;
		else
		if(reg==CC2500_10_MDMCFG4)
			val=0x7B;
		else
		if(reg==CC2500_11_MDMCFG3)
			val=0x61;
		else
		if(reg==CC2500_12_MDMCFG2)
			val=0x13;
		else
		if(reg==CC2500_15_DEVIATN)
			val=0x51;
		
		CC2500_WriteReg(reg,val);
	}

	CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);
        prev_option = option ;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	CC2500_Strobe(CC2500_SIDLE);    
	//
	for(uint8_t c=0;c < 47;c++)
	{//calibrate hop channels
		CC2500_Strobe(CC2500_SIDLE);    
		CC2500_WriteReg(CC2500_0A_CHANNR,hop(c));
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);//
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
	CC2500_Strobe(CC2500_SIDLE);    	
	CC2500_WriteReg(CC2500_0A_CHANNR,0x00);
	CC2500_Strobe(CC2500_SCAL);
	delayMicroseconds(900);
	calData[47] = CC2500_ReadReg(CC2500_25_FSCAL1);
	//#######END INIT########		
}

static void __attribute__((unused)) initialize_data(uint8_t adr)
{
	CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
	CC2500_WriteReg(CC2500_18_MCSM0,    0x8);	
	CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : rx_tx_addr[3]);
	CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);
}
/*	
	static uint8_t __attribute__((unused)) crc_Byte( uint8_t byte )
	{
		crc = (crc<<8) ^ pgm_read_word(&CRCTable[((uint8_t)(crc>>8) ^ byte) & 0xFF]);
		return byte;
	}
*/  
static uint16_t __attribute__((unused)) crc_x(uint8_t *data, uint8_t len)
{
	uint16_t crc = 0;
	for(uint8_t i=0; i < len; i++)
		crc = (crc<<8) ^ pgm_read_word(&CRCTable[((uint8_t)(crc>>8) ^ *data++) & 0xFF]);
	return crc;
}

 // 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
 //64=860,1024=1500,1984=2140//Taranis 125%

static uint16_t  __attribute__((unused)) scaleForPXX( uint8_t i )
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	return (uint16_t)(((Servo_data[i]-servo_min_125)*3)>>1)+64;
}

static void __attribute__((unused)) frskyX_build_bind_packet()
{
	packet[0] = 0x1D;       
	packet[1] = 0x03;          
	packet[2] = 0x01;               
	//	
	packet[3] = rx_tx_addr[3];
	packet[4] = rx_tx_addr[2];
	int idx = ((state -FRSKY_BIND) % 10) * 5;
	packet[5] = idx;	
	packet[6] = hop(idx++);
	packet[7] = hop(idx++);
	packet[8] = hop(idx++);
	packet[9] = hop(idx++);
	packet[10] = hop(idx++);
	packet[11] = 0x02;
	packet[12] = RX_num;
	//
	memset(&packet[13], 0, 15);	
	uint16_t lcrc = crc_x(&packet[3], 25);	
	//
	packet[28] = lcrc >> 8;
	packet[29] = lcrc;
	//
}

static void __attribute__((unused)) frskyX_data_frame()
{
	//0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
	//
	static uint8_t lpass;
	uint16_t chan_0 ;
	uint16_t chan_1 ; 
	uint8_t startChan = 0;
	//
	packet[0] = 0x1D; 
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = 0x02;
	//  
	packet[4] = (ctr<<6)+hopping_frequency_no; 
	packet[5] = counter_rst;
	packet[6] = RX_num;
	//packet[7] = FLAGS 00 - standard packet
	//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
	//20 - range check packet
	packet[7] = 0;
	packet[8] = 0;		
	//
	if ( lpass & 1 )
		startChan += 8 ;
	
	for(uint8_t i = 0; i <12 ; i+=3)
	{//12 bytes
		chan_0 = scaleForPXX(startChan);		 
		if(lpass & 1 )
			chan_0+=2048;			
		startChan+=1;
		//
		chan_1 = scaleForPXX(startChan);		
		if(lpass & 1 )
			chan_1+= 2048;		
		startChan+=1;
		//
		packet[9+i] = lowByte(chan_0);//3 bytes*4
		packet[9+i+1]=(((chan_0>>8) & 0x0F)|(chan_1 << 4));
		packet[9+i+2]=chan_1>>4;
	}

	packet[21] = seq_last_sent << 4 | seq_last_rcvd;//8 at start
	if (seq_last_sent < 0x08 && seq_last_rcvd < 8)
		seq_last_sent = (seq_last_sent + 1) % 4;
	else if (seq_last_rcvd == 0x00)
		seq_last_sent = 1;
	
	if(sub_protocol== CH_8 )// in X8 mode send only 8ch every 9ms
		lpass = 0 ;
	else
		lpass += 1 ;
	
	for (uint8_t i=22;i<28;i++)
		packet[i]=0;
	uint16_t lcrc = crc_x(&packet[3], 25);
	
	packet[28]=lcrc>>8;//high byte
	packet[29]=lcrc;//low byte
}

uint16_t ReadFrSkyX()
{
	switch(state)
	{	
		default: 
			set_start(47);		
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			frskyX_build_bind_packet();
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteData(packet, packet[0]+1);
			state++;
			return 9000;
		case FRSKY_BIND_DONE:
			initialize_data(0);
			hopping_frequency_no = 0;
			BIND_DONE;
			state++;			
			break;		
		case FRSKY_DATA1:
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
				prev_option = option ;
			}
			LED_on;
			CC2500_SetTxRxMode(TX_EN);
			set_start(hopping_frequency_no);
			CC2500_SetPower();		
			CC2500_Strobe(CC2500_SFRX);
			hopping_frequency_no = (hopping_frequency_no+chanskip)%47;
			CC2500_Strobe(CC2500_SIDLE);		
			CC2500_WriteData(packet, packet[0]+1);
			//
			frskyX_data_frame();
			state++;
			return 5500;
		case FRSKY_DATA2:
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SIDLE);
			state++;
			return 200;
		case FRSKY_DATA3:		
			CC2500_Strobe(CC2500_SRX);
			state++;
			return 3000;
		case FRSKY_DATA4:
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len && (len<MAX_PKT))
			{
				CC2500_ReadData(pkt, len);
				#if defined TELEMETRY
				frsky_check_telemetry(pkt,len);	//check if valid telemetry packets
				//parse telemetry packets here
				//The same telemetry function used by FrSky(D8).
				#endif
			} 
			else
			{
				counter++;
				// restart sequence on missed packet - might need count or timeout instead of one missed
				if(counter>100)
				{//~1sec
					seq_last_sent = 0;
					seq_last_rcvd = 8;
					counter=0;
				}
			}
			state = FRSKY_DATA1;
			return 300;
	}		
	return 1;		
}

uint16_t initFrSkyX()
{
	while(!chanskip)
	{
	#if defined STM32_board
	randomSeed((uint32_t)analogRead(PB0) << 10 | analogRead(PB1));			
	#endif
		chanskip=random(0xfefefefe)%47;
	}
	while((chanskip-ctr)%4)
	ctr=(ctr+1)%4;
	
	counter_rst=(chanskip-ctr)>>2;
	//for test***************
	//rx_tx_addr[3]=0xB3;
	//rx_tx_addr[2]=0xFD;
	//************************
	frskyX_init();
	CC2500_SetTxRxMode(TX_EN);
	//
	if(IS_AUTOBIND_FLAG_on)
	{	   
		state = FRSKY_BIND;
		initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		initialize_data(0);
	}
	seq_last_sent = 0;
	seq_last_rcvd = 8;
	return 10000;
}	
#endif
