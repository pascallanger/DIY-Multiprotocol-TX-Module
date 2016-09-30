/*
 This project is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 Deviation is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 
 rewrite v977/v966 protocol to improve reliability
 */

#if defined(FBL100_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_FBL_COUNT 800
#define FBL_SIZE 5
#define FREQUENCE_FBL_NUM  20

static uint8_t binding_fbl_adr_rf[5]; // fixed binding ids for all planes

static uint8_t bind_fbl_buf_array[4][10];

static unsigned int fbl_data[8];


// HiSky protocol uses TX id as an address for nRF24L01, and uses frequency hopping sequence
// which does not depend on this id and is passed explicitly in binding sequence. So we are free
// to generate this sequence as we wish. It should be in the range [02..77]
static void calc_fbl_channels() {
	int idx = 0;
	uint32_t rnd = MProtocol_id;
	while (idx < FREQUENCE_FBL_NUM) {
		int i;
		int count_2_26 = 0, count_27_50 = 0, count_51_74 = 0;
		rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

		// Use least-significant byte. 73 is prime, so channels 76..77 are unused
		uint8_t next_ch = ((rnd >> 8) % 73) + 2;
		// Keep the distance 2 between the channels - either odd or even
		if (((next_ch ^ MProtocol_id) & 0x01 )== 0) { continue; }
		// Check that it's not duplicate and spread uniformly
		for (i = 0; i < idx; i++) {
			if(hopping_frequency[i] == next_ch) { break; }
			if(hopping_frequency[i] <= 26) { count_2_26++; }
			else if (hopping_frequency[i] <= 50) { count_27_50++; }
			else { count_51_74++; }
		}
		if (i != idx) { continue; }
		if ((next_ch <= 26 && count_2_26 < 8) ||(next_ch >= 27 && next_ch <= 50 && count_27_50 < 8) ||(next_ch >= 51 && count_51_74 < 8)) {
			hopping_frequency[idx++] = next_ch;
		}
	}
}

static void fbl100_build_binding_packet(void) {
	uint8_t i;
	unsigned int  sum;
	uint8_t sum_l,sum_h;

	sum = 0;
	for(i=0;i<5;i++) { sum += rx_tx_addr[i]; }
	sum_l = (uint8_t)sum;
	sum >>= 8;
	sum_h = (uint8_t)sum;
	bind_fbl_buf_array[0][0] = 0xff;
	bind_fbl_buf_array[0][1] = 0xaa;
	bind_fbl_buf_array[0][2] = 0x55;
	for(i=3;i<8;i++) { bind_fbl_buf_array[0][i] = rx_tx_addr[i-3]; }

	for(i=1;i<4;i++) {
		bind_fbl_buf_array[i][0] = sum_l;
		bind_fbl_buf_array[i][1] = sum_h;
		bind_fbl_buf_array[i][2] = i-1;
	}
	for(i=0;i<7;i++) { bind_fbl_buf_array[1][i+3] = hopping_frequency[i]; }
	for(i=0;i<7;i++) { bind_fbl_buf_array[2][i+3] = hopping_frequency[i+7]; }
	for(i=0;i<6;i++) { bind_fbl_buf_array[3][i+3] = hopping_frequency[i+14]; }

	binding_idx = 0;
}

static void hp100_build_binding_packet(void) {
	memcpy(packet, rx_tx_addr, 5);
	packet[5] = hopping_frequency[0]; // start address
	for (uint8_t i = 6; i < 12; i++) { packet[i] = 0x55; }
}

static void config_nrf24l01() {
	NRF24L01_Initialize();

	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // 0:No Auto Acknoledgement; 1:Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_length); // fbl100/v922's packet size = 10, hp100 = 12
	// 2-bytes CRC, radio off
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
	NRF24L01_SetBitrate(sub_protocol == HP100? NRF24L01_BR_250K:NRF24L01_BR_1M);  //hp100:250kbps; fbl100: 1Mbps
	NRF24L01_SetPower();

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
}

// FBL100 channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITH, channel data value is from 0 to 1000
static void fbl100_build_ch_data() {
	uint32_t temp;
	uint8_t i;
	for (i = 0; i< 8; i++) {
		temp = (uint32_t)Servo_data[i] -1000;
		if (i == 2) { temp = 1000 -temp; } // It is clear that fbl100's thro stick is made reversely,so I adjust it here on purposely
		if (temp < 0) { fbl_data[i] = 0; }
		else if (temp > 1000) { fbl_data[i] = 1000; }
		else { fbl_data[i] = (unsigned int)temp; }
		packet[i] = (uint8_t)fbl_data[i];
	}

	packet[8]  = (uint8_t)((fbl_data[0]>>8)&0x0003);
	packet[8] |= (uint8_t)((fbl_data[1]>>6)&0x000c);
	packet[8] |= (uint8_t)((fbl_data[2]>>4)&0x0030);
	packet[8] |= (uint8_t)((fbl_data[3]>>2)&0x00c0);

	packet[9]  = (uint8_t)((fbl_data[4]>>8)&0x0003);
	packet[9] |= (uint8_t)((fbl_data[5]>>6)&0x000c);
	packet[9] |= (uint8_t)((fbl_data[6]>>4)&0x0030);
	packet[9] |= (uint8_t)((fbl_data[7]>>2)&0x00c0);
}

