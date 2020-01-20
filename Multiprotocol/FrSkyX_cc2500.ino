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

static void __attribute__((unused)) FrSkyX_build_bind_packet()
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
	if(binding_idx&0x01)
		memcpy(&packet[13],(void *)"\x55\xAA\x5A\xA5",4);	// Telem off
	if(binding_idx&0x02)
		memcpy(&packet[17],(void *)"\x55\xAA\x5A\xA5",4);	// CH9-16
	//
	uint16_t lcrc = FrSkyX_crc(&packet[3], limit-3);
	//
	packet[limit++] = lcrc >> 8;
	packet[limit] = lcrc;
	//
}

#define FrSkyX_FAILSAFE_TIME 1032
static void __attribute__((unused)) FrSkyX_build_packet()
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
		if (FS_flag == 0  &&  failsafe_count > FrSkyX_FAILSAFE_TIME  &&  chan_offset == 0  &&  IS_FAILSAFE_VALUES_on)
		{
			FS_flag = 0x10;
			failsafe_chan = 0;
		} else if (FS_flag & 0x10 && failsafe_chan < (sub_protocol & 0x01 ? 8-1:16-1))
		{
			FS_flag = 0x10 | ((FS_flag + 2) & 0x0F);					//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
			failsafe_chan ++;
		} else if (FS_flag & 0x10)
		{
			FS_flag = 0;
			failsafe_count = 0;
			FAILSAFE_VALUES_off;
		}
		failsafe_count++;
	#endif
	
	packet[0] = (sub_protocol & 0x02 ) ? 0x20 : 0x1D ;					// LBT or FCC
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = 0x02;
	//  
	packet[4] = (FrSkyX_chanskip<<6)|hopping_frequency_no; 
	packet[5] = FrSkyX_chanskip>>2;
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
	uint8_t startChan = chan_offset;
	for(uint8_t i = 0; i <12 ; i+=3)
	{//12 bytes of channel data
		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (startChan & 0x07)) )
				chan_0 = FrSkyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif
				chan_0 = FrSkyX_scaleForPXX(startChan);
		startChan++;
		//
		#ifdef FAILSAFE_ENABLE
			if( (FS_flag & 0x10) && ((failsafe_chan & 0x07) == (startChan & 0x07)) )
				chan_1 = FrSkyX_scaleForPXX_FS(failsafe_chan);
			else
		#endif
				chan_1 = FrSkyX_scaleForPXX(startChan);
		startChan++;
		//
		packet[9+i] = lowByte(chan_0);									//3 bytes*4
		packet[9+i+1]=(((chan_0>>8) & 0x0F)|(chan_1 << 4));
		packet[9+i+2]=chan_1>>4;
	}
	if(sub_protocol & 0x01 )											//In X8 mode send only 8ch every 9ms
		chan_offset = 0 ;
	else
		chan_offset^=0x08;
	
	//sequence and send SPort
	uint8_t limit = (sub_protocol & 2 ) ? 31 : 28 ;
	for (uint8_t i=22;i<limit;i++)
		packet[i]=0;
	packet[21] = FrSkyX_RX_Seq << 4;									//TX=8 at startup
	#ifdef SPORT_SEND
		if (FrSkyX_TX_IN_Seq!=0xFF)
		{//RX has replied at least once
			if (FrSkyX_TX_IN_Seq & 0x08)
			{//Request init
				//debugln("Init");
				FrSkyX_TX_Seq = 0 ;	
				for(uint8_t i=0;i<4;i++)
					FrSkyX_TX_Frames[i].count=0;						//Discard frames in current output buffer
			}
			else if (FrSkyX_TX_IN_Seq & 0x04)
			{//Retransmit the requested packet
				debugln("Retry:%d",FrSkyX_TX_IN_Seq&0x03);
				packet[21] |= FrSkyX_TX_IN_Seq&0x03;
				packet[22]  = FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq&0x03].count;
				for (uint8_t i=23;i<23+FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq&0x03].count;i++)
					packet[i] = FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq&0x03].payload[i];
			}
			else if ( FrSkyX_TX_Seq != 0x08 )
			{
				if(FrSkyX_TX_Seq==FrSkyX_TX_IN_Seq)
				{//Send packet from the incoming radio buffer
					//debugln("Send:%d",FrSkyX_TX_Seq);
					packet[21] |= FrSkyX_TX_Seq;
					uint8_t nbr_bytes=0;
					for (uint8_t i=23;i<limit;i++)
					{
						if(SportHead==SportTail)
							break; //buffer empty
						packet[i]=SportData[SportHead];
						FrSkyX_TX_Frames[FrSkyX_TX_Seq].payload[i-23]=SportData[SportHead];
						SportHead=(SportHead+1) & (MAX_SPORT_BUFFER-1);
						nbr_bytes++;
					}
					packet[22]=nbr_bytes;
					FrSkyX_TX_Frames[FrSkyX_TX_Seq].count=nbr_bytes;
					if(nbr_bytes)
					{//Check the buffer status
						uint8_t used = SportTail;
						if ( SportHead > SportTail )
							used += MAX_SPORT_BUFFER - SportHead ;
						else
							used -= SportHead ;
						if ( used < (MAX_SPORT_BUFFER>>1) )
						{
							DATA_BUFFER_LOW_off;
							debugln("Ok buf:%d",used);
						}
					}
					FrSkyX_TX_Seq = ( FrSkyX_TX_Seq + 1 ) & 0x03 ;		//Next iteration send next packet
				}
				else
				{//Not in sequence somehow, transmit what the receiver wants but why not asking for retransmit...
					//debugln("RX_Seq:%d,TX:%d",FrSkyX_TX_IN_Seq,FrSkyX_TX_Seq);
					packet[21] |= FrSkyX_TX_IN_Seq;
					packet[22] = FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq].count;
					for (uint8_t i=23;i<23+FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq].count;i++)
						packet[i] = FrSkyX_TX_Frames[FrSkyX_TX_IN_Seq].payload[i-23];
				}
			}
			else
				packet[21] |= 0x08 ;									//FrSkyX_TX_Seq=8 at startup
		}
		if(packet[22])
		{//Debug
			debug("SP: ");
			for(uint8_t i=0;i<packet[22];i++)
				debug("%02X ",packet[23+i]);
			debugln("");
		}
	#else
		packet[21] |= FrSkyX_TX_Seq ;//TX=8 at startup
		if ( !(FrSkyX_TX_IN_Seq & 0xF8) )
			FrSkyX_TX_Seq = ( FrSkyX_TX_Seq + 1 ) & 0x03 ;				// Next iteration send next packet
	#endif // SPORT_SEND

	uint16_t lcrc = FrSkyX_crc(&packet[3], limit-3);
	packet[limit++]=lcrc>>8;//high byte
	packet[limit]=lcrc;//low byte
}

