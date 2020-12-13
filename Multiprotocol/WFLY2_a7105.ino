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

#if defined(WFLY2_A7105_INO)

#include "iface_a7105.h"

//#define WFLY2_FORCE_ID

//WFLY2 constants & variables
#define WFLY2_BIND_COUNT		1000
#define WFLY2_PACKET_SIZE		32

enum{
	WFLY2_BIND,
	WFLY2_DATA,
	WFLY2_PLL_TX,
	WFLY2_RX,
};

static void __attribute__((unused)) WFLY2_send_bind_packet()
{
	//Header
	packet[0] = 0x0F;			// Bind packet

	//ID
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = rx_tx_addr[1];

	//Unknown
	packet[4] = 0x00;
	packet[5] = 0x01;

	//Freq
	packet[6] = (hopping_frequency_no<<1)+0x0E;
	rf_ch_num = (hopping_frequency_no<<2)+0x2C;
	hopping_frequency_no++;	// not sure which frequencies are used since the dump only goes from 0x0E to 0x2C and stops...
	if(hopping_frequency_no > 0x1A) hopping_frequency_no=0x00;

	//Unknown
	memset(&packet[7],0x00,25);

	//Debug
	#if 0
		debug("ch=%02X P=",rf_ch_num);
		for(uint8_t i=0; i<WFLY2_PACKET_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif

	//Send
	A7105_WriteData(WFLY2_PACKET_SIZE, rf_ch_num);
}

static void __attribute__((unused)) WFLY2_build_packet()
{
	static uint16_t pseudo=0;

	//Header
	packet[0] = 0x00;	// Normal packet

	//Pseudo
	uint16_t high_bit=(pseudo & 0x8000) ^ 0x8000; 							// toggle 0x8000 every other line
	pseudo <<= 1;															// *2
	if( (pseudo & 0x8000) || pseudo == 0 ) pseudo ^= 0x8A87;				// Randomisation, pseudo==0 is a guess but would give the start value seen on the dump when P[2]P[1]=0 at init and will prevent a lock up
	pseudo |= high_bit;														// include toggle
	packet[1] = pseudo;
	packet[2] = pseudo>>8;
	
	//RF channel
	int8_t prev = rf_ch_num & 0x1F;
	rf_ch_num = (pseudo ^ (pseudo >> 7)) & 0x57;
	if(rf_ch_num & 0x10)
	{
		rf_ch_num |= 0x08;
		rf_ch_num &= 0x4F;
	}
	if(rf_ch_num & 0x40)
	{
		rf_ch_num |= 0x10;
		rf_ch_num &= 0x1F;
	}
	rf_ch_num ^= rx_tx_addr[3] & 0x1F;
	if(abs((int8_t)rf_ch_num-prev) <= 9)
	{
		if(high_bit)
			rf_ch_num |= 0x20;
	}
	else
		if(!high_bit)
			rf_ch_num |= 0x20;

	//Partial ID
	packet[3] = rx_tx_addr[3];
	packet[4] = rx_tx_addr[2] & 0x03;

	//10 channels
	for(uint8_t i = 0; i < 5; i++)
	{
		uint16_t temp=convert_channel_16b_nolimit(i*2 , 0x0000, 0x0FFF);	// need to check channels min/max...
		packet[5 + i*3]  = temp&0xFF;		// low byte
		packet[7 + i*3]  = (temp>>8)&0x0F;	// high byte
		temp=convert_channel_16b_nolimit(i*2+1, 0x0000, 0x0FFF);			// need to check channels min/max...
		packet[6 + i*3]  = temp&0xFF;		// low byte
		packet[7 + i*3] |= (temp>>4)&0xF0;	// high byte
	}
	
	//Unknown
	memset(&packet[20],0x00,12);

	//Debug
	#if 0
		debug("ch=%02X,%02X P=",rf_ch_num,(rf_ch_num<<1)+0x10);
		for(uint8_t i=0; i<WFLY2_PACKET_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif
}

#ifdef WFLY2_HUB_TELEMETRY
	static void __attribute__((unused)) WFLY2_Send_Telemetry()
	{
		//Incoming packet values
		v_lipo1=packet[3]<<1;		// RX_batt*10 in V
		v_lipo2=packet[5]<<1;		// Ext_batt*10 in V
		RX_RSSI=(255-packet[7])>>1;	// Looks to be the RX RSSI value direct from A7105

		// Read TX RSSI
		TX_RSSI=255-A7105_ReadReg(A7105_1D_RSSI_THOLD);

		telemetry_counter++;			// LQI counter
		telemetry_link=1;
		if(telemetry_lost)
		{
			telemetry_lost = 0;
			packet_count = 100;
			telemetry_counter = 100;
		}
	}
#endif

#define WFLY2_PACKET_PERIOD	3600	//3600
#define WFLY2_BUFFER_TIME		1500	//1500
#define WFLY2_WRITE_TIME		800		//942

uint16_t ReadWFLY2()
{
	uint16_t start;
	#ifdef WFLY2_HUB_TELEMETRY
		uint8_t status;
	#endif
	
	#ifndef FORCE_WFLY2_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	switch(phase)
	{
		case WFLY2_BIND:
			bind_counter--;
			if (bind_counter == 0)
			{
				BIND_DONE;
				A7105_WriteID(MProtocol_id);
				rf_ch_num = 0;
				phase++;	// WFLY2_DATA
			}
			WFLY2_send_bind_packet();
			return WFLY2_PACKET_PERIOD;

		case WFLY2_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(WFLY2_PACKET_PERIOD);
			#endif
			//Build data packet
			WFLY2_build_packet();

			//Fill the TX buffer without sending
			A7105_WriteData(WFLY2_PACKET_SIZE,0);
			
			#ifdef WFLY2_HUB_TELEMETRY
				//LQI calculation
				packet_count++;
				if(packet_count>=100)
				{
					packet_count=0;
					TX_LQI=telemetry_counter;
					if(telemetry_counter==0)
						telemetry_lost = 1;
					telemetry_counter = 0;
				}
			#endif
			
			phase++;	// WFLY2_PLL_TX
			return WFLY2_BUFFER_TIME;
			
		case WFLY2_PLL_TX:
			#ifdef WFLY2_HUB_TELEMETRY
				//Check RX status
				status=A7105_ReadReg(A7105_00_MODE);
				//debugln("S:%02X", status);
			#endif
			
			//PLL
			A7105_Strobe(A7105_PLL);
			
			#ifdef WFLY2_HUB_TELEMETRY
				//Read incoming packet even if bad/not present to not change too much the TX timing, might want to reorg the code...
				A7105_ReadData(WFLY2_PACKET_SIZE);

				//Read telemetry
				if((status & 0x21)==0)
				{ // Packet received and CRC OK
					//Debug
					#if 1
						debug("T:");
						for(uint8_t i=0; i<WFLY2_PACKET_SIZE-20; i++)		// Can't send the full telemetry at full speed
							debug(" %02X", packet[i]);
						debugln("");
					#endif
					//Packet match the ID ?
					if(packet[0]==0 && packet[1]==rx_tx_addr[3] && packet[2]==(rx_tx_addr[2] & 0x03))
						WFLY2_Send_Telemetry();							// Packet looks good do send telem to the radio
				}
			#endif
			
			//Change RF channel
			A7105_WriteReg(A7105_0F_PLL_I, (rf_ch_num<<1)+0x10);
			
			//Switch to TX
			A7105_SetPower();
			A7105_SetTxRxMode(TX_EN);
			A7105_Strobe(A7105_TX);

			phase++;	// WFLY2_RX
			return WFLY2_WRITE_TIME;
			
		case WFLY2_RX:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)((uint16_t)micros()-start) < 700)				// Wait max 700µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			
			//Switch to RX
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			
			phase = WFLY2_DATA;
			return WFLY2_PACKET_PERIOD-WFLY2_WRITE_TIME-WFLY2_BUFFER_TIME;
	}
	return WFLY2_PACKET_PERIOD; // never reached, please the compiler
}

uint16_t initWFLY2()
{
	A7105_Init();

	#ifdef WFLY2_FORCE_ID
		MProtocol_id = 0x50002313;	//Richard
		//MProtocol_id = 0x50000223;	//Pascal
	#endif
	MProtocol_id &= 0x00FFFFFF;		// Since the bind ID starts with 50, let's keep only the last 3 bytes of the ID
	MProtocol_id |= 0x50000000;		// As recommended on the A7105 datasheet
	set_rx_tx_addr(MProtocol_id);	// Update the ID
	
	hopping_frequency_no=0;
	rf_ch_num = 0;

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = WFLY2_BIND_COUNT;
		A7105_WriteID(0x50FFFFFE);	// Bind ID
		phase = WFLY2_BIND;
	}
	else
	{
		A7105_WriteID(MProtocol_id);
		phase = WFLY2_DATA;
	}
	#ifdef WFLY2_HUB_TELEMETRY
		packet_count = 0;
		telemetry_lost = 1;
	#endif
	return 2000;
}
#endif
