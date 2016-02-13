/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

/* NB: Not implemented
   Uncomment define below to enable telemetry. Also add CFlie protocol to TELEMETRY_SetTypeByProtocol to set type to DSM.
#define CFLIE_TELEMETRY
   */


#if defined(CFlie_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_COUNT 60

// Address size
#define TX_ADDR_SIZE 5

// Timeout for callback in uSec, 10ms=10000us for Crazyflie
#define PACKET_PERIOD 10000


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
    CHANNEL10
};

#define PAYLOADSIZE 8       // receive data pipes set to this size, but unused
#define MAX_PACKET_SIZE 9   // YD717 packets have 8-byte payload, Syma X4 is 9

//static uint8_t packet[MAX_PACKET_SIZE];

static uint8_t data_rate, rf_channel;

enum {
    CFLIE_INIT_SEARCH = 0,
    CFLIE_INIT_DATA,
    CFLIE_SEARCH,
    CFLIE_DATA
};

#ifdef CFLIE_TELEMETRY
static const char * const cflie_opts[] = {
  _tr_noop("Telemetry"),  _tr_noop("Off"), _tr_noop("On"), NULL,
  NULL
};
enum {
    PROTOOPTS_TELEMETRY = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_OFF 0
#define TELEM_ON 1
#endif

#define PACKET_CHKTIME 500      // time to wait if packet not yet acknowledged or timed out    

static uint16_t dbg_cnt = 0;
static uint8_t packet_ack() {
	if (++dbg_cnt > 50) { dbg_cnt = 0; }
	switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
		case BV(NRF24L01_07_TX_DS):
			return PKT_ACKED;
		case BV(NRF24L01_07_MAX_RT):
			return PKT_TIMEOUT;
	}
	return PKT_PENDING;
}

static void set_rate_channel(uint8_t rate, uint8_t channel) {
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel);     // Defined by model id
	NRF24L01_SetBitrate(rate);             // Defined by model id
}

static void send_search_packet() {
	uint8_t buf[1];
	buf[0] = 0xff;
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();

	if (rf_channel++ > 125) {
		rf_channel = 0;
		switch(data_rate) {
			case NRF24L01_BR_250K:
				data_rate = NRF24L01_BR_1M;
				break;
			case NRF24L01_BR_1M:
				data_rate = NRF24L01_BR_2M;
				break;
			case NRF24L01_BR_2M:
				data_rate = NRF24L01_BR_250K;
				break;
		}
	}

	set_rate_channel(data_rate, rf_channel);

	NRF24L01_WritePayload(buf, sizeof(buf));

	++packet_counter;
}

// Frac 16.16
#define FRAC_MANTISSA 16
#define FRAC_SCALE (1 << FRAC_MANTISSA)

// Convert fractional 16.16 to float32
static void frac2float(uint32_t n, float* res) {
	if (n == 0) {
		*res = 0.0;
		return;
	}
	uint32_t m = n < 0 ? -n : n;
	int i;
	for (i = (31-FRAC_MANTISSA); (m & 0x80000000) == 0; i--, m <<= 1) ;
	m <<= 1; // Clear implicit leftmost 1
	m >>= 9;
	uint32_t e = 127 + i;
	if (n < 0) m |= 0x80000000;
	m |= e << 23;
	*((uint32_t *) res) = m;
}

static void send_cmd_packet() {
	// Commander packet, 15 bytes
	uint8_t buf[15];
	float x_roll, x_pitch, yaw;

	// Channels in AETR order

	// Roll, aka aileron, float +- 50.0 in degrees
	// float roll  = -(float) Servo_data[AILERON]*50.0/10000;
	uint32_t f_roll = -Servo_data[AILERON] * FRAC_SCALE / (10000 / 50);

	// Pitch, aka elevator, float +- 50.0 degrees
	//float pitch = -(float) Servo_data[ELEVATOR]*50.0/10000;
	uint32_t f_pitch = -Servo_data[ELEVATOR] * FRAC_SCALE / (10000 / 50);

	// Thrust, aka throttle 0..65535, working range 5535..65535
	// No space for overshoot here, hard limit Channel3 by -10000..10000
	uint32_t ch = Servo_data[THROTTLE];
	if (ch < PPM_MIN) {
		ch = PPM_MIN;
	} else if (ch > PPM_MAX) {
		ch = PPM_MAX;
	}
	uint16_t thrust  = ch*3L + 35535L;

	// Yaw, aka rudder, float +- 400.0 deg/s
	// float yaw   = -(float) Servo_data[RUDDER]*400.0/10000;
	uint32_t f_yaw = - Servo_data[RUDDER] * FRAC_SCALE / (10000 / 400);
	frac2float(f_yaw, &yaw);

	// Convert + to X. 181 / 256 = 0.70703125 ~= sqrt(2) / 2
	uint32_t f_x_roll = (f_roll + f_pitch) * 181 / 256;
	frac2float(f_x_roll, &x_roll);
	uint32_t f_x_pitch = (f_pitch - f_roll) * 181 / 256;
	frac2float(f_x_pitch, &x_pitch);

	int bufptr = 0;
	buf[bufptr++] = 0x30; // Commander packet to channel 0
	memcpy(&buf[bufptr], (char*) &x_roll, 4); bufptr += 4;
	memcpy(&buf[bufptr], (char*) &x_pitch, 4); bufptr += 4;
	memcpy(&buf[bufptr], (char*) &yaw, 4); bufptr += 4;
	memcpy(&buf[bufptr], (char*) &thrust, 2); bufptr += 2;


	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();

	NRF24L01_WritePayload(buf, sizeof(buf));

	++packet_counter;

	NRF24L01_SetPower();
}

