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
// compatible with MJX WLH08, X600, X800, H26D
// Last sync with hexfet new_protocols/mjxq_nrf24l01.c dated 2016-01-17

#if defined(MJXQ_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define MJXQ_BIND_COUNT		150
#define MJXQ_PACKET_PERIOD	4000  // Timeout for callback in uSec
#define MJXQ_INITIAL_WAIT	500
#define MJXQ_PACKET_SIZE	16
#define MJXQ_RF_NUM_CHANNELS	4
#define MJXQ_ADDRESS_LENGTH	5

#define MJXQ_PAN_TILT_COUNT	16   // for H26D - match stock tx timing
#define MJXQ_PAN_DOWN		0x08
#define MJXQ_PAN_UP			0x04
#define MJXQ_TILT_DOWN		0x20
#define MJXQ_TILT_UP		0x10
static uint8_t __attribute__((unused)) MJXQ_pan_tilt_value()
{
// Servo_AUX8	PAN			// H26D
// Servo_AUX9	TILT
	uint8_t	pan = 0;
	packet_count++;
	if(packet_count & MJXQ_PAN_TILT_COUNT)
	{
		if(Servo_AUX8)
			pan=MJXQ_PAN_UP;
		if(Servo_data[AUX8]<PPM_MIN_COMMAND)
			pan=MJXQ_PAN_DOWN;
		if(Servo_data[AUX9]>PPM_MIN_COMMAND)
			pan=MJXQ_TILT_UP;
		if(Servo_data[AUX9]<PPM_MIN_COMMAND)
			pan=MJXQ_TILT_DOWN;
	}
	return pan;
}

#define MJXQ_CHAN2TRIM(X) (((X) & 0x80 ? (X) : 0x7f - (X)) >> 1)
static void __attribute__((unused)) MJXQ_send_packet(uint8_t bind)
{
	packet[0] = convert_channel_8b(THROTTLE);
	packet[1] = convert_channel_s8b(RUDDER);
	packet[4] = 0x40;							// rudder does not work well with dyntrim
	packet[2] = convert_channel_s8b(ELEVATOR);
	packet[5] = MJXQ_CHAN2TRIM(packet[2]);		// trim elevator
	packet[3] = convert_channel_s8b(AILERON);
	packet[6] = MJXQ_CHAN2TRIM(packet[3]);		// trim aileron
	packet[7] = rx_tx_addr[0];
	packet[8] = rx_tx_addr[1];
	packet[9] = rx_tx_addr[2];

	packet[10] = 0x00;							// overwritten below for feature bits
	packet[11] = 0x00;							// overwritten below for X600
	packet[12] = 0x00;
	packet[13] = 0x00;

	packet[14] = 0xC0;							// bind value

// Servo_AUX1	FLIP
// Servo_AUX2	LED
// Servo_AUX3	PICTURE
// Servo_AUX4	VIDEO
// Servo_AUX5	HEADLESS
// Servo_AUX6	RTH
// Servo_AUX7	AUTOFLIP	// X800, X600
	switch(sub_protocol)
	{
		case H26D:
			packet[10]=MJXQ_pan_tilt_value();
			// fall through on purpose - no break
		case WLH08:
			packet[10] += GET_FLAG(Servo_AUX6, 0x02)	//RTH
						| GET_FLAG(Servo_AUX5, 0x01);	//HEADLESS
			if (!bind)
			{
				packet[14] = 0x04
						| GET_FLAG(Servo_AUX1, 0x01)	//FLIP
						| GET_FLAG(Servo_AUX3, 0x08)	//PICTURE
						| GET_FLAG(Servo_AUX4, 0x10)	//VIDEO
						| GET_FLAG(!Servo_AUX2, 0x20);	// air/ground mode
			}
			break;
		case X600:
			if(Servo_AUX5)	//HEADLESS
			{ // driven trims cause issues when headless is enabled
				packet[5] = 0x40;
				packet[6] = 0x40;
			}
			packet[10] = GET_FLAG(!Servo_AUX2, 0x02);	//LED
			packet[11] = GET_FLAG(Servo_AUX6, 0x01);	//RTH
			if (!bind)
			{
				packet[14] = 0x02						// always high rates by bit2 = 1
						| GET_FLAG(Servo_AUX1, 0x04)	//FLIP
						| GET_FLAG(Servo_AUX7, 0x10)	//AUTOFLIP
						| GET_FLAG(Servo_AUX5, 0x20);	//HEADLESS
			}
			break;
		case X800:
		default:
			packet[10] = 0x10
					| GET_FLAG(!Servo_AUX2, 0x02)	//LED
					| GET_FLAG(Servo_AUX7, 0x01);	//AUTOFLIP
			if (!bind)
			{
				packet[14] = 0x02						// always high rates by bit2 = 1
						| GET_FLAG(Servo_AUX1, 0x04)	//FLIP
						| GET_FLAG(Servo_AUX3, 0x08)	//PICTURE
						| GET_FLAG(Servo_AUX4, 0x10);	//VIDEO
			}
			break;
	}

	uint8_t sum = packet[0];
	for (uint8_t i=1; i < MJXQ_PACKET_SIZE-1; i++) sum += packet[i];
	packet[15] = sum;

	// Power on, TX mode, 2byte CRC
	if (sub_protocol == H26D)
		NRF24L01_SetTxRxMode(TX_EN);
	else
		XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++ / 2]);
	hopping_frequency_no %= 2 * MJXQ_RF_NUM_CHANNELS;	// channels repeated

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	if (sub_protocol == H26D)
		NRF24L01_WritePayload(packet, MJXQ_PACKET_SIZE);
	else
		XN297_WritePayload(packet, MJXQ_PACKET_SIZE);

	NRF24L01_SetPower();
}

