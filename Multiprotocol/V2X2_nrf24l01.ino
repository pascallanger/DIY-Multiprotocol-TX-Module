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
// compatible with WLToys V2x2, JXD JD38x, JD39x, JJRC H6C, Yizhan Tarantula X6 ...
// Last sync with hexfet new_protocols/v202_nrf24l01.c dated 2015-03-15

#if defined(V2X2_NRF24L01_INO)


#include "iface_nrf24l01.h"


#define V2X2_BIND_COUNT 1000
// Timeout for callback in uSec, 4ms=4000us for V202
#define V2X2_PACKET_PERIOD 4000
//
// Time to wait for packet to be sent (no ACK, so very short)
#define V2X2_PACKET_CHKTIME  100
#define V2X2_PAYLOADSIZE 16

// 
enum {
	V2X2_FLAG_CAMERA = 0x01, // also automatic Missile Launcher and Hoist in one direction
	V2X2_FLAG_VIDEO  = 0x02, // also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir.
	V2X2_FLAG_FLIP   = 0x04,
	V2X2_FLAG_UNK9   = 0x08,
	V2X2_FLAG_LIGHT  = 0x10,
	V2X2_FLAG_UNK10  = 0x20,
	V2X2_FLAG_BIND   = 0xC0,
	// flags going to byte 10
	V2X2_FLAG_HEADLESS  = 0x02,
	V2X2_FLAG_MAG_CAL_X = 0x08,
	V2X2_FLAG_MAG_CAL_Y = 0x20,
    V2X2_FLAG_EMERGENCY = 0x80,	// JXD-506
    // flags going to byte 11 (JXD-506)
    V2X2_FLAG_START_STOP = 0x40,
    V2X2_FLAG_CAMERA_UP  = 0x01,   
    V2X2_FLAG_CAMERA_DN  = 0x02,
};

//

enum {
	V202_INIT2 = 0,
	V202_INIT2_NO_BIND,//1
	V202_BIND1,//2
	V202_BIND2,//3
	V202_DATA//4
};

// This is frequency hopping table for V202 protocol
// The table is the first 4 rows of 32 frequency hopping
// patterns, all other rows are derived from the first 4.
// For some reason the protocol avoids channels, dividing
// by 16 and replaces them by subtracting 3 from the channel
// number in this case.
// The pattern is defined by 5 least significant bits of
// sum of 3 bytes comprising TX id
const uint8_t PROGMEM freq_hopping[][16] = {
	{ 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
		0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
	{ 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
		0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
	{ 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
		0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
	{ 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
		0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

static void __attribute__((unused)) v202_init()
{
	NRF24L01_Initialize();

	// 2-bytes CRC, radio off
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xFF); // 4ms retransmit t/o, 15 tries
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);      // Channel 8
	NRF24L01_SetBitrate(NRF24L01_BR_1M);                          // 1Mbps
	NRF24L01_SetPower();
	
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	//    NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00); // no write bits in this field
	//    NRF24L01_WriteReg(NRF24L01_00_CD, 0x00);         // same
	NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
	NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
	NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
	NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, V2X2_PAYLOADSIZE);   // bytes of data payload for pipe 1
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, V2X2_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, V2X2_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, V2X2_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, V2X2_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, V2X2_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *)"\x66\x88\x68\x68\x68", 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, (uint8_t *)"\x88\x66\x86\x86\x86", 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *)"\x66\x88\x68\x68\x68", 5);
}

static void __attribute__((unused)) V202_init2()
{
	NRF24L01_FlushTx();
	packet_sent = 0;
	hopping_frequency_no = 0;

	// Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
	//Done by TX_EN??? => NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
}

static void __attribute__((unused)) V2X2_set_tx_id(void)
{
	uint8_t sum;
	sum = rx_tx_addr[1] + rx_tx_addr[2] + rx_tx_addr[3];
	// Higher 3 bits define increment to corresponding row
	uint8_t increment = (sum & 0x1e) >> 2;
	// Base row is defined by lowest 2 bits
	sum &=0x03;
	for (uint8_t i = 0; i < 16; ++i) {
		uint8_t val = pgm_read_byte_near(&freq_hopping[sum][i]) + increment;
		// Strange avoidance of channels divisible by 16
		hopping_frequency[i] = (val & 0x0f) ? val : val - 3;
	}
}

