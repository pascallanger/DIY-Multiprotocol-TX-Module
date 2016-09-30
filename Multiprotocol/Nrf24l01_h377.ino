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

#if defined (H377_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_COUNT 800


#define TXID_H377_SIZE 5

#define FREQUENCE_NUM_H377  20
#define SET_NUM_H377  9
// available frequency must be in between 2402 and 2477

static uint8_t binding_ch=0x50;
static uint8_t hopping_frequency_data[SET_NUM_H377] = {0x1c,0x1b,0x1d,0x11,0x0e,0x0d,0x01,0x1d,0x15};

static const uint8_t  binding_adr_rf[5]={0x32,0xaa,0x45,0x45,0x78};

static uint8_t rf_adr_buf[5]; 
static uint8_t rf_adr_buf_data[SET_NUM_H377][5] = {
	{0xad,0x9a,0xa6,0x69,0xb2},//ansheng
	{0x92,0x9a,0x9d,0x69,0x99},//dc59
	{0x92,0xb2,0x9d,0x69,0x9a},//small two
	{0xad,0x9a,0x5a,0x69,0x96},//james_1
	{0x95,0x9a,0x52,0x69,0x99},//james_2
	{0x52,0x52,0x52,0x69,0xb9},//james_3
	{0x52,0x52,0x52,0x52,0x55},//small two_1
	{0x92,0xB2,0x9D,0x69,0x9A},//small two_2
	{0x96,0x9A,0x45,0x69,0xB2}//small two_3
};	

static uint8_t bind_buf_array[10];
static uint8_t bind_buf_array_data[SET_NUM_H377][4] = {
	{0xcf,0x1c,0x19,0x1a},
	{0xff,0x48,0x19,0x19},
	{0xf3,0x4d,0x19,0x19},
	{0x9e,0x1f,0x19,0x19},
	{0x8d,0x3d,0x19,0x19},
	{0xbd,0x23,0x19,0x19},
	{0xF3,0x28,0x19,0x19},
	{0xF3,0x4D,0x19,0x19},
	{0x82,0x8D,0x19,0x19}
};


static unsigned int ch_data[8];
static uint8_t payload[10];
static uint8_t counter1ms;

static int select_ch_id = 0;

static void h377_binding_packet(void) { //bind_buf_array
	uint8_t i;
	counter1ms = 0;
	hopping_frequency_no = 0;

	for(i=0;i<5;i++)
		bind_buf_array[i] = rf_adr_buf[i];

	bind_buf_array[5] = hopping_frequency[0];

	for(i=0;i<4;i++)
		bind_buf_array[i+6] = bind_buf_array_data[select_ch_id][i];
}

static void h377_init() {
	NRF24L01_Initialize();

	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rf_adr_buf, 5);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 10); // payload size = 10
	//NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81); // binding packet must be set in channel 81
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, binding_ch); // binding packet must be set in channel 81

	// 2-bytes CRC, radio off
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
	NRF24L01_SetBitrate(0);                          // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
}

// H377 channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITH, channel data value is from 0 to 1000
static void h377_ch_data() {
	uint32_t temp;
	uint8_t i,j;
	for (i = 0; i< 8; i++) {
		j=CH_AETR[i];
		temp=map(limit_channel_100(j),servo_min_100,servo_max_100,0,1000); // max/min servo range is +-125%
		if (j == THROTTLE) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
			temp = 1000 -temp;
		//if (i == 0) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
		//    temp = 1000 -temp;
		//if (i == 1) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
		//    temp = 1000 -temp;
		if (temp < 0)
			ch_data[i] = 0;
		else if (temp > 1000)
			ch_data[i] = 1000;
		else
			ch_data[i] = (unsigned int)temp;
		payload[i] = (uint8_t)ch_data[i];
	}
	payload[8]  = (uint8_t)((ch_data[0]>>8)&0x0003);
	payload[8] |= (uint8_t)((ch_data[1]>>6)&0x000c);
	payload[8] |= (uint8_t)((ch_data[2]>>4)&0x0030);
	payload[8] |= (uint8_t)((ch_data[3]>>2)&0x00c0);

	payload[9]  = (uint8_t)((ch_data[4]>>8)&0x0003);
	payload[9] |= (uint8_t)((ch_data[5]>>6)&0x000c);
	payload[9] |= (uint8_t)((ch_data[6]>>4)&0x0030);
	payload[9] |= (uint8_t)((ch_data[7]>>2)&0x00c0);
}

static uint16_t h377_cb() {
	counter1ms++;
	if(counter1ms==1) {	NRF24L01_FlushTx(); }
	//-------------------------
	else if(counter1ms==2) {
		if (bind_phase>0) {
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *)binding_adr_rf, 5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, binding_ch);
		}
	}
	else if(counter1ms==3) {
		if (bind_phase >0) {
			bind_phase--;
			if (! bind_phase) { BIND_DONE; }	// binding finished, change tx add
			NRF24L01_WritePayload(bind_buf_array,10);
		}
	} 
	else if (counter1ms==4) {	if (bind_phase > 0) {	NRF24L01_FlushTx(); }}
	//-------------------------
	else if(counter1ms==5) {	NRF24L01_SetPower(); }
	//-------------------------
	else if (counter1ms == 6) {
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
		hopping_frequency_no++;
		if (hopping_frequency_no >= FREQUENCE_NUM_H377) { hopping_frequency_no = 0; }
	}
	else if (counter1ms == 7) { h377_ch_data(); }
	else if(counter1ms>8) {
		counter1ms = 0;
		NRF24L01_WritePayload(payload,10);
	}
	return 1000;  // send 1 binding packet and 1 data packet per 9ms
}

// Linear feedback shift register with 32-bit Xilinx polinomial x^32 + x^22 + x^2 + x + 1
static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
static const uint32_t LFSR_INTAP = 32-1;

static void update_lfsr(uint32_t *lfsr, uint8_t b) {
	for (int i = 0; i < 8; ++i) {
		*lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
		b >>= 1;
	}
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)


static void H377_tx_id() {
	for(int i=0;i<5;i++)
		rf_adr_buf[i] = rf_adr_buf_data[select_ch_id][i];    

	hopping_frequency[0] = hopping_frequency_data[select_ch_id];

	for (int i = 1; i < FREQUENCE_NUM_H377; i++) {
		hopping_frequency[i] = hopping_frequency[i-1] + 3;        
	} 
}



static uint16_t h377_setup() {
	select_ch_id = MProtocol_id_master%SET_NUM_H377;

	H377_tx_id();//rf_adr_buf  hopping_frequency

	h377_binding_packet();//bind_buf_array (rf_adr_buf hopping_frequency)

	h377_init();

	if(IS_AUTOBIND_FLAG_on) {
		bind_phase = BIND_COUNT;
		BIND_IN_PROGRESS;
	} 
	else { bind_phase = 0; }
	
	return 1000;
}
#endif
