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
//**************************
// Telemetry serial code   *
//**************************
#if defined TELEMETRY

uint8_t RetrySequence ;

#if ( defined(MULTI_TELEMETRY) || defined(MULTI_STATUS) )
	#define MULTI_TIME				500	//in ms
	#define INPUT_SYNC_TIME			100	//in ms
	#define INPUT_ADDITIONAL_DELAY	100	// in 10µs, 100 => 1000 µs
	uint32_t lastMulti = 0;
#endif // MULTI_TELEMETRY/MULTI_STATUS

#if defined SPORT_TELEMETRY	
	#define SPORT_TIME 12000	//12ms
	#define FRSKY_SPORT_PACKET_SIZE   8
	#define FX_BUFFERS	4
	uint32_t last = 0;
	uint8_t sport_counter=0;
	uint8_t RxBt = 0;
	uint8_t sport = 0;
	uint8_t pktx1[FRSKY_SPORT_PACKET_SIZE*FX_BUFFERS];

	// Store for out of sequence packet
	uint8_t FrskyxRxTelemetryValidSequence ;
	struct t_fx_rx_frame
	{
		uint8_t valid ;
		uint8_t count ;
		uint8_t payload[6] ;
	} ;

	// Store for FrskyX telemetry
	struct t_fx_rx_frame FrskyxRxFrames[4] ;
	uint8_t NextFxFrameToForward ;
	#ifdef SPORT_POLLING
		uint8_t sport_rx_index[28] ;
		uint8_t ukindex ;
		uint8_t kindex ;
		uint8_t TxData[2];
		uint8_t SportIndexPolling;
		uint8_t RxData[16] ;
		volatile uint8_t RxIndex=0 ;
		uint8_t sport_bytes=0;
		uint8_t skipped_id;
		uint8_t rx_counter=0;
	#endif
#endif // SPORT_TELEMETRY

#if defined HUB_TELEMETRY
	#define USER_MAX_BYTES 6
	uint8_t prev_index;
#endif // HUB_TELEMETRY

#define START_STOP	0x7e
#define BYTESTUFF	0x7d
#define STUFF_MASK	0x20
#define MAX_PKTX	10
uint8_t pktx[MAX_PKTX];
uint8_t indx;
uint8_t frame[18];

#if ( defined(MULTI_TELEMETRY) || defined(MULTI_STATUS) )
static void multi_send_header(uint8_t type, uint8_t len)
{
	Serial_write('M');
	#ifdef MULTI_TELEMETRY
		Serial_write('P');
		Serial_write(type);
	#else
		(void)type;
	#endif
	Serial_write(len);
}

static void multi_send_status()
{
	#ifdef SPORT_POLLING
	#ifdef INVERT_SERIAL
		USART3_BASE->CR1 &= ~USART_CR1_TE ;
		TX_INV_on;	//activate inverter for both serial TX and RX signals
		USART3_BASE->CR1 |= USART_CR1_TE ;
	#endif
		rx_pause();
	#endif
	multi_send_header(MULTI_TELEMETRY_STATUS, 5);

	// Build flags
	uint8_t flags=0;
	if (IS_INPUT_SIGNAL_on)
		flags |= 0x01;
	if (mode_select==MODE_SERIAL)
		flags |= 0x02;
	if (remote_callback != 0)
	{
		flags |= 0x04;
		if (IS_WAIT_BIND_on)
			flags |= 0x10;
		else
			if (IS_BIND_IN_PROGRESS)
				flags |= 0x08;
		#ifdef FAILSAFE_ENABLE
			//Is failsafe supported?
			switch (protocol)
			{
				case PROTO_HISKY:
					if(sub_protocol!=HK310)
						break;
				case PROTO_AFHDS2A:
				case PROTO_DEVO:
				case PROTO_SFHSS:
				case PROTO_WK2x01:
				case PROTO_FRSKYX:
					flags |= 0x20;	//Yes
				default:
					break;
			}
		#endif
	}
	Serial_write(flags);

	// Version number example: 1.1.6.1
	Serial_write(VERSION_MAJOR);
	Serial_write(VERSION_MINOR);
	Serial_write(VERSION_REVISION);
	Serial_write(VERSION_PATCH_LEVEL);
}
#endif

#ifdef DSM_TELEMETRY
	#ifdef MULTI_TELEMETRY
		void DSM_frame()
		{
			if (pkt[0] == 0x80)
			{
				multi_send_header(MULTI_TELEMETRY_DSMBIND, 10);
				for (uint8_t i = 1; i < 11; i++) 	// 10 bytes of DSM bind response
					Serial_write(pkt[i]);

			}
			else
			{
				multi_send_header(MULTI_TELEMETRY_DSM, 17);
				for (uint8_t i = 0; i < 17; i++)	// RSSI value followed by 16 bytes of telemetry data
					Serial_write(pkt[i]);
			}
		}
	#else
		void DSM_frame()
		{
			Serial_write(0xAA);						// Telemetry packet
			for (uint8_t i = 0; i < 17; i++)		// RSSI value followed by 16 bytes of telemetry data
				Serial_write(pkt[i]);
		}
	#endif
#endif

