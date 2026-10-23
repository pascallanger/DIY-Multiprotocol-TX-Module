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
    HOTT_DATA2 = 0x03,
    HOTT_RX1   = 0x04,
    HOTT_RX2   = 0x05,
};

#ifdef HOTT_FW_TELEMETRY
	#define HOTT_SENSOR_TYPE 6
	#define HOTT_SENSOR_SEARCH_PERIOD 2000
	uint8_t HOTT_sensor_cur=0;
	uint8_t HOTT_sensor_pages=0;
	uint8_t HOTT_sensor_valid=false;
	uint8_t HOTT_sensor_ok[HOTT_SENSOR_TYPE];
	uint8_t HOTT_sensor_seq=0;
#endif

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
		phase = HOTT_START;								// Restart the tune process if option is changed to get good tuned values
	}
}

// Hop table from the value sent in packet[0..1]: the C library rand() (low 16
// bits) seeded with it drives forward Fisher-Yates shuffles, first of the order
// of the 5 bands of 15 channels, then of the channels of each band. Hop i is in
// the band at position i%5 of that order.
static uint16_t __attribute__((unused)) HOTT_rand(uint16_t *seed)
{
	*seed = *seed * 0x4E6D + 0x3039;
	return *seed;
}

static void __attribute__((unused)) HOTT_hop_init(uint16_t seed)
{
	uint8_t order[5], tmp, j;
	for(uint8_t i=0; i<5; i++)
		order[i] = i;
	for(uint8_t i=0; i<5; i++)
	{
		j = i + HOTT_rand(&seed) % (5-i);
		tmp = order[i]; order[i] = order[j]; order[j] = tmp;
	}
	for(uint8_t i=0; i<HOTT_NUM_RF_CHANNELS; i++)
		hopping_frequency[i] = order[i%5]*15 + i/5;
	for(uint8_t k=0; k<15; k++)							// Shuffle in place: column b of the table holds the channels of the band at position b
		for(uint8_t b=0; b<5; b++)
		{
			j = k + HOTT_rand(&seed) % (15-k);
			tmp = hopping_frequency[5*k+b]; hopping_frequency[5*k+b] = hopping_frequency[5*j+b]; hopping_frequency[5*j+b] = tmp;
		}
}

const uint16_t PROGMEM HOTT_hop_val[] = { 0xC06B, 0xC34A, 0xDB24, 0x8E09, 0x272E, 0x217F, 0x155B, 0xEDE8, 0x1D31, 0x0986, 0x56F7, 0x6454, 0xC42D, 0x01D2, 0xC253, 0x1180 };

static void __attribute__((unused)) HOTT_TXID_init()
{
	packet[0] = pgm_read_word_near( &HOTT_hop_val[num_ch] );
	packet[1] = pgm_read_word_near( &HOTT_hop_val[num_ch] )>>8;

	HOTT_hop_init(pgm_read_word_near( &HOTT_hop_val[num_ch] ));
	#ifdef HOTT_FORCE_ID
		memcpy(rx_tx_addr,"\x7C\x94\x00\x0D\x50",5);	//TX1
		memcpy(rx_tx_addr,"\xEA\x4D\x00\x01\x50",5);	//TX2
	#endif
	memset(&packet[30],0xFF,9);
	packet[39]=0x07;									// unknown and constant
	if(IS_BIND_IN_PROGRESS)
	{
		memset(&packet[40],0xFA,5);
		memcpy(&packet[45],rx_tx_addr,5);
	}
	else
	{
		memcpy(&packet[40],rx_tx_addr,5);
		uint16_t addr=HOTT_EEPROM_OFFSET+RX_num*5;
		debug("RXID: ");
		for(uint8_t i=0;i<5;i++)
		{
			packet[45+i]=eeprom_read_byte((EE_ADDR)(addr+i));
			debug(" %02X",packet[45+i]);
		}
		debugln("");
	}
}

