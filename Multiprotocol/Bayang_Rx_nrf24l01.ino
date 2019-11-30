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

#if defined(BAYANG_RX_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define BAYANG_RX_PACKET_SIZE		15
#define BAYANG_RX_RF_NUM_CHANNELS	4
#define BAYANG_RX_RF_BIND_CHANNEL	0
#define BAYANG_RX_ADDRESS_LENGTH	5

enum {
	BAYANG_RX_BIND = 0,
	BAYANG_RX_DATA
};

static void __attribute__((unused)) Bayang_Rx_init_nrf24l01()
{
	const uint8_t bind_address[BAYANG_RX_ADDRESS_LENGTH] = { 0,0,0,0,0 };
	NRF24L01_Initialize();
	XN297_SetTXAddr(bind_address, BAYANG_RX_ADDRESS_LENGTH);
	XN297_SetRXAddr(bind_address, BAYANG_RX_ADDRESS_LENGTH);
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BAYANG_RX_PACKET_SIZE + 2); // 2 extra bytes for xn297 crc
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RX_RF_BIND_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_FlushRx();
	NRF24L01_SetTxRxMode(RX_EN);
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
}

static uint8_t __attribute__((unused)) Bayang_Rx_check_validity() {
	uint8_t sum = packet[0];
	for (uint8_t i = 1; i < BAYANG_RX_PACKET_SIZE - 1; i++)
		sum += packet[i];
	return sum == packet[14];
}

static void __attribute__((unused)) Bayang_Rx_build_telemetry_packet()
{
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0;

	packet_in[idx++] = RX_LQI;
	packet_in[idx++] = RX_LQI>>1;	// no RSSI: 125..0
	packet_in[idx++] = 0;			// start channel
	packet_in[idx++] = 10;			// number of channels in packet

	// convert & pack channels
	for (uint8_t i = 0; i < packet_in[3]; i++) {
		uint32_t val = CHANNEL_MIN_100;
		if (i < 4) {
			// AETR
			//val = (((packet[4 + i * 2] & ~0x7C) << 8) | packet[5 + i * 2]) << 1;
			val=packet[4 + i * 2]&0x03;
			val=(val<<8)+packet[5 + i * 2];
			val=((val+128)<<3)/5;
		} else if (i == 4 || i == 5) {
			val=packet[i==4?1:13];
			val=((val+32)<<5)/5;						// extra analog channel
		} else if (((i == 6) && (packet[2] & 0x08)) ||	// flip
				 ((i == 7) && (packet[2] & 0x01)) ||	// rth
				 ((i == 8) && (packet[2] & 0x20)) ||	// picture
				 ((i == 9) && (packet[2] & 0x10))) {	// video
			// set channel to 100% if feature is enabled
			val = CHANNEL_MAX_100;
		}
		bits |= val << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8) {
			packet_in[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
}

uint16_t initBayang_Rx()
{
	uint8_t i;
	Bayang_Rx_init_nrf24l01();
	hopping_frequency_no = 0;
	rx_data_started = false;
	rx_data_received = false;
	
	if (IS_BIND_IN_PROGRESS) {
		phase = BAYANG_RX_BIND;
	}
	else {
		uint16_t temp = BAYANG_RX_EEPROM_OFFSET;
		for (i = 0; i < 5; i++)
			rx_tx_addr[i] = eeprom_read_byte((EE_ADDR)temp++);
		for (i = 0; i < BAYANG_RX_RF_NUM_CHANNELS; i++)
			hopping_frequency[i] = eeprom_read_byte((EE_ADDR)temp++);
		XN297_SetTXAddr(rx_tx_addr, BAYANG_RX_ADDRESS_LENGTH);
		XN297_SetRXAddr(rx_tx_addr, BAYANG_RX_ADDRESS_LENGTH);
		phase = BAYANG_RX_DATA;
	}
	return 1000;
}

uint16_t Bayang_Rx_callback()
{
	uint8_t i;
	static int8_t read_retry;
	static uint16_t pps_counter;
	static uint32_t pps_timer = 0;

	switch (phase) {
	case BAYANG_RX_BIND:
		if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
			// data received from TX
			if (XN297_ReadPayload(packet, BAYANG_RX_PACKET_SIZE) && ( packet[0] == 0xA4 || packet[0] == 0xA2 ) && Bayang_Rx_check_validity()) {
				// store tx info into eeprom
				uint16_t temp = BAYANG_RX_EEPROM_OFFSET;
				for (i = 0; i < 5; i++) {
					rx_tx_addr[i] = packet[i + 1];
					eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[i]);
				}
				for (i = 0; i < 4; i++) {
					hopping_frequency[i] = packet[i + 6];
					eeprom_write_byte((EE_ADDR)temp++, hopping_frequency[i]);
				}
				XN297_SetTXAddr(rx_tx_addr, BAYANG_RX_ADDRESS_LENGTH);
				XN297_SetRXAddr(rx_tx_addr, BAYANG_RX_ADDRESS_LENGTH);
				BIND_DONE;
				phase = BAYANG_RX_DATA;
			}
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
		}
		break;
	case BAYANG_RX_DATA:
		if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
			if (XN297_ReadPayload(packet, BAYANG_RX_PACKET_SIZE) && packet[0] == 0xA5 && Bayang_Rx_check_validity()) {
				if (telemetry_link == 0) {
					Bayang_Rx_build_telemetry_packet();
					telemetry_link = 1;
				}
				rx_data_started = true;
				rx_data_received = true;
				read_retry = 8;
				pps_counter++;
			}
		}
		// packets per second
		if (millis() - pps_timer >= 1000) {
			pps_timer = millis();
			debugln("%d pps", pps_counter);
			RX_LQI = pps_counter >> 1;
			pps_counter = 0;
		}
		// frequency hopping
		if (read_retry++ >= 8) {
			hopping_frequency_no++;
			if (hopping_frequency_no >= BAYANG_RX_RF_NUM_CHANNELS)
				hopping_frequency_no = 0;
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			if (rx_data_started)
			{
				if(rx_data_received)
				{ // In sync
					rx_data_received = false;
					read_retry = 5;
					return 1500;
				}
				else
				{ // packet lost
					read_retry = 0;
					if(RX_LQI==0)	// communication lost
						rx_data_started=false;
				}
			}
			else
				read_retry = -16; // retry longer until first packet is caught
		}
		return 250;
	}
	return 1000;
}

#endif
