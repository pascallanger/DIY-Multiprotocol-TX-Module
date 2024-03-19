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

#if defined(DSM_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define DSM_DEBUG_FWD_PGM
//#define DEBUG_BIND  1

#define CLONE_BIT_MASK			0x20
#define DSM_BIND_CHANNEL		0x0D	//13 This can be any odd channel
#define DSM2_SFC_PERIOD			16500

//During binding we will send BIND_COUNT packets
//One packet each 10msec
//
// Most RXs seems to work properly with a long BIND send count (3s): Spektrum, OrangeRX. 
// Lemon-RX G2s seems to have a timeout waiting for the channel to get quiet after the 
// first good BIND packet.. If using 3s (300), Lemon-RX will not transmit the BIND-Response packet. 

#define DSM_BIND_COUNT			180  // About 1.8s
#define DSM_BIND_COUNT_READ		600  // About 4.2s of waiting for Response

enum {
	DSM_BIND_WRITE=0,
	DSM_BIND_CHECK,
	DSM_BIND_READ,
	DSM_CHANSEL,
	DSM_CH1_WRITE_A,
	DSM_CH1_CHECK_A,
	DSM_CH2_WRITE_A,
	DSM_CH2_CHECK_A,
	DSM_CH2_READ_A,
	DSM_CH1_WRITE_B,
	DSM_CH1_CHECK_B,
	DSM_CH2_WRITE_B,
	DSM_CH2_CHECK_B,
	DSM_CH2_READ_B,
};

//
uint8_t ch_map[14];
const uint8_t PROGMEM DSM_ch_map_progmem[][14] = {
//22+11ms for 3..7 channels
	{1, 0, 2, 0xff, 0xff, 0xff, 0xff, 1,  0,   2, 0xff, 0xff, 0xff,    0xff}, //3ch  - Guess
	{1, 0, 2, 3, 0xff, 0xff, 0xff, 1,    0,    2,    3, 0xff, 0xff,    0xff}, //4ch  - Guess
	{1, 0, 2, 3, 4,    0xff, 0xff, 1,    0,    2,    3,    4, 0xff,    0xff}, //5ch  - Guess
	{1, 5, 2, 3, 0,    4,    0xff, 1,    5,    2,    3,    0,    4,    0xff}, //6ch  - HP6DSM
	{1, 5, 2, 4, 3,    6,    0,    1,    5,    2,    4,    3,    6,    0   }, //7ch  - DX6i
//22ms for 8..12 channels
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    0xff, 0xff, 0xff, 0xff}, //8ch  - DX8/DX7
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    8,    0xff, 0xff, 0xff}, //9ch  - Guess
	{1, 5, 2, 3, 6,    0xff, 0xff, 4,    0,    7,    8,    9,    0xff, 0xff}, //10ch - Guess
	{1, 5, 2, 3, 6,    10,   0xff, 4,    0,    7,    8,    9,    0xff, 0xff}, //11ch - Guess
	{1, 5, 2, 4, 6,    10,   0xff, 0,    7,    3,    8,    9   , 11  , 0xff}, //12ch - DX18/DX8G2
//11ms for 8..11 channels
	{1, 5, 2, 3, 6,    7,    0xff, 1,    5,    2,    4,    0,    0xff, 0xff}, //8ch  - DX7
	{1, 5, 2, 3, 6,    7,    0xff, 1,    5,    2,    4,    0,    8,    0xff}, //9ch  - Guess
	{1, 5, 2, 3, 4,    8,    9,    1,    5,    2,    3,    0,    7,    6   }, //10ch - DX18
	{1, 5, 2, 3, 4,    8,    9,    1,   10,    2,    3,    0,    7,    6   }, //11ch - Guess
};

