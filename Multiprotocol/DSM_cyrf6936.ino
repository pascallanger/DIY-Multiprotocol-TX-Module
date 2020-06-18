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

//#define DSM_GR300

#define DSM_BIND_CHANNEL 0x0d //13 This can be any odd channel

//During binding we will send BIND_COUNT/2 packets
//One packet each 10msec
#define DSM_BIND_COUNT 300

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
		packet[11] = num_ch;

	if (sub_protocol==DSM2_22)
		packet[12]=num_ch<8?0x01:0x02;		// DSM2/1024 1 or 2 packets depending on the number of channels
	else if(sub_protocol==DSM2_11)
		packet[12]=0x12;					// DSM2/2048 2 packets
	else if(sub_protocol==DSMX_22)
		#if defined DSM_TELEMETRY
			packet[12] = 0xb2;				// DSMX/2048 2 packets
		#else
			packet[12] = num_ch<8? 0xa2 : 0xb2;	// DSMX/2048 1 or 2 packets depending on the number of channels
		#endif
	else									// DSMX_11 && DSM_AUTO
		packet[12]=0xb2;					// DSMX/2048 2 packets
	
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
	DSM_read_code(code,0,8,8);
	CYRF_ConfigDataCode(code, 16);
	DSM_build_bind_packet();
}

static void __attribute__((unused)) DSM_update_channels()
{
	prev_option=option;
	num_ch=option & 0x0F;				// Remove flags 0x80=max_throw, 0x40=11ms

	if(num_ch<3 || num_ch>12)
		num_ch=6;						// Default to 6 channels if invalid choice...

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

	if(prev_option!=option)
		DSM_update_channels();

	if (sub_protocol==DSMX_11 || sub_protocol==DSMX_22 )
	{//DSMX
		packet[0] = cyrfmfg_id[2];
		packet[1] = cyrfmfg_id[3];
	}
	else
	{//DSM2
		packet[0] = (0xff ^ cyrfmfg_id[2]);
		packet[1] = (0xff ^ cyrfmfg_id[3]);
		if(sub_protocol==DSM2_22)
			bits=10;								// Only DSM2_22 is using a resolution of 1024
	}
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
					value=Channel_data[CH_TAER[idx]];									// -100%..+100% => 1024..1976us and -125%..+125% => 904..2096us based on Redcon 6 channel DSM2 RX
				#else
					if(option & 0x80)
						value=Channel_data[CH_TAER[idx]];								// -100%..+100% => 1024..1976us and -125%..+125% => 904..2096us based on Redcon 6 channel DSM2 RX
					else
						value=convert_channel_16b_nolimit(CH_TAER[idx],0x156,0x6AA);	// -100%..+100% => 1100..1900us and -125%..+125% => 1000..2000us based on a DX8 G2 dump
				#endif
			if(bits==10) value>>=1;
			value |= (upper && i==0 ? 0x8000 : 0) | (idx << bits);
		}	  
		packet[i*2+2] = (value >> 8) & 0xff;
		packet[i*2+3] = (value >> 0) & 0xff;
	}
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

uint16_t ReadDsm()
{
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

	#ifdef DSM_GR300
		uint16_t timing=5000+(convert_channel_8b(CH13)*100);
		debugln("T=%u",timing);
	#endif
	
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
			//64 SDR Mode is configured so only the 8 first values are needed but we need to write 16 values...
			CYRF_ConfigDataCode((const uint8_t *)"\x98\x88\x1B\xE4\x30\x79\x03\x84", 16);
			CYRF_SetTxRxMode(RX_EN);							//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);			//Prepare to receive
			bind_counter=2*DSM_BIND_COUNT;						//Timeout of 4.2s if no packet received
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
						debug("Bind");
						for(uint8_t i=0;i<10;i++)
							debug(" %02X",packet_in[i+1]);
						debugln("");
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
			phase = DSM_CH1_WRITE_A;						// in fact phase++
			DSM_set_sop_data_crc(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B, sub_protocol==DSMX_11||sub_protocol==DSMX_22);
			return 10000;
		case DSM_CH1_WRITE_A:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(11000);			// Always request 11ms spacing even if we don't use half of it in 22ms mode
			#endif
		case DSM_CH1_WRITE_B:
		case DSM_CH2_WRITE_A:
		case DSM_CH2_WRITE_B:
			DSM_build_data_packet(phase == DSM_CH1_WRITE_B||phase == DSM_CH2_WRITE_B);	// build lower or upper channels
			CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS);		// clear IRQ flags
			CYRF_WriteDataPacket(packet);
			#if 0
				for(uint8_t i=0;i<16;i++)
					debug(" %02X", packet[i]);
				debugln("");
			#endif
			phase++;										// change from WRITE to CHECK mode
			return DSM_WRITE_DELAY;
		case DSM_CH1_CHECK_A:
		case DSM_CH1_CHECK_B:
		case DSM_CH2_CHECK_A:
		case DSM_CH2_CHECK_B:
			start=(uint8_t)micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 100)			// Wait max 100µs, max I've seen is 50µs
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;
			
			if(phase==DSM_CH1_CHECK_A || phase==DSM_CH1_CHECK_B)
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
				DSM_set_sop_data_crc(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B, sub_protocol==DSMX_11 || sub_protocol==DSMX_22);
				phase++;										// change from CH1_CHECK to CH2_WRITE
				return DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
			}
			if (phase == DSM_CH2_CHECK_A)
				CYRF_SetPower(0x28);						//Keep transmit power in sync
