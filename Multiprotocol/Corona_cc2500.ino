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

#if defined(CORONA_CC2500_INO)

#include "iface_cc2500.h"

//#define CORONA_FORCE_ID

#define CORONA_RF_NUM_CHANNELS	3
#define CORONA_ADDRESS_LENGTH	4
#define CORONA_BIND_CHANNEL_V1	0xD1
#define CORONA_BIND_CHANNEL_V2	0xB8
#define CORONA_COARSE			0x00
#define CORONA_CHANNEL_TIMING	1500

const PROGMEM uint8_t CORONA_init_values[] = {
  /* 00 */ 0x29, 0x2E, 0x06, 0x07, 0xD3, 0x91, 0xFF, 0x04,
  /* 08 */ 0x05, 0x00, CORONA_BIND_CHANNEL_V1, 0x06, 0x00, 0x5C, 0x4E, 0xC4 + CORONA_COARSE,
  /* 10 */ 0x5B, 0xF8, 0x03, 0x23, 0xF8, 0x47, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11, 0x41,
  /* 28 */ 0x00, 0x59, 0x7F, 0x3F, 0x81, 0x35, 0x0B
};

static void __attribute__((unused)) CORONA_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i <= 0x2E; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&CORONA_init_values[i]));
	if(sub_protocol!=COR_V1)
	{
		CC2500_WriteReg(CC2500_0A_CHANNR, CORONA_BIND_CHANNEL_V2);
		CC2500_WriteReg(CC2500_0E_FREQ1, 0x80);
		CC2500_WriteReg(CC2500_0F_FREQ0, 0x00 + CORONA_COARSE);
		CC2500_WriteReg(CC2500_15_DEVIATN, 0x50);
		CC2500_WriteReg(CC2500_17_MCSM1, 0x00);
	    CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x67);
		CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0xFB);
		CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xDC);
	}
	
	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);

	//not sure what they are doing to the PATABLE since basically only the first byte is used and it's only 8 bytes long. So I think they end up filling the PATABLE fully with 0xFF
	CC2500_WriteRegisterMulti(CC2500_3E_PATABLE,(const uint8_t *)"\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 13);

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

// Generate id and hopping freq
static void __attribute__((unused)) CORONA_init()
{
	#ifdef CORONA_FORCE_ID
		// Example of ID and channels taken from dumps
		if(sub_protocol==COR_V1)
		{
			memcpy((void *)rx_tx_addr,(void *)"\x1F\xFE\x6C\x35",CORONA_ADDRESS_LENGTH);
			memcpy((void *)hopping_frequency,(void *)"\x17\x0D\x03\x49",CORONA_RF_NUM_CHANNELS+1);
		}
		else
		{
			memcpy((void *)rx_tx_addr,(void *)"\xFE\xFE\x02\xFB",CORONA_ADDRESS_LENGTH);
			memcpy((void *)hopping_frequency,(void *)"\x14\x3D\x35",CORONA_RF_NUM_CHANNELS);
		}
	#else
		// From dumps channels are anything between 0x00 and 0xC5 on V1.
		// But 0x00 and 0xB8 should be avoided on V2 since they are used for bind.
		// Below code make sure channels are between 0x02 and 0xA0, spaced with a minimum of 2 and not ordered (RX only use the 1st channel unless there is an issue).
		uint8_t order=rx_tx_addr[3]&0x03;
		for(uint8_t i=0; i<CORONA_RF_NUM_CHANNELS+1; i++)
			hopping_frequency[i^order]=2+rx_tx_addr[3-i]%39+(i<<5)+(i<<3);

		// ID looks random but on the 15 V1 dumps they all show the same odd/even rule
		if(rx_tx_addr[3]&0x01)
		{	// If [3] is odd then [0] is odd and [2] is even 
			rx_tx_addr[0]|=0x01;
			rx_tx_addr[2]&=0xFE;
		}
		else
		{	// If [3] is even then [0] is even and [2] is odd 
			rx_tx_addr[0]&=0xFE;
			rx_tx_addr[2]|=0x01;
		}
		rx_tx_addr[1]=0xFE;			// Always FE in the dumps of V1 and V2
	#endif
}

// 8 Channels with direct values from PPM
static void __attribute__((unused)) CORONA_build_packet()
{
	// Tune frequency if it has been changed
	if ( prev_option != option )
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		prev_option = option ;
	}
	if(IS_BIND_DONE)
	{	
		if(state==0 || sub_protocol==COR_V1)
		{	// Build standard packet

			// Set RF channel
			CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
			// Update RF power
			CC2500_SetPower();

			// Build packet
			packet[0] = 0x10;		// 17 bytes to follow
			
			// Channels
			memset(packet+9, 0x00, 4);
			for(uint8_t i=0; i<8; i++)
			{	// Channel values are packed
				uint16_t val=convert_channel_ppm(i);
				packet[i+1] = val;
				packet[9 + (i>>1)] |= (i&0x01)?(val>>4)&0xF0:(val>>8)&0x0F;
			}

			// TX ID
			for(uint8_t i=0; i<CORONA_ADDRESS_LENGTH; i++)
				packet[i+13] = rx_tx_addr[i];
			
			packet[17] = 0x00;

			// Packet period is based on hopping
			switch(hopping_frequency_no)
			{
				case 0:
					packet_period=sub_protocol==COR_V1?4991-CORONA_CHANNEL_TIMING:4248-CORONA_CHANNEL_TIMING;
					break;
				case 1:
					packet_period=sub_protocol==COR_V1?4991-CORONA_CHANNEL_TIMING:4345-CORONA_CHANNEL_TIMING;
					break;
				case 2:
					packet_period=sub_protocol==COR_V1?12520-CORONA_CHANNEL_TIMING:13468-CORONA_CHANNEL_TIMING;
					if(sub_protocol!=COR_V1)
						packet[17] = 0x03;
					break;
			}
			hopping_frequency_no++;
			hopping_frequency_no%=CORONA_RF_NUM_CHANNELS;
		}
		else
		{	// Send identifier packet for 2.65sec. This is how the RX learns the hopping table after a bind. Why it's not part of the bind like V1 is a mistery...
			// Set channel
			CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
			state--;
			packet[0]=0x07;		// 8 bytes to follow
			// Send hopping freq
			for(uint8_t i=0; i<CORONA_RF_NUM_CHANNELS; i++)
				packet[i+1]=hopping_frequency[i];
			// Send TX ID
			for(uint8_t i=0; i<CORONA_ADDRESS_LENGTH; i++)
				packet[i+4]=rx_tx_addr[i];
			packet[8]=0;
			packet_period=6647-CORONA_CHANNEL_TIMING;
		}
	}
	else
	{	// Build bind packets
		if(sub_protocol==COR_V1)
		{	// V1
			if(bind_counter&1)
			{ // Send TX ID
				packet[0]=0x04;		// 5 bytes to follow
				for(uint8_t i=0; i<CORONA_ADDRESS_LENGTH; i++)
					packet[i+1]=rx_tx_addr[i];
				packet[5]=0xCD;		// Unknown but seems to be always the same value for V1
				packet_period=3689;
			}
			else
			{ // Send hopping freq
				packet[0]=0x03;		// 4 bytes to follow
				for(uint8_t i=0; i<CORONA_RF_NUM_CHANNELS+1; i++)
					packet[i+1]=hopping_frequency[i];
				// Not sure what the last byte (+1) is for now since only the first 3 channels are used...
				packet_period=3438;
			}
		}
		else
		{	// V2
			packet[0]=0x04;		// 5 bytes to follow
			for(uint8_t i=0; i<CORONA_ADDRESS_LENGTH; i++)
				packet[i+1]=rx_tx_addr[i];
			packet[5]=0x00;		// Unknown but seems to be always the same value for V2
			packet_period=26791;
		}
	}
}

uint16_t ReadCORONA()
{
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter--;
		if (bind_counter == 0)
			BIND_DONE;
	}
	if(packet[0]==0)
	{
		CORONA_build_packet();
		if(IS_BIND_DONE)
			return CORONA_CHANNEL_TIMING;
	}
	// Send packet
	CC2500_WriteData(packet, packet[0]+2);
	packet[0]=0;
	return packet_period;
}

uint16_t initCORONA()
{
	if(sub_protocol==COR_V1)
		bind_counter=1400;		// Stay in bind mode for 5s
	else
		bind_counter=187;		// Stay in bind mode for 5s
	state=400;					// Used by V2 to send RF channels + ID for 2.65s at startup
	hopping_frequency_no=0;
	packet[0]=0;
	CORONA_init();
	CORONA_rf_init();
	return 10000;
}

#endif