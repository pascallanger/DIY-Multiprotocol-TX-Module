/***
 * iNav Protocol
 *
 * Data rate is 250Kbps - lower data rate for better reliability and range
 *
 * Uses auto acknowledgment and dynamic payload size
 *     ACK payload is used for handshaking in bind phase and telemetry in data phase
 *
 * Bind payload size is 16 bytes
 * Data payload size is 16 bytes dependent on variant of protocol, (small payload is read more quickly (marginal benefit))
 *
 * Bind and data payloads are whitened (XORed with a pseudo-random bitstream) to remove strings of 0s or 1s in transmission
 * packet. This improves reception by helping keep the TX and RX clocks in sync.
 *
 * Bind Phase
 * uses address {0x4b,0x5c,0x6d,0x7e,0x8f}
 * uses channel 0x4c (76)
 *
 * Data Phase
 * 1) Uses the address received in bind packet
 *
 * 2) Hops between RF channels generated from the address received in bind packet.
 *    The number of RF hopping channels is set during bind handshaking:
 *        the transmitter requests a number of hopping channels in payload[7]
 *        the receiver sets ackPayload[7] with the number of hopping channels actually allocated - the transmitter must
 *        use this value.
 *
 * 3) There are 16 channels: eight 10-bit analog channels, two 8-bit analog channels, and six digital channels as follows:
 *    Channels 0 to 3, are the AETR channels, values 1000 to 2000 with resolution of 1 (10-bit channels)
 *    Channel AUX1 by deviation convention is used for rate, values 1000, 1500, 2000
 *    Channels AUX2 to AUX6 are binary channels, values 1000 or 2000,
 *        by deviation convention these channels are used for: flip, picture, video, headless, and return to home
 *    Channels AUX7 to AUX10 are analog channels, values 1000 to 2000 with resolution of 1 (10-bit channels)
 *    Channels AUX11 and AUX12 are analog channels, values 1000 to 2000 with resolution of 4 (8-bit channels)
***/

// debug build flags
//#define NO_RF_CHANNEL_HOPPING

#define USE_AUTO_ACKKNOWLEDGEMENT
#define USE_WHITENING

#define INAV_TELEMETRY
//#define INAV_TELEMETRY_DEBUG

#define UNUSED(x) (void)(x)

#ifdef INAV_NRF24L01_INO
	static uint8_t INAV_setup() { return 1000; }
	static uint16_t inav_cb() {}
#endif
#if 0
/*
*/

	#include "iface_nrf24l01.h"

	#define INAV_BIND_COUNT 345   // 1.5 seconds
	#define FIRST_INAV_PACKET_DELAY  12000

	#define INAV_PACKET_PERIOD        4000     // Timeout for callback in uSec
	#define INAV_INITIAL_WAIT          500

	// For code readability
	enum {
		CHANNEL1 = 0,
		CHANNEL2,
		CHANNEL3,
		CHANNEL4,
		CHANNEL5,
		CHANNEL6,
		CHANNEL7,
		CHANNEL8,
		CHANNEL9,
		CHANNEL10,
		CHANNEL11,
		CHANNEL12,
		CHANNEL13,
		CHANNEL14,
		CHANNEL15,
		CHANNEL16,
	};

	// Deviation transmitter channels
	#define DEVIATION_CHANNEL_COUNT    12 // Max supported by Devo 7e
	//#define CHANNEL_LED         CHANNEL5
	#define CHANNEL_RATE        CHANNEL5
	#define CHANNEL_FLIP        CHANNEL6
	#define CHANNEL_PICTURE     CHANNEL7
	#define CHANNEL_VIDEO       CHANNEL8
	#define CHANNEL_HEADLESS    CHANNEL9
	#define CHANNEL_RTH         CHANNEL10
	#define CHANNEL_XCAL        CHANNEL11
	#define CHANNEL_YCAL        CHANNEL12
