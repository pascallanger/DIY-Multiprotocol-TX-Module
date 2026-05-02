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
// Ao-Sen-Ma CG022 quad copter protocol via external LT8910 RF board
// Protocol decoded from stock CG022 TX SPI captures
//
// Hardware wiring:
//   LT8910 RF board connects to MPM breakout pins:
//   - MOSI (PB15) → LT8910 MOSI
//   - MISO (PB14) → LT8910 MISO  
//   - SCK  (PB13) → LT8910 SCK
//   - CS   (PA15/SPI_CSN) → LT8910 CS
//   - 3.3V, GND
//   - RESET (PA14/LT8910_RST) → LT8910 RET pin
//
// PA14 (SWCLK) is available after afio_cfg_debug_ports(AFIO_DEBUG_NONE).

#if defined(CG022_LT8910_INO)

#include "iface_lt8910.h"

#define CG022_PACKET_PERIOD		2310	// ~2310us per hop (from captures)
#define CG022_BIND_COUNT		166		// ~0.38s of bind packets (capture 02 with RX present)
#define CG022_PAYLOAD_SIZE		10		// 10 bytes including length prefix
#define CG022_NUM_CHANNELS		8

// Channel hopping sequence from stock TX capture
static const uint8_t CG022_hop_channels[CG022_NUM_CHANNELS] = {
	0, 40, 10, 50, 20, 60, 30, 70
};

// Stock TX register initialization values from capture 02b
// Format: { register_address (byte), value_high (byte), value_low (byte) }
static const uint8_t PROGMEM CG022_init_regs[] = {
	0x00, 0x6F, 0xE0,	// Chip ID (stock TX writes this during init)
	0x01, 0x56, 0x81,
	0x02, 0x66, 0x17,
	0x04, 0x9C, 0xC9,	// RSSI
	0x05, 0x66, 0x37,
	0x07, 0x00, 0x00,	// Channel=0, TX off
	0x08, 0x6C, 0x90,
	0x09, 0x48, 0x00,	// PA control
	0x0A, 0x7F, 0xFD,
	0x0B, 0x00, 0x08,
	0x0C, 0x00, 0x00,
	0x0D, 0x48, 0xBD,
	0x16, 0x00, 0xFF,	// RX/PLL
	0x17, 0x80, 0x05,	// CRC polynomial
	0x18, 0x00, 0x67,	// Preamble (3 bytes)
	0x19, 0x16, 0x59,
	0x1A, 0x19, 0xE0,
	0x1B, 0x13, 0x00,
	0x1C, 0x18, 0x00,
	0x20, 0x48, 0x00,	// Sync word 1
	0x21, 0x3F, 0xC7,	// Sync word 2
	0x22, 0x20, 0x00,	// Sync word 3
	0x23, 0x03, 0x00,	// Packet config: CRC_INITIAL_DATA_EN + CRC_ON
	0x24, 0x22, 0x11,	// Sync word 4
	0x25, 0x06, 0x8C,	// Sync word 5
	0x26, 0x5A, 0x5A,	// Sync word 6
	0x27, 0x00, 0x33,	// Sync word 7
	0x28, 0x44, 0x02,	// CRC seed
	0x29, 0xB0, 0x00,	// TX power level 11/15
	0x2A, 0xFD, 0xB0,
	0x2B, 0x00, 0x0F,
};
#define CG022_NUM_INIT_REGS (sizeof(CG022_init_regs) / 3)

static bool cg022_first_packet = true;
static bool cg022_post_bind_sync = false;
static uint8_t cg022_txid[3] = { 0x00, 0x00, 0x00 };

static void __attribute__((unused)) CG022_restart_bind_sequence()
{
	// Restore bind-time sync words and channel before sending bind packets again.
	LT8910_WriteReg(LT8910_REG_CHANNEL, 0x0000);
	LT8910_WriteReg(LT8910_REG_SYNCWORD4, (uint16_t)(rx_tx_addr[1] << 8) | rx_tx_addr[0]);
	LT8910_WriteReg(LT8910_REG_SYNCWORD7, (uint16_t)rx_tx_addr[2]);
	LT8910_WriteReg(LT8910_REG_FIFO, 0x0000);
	bind_counter = CG022_BIND_COUNT;
	cg022_first_packet = true;
	cg022_post_bind_sync = false;
	hopping_frequency_no = 0;
}

