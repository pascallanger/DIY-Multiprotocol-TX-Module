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
// Last sync with hexfet new_protocols/hisky_nrf24l01.c dated 2015-03-27

#if defined(HISKY_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define HISKY_BIND_COUNT 1000
#define HISKY_TXID_SIZE 5
#define HISKY_FREQUENCE_NUM  20
//
uint8_t bind_buf_arry[4][10];

// HiSky protocol uses TX id as an address for nRF24L01, and uses frequency hopping sequence
// which does not depend on this id and is passed explicitly in binding sequence. So we are free
// to generate this sequence as we wish. It should be in the range [02..77]
static void __attribute__((unused)) calc_fh_channels()
{
	uint8_t idx = 0;
	uint32_t rnd = MProtocol_id;

	while (idx < HISKY_FREQUENCE_NUM)
	{
		uint8_t i;
		uint8_t count_2_26 = 0, count_27_50 = 0, count_51_74 = 0;

		rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization
		// Use least-significant byte. 73 is prime, so channels 76..77 are unused
		uint8_t next_ch = ((rnd >> 8) % 73) + 2;
		// Keep the distance 2 between the channels - either odd or even
		if (((next_ch ^ (uint8_t)rx_tx_addr[3]) & 0x01 )== 0)
			continue;
		// Check that it's not duplicated and spread uniformly
		for (i = 0; i < idx; i++) {
			if(hopping_frequency[i] == next_ch)
				break;
			if(hopping_frequency[i] <= 26)
				count_2_26++;
			else if (hopping_frequency[i] <= 50)
				count_27_50++;
			else
				count_51_74++;
		}
		if (i != idx)
			continue;
		if ( (next_ch <= 26 && count_2_26 < 8) || (next_ch >= 27 && next_ch <= 50 && count_27_50 < 8) || (next_ch >= 51 && count_51_74 < 8) )
			hopping_frequency[idx++] = next_ch;//find hopping frequency
	}
}

static void __attribute__((unused)) build_binding_packet(void)
{
	uint8_t i;
	uint16_t sum=0;
	uint8_t sum_l,sum_h;

	for(i=0;i<5;i++)
		sum += rx_tx_addr[i];

	sum_l = (uint8_t)sum;//low byte
	sum >>= 8;
	sum_h = (uint8_t)sum;//high bye

	bind_buf_arry[0][0] = 0xff;
	bind_buf_arry[0][1] = 0xaa;
	bind_buf_arry[0][2] = 0x55;

	for(i=3;i<8;i++)
		bind_buf_arry[0][i] = rx_tx_addr[i-3];

	for(i=1;i<4;i++)
	{
		bind_buf_arry[i][0] = sum_l;
		bind_buf_arry[i][1] = sum_h;
		bind_buf_arry[i][2] = i-1;
	}

	for(i=0;i<7;i++)
	{	bind_buf_arry[1][i+3] = hopping_frequency[i];
		bind_buf_arry[2][i+3] = hopping_frequency[i+7];
		bind_buf_arry[3][i+3] = hopping_frequency[i+14];
	}
}

static void __attribute__((unused)) hisky_init()
{
	NRF24L01_Initialize();

	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);		// Enable p0 rx
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5-byte RX/TX address (byte -2)
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);			// binding packet must be set in channel 81
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 10);		// payload size = 10
	if(sub_protocol==HK310)
		NRF24L01_SetBitrate(NRF24L01_BR_250K);				// 250Kbps
	else
		NRF24L01_SetBitrate(NRF24L01_BR_1M);				// 1Mbps
	NRF24L01_SetPower();								// Set power
	NRF24L01_SetTxRxMode(TX_EN);						// TX mode, 2-bytes CRC, radio on
}

