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

#if defined(HOTT_CC2500_INO)

#include "iface_cc2500.h"

//#define HOTT_FORCE_ID		// Force ID of original dump

#define HOTT_TX_PACKET_LEN	50
#define HOTT_RX_PACKET_LEN	22
#define HOTT_PACKET_PERIOD	10000
#define HOTT_NUM_RF_CHANNELS 75
#define HOTT_COARSE	0

enum {
    HOTT_START = 0x00,
    HOTT_CAL   = 0x01,
    HOTT_DATA1 = 0x02,
    HOTT_RX1   = 0x03,
    HOTT_RX2   = 0x04,
};

#define HOTT_FREQ0_VAL 0x6E

// Some important initialization parameters, all others are either default,
// or not important in the context of transmitter
// FIFOTHR  00
// SYNC1    D3
// SYNC0    91
// PKTLEN   32 - Packet length, 50 bytes
// PKTCTRL1 04 - APPEND_STATUS on=RSSI+LQI, all other are receive parameters - irrelevant
// PKTCTRL0 44 - whitening, use FIFO, use CRC, fixed packet length
// ADDR     00
// CHANNR   10
// FSCTRL1  09 - IF 
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequencyfor 26MHz crystal
// FREQ1    6C
// FREQ0    B9
// MDMCFG4  2D - 
// MDMCFG3  3B - 
// MDMCFG2  73 - disable DC blocking, MSK, no Manchester code, 32 bits sync word
// MDMCFG1  A3 - FEC enable, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  AA - CHANSPC_M - AA
// DEVIATN  47 - 
// MCSM2    07 - 
// MCSM1    00 - always use CCA, go to IDLE when done
// MCSM0    08 - disable autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D
const PROGMEM uint8_t HOTT_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x00, 0xD3, 0x91, 0x32, 0x04,
  /* 08 */ 0x44, 0x00, 0x00, 0x09, 0x00, 0x5C, 0x6C, HOTT_FREQ0_VAL + HOTT_COARSE,
  /* 10 */ 0x2D, 0x3B, 0x73, 0xA3, 0xAA, 0x47, 0x07, 0x00,
  /* 18 */ 0x08, 0x1D, 0x1C, 0xC7, 0x09, 0xF0, 0x87, 0x6B,
  /* 20 */ 0xF0, 0xB6, 0x10, 0xEA, 0x0A, 0x00, 0x11
};

static void __attribute__((unused)) HOTT_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&HOTT_init_values[i]));

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

static void __attribute__((unused)) HOTT_tune_chan()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, (rf_ch_num+1)*3);
	CC2500_Strobe(CC2500_SCAL);
}

static void __attribute__((unused)) HOTT_tune_chan_fast()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, (rf_ch_num+1)*3);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[rf_ch_num]);
}

static void __attribute__((unused)) HOTT_tune_freq()
{
	if ( prev_option != option )
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		CC2500_WriteReg(CC2500_0F_FREQ0, HOTT_FREQ0_VAL + HOTT_COARSE);
		prev_option = option ;
		phase = HOTT_START;	// Restart the tune process if option is changed to get good tuned values
	}
}

