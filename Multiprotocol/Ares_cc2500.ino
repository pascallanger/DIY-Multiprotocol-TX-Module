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
// Compatible with ARES 6HPA transmitter

#if defined(ARES_CC2500_INO)

#include "iface_cc2500.h"

//#define ARES_FORCE_ID

#define ARES_COARSE			0

#define ARES_PACKET_LEN			17
#define ARES_NUM_FREQUENCIES	60

enum {
	ARES_START = 0x00,
	ARES_CALIB = 0x01,
	ARES_PREP  = 0x02,
	ARES_DATA  = 0x03,
};

// CC2500 register init values captured from the ARES 6HPA transmitter
const PROGMEM uint8_t ARES_init_values[] = {
  /* 00 */ 0x06, 0x2E, 0x2E, 0x07, 0x5A, 0x60, 0x30, 0x04,
  /* 08 */ 0x05, 0x00, 0x00, 0x06, 0x00, 0x5C, 0xB1, 0x3B + ARES_COARSE,
  /* 10 */ 0x6A, 0xF8, 0x03, 0x23, 0x7A, 0x44, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11
};

// Fixed hopping sequence captured from the ARES 6HPA transmitter.
// This is a permutation of 60 channel values spread across the band.
static const PROGMEM uint8_t ARES_hop[] = {
	0xB0, 0x6F, 0x1D, 0xB4, 0x74, 0x20, 0xB8, 0xD8,
	0x24, 0xBC, 0xDC, 0x28, 0x48, 0xE0, 0x2C, 0x4C,
	0xE4, 0x90, 0x50, 0xE8, 0x94, 0x54, 0xEC, 0x00,
	0x98, 0x58, 0x04, 0x9B, 0x5C, 0x08, 0xA0, 0xC0,
	0x0C, 0xA4, 0xC3, 0x10, 0x30, 0xC6, 0x14, 0x34,
	0xCC, 0x78, 0x38, 0xD0, 0x7C, 0x3C, 0xD4, 0x80,
	0x40, 0x60, 0x84, 0x44, 0x64, 0x88, 0xA8, 0x68,
	0x8C, 0xAC, 0x6C, 0x18
};

static void __attribute__((unused)) ARES_CC2500_init()
{
	CC2500_Strobe(CC2500_SRES);
	delayMilliseconds(1);
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&ARES_init_values[i]));

	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	prev_option = option;

	// Write PATABLE to max power (0xFF for all 8 entries) as captured
	for (uint8_t i = 0; i < 8; i++)
		CC2500_WriteReg(CC2500_3E_PATABLE, 0xFF);

	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

// Load hopping table
static void __attribute__((unused)) ARES_RF_channels()
{
	for (uint8_t i = 0; i < ARES_NUM_FREQUENCIES; i++)
		hopping_frequency[i] = pgm_read_byte_near(&ARES_hop[i]);
}

static void __attribute__((unused)) ARES_tune_chan()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
	CC2500_Strobe(CC2500_SFTX);
	CC2500_Strobe(CC2500_SCAL);
}

static void __attribute__((unused)) ARES_change_chan_fast()
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[hopping_frequency_no]);
}

// Advance the hop counter: cycles through 0-58 with step, inserting 59 when wrapping through 0
static uint8_t __attribute__((unused)) ARES_next_counter(uint8_t current, uint8_t step)
{
	if (current == 59)
		return 0;
	uint8_t next = (current + step) % 59;
	if (next == 0)
		return 59;
	return next;
}

