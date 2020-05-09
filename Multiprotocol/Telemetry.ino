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
	uint32_t lastMulti = 0;
	#define MULTI_TIME				500	//in ms
	#ifdef MULTI_SYNC
		#define INPUT_SYNC_TIME			100	//in ms
		#define INPUT_ADDITIONAL_DELAY	100	// in 10µs, 100 => 1000 µs
		uint32_t lastInputSync = 0;
		uint16_t inputDelay = 0;
	#endif // MULTI_SYNC
#endif // MULTI_TELEMETRY/MULTI_STATUS

#if defined SPORT_TELEMETRY	
	#define FRSKY_SPORT_PACKET_SIZE   8
	#define FX_BUFFERS	4
	uint8_t RxBt = 0;
	uint8_t Sport_Data = 0;
	uint8_t pktx1[FRSKY_SPORT_PACKET_SIZE*FX_BUFFERS];

	// Store for out of sequence packet
	uint8_t FrSkyX_RX_ValidSeq ;
	struct t_FrSkyX_RX_Frame
	{
		boolean valid;
		uint8_t count;
		uint8_t payload[6];
	} ;

	// Store for FrskyX telemetry
	struct t_FrSkyX_RX_Frame FrSkyX_RX_Frames[4] ;
	uint8_t FrSkyX_RX_NextFrame=0;
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

#ifdef MULTI_SYNC
static void telemetry_set_input_sync(uint16_t refreshRate)
{
	#if defined(STM32_BOARD) && defined(DEBUG_PIN)
		static uint8_t c=0;
		if (c++%2==0)
		{	DEBUG_PIN_on;	}
		else
		{	DEBUG_PIN_off;	}
	#endif
	// Only record input Delay after a frame has really been received
	// Otherwise protocols with faster refresh rates then the TX sends (e.g. 3ms vs 6ms) will screw up the calcualtion
	inputRefreshRate = refreshRate;
	if (last_serial_input != 0)
	{
		cli();										// Disable global int due to RW of 16 bits registers
		inputDelay = TCNT1;
		sei();										// Enable global int
		//inputDelay = (inputDelay - last_serial_input)>>1;
		inputDelay -= last_serial_input;
		//if(inputDelay & 0x8000)
		//	inputDelay = inputDelay - 0x8000;
		last_serial_input=0;
	}
}
#endif

#ifdef MULTI_SYNC
	static void mult_send_inputsync()
	{
		multi_send_header(MULTI_TELEMETRY_SYNC, 6);
		Serial_write(inputRefreshRate >> 8);
		Serial_write(inputRefreshRate & 0xff);
//		Serial_write(inputDelay >> 8);
//		Serial_write(inputDelay & 0xff);
		Serial_write(inputDelay >> 9);
		Serial_write(inputDelay >> 1);
		Serial_write(INPUT_SYNC_TIME);
		Serial_write(INPUT_ADDITIONAL_DELAY);
	}
#endif //MULTI_SYNC