#ifdef AFHDS2A_FW_TELEMETRY
	void AFHDSA_short_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_AFHDS2A, 29);
		#else
			Serial_write(0xAA);						// Telemetry packet
		#endif
		for (uint8_t i = 0; i < 29; i++)			// RSSI value followed by 4*7 bytes of telemetry data
			Serial_write(pkt[i]);
	}
#endif

#ifdef HITEC_FW_TELEMETRY
	void HITEC_short_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_HITEC, 8);
		#else
			Serial_write(0xAA);					// Telemetry packet
		#endif
		for (uint8_t i = 0; i < 8; i++)			// TX RSSI and TX LQI values followed by frame number and 5 bytes of telemetry data
			Serial_write(pkt[i]);
	}
#endif

#ifdef MULTI_TELEMETRY
static void multi_send_frskyhub()
{
	multi_send_header(MULTI_TELEMETRY_HUB, 9);
	for (uint8_t i = 0; i < 9; i++)
		Serial_write(frame[i]);
}
#endif

void frskySendStuffed()
{
	Serial_write(START_STOP);
	for (uint8_t i = 0; i < 9; i++)
	{
		if ((frame[i] == START_STOP) || (frame[i] == BYTESTUFF))
		{
			Serial_write(BYTESTUFF);
			frame[i] ^= STUFF_MASK;
		}
		Serial_write(frame[i]);
	}
	Serial_write(START_STOP);
}

void frsky_check_telemetry(uint8_t *pkt,uint8_t len)
{
	uint8_t clen = pkt[0] + 3 ;
	if(pkt[1] == rx_tx_addr[3] && pkt[2] == rx_tx_addr[2] && len == clen )
	{
		telemetry_link|=1;								// Telemetry data is available
		TX_RSSI = pkt[len-2];
		if(TX_RSSI >=128)
			TX_RSSI -= 128;
		else
			TX_RSSI += 128;
		TX_LQI = pkt[len-1]&0x7F;
		for (uint8_t i=3;i<len-2;i++)
			pktt[i]=pkt[i];								// Buffer telemetry values to be sent 
		
		if(pktt[6]>0 && pktt[6]<=10)
		{
			if (protocol==PROTO_FRSKYD)
			{
				if ( ( pktt[7] & 0x1F ) == (telemetry_counter & 0x1F) )
				{
					uint8_t topBit = 0 ;
					if ( telemetry_counter & 0x80 )
						if ( ( telemetry_counter & 0x1F ) != RetrySequence )
							topBit = 0x80 ;
					telemetry_counter = ( (telemetry_counter+1)%32 ) | topBit ;	// Request next telemetry frame
				}
				else
				{
					// incorrect sequence
					RetrySequence = pktt[7] & 0x1F ;
					telemetry_counter |= 0x80 ;
					pktt[6]=0 ;							// Discard current packet and wait for retransmit
				}
			}
		}
		else
			pktt[6]=0; 									// Discard packet
		//
#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
		telemetry_lost=0;
		if (protocol==PROTO_FRSKYX)
		{
			uint16_t lcrc = frskyX_crc_x(&pkt[3], len-7 ) ;

			if ( ( (lcrc >> 8) == pkt[len-4]) && ( (lcrc & 0x00FF ) == pkt[len-3]) )
			{
				// Check if in sequence
				if ( (pkt[5] & 0x0F) == 0x08 )
				{
					FrX_receive_seq = 0x08 ;
					NextFxFrameToForward = 0 ;
					FrskyxRxFrames[0].valid = 0 ;
					FrskyxRxFrames[1].valid = 0 ;
					FrskyxRxFrames[2].valid = 0 ;
					FrskyxRxFrames[3].valid = 0 ;
				}
				else if ( (pkt[5] & 0x03) == (FrX_receive_seq & 0x03 ) )
				{
					// OK to process
					struct t_fx_rx_frame *p ;
					uint8_t count ;
					p = &FrskyxRxFrames[FrX_receive_seq & 3] ;
					count = pkt[6] ;
					if ( count <= 6 )
					{
						p->count = count ;
						for ( uint8_t i = 0 ; i < count ; i += 1 )
							p->payload[i] = pkt[i+7] ;
					}
					else
						p->count = 0 ;
					p->valid = 1 ;
		
					FrX_receive_seq = ( FrX_receive_seq + 1 ) & 0x03 ;

					if ( FrskyxRxTelemetryValidSequence & 0x80 )
					{
						FrX_receive_seq = ( FrskyxRxTelemetryValidSequence + 1 ) & 3 ;
						FrskyxRxTelemetryValidSequence &= 0x7F ;
					}

				}
				else
				{
					// Save and request correct packet
					struct t_fx_rx_frame *q ;
					uint8_t count ;
					// pkt[4] RSSI
					// pkt[5] sequence control
					// pkt[6] payload count
					// pkt[7-12] payload			
					pktt[6] = 0 ; // Don't process
					if ( (pkt[5] & 0x03) == ( ( FrX_receive_seq +1 ) & 3 ) )
					{
						q = &FrskyxRxFrames[(pkt[5] & 0x03)] ;
						count = pkt[6] ;
						if ( count <= 6 )
						{
							q->count = count ;
							for ( uint8_t i = 0 ; i < count ; i += 1 )
							{
								q->payload[i] = pkt[i+7] ;
							}
						}
						else
							q->count = 0 ;
						q->valid = 1 ;
					
						FrskyxRxTelemetryValidSequence = 0x80 | ( pkt[5] & 0x03 ) ;
					}
					 
					 FrX_receive_seq = ( FrX_receive_seq & 0x03 ) | 0x04 ;	// Request re-transmission
				}

				if (((pktt[5] >> 4) & 0x0f) == 0x08)
					FrX_send_seq = 0 ;
			}
		}
#endif
	}
}

