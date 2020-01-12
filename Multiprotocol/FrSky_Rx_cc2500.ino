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

#if defined(FRSKY_RX_CC2500_INO)

#include "iface_cc2500.h" 

 #define FRSKY_RX_D16FCC_LENGTH	32
 #define FRSKY_RX_D16LBT_LENGTH	35
 #define FRSKY_RX_D8_LENGTH		20
 #define FRSKY_RX_FORMATS		3

enum
{
	FRSKY_RX_D16FCC = 0,
	FRSKY_RX_D16LBT,
	FRSKY_RX_D8
};

enum {
	FRSKY_RX_TUNE_START,
	FRSKY_RX_TUNE_LOW,
	FRSKY_RX_TUNE_HIGH,
	FRSKY_RX_BIND,
	FRSKY_RX_DATA,
};

const PROGMEM uint8_t frsky_rx_common_reg[][2] = {
	{CC2500_02_IOCFG0, 0x01},
	{CC2500_18_MCSM0, 0x18},
	{CC2500_07_PKTCTRL1, 0x04},
	{CC2500_3E_PATABLE, 0xFF},
	{CC2500_0C_FSCTRL0, 0},
	{CC2500_0D_FREQ2, 0x5C},
	{CC2500_13_MDMCFG1, 0x23},
	{CC2500_14_MDMCFG0, 0x7A},
	{CC2500_19_FOCCFG, 0x16},
	{CC2500_1A_BSCFG, 0x6C},
	{CC2500_1B_AGCCTRL2, 0x03},
	{CC2500_1C_AGCCTRL1, 0x40},
	{CC2500_1D_AGCCTRL0, 0x91},
	{CC2500_21_FREND1, 0x56},
	{CC2500_22_FREND0, 0x10},
	{CC2500_23_FSCAL3, 0xA9},
	{CC2500_24_FSCAL2, 0x0A},
	{CC2500_25_FSCAL1, 0x00},
	{CC2500_26_FSCAL0, 0x11},
	{CC2500_29_FSTEST, 0x59},
	{CC2500_2C_TEST2, 0x88},
	{CC2500_2D_TEST1, 0x31},
	{CC2500_2E_TEST0, 0x0B},
	{CC2500_03_FIFOTHR, 0x07},
	{CC2500_09_ADDR, 0x00},
};

const PROGMEM uint8_t frsky_rx_d16fcc_reg[][2] = {
	{CC2500_17_MCSM1, 0x0C},
	{CC2500_0E_FREQ1, 0x76},
	{CC2500_0F_FREQ0, 0x27},
	{CC2500_06_PKTLEN, 0x1E},
	{CC2500_08_PKTCTRL0, 0x01},
	{CC2500_0B_FSCTRL1, 0x0A},
	{CC2500_10_MDMCFG4, 0x7B},
	{CC2500_11_MDMCFG3, 0x61},
	{CC2500_12_MDMCFG2, 0x13},
	{CC2500_15_DEVIATN, 0x51},
};

const PROGMEM uint8_t frsky_rx_d16lbt_reg[][2] = {
	{CC2500_17_MCSM1, 0x0E},
	{CC2500_0E_FREQ1, 0x80},
	{CC2500_0F_FREQ0, 0x00},
	{CC2500_06_PKTLEN, 0x23},
	{CC2500_08_PKTCTRL0, 0x01},
	{CC2500_0B_FSCTRL1, 0x08},
	{CC2500_10_MDMCFG4, 0x7B},
	{CC2500_11_MDMCFG3, 0xF8},
	{CC2500_12_MDMCFG2, 0x03},
	{CC2500_15_DEVIATN, 0x53},
};

const PROGMEM uint8_t frsky_rx_d8_reg[][2] = {
	{CC2500_17_MCSM1,    0x0C},
	{CC2500_0E_FREQ1,    0x76},
	{CC2500_0F_FREQ0,    0x27},
	{CC2500_06_PKTLEN,   0x19},
	{CC2500_08_PKTCTRL0, 0x05},
	{CC2500_0B_FSCTRL1,  0x08},
	{CC2500_10_MDMCFG4,  0xAA},
	{CC2500_11_MDMCFG3,  0x39},
	{CC2500_12_MDMCFG2,  0x11},
	{CC2500_15_DEVIATN,  0x42},
};

