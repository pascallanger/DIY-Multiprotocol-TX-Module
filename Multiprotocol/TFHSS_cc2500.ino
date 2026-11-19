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
// Futaba T-FHSS (Surface: cars, boats) and T-FHSS Air, sub protocols of
// Futaba (Futaba_cc2500), on the CC2500 as Futaba's own radios (their RF is
// a CC2500 too). From the R334SBS receiver's and the T18SZ transmitter's
// firmware, checked against captures of a 4PM Plus (Surface, SR) and a T6K
// (Air).
//
// Both: 31 channels n = 1..31 at 2407.5 + 2 (n - 1) MHz (CHANNR 5 (n - 1)),
// sync D391, hop rows of a Welch-Costas array (row r: row 0 + r), the next
// channel the next of the row, one TX packet per frame. The packets XOR a
// key made of the TX ID and the channel, then the CRC-16/MCRF4XX of the
// plain bytes. The receiver takes the row from the packets.
//
// Surface (TFHSS): 384 kbps MSK, 14 bytes, a frame every 3 ms; 4 channels
// of 12 bits ((v + 2816) x 0.3125 us: 2048 = 1520 us) and an extra one in
// turn (ch5..8: a guess, the 4PM Plus sends none); the receiver's reply
// 1.9 ms after the TX packet on the next channel (S.BUS2 slots, 4 per
// reply). SR (TFHSS_SR, "Super Response"): 1.2 ms frames, no telemetry,
// the receiver bound in SR mode. Bind (link), at bind power: plain
// packets (the TX ID and the key's 4 other bytes) on the 16-channel list,
// one per 3 ms (the 4PM Plus: channel 1 every 48 ms), until the receiver
// in its link mode answers (type 24 / 25, on the same channel); then
// TFHSS_BIND_ACKED more with byte 1 bit 7, which make the receiver store
// the bind (R334SBS firmware). Failsafe: every 5 s a group of 4 frames, one
// per channel: its F/S position, or hold (the 4PM Plus' default).
// The per channel "high speed" flags (760 us pulses) are left off.
//
// Air (TFHSS_AIR): 128 kbps 2-FSK (the S-FHSS modem), 17 bytes, a frame
// every 15.04 ms; 16 channels in 9 slots of 11 bits (us - 880), the
// channel halves in turn. Bind (link): plain link frames on a 16-channel
// list; the receiver answers with its ID, which keys its telemetry (8
// S.BUS2 slots per reply, 6.3 ms after the TX packet on the next channel).
// Without a bind since the start, the receiver's ID is found in its
// replies. No failsafe frames yet (the T6K's not fully decoded): the
// receiver's own failsafe applies.
//
// 128KB STM32 only (FUTABA_TFHSS, Validate.h): too large for the ATmega328p
// and the 64KB STM32.
//
// Telemetry (FUTABA_FW_TELEMETRY): the link frame (A1 = the receiver's
// voltage at the 13.2 V ratio, RSSI = its link quality (Surface), TX_RSSI
// = its replies here), then the S.BUS2 slots as the sensors send them
// (MULTI_TELEMETRY_FUTABA: the first slot, the reply's RSSI in dBm, the
// link quality % or 0xFF, 2 bytes per slot).

#if defined(FUTABA_TFHSS)

#include "iface_cc2500.h"

#define TFHSS_CHANNELS			31
#define TFHSS_LIST				16
#define TFHSS_PERIOD			3000
#define TFHSS_SR_PERIOD			1200
#define TFHSS_RX_ON				1000	// from the TX packet's start: RX on the next channel
#define TFHSS_RX_READ			2500	// the reply (1.9 ms after the TX packet) read
#define TFHSS_LEN				14
#define TFHSS_FS_EVERY			1667	// frames, 5 s
#define TFHSS_SR_FS_EVERY		4167
#define TFHSS_BIND_FRAMES		3200	// 9.6 s without a reply (the 4PM Plus' 9.1 s)
#define TFHSS_BIND_ACKED		240		// after the reply (the 4PM Plus: 720 ms)
#define TFHSS_TELEM_EVERY		5		// replies to the radio: 1 in 5 (all groups in turn)
#define TFHSS_AIR_PERIOD		15040
#define TFHSS_AIR_RX_ON			2000
#define TFHSS_AIR_RX_READ		9500
#define TFHSS_AIR_LEN			17
#define TFHSS_AIR_REPLY_LEN		20
#define TFHSS_AIR_LINK_FRAMES	1330	// 20 s without a reply
#define TFHSS_AIR_LINK_AFTER	50		// link frames with the receiver's ID after its reply
#define TFHSS_AIR_T_EVERY		9		// the "telemetry on" bit, as the T6K
#define TFHSS_ID_MISSES			16		// Air: replies in a row with a bad CRC, the ID forgotten

