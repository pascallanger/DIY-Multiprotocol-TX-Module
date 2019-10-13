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

#if defined(AFHDS2A_RX_A7105_INO)

#include "iface_a7105.h" 

#define AFHDS2A_RX_TXPACKET_SIZE	38
#define AFHDS2A_RX_RXPACKET_SIZE	37
#define AFHDS2A_RX_NUMFREQ			16

enum {
	AFHDS2A_RX_BIND1,
	AFHDS2A_RX_BIND2,
	AFHDS2A_RX_DATA
};

static void __attribute__((unused)) AFHDS2A_Rx_build_telemetry_packet()
{
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0;

	packet_in[idx++] = RX_LQI; // 0 - 130
	packet_in[idx++] = RX_RSSI;
	packet_in[idx++] = 0; // start channel
	packet_in[idx++] = 14; // number of channels in packet
	// pack channels
	for (uint8_t i = 0; i < 14; i++) {
		uint32_t val = packet[9+i*2] | (packet[10+i*2] << 8);
		if (val < 860)
			val = 860;
		// convert ppm (860-2140) to Multi (0-2047)
		val = min(((val-860)<<3)/5, 2047);

		bits |= val << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8) {
			packet_in[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
}

static uint8_t __attribute__((unused)) AFHDS2A_Rx_data_ready()
{
	// check if FECF+CRCF Ok
	return !(A7105_ReadReg(A7105_00_MODE) & (1 << 5 | 1 << 6 | 1 << 0));
}

uint16_t initAFHDS2A_Rx()
{
	uint8_t i;
	A7105_Init();
	hopping_frequency_no = 0;
	packet_count = 0;
	rx_data_started = false;
	rx_disable_lna = IS_POWER_FLAG_on;
	A7105_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN);
	A7105_Strobe(A7105_RX);

	if (IS_BIND_IN_PROGRESS) {
		phase = AFHDS2A_RX_BIND1;
	}
	else {
		uint16_t temp = AFHDS2A_RX_EEPROM_OFFSET;
		for (i = 0; i < 4; i++)
			rx_id[i] = eeprom_read_byte((EE_ADDR)temp++);
		for (i = 0; i < AFHDS2A_RX_NUMFREQ; i++)
			hopping_frequency[i] = eeprom_read_byte((EE_ADDR)temp++);
		phase = AFHDS2A_RX_DATA;
	}
	return 1000;
}

#define AFHDS2A_RX_WAIT_WRITE 0x80

uint16_t AFHDS2A_Rx_callback()
{
	static uint32_t pps_timer = 0;
	static uint16_t pps_counter = 0;
	static int8_t read_retry;
	int16_t temp;
	uint8_t i;

#ifndef FORCE_AFHDS2A_TUNING
	A7105_AdjustLOBaseFreq(1);
#endif
	if (rx_disable_lna != IS_POWER_FLAG_on) {
		rx_disable_lna = IS_POWER_FLAG_on;
		A7105_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN);
	}

	switch(phase) {
	case AFHDS2A_RX_BIND1:
		if (AFHDS2A_Rx_data_ready()) {
			A7105_ReadData(AFHDS2A_RX_TXPACKET_SIZE);
			if ((packet[0] == 0xbb && packet[9] == 0x01) ||	(packet[0] == 0xbc && packet[9] <= 0x02)) {
				memcpy(rx_id, &packet[1], 4); // TX id actually
				memcpy(hopping_frequency, &packet[11], AFHDS2A_RX_NUMFREQ);
				phase = AFHDS2A_RX_BIND2;
			}
		}
		A7105_WriteReg(A7105_0F_PLL_I, (packet_count++ & 1) ? 0x0D : 0x8C); // bind channels
		A7105_SetTxRxMode(RX_EN);
		A7105_Strobe(A7105_RX);
		return 10000;

	case AFHDS2A_RX_BIND2:
		// got 2nd bind packet from tx ?
		if (AFHDS2A_Rx_data_ready()) {
			A7105_ReadData(AFHDS2A_RX_TXPACKET_SIZE);
			if ((packet[0] == 0xBC && packet[9] == 0x02 && packet[10] == 0x00) &&
				(memcmp(rx_id, &packet[1], 4) == 0) &&
				(memcmp(rx_tx_addr, &packet[5], 4) == 0)) {
				// save tx info to eeprom
				temp = AFHDS2A_RX_EEPROM_OFFSET;
				for (i = 0; i < 4; i++)
					eeprom_write_byte((EE_ADDR)temp++, rx_id[i]);
				for (i = 0; i < AFHDS2A_RX_NUMFREQ; i++)
					eeprom_write_byte((EE_ADDR)temp++, hopping_frequency[i]);
				BIND_DONE;
				phase = AFHDS2A_RX_DATA;
				return 3850;
			}
		}

		// transmit response packet
		packet[0] = 0xBC;
		memcpy(&packet[1], rx_id, 4);
		memcpy(&packet[5], rx_tx_addr, 4);
		packet[9] = 0x01;
		packet[10] = 0x00;
		memset(&packet[11], 0xFF, 26);
		A7105_WriteData(AFHDS2A_RX_RXPACKET_SIZE, packet_count++ & 1 ? 0x0D : 0x8C);
		phase |= AFHDS2A_RX_WAIT_WRITE;
		return 1700;
	
	case AFHDS2A_RX_BIND2 | AFHDS2A_RX_WAIT_WRITE:
		//Wait for TX completion
		pps_timer = micros();
		while (micros() - pps_timer < 700) // Wait max 700µs, using serial+telemetry exit in about 120µs
			if (!(A7105_ReadReg(A7105_00_MODE) & 0x01))
				break;
		A7105_Strobe(A7105_RX);
		phase &= ~AFHDS2A_RX_WAIT_WRITE;
		return 10000;
	
	case AFHDS2A_RX_DATA:
		if (AFHDS2A_Rx_data_ready()) {
			A7105_ReadData(AFHDS2A_RX_TXPACKET_SIZE);
			if (memcmp(&packet[1], rx_id, 4) == 0 && memcmp(&packet[5], rx_tx_addr, 4) == 0) {
				if (packet[0] == 0x58 && packet[37] == 0x00 && telemetry_link == 0) { // standard packet, send channels to TX
					int rssi = min(A7105_ReadReg(A7105_1D_RSSI_THOLD),160);
					RX_RSSI = map16b(rssi, 160, 8, 0, 128);
					AFHDS2A_Rx_build_telemetry_packet();
					telemetry_link = 1;
				}
				rx_data_started = true;
				read_retry = 10; // hop to next channel
				pps_counter++;
			}
		}
		
		// packets per second
		if (millis() - pps_timer >= 1000) {
			pps_timer = millis();
			debugln("%d pps", pps_counter);
			RX_LQI = pps_counter / 2;
			pps_counter = 0;
		}

		// frequency hopping
		if (read_retry++ >= 10) {
			hopping_frequency_no++;
			if(hopping_frequency_no >= AFHDS2A_RX_NUMFREQ)
				hopping_frequency_no = 0;
			A7105_WriteReg(A7105_0F_PLL_I, hopping_frequency[hopping_frequency_no]);
			A7105_Strobe(A7105_RX);
			if (rx_data_started)
				read_retry = 0;
			else
				read_retry = -127; // retry longer until first packet is catched
		}
		return 385;
	}
	return 3850; // never reached
}

#endif