static void __attribute__((unused)) CG022_prepare_hardware()
{
	// Deselect all 4-in-1 onboard chips before using the external LT8910.
	// MOSI/MISO/SCK are shared; any chip with CS low would drive the bus.
	#ifdef CC25_CSN_pin
		CC25_CSN_on;
	#endif
	#ifdef NRF_CSN_pin
		NRF_CSN_on;
	#endif
	#ifdef A7105_CSN_pin
		A7105_CSN_on;
	#endif
	#ifdef CYRF_CSN_pin
		CYRF_CSN_on;
	#endif

	// Switch SPI2 to Mode 1 (CPOL=0, CPHA=1) for LT8900/LT8910.
	// The LT8910 samples MOSI on the falling SCK edge (Mode 1).
	// initSPI2() sets Mode 0 which shifts every byte by 1 bit.
	// Set SPI clock to /256 (~140kHz) to match stock TX's ~114kHz bit-bang SPI.
	// At /256, each 3-byte SPI transaction takes ~196µs (stock TX: ~223µs),
	// naturally providing the inter-write timing the LT8910 needs without
	// explicit delays.  Faster clocks (e.g. /16 = 2.25MHz) produce 14µs
	// transactions that are too fast for the LT8910 to process (MISO stays
	// 0xFF = chip not responding).
	SPI_DISABLE();
	SPI2_BASE->CR1 |= SPI_CR1_CPHA;
	SPI2_BASE->CR1 &= ~SPI_CR1_BR;
	SPI2_BASE->CR1 |= (7 << 3);	// BR=111 = /256 = ~140kHz
	SPI_ENABLE();

	// Drive MOSI HIGH to match stock TX idle state before first SPI data.
	// Stock TX (02a) has MOSI HIGH for ~500ms (during hardware RESET)
	// before writing the first register.  STM32 hardware SPI idles MOSI
	// LOW after mode switch.  Clock out 0xFF with CS deselected — the
	// LT8910 ignores SPI when CS is HIGH, but MOSI stays HIGH (last bit
	// was 1), matching the stock TX's pre-init MOSI state.
	SPI_CSN_on;
	SPI_Write(0xFF);
	while(SPI2_BASE->SR & SPI_SR_BSY);
}