static void multi_send_status()
{
	#ifdef MULTI_TELEMETRY
		#ifdef MULTI_NAMES
			multi_send_header(MULTI_TELEMETRY_STATUS, 24);
		#else
			multi_send_header(MULTI_TELEMETRY_STATUS, 5);
		#endif
	#else
		multi_send_header(MULTI_TELEMETRY_STATUS, 5);
	#endif

	// Build flags
	uint8_t flags=0;
	if (IS_INPUT_SIGNAL_on)
		flags |= 0x01;
	if (mode_select==MODE_SERIAL)
		flags |= 0x02;
	if (remote_callback != 0)
	{
		flags |= 0x04;
		#ifdef MULTI_NAMES
			if(multi_protocols_index == 0xFF)
			{
				if(protocol!=PROTO_SCANNER)
					flags &= ~0x04;			//Invalid protocol
			}
			else if(sub_protocol&0x07)
				{
					uint8_t nbr=multi_protocols[multi_protocols_index].nbrSubProto;
					if(protocol==PROTO_DSM) nbr++;	//Auto sub_protocol
					if((sub_protocol&0x07)>=nbr)
						flags &= ~0x04;		//Invalid sub protocol
				}
		#else
			if(remote_callback==0)
				flags &= ~0x04;			//Invalid protocol
		#endif
		if (IS_WAIT_BIND_on)
			flags |= 0x10;
		else
			if (IS_BIND_IN_PROGRESS)
				flags |= 0x08;
		if(IS_CHMAP_PROTOCOL)
			flags |= 0x40;				//Disable_ch_mapping supported
		#ifdef FAILSAFE_ENABLE
			if(IS_FAILSAFE_PROTOCOL)
				flags |= 0x20;			//Failsafe supported
		#endif
		if(IS_DATA_BUFFER_LOW_on)
			flags |= 0x80;
	}
	Serial_write(flags);
	
	// Version number example: 1.1.6.1
	Serial_write(VERSION_MAJOR);
	Serial_write(VERSION_MINOR);
	Serial_write(VERSION_REVISION);
	Serial_write(VERSION_PATCH_LEVEL);

	#ifdef MULTI_TELEMETRY
		// Channel order
		Serial_write(RUDDER<<6|THROTTLE<<4|ELEVATOR<<2|AILERON);
	#endif
	
	#ifdef MULTI_NAMES
		if(multi_protocols_index == 0xFF)												// selection out of list... send first available protocol
		{
			Serial_write(multi_protocols[0].protocol);									// begining of list
			Serial_write(multi_protocols[0].protocol);									// begining of list
			for(uint8_t i=0;i<16;i++)
				Serial_write(0x00);														// everything else is invalid
		}
		else
		{
			// Protocol next/prev
			if(multi_protocols[multi_protocols_index+1].protocol != 0)
				Serial_write(multi_protocols[multi_protocols_index+1].protocol);		// next protocol number
			else
				Serial_write(multi_protocols[multi_protocols_index].protocol);			// end of list
			if(multi_protocols_index>0)
				Serial_write(multi_protocols[multi_protocols_index-1].protocol);		// prev protocol number
			else
				Serial_write(multi_protocols[multi_protocols_index].protocol);			// begining of list
			// Protocol
			for(uint8_t i=0;i<7;i++)
				Serial_write(multi_protocols[multi_protocols_index].ProtoString[i]);	// protocol name
			// Sub-protocol
			uint8_t nbr=multi_protocols[multi_protocols_index].nbrSubProto;
			Serial_write(nbr | (multi_protocols[multi_protocols_index].optionType<<4));	// number of sub protocols && option type
			uint8_t j=0;
			if(nbr && (sub_protocol&0x07)<nbr)
			{
				uint8_t len=multi_protocols[multi_protocols_index].SubProtoString[0];
				uint8_t offset=len*(sub_protocol&0x07)+1;
				for(;j<len;j++)
					Serial_write(multi_protocols[multi_protocols_index].SubProtoString[j+offset]);	// current sub protocol name
			}
			for(;j<8;j++)
				Serial_write(0x00);
		}
		// Channels function
		//TODO
	#endif
}
#endif

#ifdef DSM_TELEMETRY
	#ifdef MULTI_TELEMETRY
		void DSM_frame()
		{
			if (packet_in[0] == 0x80)
			{
				multi_send_header(MULTI_TELEMETRY_DSMBIND, 10);
				for (uint8_t i = 1; i < 11; i++) 	// 10 bytes of DSM bind response
					Serial_write(packet_in[i]);

			}
			else
			{
				multi_send_header(MULTI_TELEMETRY_DSM, 17);
				for (uint8_t i = 0; i < 17; i++)	// RSSI value followed by 16 bytes of telemetry data
					Serial_write(packet_in[i]);
			}
		}
	#else
		void DSM_frame()
		{
			Serial_write(0xAA);						// Telemetry packet
			for (uint8_t i = 0; i < 17; i++)		// RSSI value followed by 16 bytes of telemetry data
				Serial_write(packet_in[i]);
		}
	#endif
