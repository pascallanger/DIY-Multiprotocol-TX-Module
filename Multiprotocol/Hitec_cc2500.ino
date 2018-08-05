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

#if defined(HITEC_CC2500_INO)

#include "iface_cc2500.h"

//#define HITEC_FORCE_ID	//Use the ID and hopping table from the original dump

#define HITEC_COARSE			0

#define HITEC_PACKET_LEN		13
#define HITEC_TX_ID_LEN			2
#define HITEC_BIND_COUNT		444	// 10sec
#define HITEC_NUM_FREQUENCE		21
#define HITEC_BIND_NUM_FREQUENCE 14

enum {
    HITEC_START = 0x00,
    HITEC_CALIB = 0x01,
    HITEC_PREP  = 0x02,
    HITEC_DATA1 = 0x03,
    HITEC_DATA2 = 0x04,
    HITEC_DATA3 = 0x05,
    HITEC_DATA4	= 0x06,
    HITEC_RX1	= 0x07,
    HITEC_RX2	= 0x08,
};

#define HITEC_FREQ0_VAL 0xE8

const PROGMEM uint8_t HITEC_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, 0xFF, 0x04,
  /* 08 */ 0x45, 0x00, 0x00, 0x12, 0x00, 0x5C, 0x85, HITEC_FREQ0_VAL + HITEC_COARSE,
  /* 10 */ 0x3D, 0x3B, 0x73, 0x73, 0x7A, 0x01, 0x07, 0x30,
  /* 18 */ 0x08, 0x1D, 0x1C, 0xC7, 0x00, 0xB0, 0x87, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x00, 0x11
};

static void __attribute__((unused)) HITEC_CC2500_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&HITEC_init_values[i]));

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

// Generate RF channels
static void __attribute__((unused)) HITEC_RF_channels()
{
	//Normal hopping
	uint8_t idx = 0;
	uint32_t rnd = MProtocol_id;

	while (idx < HITEC_NUM_FREQUENCE)
	{
		uint8_t i;
		uint8_t count_0_47 = 0, count_48_93 = 0, count_94_140 = 0;

		rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization
		// Use least-significant byte and make sure it's pair.
		uint8_t next_ch = ((rnd >> 8) % 141) & 0xFE;
		// Check that it's not duplicated and spread uniformly
		for (i = 0; i < idx; i++) {
			if(hopping_frequency[i] == next_ch)
				break;
			if(hopping_frequency[i] <= 47)
				count_0_47++;
			else if (hopping_frequency[i] <= 93)
				count_48_93++;
			else
				count_94_140++;
		}
		if (i != idx)
			continue;
		if ( (next_ch <= 47 && count_0_47 < 8) || (next_ch >= 48 && next_ch <= 93 && count_48_93 < 8) || (next_ch >= 94 && count_94_140 < 8) )
			hopping_frequency[idx++] = next_ch;//find hopping frequency
	}
}

static void __attribute__((unused)) HITEC_tune_chan()
{
	CC2500_Strobe(CC2500_SIDLE);
	if(IS_BIND_IN_PROGRESS)
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency_no*10);
	else
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
	CC2500_Strobe(CC2500_SFTX);
	CC2500_Strobe(CC2500_SCAL);
	CC2500_Strobe(CC2500_STX);
}

static void __attribute__((unused)) HITEC_change_chan_fast()
{
	CC2500_Strobe(CC2500_SIDLE);
	if(IS_BIND_IN_PROGRESS)
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency_no*10);
	else
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[hopping_frequency_no]);
}