void init_frskyd_link_telemetry()
{
	telemetry_link=0;
	telemetry_counter=0;
	v_lipo1=0;
	v_lipo2=0;
	RX_RSSI=0;
	TX_RSSI=0;
	RX_LQI=0;
	TX_LQI=0;
}

void frsky_link_frame()
{
	frame[0] = 0xFE;			// Link frame
	if (protocol==PROTO_FRSKYD)
	{		
		frame[1] = pktt[3];		// A1
		frame[2] = pktt[4];		// A2
		frame[3] = pktt[5];		// RX_RSSI
		telemetry_link &= ~1 ;		// Sent
		telemetry_link |= 2 ;		// Send hub if available
	}
	else
		if (protocol==PROTO_HUBSAN||protocol==PROTO_AFHDS2A||protocol==PROTO_BAYANG||protocol==PROTO_NCC1701||protocol==PROTO_CABELL||protocol==PROTO_HITEC||protocol==PROTO_BUGS||protocol==PROTO_BUGSMINI)
		{	
			frame[1] = v_lipo1;
			frame[2] = v_lipo2;
			frame[3] = RX_RSSI;
			telemetry_link=0;
		}
	frame[4] = TX_RSSI;
	frame[5] = RX_LQI;
	frame[6] = TX_LQI;
	frame[7] = frame[8] = 0;
	#if defined MULTI_TELEMETRY
		multi_send_frskyhub();
	#else
		frskySendStuffed();
	#endif
}

#if defined HUB_TELEMETRY
void frsky_user_frame()
{
	if(pktt[6])
	{//only send valid hub frames
		frame[0] = 0xFD;				// user frame
		if(pktt[6]>USER_MAX_BYTES)
		{
			frame[1]=USER_MAX_BYTES;	// packet size
			pktt[6]-=USER_MAX_BYTES;
			telemetry_link |= 2 ;			// 2 packets need to be sent
		}
		else
		{
			frame[1]=pktt[6];			// packet size
			telemetry_link=0;			// only 1 packet or processing second packet
		}
		frame[2] = pktt[7];
		for(uint8_t i=0;i<USER_MAX_BYTES;i++)
			frame[i+3]=pktt[i+8];
		if(telemetry_link & 2)				// prepare the content of second packet
			for(uint8_t i=8;i<USER_MAX_BYTES+8;i++)
				pktt[i]=pktt[i+USER_MAX_BYTES];
		#if defined MULTI_TELEMETRY
			multi_send_frskyhub();
		#else
			frskySendStuffed();
		#endif
	}
	else
		telemetry_link=0;
}
/*
HuB RX packets.
pkt[6]|(counter++)|00 01 02 03 04 05 06 07 08 09 
        %32        
01     08          5E 28 12 00 5E 5E 3A 06 00 5E
0A     09          28 12 00 5E 5E 3A 06 00 5E 5E  
09     0A          3B 09 00 5E 5E 06 36 7D 5E 5E 
03     0B          5E 28 11 00 5E 5E 06 06 6C 5E
0A     0C          00 5E 5E 3A 06 00 5E 5E 3B 09
07     0D          00 5E 5E 06 06 6C 5E 16 72 5E
05     0E          5E 28 11 00 5E 5E 3A 06 00 5E
0A     0F          5E 3A 06 00 5E 5E 3B 09 00 5E
05     10          5E 06 16 72 5E 5E 3A 06 00 5E
*/
#endif


#if defined SPORT_TELEMETRY
/* SPORT details serial
			100K 8E2 normal-multiprotocol
			-every 12ms-or multiple of 12; %36
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
			
			
			Telemetry frames(RF) SPORT info 
			15 bytes payload
			SPORT frame valid 6+3 bytes
			[00] PKLEN  0E 0E 0E 0E 
			[01] TXID1  DD DD DD DD 
			[02] TXID2  6D 6D 6D 6D 
			[03] CONST  02 02 02 02 
			[04] RS/RB  2C D0 2C CE	//D0;CE=2*RSSI;....2C = RX battery voltage(5V from Bec)
			[05] HD-SK  03 10 21 32	//TX/RX telemetry hand-shake bytes
			[06] NO.BT  00 00 06 03	//No.of valid SPORT frame bytes in the frame		
			[07] STRM1  00 00 7E 00 
			[08] STRM2  00 00 1A 00 
			[09] STRM3  00 00 10 00 
			[10] STRM4  03 03 03 03  
			[11] STRM5  F1 F1 F1 F1 
			[12] STRM6  D1 D1 D0 D0
			[13] CHKSUM1 --|2 CRC bytes sent by RX (calculated on RX side crc16/table)
			[14] CHKSUM2 --|
			+2	appended bytes automatically  RSSI and LQI/CRC bytes(len=0x0E+3);
			
0x06	0x06	0x06	0x06	0x06

0x7E	0x00	0x03	0x7E	0x00
0x1A	0x00	0xF1	0x1A	0x00
0x10	0x00	0xD7	0x10	0x00
0x03	0x7E	0x00	0x03	0x7E
0xF1	0x1A	0x00	0xF1	0x1A
0xD7	0x10	0x00	0xD7	0x10

0xE1	0x1C	0xD0	0xEE	0x33
0x34	0x0A	0xC3	0x56	0xF3
				
		*/