enum {
	TFHSS_CAL = 0,
	TFHSS_DATA,
	TFHSS_RX,
	TFHSS_READ,
};

// Row 0: 31, then 3^i mod 31 (i = 0..29)
const PROGMEM uint8_t TFHSS_row0[TFHSS_CHANNELS] = {
	31, 1, 3, 9, 27, 19, 26, 16, 17, 20, 29, 25, 13, 8, 24, 10,
	30, 28, 22, 4, 12, 5, 15, 14, 11, 2, 6, 18, 23, 7, 21 };
// Air's link list
const PROGMEM uint8_t TFHSS_list[TFHSS_LIST] = {
	1, 25, 3, 22, 4, 21, 6, 19, 7, 18, 9, 16, 10, 15, 12, 13 };

// Surface: 384 kbps MSK, 14 bytes, no CC2500 CRC (the R334SBS' setup)
const PROGMEM uint8_t TFHSS_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, TFHSS_LEN, 0x04,
  /* 08 */ 0x00, 0x00, 0x00, 0x0A, 0x00, 0x5C, 0x98, 0x9D,
  /* 10 */ 0x1D, 0xE4, 0x72, 0x23, 0xF8, 0x00, 0x07, 0x00,
  /* 18 */ 0x08, 0x1D, 0x1C, 0x1F, 0x40, 0xB0, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x00, 0x19
};
// Air: 128 kbps 2-FSK 38 kHz, 17 bytes (the T18SZ's), FOCCFG 1E: the
// receiver's replies up to +-58 kHz off
const PROGMEM uint8_t TFHSS_AIR_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x2F, 0x07, 0xD3, 0x91, TFHSS_AIR_LEN, 0x04,
  /* 08 */ 0x00, 0x00, 0x00, 0x1F, 0x00, 0x5C, 0x98, 0x9D,
  /* 10 */ 0x7C, 0x43, 0x02, 0x23, 0xF8, 0x44, 0x07, 0x0C,
  /* 18 */ 0x08, 0x1E, 0x1C, 0x43, 0x40, 0x91, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x11, 0x11
};

static uint8_t TFHSS_id[4];		// TX ID, little endian
static uint8_t TFHSS_b[4];		// Surface: the key's other 4 bytes
static uint8_t TFHSS_rx_id[4];	// Air: the receiver's ID
static bool TFHSS_rx_known;
static uint8_t TFHSS_misses;
static uint8_t TFHSS_row;
static bool TFHSS_list_mode;	// bind (link): hopping on the list
static uint8_t TFHSS_ch;		// the channel of the next TX packet
static uint16_t TFHSS_frame;
static uint16_t TFHSS_link;		// link frames left (after the reply) or sent
static uint8_t TFHSS_replies;

static uint16_t __attribute__((unused)) TFHSS_crc(const uint8_t *p, uint8_t len)
{
	uint16_t c = 0xFFFF;
	while(len--)
	{
		c ^= *p++;
		for(uint8_t i = 0; i < 8; i++)
			c = (c & 1) ? (c >> 1) ^ 0x8408 : c >> 1;
	}
	return c;
}

// Key byte i on channel n. Surface: 00 00 a0..a3 x n b0..b3 (TX: a = the TX
// ID, b = TFHSS_b; replies swapped). Air: 00 a0..a3 x n a0..a3 x n a0..a3 x
// (TX: the TX ID; replies: the receiver's). x = 0
static uint8_t __attribute__((unused)) TFHSS_key(uint8_t i, uint8_t n, const uint8_t *a, const uint8_t *b)
{
	if(i == 0)
		return 0;
	if(sub_protocol != TFHSS_AIR)
	{
		if(i == 1 || i == 6) return 0;
		if(i < 6) return a[i - 2];
		if(i == 7) return n;
		return b[i - 8];
	}
	uint8_t m = (i - 1) % 6;
	return m < 4 ? a[m] : m == 4 ? 0 : n;
}

