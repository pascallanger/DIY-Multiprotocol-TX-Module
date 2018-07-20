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

// Most of this code was ported from theseankelly's related DeviationTX work.

#if defined(CFLIE_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CFLIE_BIND_COUNT 60

//=============================================================================
// CRTP (Crazy RealTime Protocol) Implementation
//=============================================================================

// Port IDs
enum {
    CRTP_PORT_CONSOLE = 0x00,
    CRTP_PORT_PARAM = 0x02,
    CRTP_PORT_SETPOINT = 0x03,
    CRTP_PORT_MEM = 0x04,
    CRTP_PORT_LOG = 0x05,
    CRTP_PORT_POSITION = 0x06,
    CRTP_PORT_SETPOINT_GENERIC = 0x07,
    CRTP_PORT_PLATFORM = 0x0D,
    CRTP_PORT_LINK = 0x0F,
};

// Channel definitions for the LOG port
enum {
    CRTP_LOG_CHAN_TOC = 0x00,
    CRTP_LOG_CHAN_SETTINGS = 0x01,
    CRTP_LOG_CHAN_LOGDATA = 0x02,
};

// Command definitions for the LOG port's TOC channel
enum {
    CRTP_LOG_TOC_CMD_ELEMENT = 0x00,
    CRTP_LOG_TOC_CMD_INFO = 0x01,
};

// Command definitions for the LOG port's CMD channel
enum {
    CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK = 0x00,
    CRTP_LOG_SETTINGS_CMD_APPEND_BLOCK = 0x01,
    CRTP_LOG_SETTINGS_CMD_DELETE_BLOCK = 0x02,
    CRTP_LOG_SETTINGS_CMD_START_LOGGING = 0x03,
    CRTP_LOG_SETTINGS_CMD_STOP_LOGGING = 0x04,
    CRTP_LOG_SETTINGS_CMD_RESET_LOGGING = 0x05,
};

// Log variables types
enum {
    LOG_UINT8 = 0x01,
    LOG_UINT16 = 0x02,
    LOG_UINT32 = 0x03,
    LOG_INT8 = 0x04,
    LOG_INT16 = 0x05,
    LOG_INT32 = 0x06,
    LOG_FLOAT = 0x07,
    LOG_FP16 = 0x08,
};

#define CFLIE_TELEM_LOG_BLOCK_ID            0x01
#define CFLIE_TELEM_LOG_BLOCK_PERIOD_10MS   50 // 50*10 = 500ms

// Setpoint type definitions for the generic setpoint channel
enum {
    CRTP_SETPOINT_GENERIC_STOP_TYPE = 0x00,
    CRTP_SETPOINT_GENERIC_VELOCITY_WORLD_TYPE = 0x01,
    CRTP_SETPOINT_GENERIC_Z_DISTANCE_TYPE = 0x02,
    CRTP_SETPOINT_GENERIC_CPPM_EMU_TYPE = 0x03,
};

static inline uint8_t crtp_create_header(uint8_t port, uint8_t channel)
{
    return ((port)&0x0F)<<4 | (channel & 0x03);
}

//=============================================================================
// End CRTP implementation
//=============================================================================

// Address size
#define TX_ADDR_SIZE 5

// Timeout for callback in uSec, 10ms=10000us for Crazyflie
#define PACKET_PERIOD 10000

#define MAX_PACKET_SIZE 32  // CRTP is 32 bytes

// CPPM CRTP supports up to 10 aux channels but deviation only
// supports a total of 12 channels. R,P,Y,T leaves 8 aux channels left
#define MAX_CPPM_AUX_CHANNELS 8

static uint8_t tx_payload_len = 0; // Length of the packet stored in packet
static uint8_t rx_payload_len = 0; // Length of the packet stored in rx_packet
static uint8_t rx_packet[MAX_PACKET_SIZE]; // For reading in ACK payloads

static uint16_t cflie_counter;
static uint32_t packet_counter;
static uint8_t data_rate;

enum {
    CFLIE_INIT_SEARCH = 0,
    CFLIE_INIT_CRTP_LOG,
    CFLIE_INIT_DATA,
    CFLIE_SEARCH,
    CFLIE_DATA
};