static void __attribute__((unused)) DSM_build_bind_packet()
{
	uint8_t i;
	uint16_t sum = 384 - 0x10;//
	packet[0] = 0xff ^ cyrfmfg_id[0];
	packet[1] = 0xff ^ cyrfmfg_id[1];
	packet[2] = 0xff ^ cyrfmfg_id[2];
	packet[3] = 0xff ^ cyrfmfg_id[3];
	packet[4] = packet[0];
	packet[5] = packet[1];
	packet[6] = packet[2];
	packet[7] = packet[3];
	for(i = 0; i < 8; i++)
		sum += packet[i];
	packet[8] = sum >> 8;
	packet[9] = sum & 0xff;
	packet[10] = 0x01;						// ???
	if(sub_protocol==DSM_AUTO)
		packet[11] = 12;
	else
		packet[11] = num_ch;				// DX5 DSMR sends 0x48...

	if (sub_protocol==DSMR)
		packet[12] = 0xa2;
	else if (sub_protocol==DSM2_1F)
		packet[12] = num_ch<8?0x01:0x02;	// DSM2/1024 1 or 2 packets depending on the number of channels
	else if(sub_protocol==DSM2_2F)
		packet[12] = 0x12;					// DSM2/2048 2 packets
	else if(sub_protocol==DSMX_1F)
		#if defined DSM_TELEMETRY
			packet[12] = 0xb2;				// DSMX/2048 2 packets
		#else
			packet[12] = num_ch<8? 0xa2 : 0xb2;	// DSMX/2048 1 or 2 packets depending on the number of channels
		#endif
	else									// DSMX_2F && DSM_AUTO
		packet[12] = 0xb2;					// DSMX/2048 2 packets

	packet[13] = 0x00;						//???
	for(i = 8; i < 14; i++)
		sum += packet[i];
	packet[14] = sum >> 8;
	packet[15] = sum & 0xff;
}

static void __attribute__((unused)) DSM_initialize_bind_phase()
{
	CYRF_ConfigRFChannel(DSM_BIND_CHANNEL); //This seems to be random?
	//64 SDR Mode is configured so only the 8 first values are needed but need to write 16 values...
	uint8_t code[16];
	DSM_read_code(code,0,8);
	CYRF_ConfigDataCode(code);
	DSM_build_bind_packet();
}

static void __attribute__((unused)) DSM_update_channels()
{
	prev_option=option;
	num_ch=option & 0x0F;				// Remove flags 0x80=max_throw, 0x40=11ms

	if(num_ch<3 || num_ch>12)
		num_ch=6;						// Default to 6 channels if invalid choice...

	#ifndef MULTI_AIR
		if(sub_protocol==DSMR && num_ch>7)
			num_ch=7;						// Max 7 channels in DSMR

		if(sub_protocol==DSM2_SFC && num_ch>5)
			num_ch=5;						// Max 5 channels in DSM2_SFC
	#endif

	// Create channel map based on number of channels and refresh rate
	uint8_t idx=num_ch-3;
	if((option & 0x40) && num_ch>7 && num_ch<12)
		idx+=5;							// In 11ms mode change index only for channels 8..11
	for(uint8_t i=0;i<14;i++)
		ch_map[i]=pgm_read_byte_near(&DSM_ch_map_progmem[idx][i]);
}