static void __attribute__((unused)) ARES_build_packet()
{
	// Length byte: 16 data bytes follow
	packet[0] = 0x10;

	// TX ID
	packet[1] = rx_tx_addr[1];
	packet[2] = rx_tx_addr[2];
	packet[3] = rx_tx_addr[3];

	// 6 channels encoded as interleaved 12-bit values in bytes 4-12
	uint16_t ch[6];
	for (uint8_t i = 0; i < 6; i++)
		ch[i] = convert_channel_16b_nolimit(i, 1820, 3300, false);

	packet[4]  = ch[0] >> 4;
	packet[5]  = ((ch[0] & 0x0F) << 4) | (ch[1] & 0x0F);
	packet[6]  = ch[1] >> 4;
	packet[7]  = ch[2] >> 4;
	packet[8]  = ((ch[2] & 0x0F) << 4) | (ch[3] & 0x0F);
	packet[9]  = ch[3] >> 4;
	packet[10] = ch[4] >> 4;
	packet[11] = ((ch[4] & 0x0F) << 4) | (ch[5] & 0x0F);
	packet[12] = ch[5] >> 4;

	// Byte 16: counter step size (stored in crc, set to 1-58 in ARES_init)
	uint8_t step = crc;

	// Bytes 13-15: running counter with rotating bit 7 frame indicator
	// The counter cycles 0-58 with a step, inserting 59 before wrapping to 0
	// Each group of 3 packets has 3 consecutive counter values
	// packet_count holds the current counter value
	uint8_t c0 = packet_count;
	uint8_t c1 = ARES_next_counter(c0, step);
	uint8_t c2 = ARES_next_counter(c1, step);

	// Frame indicator: each data frame is sent 3 times
	// bind_phase tracks position 0/1/2 within the group of 3
	packet[13] = c0;
	packet[14] = c1;
	packet[15] = c2;
	packet[16] = step;

	// Set the rotating frame bit (bit 7) on one of bytes 13-15
	switch (bind_phase)
	{
		case 0:
			packet[13] |= 0x80;
			break;
		case 1:
			packet[14] |= 0x80;
			break;
		case 2:
			packet[15] |= 0x80;
			break;
	}
}

static void __attribute__((unused)) ARES_send_packet()
{
	ARES_change_chan_fast();
	CC2500_SetPower();
	CC2500_WriteData(packet, ARES_PACKET_LEN);
}

#define ARES_PACKET_PERIOD	6670	// 6.67ms between packets
#define ARES_PREP_TIMING	2000

uint16_t ARES_callback()
{
	switch(phase)
	{
		case ARES_START:
			ARES_CC2500_init();
			hopping_frequency_no = 0;
			bind_phase = 0;
			ARES_tune_chan();
			phase = ARES_CALIB;
			return ARES_PREP_TIMING;
		case ARES_CALIB:
			calData[hopping_frequency_no] = CC2500_ReadReg(CC2500_25_FSCAL1);
			hopping_frequency_no++;
			if (hopping_frequency_no < ARES_NUM_FREQUENCIES)
				ARES_tune_chan();
			else
			{
				hopping_frequency_no = 0;
				phase = ARES_PREP;
			}
			return ARES_PREP_TIMING;
		case ARES_PREP:
			if (prev_option != option)
			{
				phase = ARES_START;
				return ARES_PREP_TIMING;
			}
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(ARES_PACKET_PERIOD);
			#endif
			ARES_build_packet();
			phase = ARES_DATA;
			// Fall through
		case ARES_DATA:
			ARES_send_packet();
			hopping_frequency_no++;
			if (hopping_frequency_no >= ARES_NUM_FREQUENCIES)
				hopping_frequency_no = 0;
			bind_phase++;
			if (bind_phase >= 3)
			{
				bind_phase = 0;
				// Advance counter to start of next group
				uint8_t step = crc;
				packet_count = ARES_next_counter(packet_count, step);
				packet_count = ARES_next_counter(packet_count, step);
				packet_count = ARES_next_counter(packet_count, step);
			}
			phase = ARES_PREP;
			return ARES_PACKET_PERIOD;
	}
	return 0;
}

void ARES_init()
{
	BIND_DONE;	// Autobind protocol - no TX-initiated bind phase
	ARES_RF_channels();

	// rx_tx_addr[1] and [2] are already set from MProtocol_id by the framework
	// RX_num (0-63) in byte 3 provides model match
	rx_tx_addr[3] = RX_num;

	// Counter step and start from capture
	crc = 23;
	packet_count = 35;

	#ifdef ARES_FORCE_ID
		rx_tx_addr[1] = 0xDC;
		rx_tx_addr[2] = 0xCC;
		rx_tx_addr[3] = 0x00;
	#endif
	phase = ARES_START;
}

#endif
