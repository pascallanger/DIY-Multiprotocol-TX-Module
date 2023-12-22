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

#if defined(BUMBLEB_CCNRF_INO)

#include "iface_xn297.h"

#define FORCE_BUMBLEB_ORIGINAL_ID
#define BUMBLEB_TELEM_DEBUG

#define BUMBLEB_PACKET_PERIOD	10200
#define BUMBLEB_RF_BIND_CHANNEL	42
#define BUMBLEB_RF_NUM_CHANNELS	2
#define BUMBLEB_PAYLOAD_SIZE	7

static void __attribute__((unused)) BUMBLEB_send_packet()
{
	packet[6] = 0x00;
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = rx_tx_addr[0];
		packet[1] = rx_tx_addr[1];
		packet[2] = 0x54;			 //???
		packet[3] = 0x58;			 //???
		hopping_frequency_no ^= 0x01;
		packet[4] = hopping_frequency[hopping_frequency_no];
	}
	else
	{
		//hopping frequency
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no ^= 0x01;
		packet[0] = 0x20
					|GET_FLAG(CH6_SW,0x80);				// High rate
		packet[1] = convert_channel_8b_limit_deadband(AILERON,0xBF,0xA0,0x81,40);	// Aileron: Max values:BD..A0..82
		if(packet[1] < 0xA0)
			packet[1] = 0x20 - packet[1];				// Reverse low part of aileron
		packet[2] = convert_channel_8b(CH5)>>2;			// 01..20..3F
		if(CH7_SW)										// Drive trim from aileron
		{
			uint8_t ch=convert_channel_8b(AILERON);
			if(ch > 0x5A && ch < 0x80-0x07)
				packet[2] = ch - 0x5A;
			else if(ch < 0x5A)
			{
				if(ch < 0x5A-0x20)
					packet[2] = 0;
				else
					packet[2] = ch - (0x5A-0x20);
			}
			else if(packet[1] == 0x89)
				packet[2] = 0x20;
			else if(ch > 0xA5)
			{
				if(ch > 0xA9+0x1F)
					packet[2] = 0x3F;
				else
					packet[2] = ch - 0x89;
			}
			else if(ch > 0xA5-0x1F)
				packet[2] = ch - (0xA5-0x1F-0x20);
		}
		else
			packet[2] = convert_channel_8b(CH5)>>2;		// 01..20..3F
		packet[3] = convert_channel_8b(THROTTLE)>>2;	// 00..3F
		packet[4] = hopping_frequency[hopping_frequency_no];
	}

	packet[5] = packet[0];
	for(uint8_t i=1;i<BUMBLEB_PAYLOAD_SIZE-2;i++)
		packet[5] += packet[i];

	#if 0
		debug("P:");
		for(uint8_t i=0;i<BUMBLEB_PAYLOAD_SIZE;i++)
			debug(" %02X", packet[i]);
		debugln("");
	#endif

	XN297_SetPower();									// Set tx_power
	XN297_SetFreqOffset();								// Set frequency offset
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, BUMBLEB_PAYLOAD_SIZE);
}

static void __attribute__((unused)) BUMBLEB_RF_init()
{
	//Config CC2500
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	XN297_SetTXAddr((uint8_t*)"\x55\x55\x55\x55\x55", 5);
	XN297_HoppingCalib(BUMBLEB_RF_NUM_CHANNELS);		// Calibrate all channels
	XN297_RFChannel(BUMBLEB_RF_BIND_CHANNEL);			// Set bind channel
	XN297_SetRXAddr(rx_tx_addr, BUMBLEB_PAYLOAD_SIZE);
}

static void __attribute__((unused)) BUMBLEB_initialize_txid()
{
	calc_fh_channels(BUMBLEB_RF_NUM_CHANNELS);
	rx_tx_addr[0] = rx_tx_addr[2];
	rx_tx_addr[1] = rx_tx_addr[3];
	#ifdef FORCE_BUMBLEB_ORIGINAL_ID
		rx_tx_addr[0] = 0x33;
		rx_tx_addr[1] = 0x65;
		hopping_frequency[0] = 2;
		hopping_frequency[1] = 40;
	#endif
	rx_tx_addr[2] = rx_tx_addr[3] = rx_tx_addr[4] = 0x55;
}

enum {
	BUMBLEB_BIND		= 0x00,
	BUMBLEB_BINDRX		= 0x01,
	BUMBLEB_DATA		= 0x02,
};

#define BUMBLEB_WRITE_TIME 850

uint16_t BUMBLEB_callback()
{
	bool rx;
	switch(phase)
	{
		case BUMBLEB_BIND:
			rx = XN297_IsRX();				// Needed for the NRF24L01 since otherwise the bit gets cleared

			BUMBLEB_send_packet();

			if( rx )
			{ // a packet has been received
				#ifdef BUMBLEB_TELEM_DEBUG
					debug("RX :");
				#endif
				if(XN297_ReadPayload(packet_in, BUMBLEB_PAYLOAD_SIZE))
				{ // packet with good CRC
					#ifdef BUMBLEB_TELEM_DEBUG
						debug("OK :");
						for(uint8_t i=0;i<BUMBLEB_PAYLOAD_SIZE;i++)
							debug(" %02X",packet_in[i]);
					#endif
					// packet_in = 4F 71 55 52 58 61 AA
					rx_tx_addr[2] = packet_in[0];
					rx_tx_addr[3] = packet_in[1];
					//rx_tx_addr[4] = packet_in[2];	// to test with other planes...
					XN297_SetTXAddr(rx_tx_addr, 5);
					BIND_DONE;
					phase = BUMBLEB_DATA;
					break;
				}
			}
			phase++;
			return BUMBLEB_WRITE_TIME;
		case BUMBLEB_BINDRX:
			{
				uint16_t start=(uint16_t)micros();
				while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)
				{
					if(XN297_IsPacketSent())
						break;
				}
			}
			XN297_SetTxRxMode(RX_EN);
			phase = BUMBLEB_BIND;
			return BUMBLEB_PACKET_PERIOD-BUMBLEB_WRITE_TIME;
		case BUMBLEB_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(BUMBLEB_PACKET_PERIOD);
			#endif
			BUMBLEB_send_packet();
			break;
	}
	return BUMBLEB_PACKET_PERIOD;
}

void BUMBLEB_init()
{
	BUMBLEB_initialize_txid();
	BUMBLEB_RF_init();
	hopping_frequency_no = 0;
	
	BIND_IN_PROGRESS;	// autobind protocol
	phase = BUMBLEB_BIND;
}

#endif
