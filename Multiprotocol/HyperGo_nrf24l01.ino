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
// Compatible with MJX Hyper Go T3A / R30 (H16BM and similar)

#if defined(HYPERGO_NRF24L01_INO)

#include "iface_xn297.h"

#define FORCE_HYPERGO_ORIGINAL_ID

#define HYPERGO_PACKET_PERIOD		9870
#define HYPERGO_RF_NUM_CHANNELS		3
#define HYPERGO_PAYLOAD_SIZE		13
#define HYPERGO_STARTUP_COUNT		32		// ~300ms of 00,00 like stock TX

static void __attribute__((unused)) HYPERGO_send_packet()
{
	XN297_Hopping(hopping_frequency_no);
	hopping_frequency_no++;
	if (hopping_frequency_no >= HYPERGO_RF_NUM_CHANNELS)
		hopping_frequency_no = 0;

	packet[0] = 0x55;
	packet[1] = 0x37;
	packet[2] = 0xE1;
	// Flags: bit0x10=rate 100%, clear=70%; bit0x02=LED
	// CH5 ON→70%, CH5 OFF→100%; CH6=LED
	packet[3] = 0xC0 | GET_FLAG(!CH5_SW, 0x10) | GET_FLAG(CH6_SW, 0x02);

	if (bind_counter)
	{
		// Stock TX: [4]=00 [5]=00 for a short window after power-up
		packet[4] = 0x00;
		packet[5] = 0x00;
		bind_counter--;
		if (bind_counter == 0)
			BIND_DONE;
	}
	else
	{
		packet[4] = convert_channel_s8b(CH2);	// Throttle
		packet[5] = convert_channel_s8b(CH1);	// Steering
	}

	packet[6] = (uint8_t)convert_channel_16b_limit(CH7, 0x0E, 0x72);	// ST trim

	// Companions: center 0x7E; else copy axis
	packet[7] = (packet[4] == 0x80) ? 0x7E : packet[4];
	packet[8] = (packet[5] == 0x80) ? 0x7E : packet[5];

	// Address echo: addr[0], addr[2], addr[4]
	packet[9]  = rx_tx_addr[0];
	packet[10] = rx_tx_addr[2];
	packet[11] = rx_tx_addr[4];

	uint8_t sum = 0x6D;
	for (uint8_t i = 0; i < HYPERGO_PAYLOAD_SIZE - 1; i++)
		sum += packet[i];
	packet[12] = sum;

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, HYPERGO_PAYLOAD_SIZE);
}

static void __attribute__((unused)) HYPERGO_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr(rx_tx_addr, 5);
	XN297_HoppingCalib(HYPERGO_RF_NUM_CHANNELS);
}

static void __attribute__((unused)) HYPERGO_initialize_txid()
{
	#ifdef FORCE_HYPERGO_ORIGINAL_ID
		memcpy(rx_tx_addr, "\xB4\x46\xF2\x25\xD7", 5);
		hopping_frequency[0] = 62;
		hopping_frequency[1] = 49;
		hopping_frequency[2] = 75;
	#else
		// Placeholder until ID→hop algorithm is known
		hopping_frequency[0] = 62;
		hopping_frequency[1] = 49;
		hopping_frequency[2] = 75;
	#endif
}

uint16_t HYPERGO_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(HYPERGO_PACKET_PERIOD);
	#endif
	HYPERGO_send_packet();
	return HYPERGO_PACKET_PERIOD;
}

void HYPERGO_init()
{
	BIND_IN_PROGRESS;	// brief startup window, then BIND_DONE
	HYPERGO_initialize_txid();
	HYPERGO_RF_init();
	hopping_frequency_no = 0;
	bind_counter = HYPERGO_STARTUP_COUNT;
}

#endif