const uint8_t PROGMEM HOTT_hop[][HOTT_NUM_RF_CHANNELS]=
	{	{ 48, 37, 16, 62, 9, 50, 42, 22, 68, 0, 55, 35, 21, 74, 1, 56, 31, 20, 70, 11, 45, 32, 24, 71, 8, 54, 38, 26, 61, 13, 53, 30, 15, 65, 7, 52, 34, 28, 60, 3, 47, 39, 18, 69, 2, 49, 44, 23, 72, 5, 51, 43, 19, 64, 12, 46, 33, 17, 67, 6, 58, 36, 29, 73, 14, 57, 41, 25, 63, 4, 59, 40, 27, 66, 10 },
		{ 50, 23, 5, 34, 67, 53, 22, 12, 39, 62, 51, 21, 10, 33, 63, 59, 16, 1, 43, 66, 49, 19, 8, 30, 71, 47, 24, 2, 35, 68, 45, 25, 14, 41, 74, 55, 18, 4, 32, 61, 54, 17, 11, 31, 72, 52, 28, 6, 38, 65, 46, 15, 9, 40, 60, 48, 26, 3, 37, 70, 58, 29, 0, 36, 64, 56, 20, 7, 42, 69, 57, 27, 13, 44, 73 },
		{ 73, 51, 39, 18, 9, 64, 56, 34, 16, 12, 66, 58, 36, 25, 11, 61, 47, 40, 15, 8, 71, 50, 43, 20, 6, 62, 54, 42, 19, 3, 63, 46, 44, 29, 14, 72, 49, 33, 22, 5, 69, 57, 30, 21, 10, 70, 45, 35, 26, 7, 65, 59, 31, 28, 1, 67, 48, 32, 24, 0, 60, 55, 41, 17, 2, 74, 52, 38, 27, 4, 68, 53, 37, 23, 13 },
		{ 52, 60, 40, 21, 14, 50, 72, 41, 23, 13, 59, 61, 39, 16, 6, 58, 66, 33, 17, 5, 55, 64, 43, 20, 12, 54, 74, 35, 29, 3, 46, 63, 37, 22, 10, 48, 65, 31, 27, 9, 49, 73, 38, 24, 11, 56, 70, 32, 15, 1, 51, 71, 44, 18, 8, 45, 67, 36, 25, 7, 57, 62, 34, 28, 2, 53, 69, 42, 19, 4, 47, 68, 30, 26, 0 },
		{ 50, 16, 34, 6, 71, 51, 24, 40, 7, 68, 57, 27, 33, 14, 70, 55, 26, 30, 5, 74, 47, 28, 44, 11, 67, 49, 15, 32, 9, 61, 52, 22, 37, 13, 66, 59, 18, 42, 3, 62, 46, 29, 31, 12, 60, 48, 19, 38, 1, 72, 58, 17, 36, 4, 64, 53, 21, 39, 0, 63, 56, 20, 41, 2, 65, 45, 25, 35, 10, 69, 54, 23, 43, 8, 73 },
		{ 55, 38, 12, 62, 23, 52, 44, 3, 66, 18, 54, 36, 10, 74, 16, 56, 42, 9, 70, 17, 58, 33, 5, 69, 20, 50, 40, 1, 63, 24, 53, 37, 13, 65, 15, 48, 34, 4, 61, 22, 57, 31, 6, 64, 26, 46, 35, 11, 72, 21, 47, 30, 7, 68, 29, 45, 32, 8, 60, 19, 49, 43, 2, 67, 27, 51, 39, 0, 71, 28, 59, 41, 14, 73, 25 },
		{ 70, 32, 18, 10, 58, 69, 38, 22, 2, 54, 67, 36, 19, 12, 57, 62, 34, 20, 14, 52, 63, 41, 15, 3, 51, 73, 42, 28, 6, 48, 60, 43, 29, 5, 45, 64, 31, 17, 4, 56, 65, 35, 26, 13, 53, 61, 37, 23, 1, 49, 68, 40, 16, 9, 47, 71, 39, 25, 7, 50, 66, 33, 24, 8, 59, 72, 44, 27, 11, 46, 74, 30, 21, 0, 55 },
		{ 6, 45, 71, 27, 44, 10, 46, 74, 22, 32, 0, 55, 69, 21, 33, 4, 50, 66, 18, 38, 7, 57, 62, 19, 36, 1, 48, 70, 20, 40, 8, 47, 68, 15, 43, 2, 58, 61, 26, 42, 3, 56, 72, 23, 34, 14, 54, 67, 16, 37, 5, 59, 64, 24, 30, 12, 52, 65, 25, 39, 13, 49, 73, 17, 31, 9, 53, 60, 28, 35, 11, 51, 63, 29, 41 },
		{ 31, 65, 50, 20, 13, 37, 66, 45, 23, 5, 32, 69, 54, 19, 7, 39, 74, 52, 27, 1, 42, 64, 53, 22, 4, 43, 70, 58, 16, 3, 40, 71, 57, 17, 0, 35, 63, 56, 18, 9, 44, 72, 51, 21, 6, 33, 67, 46, 25, 11, 30, 73, 55, 15, 8, 36, 62, 48, 24, 10, 41, 60, 49, 29, 14, 34, 61, 47, 26, 2, 38, 68, 59, 28, 12 },
		{ 67, 22, 49, 36, 13, 64, 28, 57, 37, 6, 65, 29, 46, 39, 3, 70, 26, 45, 35, 1, 62, 24, 58, 34, 10, 68, 19, 53, 33, 4, 66, 21, 52, 31, 7, 74, 18, 47, 32, 5, 61, 16, 51, 38, 8, 72, 23, 55, 30, 12, 73, 17, 59, 44, 0, 60, 15, 50, 43, 14, 63, 27, 48, 42, 11, 71, 20, 54, 41, 9, 69, 25, 56, 40, 2 },
		{ 19, 38, 14, 66, 57, 18, 44, 7, 74, 48, 23, 30, 6, 71, 58, 26, 32, 5, 61, 46, 20, 34, 0, 68, 45, 24, 36, 1, 70, 50, 27, 33, 10, 63, 52, 16, 42, 9, 65, 51, 15, 41, 11, 64, 53, 22, 37, 3, 60, 56, 28, 35, 4, 67, 49, 17, 39, 13, 69, 54, 25, 43, 2, 73, 55, 21, 31, 8, 62, 47, 29, 40, 12, 72, 59 },
		{ 4, 52, 64, 28, 44, 14, 46, 74, 16, 32, 11, 50, 68, 27, 36, 0, 47, 70, 26, 34, 13, 57, 61, 18, 38, 6, 56, 62, 19, 40, 5, 58, 67, 17, 31, 12, 54, 63, 22, 33, 3, 53, 72, 21, 41, 10, 48, 66, 15, 35, 7, 45, 60, 20, 37, 9, 51, 69, 25, 42, 2, 59, 71, 24, 39, 1, 55, 65, 23, 30, 8, 49, 73, 29, 43 },
		{ 44, 66, 19, 1, 56, 35, 62, 20, 4, 54, 39, 70, 24, 5, 55, 31, 74, 26, 12, 58, 32, 60, 17, 10, 45, 37, 63, 22, 3, 50, 33, 64, 16, 7, 51, 34, 61, 21, 8, 48, 38, 68, 29, 0, 46, 36, 72, 28, 14, 49, 42, 69, 25, 6, 57, 43, 65, 18, 2, 52, 30, 71, 23, 13, 47, 41, 67, 15, 9, 53, 40, 73, 27, 11, 59 },
		{ 12, 16, 36, 46, 69, 6, 20, 44, 58, 62, 11, 19, 34, 48, 71, 1, 18, 42, 50, 74, 3, 25, 31, 47, 65, 0, 24, 33, 45, 72, 2, 23, 35, 56, 64, 10, 22, 38, 49, 63, 7, 26, 37, 51, 70, 14, 21, 30, 53, 67, 5, 15, 40, 52, 66, 9, 17, 39, 55, 60, 13, 27, 41, 54, 73, 4, 28, 32, 57, 61, 8, 29, 43, 59, 68 },
		{ 63, 42, 18, 2, 57, 71, 34, 22, 10, 48, 67, 36, 25, 4, 46, 60, 31, 28, 6, 47, 74, 37, 15, 0, 55, 65, 32, 24, 12, 56, 66, 40, 27, 14, 52, 62, 38, 19, 3, 50, 73, 33, 29, 11, 53, 61, 35, 16, 7, 58, 72, 41, 26, 5, 59, 69, 30, 20, 9, 51, 68, 44, 23, 1, 49, 70, 39, 17, 8, 54, 64, 43, 21, 13, 45 },
		{ 52, 1, 71, 17, 36, 47, 7, 64, 26, 32, 53, 5, 60, 20, 42, 57, 2, 66, 18, 34, 56, 4, 63, 24, 35, 46, 13, 72, 22, 30, 48, 0, 67, 21, 39, 50, 3, 74, 16, 31, 59, 14, 61, 23, 37, 45, 6, 65, 19, 44, 51, 11, 62, 27, 41, 55, 9, 68, 15, 38, 58, 8, 70, 29, 40, 54, 10, 69, 28, 33, 49, 12, 73, 25, 43 }
		};