static uint8_t crtp_log_setup_state;
enum {
    CFLIE_CRTP_LOG_SETUP_STATE_INIT = 0,
    CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_INFO,
    CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_INFO,
    CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_ITEM,
    CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_ITEM,
    // It might be a good idea to add a state here
    // to send the command to reset the logging engine
    // to avoid log block ID conflicts. However, there
    // is not a conflict with the current defaults in
    // cfclient and I'd rather be able to log from the Tx
    // and cfclient simultaneously
    CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK,
    CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK,
    CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_START_BLOCK,
    CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_START_BLOCK,
    CFLIE_CRTP_LOG_SETUP_STATE_COMPLETE,
};

// State variables for the crtp_log_setup_state_machine
static uint8_t toc_size;             // Size of the TOC read from the crazyflie
static uint8_t next_toc_variable;    // State variable keeping track of the next var to read
static uint8_t vbat_var_id;          // ID of the vbatMV variable
static uint8_t extvbat_var_id;       // ID of the extVbatMV variable
static uint8_t rssi_var_id;          // ID of the RSSI variable

// Constants used for finding var IDs from the toc
static const char* pm_group_name = "pm";
static const char* vbat_var_name = "vbatMV";
static const uint8_t vbat_var_type = LOG_UINT16;
static const char* extvbat_var_name = "extVbatMV";
static const uint8_t extvbat_var_type = LOG_UINT16;
static const char* radio_group_name = "radio";
static const char* rssi_var_name = "rssi";
static const uint8_t rssi_var_type = LOG_UINT8;

// Repurposing DSM Telemetry fields
#define TELEM_CFLIE_INTERNAL_VBAT   TELEM_DSM_FLOG_VOLT2    // Onboard voltage
#define TELEM_CFLIE_EXTERNAL_VBAT   TELEM_DSM_FLOG_VOLT1    // Voltage from external pin (BigQuad)
#define TELEM_CFLIE_RSSI            TELEM_DSM_FLOG_FADESA   // Repurpose FADESA for RSSI

enum {
    PROTOOPTS_TELEMETRY = 0,
    PROTOOPTS_CRTP_MODE = 1,
    LAST_PROTO_OPT,
};

#define TELEM_OFF 0
#define TELEM_ON_ACKPKT 1
#define TELEM_ON_CRTPLOG 2

#define CRTP_MODE_RPYT 0
#define CRTP_MODE_CPPM 1

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define PACKET_CHKTIME 500      // time to wait if packet not yet acknowledged or timed out    

// Helper for sending a packet
// Assumes packet data has been put in packet
// and tx_payload_len has been set correctly
static void send_packet()
{
    // clear packet status bits and Tx/Rx FIFOs
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();

    // Transmit the payload
    NRF24L01_WritePayload(packet, tx_payload_len);

    ++packet_counter;

    // // Check and adjust transmission power.
    NRF24L01_SetPower();
}

static uint16_t dbg_cnt = 0;
static uint8_t packet_ack()
{
	if (++dbg_cnt > 50)
	{
		// debugln("S: %02x\n", NRF24L01_ReadReg(NRF24L01_07_STATUS));
		dbg_cnt = 0;
	}
	switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)))
	{
		case BV(NRF24L01_07_TX_DS):
			rx_payload_len = NRF24L01_GetDynamicPayloadSize();
			if (rx_payload_len > MAX_PACKET_SIZE)
				rx_payload_len = MAX_PACKET_SIZE;
			NRF24L01_ReadPayload(rx_packet, rx_payload_len);
			return PKT_ACKED;
		case BV(NRF24L01_07_MAX_RT):
			return PKT_TIMEOUT;
	}
	return PKT_PENDING;
}

static void set_rate_channel(uint8_t rate, uint8_t channel)
{
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel);		// Defined by model id
	NRF24L01_SetBitrate(rate);							// Defined by model id
}