// packet[0..len-3] plain -> keyed (or not), the CRC appended, sent
static void __attribute__((unused)) TFHSS_send(uint8_t len, bool keyed)
{
	uint8_t data = len - 2;
	uint16_t c = TFHSS_crc(packet, data);
	if(keyed)
		for(uint8_t i = 1; i < data; i++)
			packet[i] ^= TFHSS_key(i, TFHSS_ch, TFHSS_id, TFHSS_b);
	packet[data] = c;
	packet[data + 1] = c >> 8;
	CC2500_SetTxRxMode(TX_EN);
	CC2500_WriteReg(CC2500_06_PKTLEN, len);
	CC2500_WriteData(packet, len);
}

// in (len bytes) -> out plain: CRC ok?
static bool __attribute__((unused)) TFHSS_unkey(const uint8_t *in, uint8_t len, uint8_t n, const uint8_t *a, const uint8_t *b, uint8_t *out)
{
	uint8_t data = len - 2;
	for(uint8_t i = 0; i < data; i++)
		out[i] = a ? in[i] ^ TFHSS_key(i, n, a, b) : in[i];
	return TFHSS_crc(out, data) == (in[data] | in[data + 1] << 8);
}

static uint8_t __attribute__((unused)) TFHSS_channel(uint8_t pos)
{
	if(TFHSS_list_mode)
		return pgm_read_byte_near(&TFHSS_list[pos % TFHSS_LIST]);
	return (pgm_read_byte_near(&TFHSS_row0[pos % TFHSS_CHANNELS]) + TFHSS_row - 1) % TFHSS_CHANNELS + 1;
}

static void __attribute__((unused)) TFHSS_next()
{
	hopping_frequency_no++;
	if(hopping_frequency_no >= (TFHSS_list_mode ? TFHSS_LIST : TFHSS_CHANNELS))
		hopping_frequency_no = 0;
	TFHSS_ch = TFHSS_channel(hopping_frequency_no);
}

static void __attribute__((unused)) TFHSS_tune(uint8_t n)
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_0A_CHANNR, 5 * (n - 1));
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[n - 1]);
}

// RX on the next channel for len bytes
static void __attribute__((unused)) TFHSS_rx_on(uint8_t len)
{
	TFHSS_tune(TFHSS_ch);
	CC2500_WriteReg(CC2500_06_PKTLEN, len);
	CC2500_SetTxRxMode(RX_EN);
	CC2500_Strobe(CC2500_SFRX);
	CC2500_Strobe(CC2500_SRX);
}

// The reply in packet_in (len bytes + RSSI + LQI), or false
static bool __attribute__((unused)) TFHSS_rx_read(uint8_t len)
{
	uint8_t n = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
	bool ok = n >= len + 2 && len + 2 <= TELEMETRY_BUFFER_SIZE;
	if(ok)
		CC2500_ReadData(packet_in, len + 2);
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_Strobe(CC2500_SFRX);
	return ok;
}

// A reply to the radio: n slots from first (lq 0xFF: none), every
// TFHSS_TELEM_EVERY-th reply (Surface's 333 a second would fill the link)
static void __attribute__((unused)) TFHSS_telemetry(uint8_t first, const uint8_t *slots, uint8_t n, uint8_t lq, uint8_t rssi, uint8_t lqi)
{
	#ifdef FUTABA_FW_TELEMETRY
		if(sub_protocol != TFHSS_AIR && ++TFHSS_replies < TFHSS_TELEM_EVERY)
			return;
		TFHSS_replies = 0;
		// The link frame: slot 0 half 0 = the receiver's voltage (0.1 V)
		if(first == 0 && !(slots[0] & 0x04))
			v_lipo1 = (uint16_t)slots[1] * 255 / 132;
		if(lq != 0xFF)
			RX_RSSI = lq;
		TX_RSSI = rssi >= 128 ? rssi - 128 : rssi + 128;
		TX_LQI = lqi & 0x7F;
		telemetry_link |= 1;
		#ifdef MULTI_TELEMETRY
			futaba_telem[0] = first;
			futaba_telem[1] = (uint8_t)(((int8_t)rssi) / 2 - 72);	// dBm
			futaba_telem[2] = lq;
			memcpy(futaba_telem + 3, slots, 2 * n);
			futaba_telem_len = 3 + 2 * n;
			telemetry_link |= 2;		// after the link frame
		#endif
	#else
		(void)first; (void)slots; (void)n; (void)lq; (void)rssi; (void)lqi;
	#endif
}