const uint16_t PROGMEM HOTT_hop_val[] = { 0xC06B, 0xC34A, 0xDB24, 0x8E09, 0x272E, 0x217F, 0x155B, 0xEDE8, 0x1D31, 0x0986, 0x56F7, 0x6454, 0xC42D, 0x01D2, 0xC253, 0x1180 };

static void __attribute__((unused)) HOTT_init()
{
	packet[0] = pgm_read_word_near( &HOTT_hop_val[num_ch] );
	packet[1] = pgm_read_word_near( &HOTT_hop_val[num_ch] )>>8;

	for(uint8_t i=0; i<HOTT_NUM_RF_CHANNELS; i++)
		hopping_frequency[i]=pgm_read_byte_near( &HOTT_hop[num_ch][i] );
	
	#ifdef HOTT_FORCE_ID
		memcpy(rx_tx_addr,"\x7C\x94\x00\x0D\x50",5);
	#endif
	memset(&packet[30],0xFF,9);
	packet[39]=0x07;		// unknown and constant
	if(IS_BIND_IN_PROGRESS)
	{
		packet[28] = 0x80;	// unknown 0x80 when bind starts then when RX replies start normal, 0x89/8A/8B/8C/8D/8E during normal packets
		packet[29] = 0x02;	// unknown 0x02 when bind starts then when RX replies cycle in sequence 0x1A/22/2A/0A/12, 0x02 during normal packets
		memset(&packet[40],0xFA,5);
		memcpy(&packet[45],rx_tx_addr,5);
	}
	else
	{
		packet[28] = 0x8C;	// unknown 0x80 when bind starts then when RX replies start normal, 0x89/8A/8B/8C/8D/8E during normal packets, 0x0F->config menu
		packet[29] = 0x02;	// unknown 0x02 when bind starts then when RX replies cycle in sequence 0x1A/22/2A/0A/12, 0x02 during normal packets, 0x01->config menu, 0x0A->no more RX telemetry
		memcpy(&packet[40],rx_tx_addr,5);

		uint8_t addr=HOTT_EEPROM_OFFSET+RX_num*5;
		for(uint8_t i=0;i<5;i++)
			packet[45+i]=eeprom_read_byte((EE_ADDR)(addr+i));
	}
}