static int cflie_init() {
	NRF24L01_Initialize();

	// CRC, radio on
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP)); 
	//    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);              // No Auto Acknowledgement
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x01);              // Auto Acknowledgement for data pipe 0
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);          // Enable data pipe 0
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, TX_ADDR_SIZE-2); // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x13);         // 3 retransmits, 500us delay

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);        // Defined by model id
	NRF24L01_SetBitrate(data_rate);                           // Defined by model id

	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);             // Clear data ready, data sent, and retransmit

	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00);  // Just in case, no real bits to write here

	// this sequence necessary for module from stock tx
	NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x01);       // Enable Dynamic Payload Length on pipe 0
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x06);     // Enable Dynamic Payload Length, enable Payload with ACK


	//    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, TX_ADDR_SIZE);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, TX_ADDR_SIZE);

	NRF24L01_Activate(0x53); // switch bank back

	// 50ms delay in callback
	return 50000;
}


#ifdef CFLIE_TELEMETRY
static void update_telemetry() {
  static uint8_t frameloss = 0;

  frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
  NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channel);   // reset packet loss counter

  Telemetry.p.dsm.flog.frameloss = frameloss;
//  Telemetry.p.dsm.flog.volt[0] = read battery voltage from ack payload
  TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);
}
#endif


static uint16_t cflie_callback() {
	switch (phase) {
		case CFLIE_INIT_SEARCH:
			send_search_packet();
			phase = CFLIE_SEARCH;
			break;
		case CFLIE_INIT_DATA:
			send_cmd_packet();
			phase = CFLIE_DATA;
			break;
		case CFLIE_SEARCH:
			switch (packet_ack()) {
				case PKT_PENDING:
					return PACKET_CHKTIME;                 // packet send not yet complete
				case PKT_ACKED:
					phase = CFLIE_DATA;
					BIND_DONE;
					break;
				case PKT_TIMEOUT:
					send_search_packet();
					counter = BIND_COUNT;
			}
			break;
		case CFLIE_DATA:
			#ifdef CFLIE_TELEMETRY
				update_telemetry();
			#endif
			if (packet_ack() == PKT_PENDING)
				return PACKET_CHKTIME;         // packet send not yet complete
			send_cmd_packet();
			break;
	}
	return PACKET_PERIOD;                  // Packet at standard protocol interval
}


// Generate address to use from TX id and manufacturer id (STM32 unique id)
static uint8_t initialize_rx_tx_addr() {
	rx_tx_addr[0] = 
	rx_tx_addr[1] = 
	rx_tx_addr[2] = 
	rx_tx_addr[3] = 
	rx_tx_addr[4] = 0xE7; // CFlie uses fixed address

		data_rate = NRF24L01_BR_250K;
		rf_channel = 0;
		return CFLIE_INIT_SEARCH;
		//        return CFLIE_INIT_DATA;
}

static uint16_t Cflie_setup() {
	phase = initialize_rx_tx_addr();
	packet_counter = 0;

	int delay = cflie_init();

	#ifdef CFLIE_TELEMETRY
		memset(&Telemetry, 0, sizeof(Telemetry));
		TELEMETRY_SetType(TELEM_DSM);
	#endif
	if (phase == CFLIE_INIT_SEARCH) { BIND_IN_PROGRESS; }
	return delay;
}

#endif