// Surface: the 12-bit value of a channel, 1020..2020 us at +-100 %
static uint16_t __attribute__((unused)) TFHSS_value(uint8_t num, bool failsafe)
{
	int16_t v = convert_channel_16b_nolimit(CH_AETR[num], 448, 3648, failsafe);
	return v < 0 ? 0 : v > 4095 ? 4095 : v;
}

static void __attribute__((unused)) TFHSS_surface_bind_packet()
{
	bool sr = sub_protocol == TFHSS_SR;
	// Bit 7: the receiver's reply heard
	packet[0] = (sr ? 9 : 8) | (TFHSS_row & 7) << 5;
	packet[1] = TFHSS_row >> 3 | (sr ? 0x10 : 0) | (TFHSS_rx_known ? 0x80 : 0);
	memcpy(packet + 2, TFHSS_id, 4);
	packet[6] = 0;
	packet[7] = 1;
	// SR: without b5..b8 until the reply (as the 4PM Plus)
	if(sr && !TFHSS_rx_known)
		memset(packet + 8, 0, 4);
	else
		memcpy(packet + 8, TFHSS_b, 4);
	TFHSS_send(TFHSS_LEN, false);
}

static void __attribute__((unused)) TFHSS_surface_packet()
{
	bool sr = sub_protocol == TFHSS_SR;
	uint16_t ch[4];
	for(uint8_t i = 0; i < 4; i++)
		ch[i] = TFHSS_value(i, false);
	uint8_t f = 0x13, fs = 0;	// 0x13: a normal frame
	uint16_t k = TFHSS_frame % (sr ? TFHSS_SR_FS_EVERY : TFHSS_FS_EVERY);
	if(k < 4)
	{	// Failsafe group: ch D, C, B, A (the 4PM Plus' order); code 0 + the
		// group flag: hold, 4: F/S with this frame's value as the position
		static const uint8_t ch_code[4] = { 0x12, 0x02, 0x01, 0x00 };
		uint8_t c = 3 - k;
		f = ch_code[k];
		fs = 0x10;
		#ifdef FAILSAFE_ENABLE
			uint16_t v = Failsafe_data[CH_AETR[c]];
			if(IS_FAILSAFE_VALUES_on && v != FAILSAFE_CHANNEL_HOLD && v != FAILSAFE_CHANNEL_NOPULSES)
			{
				f |= 0x04;
				ch[c] = TFHSS_value(c, true);
			}
		#endif
	}
	uint8_t slot = TFHSS_frame & 3;
	uint16_t e = TFHSS_value(4 + slot, false);
	packet[0] = (sr ? 1 : 0) | (TFHSS_row & 7) << 5;
	packet[1] = TFHSS_row >> 3 | (sr ? 0 : 0x10) | (f & 3) << 6;
	packet[2] = (f >> 2) | (ch[0] & 0x0F) << 4;
	packet[3] = ch[0] >> 4;
	packet[4] = ch[1];
	packet[5] = (ch[1] >> 8) | (ch[2] & 0x0F) << 4;
	packet[6] = ch[2] >> 4;
	packet[7] = ch[3];
	packet[8] = (ch[3] >> 8) | 0x10;
	packet[9] = fs;		// bits 0..3: the high speed flags
	packet[10] = e;
	packet[11] = (e >> 8) | slot << 4;
	TFHSS_send(TFHSS_LEN, true);
}