static void __attribute__((unused)) V2X2_add_pkt_checksum()
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < 15;  ++i)
		sum += packet[i];
	packet[15] = sum;
}

static void __attribute__((unused)) V2X2_send_packet(uint8_t bind)
{
	uint8_t flags2=0;
	if (bind)
	{
		flags     = V2X2_FLAG_BIND;
		packet[0] = 0;
		packet[1] = 0;
		packet[2] = 0;
		packet[3] = 0;
		packet[4] = 0;
		packet[5] = 0;
		packet[6] = 0;
	}
	else
	{
		packet[0] = convert_channel_8b(THROTTLE);
		packet[1] = convert_channel_s8b(RUDDER);
		packet[2] = convert_channel_s8b(ELEVATOR);
		packet[3] = convert_channel_s8b(AILERON);
		// Trims, middle is 0x40
		packet[4] = 0x40; // yaw
		packet[5] = 0x40; // pitch
		packet[6] = 0x40; // roll

		//Flags
		flags=0;
		// Channel 5
		if (CH5_SW)	flags = V2X2_FLAG_FLIP;
		// Channel 6
		if (CH6_SW)	flags |= V2X2_FLAG_LIGHT;
		// Channel 7
		if (CH7_SW)	flags |= V2X2_FLAG_CAMERA;
		// Channel 8
		if (CH8_SW)	flags |= V2X2_FLAG_VIDEO;

		//Flags2
		// Channel 9
		if (CH9_SW)
			flags2 = V2X2_FLAG_HEADLESS;
		if(sub_protocol==JXD506)
		{
			// Channel 11
			if (CH11_SW)
				flags2 |= V2X2_FLAG_EMERGENCY;
		}
		else
		{
			// Channel 10
			if (CH10_SW)
				flags2 |= V2X2_FLAG_MAG_CAL_X;
			// Channel 11
			if (CH11_SW)
				flags2 |= V2X2_FLAG_MAG_CAL_Y;
		}
	}
	// TX id
	packet[7] = rx_tx_addr[1];
	packet[8] = rx_tx_addr[2];
	packet[9] = rx_tx_addr[3];
	// flags
	packet[10] = flags2;
	packet[11] = 0x00;
	packet[12] = 0x00;
	packet[13] = 0x00;
	if(sub_protocol==JXD506)
	{
		// Channel 10
		if (CH10_SW)
			packet[11] = V2X2_FLAG_START_STOP;
		// Channel 12
		if(CH12_SW)
			packet[11] |= V2X2_FLAG_CAMERA_UP;
		else if(Channel_data[CH12] < CHANNEL_MIN_COMMAND)
			packet[11] |= V2X2_FLAG_CAMERA_DN;
		packet[12] = 0x40;
		packet[13] = 0x40;
	}
	packet[14] = flags;
	V2X2_add_pkt_checksum();

	packet_sent = 0;
	uint8_t rf_ch = hopping_frequency[hopping_frequency_no >> 1];
	hopping_frequency_no = (hopping_frequency_no + 1) & 0x1F;
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, V2X2_PAYLOADSIZE);
	packet_sent = 1;

	if (! hopping_frequency_no)
		NRF24L01_SetPower();
}

uint16_t ReadV2x2()
{
	switch (phase) {
		case V202_INIT2:
			V202_init2();
			phase = V202_BIND2;
			return 150;
			break;
		case V202_INIT2_NO_BIND:
			V202_init2();
			phase = V202_DATA;
			return 150;
			break;
		case V202_BIND2:
			if (packet_sent && NRF24L01_packet_ack() != PKT_ACKED)
				return V2X2_PACKET_CHKTIME;
			V2X2_send_packet(1);
			if (--bind_counter == 0)
			{
				phase = V202_DATA;
				BIND_DONE;
			}
			break;
		case V202_DATA:
			if (packet_sent && NRF24L01_packet_ack() != PKT_ACKED)
				return V2X2_PACKET_CHKTIME;
			V2X2_send_packet(0);
			break;
	}
	// Packet every 4ms
	return V2X2_PACKET_PERIOD;
}

uint16_t initV2x2()
{	
	v202_init();
	//
	if (IS_BIND_IN_PROGRESS)
	{
		bind_counter = V2X2_BIND_COUNT;
		phase = V202_INIT2;
	}
	else
		phase = V202_INIT2_NO_BIND;
	V2X2_set_tx_id();
	return 50000;
}

#endif