static void __attribute__((unused)) CG022_LT8910_init()
{
	// Hardware RESET via PA14 (LT8910_RST_pin) — replicates stock TX behavior.
	// Stock TX (capture 02a) drives RET LOW for 500ms before any SPI:
	//   RET=0 at t=60.275ms, RET=1 at t=561.317ms, first SPI at t=567.180ms.
	// The hardware RESET initializes the LT8910's SPI interface including the
	// MISO output driver, which may not be activated by software reset alone.
	// LT8910_RST_pin (PA14) is a dedicated line wired to STM32 package pin 37.
	// PKT is the LT8910 packet-status/IRQ output and stays unconnected here,
	// matching the stock TX hardware described in docs/LT8910_CG022_Implementation.md.
#ifdef LT8910_RST_HI
	// Ensure RET is HIGH before starting reset sequence.
	// By driving HIGH for 50ms, we ensure the chip fully exits any prior
	// reset state and stabilizes before we start the controlled 500ms LOW
	// reset pulse.
	LT8910_RST_HI;				// Ensure RET HIGH — chip fully out of reset
	delayMilliseconds(50);		// Extended settle time for rebind (was 10ms)
	LT8910_RST_LO;				// Drive RET LOW — begin hardware reset
	delayMilliseconds(480);		// Hold LOW for 480ms; with the 50ms pre-pulse and 6ms settle this matches the stock reset timing budget.
	LT8910_RST_HI;				// Release RET HIGH — chip exits reset
	delayMilliseconds(6);		// 6ms settling before first SPI (stock TX: 5.9ms)
#else
	// Fallback: software reset if LT8910_RST_HI is not defined (e.g. AVR boards).
	LT8910_WriteReg(LT8910_REG_CHANNEL, LT8910_CHIP_RST);
	delayMilliseconds(100);
#endif

	// Write all init registers from stock TX capture.
	// Stock TX (02b) has ~228µs gaps between every SPI transaction.
	// At /256, each LT8910_WriteReg() takes ~196µs on the wire, so
	// back-to-back calls produce ~200µs start-to-start spacing that
	// closely matches the stock TX's 228µs (slow 8-bit MCU SPI) timing.
	for(uint8_t i = 0; i < CG022_NUM_INIT_REGS; i++)
	{
		uint8_t  reg = pgm_read_byte_near(&CG022_init_regs[i * 3]);
		uint16_t val = ((uint16_t)pgm_read_byte_near(&CG022_init_regs[i * 3 + 1]) << 8)
					 | pgm_read_byte_near(&CG022_init_regs[i * 3 + 2]);
		LT8910_WriteReg(reg, val);
	}
	// Match stock TX post-init sequence (02b transactions [31]-[32]):
	// Clear FIFO, wait 12ms for PLL settling, then read CRC seed register.
	// Stock TX 02b has an 11.9ms gap between FIFO clear and CRC read.
	LT8910_WriteReg(LT8910_REG_FIFO, 0x0000);
	delayMilliseconds(12);
	LT8910_ReadReg(LT8910_REG_CRC_SEED);
}

static void __attribute__((unused)) CG022_reset_lt8910_for_bind()
{
	// Reinitialize LT8910 and reset bind state to match power-on autobind.
	CG022_prepare_hardware();
	CG022_LT8910_init();
	if(!LT8910_DetectChip())
	{
		SUB_PROTO_INVALID;
		BIND_DONE;
		return;
	}
	CG022_restart_bind_sequence();
}

static void __attribute__((unused)) CG022_write_fifo_post_bind(const uint8_t *data, uint8_t length)
{
	// Stock TX (02b) clears FIFO, updates sync words 4/7, then loads first data payload.
	LT8910_WriteReg(LT8910_REG_FIFO, 0x0000);
	LT8910_WriteReg(LT8910_REG_SYNCWORD4, (uint16_t)(cg022_txid[1] << 8) | cg022_txid[0]);
	// Syncword7 uses the low byte only; the high byte is unused by the LT8910 for this protocol.
	LT8910_WriteReg(LT8910_REG_SYNCWORD7, (uint16_t)cg022_txid[2]);
	LT8910_WriteReg(LT8910_REG_FIFO_CTRL, 0x8080);
	for(uint8_t i = 0; i < length; i += 2)
	{
		uint16_t word = (uint16_t)data[i] << 8;
		if(i + 1 < length)
			word |= data[i + 1];
		LT8910_WriteReg(LT8910_REG_FIFO, word);
	}
}

static void __attribute__((unused)) CG022_write_fifo_no_clear(const uint8_t *data, uint8_t length)
{
	// Stock TX does not clear FIFO between the CRC seed read and the first bind payload.
	LT8910_WriteReg(LT8910_REG_FIFO_CTRL, 0x8080);
	for(uint8_t i = 0; i < length; i += 2)
	{
		uint16_t word = (uint16_t)data[i] << 8;
		if(i + 1 < length)
			word |= data[i + 1];
		LT8910_WriteReg(LT8910_REG_FIFO, word);
	}
}