static void send_search_packet()
{
	uint8_t buf[1];
	buf[0] = 0xff;
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();

	if (rf_ch_num++ > 125)
	{
		rf_ch_num = 0;
		switch(data_rate)
		{
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
	set_rate_channel(data_rate, rf_ch_num);

	NRF24L01_WritePayload(buf, sizeof(buf));

	++packet_counter;
}

// Frac 16.16
#define FRAC_MANTISSA 16 // This means, not IEEE 754...
#define FRAC_SCALE (1 << FRAC_MANTISSA)

// Convert fractional 16.16 to float32
static void frac2float(int32_t n, float* res)
{
	if (n == 0)
	{
		*res = 0.0;
		return;
	}
	uint32_t m = n < 0 ? -n : n; // Figure out mantissa?
	int i;
	for (i = (31-FRAC_MANTISSA); (m & 0x80000000) == 0; i--, m <<= 1);
	m <<= 1; // Clear implicit leftmost 1
	m >>= 9;
	uint32_t e = 127 + i;
	if (n < 0) m |= 0x80000000;
	m |= e << 23;
	*((uint32_t *) res) = m;
}

static void send_crtp_rpyt_packet()
{
	int32_t f_roll;
	int32_t f_pitch;
	int32_t f_yaw;
	uint16_t thrust;

	uint16_t val;

	struct CommanderPacketRPYT
	{
		float roll;
		float pitch;
		float yaw;
		uint16_t thrust;
	}__attribute__((packed)) cpkt;

	// Channels in AETR order
	// Roll, aka aileron, float +- 50.0 in degrees
	// float roll  = -(float) Channels[0]*50.0/10000;
	val = convert_channel_16b_limit(AILERON, -10000, 10000);
	// f_roll = -Channels[0] * FRAC_SCALE / (10000 / 50);
	f_roll = val * FRAC_SCALE / (10000 / 50);

	frac2float(f_roll, &cpkt.roll); // TODO: Remove this and use the correct Mode switch below...
	// debugln("Roll: raw, converted:  %d, %d, %d, %0.2f", Channel_data[AILERON], val, f_roll, cpkt.roll);

	// Pitch, aka elevator, float +- 50.0 degrees
	//float pitch = -(float) Channels[1]*50.0/10000;
	val = convert_channel_16b_limit(ELEVATOR, -10000, 10000);
	// f_pitch = -Channels[1] * FRAC_SCALE / (10000 / 50);
	f_pitch = -val * FRAC_SCALE / (10000 / 50);

	frac2float(f_pitch, &cpkt.pitch); // TODO: Remove this and use the correct Mode switch below...
	// debugln("Pitch: raw, converted:  %d, %d, %d, %0.2f", Channel_data[ELEVATOR], val, f_pitch, cpkt.pitch);

	// Thrust, aka throttle 0..65535, working range 5535..65535
	// Android Crazyflie app puts out a throttle range of 0-80%: 0..52000
	thrust = convert_channel_16b_limit(THROTTLE, 0, 32767) * 2;

	// Crazyflie needs zero thrust to unlock
	if (thrust < 900)
		cpkt.thrust = 0;
	else
		cpkt.thrust = thrust;

	// debugln("Thrust: raw, converted:  %d, %u, %u", Channel_data[THROTTLE], thrust, cpkt.thrust);

	// Yaw, aka rudder, float +- 400.0 deg/s
	// float yaw   = -(float) Channels[3]*400.0/10000;
	val = convert_channel_16b_limit(RUDDER, -10000, 10000);
	// f_yaw = - Channels[3] * FRAC_SCALE / (10000 / 400);
	f_yaw = val * FRAC_SCALE / (10000 / 400);
	frac2float(f_yaw, &cpkt.yaw);

	// debugln("Yaw: raw, converted:  %d, %d, %d, %0.2f", Channel_data[RUDDER], val, f_yaw, cpkt.yaw);

	// Switch on/off?
	// TODO: Get X or + mode working again:
	// if (Channels[4] >= 0) {
	//     frac2float(f_roll, &cpkt.roll);
	//     frac2float(f_pitch, &cpkt.pitch);
	// } else {
	//     // Rotate 45 degrees going from X to + mode or opposite.
	//     // 181 / 256 = 0.70703125 ~= sqrt(2) / 2
	//     int32_t f_x_roll = (f_roll + f_pitch) * 181 / 256;
	//     frac2float(f_x_roll, &cpkt.roll);
	//     int32_t f_x_pitch = (f_pitch - f_roll) * 181 / 256;
	//     frac2float(f_x_pitch, &cpkt.pitch);
	// }

	// Construct and send packet
	packet[0] = crtp_create_header(CRTP_PORT_SETPOINT, 0); // Commander packet to channel 0
	memcpy(&packet[1], (char*) &cpkt, sizeof(cpkt));
	tx_payload_len = 1 + sizeof(cpkt);
	send_packet();
}

/*static void send_crtp_cppm_emu_packet()
{
    struct CommanderPacketCppmEmu {
        struct {
            uint8_t numAuxChannels : 4; // Set to 0 through MAX_AUX_RC_CHANNELS
            uint8_t reserved : 4;
        } hdr;
        uint16_t channelRoll;
        uint16_t channelPitch;
        uint16_t channelYaw;
        uint16_t channelThrust;
        uint16_t channelAux[10];
    } __attribute__((packed)) cpkt;

    // To emulate PWM RC signals, rescale channels from (-10000,10000) to (1000,2000)
    // This is done by dividing by 20 to get a total range of 1000 (-500,500)
    // and then adding 1500 to to rebase the offset
#define RESCALE_RC_CHANNEL_TO_PWM(chan) ((chan / 20) + 1500)

    // Make sure the number of aux channels in use is capped to MAX_CPPM_AUX_CHANNELS
    // uint8_t numAuxChannels = Model.num_channels - 4;
    uint8_t numAuxChannels = 2; // TODO: Figure this out correctly
    if(numAuxChannels > MAX_CPPM_AUX_CHANNELS)
    {
        numAuxChannels = MAX_CPPM_AUX_CHANNELS;
    }

    cpkt.hdr.numAuxChannels = numAuxChannels;

    // Remap AETR to AERT (RPYT)
    cpkt.channelRoll = convert_channel_16b_limit(AILERON,1000,2000);
    cpkt.channelPitch = convert_channel_16b_limit(ELEVATOR,1000,2000);
    // Note: T & R Swapped:
    cpkt.channelYaw = convert_channel_16b_limit(RUDDER, 1000, 2000);
    cpkt.channelThrust = convert_channel_16b_limit(THROTTLE, 1000, 2000);

    // Rescale the rest of the aux channels - RC channel 4 and up
    for (uint8_t i = 4; i < 14; i++)
    {
        cpkt.channelAux[i] = convert_channel_16b_limit(i, 1000, 2000);
    }

    // Total size of the commander packet is a 1-byte header, 4 2-byte channels and
    // a variable number of 2-byte auxiliary channels
    uint8_t commanderPacketSize = 1 + 8 + (2*numAuxChannels);

    // Construct and send packet
    packet[0] = crtp_create_header(CRTP_PORT_SETPOINT_GENERIC, 0); // Generic setpoint packet to channel 0
    packet[1] = CRTP_SETPOINT_GENERIC_CPPM_EMU_TYPE;

    // Copy the header (1) plus 4 2-byte channels (8) plus whatever number of 2-byte aux channels are in use
    memcpy(&packet[2], (char*)&cpkt, commanderPacketSize); // Why not use sizeof(cpkt) here??
    tx_payload_len = 2 + commanderPacketSize; // CRTP header, commander type, and packet
    send_packet();
}*/

static void send_cmd_packet()
{
    // TODO: Fix this so we can actually configure the packet type
    // switch(Model.proto_opts[PROTOOPTS_CRTP_MODE])
    // {
    // case CRTP_MODE_CPPM:
    //     send_crtp_cppm_emu_packet();
    //     break;
    // case CRTP_MODE_RPYT:
    //     send_crtp_rpyt_packet();
    //     break;
    // default:
    //     send_crtp_rpyt_packet();
    // }

    // send_crtp_cppm_emu_packet(); // oh maAAAn
    send_crtp_rpyt_packet();
}

// State machine for setting up CRTP logging
// returns 1 when the state machine has completed, 0 otherwise
static uint8_t crtp_log_setup_state_machine()
{
    uint8_t state_machine_completed = 0;
    // A note on the design of this state machine:
    //
    // Responses from the crazyflie come in the form of ACK payloads.
    // There is no retry logic associated with ACK payloads, so it is possible
    // to miss a response from the crazyflie. To avoid this, the request
    // packet must be re-sent until the expected response is received. However,
    // re-sending the same request generates another response in the crazyflie
    // Rx queue, which can produce large backlogs of duplicate responses.
    //
    // To avoid this backlog but still guard against dropped ACK payloads,
    // transmit cmd packets (which don't generate responses themselves)
    // until an empty ACK payload is received (the crazyflie alternates between
    // 0xF3 and 0xF7 for empty ACK payloads) which indicates the Rx queue on the
    // crazyflie has been drained. If the queue has been drained and the
    // desired ACK has still not been received, it was likely dropped and the
    // request should be re-transmit.

    switch (crtp_log_setup_state) {
    case CFLIE_CRTP_LOG_SETUP_STATE_INIT:
        toc_size = 0;
        next_toc_variable = 0;
        vbat_var_id = 0;
        extvbat_var_id = 0;
        crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_INFO;
        // fallthrough
    case CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_INFO:
        crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_INFO;
        packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC);
        packet[1] = CRTP_LOG_TOC_CMD_INFO;
        tx_payload_len = 2;
        send_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_INFO:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 3
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC)
                    && rx_packet[1] == CRTP_LOG_TOC_CMD_INFO) {
                // Received the ACK payload. Save the toc_size
                // and advance to the next state
                toc_size = rx_packet[2];
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_ITEM;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_INFO;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_ITEM:
        crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_ITEM;
        packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC);
        packet[1] = CRTP_LOG_TOC_CMD_ELEMENT;
        packet[2] = next_toc_variable;
        tx_payload_len = 3;
        send_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_ACK_CMD_GET_ITEM:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 3
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_TOC)
                    && rx_packet[1] == CRTP_LOG_TOC_CMD_ELEMENT
                    && rx_packet[2] == next_toc_variable) {
                // For every element in the TOC we must compare its
                // type (rx_packet[3]), group and name (back to back
                // null terminated strings starting with the fifth byte)
                // and see if it matches any of the variables we need
                // for logging
                //
                // Currently enabled for logging:
                //  - vbatMV (LOG_UINT16)
                //  - extVbatMV (LOG_UINT16)
                //  - rssi (LOG_UINT8)
                if(rx_packet[3] == vbat_var_type
                        && (0 == strcmp((char*)&rx_packet[4], pm_group_name))
                        && (0 == strcmp((char*)&rx_packet[4 + strlen(pm_group_name) + 1], vbat_var_name))) {
                    // Found the vbat element - save it for later
                    vbat_var_id = next_toc_variable;
                }

                if(rx_packet[3] == extvbat_var_type
                        && (0 == strcmp((char*)&rx_packet[4], pm_group_name))
                        && (0 == strcmp((char*)&rx_packet[4 + strlen(pm_group_name) + 1], extvbat_var_name))) {
                    // Found the extvbat element - save it for later
                    extvbat_var_id = next_toc_variable;
                }

                if(rx_packet[3] == rssi_var_type
                        && (0 == strcmp((char*)&rx_packet[4], radio_group_name))
                        && (0 == strcmp((char*)&rx_packet[4 + strlen(radio_group_name) + 1], rssi_var_name))) {
                    // Found the rssi element - save it for later
                    rssi_var_id = next_toc_variable;
                }

                // Advance the toc variable counter
                // If there are more variables, read them
                // If not, move on to the next state
                next_toc_variable += 1;
                if(next_toc_variable >= toc_size) {
                    crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK;
                } else {
                    // There are more TOC elements to get
                    crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_ITEM;
                }
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CMD_GET_INFO;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK:
        crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK;
        packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS);
        packet[1] = CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK;
        packet[2] = CFLIE_TELEM_LOG_BLOCK_ID; // Log block ID
        packet[3] = vbat_var_type; // Variable type
        packet[4] = vbat_var_id; // ID of the VBAT variable
        packet[5] = extvbat_var_type; // Variable type
        packet[6] = extvbat_var_id; // ID of the ExtVBat variable
        packet[7] = rssi_var_type; // Variable type
        packet[8] = rssi_var_id; // ID of the RSSI variable
        tx_payload_len = 9;
        send_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_CREATE_BLOCK:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 2
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS)
                    && rx_packet[1] == CRTP_LOG_SETTINGS_CMD_CREATE_BLOCK) {
                // Received the ACK payload. Advance to the next state
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_START_BLOCK;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_CREATE_BLOCK;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_START_BLOCK:
        crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_START_BLOCK;
        packet[0] = crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS);
        packet[1] = CRTP_LOG_SETTINGS_CMD_START_LOGGING;
        packet[2] = CFLIE_TELEM_LOG_BLOCK_ID; // Log block ID 1
        packet[3] = CFLIE_TELEM_LOG_BLOCK_PERIOD_10MS; // Log frequency in 10ms units
        tx_payload_len = 4;
        send_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_ACK_CONTROL_START_BLOCK:
        if (packet_ack() == PKT_ACKED) {
            if (rx_payload_len >= 2
                    && rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_SETTINGS)
                    && rx_packet[1] == CRTP_LOG_SETTINGS_CMD_START_LOGGING) {
                // Received the ACK payload. Advance to the next state
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_COMPLETE;
                return state_machine_completed;
            } else if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
                // "empty" ACK packet received - likely missed the ACK
                // payload we are waiting for.
                // return to the send state and retransmit the request
                crtp_log_setup_state =
                        CFLIE_CRTP_LOG_SETUP_STATE_SEND_CONTROL_START_BLOCK;
                return state_machine_completed;
            }
        }

        // Otherwise, send a cmd packet to get the next ACK in the Rx queue
        send_cmd_packet();
        break;

    case CFLIE_CRTP_LOG_SETUP_STATE_COMPLETE:
        state_machine_completed = 1;
        return state_machine_completed;
        break;
    }

    return state_machine_completed;
}