// Surface reply: type 16 = 4 S.BUS2 slots of group byte 1 >> 4; byte 10
// bit 6: byte 11 is the link quality % (0) or something else (1)
static void __attribute__((unused)) TFHSS_surface_reply(uint8_t n)
{
	uint8_t pl[TFHSS_LEN - 2];
	if(!TFHSS_unkey(packet_in, TFHSS_LEN, n, TFHSS_b, TFHSS_id, pl) || (pl[0] & 0x1F) != 16)
		return;
	TFHSS_telemetry(4 * ((pl[1] >> 4) & 7), pl + 2, 4, pl[10] & 0x40 ? 0xFF : pl[11],
		packet_in[TFHSS_LEN], packet_in[TFHSS_LEN + 1]);
}

// The receiver's bind reply: type 24 / 25, our ID, keyed as its telemetry
// with the b5..b8 it got (SR: 0 until the reply), or plain
static void __attribute__((unused)) TFHSS_surface_bind_reply(uint8_t n)
{
	static const uint8_t zero[4] = { 0, 0, 0, 0 };
	uint8_t pl[TFHSS_LEN - 2];
	const uint8_t *b = sub_protocol == TFHSS_SR && !TFHSS_rx_known ? zero : TFHSS_b;
	if((TFHSS_unkey(packet_in, TFHSS_LEN, n, b, TFHSS_id, pl) || TFHSS_unkey(packet_in, TFHSS_LEN, n, NULL, NULL, pl))
		&& (pl[0] & 0x1E) == 24 && !memcmp(pl + 2, TFHSS_id, 4) && !TFHSS_rx_known)
	{
		TFHSS_rx_known = true;
		TFHSS_link = TFHSS_BIND_ACKED;
	}
}

// Bind (link) frame: over (the radio ended the bind, the frames after the
// reply sent, or none in frames) -> back to the row
static bool __attribute__((unused)) TFHSS_link_over(uint16_t frames)
{
	if(!IS_BIND_DONE && (TFHSS_rx_known ? TFHSS_link-- != 0 : ++TFHSS_link <= frames))
		return false;
	TFHSS_list_mode = false;
	hopping_frequency_no = 0;
	TFHSS_ch = TFHSS_channel(0);
	BIND_DONE;
	return true;
}

// Air: the 11-bit value of a channel, us - 880
static uint16_t __attribute__((unused)) TFHSS_air_value(uint8_t num)
{
	int16_t v = convert_channel_16b_nolimit(CH_AETR[num], 140, 1140, false);
	return v < 0 ? 0 : v > 2047 ? 2047 : v;
}

static void __attribute__((unused)) TFHSS_air_packet()
{
	// Slots 0..5: ch1..6, or ch11..16 (flags b14 bits 0..5); 6, 7: ch7, ch8;
	// 8: ch9, or ch10 (b13 bit 3)
	bool half = TFHSS_frame & 1;
	memset(packet, 0, TFHSS_AIR_LEN);
	for(uint8_t i = 0, bit = 0; i < 9; i++)
	{
		uint8_t num = i < 6 ? (half ? i + 10 : i) : i < 8 ? i : (half ? 9 : 8);
		uint16_t v = TFHSS_air_value(num);
		for(uint8_t j = 0; j < 11; j++, bit++)
			if(v >> j & 1)
				packet[1 + bit / 8] |= 1 << (bit & 7);
	}
	packet[0] = TFHSS_row << 3;
	if(IS_DISABLE_TELEM_off && TFHSS_frame % TFHSS_AIR_T_EVERY == 0)
		packet[0] |= 0x04;
	if(half)
	{
		packet[13] |= 0x08;
		packet[14] = 0x3F;
	}
	TFHSS_send(TFHSS_AIR_LEN, true);
}

// Link frame (plain): the TX ID, the channel, the receiver's ID once known
static void __attribute__((unused)) TFHSS_air_link_packet()
{
	memset(packet, 0, TFHSS_AIR_LEN);
	packet[0] = TFHSS_rx_known ? 0xFC : 0xF8;
	memcpy(packet + 1, TFHSS_id, 4);
	packet[6] = TFHSS_ch;
	if(TFHSS_rx_known)
		memcpy(packet + 7, TFHSS_rx_id, 4);
	TFHSS_send(TFHSS_AIR_LEN, false);
}

