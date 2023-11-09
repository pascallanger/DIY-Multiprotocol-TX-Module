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
// compatible with BLUEFLY HP100

#if defined(BLUEFLY_CCNRF_INO)

#include "iface_nrf250k.h"

#define BLUEFLY_PACKET_PERIOD		7000
#define BLUEFLY_PACKET_SIZE			12
#define BLUEFLY_RF_BIND_CHANNEL		81
#define BLUEFLY_NUM_RF_CHANNELS		15
#define BLUEFLY_BIND_COUNT			800
#define BLUEFLY_TXID_SIZE			5

static void __attribute__((unused)) BLUEFLY_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		memset(packet, 0x55, BLUEFLY_PACKET_SIZE);
		memcpy(packet, rx_tx_addr, BLUEFLY_TXID_SIZE);
		packet[5] = hopping_frequency[0];
	}
	else
	{
		NRF250K_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		if(hopping_frequency_no >= BLUEFLY_NUM_RF_CHANNELS);
			hopping_frequency_no = 0;

		packet[8] = packet[9] = 0;
		for(uint8_t i=0; i<8 ; i++)
		{
			uint16_t ch = convert_channel_16b_limit(CH_AETR[i], 0, 1000);
			packet[ i] = ch;
			ch &= 0x300;
			ch >>= 2;
			packet[8 + (i>3?0:1)] = (packet[8 + (i>3?0:1)] >> 2) | ch;
		}
		uint8_t l, h, t;
		l = h = 0xff;
		for (uint8_t i=0; i<10; ++i)
		{
			h ^= packet[i];
			h ^= h >> 4;
			t = h;
			h = l;
			l = t;
			t = (l<<4) | (l>>4);
			h ^= ((t<<2) | (t>>6)) & 0x1f;
			h ^= t & 0xf0;
			l ^= ((t<<1) | (t>>7)) & 0xe0;
		}
		// Checksum
		packet[10] = h; 
		packet[11] = l;
	}
	
	NRF250K_WritePayload(packet, BLUEFLY_PACKET_SIZE);
	NRF250K_SetPower();				// Set tx_power
	NRF250K_SetFreqOffset();		// Set frequency offset
}

static void __attribute__((unused)) BLUEFLY_RF_init()
{
	NRF250K_Init();
	NRF250K_SetTXAddr((uint8_t *)"\x32\xAA\x45\x45\x78", 5);	// BLUEFLY Bind address
	NRF250K_HoppingCalib(BLUEFLY_NUM_RF_CHANNELS);				// Calibrate all channels
	NRF250K_RFChannel(BLUEFLY_RF_BIND_CHANNEL);					// Set bind channel
}

static void __attribute__((unused)) BLUEFLY_initialize_txid()
{
    uint8_t start = (rx_tx_addr[3] % 47) + 2;
	for(uint8_t i=0;i<BLUEFLY_NUM_RF_CHANNELS;i++)
		hopping_frequency[i] = start + i*2;
	hopping_frequency_no=0;
}

uint16_t BLUEFLY_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(BLUEFLY_PACKET_PERIOD);
	#endif
	if(bind_counter)
	{
		bind_counter--;
		if (bind_counter == 0)
		{
			BIND_DONE;
			NRF250K_SetTXAddr(rx_tx_addr, 5);
		}
	}
	BLUEFLY_send_packet();
	return	BLUEFLY_PACKET_PERIOD;
}

void BLUEFLY_init(void)
{
	BLUEFLY_initialize_txid();
	BLUEFLY_RF_init();

	bind_counter = IS_BIND_IN_PROGRESS ? BLUEFLY_BIND_COUNT : 1;
}

#endif