static void __attribute__((unused)) DSM_build_data_packet(uint8_t upper)
{
	uint8_t bits = 11;
	
	// Check if clone flag has changed
	if((prev_option&CLONE_BIT_MASK) != (option&CLONE_BIT_MASK))
	{
		DSM_init();
		prev_option^=CLONE_BIT_MASK;
	}
	if(prev_option!=option)
		DSM_update_channels();

	if (sub_protocol==DSMX_2F || sub_protocol==DSMX_1F)
	{//DSMX
		packet[0] = cyrfmfg_id[2];
		packet[1] = cyrfmfg_id[3];
	}
	else
	{//DSM2 && DSMR
		packet[0] = (0xff ^ cyrfmfg_id[2]);
		packet[1] = (0xff ^ cyrfmfg_id[3]);
		if(sub_protocol==DSM2_1F)
			bits=10;								// Only DSM2_1F is using a resolution of 1024
	}

	#ifndef MULTI_AIR
		if(sub_protocol == DSMR || sub_protocol == DSM2_SFC)
		{
			for (uint8_t i = 0; i < 7; i++)
			{
				uint16_t value = 0x0000;
				if(i < num_ch)
					value=Channel_data[i]<<1;
				packet[i*2+2] = (value >> 8) & 0xff;
				packet[i*2+3] = (value >> 0) & 0xff;
			}
			return;
		}
	#endif
	
	#ifdef DSM_THROTTLE_KILL_CH
		uint16_t kill_ch=Channel_data[DSM_THROTTLE_KILL_CH-1];
	#endif
	for (uint8_t i = 0; i < 7; i++)
	{	
		uint8_t idx = ch_map[(upper?7:0) + i];		// 1,5,2,3,0,4	   
		uint16_t value = 0xffff;
		if((option&0x40) == 0 && num_ch < 8 && upper)
			idx=0xff;								// in 22ms do not transmit upper channels if <8, is it the right method???
		if (idx != 0xff)
		{
			/* Spektrum own remotes transmit normal values during bind and actually use this (e.g. Nano CP X) to
			   select the transmitter mode (e.g. computer vs non-computer radio), so always send normal output */
			#ifdef DSM_THROTTLE_KILL_CH
				if(idx==CH1 && kill_ch<=604)
				{//Activate throttle kill only if channel is throttle and DSM_THROTTLE_KILL_CH below -50%
					if(kill_ch<CHANNEL_MIN_100)		// restrict val to 0...400
						kill_ch=0;
					else
						kill_ch-=CHANNEL_MIN_100;
					value=(kill_ch*21)/25;			// kill channel -100%->904us ... -50%->1100us *0x150/400
				}
				else
			#endif
				#ifdef DSM_MAX_THROW
					value=Channel_data[CH_TAER[idx]];										// -100%..+100% => 1024..1976us and -125%..+125% => 904..2096us based on Redcon 6 channel DSM2 RX
				#else
					if(option & 0x80)
						value=Channel_data[CH_TAER[idx]];									// -100%..+100% => 1024..1976us and -125%..+125% => 904..2096us based on Redcon 6 channel DSM2 RX
					else
						value=convert_channel_16b_nolimit(CH_TAER[idx],0x156,0x6AA,false);	// -100%..+100% => 1100..1900us and -125%..+125% => 1000..2000us based on a DX8 G2 dump
				#endif
			if(bits==10) value>>=1;
			value |= (upper && i==0 ? 0x8000 : 0) | (idx << bits);
		}	  
		packet[i*2+2] = value >> 8;
		packet[i*2+3] = value;
	}
	#ifdef DSM_FWD_PGM
		if(upper==0 && DSM_SerialRX && (DSM_SerialRX_val[0]&0xF8)==0x70 )
		{ // Send forward programming data if available
			for(uint8_t i=0; i<(DSM_SerialRX_val[0]&0x07);i++)
			{
				packet[i*2+4]=0x70+i;
				packet[i*2+5]=DSM_SerialRX_val[i+1];
			}
			DSM_SerialRX=false;
			#ifdef DSM_DEBUG_FWD_PGM
				debug("FWD=");
				for(uint8_t i=4; i<16;i++)
					debug(" %02X",packet[i]);
				debugln("");
			#endif
		}
	#endif
}

static uint8_t __attribute__((unused)) DSM_Check_RX_packet()
{
	uint8_t result=1;							// assume good packet
	
	uint16_t sum = 384 - 0x10;
	for(uint8_t i = 1; i < 9; i++)
	{
		sum += packet_in[i];
		if(i<5)
			if(packet_in[i] != (0xff ^ cyrfmfg_id[i-1]))
				result=0; 						// bad packet
	}
	if( packet_in[9] != (sum>>8)  && packet_in[10] != (uint8_t)sum )
		result=0;
	return result;
}

