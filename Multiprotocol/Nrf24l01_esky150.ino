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

#if defined(ESKY150_NRF24L01_INO)
#include "iface_nrf24l01.h"


// Timeout for callback in uSec, 4.8ms=4800us for ESky150
#define ESKY150_PERIOD 4800
#define ESKY150_CHKTIME  100 // Time to wait for packet to be sent (no ACK, so very short)

#define esky150_PAYLOADSIZE 15
#define ADDR_esky150_SIZE 4

static uint32_t total_packets;
enum {
    ESKY150_INIT2 = 0,
    ESKY150_DATA
};


static uint8_t esky150_packet_ack() {
	switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT))) {
		case _BV(NRF24L01_07_TX_DS):		return PKT_ACKED;
		case _BV(NRF24L01_07_MAX_RT):	return PKT_TIMEOUT;
	}
	return PKT_PENDING;
}

// 2-bytes CRC
#define CRC_CONFIG (_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO))
static uint16_t esky150_init() {
    uint8_t rx_addr[ADDR_esky150_SIZE] = { 0x73, 0x73, 0x74, 0x63 };
    uint8_t tx_addr[ADDR_esky150_SIZE] = { 0x71, 0x0A, 0x31, 0xF4 };
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, ADDR_esky150_SIZE-2);   // 4-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
    NRF24L01_SetPower();
    NRF24L01_SetBitrate(NRF24L01_BR_2M);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_addr, ADDR_esky150_SIZE);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, tx_addr, ADDR_esky150_SIZE);


    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, esky150_PAYLOADSIZE);   // bytes of data payload for pipe 0


    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
    // Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, _BV(NRF2401_1D_EN_DPL) | _BV(NRF2401_1D_EN_ACK_PAY) | _BV(NRF2401_1D_EN_DYN_ACK));

    // Delay 50 ms
    return 50000;
}


static uint16_t esky150_init2() {
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    packet_sent = 0;
    packet_count = 0;
    rf_ch_num = 0;

    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG | _BV(NRF24L01_00_PWR_UP));
    // delayMicroseconds(150);
    return 150;
}


static void esky150_send_packet() {
	uint8_t rf_ch = hopping_frequency[rf_ch_num];
	rf_ch_num = 1 - rf_ch_num;

	packet[0]  = hopping_frequency[0];
	packet[1]  = hopping_frequency[1];
	packet[2]  = highByte(Servo_data[THROTTLE]);
	packet[3]  = lowByte(Servo_data[THROTTLE]);
	packet[4]  = highByte(Servo_data[AILERON]);
	packet[5]  = lowByte(Servo_data[AILERON]);
	packet[6]  = highByte(Servo_data[ELEVATOR]);
	packet[7]  = lowByte(Servo_data[ELEVATOR]);
	packet[8]  = highByte(Servo_data[RUDDER]);
	packet[9]  = lowByte(Servo_data[RUDDER]);
	// Constant values 00 d8 18 f8
	packet[10] = 0x00;
	packet[11] = 0xd8;
	packet[12] = 0x18;
	packet[13] = 0xf8;
	uint8_t sum = 0;
	for (int i = 0; i < 14; ++i) sum += packet[i];
	packet[14] = sum;

	packet_sent = 0;
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, sizeof(packet));
	++total_packets;
	packet_sent = 1;
}

static uint16_t esky150_callback() {
	uint16_t timeout = ESKY150_PERIOD;
	switch (phase) {
		case ESKY150_INIT2:
			timeout = esky150_init2();
			phase = ESKY150_DATA;
			break;
		case ESKY150_DATA:
			if (packet_count == 4)
				packet_count = 0;
			else {
				if (packet_sent && esky150_packet_ack() != PKT_ACKED) {
					return ESKY150_CHKTIME;
				}
				esky150_send_packet();
			}
			break;
	}
	return timeout;
}

static uint16_t esky150_setup() {
    total_packets = 0;
    uint16_t timeout = esky150_init();
	
    // Use channels 2..79
    uint8_t first = MProtocol_id % 37 + 2;
    uint8_t second = first + 40;
    hopping_frequency[0] = first;  // 0x22;
    hopping_frequency[1] = second; // 0x4a;

    return timeout;
}
#endif
