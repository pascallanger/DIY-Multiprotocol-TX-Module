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

uint8_t FrX_chanskip;
uint8_t FrX_send_seq ;
uint8_t FrX_receive_seq ;

#define FRX_FAILSAFE_TIMEOUT 1032

static void __attribute__((unused)) frskyX_set_start(uint8_t ch )
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch]);
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[ch]);
}		

static void __attribute__((unused)) frskyX_init()
{
	FRSKY_init_cc2500((sub_protocol&2)?FRSKYXEU_cc2500_conf:FRSKYX_cc2500_conf); // LBT or FCC
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

static void __attribute__((unused)) frskyX_initialize_data(uint8_t adr)
{
	CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
	CC2500_WriteReg(CC2500_18_MCSM0,    0x8);	
	CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : rx_tx_addr[3]);
	CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);
}

//**CRC**
const uint16_t PROGMEM frskyX_CRC_Short[]={
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7 };
static uint16_t __attribute__((unused)) frskyX_CRCTable(uint8_t val)
{
	uint16_t word ;
	word = pgm_read_word(&frskyX_CRC_Short[val&0x0F]) ;
	val /= 16 ;
	return word ^ (0x1081 * val) ;
}
static uint16_t __attribute__((unused)) frskyX_crc_x(uint8_t *data, uint8_t len)
{
	uint16_t crc = 0;
	for(uint8_t i=0; i < len; i++)
		crc = (crc<<8) ^ frskyX_CRCTable((uint8_t)(crc>>8) ^ *data++);
	return crc;
}

static void __attribute__((unused)) frskyX_build_bind_packet()
{
	packet[0] = (sub_protocol & 2 ) ? 0x20 : 0x1D ; // LBT or FCC
	packet[1] = 0x03;
	packet[2] = 0x01;
	//
	packet[3] = rx_tx_addr[3];
	packet[4] = rx_tx_addr[2];
	int idx = ((state -FRSKY_BIND) % 10) * 5;
	packet[5] = idx;
	packet[6] = hopping_frequency[idx++];
	packet[7] = hopping_frequency[idx++];
	packet[8] = hopping_frequency[idx++];
	packet[9] = hopping_frequency[idx++];
	packet[10] = hopping_frequency[idx++];
	packet[11] = 0x02;
	packet[12] = RX_num;
	//
	uint8_t limit = (sub_protocol & 2 ) ? 31 : 28 ;
	memset(&packet[13], 0, limit - 13);
	uint16_t lcrc = frskyX_crc_x(&packet[3], limit-3);
	//
	packet[limit++] = lcrc >> 8;
	packet[limit] = lcrc;
	//
}

// 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
//64=860,1024=1500,1984=2140//Taranis 125%
static uint16_t  __attribute__((unused)) frskyX_scaleForPXX( uint8_t i )
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	uint16_t chan_val=convert_channel_frsky(i)-1226;
	if(i>7) chan_val|=2048;   // upper channels offset
	return chan_val;
}
#ifdef FAILSAFE_ENABLE
static uint16_t  __attribute__((unused)) frskyX_scaleForPXX_FS( uint8_t i )
{	//mapped 1,2046(125%) range to 64,1984(PXX values);
	uint16_t chan_val=((Failsafe_data[i]*15)>>4)+64;
	if(Failsafe_data[i]==FAILSAFE_CHANNEL_NOPULSES)
		chan_val=FAILSAFE_CHANNEL_NOPULSES;
	else if(Failsafe_data[i]==FAILSAFE_CHANNEL_HOLD)
		chan_val=FAILSAFE_CHANNEL_HOLD;
	if(i>7) chan_val|=2048;   // upper channels offset
	return chan_val;
}
#endif

