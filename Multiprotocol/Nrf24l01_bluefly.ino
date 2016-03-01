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

#if defined(BlueFly_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_BlueFly_COUNT 800

#define TXID_BlueFly_SIZE 5

#define PAYLOAD_BlueFly_SIZE  12
// available frequency must be in between 2402 and 2477
static uint8_t hopping_frequency_start;

static uint8_t  bluefly_binding_adr_rf[TXID_BlueFly_SIZE]={0x32,0xaa,0x45,0x45,0x78}; // fixed binding ids for all planes
// bluefly_rf_adr_buf can be used for fixed id
static uint8_t bluefly_rf_adr_buf[TXID_BlueFly_SIZE]; // ={0x13,0x88,0x46,0x57,0x76};

static uint8_t bind_payload[PAYLOAD_BlueFly_SIZE];

static unsigned int ch_data_bluefly[8];


static void bluefly_binding_packet(void)
{
    int i;
    for (i = 0; i < TXID_BlueFly_SIZE; ++i)
      bind_payload[i] = bluefly_rf_adr_buf[i];
    bind_payload[i++] = hopping_frequency_start;
    for (; i < PAYLOAD_BlueFly_SIZE; ++i) bind_payload[i] = 0x55;
}

static void bluefly_init() {
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bluefly_rf_adr_buf, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bluefly_rf_adr_buf, 5);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOAD_BlueFly_SIZE); // payload size = 12
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81); // binding packet must be set in channel 81

    // 2-bytes CRC, radio on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
    NRF24L01_SetBitrate(NRF24L01_BR_250K);           // BlueFly - 250kbps
    NRF24L01_SetPower();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
}

// HiSky channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITCH, channel data value is from 0 to 1000
static void bluefly_ch_data() {
    uint32_t temp;
    int i;
    for (i = 0; i< 8; ++i) {
		temp = (uint32_t)Servo_data[ch[i]] * 300/PPM_MAX + 500; // 200-800 range
		if (temp < 0)
			ch_data_bluefly[i] = 0;
		else if (temp > 1000)
			ch_data_bluefly[i] = 1000;
		else
			ch_data_bluefly[i] = (unsigned int)temp;
		
        packet[i] = (uint8_t)ch_data_bluefly[i];
    }

    packet[8]  = (uint8_t)((ch_data_bluefly[0]>>8)&0x0003);
    packet[8] |= (uint8_t)((ch_data_bluefly[1]>>6)&0x000c);
    packet[8] |= (uint8_t)((ch_data_bluefly[2]>>4)&0x0030);
    packet[8] |= (uint8_t)((ch_data_bluefly[3]>>2)&0x00c0);

    packet[9]  = (uint8_t)((ch_data_bluefly[4]>>8)&0x0003);
    packet[9] |= (uint8_t)((ch_data_bluefly[5]>>6)&0x000c);
    packet[9] |= (uint8_t)((ch_data_bluefly[6]>>4)&0x0030);
    packet[9] |= (uint8_t)((ch_data_bluefly[7]>>2)&0x00c0);

    unsigned char l, h, t;
    l = h = 0xff;
    for (int i=0; i<10; ++i) {
        h ^= packet[i];
        h ^= h >> 4;
        t = h;
        h = l;
        l = t;
        t = (l<<4) | (l>>4);
        h ^= ((t<<2) | (t>>6)) & 0x1f;
        h ^= t & 0xf0;
        l ^= ((t<<1) | (t>>7)) & 0xe0;
    }
    // Checksum
    packet[10] = h; 
    packet[11] = l;
}

static uint16_t bluefly_cb() {
    switch(phase++) {
	    case 0:
	        bluefly_ch_data();
	        break;
	    case 1:
	        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bluefly_rf_adr_buf, 5);
	        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency_start + hopping_frequency_no*2);
	        hopping_frequency_no++;
	        hopping_frequency_no %= 15;
	        NRF24L01_FlushTx();
	        NRF24L01_WritePayload(packet, PAYLOAD_BlueFly_SIZE);
	        break;
	    case 2:
	        break;
	    case 3:
	        if (counter>0) {
	            counter--;
	            if (! counter) {	BIND_DONE; }
	            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bluefly_binding_adr_rf, 5);
	            NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
	            NRF24L01_FlushTx();
	            NRF24L01_WritePayload(bind_payload, PAYLOAD_BlueFly_SIZE);
	        }
	        break;
	    case 4:
	        break;
	    case 5:
	        NRF24L01_SetPower();
	        /* FALLTHROUGH */
	    default:
	        phase = 0;
	        break;
    }
    return 1000;  // send 1 binding packet and 1 data packet per 9ms
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id() {
    uint32_t lfsr = 0x7649eca9ul;
 
    if (Model.fixed_id) {
       for (uint8_t i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (int i = 0; i < TXID_BlueFly_SIZE; ++i) rand32_r(&lfsr, 0);

    for (uint8_t i = 0; i < TXID_BlueFly_SIZE; ++i) {
        bluefly_rf_adr_buf[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }

    printf("Effective id: %02X%02X%02X%02X%02X\r\n", bluefly_rf_adr_buf[0], bluefly_rf_adr_buf[1], bluefly_rf_adr_buf[2], bluefly_rf_adr_buf[3], bluefly_rf_adr_buf[4]);

    // Use LFSR to seed frequency hopping sequence after another
    // divergence round
    for (uint8_t i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
	hopping_frequency_start = ((lfsr >> 8) % 47) + 2; 
    bluefly_binding_packet();
}

static uint16_t BlueFly_setup() {
	initialize_tx_id();

	bluefly_init();

	if(IS_AUTOBIND_FLAG_on) {
		counter = BIND_BlueFly_COUNT;
//		PROTOCOL_SetBindState(BIND_BlueFly_COUNT * 10); //8 seconds binding time
	} else {
		counter = 0;
	}

	return 1000;
}
#endif
