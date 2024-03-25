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

#if defined(KYOSHO3_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define KYOSHO3_FORCE_ID
//#define KYOSHO3_DEBUG

#define KYOSHO3_BIND_PACKET_SIZE	4
#define KYOSHO3_PACKET_SIZE			9

const uint8_t PROGMEM KYOSHO3_init_vals[][2] = {
	//Init from dump
	{CYRF_0B_PWR_CTRL, 0x00},					// PMU
	{CYRF_32_AUTO_CAL_TIME, 0x3C},				// Default init value
	{CYRF_35_AUTOCAL_OFFSET, 0x14},				// Default init value
	{CYRF_03_TX_CFG, 0x28 | CYRF_BIND_POWER},	// 8DR Mode, 64 chip codes
	{CYRF_10_FRAMING_CFG, 0xA4},				// SOP and LEN enable
	{CYRF_1F_TX_OVERRIDE, 0x05},				// Disable CRC, Data invert
	{CYRF_1E_RX_OVERRIDE, 0x04},				// CRC check disabled
	//{CYRF_11_DATA32_THOLD, 0x04},				// ???Using 64 chip...
	{CYRF_12_DATA64_THOLD, 0x0E},				// Default
	{CYRF_06_RX_CFG, 0x52},						// AGC disabled, LNA enabled, override enabled
};

static uint16_t __attribute__((unused)) KYOSHO3_send_packet()
{
	CYRF_SetPower(0x28);
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xAA;
		//ID
		memcpy(&packet[1],&rx_tx_addr[1],3);
		CYRF_WriteDataPacketLen(packet, KYOSHO3_BIND_PACKET_SIZE);
		#ifdef KYOSHO3_DEBUG
			debug("P:");
			for(uint8_t i=0;i<KYOSHO3_BIND_PACKET_SIZE;i++)
				debug(" %02X",packet[i]);
			debugln("");
		#endif
		return 2434;
	}
	else
	{
		//ID
		memcpy(packet,&rx_tx_addr[1],3);
		//Channels
		for(uint8_t i=0;i<4;i++)
		{
			packet[3] >>= 2;
			packet[3] |= Channel_data[i]<<6;
			packet[4+i]  = Channel_data[i]>>3;
		}
		//Checksum
		packet[8] = packet[3];
		for(uint8_t i=4;i<8;i++)
			packet[8] += packet[i];
		//Timing
		phase ^= 0x01;
		CYRF_WriteDataPacketLen(packet, KYOSHO3_PACKET_SIZE);
		#ifdef KYOSHO3_DEBUG
			debug("P:");
			for(uint8_t i=0;i<KYOSHO3_PACKET_SIZE;i++)
				debug(" %02X",packet[i]);
			debugln("");
		#endif
		if(phase)
			return 9047;
	}
	return 6957;
}

uint16_t KYOSHO3_callback()
{
	if(IS_BIND_IN_PROGRESS)
	{
		if(--bind_counter==0)
			BIND_DONE;
	}
	return KYOSHO3_send_packet();
}

void KYOSHO3_init()
{ 
	//Config CYRF registers
	for(uint8_t i = 0; i < sizeof(KYOSHO3_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&KYOSHO3_init_vals[i][0]), pgm_read_byte_near(&KYOSHO3_init_vals[i][1]));
	CYRF_WritePreamble(0x333304);

	//Find a free even channel
	CYRF_FindBestChannels(hopping_frequency,1,1,0x04,0x50, FIND_CHANNEL_EVEN);
	hopping_frequency[0] = 0x04;

	#ifdef KYOSHO3_FORCE_ID					// data taken from TX dump
		rx_tx_addr[1]=0x01;
		rx_tx_addr[2]=0xAB;
		rx_tx_addr[3]=0x31;
		hopping_frequency[0] = 0x04;
	#endif
	#ifdef KYOSHO3_DEBUG
		debugln("ID: %02X %02X %02X",rx_tx_addr[1],rx_tx_addr[2],rx_tx_addr[3]);
		debugln("RF CH: %02X",hopping_frequency[0]);
	#endif

	CYRF_ConfigRFChannel(hopping_frequency[0]);
	
	bind_counter=1000;
	phase=0;
}

#endif
