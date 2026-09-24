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
// Bind: CH79, addr 6D 6A 58 52 43; Data: addr B4 46 F2 25 D7, hop 62/49/75

#if defined(HYPERGO_NRF24L01_INO)

#include "iface_xn297.h"

#define FORCE_HYPERGO_ORIGINAL_ID

#define HYPERGO_PACKET_PERIOD		9870
#define HYPERGO_BIND_PACKET_PERIOD	29570	// ~3 * data period; stock stays on CH79
#define HYPERGO_RF_NUM_CHANNELS		3
#define HYPERGO_PAYLOAD_SIZE		13
#define HYPERGO_BIND_CHANNEL		79
#define HYPERGO_BIND_COUNT			100		// ~3 s of bind beacons
#define HYPERGO_BIND_ID_COUNT		2		// beacons with data-ID echo before handshake

enum {
	HYPERGO_BIND=0,
	HYPERGO_BIND_ID,
	HYPERGO_BIND_SHORT0,
	HYPERGO_BIND_FLAG50,
	HYPERGO_BIND_SHORT40,
	HYPERGO_DATA
};

static void __attribute__((unused)) HYPERGO_send_checksum()
{
	uint8_t sum = 0x6D;
	for (uint8_t i = 0; i < HYPERGO_PAYLOAD_SIZE - 1; i++)
		sum += packet[i];
	packet[12] = sum;
}

static void __attribute__((unused)) HYPERGO_send_data_packet()
{
	XN297_Hopping(hopping_frequency_no);
	hopping_frequency_no++;
	if (hopping_frequency_no >= HYPERGO_RF_NUM_CHANNELS)
		hopping_frequency_no = 0;

	packet[0] = 0x55;
	packet[1] = 0x37;
	packet[2] = 0xE1;
	// Flags: bit0x10=rate 100%, clear=70%; bit0x02=LED
	packet[3] = 0xC0 | GET_FLAG(!CH5_SW, 0x10) | GET_FLAG(CH6_SW, 0x02);
	packet[4] = convert_channel_s8b(CH2);	// Throttle
	packet[5] = convert_channel_s8b(CH1);	// Steering
	packet[6] = (uint8_t)convert_channel_16b_limit(CH7, 0x0E, 0x72);	// ST trim
	packet[7] = convert_channel_s8b(CH3);	// unused on stock T3A
	packet[8] = convert_channel_s8b(CH4);	// unused on stock T3A
	packet[9]  = rx_tx_addr[0];
	packet[10] = rx_tx_addr[2];
	packet[11] = rx_tx_addr[4];
	HYPERGO_send_checksum();

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, HYPERGO_PAYLOAD_SIZE);
}

// Bind beacon / handshake on fixed CH79 + bind address
static void __attribute__((unused)) HYPERGO_send_bind_packet(uint8_t flag, bool echo_data_id)
{
	XN297_RFChannel(HYPERGO_BIND_CHANNEL);

	packet[0] = 0x55;
	packet[1] = 0x37;
	packet[2] = 0xE1;
	packet[3] = flag;
	memcpy(&packet[4], (uint8_t*)"\x6D\x6A\x58\x52\x43", 5);	// bind ID
	if (echo_data_id)
	{
		packet[9]  = rx_tx_addr[0];	// B4
		packet[10] = rx_tx_addr[2];	// F2
		packet[11] = rx_tx_addr[4];	// D7
	}
	else
	{
		packet[9]  = 0x00;
		packet[10] = 0x00;
		packet[11] = 0x00;
	}
	HYPERGO_send_checksum();

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, HYPERGO_PAYLOAD_SIZE);
}

static void __attribute__((unused)) HYPERGO_send_bind_short(uint8_t flag, uint8_t len)
{
	XN297_RFChannel(HYPERGO_BIND_CHANNEL);
	packet[0] = 0x55;
	packet[1] = 0x37;
	packet[2] = 0xE1;
	packet[3] = flag;
	if (len >= 5)
		packet[4] = 0x00;
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, len);
}

static void __attribute__((unused)) HYPERGO_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\x6D\x6A\x58\x52\x43", 5);	// bind address
	XN297_RFChannel(HYPERGO_BIND_CHANNEL);
	XN297_HoppingCalib(HYPERGO_RF_NUM_CHANNELS);			// data hops
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
	switch (phase)
	{
		case HYPERGO_BIND:
			HYPERGO_send_bind_packet(0x10, false);
			if (--bind_counter == 0)
			{
				bind_counter = HYPERGO_BIND_ID_COUNT;
				phase = HYPERGO_BIND_ID;
			}
			return HYPERGO_BIND_PACKET_PERIOD;

		case HYPERGO_BIND_ID:
			// Stock: still on bind addr, echo data ID odd bytes
			HYPERGO_send_bind_packet(0x10, true);
			if (--bind_counter == 0)
				phase = HYPERGO_BIND_SHORT0;
			return HYPERGO_BIND_PACKET_PERIOD;

		case HYPERGO_BIND_SHORT0:
			HYPERGO_send_bind_short(0x00, 5);	// P(5)= 55 37 E1 00 00
			phase = HYPERGO_BIND_FLAG50;
			return 3000;

		case HYPERGO_BIND_FLAG50:
			HYPERGO_send_bind_packet(0x50, true);
			phase = HYPERGO_BIND_SHORT40;
			return 27000;

		case HYPERGO_BIND_SHORT40:
			HYPERGO_send_bind_short(0x40, 4);	// P(4)= 55 37 E1 40
			XN297_SetTXAddr(rx_tx_addr, 5);		// switch to data address
			BIND_DONE;
			phase = HYPERGO_DATA;
			hopping_frequency_no = 0;
			return 3000;

		default:	// HYPERGO_DATA
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(HYPERGO_PACKET_PERIOD);
			#endif
			HYPERGO_send_data_packet();
			return HYPERGO_PACKET_PERIOD;
	}
}

void HYPERGO_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	HYPERGO_initialize_txid();
	HYPERGO_RF_init();
	hopping_frequency_no = 0;
	bind_counter = HYPERGO_BIND_COUNT;
	phase = HYPERGO_BIND;
}

#endif
/* Bind / data (captured T3A + R30, 2026-09-24)
Bind beacon (R30 off): C=79 A=6D 6A 58 52 43 period~29570us
  P(13)= 55 37 E1 10 6D 6A 58 52 43 00 00 00 AE
Handshake (R30 on):
  P(13)= 55 37 E1 10 6D 6A 58 52 43 B4 F2 D7 2B
  P(5) = 55 37 E1 00 00
  P(13)= 55 37 E1 50 6D 6A 58 52 43 B4 F2 D7 6B
  P(4) = 55 37 E1 40
Data: A=B4 46 F2 25 D7 hop 62→49→75 @9870us
  P(13)= 55 37 E1 D0/C0 TH ST trim ch3 ch4 B4 F2 D7 sum
  sum = (sum(P[0..11])+0x6D)&0xFF
*/
