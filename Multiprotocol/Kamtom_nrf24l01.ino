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
#if defined(KAMTOM_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_KAMTOM_ORIGINAL_ID

#define KAMTOM_PAYLOAD_SIZE			16
#define KAMTOM_RF_NUM_CHANNELS		4
#define KAMTOM_BIND_COUNT			2000
#define KAMTOM_WRITE_TIME			650
#define KAMTOM_BIND_CHANNEL			0x28	//40
#define KAMTOM_PACKET_PERIOD		3585


enum {
	KAMTOM_DATA,
	KAMTOM_RX,
};

static void __attribute__((unused)) KAMTOM_send_packet()
{
	if(bind_counter)
	{
		bind_counter--;
		if(!bind_counter)
			BIND_DONE;
	}

	memset(packet, 0x00, 16);

	if(IS_BIND_DONE)
	{//Normal
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= 3;

		//RXID
		packet[0] = rx_tx_addr[0];
		packet[2] = rx_tx_addr[1];
		//Next RF channel
		packet[1] = hopping_frequency[hopping_frequency_no];
		//Channels and trims
		for(uint8_t i=0; i<6; i++)
		{
			packet[4+i] = convert_channel_s8b(CH_TAER[i]);
			if(i>3) //ST_TR and TH_TR
				packet[4+i] >>= 2;
		}
		//packet[11] = 0x00;	//??
		//TH_DR
		packet[12] = convert_channel_16b_limit(CH7,0x25,0x64);
	}
	else
	{
		packet[1] = KAMTOM_BIND_CHANNEL;
		memcpy(&packet[4],hopping_frequency,4);
		packet[12] = 0xA5;
	}
	packet[10] = 0x40;	//??
	//Checksum
	uint16_t sum = packet[1];
	for(uint8_t i=4;i<13;i++)
		 sum += packet[i];
	packet[13] = sum;
	packet[3] = (sum>>6) & 0xFC;
	//TXID
	packet[14] = rx_tx_addr[2];
	packet[15] = rx_tx_addr[3];
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, KAMTOM_PAYLOAD_SIZE,false);
	#if 0
	//def DEBUG_SERIAL
		for(uint8_t i=0; i < KAMTOM_PAYLOAD_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) KAMTOM_initialize_txid()
{
	calc_fh_channels(4);
	#ifdef FORCE_KAMTOM_ORIGINAL_ID
		rx_tx_addr[0] = 0xC7;
		rx_tx_addr[1] = 0x78;
		rx_tx_addr[2] = 0x2C;
		rx_tx_addr[3] = 0x25;
		hopping_frequency[0] = 59;
		hopping_frequency[1] = 59;
		hopping_frequency[2] = 71;
		hopping_frequency[3] = 65;
	#endif
}

static void __attribute__((unused)) KAMTOM_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	//Address
	XN297_SetTXAddr((uint8_t*)"\xCC\xDD\xEE\xDD", 4);
	XN297_SetRXAddr((uint8_t*)"\xCC\xDD\xEE\xDD", KAMTOM_PAYLOAD_SIZE);
	XN297_RFChannel(KAMTOM_BIND_CHANNEL);
}

uint16_t KAMTOM_callback()
{
	static bool rx=false;
	
	switch(phase)
	{
		case KAMTOM_DATA:
			rx = XN297_IsRX();
			XN297_SetTxRxMode(TXRX_OFF);
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(KAMTOM_PACKET_PERIOD);
			#endif
			KAMTOM_send_packet();
			if(rx)
			{
				uint8_t val=XN297_ReadEnhancedPayload(packet_in, KAMTOM_PAYLOAD_SIZE);
				if(val==KAMTOM_PAYLOAD_SIZE)
				{
					BIND_DONE;
					if(packet_in[0] == 0xA0 && packet_in[14] == rx_tx_addr[2] && packet_in[15] == rx_tx_addr[3])
					{
						rx_tx_addr[0] = packet_in[9];
						rx_tx_addr[1] = packet_in[10];
						//if(packet_in[1] == 0x03)		// low voltage
					}
					#if 0
						for(uint8_t i=0; i < KAMTOM_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
						debugln();
					#endif
				}
			}
			phase++;
			return KAMTOM_WRITE_TIME;
		default: //KAMTOM_RX
			//{ // Wait for packet to be sent before switching to receive mode
			//	uint16_t start=(uint16_t)micros();
			//	while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)
			//		if(XN297_IsPacketSent())
			//			break;
			//}
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = KAMTOM_DATA;
			return KAMTOM_PACKET_PERIOD - KAMTOM_WRITE_TIME;
	}
	return 0;
}

void KAMTOM_init()
{
	KAMTOM_initialize_txid();
	KAMTOM_RF_init();

	bind_counter = KAMTOM_BIND_COUNT;
	phase = KAMTOM_DATA;
	hopping_frequency_no = 0;
}

#endif
