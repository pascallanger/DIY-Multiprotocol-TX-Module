//*************************************
// FrSky Telemetry serial code        *
// By Midelic  on RCGroups                 *
//*************************************

#if defined TELEMETRY
	#if defined FRSKYX_CC2500_INO
		#define SPORT_TELEMETRY	
	#endif
	#if defined FRSKY_CC2500_INO
		#define HUB_TELEMETRY
	#endif
	#if defined SPORT_TELEMETRY	
		#define SPORT_TELEMETRY	
		#define SPORT_TIME 12000
		uint32_t last=0;
		uint8_t sport_counter=0;
		uint8_t RxBt=0;
		uint8_t rssi;
		uint8_t ADC2;
	#endif
	#if defined HUB_TELEMETRY
		#define MAX_PKTX 10
		uint8_t pktx[MAX_PKTX];
		uint8_t index;
		uint8_t prev_index;
		uint8_t pass = 0;
	#endif
	#define USER_MAX_BYTES 6
	uint8_t frame[18];
	
	void frskySendStuffed()
	{
		Serial_write(0x7E);
		for (uint8_t i = 0; i < 9; i++)
		{
			if ((frame[i] == 0x7e) || (frame[i] == 0x7d))
			{
				Serial_write(0x7D);	    	  
				frame[i] ^= 0x20;	
			}
			Serial_write(frame[i]);
		}
		Serial_write(0x7E);
	}
	
	void compute_RSSIdbm(){
		
		RSSI_dBm = (((uint16_t)(pktt[len-2])*18)>>5);
		if(pktt[len-2] >=128)
			RSSI_dBm -= 82;
		else
			RSSI_dBm += 65;
	}

	void frsky_check_telemetry(uint8_t *pkt,uint8_t len)
	{
		if(pkt[1] != rx_tx_addr[3] || pkt[2] != rx_tx_addr[2] || len != pkt[0] + 3)
		{//only packets with the required id and packet length
			for(uint8_t i=3;i<6;i++)
				pktt[i]=0;
			return;
		}
		else
		{	   
			for (uint8_t i=3;i<len;i++)
				pktt[i]=pkt[i];				 
			telemetry_link=1;
			if(pktt[6]>0)
				telemetry_counter=(telemetry_counter+1)%32;		
		}
	}

	void frsky_link_frame()
	{
		frame[0] = 0xFE;
		if ((cur_protocol[0]&0x1F)==MODE_FRSKY)
		{		
			compute_RSSIdbm();				
			frame[1] = pktt[3];
			frame[2] = pktt[4];
			frame[3] = (uint8_t)RSSI_dBm; 
			frame[4] = pktt[5]*2;
		}
		else
			if ((cur_protocol[0]&0x1F)==MODE_HUBSAN)
			{	
				frame[1] = v_lipo*2; //v_lipo; common 0x2A=42/10=4.2V
				frame[2] = frame[1];			
				frame[3] = 0x00;
				frame[4] = (uint8_t)RSSI_dBm;
			}
		frame[5] = frame[6] = frame[7] = frame[8] = 0;			
		frskySendStuffed();
	}
	
	#if defined HUB_TELEMETRY
		void frsky_user_frame()
		{
			uint8_t indexx = 0, c=0, j=8, n=0, i;
			
			if(pktt[6]>0 && pktt[6]<=MAX_PKTX)
			{//only valid hub frames	  
				frame[0] = 0xFD;
				frame[1] = 0;
				frame[2] = pktt[7];
				
				switch(pass)
				{
					case 0:
						indexx=pktt[6];
						for(i=0;i<indexx;i++)
						{
							if(pktt[j]==0x5E)
							{
								if(c++)
								{
									c=0;
									n++;
									j++;
								}
							}
							pktx[i]=pktt[j++];
						}	
						indexx = indexx-n;
						pass=1;
					
					case 1:
						index=indexx;
						prev_index = indexx; 
						if(index<USER_MAX_BYTES)
						{   			
							for(i=0;i<index;i++)
								frame[i+3]=pktx[i];
							pktt[6]=0;
							pass=0;
						}
						else
						{
							index = USER_MAX_BYTES;
							for(i=0;i<index;i++)
								frame[i+3]=pktx[i];
							pass=2;
						}			
						break;
					case 2:		
						index = prev_index - index;
						prev_index=0;
						if(index<MAX_PKTX-USER_MAX_BYTES)	//10-6=4
							for(i=0;i<index;i++)
								frame[i+3]=pktx[USER_MAX_BYTES+i];
						pass=0;
						pktt[6]=0; 
						break;
					default:
						break;
				}
				if(!index)
					return;
				frame[1] = index;
				frskySendStuffed();
			}
			else
				pass=0;
		}	   
	#endif
	
	#if defined SPORT_TELEMETRY
		
		/* SPORT details serial
			100K 8E2 normal-multiprotocol
			-every 12ms-
		    1  2  3  4  5  6  7  8  9  CRC DESCR
			7E 98 10 05 F1 20 23 0F 00 A6 SWR_ID 
			7E 98 10 01 F1 33 00 00 00 C9 RSSI_ID 
			7E 98 10 04 F1 58 00 00 00 A1 BATT_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 	
			
			
			Telemetry frames(RF) SPORT info 15 bytes
			SPORT frame 6+3 bytes
			[00] PKLEN  0E 0E 0E 0E 
			[01] TXID1  DD DD DD DD 
			[02] TXID2  6D 6D 6D 6D 
			[03] CONST  02 02 02 02 
			[04] RS/RB  2C D0 2C CE	//D0;CE=2*RSSI;....2C = RX battery voltage(5V from Bec)
			[05] ?????  03 10 21 32	//TX/RX telemetry hand-shake bytes
			[06] NO.BT  00 00 06 03	//No.of valid SPORT frame bytes in the frame		
			[07] STRM1  00 00 7E 00 
			[08] STRM2  00 00 1A 00 
			[09] STRM3  00 00 10 00 
			[10] STRM4  03 03 03 03  
			[11] STRM5  F1 F1 F1 F1 
			[12] STRM6  D1 D1 D0 D0
			[13] CHKSUM1
			[14] CHKSUM2
		*/
		
		
		void sportSend(uint8_t *p)
		{
			uint16_t crc_s = 0;
			Serial_write(0x7e);//+9
			for (uint8_t i = 0; i < 9; i++)
			{
				if (i == 8)
					p[i] = 0xff - crc_s;
				if ((p[i] == 0x7e) || (p[i] == 0x7d))
				{
					Serial_write(0x7d);
					Serial_write(0x20 ^ p[i]);
				} 
				else
					Serial_write(p[i]);
				if (i>0)
				{
					crc_s += p[i]; //0-1FF
					crc_s += crc_s >> 8; //0-100
					crc_s &= 0x00ff;
				}
			}
		}
		
		void sportIdle()
		{
			Serial_write(0x7e);
		}	

		void sportSendFrame()
		{
			//at the moment only SWR RSSI,RxBt and A2.	
			sport_counter = (sport_counter + 1) %9;
			
			for (uint8_t i=5;i<8;i++)
				frame[i]=0;
			
			switch (sport_counter)
			{
				case 0: // SWR
					frame[0] = 0x98;
					frame[1] = 0x10;
					frame[2] = 0x05;
					frame[3] = 0xf1;
					frame[4] = 0x20;//dummy values if swr 20230f00
					frame[5] = 0x23;
					frame[6] = 0x0F;
					frame[7] = 0x00;
					break;
				case 1: // RSSI
					frame[0] = 0x98;
					frame[1] = 0x10;
					frame[2] = 0x01;
					frame[3] = 0xf1;
					frame[4] = rssi;
					break;
				case 2: //BATT
					frame[0] = 0x98;
					frame[1] = 0x10;
					frame[2] = 0x04;
					frame[3] = 0xf1;
					frame[4] = RxBt;//a1;
					break;				
				case 3: //ADC2(A2)
					frame[0] = 0x1A;
					frame[1] = 0x10;
					frame[2] = 0x03;
					frame[3] = 0xf1;
					frame[4] = ADC2;//a2;;
					break;				
				default:
					sportIdle();
					return;
			}
			sportSend(frame);
		}	
		
		void process_sport_data()//only for ADC2
		{		
			uint8_t j=7;
			if(pktt[6]>0 && pktt[6]<=USER_MAX_BYTES)
			{
				for(uint8_t i=0;i<6;i++)
					if(pktt[j++]==0x03)
						if(pktt[j]==0xF1)
						{
							ADC2=pktt[j+1];
							break;
						}
				pktt[6]=0;//new frame
			}	
		}
	#endif
	
	
	void frskyUpdate()
	{		
		if(telemetry_link && (cur_protocol[0]&0x1F) != MODE_FRSKYX )
		{	
			frsky_link_frame();
			telemetry_link=0;
			return;
		}
		#if defined HUB_TELEMETRY
			if(!telemetry_link && (cur_protocol[0]&0x1F) != MODE_HUBSAN && (cur_protocol[0]&0x1F) != MODE_FRSKYX)
			{
				frsky_user_frame();
				return;
			}
		#endif
		#if defined SPORT_TELEMETRY
			if ((cur_protocol[0]&0x1F)==MODE_FRSKYX)
			{
				if(telemetry_link)
				{
					process_sport_data();
					if(pktt[4]>0x36)
						rssi=pktt[4]/2;
					else 
						RxBt=pktt[4];					
					telemetry_link=0;		
				}
				uint32_t now = micros();
				if ((now - last) > SPORT_TIME)
				{
					sportSendFrame();
					last = now;
				}
			}
		#endif					
	}
	
#endif