/*
 * MC8RE V2 A7105 protocol.  This is the fixed-ID V59 implementation.
 *
 * The first 61 normal packets use the captured header-84 startup cadence:
 * two slots are RX/idle.  At packet 61, the capture switches to header 04
 * and resumes the ordinary 16-channel sequence, retaining only the RX slot
 * following channel 02.
 */
#if defined(MC8RE_A7105_INO)

#include "iface_a7105.h"

#define MC8V43_SLOT_US          1970U
#define MC8V43_RX_START_US      1025U
#define MC8V43_RX_HOLD_US       2915U
#define MC8V43_BIND_PACKETS     6U
#define MC8V43_PACKET_SIZE      13U
#define MC8V43_STARTUP_PACKETS  61U
/* Session 1 captured this normal-mode A7105 ID.  A later attempt to derive
 * it from EdgeTX receiver number bound briefly but did not maintain control;
 * this implementation deliberately keeps the capture-proven fixed ID. */
#define MC8RE_LINK_ID           0x51D567EBUL

static const uint8_t PROGMEM mc8v43_first_tx[9] = {
	0x7A, 0x2A, 0x70, 0x20, 0x66, 0x5C, 0x0C, 0x52, 0x02
};

static const uint8_t PROGMEM mc8v43_startup_tx[15] = {
	0x98, 0x98, 0x8E, 0x3E, 0x84, 0x34, 0x7A, 0x2A,
	0x70, 0x20, 0x66, 0x5C, 0x0C, 0x52, 0x02
};

static const uint8_t PROGMEM mc8v43_normal_hop[16] = {
	0x7A, 0x2A, 0x70, 0x20, 0x66, 0x16, 0x5C, 0x0C,
	0x52, 0x02, 0x98, 0x48, 0x8E, 0x3E, 0x84, 0x34
};

/* The captured header-04 transition retains 0x98 for one extra slot.
 * Subsequent completed normal cycles use mc8v43_normal_hop above. */
static const uint8_t PROGMEM mc8v43_first_steady_hop[16] = {
	0x7A, 0x2A, 0x70, 0x20, 0x66, 0x16, 0x5C, 0x0C,
	0x52, 0x02, 0x98, 0x98, 0x8E, 0x3E, 0x84, 0x34
};

static const uint8_t PROGMEM mc8v43_rx[16] = {
	0x98, 0x48, 0x8E, 0x3E, 0x84, 0x34, 0x7A, 0x2A,
	0x70, 0x20, 0x66, 0x16, 0x5C, 0x0C, 0x52, 0x02
};

static const uint8_t PROGMEM mc8v43_startup_packet[13] = {
	0x84, 0x00, 0xE9, 0x4B, 0x5F, 0x64, 0xD2,
	0x57, 0x9F, 0xFA, 0xD4, 0xA7, 0x3E
};

static const uint8_t PROGMEM mc8v43_steady_packet[13] = {
	0x04, 0x04, 0xEC, 0x23, 0x5F, 0x77, 0xC7,
	0xD7, 0xDD, 0xF4, 0x4D, 0x0A, 0x60
};

static const uint8_t PROGMEM mc8v43_regs[0x32] = {
	0xFF, 0x42, 0xFF, 0x0C, 0x00, 0xFF, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x01, 0x50,
	0x9E, 0x4B, 0x00, 0x02, 0x16, 0x2B, 0x12, 0x00,
	0x62, 0x80, 0x80, 0x00, 0x0A, 0x32, 0xC3, 0x0F,
	0x13, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x3B, 0x00,
	0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,
	0x01, 0x0F
};

enum MC8V43_State : uint8_t {
	MC8V43_WAIT_BIND,
	MC8V43_BIND,
	MC8V43_TX,
	MC8V43_IDLE_AFTER_66,
	MC8V43_RX_START,
	MC8V43_RX_FINISH
};

static MC8V43_State mc8v43_state;
static uint8_t mc8v43_bind_sent;
static uint8_t mc8v43_tx_index;
static uint8_t mc8v43_rx_index;
static bool mc8v43_first_fragment;
static bool mc8v43_startup;
static bool mc8v43_first_steady;
static uint32_t mc8v43_packets;
static uint32_t mc8v43_frame;
static uint32_t mc8v43_link_id;