static int cflie_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x01);              // Auto Acknowledgement for data pipe 0
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);          // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, TX_ADDR_SIZE-2); // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x13);         // 3 retransmits, 500us delay

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num);        // Defined in initialize_rx_tx_addr
    NRF24L01_SetBitrate(data_rate);                          // Defined in initialize_rx_tx_addr

    NRF24L01_SetPower();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);             // Clear data ready, data sent, and retransmit

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, TX_ADDR_SIZE);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, TX_ADDR_SIZE);

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);

    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x01);       // Enable Dynamic Payload Length on pipe 0
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x06);     // Enable Dynamic Payload Length, enable Payload with ACK

    // 50ms delay in callback
    return 50000;
}

// TODO: Fix telemetry

// Update telemetry using the CRTP logging framework
// static void update_telemetry_crtplog()
// {
//     static uint8_t frameloss = 0;

//     // Read and reset count of dropped packets
//     frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
//     NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num); // reset packet loss counter
//     Telemetry.value[TELEM_DSM_FLOG_FRAMELOSS] = frameloss;
//     TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);

//     if (packet_ack() == PKT_ACKED) {
//         // See if the ACK packet is a cflie log packet
//         // A log data packet is a minimum of 5 bytes. Ignore anything less.
//         if (rx_payload_len >= 5) {
//             // Port 5 = log, Channel 2 = data
//             if (rx_packet[0] == crtp_create_header(CRTP_PORT_LOG, CRTP_LOG_CHAN_LOGDATA)) {
//                 // The log block ID
//                 if (rx_packet[1] == CFLIE_TELEM_LOG_BLOCK_ID) {
//                     // Bytes 5 and 6 are the Vbat in mV units
//                     uint16_t vBat;
//                     memcpy(&vBat, &rx_packet[5], sizeof(uint16_t));
//                     Telemetry.value[TELEM_CFLIE_INTERNAL_VBAT] = (int32_t) (vBat / 10); // The log value expects centivolts
//                     TELEMETRY_SetUpdated(TELEM_CFLIE_INTERNAL_VBAT);