#if defined DSM_TELEMETRY
			phase++;										// change from CH2_CHECK to CH2_READ
			CYRF_SetTxRxMode(RX_EN);						//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);		//0x80??? //Prepare to receive
			#ifdef DSM_GR300
				if(num_ch==3)
					return timing - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY - DSM_READ_DELAY;
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
				packet_in[0]=CYRF_ReadRegister(CYRF_13_RSSI)&0x1F;// store RSSI of the received telemetry signal
				telemetry_link=1;
			}
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Abort RX operation
			if (phase == DSM_CH2_READ_A && (sub_protocol==DSM2_22 || sub_protocol==DSMX_22) && num_ch < 8)	// 22ms mode
			{
				CYRF_SetTxRxMode(RX_EN);					// Force end state read
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX operation
				CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x87);	//0x80???	//Prepare to receive
				phase = DSM_CH2_READ_B;
				#ifdef DSM_GR300
					if(num_ch==3)
						return timing;
				#endif
				return 11000;
			}
			if (phase == DSM_CH2_READ_A)
				phase = DSM_CH1_WRITE_B;					//Transmit upper
			else
				phase = DSM_CH1_WRITE_A;					//Transmit lower
			CYRF_SetTxRxMode(TX_EN);						//TX mode
			CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		//Clear abort RX operation
			DSM_set_sop_data_crc(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B, sub_protocol==DSMX_11||sub_protocol==DSMX_22);
			return DSM_READ_DELAY;
#else
			// No telemetry
			DSM_set_sop_data_crc(phase==DSM_CH1_CHECK_A||phase==DSM_CH1_CHECK_B, sub_protocol==DSMX_11||sub_protocol==DSMX_22);
			if (phase == DSM_CH2_CHECK_A)
			{
				if(num_ch > 7 || sub_protocol==DSM2_11 || sub_protocol==DSMX_11)
					phase = DSM_CH1_WRITE_B;				//11ms mode or upper to transmit change from CH2_CHECK_A to CH1_WRITE_A
				else										
				{											//Normal mode 22ms
					phase = DSM_CH1_WRITE_A;				// change from CH2_CHECK_A to CH1_WRITE_A (ie no upper)
					#ifdef DSM_GR300
						if(num_ch==3)
							return timing - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
					#endif
					return 22000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
				}
			}
			else
				phase = DSM_CH1_WRITE_A;					// change from CH2_CHECK_B to CH1_WRITE_A (upper already transmitted so transmit lower)
			#ifdef DSM_GR300
				if(num_ch==3)
					return timing - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY ;
			#endif
			return 11000 - DSM_CH1_CH2_DELAY - DSM_WRITE_DELAY;
#endif
	}
	return 0;		
}

uint16_t initDsm()
{ 
	CYRF_GetMfgData(cyrfmfg_id);
	//Model match
	cyrfmfg_id[3]^=RX_num;
	//Calc sop_col
	sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
	//Fix for OrangeRX using wrong DSM_pncodes by preventing access to "Col 8"
	if(sop_col==0)
	{
	   cyrfmfg_id[rx_tx_addr[0]%3]^=0x01;					//Change a bit so sop_col will be different from 0
	   sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
	}
	//Calc CRC seed
	seed = (cyrfmfg_id[0] << 8) + cyrfmfg_id[1];
	//Hopping frequencies
	if (sub_protocol == DSMX_11 || sub_protocol == DSMX_22)
		DSM_calc_dsmx_channel();
	else
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
	}
	else
		phase = DSM_CHANSEL;//
	return 10000;
}

#endif
