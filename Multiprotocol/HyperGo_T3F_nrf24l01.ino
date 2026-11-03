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
// Experimental support for the captured MJX T3F / Hyper Go 43308.
// Captured: bind addr 6D 6A 58 52 43; data addr D6 7E A8 EF 47.
// Bind hop 61/79/43; data hop 75/64/53; 13-byte payload at 10ms.

#if defined(HYPERGO_T3F_NRF24L01_INO)

#include "iface_xn297.h"

// Captured ID for one T3F / 43308 pair; generic ID derivation is not known yet.

#define HYPERGO_T3F_PACKET_PERIOD		10000
#define HYPERGO_T3F_BIND_PACKET_PERIOD	10000
#define HYPERGO_T3F_RF_NUM_CHANNELS		3
#define HYPERGO_T3F_BIND_NUM_CHANNELS	3
#define HYPERGO_T3F_PAYLOAD_SIZE		13
#define HYPERGO_T3F_BIND_BEACON_COUNT	57
#define HYPERGO_T3F_BIND_FLAG50_COUNT	4

static const uint8_t HYPERGO_T3F_BIND_ADDR[5] = {0x6D, 0x6A, 0x58, 0x52, 0x43};
static const uint8_t HYPERGO_T3F_BIND_CHANNELS[HYPERGO_T3F_BIND_NUM_CHANNELS] = {61, 79, 43};

enum {
	HYPERGO_T3F_BIND=0,
	HYPERGO_T3F_BIND_ID,
	HYPERGO_T3F_BIND_FLAG50,
	HYPERGO_T3F_BIND_SWITCH,
	HYPERGO_T3F_DATA
};

static uint8_t HYPERGO_T3F_bind_channel_no;

static void __attribute__((unused)) HYPERGO_T3F_send_checksum()
{
	uint8_t sum = 0x6D;
	for (uint8_t i = 0; i < HYPERGO_T3F_PAYLOAD_SIZE - 1; i++)
		sum += packet[i];
	packet[12] = sum;
}

static void __attribute__((unused)) HYPERGO_T3F_send_data_packet()
{
	XN297_Hopping(hopping_frequency_no);
	hopping_frequency_no++;
	if (hopping_frequency_no >= HYPERGO_T3F_RF_NUM_CHANNELS)
		hopping_frequency_no = 0;

	packet[0] = 0x7A;
	packet[1] = 0xC8;
	packet[2] = 0x49;
	// T3F control captures use 0xD0 (100% rate); bit0x02 controls LEDs.
	packet[3] = 0xD0 | GET_FLAG(CH6_SW, 0x02);
	uint8_t throttle = convert_channel_8b(CH2);
	if (throttle < 128)
		packet[4] = 255 - throttle;	// Reverse: 0xFF at full reverse, approaches 0x80 near neutral
	else
		packet[4] = throttle - 128;	// Forward: 0 at neutral, 0x7F at full forward
	packet[5] = convert_channel_s8b(CH1);	// Steering
	packet[6] = (uint8_t)convert_channel_16b_limit(CH7, 0x0C, 0x70);	// ST trim, center=0x3E
	packet[7] = 0x80;	// Captured neutral value
	packet[8] = (uint8_t)convert_channel_16b_limit(CH8, 0, 100);	// Gyro setting from knob/channel, captured range 0..100
	packet[9]  = rx_tx_addr[0];
	packet[10] = rx_tx_addr[2];
	packet[11] = rx_tx_addr[4];
	HYPERGO_T3F_send_checksum();

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, HYPERGO_T3F_PAYLOAD_SIZE);
}