#if defined SPORT_POLLING || defined MULTI_TELEMETRY
const uint8_t PROGMEM Indices[] = {	0x00, 0xA1, 0x22, 0x83, 0xE4, 0x45,
									0xC6, 0x67, 0x48, 0xE9, 0x6A, 0xCB,
									0xAC, 0x0D, 0x8E, 0x2F, 0xD0, 0x71,
									0xF2, 0x53, 0x34, 0x95, 0x16, 0xB7,
									0x98, 0x39, 0xBA, 0x1B } ;
#endif

#ifdef MULTI_TELEMETRY
	void sportSend(uint8_t *p)
	{
	#ifdef SPORT_POLLING
	#ifdef INVERT_SERIAL
		USART3_BASE->CR1 &= ~USART_CR1_TE ;
		TX_INV_on;	//activate inverter for both serial TX and RX signals
		USART3_BASE->CR1 |= USART_CR1_TE ;
	#endif
	#endif
		multi_send_header(MULTI_TELEMETRY_SPORT, 9);
		uint16_t crc_s = 0;
		uint8_t x = p[0] ;
		if ( x <= 0x1B )
			x = pgm_read_byte_near( &Indices[x] ) ;
		Serial_write(x) ;
		for (uint8_t i = 1; i < 9; i++)
		{
			if (i == 8)
				p[i] = 0xff - crc_s;
				Serial_write(p[i]);

			if (i>0)
			{
				crc_s += p[i];			//0-1FF
				crc_s += crc_s >> 8;	//0-100
				crc_s &= 0x00ff;
			}
		}
	}
