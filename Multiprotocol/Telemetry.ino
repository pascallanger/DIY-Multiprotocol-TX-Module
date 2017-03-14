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
	#define MULTI_TIME 500 //in ms
	uint32_t lastMulti = 0;
#endif

#if defined SPORT_TELEMETRY	
    #define SPORT_TIME 12000  //12ms
	#define FRSKY_SPORT_PACKET_SIZE   8
	uint32_t last = 0;
	uint8_t sport_counter=0;
	uint8_t RxBt = 0;
	uint8_t sport = 0;
#endif
#if defined HUB_TELEMETRY
	#define USER_MAX_BYTES 6
	uint8_t prev_index;
#endif

#define START_STOP              0x7e
#define BYTESTUFF               0x7d
#define STUFF_MASK              0x20
#define MAX_PKTX 10
uint8_t pktx[MAX_PKTX];
uint8_t pktx1[MAX_PKTX];
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
			if (!IS_BIND_DONE_on)
				flags |= 0x08;
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
	if(pkt[1] == rx_tx_addr[3] && pkt[2] == rx_tx_addr[2] && len ==(pkt[0] + 3))
	{	   
		telemetry_link|=1;								// Telemetry data is available
		/*previous version
		RSSI_dBm = (((uint16_t)(pktt[len-2])*18)>>4);
		if(pktt[len-2] >=128) RSSI_dBm -= 164;
		else RSSI_dBm += 130;*/
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
			if (protocol==MODE_FRSKYD)
			{
				if ( ( pktt[7] & 0x1F ) == (telemetry_counter & 0x1F) )
				{
					uint8_t topBit = 0 ;
					if ( telemetry_counter & 0x80 )
					{
						if ( ( telemetry_counter & 0x1F ) != RetrySequence )
						{
							topBit = 0x80 ;
						}
					}
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
		{
			pktt[6]=0; 									// Discard packet
		}
		//
#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
		telemetry_lost=0;
		if (protocol==MODE_FRSKYX)
		{
			if ((pktt[5] >> 4 & 0x0f) == 0x08)
			{  
				seq_last_sent = 8;
				seq_last_rcvd = 0;
				pass=0;
			} 
			else
			{
				if ((pktt[5] >> 4 & 0x03) == (seq_last_rcvd + 1) % 4)
					seq_last_rcvd = (seq_last_rcvd + 1) % 4;
				else
					pass=0;//reset if sequence wrong
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
	if (protocol==MODE_FRSKYD)
	{		
		frame[1] = pktt[3];		// A1
		frame[2] = pktt[4];		// A2
		frame[3] = pktt[5];		// RX_RSSI
		telemetry_link &= ~1 ;		// Sent
		telemetry_link |= 2 ;		// Send hub if available
	}
	else
		if (protocol==MODE_HUBSAN||protocol==MODE_AFHDS2A||protocol==MODE_BAYANG)
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
#ifdef MULTI_TELEMETRY
	void sportSend(uint8_t *p)
	{
		multi_send_header(MULTI_TELEMETRY_SPORT, 9);
		uint16_t crc_s = 0;
		Serial_write(p[0]) ;
		for (uint8_t i = 1; i < 9; i++)
		{
			if (i == 8)
				p[i] = 0xff - crc_s;
				Serial_write(p[i]);

			if (i>0)
			{
				crc_s += p[i]; //0-1FF
				crc_s += crc_s >> 8; //0-100
				crc_s &= 0x00ff;
			}
		}
	}
#else
	void sportSend(uint8_t *p)
	{
		uint16_t crc_s = 0;
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

void sportIdle()
{
	#if !defined MULTI_TELEMETRY
		Serial_write(START_STOP);
	#endif
}	

void sportSendFrame()
{
	uint8_t i;
	sport_counter = (sport_counter + 1) %36;
	if(telemetry_lost)
	{
		sportIdle();
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
				sport=0;
				break;
			}
			else
			{
				sportIdle();
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
			if(data == BYTESTUFF)//if they are stuffed
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
		if ( sport )
		{
			// overrun!
		}
		else
		{
			uint8_t i ;
			for ( i = 0 ; i < FRSKY_SPORT_PACKET_SIZE ; i += 1 )
			{
				pktx1[i] = pktx[i] ;	// Double buffer
			}
			sport = 1;//ok to send
		}
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
			t += 128 - h ;
		else
			t -= h ;
		if ( t < 64 )
			return ;
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
			return ;
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
		if (protocol==MODE_FRSKYX)
		{	// FrSkyX
			if(telemetry_link)
			{		
				if(pktt[4] & 0x80)
					RX_RSSI=pktt[4] & 0x7F ;
				else 
					RxBt = (pktt[4]<<1) + 1 ;
				if(pktt[6] && pktt[6]<=6)
					for (uint8_t i=0; i < pktt[6]; i++)
						proces_sport_data(pktt[7+i]);
				telemetry_link=0;
			}
			uint32_t now = micros();
			if ((now - last) > SPORT_TIME)
			{
				sportSendFrame();
				#ifdef STM32_BOARD
					last=now;
				#else
					last += SPORT_TIME ;
				#endif
			}
		}
	#endif					

	#if defined DSM_TELEMETRY
		if(telemetry_link && protocol == MODE_DSM)
		{	// DSM
			DSM_frame();
			telemetry_link=0;
			return;
		}
	#endif
    #if defined AFHDS2A_FW_TELEMETRY     
        if(telemetry_link == 2 && protocol == MODE_AFHDS2A)
		{
			AFHDSA_short_frame();
			telemetry_link=0;
			return;
		}
    #endif        

		if((telemetry_link & 1 )&& protocol != MODE_FRSKYX)
		{	// FrSkyD + Hubsan + AFHDS2A + Bayang
			frsky_link_frame();
			return;
		}
	#if defined HUB_TELEMETRY
		if((telemetry_link & 2) && protocol == MODE_FRSKYD)
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
			tx_pause(); // Check if all data is transmitted . if yes disable transmitter UDRE interrupt
		#ifdef STM32_BOARD	
			}
		#endif		
	}
	#ifdef STM32_BOARD
		void usart2_begin(uint32_t baud,uint32_t config )
		{
			usart_init(USART2); 
			usart_config_gpios_async(USART2,GPIOA,PIN_MAP[PA3].gpio_bit,GPIOA,PIN_MAP[PA2].gpio_bit,config);
			usart_set_baud_rate(USART2, STM32_PCLK1, baud);//
			usart_enable(USART2);
		}
		void usart3_begin(uint32_t baud,uint32_t config )
		{
			usart_init(USART3);
			usart_config_gpios_async(USART3,GPIOB,PIN_MAP[PB11].gpio_bit,GPIOB,PIN_MAP[PB10].gpio_bit,config);
			usart_set_baud_rate(USART3, STM32_PCLK1, baud);
			usart_enable(USART3);
		}
	#endif
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
	uint8_t next = (SerialControl.head + 2) & 0x7f ;
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
	{
		OCR0A += 20 ;
	}
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
				ptr->tail = ( ptr->tail + 2 ) & 0x7F ;
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
	{
		OCR0B += 20 ;
	}
}

ISR(TIMER0_OVF_vect)
{
	uint8_t byte ;
	if ( GPIOR1 > 2 )
	{
		byte = GPIOR0 ;
	}
	else
	{
		byte = GPIOR2 ;
	}
	if ( byte & 0x01 )
		SERIAL_TX_on;
	else
		SERIAL_TX_off;
	byte /= 2 ;		// Generates shorter code than byte >>= 1
	if ( GPIOR1 > 2 )
	{
		GPIOR0 = byte ;
	}
	else
	{
		GPIOR2 = byte ;
	}
	if ( --GPIOR1 == 0 )
	{
		// prepare next byte
		volatile struct t_serial_bash *ptr = &SerialControl ;
		if ( ptr->head != ptr->tail )
		{
			GPIOR0 = ptr->data[ptr->tail] ;
			GPIOR2 = ptr->data[ptr->tail+1] ;
			ptr->tail = ( ptr->tail + 2 ) & 0x7F ;
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