// The receiver's link reply (plain, 17 bytes): our ID echoed, its own
static void __attribute__((unused)) TFHSS_air_link_reply()
{
	uint8_t pl[TFHSS_AIR_LEN - 2];
	if(!TFHSS_unkey(packet_in, TFHSS_AIR_LEN, 0, NULL, NULL, pl) || memcmp(pl + 1, TFHSS_id, 4))
		return;
	memcpy(TFHSS_rx_id, pl + 7, 4);
	if(!TFHSS_rx_known)
		TFHSS_link = TFHSS_AIR_LINK_AFTER;
	TFHSS_rx_known = true;
	TFHSS_misses = 0;
}

// Air telemetry: type 3 = 8 S.BUS2 slots of group byte 17 & 15, keyed with
// the receiver's ID. Not known: tried from the bytes of slots 3-4 and 6-7
// (the key itself when no sensor is there)
static void __attribute__((unused)) TFHSS_air_reply(uint8_t n)
{
	uint8_t pl[TFHSS_AIR_REPLY_LEN - 2];
	bool ok = TFHSS_rx_known && TFHSS_unkey(packet_in, TFHSS_AIR_REPLY_LEN, n, TFHSS_rx_id, NULL, pl);
	for(uint8_t o = 7; !TFHSS_rx_known && o <= 13; o += 6)
	{
		memcpy(TFHSS_rx_id, packet_in + o, 4);
		ok = TFHSS_rx_known = TFHSS_unkey(packet_in, TFHSS_AIR_REPLY_LEN, n, TFHSS_rx_id, NULL, pl);
	}
	if(!ok)
	{
		if(TFHSS_rx_known && ++TFHSS_misses >= TFHSS_ID_MISSES)
			TFHSS_rx_known = false;
		return;
	}
	TFHSS_misses = 0;
	if((pl[0] & 0x03) == 3 && (pl[17] & 0x0F) < 4)
		TFHSS_telemetry(8 * (pl[17] & 0x0F), pl + 1, 8, 0xFF,
			packet_in[TFHSS_AIR_REPLY_LEN], packet_in[TFHSS_AIR_REPLY_LEN + 1]);
}

static uint16_t __attribute__((unused)) TFHSS_air_callback()
{
	switch(phase)
	{
		case TFHSS_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(TFHSS_AIR_PERIOD);
			#endif
			CC2500_SetFreqOffset();
			CC2500_SetPower();
			TFHSS_tune(TFHSS_ch);
			if(TFHSS_list_mode)
			{	// Link: until the reply and TFHSS_AIR_LINK_AFTER frames
				if(TFHSS_link_over(TFHSS_AIR_LINK_FRAMES))
					return TFHSS_AIR_PERIOD;
				TFHSS_air_link_packet();
			}
			else
				TFHSS_air_packet();
			TFHSS_frame++;
			TFHSS_next();
			phase = TFHSS_RX;
			return TFHSS_AIR_RX_ON;
		case TFHSS_RX:
			TFHSS_rx_on(TFHSS_list_mode ? TFHSS_AIR_LEN : TFHSS_AIR_REPLY_LEN);
			phase = TFHSS_READ;
			return TFHSS_AIR_RX_READ - TFHSS_AIR_RX_ON;
		case TFHSS_READ:
			if(TFHSS_list_mode)
			{
				if(TFHSS_rx_read(TFHSS_AIR_LEN))
					TFHSS_air_link_reply();
			}
			else if(TFHSS_rx_read(TFHSS_AIR_REPLY_LEN))
				TFHSS_air_reply(TFHSS_ch);
			phase = TFHSS_DATA;
			return TFHSS_AIR_PERIOD - TFHSS_AIR_RX_READ;
	}
	return TFHSS_AIR_PERIOD;
}