//	#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)


	enum {
		INAV_RATE_LOW = 0,
		INAV_RATE_MID = 1,
		INAV_RATE_HIGH = 2,
	};

	enum {
		INAV_FLAG_FLIP     = 0x01,
		INAV_FLAG_PICTURE  = 0x02,
		INAV_FLAG_VIDEO    = 0x04,
		INAV_FLAG_RTH      = 0x08,
		INAV_FLAG_HEADLESS = 0x10,
	};

	typedef enum {
		PHASE_INIT = 0,
		PHASE_INAV_BIND,
		PHASE_INAV_DATA
	};

	typedef enum {
		DATA_INAV_PACKET = 0,
		BIND_INAV_PACKET = 1,
	};

	static const char * const inav_opts[] = {
	#ifdef INAV_TELEMETRY
	  _tr_noop("Telemetry"),  _tr_noop("On"), _tr_noop("Off"), NULL,
	#endif
	  "RxTx Addr1", "-32768", "32767", "1", NULL, // todo: store that elsewhere
	  "RxTx Addr2", "-32768", "32767", "1", NULL, // ^^^^^^^^^^^^^^^^^^^^^^^^^^
	  NULL
	};

	enum {
	#ifdef INAV_TELEMETRY
		PROTOOPTS_TELEMETRY,
	#endif
		PROTOOPTS_RX_TX_ADDR1, // todo: store that elsewhere
		PROTOOPTS_RX_TX_ADDR2, // ^^^^^^^^^^^^^^^^^^^^^^^^^^
		LAST_PROTO_OPT
	};

	enum {
		OPTS_6_CHANNELS = 0,
		OPTS_12_CHANNELS
	};

	enum {
		TELEM_ON = 0,
		TELEM_OFF,
	};

	// Bit vector from bit position
	#define BV(bit) (1 << bit)

	// Bit position mnemonics
	enum {

		NRF24L01_1D_EN_DYN_ACK              = 0,
		NRF24L01_1D_EN_ACK_PAY              = 1,
		NRF24L01_1D_EN_DPL                  = 2,
	};

	// Pre-shifted and combined bits
	enum {
		NRF24L01_03_SETUP_AW_5BYTES         = 0x03,
	};

	enum {
		NRF24L01_MAX_PAYLOAD_SIZE           = 32,
	};

	#define INAV_PROTOCOL_PAYLOAD_SIZE_MIN      8
	#define INAV_PROTOCOL_PAYLOAD_SIZE_DEFAULT  16
	#define INAV_PROTOCOL_PAYLOAD_SIZE_MAX      16

	static uint32_t lost_packet_counter;

	#define RC_CHANNEL_COUNT_MIN         6
	#define RC_CHANNEL_COUNT_DEFAULT    16
	#define RC_CHANNEL_COUNT_MAX        18

	static uint8_t rc_channel_count;

	static uint8_t tx_power;

	#define RX_TX_ADDR_LEN 5
	static const uint8_t rx_tx_addr_bind[RX_TX_ADDR_LEN] = {0x4b,0x5c,0x6d,0x7e,0x8f};
	static uint8_t rx_tx_addr[RX_TX_ADDR_LEN];
	#define RX_TX_ADDR_4 0xD2 // rxTxAddr[4] always set to this value

	#define BIND_PAYLOAD0           0xad // 10101101
	#define BIND_PAYLOAD1           0xc9 // 11001001
	#define BIND_ACK_PAYLOAD0       0x95 // 10010101
	#define BIND_ACK_PAYLOAD1       0xa9 // 10101001
	#define TELEMETRY_ACK_PAYLOAD0  0x5a // 01011010
	// TELEMETRY_ACK_PAYLOAD1 is sequence count
	#define DATA_PAYLOAD0           0x00
	#define DATA_PAYLOAD1           0x00

	#ifdef USE_AUTO_ACKKNOWLEDGEMENT
	static uint8_t ackPayloadSize;
	static uint8_t ackPayload[NRF24L01_MAX_PAYLOAD_SIZE];
	#endif

	// frequency channel management
	#define RF_CHANNEL_COUNT_DEFAULT 4
	#define RF_CHANNEL_COUNT_MAX 4
	#define RF_CHANNEL_BIND 0x4c
	static uint8_t rf_channel_index;
	static uint8_t rf_channels[RF_CHANNEL_COUNT_MAX];
	static uint8_t rf_channel_count;

	static protocol_phase_t phase;

	#ifndef TELEM_LTM
	// use DSM telemetry until LTM telemetry is implemented
	#define TELEM_LTM TELEM_DSM
	// repurpose the DSM FADESL, FADESR and HOLDS fields to display roll, pitch and yaw
	#define TELEM_LTM_ATTITUDE_PITCH    TELEM_DSM_FLOG_FADESL
	#define TELEM_LTM_ATTITUDE_ROLL     TELEM_DSM_FLOG_FADESR
	#define TELEM_LTM_ATTITUDE_YAW      TELEM_DSM_FLOG_HOLDS
	// use  DSM VOLT2, AMPS1 and AIRSPEED fields
	#define TELEM_LTM_STATUS_VBAT       TELEM_DSM_FLOG_VOLT2
	#define TELEM_LTM_STATUS_CURRENT    TELEM_DSM_AMPS1
	#define TELEM_LTM_STATUS_RSSI       TELEM_DSM_FLOG_FRAMELOSS
	#define TELEM_LTM_STATUS_AIRSPEED   TELEM_DSM_AIRSPEED
	/*
	currently unused LTM telemetry fields
	TELEM_LTM_STATUS_ARMED
	TELEM_LTM_STATUS_FAILSAFE
	TELEM_LTM_STATUS_FLIGHTMODE
	TELEM_LTM_NAVIGATION_GPS_MODE
	TELEM_LTM_NAVIGATION_NAV_MODE
	TELEM_LTM_NAVIGATION_ACTION
	TELEM_LTM_NAVIGATION_WAYPOINT_NUMBER
	TELEM_LTM_NAVIGATION_ERROR
	TELEM_LTM_NAVIGATION_FLAGS
	TELEM_LTM_GPSX_HDOP
	TELEM_LTM_TUNING_P_ROLL
	TELEM_LTM_TUNING_I_ROLL
	TELEM_LTM_TUNING_D_ROLL
	TELEM_LTM_TUNING_P_PITCH
	TELEM_LTM_TUNING_I_PITCH
	TELEM_LTM_TUNING_D_PITCH
	TELEM_LTM_TUNING_P_YAW
	TELEM_LTM_TUNING_I_YAW
	TELEM_LTM_TUNING_D_YAW
	TELEM_LTM_TUNING_RATES_ROLL
	TELEM_LTM_TUNING_RATES_PITCH
	TELEM_LTM_TUNING_RATES_YAW
	*/
	#endif

	/*
	uint8_t convert_channel_8b(uint8_t channel)
	{
		return (uint8_t)(convert_channel(channel) >> 2);
	}
	 */

	static void whiten_payload(uint8_t *payload, uint8_t len)
	{
	#ifdef USE_WHITENING
		uint8_t whitenCoeff = 0x6b; // 01101011
		while (len--) {
			for (uint8_t m = 1; m; m <<= 1) {
				if (whitenCoeff & 0x80) {
					whitenCoeff ^= 0x11;
					(*payload) ^= m;
				}
				whitenCoeff <<= 1;
			}
			payload++;
		}
	#else
		UNUSED(payload);
		UNUSED(len);
	#endif
	}

	static void build_bind_packet(void)
	{
		memset(packet, 0, INAV_PROTOCOL_PAYLOAD_SIZE_MAX);
		packet[0] = BIND_PAYLOAD0;
		packet[1] = BIND_PAYLOAD1;
		packet[2] = rx_tx_addr[0];
		packet[3] = rx_tx_addr[1];
		packet[4] = rx_tx_addr[2];
		packet[5] = rx_tx_addr[3];
		packet[6] = rx_tx_addr[4];
		packet[7] = rf_channel_count;
		packet[8] = packet_length;
		packet[9] = rc_channel_count;
	}

	static void build_data_packet(void)
	{
		packet[0] = 0;
		packet[1] = 0;
		// AETR channels have 10 bit resolution
		const uint16_t aileron  = convert_channel_10b(AILERON);
		const uint16_t elevator = convert_channel_10b(ELEVATOR);
		const uint16_t throttle = convert_channel_10b(THROTTLE);
		const uint16_t rudder   = convert_channel_10b(RUDDER);
		packet[2] = aileron >> 2;
		packet[3] = elevator >> 2;
		packet[4] = throttle >> 2;
		packet[5] = rudder >> 2;
		// pack the AETR low bits
		packet[6] = (aileron & 0x03) | ((elevator & 0x03) << 2) | ((throttle & 0x03) << 4) | ((rudder & 0x03) << 6);

		uint8_t rate = INAV_RATE_LOW;
		if (Channels[CHANNEL_RATE] > 0) {
			rate = INAV_RATE_HIGH;
		} else if (Channels[CHANNEL_RATE] == 0) {
			rate = INAV_RATE_MID;
		}
		packet[7] = rate; // rate, deviation channel 5, is mapped to AUX1

		const uint8_t flags = GET_FLAG(CHANNEL_FLIP, INAV_FLAG_FLIP)
				   | GET_FLAG(CHANNEL_PICTURE, INAV_FLAG_PICTURE)
				   | GET_FLAG(CHANNEL_VIDEO, INAV_FLAG_VIDEO)
				   | GET_FLAG(CHANNEL_RTH, INAV_FLAG_RTH)    
				   | GET_FLAG(CHANNEL_HEADLESS, INAV_FLAG_HEADLESS);
		packet[8] = flags; // flags, deviation channels 6-10 are mapped to AUX2 t0 AUX6

		// map deviation channels 9-12 to RC channels AUX7-AUX10, use 10 bit resolution
		// deviation CHANNEL9 (headless) and CHANNEL10 (RTH) are mapped for a second time
		// duplicate mapping makes maximum use of deviation's 12 channels
		const uint16_t channel9 = convert_channel_10b(CHANNEL9);
		const uint16_t channel10 = convert_channel_10b(CHANNEL10);
		const uint16_t channel11 = convert_channel_10b(CHANNEL11);
		const uint16_t channel12 = convert_channel_10b(CHANNEL12);
		packet[9] = channel9 >> 2;
		packet[10] = channel10 >> 2;
		packet[11] = channel11 >> 2;
		packet[12] = channel12 >> 2;
		packet[13] = (channel9 & 0x03) | ((channel10 & 0x03) << 2) | ((channel11 & 0x03) << 4) | ((channel12 & 0x03) << 6);

		// map deviation channels 7 and 8 to RC channels AUX11 and AUX12, use 10 bit resolution
		// deviation channels 7 (picture) and 8 (video) are mapped for a second time
		packet[14] = convert_channel_8b(CHANNEL7);
		packet[15] = convert_channel_8b(CHANNEL8);
	}


	static uint8_t packet_ack(void)
	{
	#ifdef USE_AUTO_ACKKNOWLEDGEMENT
		const uint8_t status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
		if (status & BV(NRF24L01_07_TX_DS)) { // NRF24L01_07_TX_DS asserted when ack payload received
			// ack payload recieved
			ackPayloadSize = NRF24L01_GetDynamicPayloadSize();
			if (ackPayloadSize > NRF24L01_MAX_PAYLOAD_SIZE) {
				ackPayloadSize = 0;
				NRF24L01_FlushRx();
				return PKT_PENDING;
			}
			NRF24L01_ReadPayload(ackPayload, ackPayloadSize);
			whiten_payload(ackPayload, ackPayloadSize);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_TX_DS)); // clear TX_DS interrupt
			return PKT_ACKED;
		}
		if (status & BV(NRF24L01_07_MAX_RT)) {
			// max retries exceeded
			// clear MAX_RT interrupt to allow further transmission
			NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_MAX_RT));
			NRF24L01_FlushTx(); // payload wasn't successfully transmitted, so remove it from TX FIFO
			return PKT_TIMEOUT;
		}
		return PKT_PENDING;
	#else
		return PKT_TIMEOUT; // if not using AUTO ACK just return a timeout
	#endif
	}

	void transmit_packet(void)
	{
		// clear packet MAX_RT status bit so that transmission is not blocked
		NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_MAX_RT));
		// set NRF24L01_00_MASK_TX_DS and clear NRF24L01_00_PRIM_RX to initiate transmit
	//    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_MASK_TX_DS) | BV(NRF24L01_00_MASK_MAX_RT));
		whiten_payload(packet, packet_length);
		NRF24L01_WritePayload(packet, packet_length);// asynchronous call
	}

	static void hop_to_next_channel(void)
	{
	#ifndef NO_RF_CHANNEL_HOPPING
		if (phase == PHASE_INAV_BIND) {
			return;
		}
		++rf_channel_index;
		if (rf_channel_index >= rf_channel_count) {
			rf_channel_index = 0;
		}
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_channel_index]);
	#endif
	}

	static void send_packet(uint8_t packet_type)
	{
		if (packet_type == DATA_INAV_PACKET) {
			build_data_packet();
		} else {
			build_bind_packet();
		}
		transmit_packet();
		hop_to_next_channel();
		++packet_count;

		// Check and adjust transmission power. We do this after
		// transmission to not bother with timeout after power
		// settings change -  we have plenty of time until next
		// packet.
		if (tx_power != Model.tx_power) {
			//Keep transmit power updated
			tx_power = Model.tx_power;
			NRF24L01_SetPower(tx_power);
		}
	}

	static void set_data_phase(void)
	{
		packet_count = 0;
		lost_packet_counter = 0;
		phase = PHASE_INAV_DATA;
		rf_channel_index = 0;
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[0]);
		 // RX_ADDR_P0 must equal TX_ADDR for auto ACK
		NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, RX_TX_ADDR_LEN);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, RX_TX_ADDR_LEN);
	}

	static void init_nrf24l01(void)
	{
		NRF24L01_Initialize();
		// sets PWR_UP, EN_CRC, CRCO - 2 byte CRC
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO));

	#ifdef USE_AUTO_ACKKNOWLEDGEMENT
		// Note that for some cheap NFR24L01 clones the NO_ACK bit in the packet control field is inverted,
		// see https://ncrmnt.org/2015/03/13/how-do-i-cost-optimize-nrf24l01/
		// (see section 7.3.3, p28 of nRF24L01+ Product Specification for descripton of packet control field).
		// This means AUTO_ACK will no work between them and genuine NRF24L01 modules, although AUTO_ACK
		// between these clones should work.
		NRF24L01_WriteReg(NRF24L01_01_EN_AA, BV(NRF24L01_01_ENAA_P0)); // auto acknowledgment on P0
		NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, BV(NRF24L01_02_ERX_P0)); // Enable RX on P0 for Auto Ack
	#else
		NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);
	#endif
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, NRF24L01_03_SETUP_AW_5BYTES); // 5-byte RX/TX address
		// ARD of 1500us is minimum required for 32 byte ACK payload in 250kbps mode (section 7.4.2, p33 of nRF24L01+ Product Specification)
		NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, NRF24L01_04_ARD_2500us | NRF24L01_04_ARC_1); // 1 retry after 1500us
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL_BIND);
		// bitrate and power are set using regigister NRF24L01_06_RF_SETUP
		NRF24L01_SetBitrate(NRF24L01_BR_250K);
		NRF24L01_SetPower(Model.tx_power);
		// Writing to the STATUS register clears the specified interrupt bits
		NRF24L01_WriteReg(NRF24L01_07_STATUS, BV(NRF24L01_07_RX_DR) | BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT));
		// NRF24L01_08_OBSERVE_TX is read only register
		// NRF24L01_09_RPD is read only register (called RPD for NRF24L01+, CD for NRF24L01)
		NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr_bind, RX_TX_ADDR_LEN); // RX_ADDR_P0 must equal TX_ADDR for auto ACK
		// RX_ADDR for pipes P1-P5 (registers 0B to 0F) aret left at default values
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr_bind, RX_TX_ADDR_LEN);
		// Payload Widths for P0-P5 (registers 11 to 16) aret left at default values
		// NRF24L01_17_FIFO_STATUS is read only register for all non-reserved bits
		// No registers with values 18 to 1B
	#ifdef USE_AUTO_ACKKNOWLEDGEMENT
		NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
		NRF24L01_Activate(0x73); // Activate feature register, needed for NRF24L01 (harmless for NRF24L01+)
		NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
		NRF24L01_WriteReg(NRF24L01_1C_DYNPD, BV(NRF24L01_1C_DYNPD_P0)); // dynamic payload length on pipes P0
		NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF24L01_1D_EN_ACK_PAY) | BV(NRF24L01_1D_EN_DPL));
	#endif

		NRF24L01_Activate(0x53); // switch bank back

		NRF24L01_SetTxRxMode(TX_EN); // enter transmit mode, sets up NRF24L01_00_CONFIG register
	}

	// Generate address to use from TX id and manufacturer id (STM32 unique id)
	static void initialize_rx_tx_addr(void)
	{
		uint32_t lfsr = 0xb2c54a2ful;

	#ifndef USE_FIXED_MFGID
		uint8_t var[12];
		MCU_SerialNumber(var, 12);
		for (int i = 0; i < 12; ++i) {
			rand32_r(&lfsr, var[i]);
		}
	#endif

		if (Model.fixed_id) {
		   for (uint8_t i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
			   rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
		}
		// Pump zero bytes for LFSR to diverge more
		for (uint8_t i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

		for (uint8_t i = 0; i < sizeof(rx_tx_addr)-1; ++i) {
			rx_tx_addr[i] = lfsr & 0xff;
			rand32_r(&lfsr, i);
		}
		rx_tx_addr[1] &= 0x7f; // clear top bit so saved Model.proto_opts value is positive
		rx_tx_addr[3] &= 0x7f; // clear top bit so saved Model.proto_opts value is positive
		rx_tx_addr[4] = RX_TX_ADDR_4; // set to constant, so variable part of rx_tx_addr can be stored in 32-bit value
	}

	// The hopping channels are determined by the value of rx_tx_addr[0]
	static void set_hopping_channels(void)
	{
		rf_channel_index = 0;
		const uint8_t addr = rx_tx_addr[0];
		uint8_t ch = 0x10 + (addr & 0x07);
		for (int i = 0; i < rf_channel_count; ++i) {
			rf_channels[i] = ch;
			ch += 0x0c;
		}
	}

	static void update_telemetry(void)
	{
	#ifdef INAV_TELEMETRY
		const uint8_t observeTx = NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX);
		const uint8_t lostPacketCount = (observeTx & NRF24L01_08_PLOS_CNT_MASK) >> 4;
	//    const uint8_t autoRetryCount = observeTx & NRF24L01_08_ARC_CNT_MASK;
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_channel_index]); // reset packet loss counter

		lost_packet_counter += lostPacketCount;
		Telemetry.value[TELEM_LTM_STATUS_RSSI] = 100 - 100 * lost_packet_counter / packet_count;
		TELEMETRY_SetUpdated(TELEM_LTM_STATUS_RSSI);

		// ackPayload contains telemetry in LTM format.
		// See https://github.com/stronnag/mwptools/blob/master/docs/ltm-definition.txt
		// ackPayload[0] is a sequence count, LTM begins at ackPayload[2]
		const uint8_t *ltm = &ackPayload[2];
		switch (ltm[0]) { // frame type
		case 'G': // GPS frame
			{
			// LTM lat/long: int32 decimal degrees * 10,000,000 (1E7)
			// LTM 1 degree = 1,000,000
			// Deviation longitude: +/-180degrees = +/- 180*60*60*1000; W if value<0, E if value>=0; -180degrees = 180degrees
			// Deviation latitude:   +/-90degrees =  +/- 90*60*60*1000; S if value<0, N if value>=0
			// Deviation 1 degree = 3,600,000
			// Scale factor = 36/10 = 18/5
			uint32_t latitude;
			memcpy(&latitude, &ltm[1], sizeof(uint32_t));
			Telemetry.gps.latitude = latitude * 18 / 5;
			TELEMETRY_SetUpdated(TELEM_GPS_LAT);

			uint32_t longitude;
			memcpy(&longitude, &ltm[5], sizeof(uint32_t));
			Telemetry.gps.longitude = longitude * 18 / 5;
			TELEMETRY_SetUpdated(TELEM_GPS_LONG);

			Telemetry.gps.velocity = ltm[9] * 1000; // LTM m/s; DEV mm/s
			TELEMETRY_SetUpdated(TELEM_GPS_SPEED);

			uint32_t altitude;
			memcpy(&altitude, &ltm[10], sizeof(uint32_t)); 
			Telemetry.gps.altitude = altitude * 10; // LTM:cm; DEV:mm
			TELEMETRY_SetUpdated(TELEM_GPS_ALT);

			Telemetry.gps.satcount = ltm[14] >> 2;
			TELEMETRY_SetUpdated(TELEM_GPS_SATCOUNT);
			}
			break;
		case 'A': // Attitude frame
			{
			const uint16_t heading = ltm[5] | (ltm[6] << 8);
			Telemetry.gps.heading = heading;
			TELEMETRY_SetUpdated(TELEM_GPS_HEADING);

			const s16 pitch = ltm[1] | (ltm[2] << 8);
			Telemetry.value[TELEM_LTM_ATTITUDE_PITCH] = pitch; // LTM:degrees [-180,180]
			TELEMETRY_SetUpdated(TELEM_LTM_ATTITUDE_PITCH);

			const s16 roll = ltm[3] | (ltm[4] << 8);
			Telemetry.value[TELEM_LTM_ATTITUDE_ROLL] = roll; // LTM:degrees [-180,180]
			TELEMETRY_SetUpdated(TELEM_LTM_ATTITUDE_ROLL);

			Telemetry.value[TELEM_LTM_ATTITUDE_YAW] = heading;
			TELEMETRY_SetUpdated(TELEM_LTM_ATTITUDE_YAW);
			}
			break;
		case 'S': // Status frame
			{
			const uint16_t vBat = ltm[1] | (ltm[2] << 8);
			Telemetry.value[TELEM_LTM_STATUS_VBAT] = (uint32_t)(vBat); // LTM:mV; DEV:V/10
			TELEMETRY_SetUpdated(TELEM_LTM_STATUS_VBAT);

			const uint16_t amps = ltm[3] | (ltm[4] << 8);
			Telemetry.value[TELEM_LTM_STATUS_CURRENT] = (uint32_t)(amps); // LTM:mA; DEV:A/10
			TELEMETRY_SetUpdated(TELEM_LTM_STATUS_CURRENT);

			Telemetry.value[TELEM_LTM_STATUS_AIRSPEED] = ltm[6];
			TELEMETRY_SetUpdated(TELEM_LTM_STATUS_AIRSPEED);
			}
			break;
		default:
			break;
		}
	#endif
	}

	void save_rx_tx_addr(void)
	{
		Model.proto_opts[PROTOOPTS_RX_TX_ADDR1] = rx_tx_addr[0] | ((uint16_t)rx_tx_addr[1] << 8);
		Model.proto_opts[PROTOOPTS_RX_TX_ADDR2] = rx_tx_addr[2] | ((uint16_t)rx_tx_addr[3] << 8);
	}

	void load_rx_tx_addr(void)
	{
		rx_tx_addr[0] = (Model.proto_opts[PROTOOPTS_RX_TX_ADDR1]) & 0xff;
		rx_tx_addr[1] = (Model.proto_opts[PROTOOPTS_RX_TX_ADDR1] >> 8) & 0xff;
		rx_tx_addr[2] = (Model.proto_opts[PROTOOPTS_RX_TX_ADDR2]) & 0xff;
		rx_tx_addr[3] = (Model.proto_opts[PROTOOPTS_RX_TX_ADDR2] >> 8) & 0xff;
		rx_tx_addr[4] = RX_TX_ADDR_4; // set to constant, so variable part of rx_tx_addr can be stored in 32-bit value
	}

	static void inav_init()
	{
		// check options before proceeding with initialiasation
		packet_length = INAV_PROTOCOL_PAYLOAD_SIZE_DEFAULT;
		rc_channel_count = RC_CHANNEL_COUNT_DEFAULT;
		rf_channel_count = RF_CHANNEL_COUNT_DEFAULT;
		if(IS_AUTOBIND_FLAG_on) {
			initialize_rx_tx_addr();
			save_rx_tx_addr();
		} else {
			load_rx_tx_addr();
		}

		packet_count = 0;
		set_hopping_channels();
	}

	static uint16_t inav_cb()
	{
		static uint16_t bind_counter;
		static uint8_t bind_acked;

		switch (phase) {
		case PHASE_INAV_INIT:
			phase = PHASE_INAV_BIND;
			bind_counter = INAV_BIND_COUNT;
			bind_acked = 0;
			return FIRST_INAV_PACKET_DELAY;
			break;

		case PHASE_INAV_BIND:
			if (bind_counter == 0) {
				set_data_phase();
				BIND_DONE;
			} else {
				if (packet_ack() == PKT_ACKED) {
					bind_acked = 1;
				}
				if (bind_acked == 1) {
					// bind packet acked, so set bind_counter to zero to enter data phase on next callback
					bind_counter = 0;
					return INAV_PACKET_PERIOD;
				}
				send_packet(BIND_INAV_PACKET);
				--bind_counter;
			}
			break;

		case PHASE_INAV_DATA:
			update_telemetry();
			/*#define PACKET_CHKTIME 500 // time to wait if packet not yet acknowledged or timed out    
			if (packet_ack() == PKT_PENDING) {
				return PACKET_CHKTIME; // packet send not yet complete
			}*/
			packet_ack();
			send_packet(DATA_INAV_PACKET);
			break;
		}
		return INAV_PACKET_PERIOD;
	}

	static uint8_t INAV_setup()
	{
		CLOCK_StopTimer();
		tx_power = Model.tx_power;
		ackPayloadSize = 0;
		memset(ackPayload, 0, NRF24L01_MAX_PAYLOAD_SIZE);
		inav_init(bind);
		init_nrf24l01();

	#ifdef INAV_TELEMETRY
		memset(&Telemetry, 0, sizeof(Telemetry));
		TELEMETRY_SetType(TELEM_LTM);
	#endif

		if(IS_AUTOBIND_FLAG_on) {
			phase = PHASE_INAV_INIT;
//			PROTOCOL_SetBindState(INAV_BIND_COUNT * INAV_PACKET_PERIOD / 1000);
		} else {
			set_data_phase();
//			BIND_DONE;
		}
		return INAV_INITIAL_WAIT;
	}
/*
	const void *INAV_Cmds(enum ProtoCmds cmd)
	{
		switch(cmd) {
			case PROTOCMD_INIT:  initialize(INIT_BIND); return 0;
			case PROTOCMD_DEINIT:
			case PROTOCMD_RESET:
				CLOCK_StopTimer();
				return (void *)(NRF24L01_Reset() ? 1L : -1L);
			//case PROTOCMD_CHECK_AUTOBIND: return 0; 
			case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
			case PROTOCMD_BIND:  initialize(INIT_BIND); return 0;
			case PROTOCMD_NUMCHAN: return (void *)((long)rc_channel_count);
			case PROTOCMD_DEFAULT_NUMCHAN: return (void *)((long)RC_CHANNEL_COUNT_DEFAULT);
			case PROTOCMD_CURRENT_ID: return 0;
			case PROTOCMD_GETOPTIONS: return inav_opts;
	#ifdef INAV_TELEMETRY
			case PROTOCMD_TELEMETRYSTATE: return (void *)(long)(Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
	#else
			case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
	#endif
			default: break;
		}
		return 0;
	}
*/
#endif