static void __attribute__((unused)) MJXQ_init()
{
	uint8_t addr[MJXQ_ADDRESS_LENGTH];
	memcpy(addr, "\x6d\x6a\x77\x77\x77", MJXQ_ADDRESS_LENGTH);
	if (sub_protocol == WLH08)
		memcpy(hopping_frequency, "\x12\x22\x32\x42", MJXQ_RF_NUM_CHANNELS);
	else
		if (sub_protocol == H26D)
			memcpy(hopping_frequency, "\x36\x3e\x46\x2e", MJXQ_RF_NUM_CHANNELS);
		else
		{
			memcpy(hopping_frequency, "\x0a\x35\x42\x3d", MJXQ_RF_NUM_CHANNELS);
			memcpy(addr, "\x6d\x6a\x73\x73\x73", MJXQ_RF_NUM_CHANNELS);
		}

	
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	if (sub_protocol == H26D)
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, MJXQ_ADDRESS_LENGTH);
	else
		XN297_SetTXAddr(addr, MJXQ_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);		// no retransmits
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, MJXQ_PACKET_SIZE);	// rx pipe 0 (used only for blue board)
	NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
	NRF24L01_SetPower();
}

static void __attribute__((unused)) MJXQ_init2()
{
	// haven't figured out txid<-->rf channel mapping for MJX models
	static const uint8_t rf_map[][4] = {
				{0x0A, 0x46, 0x3A, 0x42},
				{0x0A, 0x3C, 0x36, 0x3F},
				{0x0A, 0x43, 0x36, 0x3F}	};
	if (sub_protocol == H26D)
		memcpy(hopping_frequency, "\x32\x3e\x42\x4e", MJXQ_RF_NUM_CHANNELS);
	else
		if (sub_protocol == WLH08)
			memcpy(hopping_frequency, rf_map[rx_tx_addr[0]%3], MJXQ_RF_NUM_CHANNELS);
}

static void __attribute__((unused)) MJXQ_initialize_txid()
{
	// haven't figured out txid<-->rf channel mapping for MJX models
	static const uint8_t tx_map[][3]={
				{0xF8, 0x4F, 0x1C},
				{0xC8, 0x6E, 0x02},
				{0x48, 0x6A, 0x40}	};
	if (sub_protocol == WLH08)
		rx_tx_addr[0]&=0xF8;	// txid must be multiple of 8
	else
		memcpy(rx_tx_addr,tx_map[rx_tx_addr[0]%3],3);
}

uint16_t MJXQ_callback()
{
	if(IS_BIND_DONE_on)
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