static void __attribute__((unused)) HOTT_data_packet()
{
	packet[2] = hopping_frequency_no;

	packet[3] = 0x00;	// used for failsafe but may also be used for additional channels
	#ifdef FAILSAFE_ENABLE
		static uint8_t failsafe_count=0;
		if(IS_FAILSAFE_VALUES_on && IS_BIND_DONE)
		{
			failsafe_count++;
			if(failsafe_count>=3)
			{
				FAILSAFE_VALUES_off;
				failsafe_count=0;
			}
		}
		else
			failsafe_count=0;
	#endif

	// Channels value are PPM*2, -100%=1100µs, +100%=1900µs, order TAER
	uint16_t val;
	for(uint8_t i=4;i<28;i+=2)
	{
		val=Channel_data[(i-4)>>1];
		val=(((val<<2)+val)>>2)+860*2;					// value range 860<->2140 *2 <-> -125%<->+125%
		#ifdef FAILSAFE_ENABLE
			if(failsafe_count==1)
			{ // first failsafe packet
				packet[3]=0x40;
				uint16_t fs=Failsafe_data[(i-4)>>1];
				if( fs == FAILSAFE_CHANNEL_HOLD || fs == FAILSAFE_CHANNEL_NOPULSES)
					val|=0x8000;						// channel hold flag
				else
				{
					val=(((fs<<2)+fs)>>2)+860*2;		// value range 860<->2140 *2 <-> -125%<->+125%
					val|=0x4000;						// channel specific position flag
				}
			}
			else if(failsafe_count==2)
			{ // second failsafe packet=timing?
				packet[3]=0x50;
				if(i==4)
					val=2;
				else
					val=0;
			}
		#endif
		packet[i] = val;
		packet[i+1] = val>>8;
	}

	#ifdef HOTT_FW_TELEMETRY
		static uint8_t prev_SerialRX_val=0;
		if(HoTT_SerialRX && HoTT_SerialRX_val >= 0xD7 && HoTT_SerialRX_val <= 0xDF)
		{
			if(prev_SerialRX_val!=HoTT_SerialRX_val)
			{
				prev_SerialRX_val=HoTT_SerialRX_val;
				packet[28] = HoTT_SerialRX_val;			// send the touch being pressed only once
			}
			else
				packet[28] = 0xDF;						// no touch pressed
			packet[29] = 0x01;							// 0x01->config menu
		}
		else
		{
			packet[28] = 0x8C;							// unknown 0x80 when bind starts then when RX replies start normal, 0x89/8A/8B/8C/8D/8E during normal packets, 0x0F->config menu
			packet[29] = 0x02;							// unknown 0x02 when bind starts then when RX replies cycle in sequence 0x1A/22/2A/0A/12, 0x02 during normal packets, 0x01->config menu, 0x0A->no more RX telemetry
		}
	#endif

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
	CC2500_WriteReg(CC2500_06_PKTLEN, 0x32);
	CC2500_WriteData(packet, HOTT_TX_PACKET_LEN);
	#if 0
		debug("RF:%02X P:",rf_ch_num);
		for(uint8_t i=0;i<HOTT_TX_PACKET_LEN;i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
	hopping_frequency_no++;
	hopping_frequency_no %= HOTT_NUM_RF_CHANNELS;
	rf_ch_num=hopping_frequency[hopping_frequency_no];
}

uint16_t ReadHOTT()
{
	#ifdef HOTT_FW_TELEMETRY
		static uint8_t pps_counter=0;
	#endif

	switch(phase)
	{
		case HOTT_START:
			rf_ch_num = 0;
			HOTT_tune_chan();
			phase = HOTT_CAL;
			return 2000;
		case HOTT_CAL:
			calData[rf_ch_num]=CC2500_ReadReg(CC2500_25_FSCAL1);
			if (++rf_ch_num < HOTT_NUM_RF_CHANNELS)
				HOTT_tune_chan();
			else
			{
				hopping_frequency_no = 0;
				rf_ch_num=hopping_frequency[hopping_frequency_no];
				counter = 0;
				phase = HOTT_DATA1;
			}
			return 2000;

		/* Work cycle: 10ms */
		case HOTT_DATA1:
			//TX
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(HOTT_PACKET_PERIOD);
			#endif
			HOTT_tune_freq();
			HOTT_tune_chan_fast();
			HOTT_data_packet();
			phase = HOTT_RX1;
			return 4500;
		case HOTT_RX1:
			//RX
			CC2500_SetTxRxMode(RX_EN);
			CC2500_WriteReg(CC2500_06_PKTLEN, HOTT_RX_PACKET_LEN);
			CC2500_Strobe(CC2500_SRX);
			phase = HOTT_RX2;
			return 4500;
		case HOTT_RX2:
			//Telemetry
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len==HOTT_RX_PACKET_LEN+2)
			{
				CC2500_ReadData(packet_in, len);
				if(memcmp(rx_tx_addr,packet_in,5)==0)
				{ // TX ID matches
					if(IS_BIND_IN_PROGRESS)
					{
						debug("B:");
						for(uint8_t i=0;i<HOTT_RX_PACKET_LEN;i++)
							debug(" %02X", packet_in[i]);
						debugln("");
						uint8_t addr=HOTT_EEPROM_OFFSET+RX_num*5;
						for(uint8_t i=0; i<5; i++)
							eeprom_write_byte((EE_ADDR)(addr+i),packet_in[5+i]);
						BIND_DONE;
						HOTT_init();
					}
					#ifdef HOTT_FW_TELEMETRY
						else
						{	//Telemetry
							// [0..4] = TXID
							// [5..9] = RXID
							// [10] = 0x40 bind, 0x00 normal, 0x80 config menu
							// [11] = telmetry pages. For sensors 0x00 to 0x04, for config mennu 0x00 to 0x12.
							// Normal telem page 0 = 0x00, 0x33, 0x34, 0x46, 0x64, 0x33, 0x0A, 0x00, 0x00, 0x00
							//				= 0x55, 0x32, 0x38, 0x55, 0x64, 0x32, 0xD0, 0x07, 0x00, 0x55
							//   Page 0 [12] = [21] = ??
							//   Page 0 [13] = RX_Voltage*10 in V
							//   Page 0 [14] = Temperature-20 in °C
							//   Page 0 [15] = RX_RSSI
							//   Page 0 [16] = RX_LQI ??
							//   Page 0 [17] = RX_STR ??
							//   Page 0 [18,19] = [19]*256+[18]=max lost packet time in ms, max value seems 2s=0x7D0
							//   Page 0 [20] = 0x00 ??
							// Config menu consists of the different telem pages put all together
							//   Page X [12] = seems like all the telem pages with the same value are going together to make the full config menu text. Seen so far 'a', 'b', 'c', 'd' 
							//   Page X [13..21] = 9 ascii chars to be displayed, char is highlighted when ascii|0x80
							//   Screen display is 21 characters large which means that once the first 21 chars are filled go to the begining of the next line
							//   Menu commands are sent through TX packets:
							//     packet[28]= 0xXF=>no key press, 0xXD=>down, 0xXB=>up, 0xX9=>enter, 0xXE=>right, 0xX7=>left with X=0 or D
							//     packet[29]= 0xX1/0xX9 with X=0 or X counting 0,1,1,2,2,..,9,9
							TX_RSSI = packet_in[22];
							if(TX_RSSI >=128)
								TX_RSSI -= 128;
							else
								TX_RSSI += 128;
							// Reduce telemetry to 14 bytes
							packet_in[0]= TX_RSSI;
							packet_in[1]= TX_LQI;
							debug("T=");
							for(uint8_t i=10;i < HOTT_RX_PACKET_LEN; i++)
							{
								packet_in[i-8]=packet_in[i];
								debug(" %02X",packet_in[i]);
							}
							debugln("");
							telemetry_link=2;
						}
						pps_counter++;
					#endif
				}
			}
			#ifdef HOTT_FW_TELEMETRY
				packet_count++;
				if(packet_count>=100)
				{
					TX_LQI=pps_counter;
					pps_counter=packet_count=0;
				}
			#endif
			CC2500_Strobe(CC2500_SFRX);							//Flush the RXFIFO
			phase=HOTT_DATA1;
			return 1000;
	}
	return 0;
}

uint16_t initHOTT()
{
	num_ch=random(0xfefefefe)%16;
	HOTT_init();
	HOTT_rf_init();
	packet_count=0;
	#ifdef HOTT_FW_TELEMETRY
		HoTT_SerialRX_val=0;
		HoTT_SerialRX=false;
	#endif
	phase = HOTT_START;
	return 10000;
}

#endif