static uint8_t frsky_rx_chanskip;
static int8_t  frsky_rx_finetune;
static uint8_t frsky_rx_format;

static void __attribute__((unused)) frsky_rx_strobe_rx()
{
	 CC2500_Strobe(CC2500_SIDLE);
	 CC2500_Strobe(CC2500_SFRX);
	 CC2500_Strobe(CC2500_SRX);
}

static void __attribute__((unused)) frsky_rx_initialise_cc2500() {
	const uint8_t frsky_rx_length[] = { FRSKY_RX_D16FCC_LENGTH, FRSKY_RX_D16LBT_LENGTH, FRSKY_RX_D8_LENGTH };
	packet_length = frsky_rx_length[frsky_rx_format];
	CC2500_Reset();
	CC2500_Strobe(CC2500_SIDLE);
	for (uint8_t i = 0; i < sizeof(frsky_rx_common_reg) / 2; i++)
		CC2500_WriteReg(pgm_read_byte_near(&frsky_rx_common_reg[i][0]), pgm_read_byte_near(&frsky_rx_common_reg[i][1]));

	switch (frsky_rx_format) {
	case FRSKY_RX_D16FCC:
		for (uint8_t i = 0; i < sizeof(frsky_rx_d16fcc_reg) / 2; i++)
			CC2500_WriteReg(pgm_read_byte_near(&frsky_rx_d16fcc_reg[i][0]), pgm_read_byte_near(&frsky_rx_d16fcc_reg[i][1]));
		break;
	case FRSKY_RX_D16LBT:
		for (uint8_t i = 0; i < sizeof(frsky_rx_d16lbt_reg) / 2; i++)
			CC2500_WriteReg(pgm_read_byte_near(&frsky_rx_d16lbt_reg[i][0]), pgm_read_byte_near(&frsky_rx_d16lbt_reg[i][1]));
		break;
	case FRSKY_RX_D8:
		for (uint8_t i = 0; i < sizeof(frsky_rx_d8_reg) / 2; i++)
			CC2500_WriteReg(pgm_read_byte_near(&frsky_rx_d8_reg[i][0]), pgm_read_byte_near(&frsky_rx_d8_reg[i][1]));
		CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05); // always check address
		CC2500_WriteReg(CC2500_09_ADDR, 0x03); // bind address
		CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
		break;
	}
	CC2500_WriteReg(CC2500_0A_CHANNR, 0);  // bind channel
	rx_disable_lna = IS_POWER_FLAG_on;
	CC2500_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN); // lna disable / enable
	frsky_rx_strobe_rx();
	delayMicroseconds(1000); // wait for RX to activate
}

static void __attribute__((unused)) frsky_rx_set_channel(uint8_t channel)
{
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[channel]);
	if(frsky_rx_format == FRSKY_RX_D8)
		CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[channel]);
	frsky_rx_strobe_rx();
}

static void __attribute__((unused)) frsky_rx_calibrate()
{
	frsky_rx_strobe_rx();
	for (unsigned c = 0; c < 47; c++)
	{
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[c]);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
}

static uint8_t __attribute__((unused)) frskyx_rx_check_crc()
{
	// check D8 checksum
	if (frsky_rx_format == FRSKY_RX_D8)
		return (packet[packet_length-1] & 0x80) == 0x80; // check CRC_OK flag in status byte 2
	// check D16 checksum
	uint8_t limit = packet_length - 4;
	uint16_t lcrc = FrSkyX_crc(&packet[3], limit - 3); // computed crc
	uint16_t rcrc = (packet[limit] << 8) | (packet[limit + 1] & 0xff); // received crc
	return lcrc == rcrc;
}

