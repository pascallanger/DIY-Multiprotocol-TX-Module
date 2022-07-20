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

#if defined(KYOSHO2_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define KYOSHO2_PACKET_PERIOD			1120	// 1600 for bind, let's see
#define KYOSHO2_BIND_COUNT				2000	// about 3sec
#define KYOSHO2_BIND_CHANNEL			0x50
#define KYOSHO2_PAYLOAD_SIZE			28
#define KYOSHO2_RF_CHANNELS				15
#define KYOSHO2_START_RF_CHANNEL		0x13	// No idea where it comes from... ID or unknown bytes during the bind?
#define KYOSHO2_NUM_CHANNEL				10		// Only 4 on the dumps but there is space for 10 channels in the payload...

#define FORCE_KYOSHO2_ID

bool KYOSHO2_resend;
//
static void __attribute__((unused)) KYOSHO2_send_packet()
{
	if(KYOSHO2_resend == true)
	{
		NRF24L01_Strobe(NRF24L01_E3_REUSE_TX_PL);
		if(IS_BIND_DONE)
			KYOSHO2_resend = false;
		return;
	}
	
	memset(packet,0x00,KYOSHO2_PAYLOAD_SIZE);
	
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(packet, (uint8_t*)"\x01\x02\x05\x08\x1A\x2B\x3C\x4D", 8);	// unknown bytes, parameters on how to build the rf channels?
		memcpy(&packet[8], rx_tx_addr, 4);
	}
	else
	{
		memcpy(packet, rx_tx_addr, 4);
		//Hopp
		packet[6] = hopping_frequency_no + KYOSHO2_START_RF_CHANNEL;
		packet[7] = hopping_frequency[hopping_frequency_no];
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, packet[6+(rf_ch_num&0x01)]);
		rf_ch_num++;
		//Channels
		uint16_t temp;
		for (uint8_t i = 0; i< KYOSHO2_NUM_CHANNEL; i++)
		{
			temp=convert_channel_16b_limit(i,0,0x3FF);
			packet[8+i*2] = temp >> 8;
			packet[9+i*2] = temp;
		}
	}
	//Send
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));	// Reset flags
    NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet,KYOSHO2_PAYLOAD_SIZE);
	NRF24L01_SetPower();
	KYOSHO2_resend = true;
	
	#if 0
		for(uint8_t i=0;i<KYOSHO2_PAYLOAD_SIZE;i++)
			debug("%02X ", packet[i]);
		debugln("");
	#endif
}

static void __attribute__((unused)) KYOSHO2_RF_init()
{
	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, KYOSHO2_BIND_CHANNEL);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"\x69\x53\x10\xAC\xEF", 5);
}

static void __attribute__((unused)) KYOSHO2_initialize_tx_id()
{
	hopping_frequency_no = rx_tx_addr[3]%KYOSHO2_RF_CHANNELS;
	hopping_frequency[0] = 0x4A;
	#ifdef FORCE_KYOSHO2_ID
		memcpy(rx_tx_addr, (uint8_t*)"\x0A\xBD\x31\xDF", 4);
		hopping_frequency[0] = 0x4A;						// No idea where it comes from... ID or unknown bytes during the bind?
	#endif
	for(uint8_t i=1;i<KYOSHO2_RF_CHANNELS;i++)
	{
		if(hopping_frequency[i-1]+5 < 0x50)
			hopping_frequency[i] = hopping_frequency[i-1]+5;
		else
			hopping_frequency[i] = hopping_frequency[i-1]-0x21;
	}
	#if 0
		for(uint8_t i=0;i<KYOSHO2_RF_CHANNELS;i++)
			debugln("1:%02X, 2: %02X", i + KYOSHO2_START_RF_CHANNEL, hopping_frequency[i]);
		debugln("Selected 1:%02X, 2: %02X", hopping_frequency_no + KYOSHO2_START_RF_CHANNEL, hopping_frequency[hopping_frequency_no]);
	#endif
}

uint16_t KYOSHO2_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(KYOSHO2_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
		{
			BIND_DONE;
			KYOSHO2_resend = false;
		}
	KYOSHO2_send_packet();
	return KYOSHO2_PACKET_PERIOD;
}

void KYOSHO2_init()
{
	KYOSHO2_initialize_tx_id();
	KYOSHO2_RF_init();
	rf_ch_num = 0;

	if(IS_BIND_IN_PROGRESS)
		bind_counter = KYOSHO2_BIND_COUNT;
	else 
		bind_counter = 0;
	KYOSHO2_resend = false;
}

#endif
