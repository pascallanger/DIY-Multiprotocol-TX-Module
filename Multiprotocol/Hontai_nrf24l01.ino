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

#if defined(HONTAI_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define HONTAI_BIND_COUNT 80
#define HONTAI_PACKET_PERIOD    13500
#define FQ777_951_PACKET_PERIOD	10000
#define HONTAI_INITIAL_WAIT       500
#define HONTAI_BIND_PACKET_SIZE   10
#define HONTAI_PACKET_SIZE        12
#define HONTAI_RF_BIND_CHANNEL    0

enum{
    HONTAI_FLAG_FLIP      = 0x01, 
    HONTAI_FLAG_PICTURE   = 0x02, 
    HONTAI_FLAG_VIDEO     = 0x04, 
    HONTAI_FLAG_HEADLESS  = 0x08, 
    HONTAI_FLAG_RTH       = 0x10,
    HONTAI_FLAG_CALIBRATE = 0x20,
};

// proudly swiped from http://www.drdobbs.com/implementing-the-ccitt-cyclical-redundan/199904926
#define HONTAI_POLY 0x8408
static void __attribute__((unused)) crc16(uint8_t *data_p, uint8_t length)
{
	uint16_t crc = 0xffff;

	length -= 2;
	do
	{
		for (uint8_t i = 0, data = (uint8_t)*data_p++;
		i < 8;
		i++, data >>= 1)
		{
			if ((crc & 0x01) ^ (data & 0x01))
				crc = (crc >> 1) ^ HONTAI_POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	crc = ~crc;
	*data_p++ = crc & 0xff;
	*data_p   = crc >> 8;
}

static void __attribute__((unused)) HONTAI_send_packet(uint8_t bind)
{
	if (bind)
	{
		memcpy(packet, rx_tx_addr, 5);
		memset(&packet[5], 0, 3);
	}
	else
	{
		memset(packet,0,HONTAI_PACKET_SIZE);
		packet[3] = convert_channel_16b_limit(THROTTLE, 0, 127) << 1;	// Throttle
		packet[4] = convert_channel_16b_limit(AILERON, 63, 0);			// Aileron
		packet[5] = convert_channel_16b_limit(ELEVATOR, 0, 63);			// Elevator
		packet[6] = convert_channel_16b_limit(RUDDER, 0, 63);			// Rudder
		if(sub_protocol == X5C1)
			packet[7] = convert_channel_16b_limit(AILERON, 0, 63)-31;	// Aileron trim
		else
			packet[7] = convert_channel_16b_limit(AILERON, 0, 32)-16;	// Aileron trim
		packet[8] = convert_channel_16b_limit(RUDDER, 0, 32)-16;			// Rudder trim
		if (sub_protocol == X5C1)
			packet[9] = convert_channel_16b_limit(ELEVATOR, 0, 63)-31;	// Elevator trim
		else
			packet[9] = convert_channel_16b_limit(ELEVATOR, 0, 32)-16;	// Elevator trim
		switch(sub_protocol)
		{
			case HONTAI:
				packet[0]  = 0x0B;
				packet[3] |= GET_FLAG(CH7_SW, 0x01);				// Picture
				packet[4] |= GET_FLAG(CH10_SW, 0x80)					// RTH
						  |  GET_FLAG(CH9_SW, 0x40);				// Headless
				packet[5] |= GET_FLAG(CH11_SW, 0x80)					// Calibrate
						  |  GET_FLAG(CH5_SW, 0x40);				// Flip
				packet[6] |= GET_FLAG(CH8_SW, 0x80);				// Video
				break;
			case JJRCX1:
				packet[0]  = GET_FLAG(CH6_SW, 0x02);				// Arm
				packet[3] |= GET_FLAG(CH7_SW, 0x01);				// Picture
				packet[4] |= 0x80;										// unknown
				packet[5] |= GET_FLAG(CH11_SW, 0x80)					// Calibrate
						  |  GET_FLAG(CH5_SW, 0x40);				// Flip
				packet[6] |= GET_FLAG(CH8_SW, 0x80);				// Video
				packet[8]  = 0xC0										// high rate, no rudder trim
						  |  GET_FLAG(CH10_SW, 0x02)					// RTH
						  |  GET_FLAG(CH9_SW, 0x01);				// Headless
				break;
			case X5C1:
				packet[0]  = 0x0B;
				packet[3] |= GET_FLAG(CH7_SW, 0x01);				// Picture
				packet[4]  = 0x80										// unknown
						  |  GET_FLAG(CH6_SW, 0x40);				// Lights
				packet[5] |= GET_FLAG(CH11_SW, 0x80)					// Calibrate
						  |  GET_FLAG(CH5_SW, 0x40);				// Flip
				packet[6] |= GET_FLAG(CH8_SW, 0x80);				// Video
				packet[8]  = 0xC0										// high rate, no rudder trim
						  |  GET_FLAG(CH10_SW, 0x02)					// RTH
						  |  GET_FLAG(CH9_SW, 0x01);				// Headless
				break;
			case FQ777_951:
				packet[0]  = GET_FLAG(CH7_SW, 0x01)					// Picture
						  |  GET_FLAG(CH8_SW, 0x02);				// Video
				packet[3] |= GET_FLAG(CH5_SW, 0x01);				// Flip
				packet[4] |= 0xC0;										// High rate (mid=0xa0, low=0x60)
				packet[5] |= GET_FLAG(CH11_SW, 0x80);				// Calibrate
				packet[6] |= GET_FLAG(CH9_SW, 0x40);				// Headless
				break;
		}
	}
	crc16(packet, bind ? HONTAI_BIND_PACKET_SIZE:HONTAI_PACKET_SIZE);

	// Power on, TX mode, 2byte CRC
	if(sub_protocol == JJRCX1)
		NRF24L01_SetTxRxMode(TX_EN);
	else
		XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? HONTAI_RF_BIND_CHANNEL : hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no %= 3;

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	if(sub_protocol == JJRCX1)
		NRF24L01_WritePayload(packet, bind ? HONTAI_BIND_PACKET_SIZE:HONTAI_PACKET_SIZE);
	else
		XN297_WritePayload(packet, bind ? HONTAI_BIND_PACKET_SIZE:HONTAI_PACKET_SIZE);

	NRF24L01_SetPower();
}

static void __attribute__((unused)) HONTAI_init()
{
	NRF24L01_Initialize();

	NRF24L01_SetTxRxMode(TX_EN);

	if(sub_protocol == JJRCX1)
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"\xd2\xb5\x99\xb3\x4a", 5);
	else
		XN297_SetTXAddr((const uint8_t*)"\xd2\xb5\x99\xb3\x4a", 5);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowldgement on all data pipes
	NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);								// Activate feature register
	if(sub_protocol == JJRCX1)
	{
		NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xff);	// JJRC uses dynamic payload length
		NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);			// match other stock settings even though AA disabled...
		NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
	}
	else
	{
		NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// no retransmits
		NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
		NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
	}
	NRF24L01_Activate(0x73);								// Deactivate feature register
}