// HiSky channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITCH, channel data value is from 0 to 1000
// Channel 7 - Gyro mode, 0 - 6 axis, 3 - 3 axis 
static void __attribute__((unused)) build_ch_data()
{
	uint16_t temp;
	uint8_t i,j;
	for (i = 0; i< 8; i++) {
		j=CH_AETR[i];
		temp=convert_channel_16b_limit(j,0,1000);            			
		if (j == THROTTLE) // It is clear that hisky's throttle stick is made reversely, so I adjust it here on purpose
			temp = 1000 -temp;
		if (j == CH7)
			temp = temp < 400 ? 0 : 3; // Gyro mode, 0 - 6 axis, 3 - 3 axis 
		packet[i] = (uint8_t)(temp&0xFF);
		packet[i<4?8:9]>>=2;
		packet[i<4?8:9]|=(temp>>2)&0xc0;
	}
}

uint16_t hisky_cb()
{
	phase++;
	if(sub_protocol==HK310)
		switch(phase)
		{
			case 1:
				NRF24L01_SetPower();
				phase=2;
				break;
			case 3:
				if (! bind_counter)
					NRF24L01_WritePayload(packet,10); // 2 packets per 5ms
				break;
			case 4:
				phase=6;
				break;
			case 7:	// build packet
				#ifdef FAILSAFE_ENABLE
					if(IS_FAILSAFE_VALUES_on && hopping_frequency_no==0)
					{ //  send failsafe every 100ms
						convert_failsafe_HK310(RUDDER,  &packet[0],&packet[1]);
						convert_failsafe_HK310(THROTTLE,&packet[2],&packet[3]);
						convert_failsafe_HK310(CH5,    &packet[4],&packet[5]);
						packet[7]=0xAA;
						packet[8]=0x5A;
					}
					else
				#endif
					{
						convert_channel_HK310(RUDDER,  &packet[0],&packet[1]);
						convert_channel_HK310(THROTTLE,&packet[2],&packet[3]);
						convert_channel_HK310(CH5,    &packet[4],&packet[5]);
						packet[7]=0x55;
						packet[8]=0x67;
					}
				phase=8;
				break;
		}
	switch(phase)
	{
		case 1:
			NRF24L01_FlushTx();
			break;
		case 2:
			if (bind_counter != 0)
			{
				//Set TX id and channel for bind packet
				NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *)"\x12\x23\x23\x45\x78", 5);
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81);
			}
			break;
		case 3:
			if (bind_counter != 0)
			{
				bind_counter--;//
				if (! bind_counter) //Binding complete
					BIND_DONE;//
				//Send bind packet
				NRF24L01_WritePayload(bind_buf_arry[binding_idx],10);
				binding_idx++;
				if (binding_idx >= 4)
					binding_idx = 0;
			}
			break;
		case 4:
			if (bind_counter != 0)
				NRF24L01_FlushTx();
			break;
		case 5:
			//Set TX power
			NRF24L01_SetPower();
			break;
		case 6:
			//Set TX id and channel for normal packet
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			hopping_frequency_no++;
			if (hopping_frequency_no >= HISKY_FREQUENCE_NUM)
				hopping_frequency_no = 0;
			break;
		case 7:
			//Build normal packet
			build_ch_data();
			break;
		case 8:
			break;
		default:
			//Send normal packet
			phase = 0;
			NRF24L01_WritePayload(packet,10);
			break;
	}
	return 1000;  // send 1 binding packet and 1 data packet per 9ms	
}

static void __attribute__((unused)) initialize_tx_id()
{
	//Generate frequency hopping table	
	if(sub_protocol==HK310)
	{
		// for HiSky surface protocol, the transmitter always generates hop channels in sequential order. 
		// The transmitter only generates the first hop channel between 0 and 49. So the channel range is from 0 to 69.
		hopping_frequency_no=rx_tx_addr[0]%50;
		for(uint8_t i=0;i<HISKY_FREQUENCE_NUM;i++)
			hopping_frequency[i]=hopping_frequency_no++;	// Sequential order hop channels...
	}
	else
		calc_fh_channels();
}

uint16_t initHiSky()
{
	initialize_tx_id();
	build_binding_packet();
	hisky_init();
	phase = 0;
	hopping_frequency_no = 0;
	binding_idx = 0;

	if(IS_BIND_IN_PROGRESS)
		bind_counter = HISKY_BIND_COUNT;
	else 
		bind_counter = 0;
	return 1000;
}

#endif