uint16_t ReadFrSkyX()
{
	static bool transmit=true;
	#ifdef DEBUG_SERIAL
		static uint16_t fr_time=0;
	#endif
	
	switch(state)
	{	
		default: 
			FrSkyX_set_start(47);		
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			FrSkyX_build_bind_packet();
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteData(packet, packet[0]+1);
			if(IS_BIND_DONE)
				state = FRSKY_BIND_DONE;
			else
				state++;
			return 9000;
		case FRSKY_BIND_DONE:
			FrSkyX_initialize_data(0);
			hopping_frequency_no=0;
			BIND_DONE;
			state++;													//FRSKY_DATA1
			break;
		case FRSKY_DATA5:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(9000);
			#endif
			#if defined TELEMETRY
				telemetry_link=1;										//Send telemetry out anyway
			#endif
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len && (len<=(0x0E + 3)))								//Telemetry frame is 17
			{
				packet_count=0;
				CC2500_ReadData(packet_in, len);
				#if defined TELEMETRY
					frsky_check_telemetry(packet_in,len);				//Check and parse telemetry packets
				#endif
			}
			else
			{
				packet_count++;
				//debugln("M %d",packet_count);
				// restart sequence on missed packet - might need count or timeout instead of one missed
				if(packet_count>100)
				{//~1sec
					FrSkyX_TX_Seq = 0x08 ;								//Request init
					FrSkyX_TX_IN_Seq = 0xFF ;							//No sequence received yet
					#ifdef SPORT_SEND
						for(uint8_t i=0;i<4;i++)
							FrSkyX_TX_Frames[i].count=0;				//Discard frames in current output buffer
					#endif
					packet_count=0;
					#if defined TELEMETRY
						telemetry_lost=1;
						telemetry_link=0;								//Stop sending telemetry
					#endif
				}
				CC2500_Strobe(CC2500_SFRX);								//Flush the RXFIFO
			}
			FrSkyX_build_packet();
			state = FRSKY_DATA1;
#if not defined(FRSKYX_LBT)
			return 500;
#endif // for LBT just continue to DATA1 right away
		case FRSKY_DATA1:
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);				//Frequency offset hack 
				prev_option = option ;
			}
			FrSkyX_set_start(hopping_frequency_no);
			transmit=true;
