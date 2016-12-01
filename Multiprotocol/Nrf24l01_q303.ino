#ifdef Q303_NRF24L01_INO

#include "iface_nrf24l01.h"

#define Q303_BIND_COUNT 2335

#define Q303_PACKET_SIZE  10
#define Q303_PACKET_PERIOD   1500  // Timeout for callback in uSec
#define Q303_INITIAL_WAIT     500
#define Q303_RF_BIND_CHANNEL 0x02
#define Q303_NUM_RF_CHANNELS    4

static uint8_t tx_addr[5];
static uint8_t current_chan;
/*
static const struct {
    u8 txid[sizeof(txid)];
    u8 rfchan[NUM_RF_CHANNELS];
} q303_tx_rf_map[] =  {
	{{0xb8, 0x69, 0x64, 0x67}, {0x48, 0x4a, 0x4c, 0x46}}, // tx2
	{{0xAE, 0x89, 0x97, 0x87}, {0x4A, 0x4C, 0x4E, 0x48}}
}; // tx1
*/
uint8_t txid[4] = {0xAE, 0x89, 0x97, 0x87};
static uint8_t rf_chans[4] = {0x4A, 0x4C, 0x4E, 0x48};

// haven't figured out txid<-->rf channel mapping yet
//	static const struct {    uint8_t txid[sizeof(txid)];    uint8_t rfchan[Q303_NUM_RF_CHANNELS];	} q303_tx_rf_map[] = {{{0xAE, 0x89, 0x97, 0x87}, {0x4A, 0x4C, 0x4E, 0x48}}};

enum {
    Q303_BIND,
    Q303_DATA
};

// flags going to packet[8]
#define Q303_FLAG_HIGHRATE 0x03
#define Q303_FLAG_AHOLD    0x40
#define Q303_FLAG_RTH      0x80

// flags going to packet[9]
#define Q303_FLAG_GIMBAL_DN 0x04
#define Q303_FLAG_GIMBAL_UP 0x20
#define Q303_FLAG_HEADLESS  0x08
#define Q303_FLAG_FLIP      0x80
#define Q303_FLAG_SNAPSHOT  0x10
#define Q303_FLAG_VIDEO     0x01

static void send_packet(uint8_t bind)
{
	if(bind) {
		packet[0] = 0xaa;
		memcpy(&packet[1], txid, 4);
		memset(&packet[5], 0, 5);
	}
	else {
		aileron  = 1000 -	map(Servo_data[AILERON], 1000, 2000, 0, 1000);
		elevator = 1000 - 	map(Servo_data[ELEVATOR], 1000, 2000, 0, 1000);
		throttle = 			map(Servo_data[THROTTLE], 1000, 2000, 0, 1000);
		rudder   = 		 	map(Servo_data[RUDDER], 1000, 2000, 0, 1000);

		packet[0] = 0x55;
		packet[1] = aileron >> 2     ;     // 8 bits
		packet[2] = (aileron & 0x03) << 6  // 2 bits
				| (elevator >> 4);       // 6 bits
		packet[3] = (elevator & 0x0f) << 4 // 4 bits
				| (throttle >> 6);       // 4 bits
		packet[4] = (throttle & 0x3f) << 2 // 6 bits 
				| (rudder >> 8);         // 2 bits
		packet[5] = rudder & 0xff;         // 8 bits
		packet[6] = 0x10; // trim(s) ?
		packet[7] = 0x10; // trim(s) ?
		packet[8] = 0x03  // high rate (0-3)
				| GET_FLAG(Servo_AUX1, Q303_FLAG_AHOLD)
				| GET_FLAG(Servo_AUX6, Q303_FLAG_RTH);
		packet[9] = 0x40 // always set
				| GET_FLAG(Servo_AUX5, Q303_FLAG_HEADLESS)
				| GET_FLAG(Servo_AUX2, Q303_FLAG_FLIP)
				| GET_FLAG(Servo_AUX3, Q303_FLAG_SNAPSHOT)
				| GET_FLAG(Servo_AUX4, Q303_FLAG_VIDEO);
		if(Servo_data[AUX7] < (servo_max_100-PPM_SWITCH))
			packet[9] |= Q303_FLAG_GIMBAL_DN;
		else if(Servo_data[AUX7] > PPM_SWITCH)
			packet[9] |= Q303_FLAG_GIMBAL_UP;
		// set low rate for first packets
		if(bind_counter != 0) {
			packet[8] &= ~0x03;
			bind_counter--;
		}
	}

	// Power on, TX mode, CRC enabled
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if (bind) {
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, Q303_RF_BIND_CHANNEL);
	} else {
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
		current_chan %= Q303_NUM_RF_CHANNELS;
	}

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	XN297_WritePayload(packet, Q303_PACKET_SIZE);
}

static uint16_t q303_callback()
{
	switch (phase) {
		case Q303_BIND:
			if (bind_counter == 0) {
				tx_addr[0] = 0x55;
				memcpy(&tx_addr[1], txid, 4);
				XN297_SetTXAddr(tx_addr, 5);
				phase = Q303_DATA;
				bind_counter = Q303_BIND_COUNT;
				BIND_DONE;
			} else {
				send_packet(1);
				bind_counter--;
			}
			break;
		case Q303_DATA:
			send_packet(0);
			break;
	}
	return Q303_PACKET_PERIOD;
}

static uint16_t q303_init()
{
//	memcpy(txid, q303_tx_rf_map[MProtocol_id % (sizeof(q303_tx_rf_map)/sizeof(q303_tx_rf_map[0]))].txid, sizeof(txid));
//	memcpy(rf_chans, q303_tx_rf_map[MProtocol_id % (sizeof(q303_tx_rf_map)/sizeof(q303_tx_rf_map[0]))].rfchan, sizeof(rf_chans));

	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	XN297_SetScrambledMode(XN297_UNSCRAMBLED);
	XN297_SetTXAddr((uint8_t *) "\xCC\xCC\xCC\xCC\xCC", 5);
	NRF24L01_FlushTx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits	NRF24L01_SetBitrate(NRF24L01_BR_250K);
	NRF24L01_SetPower();

	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
	NRF24L01_Activate(0x73);
	
	NRF24L01_Activate(0x53); // switch bank back

	BIND_IN_PROGRESS;
	bind_counter = Q303_BIND_COUNT;
	current_chan = 0;
//	PROTOCOL_SetBindState(Q303_BIND_COUNT * Q303_PACKET_PERIOD / 1000);
	phase = Q303_BIND;
	return Q303_INITIAL_WAIT;
}

#endif