const uint8_t PROGMEM HONTAI_hopping_frequency_nonels[][3] = {
	{0x05, 0x19, 0x28},     // Hontai
	{0x0a, 0x1e, 0x2d}};    // JJRC X1

const uint8_t PROGMEM HONTAI_addr_vals[4][16] = {
	{0x24, 0x26, 0x2a, 0x2c, 0x32, 0x34, 0x36, 0x4a, 0x4c, 0x4e, 0x54, 0x56, 0x5a, 0x64, 0x66, 0x6a},
	{0x92, 0x94, 0x96, 0x9a, 0xa4, 0xa6, 0xac, 0xb2, 0xb4, 0xb6, 0xca, 0xcc, 0xd2, 0xd4, 0xd6, 0xda},
	{0x93, 0x95, 0x99, 0x9b, 0xa5, 0xa9, 0xab, 0xad, 0xb3, 0xb5, 0xc9, 0xcb, 0xcd, 0xd3, 0xd5, 0xd9},
	{0x25, 0x29, 0x2b, 0x2d, 0x33, 0x35, 0x49, 0x4b, 0x4d, 0x59, 0x5b, 0x65, 0x69, 0x6b, 0x6d, 0x6e}};

static void __attribute__((unused)) HONTAI_init2()
{
	uint8_t data_tx_addr[5];

	//TX address
	data_tx_addr[0] = pgm_read_byte_near( &HONTAI_addr_vals[0][ rx_tx_addr[3]       & 0x0f]);
	data_tx_addr[1] = pgm_read_byte_near( &HONTAI_addr_vals[1][(rx_tx_addr[3] >> 4) & 0x0f]);
	data_tx_addr[2] = pgm_read_byte_near( &HONTAI_addr_vals[2][ rx_tx_addr[4]       & 0x0f]);
	data_tx_addr[3] = pgm_read_byte_near( &HONTAI_addr_vals[3][(rx_tx_addr[4] >> 4) & 0x0f]);
	data_tx_addr[4] = 0x24;
	if(sub_protocol == JJRCX1)
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, data_tx_addr, sizeof(data_tx_addr));
	else
		XN297_SetTXAddr(data_tx_addr, sizeof(data_tx_addr));

	//Hopping frequency table
	for(uint8_t i=0;i<3;i++)
		hopping_frequency[i]=pgm_read_byte_near( &HONTAI_hopping_frequency_nonels[sub_protocol == JJRCX1?1:0][i] );
	hopping_frequency_no=0;
}

static void __attribute__((unused)) HONTAI_initialize_txid()
{
	rx_tx_addr[4] = rx_tx_addr[2]; 
	if(sub_protocol == HONTAI || sub_protocol == FQ777_951)
	{
		rx_tx_addr[0] = 0x4c; // first three bytes some kind of model id? - set same as stock tx
		rx_tx_addr[1] = 0x4b;
		rx_tx_addr[2] = 0x3a;
	}
	else
	{
		rx_tx_addr[0] = 0x4b; // JJRC X1
		rx_tx_addr[1] = 0x59;
		rx_tx_addr[2] = 0x3a;
	}
}

uint16_t HONTAI_callback()
{
	if(bind_counter!=0)
	{
		HONTAI_send_packet(1);
		bind_counter--;
		if (bind_counter == 0)
		{
			HONTAI_init2();
			BIND_DONE;
		}
	}
	else
		HONTAI_send_packet(0);

	return sub_protocol == FQ777_951 ? FQ777_951_PACKET_PERIOD : HONTAI_PACKET_PERIOD;
}

uint16_t initHONTAI()
{
	BIND_IN_PROGRESS;	// autobind protocol
	bind_counter = HONTAI_BIND_COUNT;
	HONTAI_initialize_txid();
	HONTAI_init();
	return HONTAI_INITIAL_WAIT;
}
#endif