#define FRX_FAILSAFE_TIME 1032
static void __attribute__((unused)) frskyX_data_frame()
{
	//0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
	//
	static uint8_t chan_offset=0;
	uint16_t chan_0 ;
	uint16_t chan_1 ; 
	//
    // data frames sent every 9ms; failsafe every 9 seconds
	#ifdef FAILSAFE_ENABLE
		static uint16_t failsafe_count=0;
		static uint8_t FS_flag=0,failsafe_chan=0;
		if (FS_flag == 0  &&  failsafe_count > FRX_FAILSAFE_TIME  &&  chan_offset == 0  &&  IS_FAILSAFE_VALUES_on)
		{
			FS_flag = 0x10;
			failsafe_chan = 0;
		} else if (FS_flag & 0x10 && failsafe_chan < (sub_protocol & 0x01 ? 8-1:16-1))
		{
			FS_flag = 0x10 | ((FS_flag + 2) & 0x0F);	//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
			failsafe_chan ++;
		} else if (FS_flag & 0x10)
		{
			FS_flag = 0;
			failsafe_count = 0;
		}
		failsafe_count++;
	#endif
	
	packet[0] = (sub_protocol & 0x02 ) ? 0x20 : 0x1D ;	// LBT or FCC
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = 0x02;
	//  
	packet[4] = (FrX_chanskip<<6)|hopping_frequency_no; 
	packet[5] = FrX_chanskip>>2;
	packet[6] = RX_num;
	//packet[7] = FLAGS 00 - standard packet
	//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
	//20 - range check packet
	#ifdef FAILSAFE_ENABLE
		packet[7] = FS_flag;
	#else
		packet[7] = 0;
	#endif
	packet[8] = 0;		
	//
	uint8_t startChan = chan_offset;	for(uint8_t i = 0; i <12 ; i+=3)
	{//12 bytes of channel data
		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (startChan & 0x07)) )
				chan_0 = frskyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif
				chan_0 = frskyX_scaleForPXX(startChan);
		startChan++;
		//
		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (startChan & 0x07)) )
				chan_1 = frskyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif
				chan_1 = frskyX_scaleForPXX(startChan);
		startChan++;
		//
		packet[9+i] = lowByte(chan_0);	//3 bytes*4
		packet[9+i+1]=(((chan_0>>8) & 0x0F)|(chan_1 << 4));
		packet[9+i+2]=chan_1>>4;
	}
	packet[21] = (FrX_receive_seq << 4) | FrX_send_seq ;//8 at start
	
	if(sub_protocol & 0x01 )			// in X8 mode send only 8ch every 9ms
		chan_offset = 0 ;
	else
		chan_offset^=0x08;
	
	uint8_t limit = (sub_protocol & 2 ) ? 31 : 28 ;
	for (uint8_t i=22;i<limit;i++)
		packet[i]=0;
	#if defined SPORT_POLLING
		uint8_t idxs=0;
		if(ok_to_send)
			for (uint8_t i=23;i<limit;i++)
			{//
				if(sport_index==sport_idx)
				{//no new data
					ok_to_send=false;
					break;
				}
				packet[i]=SportData[sport_index];	
				sport_index= (sport_index+1)& (MAX_SPORT_BUFFER-1);
				idxs++;
			}
		packet[22]= idxs;
		#ifdef DEBUG_SERIAL
			for(uint8_t i=0;i<idxs;i++)
			{
				Serial.print(packet[23+i],HEX);
				Serial.print(" ");	
			}		
			Serial.println(" ");
		#endif
	#endif // SPORT_POLLING

	uint16_t lcrc = frskyX_crc_x(&packet[3], limit-3);
	packet[limit++]=lcrc>>8;//high byte
	packet[limit]=lcrc;//low byte
}

uint16_t ReadFrSkyX()
{
	switch(state)
	{	
		default: 
			frskyX_set_start(47);		
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			frskyX_build_bind_packet();
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteData(packet, packet[0]+1);
			if(IS_BIND_DONE)
				state = FRSKY_BIND_DONE;
			else
				state++;
			return 9000;
		case FRSKY_BIND_DONE:
			frskyX_initialize_data(0);
			hopping_frequency_no=0;
			BIND_DONE;
			state++;			
			break;		
		case FRSKY_DATA1:
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
				prev_option = option ;
			}
			CC2500_SetTxRxMode(TX_EN);
			frskyX_set_start(hopping_frequency_no);
			CC2500_SetPower();		
			CC2500_Strobe(CC2500_SFRX);
			hopping_frequency_no = (hopping_frequency_no+FrX_chanskip)%47;
			CC2500_Strobe(CC2500_SIDLE);		
			CC2500_WriteData(packet, packet[0]+1);
			//
//			frskyX_data_frame();
			state++;
			return 5200;
		case FRSKY_DATA2:
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SIDLE);
			state++;
			return 200;
		case FRSKY_DATA3:		
			CC2500_Strobe(CC2500_SRX);
			state++;
			return 3100;
		case FRSKY_DATA4:
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len && (len<=(0x0E + 3)))				//Telemetry frame is 17
			{
				packet_count=0;
				CC2500_ReadData(pkt, len);
				#if defined TELEMETRY
					frsky_check_telemetry(pkt,len);	//check if valid telemetry packets
					//parse telemetry packets here
					//The same telemetry function used by FrSky(D8).
				#endif
			} 
			else
			{
				packet_count++;
				// restart sequence on missed packet - might need count or timeout instead of one missed
				if(packet_count>100)
				{//~1sec
//					seq_last_sent = 0;
//					seq_last_rcvd = 8;
					FrX_send_seq = 0x08 ;
//					FrX_receive_seq = 0 ;
					packet_count=0;
					#if defined TELEMETRY
						telemetry_lost=1;
					#endif
				}
				CC2500_Strobe(CC2500_SFRX);			//flush the RXFIFO
			}
			frskyX_data_frame();
			if ( FrX_send_seq != 0x08 )
			{
				FrX_send_seq = ( FrX_send_seq + 1 ) & 0x03 ;
			}
			state = FRSKY_DATA1;
			return 500;
	}		
	return 1;		
}

uint16_t initFrSkyX()
{
	set_rx_tx_addr(MProtocol_id_master);
	Frsky_init_hop();
	packet_count=0;
	while(!FrX_chanskip)
		FrX_chanskip=random(0xfefefefe)%47;

	//for test***************
	//rx_tx_addr[3]=0xB3;
	//rx_tx_addr[2]=0xFD;
	//************************
	frskyX_init();
#if defined  SPORT_POLLING
#ifdef INVERT_SERIAL
	start_timer4() ;
#endif
#endif
	//
	if(IS_BIND_IN_PROGRESS)
	{	   
		state = FRSKY_BIND;
		frskyX_initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		frskyX_initialize_data(0);
	}
//	seq_last_sent = 0;
//	seq_last_rcvd = 8;
	FrX_send_seq = 0x08 ;
	FrX_receive_seq = 0 ;
	return 10000;
}	
#endif