//                     // Bytes 7 and 8 are the ExtVbat in mV units
//                     uint16_t extVBat;
//                     memcpy(&extVBat, &rx_packet[7], sizeof(uint16_t));
//                     Telemetry.value[TELEM_CFLIE_EXTERNAL_VBAT] = (int32_t) (extVBat / 10); // The log value expects centivolts
//                     TELEMETRY_SetUpdated(TELEM_CFLIE_EXTERNAL_VBAT);

//                     // Byte 9 is the RSSI
//                     Telemetry.value[TELEM_CFLIE_RSSI] = rx_packet[9];
//                     TELEMETRY_SetUpdated(TELEM_CFLIE_RSSI);
//                 }
//             }
//         }
//     }
// }

// // Update telemetry using the ACK packet payload
// static void update_telemetry_ackpkt()
// {
//     static uint8_t frameloss = 0;

//     // Read and reset count of dropped packets
//     frameloss += NRF24L01_ReadReg(NRF24L01_08_OBSERVE_TX) >> 4;
//     NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num); // reset packet loss counter
//     Telemetry.value[TELEM_DSM_FLOG_FRAMELOSS] = frameloss;
//     TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);

//     if (packet_ack() == PKT_ACKED) {
//         // Make sure this is an ACK packet (first byte will alternate between 0xF3 and 0xF7
//         if (rx_packet[0] == 0xF3 || rx_packet[0] == 0xF7) {
//             // If ACK packet contains RSSI (proper length and byte 1 is 0x01)
//             if(rx_payload_len >= 3 && rx_packet[1] == 0x01) {
//                 Telemetry.value[TELEM_CFLIE_RSSI] = rx_packet[2];
//                 TELEMETRY_SetUpdated(TELEM_CFLIE_RSSI);
//             }
//             // If ACK packet contains VBAT (proper length and byte 3 is 0x02)
//             if(rx_payload_len >= 8 && rx_packet[3] == 0x02) {
//                 uint32_t vBat = 0;
//                 memcpy(&vBat, &rx_packet[4], sizeof(uint32_t));
//                 Telemetry.value[TELEM_CFLIE_INTERNAL_VBAT] = (int32_t)(vBat / 10); // The log value expects centivolts
//                 TELEMETRY_SetUpdated(TELEM_CFLIE_INTERNAL_VBAT);
//             }
//         }
//     }
// }

