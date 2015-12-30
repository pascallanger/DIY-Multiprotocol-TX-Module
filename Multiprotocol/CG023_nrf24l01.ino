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
// compatible with EAchine 3D X4, CG023/CG031, Attop YD-822/YD-829/YD-829C

#if defined(CG023_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CG023_PACKET_PERIOD		8200 // Timeout for callback in uSec
#define CG023_INITIAL_WAIT		500
#define CG023_PACKET_SIZE		15   // packets have 15-byte payload
#define CG023_RF_BIND_CHANNEL	0x2D
#define CG023_BIND_COUNT		800  // 6 seconds
#define YD829_PACKET_PERIOD		4100 // Timeout for callback in uSec

enum CG023_FLAGS {
    // flags going to packet[13]
    CG023_FLAG_FLIP     = 0x01, 
    CG023_FLAG_EASY     = 0x02, 
    CG023_FLAG_VIDEO    = 0x04, 
    CG023_FLAG_STILL    = 0x08, 
    CG023_FLAG_LED_OFF  = 0x10,
    CG023_FLAG_RATE_LOW = 0x00,
    CG023_FLAG_RATE_MID = 0x20,
    CG023_FLAG_RATE_HIGH= 0x40,
};

enum YD829_FLAGS {
    // flags going to packet[13] (YD-829)
    YD829_FLAG_FLIP     = 0x01,
    YD829_MASK_RATE     = 0x0C,
    YD829_FLAG_RATE_MID = 0x04,
    YD829_FLAG_RATE_HIGH= 0x08,
    YD829_FLAG_HEADLESS = 0x20,
    YD829_FLAG_VIDEO    = 0x40, 
    YD829_FLAG_STILL    = 0x80,
};

enum CG023_PHASES {
    CG023_BIND = 0,
    CG023_DATA
};

void CG023_send_packet(uint8_t bind)
{
	if (bind)
		packet[0]= 0xaa;
	else
		packet[0]= 0x55;
	// transmitter id
	packet[1] = rx_tx_addr[0]; 
	packet[2] = rx_tx_addr[1];
	// unknown
	packet[3] = 0x00;
	packet[4] = 0x00;
	// throttle : 0x00 - 0xFF
	packet[5] = convert_channel_8b(THROTTLE);
	// rudder
	packet[6] = convert_channel_8b_scale(RUDDER,0x44,0xBC);	// yaw right : 0x80 (neutral) - 0xBC (right)
	if (packet[6]<=0x80)
		packet[6]=0x80-packet[6];							// yaw left : 0x00 (neutral) - 0x3C (left)
	// elevator : 0xBB - 0x7F - 0x43
	packet[7] = convert_channel_8b_scale(ELEVATOR, 0x43, 0xBB); 
	// aileron : 0x43 - 0x7F - 0xBB
	packet[8] = convert_channel_8b_scale(AILERON, 0x43, 0xBB); 
	// throttle trim : 0x30 - 0x20 - 0x10
	packet[9] = 0x20; // neutral
	// neutral trims
	packet[10] = 0x20;
	packet[11] = 0x40;
	packet[12] = 0x40;
	if(sub_protocol==CG023)
	{
		// rate
		packet[13] = CG023_FLAG_RATE_HIGH; 
		// flags
		if(Servo_data[AUX1] > PPM_SWITCH)
			packet[13] |= CG023_FLAG_FLIP;
		if(Servo_data[AUX2] > PPM_SWITCH)
			packet[13] |= CG023_FLAG_LED_OFF;
		if(Servo_data[AUX3] > PPM_SWITCH)
			packet[13] |= CG023_FLAG_STILL;
		if(Servo_data[AUX4] > PPM_SWITCH)
			packet[13] |= CG023_FLAG_VIDEO;
		if(Servo_data[AUX5] > PPM_SWITCH)
			packet[13] |= CG023_FLAG_EASY;
	}
	else
	{// YD829
		// rate
		packet[13] = YD829_FLAG_RATE_HIGH; 
		// flags
		if(Servo_data[AUX1] > PPM_SWITCH)
			packet[13] |= YD829_FLAG_FLIP;
		if(Servo_data[AUX3] > PPM_SWITCH)
			packet[13] |= YD829_FLAG_STILL;
		if(Servo_data[AUX4] > PPM_SWITCH)
			packet[13] |= YD829_FLAG_VIDEO;
		if(Servo_data[AUX5] > PPM_SWITCH)
			packet[13] |= YD829_FLAG_HEADLESS;
	}
	packet[14] = 0;

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, CG023_RF_BIND_CHANNEL);
	else
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency_no);
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, CG023_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

void CG023_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    XN297_SetTXAddr((uint8_t *)"\x26\xA8\x67\x35\xCC", 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower();
}

uint16_t CG023_callback()
{
	switch (phase)
	{
		case CG023_BIND:
			if (bind_counter == 0)
			{
				phase = CG023_DATA;
				BIND_DONE;
			}
			else
			{
				CG023_send_packet(1);
				bind_counter--;
			}
			break;
		case CG023_DATA:
			CG023_send_packet(0);
			break;
	}
	if(sub_protocol==CG023)
		return CG023_PACKET_PERIOD;
	else
		return YD829_PACKET_PERIOD;
}

void CG023_initialize_txid()
{
	rx_tx_addr[0]= 0x80 | (rx_tx_addr[0] % 0x40);
	if( rx_tx_addr[0] == 0xAA)			// avoid using same freq for bind and data channel
		rx_tx_addr[0] ++;
	
	hopping_frequency_no = rx_tx_addr[0] - 0x7D;	// rf channel for data packets
}

uint16_t initCG023(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = CG023_BIND_COUNT;
	CG023_initialize_txid();
	CG023_init();
	phase=CG023_BIND;
	if(sub_protocol==CG023)
		return CG023_INITIAL_WAIT+CG023_PACKET_PERIOD;
	else
		return CG023_INITIAL_WAIT+YD829_PACKET_PERIOD;
}

#endif