static void __attribute__((unused)) HITEC_build_packet()
{
	static boolean F5_packet=false;
	uint8_t offset;
	
	packet[1] = 0x00;			// unknown always 0x00 in the dumps. TODO: test if part of ID.
	packet[2] = rx_tx_addr[3];
	packet[3] = rx_tx_addr[2];
	packet[22] = 0xEE;			// unknown always 0xEE
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0x16;		// 22 bytes to follow
		memset(packet+5,0x00,14);
		switch(bind_phase)
		{
			case 0x72:			// first part of the hopping table
				for(uint8_t i=0;i<14;i++)
					packet[5+i]=hopping_frequency[i]>>1;
				break;
			case 0x73:			// second part of the hopping table
				for(uint8_t i=0;i<7;i++)
					packet[5+i]=hopping_frequency[i+14]>>1;
				break;
			case 0x74:
				packet[7]=0x55;	// unknown but bind does not complete if not there
				packet[8]=0x55;	// unknown but bind does not complete if not there
				break;
			case 0x7B:
				packet[5]=hopping_frequency[13]>>1;	// if not there the Optima link is jerky...
				break;
		}
		if(sub_protocol==OPTIMA)
			packet[4] = bind_phase;	// increments based on RX answer
		else
		{
			packet[4] = bind_phase+0x10;
			bind_phase^=0x01;	// switch between 0x82=first part of the hopping table and 0x83=second part
		}
		packet[19] = 0x08;		// packet number
		offset=20;				// packet[20] and [21]
	}
	else
	{
		packet[0] = 0x1A;		// 26 bytes to follow
		for(uint8_t i=0;i<9;i++)
		{
			uint16_t ch = convert_channel_16b_nolimit(i,0x1B87,0x3905);
			packet[4+2*i] = ch >> 8;
			packet[5+2*i] = ch & 0xFF;
		}
		packet[23] = 0x80;		// packet number
		offset=24;				// packet[24] and [25]
		packet[26] = 0x00;		// unknown always 0 and the RX doesn't seem to care about the value?
	}

	//packet[offset] starts with 0x00 and after some time alternate between 0x00 and 0xF5
	//packet[offset+1] equals to 0x00 if [offset]=0x00 but if [offset]=0xF5 then the values are between 0xDB and 0xE0
	//TODO: test if it could be RSSI, time, ... related?
	if(F5_packet)
	{
		packet[offset] = 0xF5;		
		packet[offset+1] = 0xE0;	//value in the dumps between 0xDB and 0xE0
		F5_packet=false;
	}
	else
	{
		packet[offset] = 0x00;
		packet[offset+1] = 0x00;
		F5_packet=true;
	}
/*	debug("P:");
	for(uint8_t i=0;i<packet[0]+1;i++)
		debug("%02X,",packet[i]);
	debugln("");
*/
}

static void __attribute__((unused)) HITEC_send_packet()
{
	CC2500_WriteData(packet, packet[0]+1);
	if(IS_BIND_IN_PROGRESS)
		packet[19] >>= 1;	// packet number
	else
		packet[23] >>= 1;	// packet number
}