static void __attribute__((unused)) HOTT_prep_data_packet() {
	static uint8_t upper = 0;								// toggles between sending channels 1..8,9..12 and 1..8,13..16
	
	packet[2] = hopping_frequency_no;						// send next frequency to be used
	packet[3] = upper;										// indicate upper or lower channels (only supporting 16 channels)										
	
	#ifdef FAILSAFE_ENABLE
		static uint8_t failsafe_count = 0;					// failsafe packet state machine (need to send two packets)

		if(IS_FAILSAFE_VALUES_on && IS_BIND_DONE) {			// if TX wants to send failsafe data and RX is connected
			failsafe_count++;								// prepare to send next packet

			if(failsafe_count >= 3) {						// done sending two failsafe channel data packets
				FAILSAFE_VALUES_off;					
				failsafe_count = 0;
			}
		}
		else
			failsafe_count = 0;
	#endif

	// Channels value are PPM*2, -100%=1100µs, +100%=1900µs, order TAER
	//
	// Note: failsafe packets are differnt to normal operation packets
	// normal operation toggles between sending channels 1..8,9..12 and 1..8,13..16 using bit0 as high/low indicator in packet[3]
	// while failsafe packets send channels 1..12, 13..24 (and probably 25..32) using bit0 in packet[3] as packet type indicator
	// packet[3] = 0x40 -> failsafe packet with data for channels 1..12
	// packet[3] = 0x41 -> failsafe packet with data for channels 13..24
	//
	uint16_t val;
	for(uint8_t i = 0 ; i < 12*2 ; i += 2) {				// working 12 channels (using 2 bytes per channel)
		uint8_t chIndex = i >> 1 ;							// normal operation channel number
		uint8_t fschIndex = chIndex;						// linear channel number for failsafe
		
		if(upper && chIndex >= 8) chIndex += 4;				// for normal operation toggle between channels 1..8,9..12 and 1..8,13..16
		val = Channel_data[chIndex];						// get normal operation channel data
		val = (((val << 2) + val) >> 2)+ 860*2;				// convert channel data 0..2047 to 1720..4278 <-> -125%<->+125%
															// val = (val*5/4+860*2)

		#ifdef FAILSAFE_ENABLE
			if(failsafe_count == 1 || failsafe_count == 2) {// failsafe data needs to be sent to RX
				uint16_t fs = 0x8000;						// default failsafe mode is hold

				if(failsafe_count == 1) {					// send fail safe packet containing channels 1..12
					packet[3] = 0x40;						// indicate packet has failsafe values for channels 1..12
					fs = Failsafe_data[fschIndex];			// get failsafe channel data
				} else {									// send fail safe packet containing channels 13..24
					packet[3] = 0x41;						// indicate packet has failsafe values for channels 13..24
					if(fschIndex < 4)						// we only work 16 channels so send channels 13..16, rest default
						fs = Failsafe_data[fschIndex+12];	// get failsafe channel data 13..16
				  } 

				if( fs == FAILSAFE_CHANNEL_HOLD ||			// treat HOLD and NOPULSES as channel hold 
					fs == FAILSAFE_CHANNEL_NOPULSES)
					val = 0x8000;							// set channel failsafe mode hold flag
				else {
					val = (((fs << 2) + fs) >> 2) +860*2; 	// convert channel data 0..2047 to 1720..4278 <-> -125%<->+125%
					val |= 0x4000;							// set channel failsafe mode position flag					
				}
			} 
		#endif

		packet[i + 4]   = val;								// first channel data at packet[4] and packet[5]
		packet[i + 4+1] = val >> 8;
	}

	upper ^= 0x01;											// toggle to upper and lower channels

	packet[28] = 0x80;										// no sensor
	packet[29] = 0x02;										// 0x02 when bind starts then when RX replies cycle in sequence 0x1A/22/2A/0A/12, 0x02 during normal packets, 0x01->text config menu, 0x0A->no more RX telemetry

	#ifdef HOTT_FW_TELEMETRY
		if(IS_BIND_DONE)
		{
			static uint8_t prev_SerialRX_val=0;
			if(HoTT_SerialRX)
			{//Text mode
				uint8_t sensor=HoTT_SerialRX_val&0xF0;
				if((sensor&0x80) && sensor!=0xF0 && (HoTT_SerialRX_val&0x0F) >= 0x07)
				{//Valid Text query
					if(sensor==0x80) HoTT_SerialRX_val&=0x0F;	// RX only
					if(prev_SerialRX_val!=HoTT_SerialRX_val)
					{
						prev_SerialRX_val=HoTT_SerialRX_val;
						packet[28] = HoTT_SerialRX_val;			// send the button being pressed only once
					}
					else
						packet[28] = HoTT_SerialRX_val | 0x0F;	// no button pressed
				}
				else
					packet[28] = 0x0F;							// RX, no button pressed
				if(sub_protocol == HOTT_SYNC)
					packet[29] = ((HOTT_sensor_seq+1)<<3) | 1;	// Telemetry packet sequence
				else
					packet[29] = 0x01;							// 0x01->Text config menu
			}
			else
			{
				packet[28] = 0x89+HOTT_sensor_cur;				// 0x89/8A/8B/8C/8D/8E during normal packets
				if(sub_protocol == HOTT_SYNC)
					packet[29] = ((HOTT_sensor_seq+1)<<3) | 2;	// Telemetry packet sequence
			}
			//debugln("28=%02X,29=%02X",packet[28],packet[29]);
		}
	#endif

	CC2500_WriteReg(CC2500_06_PKTLEN, HOTT_TX_PACKET_LEN);
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, packet, HOTT_TX_PACKET_LEN);
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