static void __attribute__((unused)) CG022_send_bind_packet()
{
	// Unique bind payloads from four stock TX captures:
	// TX1 (02b): 0A 00 11 22 33 06 AB FC AD 00
	// TX2 (22b): 0A 00 11 22 33 FB E0 FC D7 00
	// TX3 (32b): 0A 00 11 22 33 F4 B9 FA A7 00
	// TX4 (42b): 0A 00 11 22 33 14 D2 F9 DF 00
	// Bind payload from stock TX captures (02b/22b/32b/42b):
	// 0A 00 11 22 33 xx yy zz cc 00
	// Byte 0: length (0x0A = 10)
	// Byte 1: 0x00 (bind indicator)
	// Bytes 2-4: fixed bind prefix (11 22 33) from bind-time sync words
	// Bytes 5-7: unique TX ID bytes (match post-bind syncword4/7 writes)
	// Byte 8: checksum = (byte5 + byte6 + byte7) & 0xFF
	// Byte 9: 0x00
	uint8_t buf[CG022_PAYLOAD_SIZE];
	buf[0] = 0x0A;			// Length
	buf[1] = 0x00;			// Bind mode
	buf[2] = rx_tx_addr[0];	// Bind prefix byte 0 (syncword4 LSB)
	buf[3] = rx_tx_addr[1];	// Bind prefix byte 1 (syncword4 MSB)
	buf[4] = rx_tx_addr[2];	// Bind prefix byte 2 (syncword7 LSB)
	buf[5] = cg022_txid[0];	// TX ID byte 0 (post-bind syncword4 LSB)
	buf[6] = cg022_txid[1];	// TX ID byte 1 (post-bind syncword4 MSB)
	buf[7] = cg022_txid[2];	// TX ID byte 2 (post-bind syncword7 LSB)
	buf[8] = (uint8_t)(cg022_txid[0] + cg022_txid[1] + cg022_txid[2]);	// Checksum of bytes 5-7
	buf[9] = 0x00;

	if(cg022_first_packet)
		CG022_write_fifo_no_clear(buf, CG022_PAYLOAD_SIZE);
	else
		LT8910_WriteFIFO(buf, CG022_PAYLOAD_SIZE);
}

static void __attribute__((unused)) CG022_send_data_packet()
{
	uint8_t buf[CG022_PAYLOAD_SIZE];

	// Channel values: 0x00..0x3F with 0x20 center
	uint8_t throttle = convert_channel_16b_limit(THROTTLE, 0x00, 0x3F);
	uint8_t elevator = convert_channel_16b_limit(ELEVATOR, 0x00, 0x3F);
	uint8_t rudder   = convert_channel_16b_limit(RUDDER,   0x00, 0x3F);
	uint8_t aileron  = convert_channel_16b_limit(AILERON,  0x00, 0x3F);
	rudder = 0x3F - rudder;
	aileron = 0x3F - aileron;

	// Rate mode (3-position on CH5): 20%/60%/100%
	// From stock TX captures: 20% = 0x00 (03b_20), 60% = 0x11 (23b_60), 100% = 0x22 (03b)
	// CH5 > CHANNEL_SWITCH = 100% (high rate), CH5 < CHANNEL_MIN_COMMAND = 20% (low rate)
	// Otherwise 60% (mid rate)
	uint8_t rate_byte = 0x11;				// Mid rate (60%) - from capture 23b_60
	if(CH5_SW)
		rate_byte = 0x22;					// High rate (100%) - from capture 03b
	else if(Channel_data[CH5] < CHANNEL_MIN_COMMAND)
		rate_byte = 0x00;					// Low rate (20%) - from capture 03b_20

	// Flags byte 6: bit7 = LED off (CH8)
	uint8_t flags6 = 0x20;
	if(CH8_SW)
		flags6 |= 0x80;		// LED off

	// Flags byte 7: bit7 = headless (CH7), bit6 = flip (CH6)
	uint8_t flags7 = 0x20;
	if(CH6_SW)
		flags7 |= 0x40;		// Flip
	if(CH7_SW)
		flags7 |= 0x80;		// Headless mode

	buf[0] = 0x0A;				// Length
	buf[1] = rate_byte;			// Rate mode: 0x00=20%, 0x11=60%, 0x22=100%
	buf[2] = throttle;
	buf[3] = elevator;
	buf[4] = rudder;
	buf[5] = aileron;
	buf[6] = flags6;
	buf[7] = flags7;
	buf[8] = 0x20;				// Fixed from captures

	// Checksum: sum of bytes 2-8
	uint8_t checksum = 0;
	for(uint8_t i = 2; i <= 8; i++)
		checksum += buf[i];
	buf[9] = checksum;

	if(cg022_post_bind_sync)
	{
		CG022_write_fifo_post_bind(buf, CG022_PAYLOAD_SIZE);
		cg022_post_bind_sync = false;
	}
	else
		LT8910_WriteFIFO(buf, CG022_PAYLOAD_SIZE);
}