static void hp100_build_ch_data() {
	uint32_t temp;
	uint8_t i;
	for (i = 0; i< 8; i++) {
		temp=map(limit_channel_100(i),servo_min_100,servo_max_100,200,800);
/*		temp = (uint32_t)Servo_data[i] * 300/PPM_MAX + 500;
		if (temp < 0) { temp = 0; }
		else if (temp > 1000) { temp = 1000; }
		if (i == 3 || i == 5) { temp = 1000 -temp; }	// hp100's rudd and pit channel are made reversely,so I adjust them on purposely
*/
		fbl_data[i] = (unsigned int)temp;
		packet[i] = (uint8_t)fbl_data[i];
	}

	packet[8]  = (uint8_t)((fbl_data[0]>>8)&0x0003);
	packet[8] |= (uint8_t)((fbl_data[1]>>6)&0x000c);
	packet[8] |= (uint8_t)((fbl_data[2]>>4)&0x0030);
	packet[8] |= (uint8_t)((fbl_data[3]>>2)&0x00c0);

	packet[9]  = (uint8_t)((fbl_data[4]>>8)&0x0003);
	packet[9] |= (uint8_t)((fbl_data[5]>>6)&0x000c);
	packet[9] |= (uint8_t)((fbl_data[6]>>4)&0x0030);
	packet[9] |= (uint8_t)((fbl_data[7]>>2)&0x00c0);

	unsigned char l, h, t;
	l=h=0xff;
	for(i=0; i<10; i++ ) {
		h ^= packet[i];
		h ^= h>>4;
		t = h;
		h = l;
		l = t;
		t = (l<<4) | (l>>4);
		h^=((t<<2) | (t>>6)) & 0x1f;
		h^=t&0xf0;
		l^=((t<<1) | (t>>7)) & 0xe0;
	}
	packet[10] = h;
	packet[11] = l;
}


static uint16_t fbl100_cb() {
	switch(phase) {
		case 0:
			fbl100_build_ch_data();
			break;
		case 1:
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			hopping_frequency_no++;
			if (hopping_frequency_no >= FREQUENCE_FBL_NUM) { hopping_frequency_no = 0; }
			break;
		case 2:
			NRF24L01_FlushTx();
			NRF24L01_WritePayload(packet, packet_length);
			break;
		case 3:
			break;
		case 4:
			if (bind_phase>0) {
				NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, binding_fbl_adr_rf, 5);
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
			}
			break;
		case 5:
			if (bind_phase >0) {
			bind_phase--;
			if (! bind_phase) { BIND_DONE; }	// binding finished, change tx add
			NRF24L01_FlushTx(); // must be invoked before NRF24L01_WritePayload()
			NRF24L01_WritePayload(bind_fbl_buf_array[binding_idx], packet_length);
			binding_idx++;
			if (binding_idx >= 4)
			binding_idx = 0;
			}
			break;
		case 6:
			break;
		case 7:
			NRF24L01_SetPower();
			break;
		default:
			break;
	}
	phase++;
	if (phase >=9) { phase = 0; }	// send 1 binding packet and 1 data packet per 9ms
	return 1000;
}

static uint16_t hp100_cb() {
	switch(phase) {
		case 0:
			hp100_build_ch_data();
			break;
		case 1:
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[0] + hopping_frequency_no*2);
			hopping_frequency_no++;
			hopping_frequency_no %= 15;
			NRF24L01_FlushTx();
			NRF24L01_WritePayload(packet, packet_length);
			break;
		case 2:
			if(bind_phase>0) { hp100_build_binding_packet(); }
			break;
		case 3:
			if (bind_phase>0) {
				bind_phase--;
				if (! bind_phase) {	BIND_DONE; }
				NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, binding_fbl_adr_rf, 5);
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
				NRF24L01_FlushTx();
				NRF24L01_WritePayload(packet, packet_length);
			}
			break;
		case 4:
			break;
		case 5:
			NRF24L01_SetPower();
			break;
		default:
			break;
	}
	phase++;
	if (phase >= 6) { phase = 0; }	// send 1 binding packet and 1 data packet per 10ms
	return 1000;
}

static uint8_t fbl_setup() {
	calc_fbl_channels();

	printf("FH Seq: ");
	for (int i = 0; i < FREQUENCE_FBL_NUM; ++i) {	printf("%d, ", hopping_frequency[i]); }
	printf("\r\n");
	
	// debut init
	if (sub_protocol == HP100) {
		packet_length = 12;
		binding_fbl_adr_rf[0] = 0x32; binding_fbl_adr_rf[1] = 0xaa; binding_fbl_adr_rf[2] = 0x45;
		binding_fbl_adr_rf[3] = 0x45; binding_fbl_adr_rf[4] = 0x78;
	} else {
		packet_length = 10;
		binding_fbl_adr_rf[0] = 0x12; binding_fbl_adr_rf[1] = 0x23; binding_fbl_adr_rf[2] = 0x23;
		binding_fbl_adr_rf[3] = 0x45; binding_fbl_adr_rf[4] = 0x78;
		fbl100_build_binding_packet();
	}
	config_nrf24l01();

	if(IS_AUTOBIND_FLAG_on) {	bind_phase = BIND_FBL_COUNT; }
	else {	bind_phase = 0; }

//	CLOCK_StartTimer(1000, sub_protocol == HP100?hp100_cb:fbl100_cb);
}
#endif