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
#define WFLYRF_BIND_COUNT 2500

static void __attribute__((unused)) WFLYRF_send_packet()
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

uint16_t ReadWFLYRF()
{
	#ifndef FORCE_WFLYRF_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter--;
		if (bind_counter==0)
		{
			BIND_DONE;
			if(sub_protocol==WFLYRF_HYPE)
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
	WFLYRF_send_packet();
	return packet_period;
}

uint16_t initWFLYRF()
{
	A7105_Init();

	// compute channels from ID
	calc_fh_channels(sub_protocol==WFLYRF_FHSS?32:15);
	hopping_frequency_no=0;

	#ifdef WFLYRF_FORCE_ID_FHSS
		if(sub_protocol==WFLYRF_FHSS)
		{
			memcpy(rx_tx_addr,"\x3A\x39\x37\x00",4);
			memcpy(hopping_frequency,"\x29\x4C\x67\x92\x31\x1C\x77\x18\x23\x6E\x81\x5C\x8F\x5A\x51\x94\x7A\x12\x45\x6C\x7F\x1E\x0D\x88\x63\x8C\x4F\x37\x26\x61\x2C\x8A",32);
		}
	#endif
	if(sub_protocol==WFLYRF_HYPE)
	{
		MProtocol_id &= 0x00FF00FF;
		rx_tx_addr[0] = 0xAF - (rx_tx_addr[1]&0x0F);
		rx_tx_addr[2] = 0xFF -  rx_tx_addr[3];
		MProtocol_id |= (rx_tx_addr[0]<<24) + (rx_tx_addr[2]<<8);
		#ifdef WFLYRF_FORCE_ID_HYPE
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
		bind_counter = WFLYRF_BIND_COUNT;

	packet_sent=0;
	packet_period=3852;		//FHSS
	return 2000;
}
#endif
