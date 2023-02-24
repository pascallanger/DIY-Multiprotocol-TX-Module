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

#if defined(MICROZONE_A7105_INO)

#include "iface_a7105.h"

//#define MICROZONE_FORCE_ID_FHSS
//#define MICROZONE_FORCE_ID_HYPE

//MICROZONE constants & variables
#define MICROZONE_BIND_COUNT 2500

static void __attribute__((unused)) MICROZONE_send_packet()
{
	//ID
	packet[1] = rx_tx_addr[0];
	packet[2] = rx_tx_addr[1];
	packet[3] = rx_tx_addr[2];
	packet[4] = rx_tx_addr[3];
	//unknown may be RX ID on some other remotes
	memset(packet+5,0xFF,4);
	
	if(IS_BIND_IN_PROGRESS)
	{
		packet[ 0]  = 0xBC;						// bind indicator
		packet[ 9] &= 0x01;
		packet[ 9] ^= 0x01;						// high/ low part of the RF table
		packet[10]  = 0x00;
		//RF table
		for(uint8_t i=0; i<16;i++)
			packet[i+11]=hopping_frequency[i+(packet[9]<<4)];
		//unknwon
		packet[27]  = 0x05;
		packet[28]  = 0x00;
		memset(packet+29,0xFF,8);
		//frequency hop during bind
		if(packet[9])
			rf_ch_num=0x8C;
		else
			rf_ch_num=0x0D;
	}
	else
	{
		packet[ 0]  = 0x58;						// normal packet
		//14 channels: steering, throttle, ...
		for(uint8_t i = 0; i < 14; i++)
		{
			uint16_t temp=convert_channel_ppm(i);
			packet[9 + i*2]=temp&0xFF;			// low byte of servo timing(1000-2000us)
			packet[10 + i*2]=(temp>>8)&0xFF;	// high byte of servo timing(1000-2000us)
		}
		rf_ch_num=hopping_frequency[hopping_frequency_no];
		hopping_frequency_no++;
		packet[34] |= (hopping_frequency_no&0x0F)<<4;
		packet[36] |= (hopping_frequency_no&0xF0);		// last byte is ending with F on the dumps so let's see
		hopping_frequency_no &= 0x1F;
	}
	#if 0
		debug("ch=%02X P=",rf_ch_num);
		for(uint8_t i=0; i<37; i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif
	A7105_WriteData(37, rf_ch_num);
}

static void __attribute__((unused)) MICROZONE_hype_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		if(packet_sent==0)
		{//build the packet and send it
			packet[0] = rx_tx_addr[1];
			packet[1] = rx_tx_addr[3];
			//RF table
			for(uint8_t i=0; i<15;i++)
				packet[i+2]=hopping_frequency[i];
			A7105_WriteData(17, 0x01);
			packet_sent++;
			packet_period=1421;
			#if 0
				debug("ch=01 P=");
				for(uint8_t i=0; i<17; i++)
					debug("%02X ", packet[i]);
				debugln("");
			#endif
		}
		else
			A7105_Strobe(A7105_TX);	//only send
	}
	else
	{
		//original TX is only refreshing the packet every 20ms and keep repeating the same packet in between (STROBE_TX) 
		//build packet=6 channels with order AETR
		for(uint8_t i=0;i<6;i++)
			packet[i] = convert_channel_8b(CH_AETR[i]);
		//set RF channel
		rf_ch_num=hopping_frequency[hopping_frequency_no];
		hopping_frequency_no++;
		if(hopping_frequency_no>14)
			hopping_frequency_no = 0;
		//send it
		A7105_WriteData(6, rf_ch_num);
		packet_period=931;	//packet period fluctuates a lot on the original TX from one packet to the other but stable if looking over a period of 40ms
		#if 0
			debug("ch=%02X P=",rf_ch_num);
			for(uint8_t i=0; i<6; i++)
				debug("%02X ", packet[i]);
			debugln("");
		#endif
	}
}

uint16_t MICROZONE_callback()
{
	#ifndef FORCE_MICROZONE_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter--;
		if (bind_counter==0)
		{
			BIND_DONE;
			if(sub_protocol==MICROZONE_M1)
			{
				A7105_WriteID(MProtocol_id);
				A7105_WriteReg(A7105_03_FIFOI,0x05);
			}
		}
	}
	else
	{
		if(hopping_frequency_no==0)
			A7105_SetPower();
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(packet_period);
		#endif
	}
//	if(sub_protocol==MICROZONE_FHSS)
//		MICROZONE_send_packet();
//	else//HYPE
		MICROZONE_hype_send_packet();
	return packet_period;
}

void MICROZONE_init()
{
	A7105_Init();

	// compute channels from ID
	calc_fh_channels(15);
	hopping_frequency_no=0;

	if(sub_protocol==MICROZONE_M1)
	{
		MProtocol_id &= 0x00FF00FF;
		rx_tx_addr[0] = 0xAF - (rx_tx_addr[1]&0x0F);
		rx_tx_addr[2] = 0xFF -  rx_tx_addr[3];
		MProtocol_id |= (rx_tx_addr[0]<<24) + (rx_tx_addr[2]<<8);
		#ifdef MICROZONE_FORCE_ID
			MProtocol_id=0xAF90738C;
			set_rx_tx_addr(MProtocol_id);
			memcpy(hopping_frequency,"\x27\x1B\x63\x75\x03\x39\x57\x69\x87\x0F\x7B\x3F\x33\x51\x6F",15);
		#endif
		if(IS_BIND_IN_PROGRESS)
			A7105_WriteID(0xAF00FF00);
		else
		{
			A7105_WriteID(MProtocol_id);
			A7105_WriteReg(A7105_03_FIFOI,0x05);
		}
	}

	if(IS_BIND_IN_PROGRESS)
		bind_counter = MICROZONE_BIND_COUNT;

	packet_sent=0;
	packet_period=3852;		//FHSS
}
#endif
