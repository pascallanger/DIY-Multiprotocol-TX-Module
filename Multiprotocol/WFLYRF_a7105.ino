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

#if defined(WFLYRF_A7105_INO)

#include "iface_a7105.h"

#define WFLYRF_FORCE_ID

//WFLYRF constants & variables
#define WFLYRF_BIND_COUNT		2500
#define WFLYRF_PACKET_SIZE		32

enum{
	WFLYRF_BIND,
	WFLYRF_DATA,
	WFLYRF_PLL_TX,
	WFLYRF_RX,
};

static void __attribute__((unused)) WFLYRF_send_bind_packet()
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
		for(uint8_t i=0; i<WFLYRF_PACKET_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif

	//Send
	A7105_WriteData(WFLYRF_PACKET_SIZE, rf_ch_num);
}

static void __attribute__((unused)) WFLYRF_build_packet()
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
	packet[4] = rx_tx_addr[2] & 0x1F;	//not sure... could be 0x1F down to 0x03

	//10 channels
	for(uint8_t i = 0; i < 5; i++)
	{
		uint16_t temp=convert_channel_16b_nolimit(i*2 , 0x0000, 0x0FFF);	// need to check channels min/max...
		packet[5 + i*3]  = temp&0xFF;		// low byte
		packet[7 + i*3]  = (temp>>8)&0x0F;	// high byte
		temp=convert_channel_16b_nolimit(i*2+1, 0x0000, 0x0FFF);	// need to check channels min/max...
		packet[6 + i*3]  = temp&0xFF;		// low byte
		packet[7 + i*3] |= (temp>>4)&0xF0;	// high byte
	}
	
	//Unknown
	memset(&packet[20],0x00,12);

	//Debug
	#if 0
		debug("ch=%02X,%02X P=",rf_ch_num,(rf_ch_num<<1)+0x10);
		for(uint8_t i=0; i<WFLYRF_PACKET_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif
}

#ifdef WFLYRF_HUB_TELEMETRY
	static void __attribute__((unused)) WFLYRF_Send_Telemetry()
	{
		//Incoming packet values
		v_lipo1=packet[3]<<1;		// RX_batt*10 in V
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

#define WFLYRF_PACKET_PERIOD	3600	//3600
#define WFLYRF_BUFFER_TIME		1500	//1500
#define WFLYRF_WRITE_TIME		800		//942

uint16_t ReadWFLYRF()
{
	uint16_t start;
	#ifdef WFLYRF_HUB_TELEMETRY
		uint8_t status;
	#endif
	
	#ifndef FORCE_WFLYRF_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	switch(phase)
	{
		case WFLYRF_BIND:
			bind_counter--;
			if (bind_counter == 0)
			{
				BIND_DONE;
				A7105_WriteID(MProtocol_id);
				rf_ch_num = 0;
				phase++;	// WFLYRF_DATA
			}
			WFLYRF_send_bind_packet();
			return WFLYRF_PACKET_PERIOD;

		case WFLYRF_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(WFLYRF_PACKET_PERIOD);
			#endif
			//Build data packet
			WFLYRF_build_packet();

			//Fill the TX buffer without sending
			A7105_WriteReg(A7105_03_FIFOI, 0x1F);
			A7105_CSN_off;
			SPI_Write(A7105_RST_WRPTR);
			SPI_Write(A7105_05_FIFO_DATA);
			for (uint8_t i = 0; i < WFLYRF_PACKET_SIZE; i++)
				SPI_Write(packet[i]);
			A7105_CSN_on;
			
			#ifdef WFLYRF_HUB_TELEMETRY
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
			
			phase++;	// WFLYRF_PLL_TX
			return WFLYRF_BUFFER_TIME;
			
		case WFLYRF_PLL_TX:
			#ifdef WFLYRF_HUB_TELEMETRY
				//Check RX status
				status=A7105_ReadReg(A7105_00_MODE);
				//debugln("S:%02X", status);
			#endif
			
			//PLL
			A7105_Strobe(A7105_PLL);
			
			#ifdef WFLYRF_HUB_TELEMETRY
				//Read incoming packet even if bad/not present to not change too much the TX timing, might want to reorg the code...
				A7105_ReadData(WFLYRF_PACKET_SIZE);

				//Read telemetry
				if((status & 0x21)==0)
				{ // Packet received and CRC OK
					//Debug
					#if 1
						debug("T:");
						for(uint8_t i=0; i<WFLYRF_PACKET_SIZE-20; i++)		// Can't send the full telemetry at full speed
							debug(" %02X", packet[i]);
						debugln("");
					#endif
					//Packet match the ID ?
					if(packet[0]==0 && packet[1]==rx_tx_addr[3] && packet[2]==rx_tx_addr[2]) //need to check if this is the full rx_tx_addr[2] or the partial one...
						WFLYRF_Send_Telemetry();							// Packet looks good do send telem to the radio
				}
			#endif
			
			//Change RF channel
			A7105_WriteReg(A7105_0F_PLL_I, (rf_ch_num<<1)+0x10);
			
			//Switch to TX
			A7105_SetPower();
			A7105_SetTxRxMode(TX_EN);
			A7105_Strobe(A7105_TX);

			phase++;	// WFLYRF_RX
			return WFLYRF_WRITE_TIME;
			
		case WFLYRF_RX:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)((uint16_t)micros()-start) < 700)				// Wait max 700µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			
			//A7105_WriteReg(A7105_0F_PLL_I, (rf_ch_num<<1)+0x10);			// Again in dumps?? It should not be needed...
			
			//Switch to RX
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			
			phase = WFLYRF_DATA;
			return WFLYRF_PACKET_PERIOD-WFLYRF_WRITE_TIME-WFLYRF_BUFFER_TIME;
	}
	return WFLYRF_PACKET_PERIOD; // never reached, please the compiler
}

uint16_t initWFLYRF()
{
	A7105_Init();

	#ifdef WFLYRF_FORCE_ID
		//MProtocol_id = 0x50002313;	//Richard
		MProtocol_id = 0x50000223;	//Pascal
	#endif
	MProtocol_id &= 0x0FFFFFFF;		// Since the bind ID starts with 50 this mask might start with 00 instead 0F
	MProtocol_id |= 0x50000000;		// As recommended on the A7105 datasheet
	set_rx_tx_addr(MProtocol_id);	// Update the ID
	
	hopping_frequency_no=0;
	rf_ch_num = 0;

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = WFLYRF_BIND_COUNT;
		A7105_WriteID(0x50FFFFFE);	// Bind ID
		phase = WFLYRF_BIND;
	}
	else
	{
		A7105_WriteID(MProtocol_id);
		phase = WFLYRF_DATA;
	}
	#ifdef WFLYRF_HUB_TELEMETRY
		packet_count = 0;
		telemetry_lost = 1;
	#endif
	return 2000;
}
#endif