#else
	void sportSend(uint8_t *p)
	{
		uint16_t crc_s = 0;
	#ifdef SPORT_POLLING
	#ifdef INVERT_SERIAL
		USART3_BASE->CR1 &= ~USART_CR1_TE ;
		TX_INV_on;	//activate inverter for both serial TX and RX signals
		USART3_BASE->CR1 |= USART_CR1_TE ;
	#endif
	#endif
		Serial_write(START_STOP);//+9
		Serial_write(p[0]) ;
		for (uint8_t i = 1; i < 9; i++)
		{
			if (i == 8)
				p[i] = 0xff - crc_s;
			
			if ((p[i] == START_STOP) || (p[i] == BYTESTUFF))
			{
				Serial_write(BYTESTUFF);//stuff again
				Serial_write(STUFF_MASK ^ p[i]);
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
	
#endif

#if defined  SPORT_POLLING
uint8_t nextID()
{
	uint8_t i ;
	uint8_t poll_idx ; 
	if (phase)
	{
		poll_idx = 99 ;
		for ( i = 0 ; i < 28 ; i++ )
		{
			if ( sport_rx_index[kindex] )
			{
				poll_idx = kindex ;
			}
			kindex++ ;
			if ( kindex>= 28 )
			{
				kindex = 0 ;
				phase = 0 ;
				break ;
			}
			if ( poll_idx != 99 )
			{
				break ;
			}
		}
		if ( poll_idx != 99 )
		{
			return poll_idx ;
		}
	}
	if ( phase == 0 )
	{
		for ( i = 0 ; i < 28 ; i++ )
		{
			if ( sport_rx_index[ukindex] == 0 )
			{
				poll_idx = ukindex ;
				phase = 1 ;
			}
			ukindex++;
			if (ukindex >= 28 )
			{
				ukindex = 0 ;
			}
			if ( poll_idx != 99 )
			{
				return poll_idx ;
			}
		}
		if ( poll_idx == 99 )
		{
			phase = 1 ;
			return 0 ;
		}
	}
	return poll_idx ;
}

#ifdef INVERT_SERIAL
void start_timer4()
{
	TIMER4_BASE->PSC = 71;								// 72-1;for 72 MHZ / 1.0sec/(71+1)
	TIMER4_BASE->CCER = 0 ;
	TIMER4_BASE->DIER = 0 ;
	TIMER4_BASE->CCMR1 = 0 ;
	TIMER4_BASE->CCMR1 = TIMER_CCMR1_OC1M ;
	HWTimer4.attachInterrupt(TIMER_CH1, __irq_timer4);		// Assign function to Timer2/Comp2 interrupt
	nvic_irq_set_priority( NVIC_TIMER4, 14 ) ;
}

void stop_timer4()
{
	TIMER5_BASE->CR1 = 0 ;
	nvic_irq_disable( NVIC_TIMER4 ) ;
}

void __irq_timer4(void)			
{
	TIMER4_BASE->DIER = 0 ;
	TIMER4_BASE->CR1 = 0 ;
	TX_INV_on;	//activate inverter for both serial TX and RX signals
}

#endif

void pollSport()
{
	uint8_t pindex = nextID() ;
	TxData[0]  = START_STOP;
	TxData[1] = pgm_read_byte_near(&Indices[pindex]) ;
	if(!telemetry_lost && ((TxData[1] &0x1F)== skipped_id ||TxData[1]==0x98))
	{//98 ID(RSSI/RxBat and SWR ) and ID's from sport telemetry
		pindex = nextID() ;	
		TxData[1] = pgm_read_byte_near(&Indices[pindex]);
	}		
	SportIndexPolling = pindex ;
	RxIndex = 0;
	#ifdef INVERT_SERIAL
		USART3_BASE->CR1 &= ~USART_CR1_TE ;
		TX_INV_on;	//activate inverter for both serial TX and RX signals
		USART3_BASE->CR1 |= USART_CR1_TE ;
	#endif
#ifdef MULTI_TELEMETRY
	multi_send_header(MULTI_TELEMETRY_SPORT_POLLING, 1);
#else
    Serial_write(TxData[0]);
#endif
	RxIndex=0;
	Serial_write(TxData[1]);
	USART3_BASE->CR1 |= USART_CR1_TCIE ;
#ifdef INVERT_SERIAL
	TIMER4_BASE->CNT = 0 ;
	TIMER4_BASE->CCR1 = 3000 ;
	TIMER4_BASE->DIER = TIMER_DIER_CC1IE ;
	TIMER4_BASE->CR1 = TIMER_CR1_CEN ;
#endif
}

bool checkSportPacket()
{
	uint8_t *packet = RxData ;
	uint16_t crc = 0 ;
	if ( RxIndex < 8 )
		return 0 ;
	for ( uint8_t i = 0 ; i<8 ; i += 1 )
	{
		crc += packet[i]; 
		crc += crc >> 8; 
		crc &= 0x00ff;
	}
	return (crc == 0x00ff) ;
}

uint8_t unstuff()
{
	uint8_t i ;
	uint8_t j ;
	j = 0 ;
	for ( i = 0 ; i < RxIndex ; i += 1 )
	{
		if ( RxData[i] == BYTESTUFF )
		{
			i += 1 ;
			RxData[j] = RxData[i] ^ STUFF_MASK ; ;
		}
		else
			RxData[j] = RxData[i] ;
		j += 1 ;
	}
	return j ;
}

void processSportData(uint8_t *p)
{	

	RxIndex = unstuff() ;
	uint8_t x=checkSportPacket() ;
	if (x)
	{
		SportData[sport_idx]=0x7E;
		sport_idx =(sport_idx+1) & (MAX_SPORT_BUFFER-1);
		SportData[sport_idx]=TxData[1]&0x1F;
		sport_idx =(sport_idx+1) & (MAX_SPORT_BUFFER-1);	
		
		for(uint8_t i=0;i<(RxIndex-1);i++)
		{//no crc		
			if(p[i]==START_STOP || p[i]==BYTESTUFF)
			{//stuff back
				SportData[sport_idx]=BYTESTUFF;
				sport_idx =(sport_idx+1) & (MAX_SPORT_BUFFER-1);
				SportData[sport_idx]=p[i]^STUFF_MASK;
			}
			else
				SportData[sport_idx]=p[i];
			sport_idx =(sport_idx+1) & (MAX_SPORT_BUFFER-1);
		}
		sport_rx_index[SportIndexPolling] = 1 ;	
		ok_to_send=true;
		RxIndex =0 ; 
	}
}

inline void rx_pause()
{
	USART3_BASE->CR1 &= ~ USART_CR1_RXNEIE;	//disable rx interrupt on USART3	
}
inline void rx_resume()
{
	USART3_BASE->CR1 |= USART_CR1_RXNEIE;	//enable rx interrupt on USART3
}	
#endif//end SPORT_POLLING

void sportIdle()
{
	#if !defined MULTI_TELEMETRY
		Serial_write(START_STOP);
	#endif
}	

void sportSendFrame()
{
	#if defined SPORT_POLLING
		rx_pause();
	#endif
	uint8_t i;
	sport_counter = (sport_counter + 1) %36;
	if(telemetry_lost)
	{
		#ifdef SPORT_POLLING
			pollSport();
		#else
			sportIdle();
         #endif
		return;
	}
	if(sport_counter<6)
	{
		frame[0] = 0x98;
		frame[1] = 0x10;
		for (i=5;i<8;i++)
			frame[i]=0;
	}
	switch (sport_counter)
	{
		case 0:
			frame[2] = 0x05;
			frame[3] = 0xf1;
			frame[4] = 0x02 ;//dummy values if swr 20230f00
			frame[5] = 0x23;
			frame[6] = 0x0F;
			break;
		case 2: // RSSI
			frame[2] = 0x01;
			frame[3] = 0xf1;
			frame[4] = RX_RSSI;
			frame[5] = TX_RSSI;
			frame[6] = RX_LQI;
			frame[7] = TX_LQI;
			break;
		case 4: //BATT
			frame[2] = 0x04;
			frame[3] = 0xf1;
			frame[4] = RxBt;//a1;
			break;								
		default:
			if(sport)
			{	
				for (i=0;i<FRSKY_SPORT_PACKET_SIZE;i++)
				frame[i]=pktx1[i];
				sport -= 1 ;
				#ifdef SPORT_POLLING
					skipped_id=frame[0];
				#endif
				if ( sport )
				{
					uint8_t j = sport * FRSKY_SPORT_PACKET_SIZE ;
					for (i=0;i<j;i++)
					pktx1[i] = pktx1[i+FRSKY_SPORT_PACKET_SIZE] ;
				}
				break;
			}
			else
			{
				#ifdef SPORT_POLLING
					pollSport();
				#else
					sportIdle();
				#endif
				return;
			}		
	}
	sportSend(frame);
}	

void proces_sport_data(uint8_t data)
{
	switch (pass)
	{
		case 0:
			if (data == START_STOP)
			{//waiting for 0x7e
				indx = 0;
				pass = 1;
			}
			break;		
		case 1:
			if (data == START_STOP)	// Happens if missed packet
			{//waiting for 0x7e
				indx = 0;
				pass = 1;
				break;		
			}
			if(data == BYTESTUFF)	//if they are stuffed
				pass=2;
			else
				if (indx < MAX_PKTX)		
					pktx[indx++] = data;		
			break;
		case 2:	
			if (indx < MAX_PKTX)	
				pktx[indx++] = data ^ STUFF_MASK;	//unstuff bytes	
			pass=1;
			break;	
	} // end switch
	if (indx >= FRSKY_SPORT_PACKET_SIZE)
	{//8 bytes no crc 
		if ( sport < FX_BUFFERS )
		{
			uint8_t dest = sport * FRSKY_SPORT_PACKET_SIZE ;
			uint8_t i ;
			for ( i = 0 ; i < FRSKY_SPORT_PACKET_SIZE ; i += 1 )
				pktx1[dest++] = pktx[i] ;	// Triple buffer
			sport += 1 ;//ok to send
		}
//		else
//		{
//			// Overrun
//		}
		pass = 0;//reset
	}
}

#endif

void TelemetryUpdate()
{
	// check for space in tx buffer
	#ifdef BASH_SERIAL
		uint8_t h ;
		uint8_t t ;
		h = SerialControl.head ;
		t = SerialControl.tail ;
		if ( h >= t )
			t += TXBUFFER_SIZE - h ;
		else
			t -= h ;
		if ( t < 64 )
		{
			return ;
		}
	#else
		uint8_t h ;
		uint8_t t ;
		h = tx_head ;
		t = tx_tail ;
		if ( h >= t )
			t += TXBUFFER_SIZE - h ;
		else
			t -= h ;
		if ( t < 32 )
		{
			return ;
		}
	#endif
	#if ( defined(MULTI_TELEMETRY) || defined(MULTI_STATUS) )
		{
			uint32_t now = millis();
			if ((now - lastMulti) > MULTI_TIME)
			{
				multi_send_status();
				lastMulti = now;
				return;
			}
		}
	#endif
	 
	#if defined SPORT_TELEMETRY
		if (protocol==PROTO_FRSKYX)
		{	// FrSkyX
			for(;;)
			{
				struct t_fx_rx_frame *p ;
				uint8_t count ;
				p = &FrskyxRxFrames[NextFxFrameToForward] ;
				if ( p->valid )
				{
					count = p->count ;
					for (uint8_t i=0; i < count ; i++)
						proces_sport_data(p->payload[i]) ;
					p->valid = 0 ;	// Sent on
					NextFxFrameToForward = ( NextFxFrameToForward + 1 ) & 3 ;
				}
				else
				{
					break ;
				}
			}
			 
			if(telemetry_link)
			{		
				if(pktt[4] & 0x80)
					RX_RSSI=pktt[4] & 0x7F ;
				else 
					RxBt = (pktt[4]<<1) + 1 ;
				telemetry_link=0;
			}
			uint32_t now = micros();
			if ((now - last) > SPORT_TIME)
			{
				#if defined SPORT_POLLING
					processSportData(RxData);	//process arrived data before polling
				#endif
				sportSendFrame();
				#ifdef STM32_BOARD
					last=now;
				#else
					last += SPORT_TIME ;
				#endif
			}
		}
	#endif // SPORT_TELEMETRY

	#if defined DSM_TELEMETRY
		if(telemetry_link && protocol == PROTO_DSM)
		{	// DSM
			DSM_frame();
			telemetry_link=0;
			return;
		}
	#endif
	#if defined AFHDS2A_FW_TELEMETRY
		if(telemetry_link == 2 && protocol == PROTO_AFHDS2A)
		{
			AFHDSA_short_frame();
			telemetry_link=0;
			return;
		}
	#endif
	#if defined HITEC_FW_TELEMETRY
		if(telemetry_link == 2 && protocol == PROTO_HITEC)
		{
			HITEC_short_frame();
			telemetry_link=0;
			return;
		}
	#endif

		if((telemetry_link & 1 )&& protocol != PROTO_FRSKYX)
		{	// FrSkyD + Hubsan + AFHDS2A + Bayang + Cabell + Hitec + Bugs + BugsMini + NCC1701
			frsky_link_frame();
			return;
		}
	#if defined HUB_TELEMETRY
		if((telemetry_link & 2) && protocol == PROTO_FRSKYD)
		{	// FrSkyD
			frsky_user_frame();
			return;
		}
	#endif
}


/**************************/
/**************************/
/**  Serial TX routines  **/
/**************************/
/**************************/

#ifndef BASH_SERIAL
	// Routines for normal serial output
	void Serial_write(uint8_t data)
	{
		uint8_t nextHead ;
		nextHead = tx_head + 1 ;
		if ( nextHead >= TXBUFFER_SIZE )
			nextHead = 0 ;
		tx_buff[nextHead]=data;
		tx_head = nextHead ;
		tx_resume();
	}

	void initTXSerial( uint8_t speed)
	{
		#ifdef ENABLE_PPM
			if(speed==SPEED_9600)
			{ // 9600
				#ifdef ORANGE_TX
					USARTC0.BAUDCTRLA = 207 ;
					USARTC0.BAUDCTRLB = 0 ;
					USARTC0.CTRLB = 0x18 ;
					USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
					USARTC0.CTRLC = 0x03 ;
				#else
					#ifdef STM32_BOARD
						usart3_begin(9600,SERIAL_8N1);		//USART3 
						USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
					#else
						UBRR0H = 0x00;
						UBRR0L = 0x67;
						UCSR0A = 0 ;						// Clear X2 bit
						//Set frame format to 8 data bits, none, 1 stop bit
						UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
					#endif
				#endif
			}
			else if(speed==SPEED_57600)
			{ // 57600
				#ifdef ORANGE_TX
					/*USARTC0.BAUDCTRLA = 207 ;
					USARTC0.BAUDCTRLB = 0 ;
					USARTC0.CTRLB = 0x18 ;
					USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
					USARTC0.CTRLC = 0x03 ;*/
				#else
					#ifdef STM32_BOARD
						usart3_begin(57600,SERIAL_8N1);		//USART3 
						USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
					#else
						UBRR0H = 0x00;
						UBRR0L = 0x22;
						UCSR0A = 0x02 ;	// Set X2 bit
						//Set frame format to 8 data bits, none, 1 stop bit
						UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
					#endif
				#endif
			}
			else if(speed==SPEED_125K)
			{ // 125000
				#ifdef ORANGE_TX
					/*USARTC0.BAUDCTRLA = 207 ;
					USARTC0.BAUDCTRLB = 0 ;
					USARTC0.CTRLB = 0x18 ;
					USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
					USARTC0.CTRLC = 0x03 ;*/
				#else
					#ifdef STM32_BOARD
						usart3_begin(125000,SERIAL_8N1);	//USART3 
						USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable RX leave TX enabled
					#else
						UBRR0H = 0x00;
						UBRR0L = 0x07;
						UCSR0A = 0x00 ;	// Clear X2 bit
						//Set frame format to 8 data bits, none, 1 stop bit
						UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
					#endif
				#endif
			}
		#else
			(void)speed;
		#endif
		#ifndef ORANGE_TX
			#ifndef STM32_BOARD
				UCSR0B |= (1<<TXEN0);//tx enable
			#endif
		#endif
	}

	//Serial TX
	#ifdef ORANGE_TX
		ISR(USARTC0_DRE_vect)
	#else
		#ifdef STM32_BOARD
			void __irq_usart3()			
		#else
			ISR(USART_UDRE_vect)
		#endif
	#endif
	{	// Transmit interrupt
		#ifdef STM32_BOARD
			#ifdef SPORT_POLLING		
				if(USART3_BASE->SR & USART_SR_TC) 
				{
					if ( USART3_BASE->CR1 & USART_CR1_TCIE )
					{
						USART3_BASE->CR1 &= ~USART_CR1_TCIE ;
						TX_INV_off;
					}
				}

				if(USART3_BASE->SR & USART_SR_RXNE) 
				{
					USART3_BASE->SR &= ~USART_SR_RXNE;
					if (RxIndex < 16 )
					{
						if(RxData[0]==TxData[0] && RxData[1]==TxData[1])
							RxIndex=0;
						RxData[RxIndex++] = USART3_BASE->DR & 0xFF ;					
					}
				}
			#endif
			if(USART3_BASE->SR & USART_SR_TXE)
			{
		#endif
				if(tx_head!=tx_tail)
				{
					if(++tx_tail>=TXBUFFER_SIZE)//head 
						tx_tail=0;
					#ifdef STM32_BOARD	
						USART3_BASE->DR=tx_buff[tx_tail];//clears TXE bit				
					#else
						UDR0=tx_buff[tx_tail];
					#endif
				}
				if (tx_tail == tx_head)
				{
					tx_pause(); // Check if all data is transmitted . if yes disable transmitter UDRE interrupt
					#ifdef  SPORT_POLLING
						rx_resume();
					#endif
				}
		#ifdef STM32_BOARD	
			}
		#endif		
	}
#else	//BASH_SERIAL
// Routines for bit-bashed serial output

// Speed is 0 for 100K and 1 for 9600
void initTXSerial( uint8_t speed)
{
	TIMSK0 = 0 ;	// Stop all timer 0 interrupts
	#ifdef INVERT_SERIAL
		SERIAL_TX_off;
	#else
		SERIAL_TX_on;
	#endif
	UCSR0B &= ~(1<<TXEN0) ;

	SerialControl.speed = speed ;
	if ( speed == SPEED_9600 )
	{
		OCR0A = 207 ;	// 104uS period
		TCCR0A = 3 ;
		TCCR0B = 0x0A ; // Fast PMM, 2MHz
	}
	else	// 100K
	{
		TCCR0A = 0 ;
		TCCR0B = 2 ;	// Clock/8 (0.5uS)
	}
}

void Serial_write( uint8_t byte )
{
	uint8_t temp ;
	uint8_t temp1 ;
	uint8_t byteLo ;

	#ifdef INVERT_SERIAL
		byte = ~byte ;
	#endif

	byteLo = byte ;
	byteLo >>= 7 ;		// Top bit
	if ( SerialControl.speed == SPEED_100K )
	{
		#ifdef INVERT_SERIAL
				byteLo |= 0x02 ;	// Parity bit
		#else
				byteLo |= 0xFC ;	// Stop bits
		#endif
		// calc parity
		temp = byte ;
		temp >>= 4 ;
		temp = byte ^ temp ;
		temp1 = temp ;
		temp1 >>= 2 ;
		temp = temp ^ temp1 ;
		temp1 = temp ;
		temp1 <<= 1 ;
		temp ^= temp1 ;
		temp &= 0x02 ;
		#ifdef INVERT_SERIAL
				byteLo ^= temp ;
		#else	
				byteLo |= temp ;
		#endif
	}
	else
	{
		byteLo |= 0xFE ;	// Stop bit
	}
	byte <<= 1 ;
	#ifdef INVERT_SERIAL
		byte |= 1 ;		// Start bit
	#endif
	uint8_t next = SerialControl.head + 2;
	if(next>=TXBUFFER_SIZE)
		next=0;
	if ( next != SerialControl.tail )
	{
		SerialControl.data[SerialControl.head] = byte ;
		SerialControl.data[SerialControl.head+1] = byteLo ;
		SerialControl.head = next ;
	}
	if(!IS_TX_PAUSE_on)
		tx_resume();
}

void resumeBashSerial()
{
	cli() ;
	if ( SerialControl.busy == 0 )
	{
		sei() ;
		// Start the transmission here
		#ifdef INVERT_SERIAL
			GPIOR2 = 0 ;
		#else
			GPIOR2 = 0x01 ;
		#endif
		if ( SerialControl.speed == SPEED_100K )
		{
			GPIOR1 = 1 ;
			OCR0B = TCNT0 + 40 ;
			OCR0A = OCR0B + 210 ;
			TIFR0 = (1<<OCF0A) | (1<<OCF0B) ;
			TIMSK0 |= (1<<OCIE0B) ;
			SerialControl.busy = 1 ;
		}
		else
		{
			GPIOR1 = 1 ;
			TIFR0 = (1<<TOV0) ;
			TIMSK0 |= (1<<TOIE0) ;
			SerialControl.busy = 1 ;
		}
	}
	else
	{
		sei() ;
	}
}

// Assume timer0 at 0.5uS clock

ISR(TIMER0_COMPA_vect)
{
	uint8_t byte ;
	byte = GPIOR0 ;
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	GPIOR0 = byte ;
	if ( --GPIOR1 == 0 )
	{
		TIMSK0 &= ~(1<<OCIE0A) ;
		GPIOR1 = 3 ;
	}
	else
		OCR0A += 20 ;
}

ISR(TIMER0_COMPB_vect)
{
	uint8_t byte ;
	byte = GPIOR2 ;
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	GPIOR2 = byte ;
	if ( --GPIOR1 == 0 )
	{
		if ( IS_TX_PAUSE_on )
		{
			SerialControl.busy = 0 ;
			TIMSK0 &= ~(1<<OCIE0B) ;
		}
		else
		{
			// prepare next byte and allow for 2 stop bits
			volatile struct t_serial_bash *ptr = &SerialControl ;
			if ( ptr->head != ptr->tail )
			{
				GPIOR0 = ptr->data[ptr->tail] ;
				GPIOR2 = ptr->data[ptr->tail+1] ;
				uint8_t nextTail = ptr->tail + 2 ;
				if ( nextTail >= TXBUFFER_SIZE )
					nextTail = 0 ;
				ptr->tail = nextTail ;
				GPIOR1 = 8 ;
				OCR0A = OCR0B + 40 ;
				OCR0B = OCR0A + 8 * 20 ;
				TIMSK0 |= (1<<OCIE0A) ;
			}
			else
			{
				SerialControl.busy = 0 ;
				TIMSK0 &= ~(1<<OCIE0B) ;
			}
		}
	}
	else
		OCR0B += 20 ;
}

ISR(TIMER0_OVF_vect)
{
	uint8_t byte ;
	if ( GPIOR1 > 2 )
		byte = GPIOR0 ;
	else
		byte = GPIOR2 ;
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	if ( GPIOR1 > 2 )
		GPIOR0 = byte ;
	else
		GPIOR2 = byte ;
	if ( --GPIOR1 == 0 )
	{	// prepare next byte
		volatile struct t_serial_bash *ptr = &SerialControl ;
		if ( ptr->head != ptr->tail )
		{
			GPIOR0 = ptr->data[ptr->tail] ;
			GPIOR2 = ptr->data[ptr->tail+1] ;
			uint8_t nextTail = ptr->tail + 2 ;
			if ( nextTail >= TXBUFFER_SIZE )
				nextTail = 0 ;
			ptr->tail = nextTail ;
			GPIOR1 = 10 ;
		}
		else
		{
			SerialControl.busy = 0 ;
			TIMSK0 &= ~(1<<TOIE0) ;
		}
	}
}


#endif // BASH_SERIAL

#endif // TELEMETRY