uint16_t CG022_callback()
{
	// CRITICAL: Exit immediately if protocol is being changed.
	// modules_reset() restores SPI to Mode 0, but CG022 uses Mode 1.
	// Doing ANY SPI operations after SPI settings change would hang/corrupt.
	if(IS_CHANGE_PROTOCOL_FLAG_on)
		return CG022_PACKET_PERIOD;

	#ifdef MULTI_SYNC
		telemetry_set_input_sync(CG022_PACKET_PERIOD);
	#endif

	// Handle bind counter exhausted during bind - restart bind sequence.
	// This only triggers the soft reset (sync words, channel) since the
	// hardware is already initialized.
	if(IS_BIND_IN_PROGRESS && bind_counter == 0 && !IS_CHANGE_PROTOCOL_FLAG_on)
		CG022_restart_bind_sequence();

	uint8_t channel = CG022_hop_channels[hopping_frequency_no];

	// Stock TX (02b) cycle: SetChannel → FIFO writes → SetTxOn
	// Each SPI write takes ~196µs on the wire at /256 clock, closely matching
	// the stock TX's ~228µs per transaction at ~114kHz bit-bang SPI.
	// Total SetChannel→SetTxOn ≈ 9 writes × ~200µs ≈ 1800µs (stock TX: 1829µs).
	// R07 controls both channel and TX_EN; SetChannel must come BEFORE SetTxOn,
	// and nothing must write R07 AFTER SetTxOn (that would abort the transmission).
	if(!cg022_first_packet)
		LT8910_SetChannel(channel);

	// PLL settles concurrently during FIFO write transactions (same as stock TX).
	if(bind_counter)
	{
		CG022_send_bind_packet();
		bind_counter--;
		if(bind_counter == 0)
		{
			cg022_post_bind_sync = true;
			BIND_DONE;
		}
	}
	else
		CG022_send_data_packet();

	// Transmit on current channel (PLL fully settled).
	// After this, R07 must NOT be written again until the next callback —
	// the packet needs ~480µs on-air before the next SetChannel.
	LT8910_SetTxOn(channel);

	// Advance to next hop channel for next callback
	hopping_frequency_no++;
	if(hopping_frequency_no >= CG022_NUM_CHANNELS)
		hopping_frequency_no = 0;
	cg022_first_packet = false;

	return CG022_PACKET_PERIOD;
}

static void __attribute__((unused)) CG022_initialize_txid()
{
	// Fixed bind prefix bytes from stock capture bind packets (capture 02b)
	// Stock FIFO word 2 = 0x1122, word 3 = 0x3306
	// -> bind prefix bytes: 0x11, 0x22, 0x33 (bytes 2-4)
	// -> TX ID bytes 0-2 derived from MProtocol_id lower 24 bits
	rx_tx_addr[0] = 0x11;
	rx_tx_addr[1] = 0x22;
	rx_tx_addr[2] = 0x33;
	uint32_t txid = MProtocol_id & 0x00FFFFFF;
	cg022_txid[0] = (uint8_t)txid;
	cg022_txid[1] = (uint8_t)(txid >> 8);
	cg022_txid[2] = (uint8_t)(txid >> 16);
}

void CG022_init(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	CG022_initialize_txid();
	// Initial startup uses blocking reset - telemetry not started yet.
	CG022_reset_lt8910_for_bind();
	// Init writes R07=0x0000 (channel 0, TX off) as part of register config.
}

#endif