#endif

#ifdef SCANNER_TELEMETRY
	void spectrum_scanner_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_SCANNER, SCAN_CHANS_PER_PACKET + 1);
		#else
			Serial_write(0xAA);						// Telemetry packet
		#endif
		Serial_write(packet_in[0]);					// start channel
		for(uint8_t ch = 0; ch < SCAN_CHANS_PER_PACKET; ch++)
			Serial_write(packet_in[ch+1]);			// RSSI power levels
	}
#endif

#if defined (FRSKY_RX_TELEMETRY) || defined (AFHDS2A_RX_TELEMETRY) || defined (BAYANG_RX_TELEMETRY)
	void receiver_channels_frame()
	{
		uint16_t len = packet_in[3] * 11;			// 11 bit per channel
		if (len % 8 == 0)
			len = 4 + (len / 8);
		else
			len = 5 + (len / 8);
		#if defined MULTI_TELEMETRY
				multi_send_header(MULTI_TELEMETRY_RX_CHANNELS, len);
		#else
				Serial_write(0xAA);					// Telemetry packet
		#endif
		for (uint8_t i = 0; i < len; i++)
			Serial_write(packet_in[i]);				// pps, rssi, ch start, ch count, 16x ch data
	}
#endif

#ifdef AFHDS2A_FW_TELEMETRY
	void AFHDSA_short_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(packet_in[29]==0xAA?MULTI_TELEMETRY_AFHDS2A:MULTI_TELEMETRY_AFHDS2A_AC, 29);
		#else
			Serial_write(packet_in[29]);			// Telemetry packet 0xAA or 0xAC
		#endif
		for (uint8_t i = 0; i < 29; i++)			// RSSI value followed by 4*7 bytes of telemetry data
			Serial_write(packet_in[i]);
	}
#endif

#ifdef HITEC_FW_TELEMETRY
	void HITEC_short_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_HITEC, 8);
		#else
			Serial_write(0xAA);						// Telemetry packet
		#endif
		for (uint8_t i = 0; i < 8; i++)				// TX RSSI and TX LQI values followed by frame number and 5 bytes of telemetry data
			Serial_write(packet_in[i]);
	}
#endif

#ifdef HOTT_FW_TELEMETRY
	void HOTT_short_frame()
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_HOTT, 14);
		#else
			Serial_write(0xAA);						// Telemetry packet
		#endif
		for (uint8_t i = 0; i < 14; i++)			// TX RSSI and TX LQI values followed by frame number and telemetry data
			Serial_write(packet_in[i]);
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

