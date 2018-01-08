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
// compatible with MJX WLH08, X600, X800, H26D, Eachine E010
// Last sync with hexfet new_protocols/mjxq_nrf24l01.c dated 2016-01-17

#if defined(MJXQ_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define MJXQ_BIND_COUNT		150
#define MJXQ_PACKET_PERIOD	4000  // Timeout for callback in uSec
#define MJXQ_INITIAL_WAIT	500
#define MJXQ_PACKET_SIZE	16
#define MJXQ_RF_NUM_CHANNELS	4
#define MJXQ_ADDRESS_LENGTH	5

// haven't figured out txid<-->rf channel mapping for MJX models
const uint8_t PROGMEM MJXQ_map_txid[][3] = {
				{0xF8, 0x4F, 0x1C},
				{0xC8, 0x6E, 0x02},
				{0x48, 0x6A, 0x40}	};
const uint8_t PROGMEM MJXQ_map_rfchan[][4] = {
				{0x0A, 0x46, 0x3A, 0x42},
				{0x0A, 0x3C, 0x36, 0x3F},
				{0x0A, 0x43, 0x36, 0x3F}	};

const uint8_t PROGMEM E010_map_txid[][2] = {
					{0x4F, 0x1C},
					{0x90, 0x1C},
					{0x24, 0x36},
					{0x7A, 0x40},
					{0x61, 0x31},
					{0x5D, 0x37},
					{0xFD, 0x4F},
					{0x86, 0x3C},
					{0x41, 0x22},
					{0xEE, 0xB3},
					{0x9A, 0xB2},
					{0xC0, 0x44},
					{0x2A, 0xFE},
					{0xD7, 0x6E},
					{0x3C, 0xCD}, // for this ID rx_tx_addr[2]=0x01
					{0xF5, 0x2B} // for this ID rx_tx_addr[2]=0x02
					};
const uint8_t PROGMEM E010_map_rfchan[][2] = {
					{0x3A, 0x35},
					{0x2E, 0x36},
					{0x32, 0x3E},
					{0x2E, 0x3C},
					{0x2F, 0x3B},
					{0x33, 0x3B},
					{0x33, 0x3B},
					{0x34, 0x3E},
					{0x34, 0x2F},
					{0x39, 0x3E},
					{0x2E, 0x38},
					{0x2E, 0x36},
					{0x2E, 0x38},
					{0x3A, 0x41},
					{0x32, 0x3E},
					{0x33, 0x3F}
					};

#define MJXQ_PAN_TILT_COUNT	16   // for H26D - match stock tx timing
#define MJXQ_PAN_DOWN		0x08
#define MJXQ_PAN_UP			0x04
#define MJXQ_TILT_DOWN		0x20
#define MJXQ_TILT_UP		0x10
static uint8_t __attribute__((unused)) MJXQ_pan_tilt_value()
{
// CH12_SW	PAN			// H26D
// CH13_SW	TILT
	uint8_t	pan = 0;
	packet_count++;
	if(packet_count & MJXQ_PAN_TILT_COUNT)
	{
		if(CH12_SW)
			pan=MJXQ_PAN_UP;
		if(Channel_data[CH12]<CHANNEL_MIN_COMMAND)
			pan=MJXQ_PAN_DOWN;
		if(CH13_SW)
			pan+=MJXQ_TILT_UP;
		if(Channel_data[CH13]<CHANNEL_MIN_COMMAND)
			pan+=MJXQ_TILT_DOWN;
	}
	return pan;
}

