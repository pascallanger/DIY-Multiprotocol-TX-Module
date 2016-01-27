/*
 This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License. If not, see <http://www.gnu.org/licenses/>.
 */

// EAchine MT99xx (H7, MT9916 ...) TX protocol

// Auxiliary channels:
// CH5:  rate (3 pos)
// CH6:  flip flag
// CH7:  still camera
// CH8:  video camera
// CH10: elevator trim
// CH11: aileron trim

#if defined(H7_NRF24L01_INO)

#include "iface_nrf24l01.h"

static const uint8_t H7_freq[] = {
    0x02, 0x48, 0x0C, 0x3e, 0x16, 0x34, 0x20, 0x2A,
    0x2A, 0x20, 0x34, 0x16, 0x3e, 0x0c, 0x48, 0x02
};

static const uint8_t H7_mys_byte[] = {
    0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 
    0x05, 0x15, 0x06, 0x16, 0x07, 0x17, 0x00, 0x10
};

// flags going to packet[6]
// H7_FLAG_RATE0, // default rate, no flag
#define H7_FLAG_RATE1 0x01
#define H7_FLAG_RATE2 0x02
#define H7_FLAG_VIDEO 0x10
#define H7_FLAG_SNAPSHOT 0x20
#define H7_FLAG_FLIP 0x80

uint8_t H7_tx_addr[5];

uint8_t checksum_offset;
uint8_t channel_offset;

#define H7_PACKET_PERIOD 2625
#define H7_PAYPLOAD_SIZE 9

void H7_initTXID() {
	checksum_offset = (rx_tx_addr[0] + rx_tx_addr[1]) & 0xff;
	channel_offset = (((checksum_offset & 0xf0)>>4) + (checksum_offset & 0x0f)) % 8;
}

uint16_t H7_init() {
	H7_initTXID();
	NRF24L01_Reset();
	NRF24L01_Initialize();
	delay(10);
	NRF24L01_FlushTx();
	for(u8 i=0; i<5; i++) { H7_tx_addr[i] = 0xCC; }
	XN297_SetTXAddr(H7_tx_addr, 5);
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);    // clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);     // no AA
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01); // enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);  // 5 bytes address
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x2D);     // set RF channel
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);// no auto retransmit
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x09);  // rx payload size (unused ?)
	NRF24L01_SetBitrate(NRF24L01_BR_1M);            // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
	NRF24L01_ReadReg(NRF24L01_1D_FEATURE); // read reg 1D back ?
	delay(150);
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	delay(100);
	H7_bind();
	return H7_PACKET_PERIOD;
}

void H7_bind() {
	BIND_IN_PROGRESS;
	uint8_t counter = 58;
	packet[0] = 0x20; // fixed (firmware date 2014-03-25 ?)
	packet[1] = 0x14; // fixed
	packet[2] = 0x03; // fixed
	packet[3] = 0x25; // fixed
	packet[4] = rx_tx_addr[0]; // 1st byte for data phase tx address  
	packet[5] = rx_tx_addr[1]; // 2nd byte for data phase tx address 
	packet[6] = 0x00; // 3rd byte for data phase tx address (always 0x00 ?)
	packet[7] = checksum_offset; // checksum offset
	packet[8] = 0xAA; // fixed
	while(counter--) {
		for (uint8_t ch = 0; ch < 16; ch++) {
			delayMicroseconds(5);
			NRF24L01_WriteReg(NRF24L01_07_STATUS,0x70);
			NRF24L01_FlushTx();
			NRF24L01_WriteReg(NRF24L01_05_RF_CH,H7_freq[ch]);
			XN297_WritePayload(packet, H7_PAYPLOAD_SIZE); //(bind packet)
			delayMicroseconds(H7_PACKET_PERIOD);
		}
	}
	delay(15);
	H7_tx_addr[0] = rx_tx_addr[0];
	H7_tx_addr[1] = rx_tx_addr[1];
	H7_tx_addr[2] = 0;
	XN297_SetTXAddr(H7_tx_addr, 5);
	BIND_DONE;
}

uint8_t H7_calcChecksum() {
	uint8_t result=checksum_offset;
	for(uint8_t i=0; i<8; i++) { result += packet[i]; }
	return result & 0xFF;
}

void H7_WritePacket() {
	static uint8_t channel=0;
	packet[0] = map(Servo_data[THROTTLE], PPM_MIN, PPM_MAX, 0xE1, 0x00);
	packet[1] = map(Servo_data[RUDDER], PPM_MIN, PPM_MAX, 0xE1, 0x00);
	packet[2] = map(Servo_data[AILERON], PPM_MIN, PPM_MAX, 0x00, 0xE1);
	packet[3] = map(Servo_data[ELEVATOR], PPM_MIN, PPM_MAX, 0x00, 0xE1);
	packet[4] = map(Servo_data[AUX7], PPM_MIN, PPM_MAX, 0x3f, 0x00); // elevator trim 0x3f - 0x00
	packet[5] = map(Servo_data[AUX8], PPM_MIN, PPM_MAX, 0x3f, 0x00); // aileron trim 0x3f - 0x00
	packet[6] = 0x40; // flags (default is 0x00 on H7, 0x40 on MT9916 stock TX)
	if(Servo_data[AUX2] > PPM_MAX_COMMAND) { packet[6] |= H7_FLAG_FLIP; }

	if(Servo_data[AUX1] > PPM_MAX_COMMAND) { packet[6] |= H7_FLAG_RATE2; }
	else if(Servo_data[AUX1] > PPM_MIN_COMMAND) { packet[6] |= H7_FLAG_RATE1; }

	if(Servo_data[AUX5] > PPM_MAX_COMMAND) { packet[6] |= H7_FLAG_SNAPSHOT; }
	if(Servo_data[AUX6] > PPM_MAX_COMMAND) { packet[6] |= H7_FLAG_VIDEO; }
	packet[7] = H7_mys_byte[channel]; // looks like this byte has no importance actually   
	packet[8] = H7_calcChecksum();
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, H7_freq[channel]+channel_offset);
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70); 
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, H7_PAYPLOAD_SIZE);
	channel++;
	if(channel > 15) { channel = 0; }        
}

uint16_t process_H7() {
	H7_WritePacket();
	return H7_PACKET_PERIOD;
}

#endif
