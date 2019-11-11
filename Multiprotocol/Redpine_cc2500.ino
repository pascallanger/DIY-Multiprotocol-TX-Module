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

#if defined(REDPINE_CC2500_INO)

#include "iface_cc2500.h"

#define REDPINE_LOOPTIME_FAST 25	//2.5ms
#define REDPINE_LOOPTIME_SLOW 6  	//6ms

#define REDPINE_BIND 1000
#define REDPINE_PACKET_SIZE 11
#define REDPINE_FEC false  // from cc2500 datasheet: The convolutional coder is a rate 1/2 code with a constraint length of m=4
#define REDPINE_NUM_HOPS 50

static void REDPINE_set_channel(uint8_t ch)
{
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch]);
    CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[ch]);
}

static void REDPINE_build_bind_packet()
{
    memset(&packet[0], 0, REDPINE_PACKET_SIZE);

    packet[0] = REDPINE_PACKET_SIZE - 1;
    packet[1] = 0x03;
    packet[2] = 0x01;
    packet[3] = rx_tx_addr[2];
    packet[4] = rx_tx_addr[3];	// Use RX_Num
    uint16_t idx = ((REDPINE_BIND - bind_counter) % 10) * 5;
    packet[5] = idx;
    packet[6] = hopping_frequency[idx++];
    packet[7] = hopping_frequency[idx++];
    packet[8] = hopping_frequency[idx++];
    packet[9] = hopping_frequency[idx++];
    packet[10] = hopping_frequency[idx++];
    // packet[11] = 0x02;
    // packet[12] = RXNUM;
}

static uint16_t Redpine_Scale(uint8_t chan)
{
    uint16_t chan_val=Channel_data[chan];	// -125%..+125% <=> 0..2047
	if (chan_val > 2046)   chan_val = 2046;
    else if (chan_val < 10) chan_val = 10;
    return chan_val;
}


static void REDPINE_data_frame() {
    uint16_t chan[4];

    memset(&packet[0], 0, REDPINE_PACKET_SIZE);

    packet[0] = REDPINE_PACKET_SIZE - 1;
    packet[1] = rx_tx_addr[2];
    packet[2] = rx_tx_addr[3];	// Use RX_Num

    chan[0] = Redpine_Scale(0);
    chan[1] = Redpine_Scale(1);
    chan[2] = Redpine_Scale(2);
    chan[3] = Redpine_Scale(3);

    packet[3] = chan[0];
    packet[4] = (((chan[0] >> 8) & 0x07) | (chan[1] << 4)) | GET_FLAG(CH5_SW, 0x08);
    packet[5] = ((chan[1] >> 4) & 0x7F) | GET_FLAG(CH6_SW, 0x80);
    packet[6] = chan[2];
    packet[7] = (((chan[2] >> 8) & 0x07) | (chan[3] << 4))  | GET_FLAG(CH7_SW, 0x08);
    packet[8] = ((chan[3] >> 4) & 0x7F) | GET_FLAG(CH8_SW, 0x80);
    packet[9] = GET_FLAG(CH9_SW, 0x01)
            | GET_FLAG(CH10_SW, 0x02)
            | GET_FLAG(CH11_SW, 0x04)
            | GET_FLAG(CH12_SW, 0x08)
            | GET_FLAG(CH13_SW, 0x10)
            | GET_FLAG(CH14_SW, 0x20)
            | GET_FLAG(CH15_SW, 0x40)
            | GET_FLAG(CH16_SW, 0x80);

    if (sub_protocol==0)
        packet[10] = REDPINE_LOOPTIME_FAST;
    else
        packet[10] = REDPINE_LOOPTIME_SLOW;
}

static uint16_t ReadREDPINE()
{
	if ( prev_option != option )
	{ // Frequency adjust
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		prev_option = option ;
	}
	if(IS_BIND_IN_PROGRESS)
	{
		if(bind_counter == REDPINE_BIND)
			REDPINE_init(0);
		if(bind_counter == REDPINE_BIND/2)
			REDPINE_init(1);
		REDPINE_set_channel(49);
        CC2500_SetTxRxMode(TX_EN);
		CC2500_SetPower();
		CC2500_Strobe(CC2500_SFRX);
		REDPINE_build_bind_packet();
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteData(packet, REDPINE_PACKET_SIZE);
		if(--bind_counter==0)
		{
			BIND_DONE;
			REDPINE_init(sub_protocol);
		}
		return 9000;
	}
	else
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(packet_period);
		#endif
		CC2500_SetTxRxMode(TX_EN);
		REDPINE_set_channel(hopping_frequency_no);
		CC2500_SetPower();
		CC2500_Strobe(CC2500_SFRX);
		REDPINE_data_frame();
		CC2500_Strobe(CC2500_SIDLE);
		hopping_frequency_no = (hopping_frequency_no + 1) % 49;
		CC2500_WriteData(packet, REDPINE_PACKET_SIZE);
		return packet_period;
	}
	return 1;
}