/* P1 is a rolling frame counter.  P2..P12 are 88 bits containing eight
 * consecutive little-endian 11-bit channel values.  Captured hardware tests
 * established 358/1024/1689 as the wire values for 1000/1500/2000 us. */
static uint16_t mc8v53_standard_channel(uint8_t source)
{
	const uint16_t input = limit_channel_100(CH_AETR[source]);
	const uint16_t low = 358U;
	const uint16_t high = 1689U;
	if(input <= CHANNEL_MID)
		return 1024U - ((uint32_t)(CHANNEL_MID - input) * (1024U - low)
			+ (CHANNEL_MID - CHANNEL_MIN_100) / 2U)
			/ (CHANNEL_MID - CHANNEL_MIN_100);
	return 1024U + ((uint32_t)(input - CHANNEL_MID) * (high - 1024U)
		+ (CHANNEL_MAX_100 - CHANNEL_MID) / 2U)
		/ (CHANNEL_MAX_100 - CHANNEL_MID);
}

static void mc8v53_put_u11(uint8_t slot, uint16_t value)
{
	const uint8_t first_bit = slot * 11U;
	value &= 0x07FF;
	for(uint8_t bit = 0; bit < 11U; bit++)
	{
		const uint8_t packed_bit = first_bit + bit;
		const uint8_t offset = 2U + (packed_bit >> 3);
		const uint8_t mask = 1U << (packed_bit & 7U);
		if(value & (1U << bit))
			packet[offset] |= mask;
		else
			packet[offset] &= ~mask;
	}
}

static void mc8v53_apply_packed_channels()
{
	for(uint8_t source = 0; source < 8U; source++)
		mc8v53_put_u11(source, mc8v53_standard_channel(source));
}

#if defined(TELEMETRY)
/*
 * Session 1 was captured with the receiver's external voltage input at 0 V.
 * Its replies were 01 0D 00 31 ... (149 times) and 01 0D 00 30 ... (once).
 * V54 originally used byte 2 as a tentative external-voltage value.  V57
 * raw telemetry instead measured byte 3 = 0x32 for the 5.0 V BEC and byte 4
 * = 0xA0..0xA3 with a known 16.3 V external pack.  V58/V59 therefore use
 * byte 4 for A2 external voltage, with 0.1 V/count.  They send the unscaled
 * decivolt value so that the EdgeTX model can set A2's ratio to 25.5 V for
 * 4S range.
 */
static uint8_t mc8v54_frsky_adc(uint8_t decivolts)
{
	const uint16_t adc = ((uint16_t)decivolts * 255U + 66U) / 132U;
	return adc > 255U ? 255U : adc;
}

/* Register 1D is the A7105 RSSI ADC while 1E is configured for continuous
 * RSSI sampling by the captured register table.  Its useful reference-code
 * span is about 8 (very strong) to 160 (very weak).  This is intentionally a
 * relative 0..100 indication, not a calibrated dBm conversion. */
static uint8_t mc8v59_relative_rssi(uint8_t raw)
{
	if(raw <= 8U)
		return 100U;
	if(raw >= 160U)
		return 0U;
	return ((uint16_t)(160U - raw) * 100U + 76U) / 152U;
}

static uint8_t mc8v59_rssi;
static bool mc8v59_rssi_valid;

static void mc8v54_process_reply()
{
	if(packet[0] != 0x01 || packet[1] != 0x0D)
		return;

	const uint8_t external_raw = packet[4];
	const uint8_t link_raw = packet[2];
	const uint8_t receiver_raw = packet[3];
	v_lipo1 = mc8v54_frsky_adc(receiver_raw);
	/* The default FrSky-D A2 ratio is only 13.2 V.  Preserve the original
	 * 0.1 V/count receiver value for an EdgeTX A2 ratio of 25.5 V. */
	v_lipo2 = external_raw;
	{
		const uint8_t sample = mc8v59_relative_rssi(A7105_ReadReg(A7105_1D_RSSI_THOLD));
		if(!mc8v59_rssi_valid)
		{
			mc8v59_rssi = sample;
			mc8v59_rssi_valid = true;
		}
		else
			mc8v59_rssi = ((uint16_t)mc8v59_rssi * 3U + sample + 2U) / 4U;
		RX_RSSI = mc8v59_rssi;
	}
	TX_RSSI = receiver_raw <= 127U ? receiver_raw << 1 : 0xFF;
	RX_LQI = link_raw;
	TX_LQI = receiver_raw;
	telemetry_counter++;
	telemetry_lost = 0;
	telemetry_link = 1;
}
#endif