// Bind beacon / handshake on the captured three-channel bind sequence.
static void __attribute__((unused)) HYPERGO_T3F_send_bind_packet(uint8_t flag, bool echo_data_id)
{
	XN297_RFChannel(HYPERGO_T3F_BIND_CHANNELS[HYPERGO_T3F_bind_channel_no]);
	HYPERGO_T3F_bind_channel_no++;
	if (HYPERGO_T3F_bind_channel_no >= HYPERGO_T3F_BIND_NUM_CHANNELS)
		HYPERGO_T3F_bind_channel_no = 0;

	packet[0] = 0x7A;
	packet[1] = 0xC8;
	packet[2] = 0x49;
	packet[3] = flag;
	memset(&packet[4], 0, 5);
	if (echo_data_id)
	{
		packet[9]  = rx_tx_addr[0];
		packet[10] = rx_tx_addr[2];
		packet[11] = rx_tx_addr[4];
	}
	else
	{
		packet[9]  = 0x00;
		packet[10] = 0x00;
		packet[11] = 0x00;
	}
	HYPERGO_T3F_send_checksum();

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, HYPERGO_T3F_PAYLOAD_SIZE);
}

static void __attribute__((unused)) HYPERGO_T3F_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr(HYPERGO_T3F_BIND_ADDR, 5);
	XN297_SetRXAddr(HYPERGO_T3F_BIND_ADDR, HYPERGO_T3F_PAYLOAD_SIZE);
	XN297_RFChannel(HYPERGO_T3F_BIND_CHANNELS[0]);
	XN297_HoppingCalib(HYPERGO_T3F_RF_NUM_CHANNELS);			// data hops
}

static void __attribute__((unused)) HYPERGO_T3F_initialize_txid()
{
	memcpy(rx_tx_addr, "\xD6\x7E\xA8\xEF\x47", 5);
	hopping_frequency[0] = 75;
	hopping_frequency[1] = 64;
	hopping_frequency[2] = 53;
}

uint16_t HYPERGO_T3F_callback()
{
	switch (phase)
	{
		case HYPERGO_T3F_BIND:
			HYPERGO_T3F_send_bind_packet(0x10, false);
			if (--bind_counter == 0)
				phase = HYPERGO_T3F_BIND_ID;
			return HYPERGO_T3F_BIND_PACKET_PERIOD;

		case HYPERGO_T3F_BIND_ID:
			HYPERGO_T3F_send_bind_packet(0x10, true);
			bind_counter = HYPERGO_T3F_BIND_FLAG50_COUNT;
			phase = HYPERGO_T3F_BIND_FLAG50;
			return HYPERGO_T3F_BIND_PACKET_PERIOD;

		case HYPERGO_T3F_BIND_FLAG50:
			HYPERGO_T3F_send_bind_packet(0x50, true);
			if (--bind_counter == 0)
				phase = HYPERGO_T3F_BIND_SWITCH;
			return HYPERGO_T3F_PACKET_PERIOD;

		case HYPERGO_T3F_BIND_SWITCH:
			XN297_SetTXAddr(rx_tx_addr, 5);
			XN297_SetRXAddr(rx_tx_addr, HYPERGO_T3F_PAYLOAD_SIZE);
			hopping_frequency_no = 0;
			BIND_DONE;
			phase = HYPERGO_T3F_DATA;
			return 500;

		default:	// HYPERGO_T3F_DATA
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(HYPERGO_T3F_PACKET_PERIOD);
			#endif
			HYPERGO_T3F_send_data_packet();
			return HYPERGO_T3F_PACKET_PERIOD;
	}
}

void HYPERGO_T3F_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	HYPERGO_T3F_initialize_txid();
	HYPERGO_T3F_RF_init();
	hopping_frequency_no = 0;
	HYPERGO_T3F_bind_channel_no = 0;
	bind_counter = HYPERGO_T3F_BIND_BEACON_COUNT;
	phase = HYPERGO_T3F_BIND;
}

#endif
/* Captured from a T3F / 43308 pair while linked:
Bind address: 6D 6A 58 52 43; bind channels 61/79/43 @ 10ms.
Bind beacon: 7A C8 49 10 00 00 00 00 00 00 00 00 08.
Receiver reply flags: 40, then C0; echoed ID bytes D6 A8 47.
Data address: D6 7E A8 EF 47; data channels 75/64/53 @ 10ms.
Data packet: 7A C8 49 D0 TH ST 3E 80 GYRO D6 A8 47 sum.
  sum = (sum(P[0..11])+0x6D)&0xFF
*/