uint16_t ReadHITEC()
{
	switch(phase)
	{
		case HITEC_START:
			HITEC_CC2500_init();
			bind_phase=0x72;
			if(IS_BIND_IN_PROGRESS)
			{
				bind_counter = HISKY_BIND_COUNT;
				rf_ch_num=HITEC_BIND_NUM_FREQUENCE;
			}
			else
			{
				bind_counter=0;
				rf_ch_num=HITEC_NUM_FREQUENCE;
				//Set TXID
				CC2500_WriteReg(CC2500_04_SYNC1,rx_tx_addr[2]);
				CC2500_WriteReg(CC2500_05_SYNC0,rx_tx_addr[3]);
			}
			hopping_frequency_no=0;
			HITEC_tune_chan();
			phase = HITEC_CALIB;
			return 2000;
		case HITEC_CALIB:
			calData[hopping_frequency_no]=CC2500_ReadReg(CC2500_25_FSCAL1);
			hopping_frequency_no++;
			if (hopping_frequency_no < rf_ch_num)
				HITEC_tune_chan();
			else
			{
				hopping_frequency_no = 0;
				phase = HITEC_PREP;
			}
			return 2000;

		/* Work cycle: 22.5ms */
#define HITEC_PACKET_PERIOD	22500
#define HITEC_PREP_TIMING	462
#define HITEC_DATA_TIMING	2736
#define HITEC_RX1_TIMING	4636
		case HITEC_PREP:
			if ( prev_option == option )
			{	// No user frequency change
				HITEC_change_chan_fast();
				hopping_frequency_no++;
				if(hopping_frequency_no>=rf_ch_num)
					hopping_frequency_no=0;
				CC2500_SetPower();
				CC2500_SetTxRxMode(TX_EN);
				HITEC_build_packet();
				phase++;
			}
			else
				phase = HITEC_START;	// Restart the tune process if option is changed to get good tuned values
			return HITEC_PREP_TIMING;
		case HITEC_DATA1:
		case HITEC_DATA2:
		case HITEC_DATA3:
		case HITEC_DATA4:
			HITEC_send_packet();
			phase++;
			return HITEC_DATA_TIMING;
		case HITEC_RX1:
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SRX);	// Turn RX ON
			phase++;
			return HITEC_RX1_TIMING;
		case HITEC_RX2:
			uint8_t len=CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
			if(len && len<MAX_PKT)
			{ // Something has been received
				CC2500_ReadData(pkt, len);
				if( (pkt[len-1] & 0x80) && pkt[0]==len-3 && packet[2]==rx_tx_addr[3] && packet[3]==rx_tx_addr[2])
				{ //valid crc && length ok && tx_id ok
					debug("RX:l=%d",len);
					for(uint8_t i=0;i<len;i++)
						debug(",%02X",pkt[i]);
					if(IS_BIND_IN_PROGRESS)
					{
						if(len==13)	// Bind packets have a length of 13
						{ // bind packet: 0A,00,E5,F2,7X,05,06,07,08,09,00
							debug(",bind");
							boolean check=true;
							for(uint8_t i=5;i<=10;i++)
								if(pkt[i]!=i%10) check=false;
							if((pkt[4]&0xF0)==0x70 && check)
							{
								bind_phase=pkt[4]+1;
								if(bind_phase==0x7B)
									bind_counter=164;	// in dumps the RX stops to reply at 0x7B so wait a little and exit
							}
						}
					}
					else
						if(len==15)	// Telemetry packets have a length of 15
						{ // telem packet: 0C, 00, 6A, 03, 00, 00, 00, 00, 00, 00, 00, 8E, 00, 5E, 94 
						//Not fully handled since the RX I have only send 1 frame where from what I've been reading on Hitec telemetry should have at least 4 frames...
							debug(",telem");
							#if defined(HITEC_HUB_TELEMETRY)
								TX_RSSI = pkt[len-2];
								if(TX_RSSI >=128)
									TX_RSSI -= 128;
								else
									TX_RSSI += 128;
								TX_LQI = pkt[len-1]&0x7F;
								//TODO: where is the frame number...
								v_lipo1 = (pkt[len-3])<<5 | (pkt[len-4])>>3;	// calculation in decimal is volt=(pkt[len-3]<<8+pkt[len-4])/28
								telemetry_link=1;			// telemetry hub available
							#elif defined(HITEC_FW_TELEMETRY)
								for(uint8_t i=4;i < len; i++)
									pkt[i-4]=pkt[i];		// remove header, 11 bytes of data
								telemetry_link=2;			// telemetry forward available
							#endif
						}
					debugln("");
				}
			}
			CC2500_Strobe(CC2500_SRX);	// Flush the RX FIFO buffer
			phase = HITEC_PREP;
			if(bind_counter)
			{
				bind_counter--;
				if(!bind_counter)
				{
					BIND_DONE;
					phase=HITEC_START;
				}
			}
			return (HITEC_PACKET_PERIOD -HITEC_PREP_TIMING -4*HITEC_DATA_TIMING -HITEC_RX1_TIMING);
	}
	return 0;
}

uint16_t initHITEC()
{
	HITEC_RF_channels();
	#ifdef HITEC_FORCE_ID	// ID and channels taken from dump
		rx_tx_addr[3]=0x6A;
		rx_tx_addr[2]=0x03;
		memcpy((void *)hopping_frequency,(void *)"\x00\x3A\x4A\x32\x0C\x58\x2A\x10\x26\x20\x08\x60\x68\x70\x78\x80\x88\x56\x5E\x66\x6E",HITEC_NUM_FREQUENCE);
	#endif
	phase = HITEC_START;
	return 10000;
}

#endif