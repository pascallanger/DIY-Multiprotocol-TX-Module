
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
	uint8_t calData[48][3];
	uint8_t channr;
	uint8_t pass_ = 1 ;
	uint8_t counter_rst;
	uint8_t ctr;
	uint8_t FS_flag=0;
   // uint8_t ptr[4]={0x01,0x12,0x23,0x30};
	//uint8_t ptr[4]={0x00,0x11,0x22,0x33};
	
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

	static uint8_t __attribute__((unused)) hop(uint8_t byte)
	{
		return pgm_read_byte_near(&hop_data[byte]);
	}

	static void __attribute__((unused)) set_start(uint8_t ch )
	{
		cc2500_strobe(CC2500_SIDLE);
		cc2500_writeReg(CC2500_23_FSCAL3, calData[ch][0]);
		cc2500_writeReg(CC2500_24_FSCAL2, calData[ch][1]);
		cc2500_writeReg(CC2500_25_FSCAL1, calData[ch][2]);
		cc2500_writeReg(CC2500_0A_CHANNR, ch==47?0:pgm_read_word(&hop_data[ch]));
	}		
	
	static void __attribute__((unused)) frskyX_init()
	{
		CC2500_Reset();
		
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
						
			cc2500_writeReg(reg,val);
		}

		cc2500_writeReg(CC2500_07_PKTCTRL1, 0x04);			
		cc2500_writeReg(CC2500_0C_FSCTRL0, option);
		cc2500_strobe(CC2500_SIDLE);    
		//
		for(uint8_t c=0;c < 47;c++){//calibrate hop channels
			cc2500_strobe(CC2500_SIDLE);    
			cc2500_writeReg(CC2500_0A_CHANNR,pgm_read_word(&hop_data[c]));
			cc2500_strobe(CC2500_SCAL);
			delayMicroseconds(900);//
			calData[c][0] = cc2500_readReg(CC2500_23_FSCAL3);
			calData[c][1] = cc2500_readReg(CC2500_24_FSCAL2);	
			calData[c][2] = cc2500_readReg(CC2500_25_FSCAL1);
		}
		cc2500_strobe(CC2500_SIDLE);    	
		cc2500_writeReg(CC2500_0A_CHANNR,0x00);
		cc2500_strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[47][0] = cc2500_readReg(CC2500_23_FSCAL3);
		calData[47][1] = cc2500_readReg(CC2500_24_FSCAL2);	
		calData[47][2] = cc2500_readReg(CC2500_25_FSCAL1);
		//#######END INIT########		
	}
	
	static void __attribute__((unused)) initialize_data(uint8_t adr)
	{
		cc2500_writeReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
		cc2500_writeReg(CC2500_18_MCSM0,    0x8);	
		cc2500_writeReg(CC2500_09_ADDR, adr ? 0x03 : rx_tx_addr[3]);
		cc2500_writeReg(CC2500_07_PKTCTRL1,0x05);
	}
	
	static uint8_t __attribute__((unused)) crc_Byte( uint8_t byte )
	{
		crc = (crc<<8) ^ pgm_read_word(&CRCTable[((uint8_t)(crc>>8) ^ byte) & 0xFF]);
		return byte;
	}
    
	static uint16_t  __attribute__((unused)) scaleForPXX( uint8_t i )
	{	//mapped 860,2140(125%) range to 64,1984(PXX values);
		return (uint16_t)(((Servo_data[i]-PPM_MIN)*3)>>1)+64;
	}
	
	static void __attribute__((unused)) frskyX_build_bind_packet()
	{
		crc=0;
		packet[0] = 0x1D;       
		packet[1] = 0x03;          
		packet[2] = 0x01;               
		//	
		packet[3] = crc_Byte(rx_tx_addr[3]);
		packet[4] = crc_Byte(rx_tx_addr[2]);
		int idx = ((state -FRSKY_BIND) % 10) * 5;
		packet[5] = crc_Byte(idx);	
		packet[6] = crc_Byte(pgm_read_word(&hop_data[idx++]));
		packet[7] = crc_Byte(pgm_read_word(&hop_data[idx++]));
		packet[8] = crc_Byte(pgm_read_word(&hop_data[idx++]));
		packet[9] = crc_Byte(pgm_read_word(&hop_data[idx++]));
		packet[10] = crc_Byte(pgm_read_word(&hop_data[idx++]));
		packet[11] = crc_Byte(0x02);
		packet[12] = crc_Byte(RX_num);
		//
		for(uint8_t i=13;i<28;i++)
			packet[i]=crc_Byte(0);
		//
		packet[28]=highByte(crc);
		packet[29]=lowByte(crc);
		//
	}
	
	static void __attribute__((unused)) frskyX_data_frame()
	{
		//0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
		//
		uint8_t lpass = pass_ ;
		uint16_t chan_0 ;
		uint16_t chan_1 ; 
		uint8_t flag2 = 0;
		uint8_t startChan = 0;
		crc = 0;
		//static uint8_t p = 0;
		//
		packet[0] = 0x1D; 
		packet[1] = rx_tx_addr[3];
		packet[2] = rx_tx_addr[2];
		packet[3] = crc_Byte(0x02);
		//  
		packet[4] = crc_Byte((ctr<<6)+channr);	//*64
		packet[5] = crc_Byte(counter_rst);
		packet[6] = crc_Byte(RX_num);
		//	FLAGS 00 - standard packet
		//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
		//20 - range check packet
		packet[7] = crc_Byte(FS_flag);
		packet[8] = crc_Byte(flag2);
		//
		if ( lpass & 1 )
			startChan += 8 ;
		
		for(uint8_t i = 0; i <12 ; i+=3)
		{//12 bytes
			chan_0 = scaleForPXX(startChan);		 
			if(lpass & 1 )
				chan_0+=2048;
			
			packet[9+i] = crc_Byte(lowByte(chan_0));//3 bytes*4
			startChan++;
			chan_1 = scaleForPXX(startChan);
			if(lpass & 1 )
				chan_1+= 2048;
			
			startChan++;
			packet[9+i+1]=crc_Byte((((chan_0>>8) & 0x0F)|(chan_1 << 4)));
			packet[9+i+2]=crc_Byte(chan_1>>4);
		}
		//packet[21]=crc_Byte(0x08);//first 
		packet[21]=crc_Byte(0x80);//??? when received first telemetry frame is changed to 0x80
		//packet[21]=crc_Byte(ptr[p]);//??? 
        //p=(p+1)%4;//repeating 4 bytes sequence pattern  every 4th frame.
		
		pass_=lpass+1;
		
		for (uint8_t i=22;i<28;i++)
			packet[i]=crc_Byte(0);
		
		packet[28]=highByte(crc);
		packet[29]=lowByte(crc);
	}

	uint16_t ReadFrSkyX()
	{
		switch(state)
		{	
			default: 
				set_start(47);		
				CC2500_SetPower();
				cc2500_strobe(CC2500_SFRX);
				//		
				frskyX_build_bind_packet();
				cc2500_strobe(CC2500_SIDLE);
				cc2500_writeFifo(packet, packet[0]+1);
				state++;
				return 9000;
			case FRSKY_BIND_DONE:
				initialize_data(0);
				channr=0;
				BIND_DONE;
				state++;			
				break;		
			case FRSKY_DATA1:
				LED_ON;
				CC2500_SetTxRxMode(TX_EN);
				set_start(channr);
				CC2500_SetPower();		
				cc2500_strobe(CC2500_SFRX);
				channr = (channr+chanskip)%47;
				cc2500_strobe(CC2500_SIDLE);		
				cc2500_writeFifo(packet, packet[0]+1);
				//
				frskyX_data_frame();
				state++;
				return 5500;
			case FRSKY_DATA2:
				CC2500_SetTxRxMode(RX_EN);
				cc2500_strobe(CC2500_SIDLE);
				state++;
				return 200;
			case FRSKY_DATA3:		
				cc2500_strobe(CC2500_SRX);
				state++;
				return 3000;
			case FRSKY_DATA4:
				len = cc2500_readReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
				if (len &&(len<MAX_PKT))
				{
					cc2500_readFifo(pkt, len);
					#if defined TELEMETRY
						frsky_check_telemetry(pkt,len);	//check if valid telemetry packets
						//parse telemetry packets here
						//The same telemetry function used by FrSky(D8).
					#endif
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
			randomSeed((uint32_t)analogRead(A6) << 10 | analogRead(A7));
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
		return 10000;
	}	
#endif