static uint16_t __attribute__((unused)) TFHSS_callback()
{
	if(phase == TFHSS_CAL)
	{	// Calibration of the 31 channels, one per call
		if(rf_ch_num)
			calData[rf_ch_num - 1] = CC2500_ReadReg(CC2500_25_FSCAL1);
		if(rf_ch_num < TFHSS_CHANNELS)
		{
			rf_ch_num++;
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteReg(CC2500_0A_CHANNR, 5 * (rf_ch_num - 1));
			CC2500_Strobe(CC2500_SCAL);
			return 1000;
		}
		phase = TFHSS_DATA;
		return 1000;
	}
	if(sub_protocol == TFHSS_AIR)
		return TFHSS_air_callback();

	bool sr = sub_protocol == TFHSS_SR;
	switch(phase)
	{
		case TFHSS_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(sr ? TFHSS_SR_PERIOD : TFHSS_PERIOD);
			#endif
			CC2500_SetFreqOffset();
			CC2500_SetPower();
			TFHSS_tune(TFHSS_ch);
			if(TFHSS_list_mode)
			{	// Bind: 3 ms frames, the reply on the same channel
				if(TFHSS_link_over(TFHSS_BIND_FRAMES))
					return TFHSS_PERIOD;
				TFHSS_surface_bind_packet();
				phase = TFHSS_RX;
				return TFHSS_RX_ON;
			}
			TFHSS_surface_packet();
			TFHSS_frame++;
			TFHSS_next();
			if(sr || IS_DISABLE_TELEM_on)
				return sr ? TFHSS_SR_PERIOD : TFHSS_PERIOD;
			phase = TFHSS_RX;
			return TFHSS_RX_ON;
		case TFHSS_RX:
			TFHSS_rx_on(TFHSS_LEN);
			phase = TFHSS_READ;
			return TFHSS_RX_READ - TFHSS_RX_ON;
		case TFHSS_READ:
			if(TFHSS_list_mode)
			{
				if(TFHSS_rx_read(TFHSS_LEN))
					TFHSS_surface_bind_reply(TFHSS_ch);
				TFHSS_next();
			}
			else if(TFHSS_rx_read(TFHSS_LEN))
				TFHSS_surface_reply(TFHSS_ch);
			phase = TFHSS_DATA;
			return TFHSS_PERIOD - TFHSS_RX_READ;
	}
	return TFHSS_PERIOD;
}

static void __attribute__((unused)) TFHSS_init()
{
	// The TX ID: 9 decimal digits (as Futaba's), little endian; the
	// Surface key's other 4 bytes another such number
	uint32_t id = 100000000 + MProtocol_id % 900000000;
	uint32_t b = 100000000 + (MProtocol_id ^ 0x5A5A5A5A) % 900000000;
	for(uint8_t i = 0; i < 4; i++)
	{
		TFHSS_id[i] = id >> (8 * i);
		TFHSS_b[i] = b >> (8 * i);
	}
	TFHSS_row = id % TFHSS_CHANNELS;
	TFHSS_rx_known = false;
	TFHSS_misses = 0;
	TFHSS_frame = 0;
	TFHSS_replies = 0;
	TFHSS_link = 0;
	TFHSS_list_mode = IS_BIND_IN_PROGRESS;
	hopping_frequency_no = 0;
	TFHSS_ch = TFHSS_channel(0);

	const uint8_t *regs = sub_protocol == TFHSS_AIR ? TFHSS_AIR_init_values : TFHSS_init_values;
	CC2500_Strobe(CC2500_SIDLE);
	for(uint8_t i = 0; i < 0x27; i++)
		CC2500_WriteReg(i, pgm_read_byte_near(&regs[i]));
	CC2500_WriteReg(CC2500_2C_TEST2, 0x88);
	CC2500_WriteReg(CC2500_2D_TEST1, 0x31);
	CC2500_WriteReg(CC2500_2E_TEST0, 0x0B);
	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();

	rf_ch_num = 0;
	phase = TFHSS_CAL;
}

// Futaba (21): S-FHSS (Futaba_cc2500.ino) or T-FHSS
void SFHSS_init();
uint16_t SFHSS_callback();

uint16_t FUTABA_callback()
{
	return sub_protocol == SFHSS ? SFHSS_callback() : TFHSS_callback();
}

void FUTABA_init()
{
	if(sub_protocol == SFHSS)
		SFHSS_init();
	else
		TFHSS_init();
}

#endif