// register, fast 250k, slow
static const uint8_t REDPINE_init_data[][3] = {
    {CC2500_00_IOCFG2,    0x06, 0x06},
    {CC2500_02_IOCFG0,    0x06, 0x06},
    {CC2500_03_FIFOTHR,   0x07, 0x07},
    {CC2500_07_PKTCTRL1,  0x04, 0x04},
    {CC2500_08_PKTCTRL0,  0x05, 0x05},
    {CC2500_09_ADDR,      0x00, 0x00},
    {CC2500_0B_FSCTRL1,   0x0A, 0x0A},
    {CC2500_0C_FSCTRL0,   0x00, 0x00},
    {CC2500_0D_FREQ2,     0x5D, 0x5c},
    {CC2500_0E_FREQ1,     0x93, 0x76},
    {CC2500_0F_FREQ0,     0xB1, 0x27},
    {CC2500_10_MDMCFG4,   0x2D, 0x7B},
    {CC2500_11_MDMCFG3,   0x3B, 0x61},
    {CC2500_12_MDMCFG2,   0x73, 0x13},
    #ifdef REDPINE_FEC    
        {CC2500_13_MDMCFG1,   0xA3, 0xA3},
    #else
        {CC2500_13_MDMCFG1,   0x23, 0x23},
    #endif
    {CC2500_14_MDMCFG0,   0x56, 0x7a},  // Chan space
    {CC2500_15_DEVIATN,   0x00, 0x51},
    {CC2500_17_MCSM1,     0x0c, 0x0c},
    {CC2500_18_MCSM0,     0x08, 0x08}, //??? 0x18, 0x18},
    {CC2500_19_FOCCFG,    0x1D, 0x16},
    {CC2500_1A_BSCFG,     0x1C, 0x6c},
    {CC2500_1B_AGCCTRL2,  0xC7, 0x43},
    {CC2500_1C_AGCCTRL1,  0x00, 0x40},
    {CC2500_1D_AGCCTRL0,  0xB0, 0x91},
    {CC2500_21_FREND1,    0xB6, 0x56},
    {CC2500_22_FREND0,    0x10, 0x10},
    {CC2500_23_FSCAL3,    0xEA, 0xA9},
    {CC2500_24_FSCAL2,    0x0A, 0x0A},
    {CC2500_25_FSCAL1,    0x00, 0x00},
    {CC2500_26_FSCAL0,    0x11, 0x11},
    {CC2500_29_FSTEST,    0x59, 0x59},
    {CC2500_2C_TEST2,     0x88, 0x88},
    {CC2500_2D_TEST1,     0x31, 0x31},
    {CC2500_2E_TEST0,     0x0B, 0x0B},
    {CC2500_3E_PATABLE,   0xff, 0xff}
};

static void REDPINE_init(uint8_t format)
{
	CC2500_Reset();

	CC2500_WriteReg(CC2500_06_PKTLEN, REDPINE_PACKET_SIZE);

	for (uint8_t i=0; i < ((sizeof REDPINE_init_data) / (sizeof REDPINE_init_data[0])); i++)
		CC2500_WriteReg(REDPINE_init_data[i][0], REDPINE_init_data[i][format+1]);

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	CC2500_Strobe(CC2500_SIDLE);

	// calibrate hop channels
	for (uint8_t c = 0; c < REDPINE_NUM_HOPS; c++)
	{
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[c]);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
}

static uint16_t initREDPINE()
{
	hopping_frequency_no = 0;
	// Used from kn_nrf24l01.c : kn_calculate_freqency_hopping_channels
	uint32_t idx = 0;
	uint32_t rnd = MProtocol_id;
	#define REDPINE_MAX_RF_CHANNEL 255
	hopping_frequency[idx++] = 1;
	while (idx < REDPINE_NUM_HOPS-1)
	{
		uint32_t i;
		rnd = rnd * 0x0019660D + 0x3C6EF35F;  // Randomization
		// Drop least-significant byte for better randomization. Start from 1
		uint8_t next_ch = (rnd >> 8) % REDPINE_MAX_RF_CHANNEL + 1;
		// Check that it's not duplicate nor adjacent nor channel 0 or 1
		for (i = 0; i < idx; i++)
		{
			uint8_t ch = hopping_frequency[i];
			if ((ch <= next_ch + 1) && (ch >= next_ch - 1) && (ch > 1))
				break;
		}
		if (i != idx)
			continue;
		hopping_frequency[idx++] = next_ch;
	}
	hopping_frequency[49] = 0;  // Last channel is the bind channel at hop 0

	if (sub_protocol==0)
		packet_period = REDPINE_LOOPTIME_FAST*100;
	else
		packet_period = REDPINE_LOOPTIME_SLOW*1000;

	bind_counter=REDPINE_BIND;
	REDPINE_init(sub_protocol);
	CC2500_SetTxRxMode(TX_EN);  // enable PA
	return 10000;
}
#endif