uint16_t HOTT_callback()
{
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
				CC2500_SetTxRxMode(RX_EN);
				phase = HOTT_DATA1;
			}
			return 2000;

		/* Work cycle: 10ms */
		case HOTT_DATA1:
			//Set RF freq, setup LBT and prep packet
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(HOTT_PACKET_PERIOD);
			#endif
			//Clear all
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SNOP);
			CC2500_Strobe(CC2500_SFTX);
			CC2500_Strobe(CC2500_SFRX);
			CC2500_WriteReg(CC2500_04_SYNC1, 0xD3);
			CC2500_WriteReg(CC2500_05_SYNC0, 0x91);
			//Set RF freq
			HOTT_tune_freq();
			HOTT_tune_chan_fast();
			//Setup LBT
			CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xFF);
			CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x0C);
			CC2500_Strobe(CC2500_SRX);
			//Prep packet
			HOTT_prep_data_packet();
			//Listen
			CC2500_WriteReg(CC2500_17_MCSM1, 0x10);		//??
			CC2500_WriteReg(CC2500_18_MCSM0, 0x18);		//??
			CC2500_Strobe(CC2500_SRX);					//??
			phase++;		//HOTT_DATA2
			return 1095;
		case HOTT_DATA2:
			//LBT
			if((CC2500_ReadReg(CC2500_38_PKTSTATUS | CC2500_READ_BURST)&0x10)==0)
			{ //Channel is busy
				LBT_POWER_on;										// Reduce to low power before transmitting
				debugln("Busy %d",rf_ch_num);
			}
			CC2500_WriteReg(CC2500_17_MCSM1, 0x00);		//??
			CC2500_WriteReg(CC2500_18_MCSM0, 0x08);		//??
			CC2500_SetPower();
			//Send packet
			CC2500_SetTxRxMode(TX_EN);
			CC2500_Strobe(CC2500_STX);
			phase++;		//HOTT_RX1
			return 3880;
		case HOTT_RX1:
			//Clear all
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFTX);
			CC2500_Strobe(CC2500_SFRX);
			//RX
			if(packet[29] & 0xF8)
			{// Sync telemetry
				CC2500_WriteReg(CC2500_04_SYNC1, 0x2C);
				CC2500_WriteReg(CC2500_05_SYNC0, 0x6E);
			}
			CC2500_SetTxRxMode(RX_EN);
			CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);
			CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x09);
			CC2500_WriteReg(CC2500_06_PKTLEN, HOTT_RX_PACKET_LEN);
			CC2500_Strobe(CC2500_SRX);
			phase++;		//HOTT_RX2
			return 4025;
		case HOTT_RX2:
			//Telemetry
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
			if (len==HOTT_RX_PACKET_LEN+2)
			{
				CC2500_ReadData(packet_in, len);
				if((packet_in[HOTT_RX_PACKET_LEN+1]&0x80) && memcmp(rx_tx_addr,packet_in,5)==0)
				{ // CRC OK and TX ID matches
					if(IS_BIND_IN_PROGRESS)
					{
						//GR-16:  D4 20 F2 E6 F6 31 BD 01 00 90 00 FF 03 00 9E 1B 00 00 00 00 00 00
						//GR-12L: D4 20 F2 E6 F6 6E EE 01 00 B1 00 FF 03 00 0E 08 10 00 02 00 00 00
						//Vector: D4 20 F2 E6 F6 00 00 3A 01 A1 00 00 1A 24 35 1A 00 24 00 00 00 1A
						//        -----TXID----- -----RXID----- ---------------Unknown-------------
						debug("B:");
						for(uint8_t i=0;i<HOTT_RX_PACKET_LEN;i++)
							debug(" %02X", packet_in[i]);
						debugln("");
						uint16_t addr=HOTT_EEPROM_OFFSET+RX_num*5;
						for(uint8_t i=0; i<5; i++)
							eeprom_write_byte((EE_ADDR)(addr+i),packet_in[5+i]);
						BIND_DONE;
						HOTT_TXID_init();
					}
					#ifdef HOTT_FW_TELEMETRY
						else
						{	//Telemetry
							// [0..4] = TXID
							// [5..9] = RXID
							// [10] = holds warnings issued by hott devices
							// [11] = telmetry pages. For sensors 0x00 to 0x04, for config mennu 0x00 to 0x12.
							// Normal telem page 0 = 0x55, 0x32, 0x38, 0x55, 0x64, 0x32, 0xD0, 0x07, 0x00, 0x55
							//   Page 0 [12] = [21] = [15]
							//   Page 0 [13] = RX_Voltage Cur*10 in V
							//   Page 0 [14] = Temperature-20 in °C
							//   Page 0 [15] = RX_RSSI CC2500 formated (a<128:a/2-71dBm, a>=128:(a-256)/2-71dBm)
							//   Page 0 [16] = RX_LQI in %
							//   Page 0 [17] = RX_Voltage Min*10 in V
							//   Page 0 [18,19] = [19]<<8+[18]=max lost packet time in ms, max value seems 2s=0x7D0
							//   Page 0 [20] = rx events
							//
							// Config menu consists of the different telem pages put all together
							//   Page X [12] = seems like all the telem pages with the same value are going together to make the full config menu text. Seen so far 'a', 'b', 'c', 'd' 
							//   Page X [13..21] = 9 ascii chars to be displayed, char is highlighted when ascii|0x80
							//   Screen display is 21 characters large which means that once the first 21 chars are filled go to the begining of the next line
							//   Menu commands are sent through TX packets:
							//     packet[28]= 0xXF=>no key press, 0xXD=>down, 0xXB=>up, 0xX9=>enter, 0xXE=>right, 0xX7=>left with X=0 or D
							//     packet[29]= 0xX1/0xX9 with X=0 or X counting 0,1,1,2,2,..,9,9
							// Reduce telemetry to 14 bytes
							packet_in[0]= packet_in[HOTT_RX_PACKET_LEN];
							packet_in[1]= TX_LQI;
							uint8_t hott_warnings = packet_in[10];						// save warnings from hott devices
							bool send_telem=true;
							HOTT_sensor_seq++;											// Increment RX sequence counter
							if(packet[29] & 1)
							{ //Text mode
								HOTT_sensor_seq %= 19;									// 19 pages in Text mode
								HOTT_sensor_pages = 0;
								HOTT_sensor_valid = false;
								packet_in[10] = 0x80;									// Marking telem Text mode
								packet_in[12] = 0;
								for(uint8_t i=0; i<HOTT_SENSOR_TYPE;i++)
									packet_in[12] |= HOTT_sensor_ok[i]<<i;				// Send detected sensors
							}
							else
							{ //Binary sensor
								HOTT_sensor_seq %= 5;									// 5 pages in binary mode per sensor
								if(state==0 && HOTT_sensor_ok[0]==false && HOTT_sensor_ok[1]==false && HOTT_sensor_ok[2]==false && HOTT_sensor_ok[3]==false && HOTT_sensor_ok[4]==false && HOTT_sensor_ok[5]==false)
									HOTT_sensor_seq=0;									// No sensors always ask page 0
								if(state)
									state--;
								if( packet_in[11]==1 )									// Page 1
								{
									if( packet_in[12] == (HOTT_sensor_cur+9)<<4 )
									{ //Requested sensor is sending: 0x90/A0/B0/C0/D0/E0
										HOTT_sensor_pages = 0;							// Sensor first page received
										HOTT_sensor_valid = true;						// Data from the expected sensor is being received
										HOTT_sensor_ok[(packet_in[12]>>4)-9]=true;					
									}
									else
									{
										HOTT_sensor_valid = false;
										HOTT_sensor_pages = 0x1E;						// Switch to next sensor
									}
								}
								if(packet_in[11])
								{ //Page != 0
									if(HOTT_sensor_valid)								// Valid
									{
										packet_in[10] = HOTT_sensor_cur+9;				// Mark telem with sensor ID
										HOTT_sensor_pages |= 1<<packet_in[11];			// Page received
									}
									else
										send_telem=false;								// Do not send
								}
								else
									packet_in[10]=0;									// Mark telem with sensor 0=RX
							}
							debug("T%d=",send_telem);
							for(uint8_t i=10;i < HOTT_RX_PACKET_LEN; i++)
							{
								packet_in[i-8]=packet_in[i];
								debug(" %02X",packet_in[i]);
							}
							packet_in[14] = hott_warnings;								// restore saved warnings from hott devices
							debugln("");
							if(send_telem)
								telemetry_link=2;
							if((HOTT_sensor_pages&0x1E) == 0x1E)						// All 4 pages received from the sensor
							{ //Next sensor
								uint8_t loop=0;
								do
								{
									HOTT_sensor_cur++;									// Switch to next sensor
									HOTT_sensor_cur %= HOTT_SENSOR_TYPE;
									loop++;
								}
								while(HOTT_sensor_ok[HOTT_sensor_cur]==false && loop<HOTT_SENSOR_TYPE+1 && state==0);
								HOTT_sensor_valid=false;
								HOTT_sensor_pages=0;
								HOTT_sensor_seq=0;
								debugln("Sensor:%02X",HOTT_sensor_cur+9);
							}										
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
					if(pps_counter==0)
					{ // lost connection with RX, power cycle? research sensors again.
						HOTT_sensor_cur=3;
						HOTT_sensor_seq=0;
						HOTT_sensor_valid=false;
						for(uint8_t i=0; i<HOTT_SENSOR_TYPE;i++)
							HOTT_sensor_ok[i]=false;	// no sensors detected
						state=HOTT_SENSOR_SEARCH_PERIOD;
					}
					pps_counter=packet_count=0;
				}
			#endif
			phase=HOTT_DATA1;
			return 1000;
	}
	return 0;
}

void HOTT_init()
{
	num_ch=random(0xfefefefe)%16;
	HOTT_TXID_init();
	HOTT_rf_init();
	#ifdef HOTT_FW_TELEMETRY
		HoTT_SerialRX_val=0;
		HoTT_SerialRX=false;
		HOTT_sensor_cur=3;
		HOTT_sensor_pages=0;
		HOTT_sensor_valid=false;
		HOTT_sensor_seq=0;
		for(uint8_t i=0; i<HOTT_SENSOR_TYPE;i++)
			HOTT_sensor_ok[i]=false;	// no sensors detected
		packet_count=0;
		state=HOTT_SENSOR_SEARCH_PERIOD;
	#endif
	phase = HOTT_START;
}

#endif