uint16_t DSM_callback()
{
	#if defined MULTI_EU
		if(sub_protocol == DSM2_1F || sub_protocol == DSM2_2F || sub_protocol == DSM2_SFC)
		{
			SUB_PROTO_INVALID;
			return 11000;
		}
	#endif
	#if defined MULTI_AIR
		if(sub_protocol == DSMR || sub_protocol == DSM2_SFC)
		{
			SUB_PROTO_INVALID;
			return 11000;
		}
	#endif
	#define DSM_CH1_CH2_DELAY	4010			// Time between write of channel 1 and channel 2
	#ifdef STM32_BOARD
		#define DSM_WRITE_DELAY		1600		// Time after write to verify write complete
	#else
		#define DSM_WRITE_DELAY		1950		// Time after write to verify write complete
	#endif
	#define DSM_READ_DELAY		600				// Time before write to check read phase, and switch channels. Was 400 but 600 seems what the 328p needs to read a packet
	#if defined DSM_TELEMETRY
		uint8_t rx_phase;
		uint8_t len;
	#endif
	uint8_t start;

	switch(phase)
	{
		case DSM_BIND_WRITE:
			if(bind_counter--==0)
			#if defined DSM_TELEMETRY
				phase=DSM_BIND_CHECK;							//Check RX answer
			#else
				phase=DSM_CHANSEL;								//Switch to normal mode
			#endif
			CYRF_WriteDataPacket(packet);
			return 10000;
	#if defined DSM_TELEMETRY
		case DSM_BIND_CHECK:
      #if DEBUG_BIND
        debugln("Bind Check");
      #endif
      //64 SDR Mode is configured so only the 8 first values are needed
      CYRF_ConfigDataCode((const uint8_t *)"\x98\x88\x1B\xE4\x30\x79\x03\x84");
			CYRF_SetTxRxMode(RX_EN);							//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);			//Prepare to receive
			bind_counter=DSM_BIND_COUNT_READ; //Timeout of 4.2s if no packet received
			phase++;											// change from BIND_CHECK to BIND_READ
			return 2000;
		case DSM_BIND_READ:
			//Read data from RX
			rx_phase = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x03) == 0x02)  						// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_phase |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x07) == 0x02)
			{ // data received with no errors
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);// Need to set RXOW before data read
				if(CYRF_ReadRegister(CYRF_09_RX_COUNT)==10)		// Len
				{
					CYRF_ReadDataPacketLen(packet_in+1, 10);
					if(DSM_Check_RX_packet())
					{
						#if DEBUG_BIND
  						  debug("Bind");
  						  for(uint8_t i=0;i<10;i++)
  							debug(" %02X",packet_in[i+1]);
  						  debugln("");
						#endif
						packet_in[0]=0x80;
						packet_in[6]&=0x0F;						// It looks like there is a flag 0x40 being added by some receivers
						if(packet_in[6]>12) packet_in[6]=12;
						else if(packet_in[6]<3) packet_in[6]=6;
						telemetry_link=1;						// Send received data on serial
						phase++;
						return 2000;
					}
				}
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Abort RX operation
				CYRF_SetTxRxMode(RX_EN);						// Force end state read
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		// Clear abort RX operation
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);		// Prepare to receive
			}
			else
				if((rx_phase & 0x02) != 0x02)
				{ // data received with errors
					CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);	// Abort RX operation
					CYRF_SetTxRxMode(RX_EN);					// Force end state read
					CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX operation
					CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);	// Prepare to receive
				}
			if( --bind_counter == 0 )
			{ // Exit if no answer has been received for some time
        #if DEBUG_BIND
          debugln("Bind Read TIMEOUT");
        #endif
				phase++;										// DSM_CHANSEL
				return 7000 ;
			}
			return 7000;
	#endif
		case DSM_CHANSEL:
			BIND_DONE;
			DSM_cyrf_configdata();
			CYRF_SetTxRxMode(TX_EN);
			hopping_frequency_no = 0;
			phase = DSM_CH1_WRITE_A;							// in fact phase++
			#ifndef MULTI_AIR
			if(sub_protocol == DSMR)
				DSM_set_sop_data_crc(false, true);
			else
			#endif
				DSM_set_sop_data_crc(true, sub_protocol==DSMX_2F||sub_protocol==DSMX_1F);	//prep CH1
			return 10000;
		case DSM_CH1_WRITE_A:
			#ifdef MULTI_SYNC
				if(sub_protocol!=DSM2_SFC)
					telemetry_set_input_sync(11000);			// Always request 11ms spacing even if we don't use half of it in 22ms mode
				else
					telemetry_set_input_sync(DSM2_SFC_PERIOD);
			#endif
			#ifndef MULTI_AIR
			if(sub_protocol == DSMR)
				CYRF_SetPower(0x08);							//Keep transmit power in sync
			else
			#endif
				CYRF_SetPower(0x28);							//Keep transmit power in sync
		case DSM_CH1_WRITE_B:
			DSM_build_data_packet(phase == DSM_CH1_WRITE_B);	// build lower or upper channels
		case DSM_CH2_WRITE_A:
		case DSM_CH2_WRITE_B:
			CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);			// clear IRQ flags
			CYRF_WriteDataPacket(packet);
			#if 0
				for(uint8_t i=0;i<16;i++)
					debug(" %02X", packet[i]);
				debugln("");
			#endif
			phase++;											// change from WRITE to CHECK mode
			return DSM_WRITE_DELAY;
		case DSM_CH1_CHECK_A:
		case DSM_CH1_CHECK_B:
		case DSM_CH2_CHECK_A:
		case DSM_CH2_CHECK_B:
			start=(uint8_t)micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 100)			// Wait max 100µs, max I've seen is 50µs
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;
			
			if((phase==DSM_CH1_CHECK_A || phase==DSM_CH1_CHECK_B) && sub_protocol!=DSMR)
			{
				#if defined DSM_TELEMETRY
					// reset cyrf6936 if freezed after switching from TX to RX
					if (((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS) & 0x22) == 0x20) || (CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80))
					{
						CYRF_Reset();
						DSM_cyrf_config();
						DSM_cyrf_configdata();
						CYRF_SetTxRxMode(TX_EN);
					}
				#endif
				DSM_set_sop_data_crc(true, sub_protocol==DSMX_2F||sub_protocol==DSMX_1F);	// prep CH2
				phase++;									// change from CH1_CHECK to CH2_WRITE
				return DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
			}