static void mc8v43_radio_init()
{
	A7105_Reset();
	A7105_WriteID(0x5475C52A);
	for(uint8_t reg = 0; reg < 0x32; reg++)
	{
		const uint8_t value = pgm_read_byte_near(&mc8v43_regs[reg]);
		if(value != 0xFF)
			A7105_WriteReg(reg, value);
	}
	A7105_Strobe(A7105_STANDBY);
	A7105_WriteReg(A7105_02_CALC, 0x01);
	while(A7105_ReadReg(A7105_02_CALC));
	A7105_WriteReg(A7105_24_VCO_CURCAL, 0x13);
	A7105_WriteReg(A7105_26_VCO_SBCAL_II, 0x3B);
	A7105_WriteReg(A7105_0F_CHANNEL, 0x00);
	A7105_WriteReg(A7105_02_CALC, 0x02);
	while(A7105_ReadReg(A7105_02_CALC));
	A7105_WriteReg(A7105_0F_CHANNEL, 0xA0);
	A7105_WriteReg(A7105_02_CALC, 0x02);
	while(A7105_ReadReg(A7105_02_CALC));
	A7105_WriteReg(A7105_25_VCO_SBCAL_I, 0x0A);
	A7105_SetTxRxMode(TX_EN);
	/* The register table has already written the captured init value 0x17.
	 * send_bind() changes it to 0x0D immediately before the first FIFO load. */
	prev_power = 0x17;
	A7105_Strobe(A7105_STANDBY);
}

static void mc8v43_build_bind()
{
	memset(packet, 0, MC8V43_PACKET_SIZE);
	packet[0] = 0xAA;
	packet[1] = 0x11;
	packet[2] = (uint8_t)(mc8v43_link_id >> 16);
	packet[3] = (uint8_t)(mc8v43_link_id >> 8);
	packet[4] = (uint8_t)mc8v43_link_id;
}

static void mc8v43_build_normal()
{
	const uint8_t *source = mc8v43_startup
		? mc8v43_startup_packet : mc8v43_steady_packet;
	for(uint8_t i = 0; i < MC8V43_PACKET_SIZE; i++)
		packet[i] = pgm_read_byte_near(source + i);
	/* P1 is the rolling counter; P2..P12 contain eight packed 11-bit values. */
	packet[1] = mc8v43_frame & 0x0F;
	if(!mc8v43_startup)
		mc8v53_apply_packed_channels();
}

static void mc8v43_send_bind()
{
	mc8v43_build_bind();
	A7105_WriteReg(A7105_28_TX_TEST, 0x0D);
	prev_power = 0x0D;
	A7105_WriteReg(A7105_27_BATTERY_DET, 0x0F);
	A7105_Strobe(A7105_STANDBY);
	A7105_WriteData(MC8V43_PACKET_SIZE, 0x01);
}

static void mc8v43_enter_normal()
{
	A7105_Strobe(A7105_STANDBY);
	A7105_WriteID(mc8v43_link_id);
	A7105_WriteReg(A7105_28_TX_TEST, 0x1F);
	prev_power = 0x1F;
	mc8v43_state = MC8V43_TX;
	mc8v43_tx_index = 0;
	mc8v43_rx_index = 0;
	mc8v43_first_fragment = true;
	mc8v43_startup = true;
	mc8v43_first_steady = false;
	mc8v43_packets = 0;
	mc8v43_frame = 0;
	if(IS_BIND_IN_PROGRESS)
		BIND_DONE;
}

void MC8V43_init()
{
	mc8v43_link_id = MC8RE_LINK_ID;
	mc8v43_radio_init();
	/* Do not radiate until the Multi protocol's Bind flag is present.
	 * This also handles the path where WAIT_FOR_BIND has already initialized
	 * the radio and the subsequent EdgeTX Bind request only releases it. */
	mc8v43_state = IS_BIND_IN_PROGRESS ? MC8V43_BIND : MC8V43_WAIT_BIND;
	mc8v43_bind_sent = 0;
	mc8v43_tx_index = 0;
	mc8v43_rx_index = 0;
	mc8v43_first_fragment = true;
	mc8v43_startup = true;
	mc8v43_first_steady = false;
	mc8v43_packets = 0;
	mc8v43_frame = 0;
	#if defined(TELEMETRY)
		mc8v59_rssi = 0;
		mc8v59_rssi_valid = false;
	#endif
}

