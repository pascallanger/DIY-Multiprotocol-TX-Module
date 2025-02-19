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
#if defined(UDIRC_CCNRF_INO)

#include "iface_xn297.h"

#define FORCE_UDIRC_ORIGINAL_ID

#define UDIRC_PAYLOAD_SIZE			15
#define UDIRC_RF_NUM_CHANNELS		4
#define UDIRC_PACKET_PERIOD			21000
#define UDIRC_BIND_COUNT			2000
#define UDIRC_P1_P2_TIME			5000
#define UDIRC_WRITE_TIME			1500

enum {
	UDIRC_DATA1=0,
	UDIRC_DATA2,
	UDIRC_RX,
};

static void __attribute__((unused)) UDIRC_send_packet()
{
	if(rf_ch_num==0)
	{
		XN297_Hopping(hopping_frequency_no);
		debug("H %d ",hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= 3;
	}

	memset(&packet[3], 0x00, 12);
	if(bind_counter)
	{//Bind in progress
		bind_counter--;
		if(bind_counter)
		{//Bind
			packet[0] = 0x01;
			memcpy(&packet[1],rx_tx_addr,5);
		}
		else
		{//Switch to normal
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 5);
			XN297_SetRXAddr(rx_tx_addr, UDIRC_PAYLOAD_SIZE);
		}
	}
	if(!bind_counter)
	{//Normal
		packet[0] = 0x08;
		//Channels ST/TH/CH4 /CH3  /UNK/UNK/UNK/UNK/GYRO/ST_TRIM/ST_DR
		//Channels ST/TH/RATE/LIGHT/UNK/UNK/UNK/UNK/GYRO/ST_TRIM/ST_DR
		for(uint8_t i=0; i<9; i++)
			packet[i+1] = convert_channel_16b_limit(i,0,200);
		//Just for now let's set the additional channels to 0
		packet[5] = packet[6] = packet[7] = packet[8] = 0;
	}
	packet[12] = GET_FLAG(CH12_SW,  0x40)						//TH.REV
				|GET_FLAG(CH13_SW,  0x80);						//ST.REV
	//packet[13] = 00; //Unknown, future flags?
	for(uint8_t i=0;i<UDIRC_PAYLOAD_SIZE-1;i++)
		packet[14] += packet[i];
	// Send
	XN297_SetFreqOffset();
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, UDIRC_PAYLOAD_SIZE,false);
	#ifdef DEBUG_SERIAL
		for(uint8_t i=0; i < UDIRC_PAYLOAD_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) UDIRC_initialize_txid()
{
	#ifdef FORCE_UDIRC_ORIGINAL_ID
		if(RX_num)
		{
			rx_tx_addr[0] = 0xD0;
			rx_tx_addr[1] = 0x06;
			rx_tx_addr[2] = 0x00;
			rx_tx_addr[3] = 0x00;
			rx_tx_addr[4] = 0x81;
		}
		else
		{
			rx_tx_addr[0] = 0xF6;
			rx_tx_addr[1] = 0x96;
			rx_tx_addr[2] = 0x01;
			rx_tx_addr[3] = 0x00;
			rx_tx_addr[4] = 0x81;
		}
		hopping_frequency[0] = 45;
		hopping_frequency[1] = 59;
		hopping_frequency[2] = 52;
		hopping_frequency[3] = 67;
	#endif
}

static void __attribute__((unused)) UDIRC_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	//Bind address
	XN297_SetTXAddr((uint8_t*)"\x01\x03\x05\x07\x09", 5);
	XN297_SetRXAddr((uint8_t*)"\x01\x03\x05\x07\x09", UDIRC_PAYLOAD_SIZE);
	XN297_HoppingCalib(UDIRC_RF_NUM_CHANNELS);
}

uint16_t UDIRC_callback()
{
	bool rx;
	switch(phase)
	{
		case UDIRC_DATA1:
			rx = XN297_IsRX();
			XN297_SetTxRxMode(TXRX_OFF);
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(UDIRC_PACKET_PERIOD);
			#endif
			UDIRC_send_packet();
			if(rx)
			{
				uint8_t val=XN297_ReadEnhancedPayload(packet_in, UDIRC_PAYLOAD_SIZE);
				debug("RX %d",val);
				if(val != 255)
				{
					rf_ch_num = 1;
					if(bind_counter)
						bind_counter=1;
					#ifdef DEBUG_SERIAL
						for(uint8_t i=0; i < UDIRC_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
						debugln();
					#endif
				}
				debugln("");
			}
			phase++;
			return UDIRC_P1_P2_TIME;
		case UDIRC_DATA2:
			//Resend packet
			NRF24L01_Strobe(NRF24L01_E3_REUSE_TX_PL);
			phase++;
			return UDIRC_WRITE_TIME;
		default: //UDIRC_RX
			//Wait for the packet transmission to finish
			while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = UDIRC_DATA1;
			return UDIRC_PACKET_PERIOD - UDIRC_P1_P2_TIME - UDIRC_WRITE_TIME;
	}
	return 0;
}

void UDIRC_init()
{
	UDIRC_initialize_txid();
	UDIRC_RF_init();

	bind_counter = IS_BIND_IN_PROGRESS ? UDIRC_BIND_COUNT : 1;
	phase = UDIRC_DATA1;
	hopping_frequency_no = 0;
	rf_ch_num = 0;
}

#endif