#if defined DSM_TELEMETRY
			phase++;										// change from CH2_CHECK to CH2_READ
			CYRF_SetTxRxMode(RX_EN);						//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);		//0x80??? //Prepare to receive
			#ifndef MULTI_AIR
				if(sub_protocol==DSMR)
				{
					phase = DSM_CH2_READ_B;
					return 11000 - DSM_WRITE_DELAY - DSM_READ_DELAY;
				}
				if(sub_protocol==DSM2_SFC)
					return DSM2_SFC_PERIOD - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY - DSM_READ_DELAY;
			#endif
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY - DSM_READ_DELAY;
		case DSM_CH2_READ_A:
		case DSM_CH2_READ_B:
			//Read telemetry
			rx_phase = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x03) == 0x02)  					// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_phase |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_phase & 0x07) == 0x02)
			{ // good data (complete with no errors)
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				if(len>TELEMETRY_BUFFER_SIZE-2)
					len=TELEMETRY_BUFFER_SIZE-2;
				CYRF_ReadDataPacketLen(packet_in+1, len);
				#ifdef DSM_DEBUG_FWD_PGM
					//debug(" %02X", packet_in[1]);
					if(packet_in[1]==9)
					{
						for(uint8_t i=0;i<len;i++)
							debug(" %02X", packet_in[i+1]);
						debugln("");
					}
				#endif
				packet_in[0]=CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;// store RSSI of the received telemetry signal
				telemetry_link=1;
			}
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Abort RX operation
			if (phase == DSM_CH2_READ_A && (sub_protocol==DSM2_1F || sub_protocol==DSMX_1F || sub_protocol==DSM2_SFC) && num_ch < 8)	// 22ms mode
			{
				CYRF_SetTxRxMode(RX_EN);					// Force end state read
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX operation
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);	//0x80???	//Prepare to receive
				phase = DSM_CH2_READ_B;
				#ifndef MULTI_AIR
					if(sub_protocol==DSM2_SFC)
						return DSM2_SFC_PERIOD;
				#endif
				return 11000;
			}
			if (phase == DSM_CH2_READ_A)
				phase = DSM_CH1_WRITE_B;					//Transmit upper
			else
				phase = DSM_CH1_WRITE_A;					//Transmit lower
			CYRF_SetTxRxMode(TX_EN);						//TX mode
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		//Clear abort RX operation
			DSM_set_sop_data_crc(false, sub_protocol==DSMX_2F||sub_protocol==DSMX_1F||sub_protocol==DSMR);
			return DSM_READ_DELAY;