#define MJXQ_CHAN2TRIM(X) (((X) & 0x80 ? (X) : 0x7f - (X)) >> 1)
static void __attribute__((unused)) MJXQ_send_packet(uint8_t bind)
{
	packet[0] = convert_channel_8b(THROTTLE);
	packet[1] = convert_channel_s8b(RUDDER);
	packet[4] = 0x40;							// rudder does not work well with dyntrim
	packet[2] = 0x80 ^ convert_channel_s8b(ELEVATOR);
	packet[5] = (CH9_SW || CH14_SW) ? 0x40 : MJXQ_CHAN2TRIM(packet[2]);	// trim elevator
	packet[3] = convert_channel_s8b(AILERON);
	packet[6] = (CH9_SW || CH14_SW) ? 0x40 : MJXQ_CHAN2TRIM(packet[3]);	// trim aileron
	packet[7] = rx_tx_addr[0];
	packet[8] = rx_tx_addr[1];
	packet[9] = rx_tx_addr[2];

	packet[10] = 0x00;							// overwritten below for feature bits
	packet[11] = 0x00;							// overwritten below for X600
	packet[12] = 0x00;
	packet[13] = 0x00;

	packet[14] = 0xC0;							// bind value

// CH5_SW	FLIP
// CH6_SW	LED / ARM
// CH7_SW	PICTURE
// CH8_SW	VIDEO
// CH9_SW	HEADLESS
// CH10_SW	RTH
// CH11_SW	AUTOFLIP	// X800, X600
// CH12_SW	PAN
// CH13_SW	TILT
// CH14_SW	XTRM		// Dyntrim, don't use if high.
	switch(sub_protocol)
	{
		case H26WH:
		case H26D:
			packet[10]=MJXQ_pan_tilt_value();
			// fall through on purpose - no break
		case WLH08:
		case E010:
			packet[10] += GET_FLAG(CH10_SW, 0x02)	//RTH
						| GET_FLAG(CH9_SW, 0x01);	//HEADLESS
			if (!bind)
			{
				packet[14] = 0x04
						| GET_FLAG(CH5_SW, 0x01)	//FLIP
						| GET_FLAG(CH7_SW, 0x08)	//PICTURE
						| GET_FLAG(CH8_SW, 0x10)	//VIDEO
						| GET_FLAG(!CH6_SW, 0x20);	// LED or air/ground mode
				if(sub_protocol==H26WH)
				{
					packet[10] |=0x40;					//High rate
					packet[14] &= ~0x24;				// unset air/ground & arm flags
					packet[14] |= GET_FLAG(CH6_SW, 0x02);	// arm
				}
			}
			break;
		case X600:
			packet[10] = GET_FLAG(!CH6_SW, 0x02);	//LED
			packet[11] = GET_FLAG(CH10_SW, 0x01);	//RTH
			if (!bind)
			{
				packet[14] = 0x02						// always high rates by bit2 = 1
						| GET_FLAG(CH5_SW, 0x04)	//FLIP
						| GET_FLAG(CH11_SW, 0x10)	//AUTOFLIP
						| GET_FLAG(CH9_SW, 0x20);	//HEADLESS
			}
			break;
		case X800:
		default:
			packet[10] = 0x10
					| GET_FLAG(!CH6_SW, 0x02)	//LED
					| GET_FLAG(CH11_SW, 0x01);	//AUTOFLIP
			if (!bind)
			{
				packet[14] = 0x02						// always high rates by bit2 = 1
						| GET_FLAG(CH5_SW, 0x04)	//FLIP
						| GET_FLAG(CH7_SW, 0x08)	//PICTURE
						| GET_FLAG(CH8_SW, 0x10);	//VIDEO
			}
			break;
	}

	uint8_t sum = packet[0];
	for (uint8_t i=1; i < MJXQ_PACKET_SIZE-1; i++) sum += packet[i];
	packet[15] = sum;

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++ / 2]);
	hopping_frequency_no %= 2 * MJXQ_RF_NUM_CHANNELS;	// channels repeated

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	// Power on, TX mode, 2byte CRC and send packet
	if (sub_protocol == H26D || sub_protocol == H26WH)
	{
		NRF24L01_SetTxRxMode(TX_EN);
		NRF24L01_WritePayload(packet, MJXQ_PACKET_SIZE);
	}
	else
	{
		XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
		XN297_WritePayload(packet, MJXQ_PACKET_SIZE);
	}
	NRF24L01_SetPower();
}