void frsky_check_telemetry(uint8_t *packet_in,uint8_t len)
{
	if(packet_in[1] != rx_tx_addr[3] || packet_in[2] != rx_tx_addr[2] || len != packet_in[0] + 3 )
		return;										// Bad address or length...

	telemetry_link|=1;								// Telemetry data is available
	// RSSI and LQI are the 2 last bytes
	TX_RSSI = packet_in[len-2];
	if(TX_RSSI >=128)
		TX_RSSI -= 128;
	else
		TX_RSSI += 128;
	TX_LQI = packet_in[len-1]&0x7F;
	
#if defined FRSKYD_CC2500_INO
	if (protocol==PROTO_FRSKYD)
	{
		//Save current buffer
		for (uint8_t i=3;i<len-2;i++)
			telemetry_in_buffer[i]=packet_in[i];	// Buffer telemetry values to be sent
	
		//Check incoming telemetry sequence
		if(telemetry_in_buffer[6]>0 && telemetry_in_buffer[6]<=10)
		{ //Telemetry length ok
			if ( ( telemetry_in_buffer[7] & 0x1F ) == (telemetry_counter & 0x1F) )
			{//Sequence is ok
				uint8_t topBit = 0 ;
				if ( telemetry_counter & 0x80 )
					if ( ( telemetry_counter & 0x1F ) != RetrySequence )
						topBit = 0x80 ;
				telemetry_counter = ( (telemetry_counter+1)%32 ) | topBit ;	// Request next telemetry frame
			}
			else
			{//Incorrect sequence
				RetrySequence = telemetry_in_buffer[7] & 0x1F ;
				telemetry_counter |= 0x80 ;
				telemetry_in_buffer[6]=0 ;			// Discard current packet and wait for retransmit
			}
		}
		else
			telemetry_in_buffer[6]=0; 				// Discard packet
	}
#endif

#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
	if (protocol==PROTO_FRSKYX||protocol==PROTO_FRSKYX2)
	{
		/*Telemetry frames(RF) SPORT info 
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
		[14] CHKSUM2 --|*/
		telemetry_lost=0;

		uint16_t lcrc = FrSkyX_crc(&packet_in[3], len-7 ) ;
		if ( ( (lcrc >> 8) != packet_in[len-4]) || ( (lcrc & 0x00FF ) != packet_in[len-3]) )
			return;									// Bad CRC
		
		if(packet_in[4] & 0x80)
			RX_RSSI=packet_in[4] & 0x7F ;
		else
			RxBt = (packet_in[4]<<1) + 1 ;
		#if defined(TELEMETRY_FRSKYX_TO_FRSKYD) && defined(ENABLE_PPM)
			if(mode_select != MODE_SERIAL)
			{//PPM
				v_lipo1=RxBt;
				return;
			}
		#endif
		//Save outgoing telemetry sequence
		FrSkyX_TX_IN_Seq=packet_in[5] >> 4;

		//Check incoming telemetry sequence
		uint8_t packet_seq=packet_in[5] & 0x03;
		if ( packet_in[5] & 0x08 )
		{//Request init
			FrSkyX_RX_Seq = 0x08 ;
			FrSkyX_RX_NextFrame = 0x00 ;
			FrSkyX_RX_Frames[0].valid = false ;
			FrSkyX_RX_Frames[1].valid = false ;
			FrSkyX_RX_Frames[2].valid = false ;
			FrSkyX_RX_Frames[3].valid = false ;
		}
		else if ( packet_seq == (FrSkyX_RX_Seq & 0x03 ) )
		{//In sequence
			struct t_FrSkyX_RX_Frame *p ;
			uint8_t count ;
			// packet_in[4] RSSI
			// packet_in[5] sequence control
			// packet_in[6] payload count
			// packet_in[7-12] payload			
			p = &FrSkyX_RX_Frames[packet_seq] ;
			count = packet_in[6];					// Payload length
			if ( count <= 6 )
			{//Store payload
				p->count = count ;
				for ( uint8_t i = 0 ; i < count ; i++ )
					p->payload[i] = packet_in[i+7] ;
			}
			else
				p->count = 0 ;						// Discard
			p->valid = true ;

			FrSkyX_RX_Seq = ( FrSkyX_RX_Seq + 1 ) & 0x03 ;	// Move to next sequence

			if ( FrSkyX_RX_ValidSeq & 0x80 )
			{
				FrSkyX_RX_Seq = ( FrSkyX_RX_ValidSeq + 1 ) & 3 ;
				FrSkyX_RX_ValidSeq &= 0x7F ;
			}

		}
		else
		{//Not in sequence
			struct t_FrSkyX_RX_Frame *q ;
			uint8_t count ;
			// packet_in[4] RSSI
			// packet_in[5] sequence control
			// packet_in[6] payload count
			// packet_in[7-12] payload			
			if ( packet_seq == ( ( FrSkyX_RX_Seq +1 ) & 3 ) )
			{//Received next sequence -> save it
				q = &FrSkyX_RX_Frames[packet_seq] ;
				count = packet_in[6];				// Payload length
				if ( count <= 6 )
				{//Store payload
					q->count = count ;
					for ( uint8_t i = 0 ; i < count ; i++ )
						q->payload[i] = packet_in[i+7] ;
				}
				else
					q->count = 0 ;
				q->valid = true ;
			
				FrSkyX_RX_ValidSeq = 0x80 | packet_seq ;
			}
			FrSkyX_RX_Seq = ( FrSkyX_RX_Seq & 0x03 ) | 0x04 ;	// Request re-transmission of original sequence
		}
	}
#endif
}