#else
			// No telemetry
			DSM_set_sop_data_crc(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B, sub_protocol==DSMX_2F||sub_protocol==DSMX_1F);
			if (phase == DSM_CH2_CHECK_A)
			{
				if(num_ch > 7 || sub_protocol==DSM2_2F || sub_protocol==DSMX_2F)
					phase = DSM_CH1_WRITE_B;				//11ms mode or upper to transmit change from CH2_CHECK_A to CH1_WRITE_A
				else										
				{											//Normal mode 22ms
					phase = DSM_CH1_WRITE_A;				// change from CH2_CHECK_A to CH1_WRITE_A (ie no upper)
					#ifndef MULTI_AIR
						if(sub_protocol==DSM2_SFC)
							return DSM2_SFC_PERIOD - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
					#endif
					return 22000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
				}
			}
			else
				phase = DSM_CH1_WRITE_A;					// change from CH2_CHECK_B to CH1_WRITE_A (upper already transmitted so transmit lower)
			#ifndef MULTI_AIR
				if(sub_protocol==DSM2_SFC)
					return DSM2_SFC_PERIOD - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
			#endif
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
#endif
	}
	return 0;		
}


#ifndef MULTI_AIR
const uint8_t PROGMEM DSMR_ID_FREQ[][4 + 23] = {
	{ 0x71, 0x74, 0x1c, 0xe4, 0x11, 0x2f, 0x17, 0x3d, 0x23, 0x3b, 0x0f, 0x21, 0x25, 0x49, 0x1d, 0x13, 0x4d, 0x1f, 0x41, 0x4b, 0x47, 0x05, 0x27, 0x15, 0x19, 0x3f, 0x07 },
	{ 0xfe, 0xfe, 0xfe, 0xfe, 0x45, 0x31, 0x33, 0x4b, 0x11, 0x29, 0x49, 0x3f, 0x09, 0x13, 0x47, 0x21, 0x1d, 0x43, 0x1f, 0x05, 0x41, 0x19, 0x1b, 0x2d, 0x15, 0x4d, 0x0f },
	{ 0xfe, 0xff, 0xff, 0xff, 0x2a, 0x06, 0x22, 0x28, 0x16, 0x24, 0x38, 0x0e, 0x32, 0x2e, 0x14, 0x3a, 0x04, 0x44, 0x0c, 0x42, 0x1c, 0x4a, 0x10, 0x36, 0x3c, 0x48, 0x26 },
	{ 0xff, 0xfe, 0xff, 0xff, 0x28, 0x34, 0x48, 0x46, 0x3a, 0x12, 0x18, 0x32, 0x14, 0x42, 0x16, 0x40, 0x22, 0x44, 0x1c, 0x0a, 0x36, 0x20, 0x10, 0x0c, 0x3c, 0x26, 0x2e },
	{ 0xff, 0xff, 0xfe, 0xff, 0x3c, 0x16, 0x04, 0x48, 0x1e, 0x4a, 0x10, 0x18, 0x22, 0x28, 0x38, 0x40, 0x20, 0x06, 0x3e, 0x42, 0x30, 0x1a, 0x2c, 0x1c, 0x46, 0x14, 0x34 },
	{ 0xff, 0xff, 0xff, 0xfe, 0x4d, 0x39, 0x1b, 0x13, 0x45, 0x2f, 0x0d, 0x3d, 0x0b, 0x11, 0x47, 0x2d, 0x19, 0x1d, 0x23, 0x35, 0x33, 0x3b, 0x21, 0x31, 0x17, 0x0f, 0x43 },
	{ 0xff, 0xff, 0xff, 0xff, 0x14, 0x28, 0x2e, 0x32, 0x3e, 0x10, 0x38, 0x0e, 0x12, 0x06, 0x2c, 0x26, 0x30, 0x4c, 0x34, 0x16, 0x04, 0x3a, 0x42, 0x48, 0x36, 0x46, 0x1a },
	{ 0x00, 0xff, 0xff, 0xff, 0x3e, 0x30, 0x42, 0x24, 0x06, 0x0e, 0x14, 0x1c, 0x08, 0x10, 0x20, 0x22, 0x04, 0x32, 0x0c, 0x44, 0x3c, 0x46, 0x4a, 0x26, 0x4c, 0x48, 0x1e },
	{ 0xff, 0x00, 0xff, 0xff, 0x38, 0x0e, 0x22, 0x2a, 0x44, 0x3a, 0x4a, 0x3e, 0x16, 0x20, 0x36, 0x24, 0x46, 0x18, 0x1e, 0x12, 0x1c, 0x30, 0x2c, 0x14, 0x06, 0x0c, 0x40 },
	{ 0x00, 0x00, 0xff, 0xff, 0x06, 0x4c, 0x26, 0x08, 0x46, 0x3e, 0x30, 0x12, 0x38, 0x1c, 0x04, 0x4a, 0x2c, 0x1a, 0x20, 0x3a, 0x18, 0x36, 0x28, 0x2e, 0x22, 0x40, 0x10 },
	{ 0xff, 0xff, 0x00, 0xff, 0x12, 0x06, 0x3c, 0x2a, 0x22, 0x38, 0x48, 0x4c, 0x32, 0x44, 0x26, 0x16, 0x0c, 0x28, 0x2c, 0x36, 0x1c, 0x1a, 0x42, 0x10, 0x08, 0x4a, 0x34 },
	{ 0x00, 0xff, 0x00, 0xff, 0x04, 0x4c, 0x4a, 0x28, 0x2a, 0x24, 0x14, 0x1e, 0x40, 0x48, 0x44, 0x2c, 0x2e, 0x1a, 0x12, 0x46, 0x3a, 0x0e, 0x18, 0x1c, 0x20, 0x10, 0x42 },
	{ 0xff, 0x00, 0x00, 0xff, 0x06, 0x10, 0x14, 0x16, 0x48, 0x18, 0x44, 0x2c, 0x0a, 0x26, 0x24, 0x42, 0x36, 0x30, 0x38, 0x3e, 0x0c, 0x3c, 0x34, 0x46, 0x2a, 0x32, 0x0e },
	{ 0x00, 0x00, 0x00, 0xff, 0x2c, 0x0a, 0x46, 0x28, 0x38, 0x24, 0x14, 0x06, 0x04, 0x10, 0x18, 0x30, 0x12, 0x20, 0x3a, 0x1a, 0x32, 0x3c, 0x3e, 0x4a, 0x1e, 0x44, 0x36 },
	{ 0x00, 0x00, 0x00, 0x00, 0x45, 0x23, 0x07, 0x37, 0x4b, 0x13, 0x3d, 0x31, 0x19, 0x2b, 0x2f, 0x2d, 0x1f, 0x4d, 0x3f, 0x1b, 0x43, 0x27, 0x3b, 0x11, 0x05, 0x0d, 0x17 },
	{ 0xff, 0xff, 0xff, 0x00, 0x0b, 0x4b, 0x1d, 0x39, 0x09, 0x0f, 0x49, 0x25, 0x07, 0x35, 0x3b, 0x05, 0x33, 0x17, 0x2d, 0x11, 0x2b, 0x29, 0x1f, 0x45, 0x1b, 0x41, 0x47 },
	{ 0x00, 0xff, 0xff, 0x00, 0x41, 0x35, 0x11, 0x25, 0x29, 0x27, 0x33, 0x47, 0x4d, 0x31, 0x05, 0x37, 0x15, 0x1f, 0x23, 0x07, 0x1b, 0x0f, 0x3b, 0x49, 0x19, 0x3f, 0x0b },
	{ 0xff, 0x00, 0xff, 0x00, 0x25, 0x47, 0x05, 0x0b, 0x45, 0x1f, 0x2b, 0x27, 0x2d, 0x09, 0x07, 0x43, 0x49, 0x29, 0x4d, 0x39, 0x33, 0x41, 0x17, 0x0f, 0x15, 0x19, 0x3b },
	{ 0x00, 0x00, 0xff, 0x00, 0x3b, 0x05, 0x21, 0x0d, 0x1b, 0x43, 0x17, 0x2d, 0x1d, 0x25, 0x4b, 0x35, 0x4d, 0x3f, 0x07, 0x09, 0x37, 0x41, 0x15, 0x1f, 0x0f, 0x27, 0x29 },
	{ 0xff, 0xff, 0x00, 0x00, 0x2b, 0x35, 0x1b, 0x1d, 0x0f, 0x47, 0x09, 0x0d, 0x45, 0x41, 0x21, 0x11, 0x2f, 0x43, 0x27, 0x33, 0x4b, 0x37, 0x13, 0x19, 0x4d, 0x23, 0x17 },
	{ 0x00, 0xff, 0x00, 0x00, 0x1b, 0x1d, 0x33, 0x13, 0x2b, 0x27, 0x09, 0x41, 0x25, 0x17, 0x19, 0x2d, 0x4b, 0x37, 0x45, 0x11, 0x21, 0x0d, 0x3d, 0x4d, 0x07, 0x39, 0x43 },
	{ 0xff, 0x00, 0x00, 0x00, 0x37, 0x27, 0x43, 0x4b, 0x39, 0x13, 0x07, 0x0d, 0x25, 0x17, 0x29, 0x1b, 0x1d, 0x45, 0x19, 0x2d, 0x0b, 0x3d, 0x15, 0x47, 0x1f, 0x21, 0x4d } };