static void __attribute__((unused)) frsky_rx_build_telemetry_packet()
{
	uint16_t raw_channel[8];
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0;
	uint8_t i;

	if (frsky_rx_format == FRSKY_RX_D16FCC || frsky_rx_format == FRSKY_RX_D16LBT) {
		// decode D16 channels
		raw_channel[0] = ((packet[10] << 8) & 0xF00) | packet[9];
		raw_channel[1] = ((packet[11] << 4) & 0xFF0) | (packet[10] >> 4);
		raw_channel[2] = ((packet[13] << 8) & 0xF00) | packet[12];
		raw_channel[3] = ((packet[14] << 4) & 0xFF0) | (packet[13] >> 4);
		raw_channel[4] = ((packet[16] << 8) & 0xF00) | packet[15];
		raw_channel[5] = ((packet[17] << 4) & 0xFF0) | (packet[16] >> 4);
		raw_channel[6] = ((packet[19] << 8) & 0xF00) | packet[18];
		raw_channel[7] = ((packet[20] << 4) & 0xFF0) | (packet[19] >> 4);	
		for (i = 0; i < 8; i++) {
			// ignore failsafe channels
			if(packet[7] != 0x10+(i<<1)) {
				uint8_t shifted = (raw_channel[i] & 0x800)>0;
				uint16_t channel_value = raw_channel[i] & 0x7FF;
				if (channel_value < 64)
					rx_rc_chan[shifted ? i + 8 : i] = 0;
				else
					rx_rc_chan[shifted ? i + 8 : i] = min(((channel_value - 64) << 4) / 15, 2047);
			}
		}
	}
	else {
		// decode D8 channels
		raw_channel[0] = ((packet[10] & 0x0F) << 8 | packet[6]);
		raw_channel[1] = ((packet[10] & 0xF0) << 4 | packet[7]);
		raw_channel[2] = ((packet[11] & 0x0F) << 8 | packet[8]);
		raw_channel[3] = ((packet[11] & 0xF0) << 4 | packet[9]);
		raw_channel[4] = ((packet[16] & 0x0F) << 8 | packet[12]);
		raw_channel[5] = ((packet[16] & 0xF0) << 4 | packet[13]);
		raw_channel[6] = ((packet[17] & 0x0F) << 8 | packet[14]);
		raw_channel[7] = ((packet[17] & 0xF0) << 4 | packet[15]);
		for (i = 0; i < 8; i++) {
			if (raw_channel[i] < 1290)
				raw_channel[i] = 1290;
			rx_rc_chan[i] = min(((raw_channel[i] - 1290) << 4) / 15, 2047);
		}
	}

	// buid telemetry packet
	packet_in[idx++] = RX_LQI;
	packet_in[idx++] = RX_RSSI;
	packet_in[idx++] = 0;  // start channel
	packet_in[idx++] = frsky_rx_format == FRSKY_RX_D8 ? 8 : 16; // number of channels in packet

	// pack channels
	for (i = 0; i < packet_in[3]; i++) {
		bits |= ((uint32_t)rx_rc_chan[i]) << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8) {
			packet_in[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
}

uint16_t initFrSky_Rx()
{
	state = 0;
	frsky_rx_chanskip = 1;
	hopping_frequency_no = 0;
	rx_data_started = false;
	frsky_rx_finetune = 0;
	telemetry_link = 0;
	if (IS_BIND_IN_PROGRESS) {
		frsky_rx_format = FRSKY_RX_D8;
		frsky_rx_initialise_cc2500();
		phase = FRSKY_RX_TUNE_START;
	}
	else {
		uint16_t temp = FRSKY_RX_EEPROM_OFFSET;
		frsky_rx_format = eeprom_read_byte((EE_ADDR)temp++) % FRSKY_RX_FORMATS;
		rx_tx_addr[0] = eeprom_read_byte((EE_ADDR)temp++);
		rx_tx_addr[1] = eeprom_read_byte((EE_ADDR)temp++);
		rx_tx_addr[2] = eeprom_read_byte((EE_ADDR)temp++);
		frsky_rx_finetune = eeprom_read_byte((EE_ADDR)temp++);
		for (uint8_t ch = 0; ch < 47; ch++)
			hopping_frequency[ch] = eeprom_read_byte((EE_ADDR)temp++);
		frsky_rx_initialise_cc2500();
		frsky_rx_calibrate();
		CC2500_WriteReg(CC2500_18_MCSM0, 0x08); // FS_AUTOCAL = manual
		CC2500_WriteReg(CC2500_09_ADDR, rx_tx_addr[0]); // set address
		CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05); // check address
		if (option == 0)
			CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
		else
			CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		frsky_rx_set_channel(hopping_frequency_no);
		phase = FRSKY_RX_DATA;
	}
	return 1000;
}

uint16_t FrSky_Rx_callback()
{
	static uint32_t pps_timer=0;
	static uint8_t pps_counter=0;
	static int8_t read_retry = 0;
	static int8_t tune_low, tune_high;
	uint8_t len, ch;

	if ((prev_option != option) && (phase >= FRSKY_RX_DATA)) {
		if (option == 0)
			CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
		else
			CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		prev_option = option;
	}

	if (rx_disable_lna != IS_POWER_FLAG_on) {
		rx_disable_lna = IS_POWER_FLAG_on;
		CC2500_SetTxRxMode(rx_disable_lna ? TXRX_OFF : RX_EN);
	}

	len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;

	switch(phase) {
	case FRSKY_RX_TUNE_START:
		if (len >= packet_length) {
			CC2500_ReadData(packet, packet_length);
			if(packet[1] == 0x03 && packet[2] == 0x01 && frskyx_rx_check_crc()) {
				rx_tx_addr[0] = packet[3];	// TXID
				rx_tx_addr[1] = packet[4];	// TXID
				frsky_rx_finetune = -127;
				CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
				phase = FRSKY_RX_TUNE_LOW;
				frsky_rx_strobe_rx();
				return 1000;
			}
		}
		frsky_rx_format = (frsky_rx_format + 1) % FRSKY_RX_FORMATS; // switch to next format (D16FCC, D16LBT, D8)
		frsky_rx_initialise_cc2500();
		frsky_rx_finetune += 10;
		CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
		frsky_rx_strobe_rx();
		return 18000;

	case FRSKY_RX_TUNE_LOW:
		if (len >= packet_length) {
			CC2500_ReadData(packet, packet_length);
			if(packet[1] == 0x03 && packet[2] == 0x01 && frskyx_rx_check_crc() && packet[3] == rx_tx_addr[0] && packet[4] == rx_tx_addr[1]) {
				tune_low = frsky_rx_finetune;
				frsky_rx_finetune = 127;
				CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
				phase = FRSKY_RX_TUNE_HIGH;
				frsky_rx_strobe_rx();
				return 1000;
			}
		}
		frsky_rx_finetune += 1;
		CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
		frsky_rx_strobe_rx();
		return 18000;

	case FRSKY_RX_TUNE_HIGH:
		if (len >= packet_length) {
			CC2500_ReadData(packet, packet_length);
			if(packet[1] == 0x03 && packet[2] == 0x01 && frskyx_rx_check_crc() && packet[3] == rx_tx_addr[0] && packet[4] == rx_tx_addr[1]) {
				tune_high = frsky_rx_finetune;
				frsky_rx_finetune = (tune_low + tune_high) / 2;
				CC2500_WriteReg(CC2500_0C_FSCTRL0, (int8_t)frsky_rx_finetune);
				if(tune_low < tune_high)
					phase = FRSKY_RX_BIND;
				else
					phase = FRSKY_RX_TUNE_START;
				frsky_rx_strobe_rx();
				return 1000;
			}
		}
		frsky_rx_finetune -= 1;
		CC2500_WriteReg(CC2500_0C_FSCTRL0, frsky_rx_finetune);
		frsky_rx_strobe_rx();
		return 18000;

	case FRSKY_RX_BIND:
		if(len >= packet_length) {
			CC2500_ReadData(packet, packet_length);
			if(packet[1] == 0x03 && packet[2] == 0x01 && frskyx_rx_check_crc() && packet[3] == rx_tx_addr[0] && packet[4] == rx_tx_addr[1] && packet[5] <= 0x2D) {
				for (ch = 0; ch < 5; ch++)
					hopping_frequency[packet[5]+ch] = packet[6+ch];
				state |= 1 << (packet[5] / 5);
				if (state == 0x3ff) {
					debug("Bind complete: ");
					frsky_rx_calibrate();
					rx_tx_addr[2] = packet[12]; // RX # (D16)
					CC2500_WriteReg(CC2500_18_MCSM0, 0x08); // FS_AUTOCAL = manual
					CC2500_WriteReg(CC2500_09_ADDR, rx_tx_addr[0]); // set address
					CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05); // check address
					phase = FRSKY_RX_DATA;
					frsky_rx_set_channel(hopping_frequency_no);
					// store format, finetune setting, txid, channel list
					uint16_t temp = FRSKY_RX_EEPROM_OFFSET;
					eeprom_write_byte((EE_ADDR)temp++, frsky_rx_format);
					debug("format=%d, ", frsky_rx_format);
					eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[0]);
					debug("addr[0]=%02X, ", rx_tx_addr[0]);
					eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[1]);
					debug("addr[1]=%02X, ", rx_tx_addr[1]);
					eeprom_write_byte((EE_ADDR)temp++, rx_tx_addr[2]);
					debug("rx_num=%02X, ", rx_tx_addr[2]);
					eeprom_write_byte((EE_ADDR)temp++, frsky_rx_finetune);
					debugln("tune=%d", (int8_t)frsky_rx_finetune);
					for (ch = 0; ch < 47; ch++)
					{
						eeprom_write_byte((EE_ADDR)temp++, hopping_frequency[ch]);
						debug("%02X ", hopping_frequency[ch]);
					}
					debugln("");
					BIND_DONE;
				}
			}
			frsky_rx_strobe_rx();
		}
		return 1000;

	case FRSKY_RX_DATA:
		if (len >= packet_length) {
			CC2500_ReadData(packet, packet_length);
			if (packet[1] == rx_tx_addr[0] && packet[2] == rx_tx_addr[1] && frskyx_rx_check_crc() && (frsky_rx_format == FRSKY_RX_D8 || packet[6] == rx_tx_addr[2])) {
				RX_RSSI = packet[packet_length-2];
				if(RX_RSSI >= 128)
					RX_RSSI -= 128;
				else
					RX_RSSI += 128;
				// hop to next channel
				if (frsky_rx_format == FRSKY_RX_D16FCC || frsky_rx_format == FRSKY_RX_D16LBT)
					frsky_rx_chanskip = ((packet[4] & 0xC0) >> 6) | ((packet[5] & 0x3F) << 2);
				hopping_frequency_no = (hopping_frequency_no + frsky_rx_chanskip) % 47;
				frsky_rx_set_channel(hopping_frequency_no);
				if (telemetry_link == 0) { // send channels to TX
						frsky_rx_build_telemetry_packet();
						telemetry_link = 1;
				}
				rx_data_started = true;
				read_retry = 0;
				pps_counter++;
			}
		}
		
		// packets per second
		if (millis() - pps_timer >= 1000) {
			pps_timer = millis();
			debugln("%d pps", pps_counter);
			RX_LQI = pps_counter;
			pps_counter = 0;
		}

		// skip channel if no packet received in time
		if (read_retry++ >= 9) {
			hopping_frequency_no = (hopping_frequency_no + frsky_rx_chanskip) % 47;
			frsky_rx_set_channel(hopping_frequency_no);
			if(rx_data_started)
				read_retry = 0;
			else
				read_retry = -50; // retry longer until first packet is catched
		}
		break;
	}
	return 1000;
}

#endif