void init_frskyd_link_telemetry()
{
	telemetry_link=0;
	telemetry_counter=0;
	telemetry_lost=1;
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
		frame[1] = telemetry_in_buffer[3];		// A1
		frame[2] = telemetry_in_buffer[4];		// A2
		frame[3] = telemetry_in_buffer[5];		// RX_RSSI
		telemetry_link &= ~1 ;		// Sent
		telemetry_link |= 2 ;		// Send hub if available
	}
	else
	{//PROTO_HUBSAN, PROTO_AFHDS2A, PROTO_BAYANG, PROTO_NCC1701, PROTO_CABELL, PROTO_HITEC, PROTO_BUGS, PROTO_BUGSMINI, PROTO_FRSKYX, PROTO_FRSKYX2, PROTO_PROPEL, PROTO_DEVO
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
	if(telemetry_in_buffer[6])
	{//only send valid hub frames
		frame[0] = 0xFD;				// user frame
		if(telemetry_in_buffer[6]>USER_MAX_BYTES)
		{
			frame[1]=USER_MAX_BYTES;	// packet size
			telemetry_in_buffer[6]-=USER_MAX_BYTES;
			telemetry_link |= 2 ;			// 2 packets need to be sent
		}
		else
		{
			frame[1]=telemetry_in_buffer[6];			// packet size
			telemetry_link=0;			// only 1 packet or processing second packet
		}
		frame[2] = telemetry_in_buffer[7];
		for(uint8_t i=0;i<USER_MAX_BYTES;i++)
			frame[i+3]=telemetry_in_buffer[i+8];
		if(telemetry_link & 2)				// prepare the content of second packet
			for(uint8_t i=8;i<USER_MAX_BYTES+8;i++)
				telemetry_in_buffer[i]=telemetry_in_buffer[i+USER_MAX_BYTES];
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
packet_in[6]|(counter++)|00 01 02 03 04 05 06 07 08 09 
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

#if defined MULTI_TELEMETRY
const uint8_t PROGMEM Indices[] = {	0x00, 0xA1, 0x22, 0x83, 0xE4, 0x45,
									0xC6, 0x67, 0x48, 0xE9, 0x6A, 0xCB,
									0xAC, 0x0D, 0x8E, 0x2F, 0xD0, 0x71,
									0xF2, 0x53, 0x34, 0x95, 0x16, 0xB7,
									0x98, 0x39, 0xBA, 0x1B } ;
#endif

#ifdef MULTI_TELEMETRY
	void sportSend(uint8_t *p)
	{
		multi_send_header(MULTI_TELEMETRY_SPORT, 9);
		uint16_t crc_s = 0;
		uint8_t x = p[0] ;
		if ( x <= 0x1B )
			x = pgm_read_byte_near( &Indices[x] ) ;
		Serial_write(x) ;
		for (uint8_t i = 1; i < 8; i++)
		{
			Serial_write(p[i]);
			crc_s += p[i];			//0-1FF
			crc_s += crc_s >> 8;	//0-100
			crc_s &= 0x00ff;
		}
		Serial_write(0xff - crc_s);
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
			
			crc_s += p[i]; //0-1FF
			crc_s += crc_s >> 8; //0-100
			crc_s &= 0x00ff;
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
	static uint8_t sport_counter=0;
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
			if(Sport_Data)
			{	
				for (i=0;i<FRSKY_SPORT_PACKET_SIZE;i++)
					frame[i]=pktx1[i];
				Sport_Data -- ;
				if ( Sport_Data )
				{
					uint8_t j = Sport_Data * FRSKY_SPORT_PACKET_SIZE ;
					for (i=0;i<j;i++)
						pktx1[i] = pktx1[i+FRSKY_SPORT_PACKET_SIZE] ;
				}
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
	static uint8_t pass = 0, indx = 0;
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
		if ( Sport_Data < FX_BUFFERS )
		{
			uint8_t dest = Sport_Data * FRSKY_SPORT_PACKET_SIZE ;
			uint8_t i ;
			for ( i = 0 ; i < FRSKY_SPORT_PACKET_SIZE ; i++ )
				pktx1[dest++] = pktx[i] ;	// Triple buffer
			Sport_Data += 1 ;//ok to send
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
			debugln("TEL_BUF_FULL %d",t);
			return ;
		}
/*		else
			if(t!=96)
				debugln("TEL_BUF %d",t);
*/
	#endif
	#if defined(MULTI_TELEMETRY) || defined(MULTI_STATUS)
		uint32_t now = millis();
		if ((IS_SEND_MULTI_STATUS_on || ((now - lastMulti) > MULTI_TIME))&& protocol != PROTO_SCANNER)
		{
			multi_send_status();
			SEND_MULTI_STATUS_off;
			lastMulti = now;
			return;
		}
		#ifdef MULTI_SYNC
			if ( inputRefreshRate && (now - lastInputSync) > INPUT_SYNC_TIME )
			{
				mult_send_inputsync();
				lastInputSync = now;
				return;
			}
		#endif
	#endif
	#if defined SPORT_TELEMETRY
		if ((protocol==PROTO_FRSKYX || protocol==PROTO_FRSKYX2) && telemetry_link 
		#ifdef TELEMETRY_FRSKYX_TO_FRSKYD
			&& mode_select==MODE_SERIAL
		#endif
		)
		{	// FrSkyX
			for(;;)
			{ //Empty buffer
				struct t_FrSkyX_RX_Frame *p ;
				uint8_t count ;
				p = &FrSkyX_RX_Frames[FrSkyX_RX_NextFrame] ;
				if ( p->valid )
				{
					count = p->count ;
					for (uint8_t i=0; i < count ; i++)
						proces_sport_data(p->payload[i]) ;
					p->valid = false ;	// Sent
					FrSkyX_RX_NextFrame = ( FrSkyX_RX_NextFrame + 1 ) & 3 ;
				}
				else
					break ;
			}
			telemetry_link=0; 
			sportSendFrame();
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
	#if defined HOTT_FW_TELEMETRY
		if(telemetry_link == 2 && protocol == PROTO_HOTT)
		{
			HOTT_short_frame();
			telemetry_link=0;
			return;
		}
	#endif

	#if defined SCANNER_TELEMETRY
		if (telemetry_link && protocol == PROTO_SCANNER)
		{
			spectrum_scanner_frame();
			telemetry_link = 0;
			return;
		}
	#endif

	#if defined (FRSKY_RX_TELEMETRY) || defined(AFHDS2A_RX_TELEMETRY) || defined (BAYANG_RX_TELEMETRY)
		if ((telemetry_link & 1) && (protocol == PROTO_FRSKY_RX || protocol == PROTO_AFHDS2A_RX || protocol == PROTO_BAYANG_RX))
		{
			receiver_channels_frame();
			telemetry_link &= ~1;
			return;
		}
	#endif

		if( telemetry_link & 1 )
		{	// FrSkyD + Hubsan + AFHDS2A + Bayang + Cabell + Hitec + Bugs + BugsMini + NCC1701 + PROPEL
			// FrSkyX telemetry if in PPM
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
					tx_pause(); // Check if all data is transmitted. If yes disable transmitter UDRE interrupt.
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
