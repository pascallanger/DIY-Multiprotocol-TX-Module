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

#if defined(V2X2_RX_NRF24L01_INO)

#define V2X2_RX_PACKET_SIZE 16
#define V2X2_RX_RF_BIND_CHANNEL 0x08

enum {
	V2X2_RX_BIND,
	V2X2_RX_DATA
};

static void __attribute__((unused)) V2X2_Rx_init_nrf24l01()
{
	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5-byte RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, (uint8_t*)"\x66\x88\x68\x68\x68", 5);
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// disable Auto Acknowldgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x02);  	// Enable rx data pipe 1
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, V2X2_RX_PACKET_SIZE);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, V2X2_RX_RF_BIND_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00);
	NRF24L01_SetTxRxMode(RX_EN);						// enable LNA
	// switch to RX mode, 16 bit CRC
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
}

static uint8_t __attribute__((unused)) V2X2_Rx_check_validity()
{
	uint8_t i, sum;
	// check transmitter id
	if (phase == V2X2_RX_DATA) {
		for (i = 0; i < 3; i++) {
			if (rx_tx_addr[i + 1] != packet[i + 7]) {
				return 0;
			}
		}
	}
	// checksum
	sum = packet[0];
	for (i=1; i<15; i++)
		sum += packet[i];
	return (sum == packet[15]);
}

static void __attribute__((unused)) V2X2_Rx_build_telemetry_packet()
{
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0;

	packet_in[idx++] = RX_LQI;
	packet_in[idx++] = RX_LQI >> 1;	// no RSSI: 125..0
	packet_in[idx++] = 0;			// start channel
	packet_in[idx++] = 11;			// number of channels in packet

	// convert & pack channels
	const uint8_t aetr_idx[4] = { 3, 2, 0, 1 };
	for (uint8_t i = 0; i < packet_in[3]; i++) {
		uint32_t val = CHANNEL_MIN_100;
		if (i < 4) {
			// AETR
			uint8_t rx_val = packet[aetr_idx[i]];
			if (i != 2 && rx_val < 128)
				rx_val = 127 - rx_val;
			val += (rx_val << 5) / 5;
			val += (rx_val >> 5);
		}
		else if (((i == 4) && (packet[14] & 0x04)) ||	// flip
				((i == 5) && (packet[14] & 0x10)) ||	// led light
				((i == 6) && (packet[14] & 0x01)) ||	// snapshot
				((i == 7) && (packet[14] & 0x02)) ||	// video
				((i == 8) && (packet[10] & 0x02)) ||	// headless
				((i == 9) && (packet[10] & 0x08)) ||	// calibrate x
				((i == 10) && (packet[10] & 0x20))) {	// calibrate y
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

uint16_t initV2X2_Rx()
{
	debugln("initV2X2_Rx()");
	V2X2_Rx_init_nrf24l01();
	hopping_frequency_no = 0;
	rx_data_started = false;
	rx_data_received = false;
	if (IS_BIND_IN_PROGRESS) {
		phase = V2X2_RX_BIND;
	}
	else {
		uint16_t temp = V2X2_RX_EEPROM_OFFSET;
		for (uint8_t i = 1; i < 4; i++)
			rx_tx_addr[i] = eeprom_read_byte((EE_ADDR)temp++);
		V2X2_set_tx_id(); // compute frequency hopping channels
		phase = V2X2_RX_DATA;
	}
	return 1000;
}

uint16_t V2X2_Rx_callback()
{
	uint8_t i;
	static int8_t read_retry;
	static uint16_t pps_counter;
	static uint32_t pps_timer = 0;

	switch (phase) {
		case V2X2_RX_BIND:
			if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
				NRF24L01_ReadPayload(packet, V2X2_RX_PACKET_SIZE);
				if (packet[14] == 0xC0 && V2X2_Rx_check_validity()) {
					// store transmitter id into eeprom
					uint16_t temp = V2X2_RX_EEPROM_OFFSET;
					for (i = 0; i < 3; i++) {
						rx_tx_addr[i+1] = packet[i+7];
						eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[i+1]);
					}
					V2X2_set_tx_id(); // compute frequency hopping channels
					NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
					phase = V2X2_RX_DATA;
					BIND_DONE;
				}
			}
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			break;
		case V2X2_RX_DATA:
			if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) {
				NRF24L01_ReadPayload(packet, V2X2_RX_PACKET_SIZE);
				if (V2X2_Rx_check_validity()) {
					if (telemetry_link == 0) {
						V2X2_Rx_build_telemetry_packet();
						telemetry_link = 1;
					}
					rx_data_started = true;
					rx_data_received = true;
					read_retry = 16; // hop to next channel
					pps_counter++;
				}
			}
			// packets per second
			if (millis() - pps_timer >= 1000) {
				pps_timer = millis();
				debugln("%d pps", pps_counter);
				RX_LQI = pps_counter; // 0-250
				pps_counter = 0;
			}
			// frequency hopping, 16x250us = 4ms
			if (read_retry++ >= 16) {
				hopping_frequency_no = (hopping_frequency_no + 1) & 0x1F;
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no >> 1]);
				NRF24L01_FlushRx();
				NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
				if (rx_data_started)
				{
					if (rx_data_received)
					{ // In sync, take a rest
						rx_data_received = false;
						read_retry = 13;
						return 3500;
					}
					else
					{ // packet lost
						read_retry = 0;
						if (RX_LQI == 0)	// communication lost
							rx_data_started = false;
					}
				}
				else
					read_retry = -32; // retry longer until first packet is caught
			}
			return 250;
	}
	return 1000;
}

#endif