#ifdef FRSKYX_LBT
			CC2500_Strobe(CC2500_SIDLE);
			delayMicroseconds(90);										//Wait for the freq to stabilize
			CC2500_Strobe(CC2500_SRX);									//Acquire RSSI
			state++;
			return 500;
		case FRSKY_DATA2:
			uint8_t rssi;
			rssi = CC2500_ReadReg(CC2500_34_RSSI | CC2500_READ_BURST);	// 0.5 db/count, RSSI value read from the RSSI status register is a 2's complement number
			if ((sub_protocol & 2) && rssi > 72 && rssi < 128)			//LBT and RSSI between -36 and -8.5 dBm
			{
				transmit=false;
				debugln("Busy %d %d",hopping_frequency_no,rssi);
			}
#endif
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFRX);
			CC2500_SetTxRxMode(TX_EN);
			CC2500_SetPower();
			hopping_frequency_no = (hopping_frequency_no+FrSkyX_chanskip)%47;
			if(transmit)
			{
				#ifdef DEBUG_SERIAL
					uint16_t fr_cur=millis();
					fr_time=fr_cur-fr_time;
					if(fr_time!=9)
						debugln("Bad timing: %d",fr_time);
					fr_time=fr_cur;
				#endif
				CC2500_WriteData(packet, packet[0]+1);
			}
			state=FRSKY_DATA3;
			return 5200;
		case FRSKY_DATA3:
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SIDLE);
			state++;
			return 200;
		case FRSKY_DATA4:
			CC2500_Strobe(CC2500_SRX);
			state++;
			return 3100;
	}		
	return 1;		
}

uint16_t initFrSkyX()
{
	set_rx_tx_addr(MProtocol_id_master);
	Frsky_init_hop();
	packet_count=0;
	while(!FrSkyX_chanskip)
		FrSkyX_chanskip=random(0xfefefefe)%47;

	//for test***************
	//rx_tx_addr[3]=0xB3;
	//rx_tx_addr[2]=0xFD;
	//************************
	FrSkyX_init();

	if(IS_BIND_IN_PROGRESS)
	{	   
		state = FRSKY_BIND;
		FrSkyX_initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		FrSkyX_initialize_data(0);
	}
	FrSkyX_TX_Seq = 0x08 ;					// Request init
	FrSkyX_TX_IN_Seq = 0xFF ;				// No sequence received yet
	#ifdef SPORT_SEND
		for(uint8_t i=0;i<4;i++)
			FrSkyX_TX_Frames[i].count=0;	// discard frames in current output buffer
		SportHead=SportTail=0;				// empty data buffer
	#endif
	FrSkyX_RX_Seq = 0 ;						// Seq 0 to start with
	binding_idx=0;							// CH1-8 and Telem on
	return 10000;
}	
#endif