static uint16_t cflie_callback()
{
    switch (phase) {
    case CFLIE_INIT_SEARCH:
        send_search_packet();
        phase = CFLIE_SEARCH;
        break;
    case CFLIE_INIT_CRTP_LOG:
        if (crtp_log_setup_state_machine()) {
            phase = CFLIE_INIT_DATA;
        }
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
            // PROTOCOL_SetBindState(0);
            // MUSIC_Play(MUSIC_DONE_BINDING);
            BIND_DONE;
            break;
        case PKT_TIMEOUT:
            send_search_packet();
            cflie_counter = CFLIE_BIND_COUNT;
        }
        break;

    case CFLIE_DATA:
        // if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON_CRTPLOG) {
        //     update_telemetry_crtplog();
        // } else if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON_ACKPKT) {
        //     update_telemetry_ackpkt();
        // }

        if (packet_ack() == PKT_PENDING)
            return PACKET_CHKTIME;         // packet send not yet complete
        send_cmd_packet();
        break;
    }
    return PACKET_PERIOD;                  // Packet at standard protocol interval
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static uint8_t initialize_rx_tx_addr()
{
    rx_tx_addr[0] = 
    rx_tx_addr[1] = 
    rx_tx_addr[2] = 
    rx_tx_addr[3] = 
    rx_tx_addr[4] = 0xE7; // CFlie uses fixed address

    // if (Model.fixed_id) {
    //     rf_ch_num = Model.fixed_id % 100;
    //     switch (Model.fixed_id / 100) {
    //     case 0:
    //         data_rate = NRF24L01_BR_250K;
    //         break;
    //     case 1:
    //         data_rate = NRF24L01_BR_1M;
    //         break;
    //     case 2:
    //         data_rate = NRF24L01_BR_2M;
    //         break;
    //     default:
    //         break;
    //     }

    //     if (Model.proto_opts[PROTOOPTS_TELEMETRY] == TELEM_ON_CRTPLOG) {
    //         return CFLIE_INIT_CRTP_LOG;
    //     } else {
    //         return CFLIE_INIT_DATA;
    //     }
    // } else {
    //     data_rate = NRF24L01_BR_250K;
    //     rf_ch_num = 10;
    //     return CFLIE_INIT_SEARCH;
    // }

    // Default 1
    data_rate = NRF24L01_BR_1M;
    rf_ch_num = 10;

    // Default 2
    // data_rate = NRF24L01_BR_2M;
    // rf_ch_num = 110;
    return CFLIE_INIT_SEARCH;
}

uint16_t initCFlie(void)
{
	BIND_IN_PROGRESS;	// autobind protocol

    phase = initialize_rx_tx_addr();
    crtp_log_setup_state = CFLIE_CRTP_LOG_SETUP_STATE_INIT;
    packet_count=0;

    int delay = cflie_init();

    // debugln("CFlie init!");

	return delay;
}

#endif