#endif

void DSM_init()
{ 
	if(sub_protocol == DSMR)
	{
		#ifndef MULTI_AIR
			if(option&CLONE_BIT_MASK)
				SUB_PROTO_INVALID;
			else
			{
				//SUB_PROTO_VALID;
				uint8_t row = rx_tx_addr[3]%22;
				for(uint8_t i=0; i< 4; i++)
					cyrfmfg_id[i] = pgm_read_byte_near(&DSMR_ID_FREQ[row][i]);
				for(uint8_t i=0; i< 23; i++)
					hopping_frequency[i] = pgm_read_byte_near(&DSMR_ID_FREQ[row][i+4]);
			}
		#endif
	}
	else
	{
		if(option&CLONE_BIT_MASK)
		{ 
			if(eeprom_read_byte((EE_ADDR)DSM_CLONE_EEPROM_OFFSET+4)==0xF0)
			{
				//read cloned ID from EEPROM
				uint16_t temp = DSM_CLONE_EEPROM_OFFSET;
				for(uint8_t i=0;i<4;i++)
					cyrfmfg_id[i] = eeprom_read_byte((EE_ADDR)temp++);
				#if DEBUG_BIND
					debugln("Using cloned ID");  
					debug("Clone ID=")
					for(uint8_t i=0;i<4;i++)
						debug("%02x ", cyrfmfg_id[i]);
					debugln("");
				#endif
			}
			else
			{
				SUB_PROTO_INVALID;
				#if DEBUG_BIND
				  debugln("No valid cloned ID");
				#endif
			}
		}
		else
		{
			//SUB_PROTO_VALID;
			CYRF_GetMfgData(cyrfmfg_id);
		}
	}
	//Model match
	cyrfmfg_id[3]^=RX_num;

	//Calc sop_col
	sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;

	//We cannot manipulate the ID if we are cloning
	if(!(option&CLONE_BIT_MASK))
	{	
		//Fix for OrangeRX using wrong DSM_pncodes by preventing access to "Col 8"
		if(sop_col==0 && sub_protocol != DSMR)
		{
			cyrfmfg_id[rx_tx_addr[0]%3]^=0x01;					//Change a bit so sop_col will be different from 0
			sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
		}
	}

	//Calc CRC seed
	seed = (cyrfmfg_id[0] << 8) + cyrfmfg_id[1];

	//Hopping frequencies
	if (sub_protocol == DSMX_2F || sub_protocol == DSMX_1F)
		DSM_calc_dsmx_channel();
	else if(sub_protocol != DSMR)
	{ 
		uint8_t tmpch[10];
		CYRF_FindBestChannels(tmpch, 10, 5, 3, 75);
		//
		uint8_t idx = random(0xfefefefe) % 10;
		hopping_frequency[0] = tmpch[idx];
		while(1)
		{
			idx = random(0xfefefefe) % 10;
			if (tmpch[idx] != hopping_frequency[0])
				break;
		}
		hopping_frequency[1] = tmpch[idx];
	}

	//
	DSM_cyrf_config();
	CYRF_SetTxRxMode(TX_EN);
	//
	DSM_update_channels();
	//
	if(IS_BIND_IN_PROGRESS)
	{
		DSM_initialize_bind_phase();		
		phase = DSM_BIND_WRITE;
		bind_counter=DSM_BIND_COUNT;
		#if DEBUG_BIND
			debugln("Bind Started: write count=%d",bind_counter);
		#endif
	}
	else
		phase = DSM_CHANSEL;
}

#endif