uint16_t MC8V43_callback()
{
	if(mc8v43_state == MC8V43_WAIT_BIND)
	{
		if(!IS_BIND_IN_PROGRESS)
			return MC8V43_SLOT_US;
		mc8v43_state = MC8V43_BIND;
	}

	if(mc8v43_state == MC8V43_BIND)
	{
		if(mc8v43_bind_sent < MC8V43_BIND_PACKETS)
		{
			mc8v43_send_bind();
			mc8v43_bind_sent++;
			return MC8V43_SLOT_US;
		}
		mc8v43_enter_normal();
	}

	if(mc8v43_state == MC8V43_IDLE_AFTER_66)
	{
		mc8v43_tx_index++;
		mc8v43_state = MC8V43_TX;
		return MC8V43_SLOT_US;
	}

	if(mc8v43_state == MC8V43_RX_START)
	{
		A7105_Strobe(A7105_STANDBY);
		A7105_SetTxRxMode(RX_EN);
		A7105_WriteReg(A7105_0F_CHANNEL,
			pgm_read_byte_near(&mc8v43_rx[mc8v43_rx_index]));
		A7105_Strobe(A7105_RST_RDPTR);
		A7105_Strobe(A7105_RX);
		mc8v43_rx_index = (mc8v43_rx_index + 1) & 0x0F;
		mc8v43_state = MC8V43_RX_FINISH;
		return MC8V43_RX_HOLD_US;
	}

	if(mc8v43_state == MC8V43_RX_FINISH)
	{
		if((A7105_ReadReg(A7105_00_MODE) & 0x01) == 0)
		{
			A7105_ReadData(MC8V43_PACKET_SIZE);
			#if defined(TELEMETRY)
				mc8v54_process_reply();
			#endif
		}
		mc8v43_frame++;
		if(mc8v43_startup)
		{
			mc8v43_tx_index = 0;
			mc8v43_first_fragment = false;
		}
		mc8v43_state = MC8V43_TX;
	}

	mc8v43_build_normal();
	const uint8_t channel = mc8v43_startup
		? (mc8v43_first_fragment
			? pgm_read_byte_near(&mc8v43_first_tx[mc8v43_tx_index])
			: pgm_read_byte_near(&mc8v43_startup_tx[mc8v43_tx_index]))
		: pgm_read_byte_near(mc8v43_first_steady
			? &mc8v43_first_steady_hop[mc8v43_tx_index]
			: &mc8v43_normal_hop[mc8v43_tx_index]);
	A7105_WriteReg(A7105_27_BATTERY_DET, 0x0F);
	A7105_Strobe(A7105_STANDBY);
	A7105_WriteData(MC8V43_PACKET_SIZE, channel);
	mc8v43_packets++;

	if(mc8v43_startup && mc8v43_packets == MC8V43_STARTUP_PACKETS)
	{
		/* The observed header-04 transition is at 2A, normal_hop[1]. */
		mc8v43_startup = false;
		mc8v43_first_steady = true;
		mc8v43_tx_index = 1;
		return MC8V43_SLOT_US;
	}
	if(mc8v43_startup && channel == 0x66)
	{
		mc8v43_state = MC8V43_IDLE_AFTER_66;
		return MC8V43_SLOT_US;
	}
	if(channel == 0x02)
	{
		if(!mc8v43_startup)
			mc8v43_tx_index = (mc8v43_tx_index + 1) & 0x0F;
		mc8v43_state = MC8V43_RX_START;
		return MC8V43_RX_START_US;
	}
	/* Complete the captured first-steady table, then use the normal table. */
	if(!mc8v43_startup && mc8v43_tx_index == 0x0F)
	{
		mc8v43_tx_index = 0;
		mc8v43_first_steady = false;
	}
	else
		mc8v43_tx_index++;
	return MC8V43_SLOT_US;
}

void MC8RE_init()
{
	MC8V43_init();
}

uint16_t MC8RE_callback()
{
	return MC8V43_callback();
}

#endif