static void __attribute__((unused)) MJXQ_init()
{
	uint8_t addr[MJXQ_ADDRESS_LENGTH];
	memcpy(addr, "\x6d\x6a\x77\x77\x77", MJXQ_ADDRESS_LENGTH);
	if (sub_protocol == WLH08)
		memcpy(hopping_frequency, "\x12\x22\x32\x42", MJXQ_RF_NUM_CHANNELS);
	else
		if (sub_protocol == H26D || sub_protocol == H26D || sub_protocol == E010)
			memcpy(hopping_frequency, "\x2e\x36\x3e\x46", MJXQ_RF_NUM_CHANNELS);
		else
		{
			memcpy(hopping_frequency, "\x0a\x35\x42\x3d", MJXQ_RF_NUM_CHANNELS);
			memcpy(addr, "\x6d\x6a\x73\x73\x73", MJXQ_ADDRESS_LENGTH);
		}
	
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	if (sub_protocol == H26D || sub_protocol == H26WH)
	{
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW,		0x03);		// 5-byte RX/TX address
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, MJXQ_ADDRESS_LENGTH);
	}
	else
		XN297_SetTXAddr(addr, MJXQ_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS,		0x70);		// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA,		0x00);		// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR,	0x01);		// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR,	0x00);		// no retransmits
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0,		MJXQ_PACKET_SIZE);
	if (sub_protocol == E010)
		NRF24L01_SetBitrate(NRF24L01_BR_250K);				// 250K
	else
		NRF24L01_SetBitrate(NRF24L01_BR_1M);				// 1Mbps
	NRF24L01_SetPower();
}

static void __attribute__((unused)) MJXQ_init2()
{
	switch(sub_protocol)
	{
		case H26D:
			memcpy(hopping_frequency, "\x32\x3e\x42\x4e", MJXQ_RF_NUM_CHANNELS);
			break;
		case H26WH:
			memcpy(hopping_frequency, "\x37\x32\x47\x42", MJXQ_RF_NUM_CHANNELS);
			break;
		case E010:
			for(uint8_t i=0;i<2;i++)
			{
				hopping_frequency[i]=pgm_read_byte_near( &E010_map_rfchan[rx_tx_addr[3]&0x0F][i] );
				hopping_frequency[i+2]=hopping_frequency[i]+0x10;
			}
			break;
		case WLH08:
			// do nothing
			break;
		default:
			for(uint8_t i=0;i<MJXQ_RF_NUM_CHANNELS;i++)
				hopping_frequency[i]=pgm_read_byte_near( &MJXQ_map_rfchan[rx_tx_addr[3]%3][i] );
			break;
	}
}

static void __attribute__((unused)) MJXQ_initialize_txid()
{
	switch(sub_protocol)
	{
		case H26WH:
			memcpy(rx_tx_addr, "\xa4\x03\x00", 3); 
			break;
		case E010:
			for(uint8_t i=0;i<2;i++)
				rx_tx_addr[i]=pgm_read_byte_near( &E010_map_txid[rx_tx_addr[3]&0x0F][i] );
			if((rx_tx_addr[3]&0x0E) == 0x0E)
				rx_tx_addr[2]=(rx_tx_addr[3]&0x01)+1;
			else
				rx_tx_addr[2]=0;
			break;
		case WLH08:
			rx_tx_addr[0]&=0xF8;
			rx_tx_addr[2]=rx_tx_addr[3];	// Make use of RX_Num
			break;
		default:
			for(uint8_t i=0;i<3;i++)
				rx_tx_addr[i]=pgm_read_byte_near( &MJXQ_map_txid[rx_tx_addr[3]%3][i] );
			break;
	}
}

uint16_t MJXQ_callback()
{
	if(IS_BIND_DONE)
		MJXQ_send_packet(0);
	else
	{
		if (bind_counter == 0)
		{
			MJXQ_init2();
			BIND_DONE;
		}
		else
		{
			bind_counter--;
			MJXQ_send_packet(1);
		}
	}

    return MJXQ_PACKET_PERIOD;
}

uint16_t initMJXQ(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	bind_counter = MJXQ_BIND_COUNT;
    MJXQ_initialize_txid();
    MJXQ_init();
	packet_count=0;
	return MJXQ_INITIAL_WAIT+MJXQ_PACKET_PERIOD;
}

#endif
