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
// compatible with EAchine 3D X4, CG023/CG031, Attop YD-822/YD-829/YD-829C and H8_3D/JJRC H20/H22

#if defined(CG023_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CG023_PACKET_PERIOD		8200 // Timeout for callback in uSec
#define CG023_INITIAL_WAIT		500
#define CG023_PACKET_SIZE		15   // packets have 15-byte payload
#define CG023_RF_BIND_CHANNEL	0x2D
#define CG023_BIND_COUNT		500  // 4 seconds
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

static void __attribute__((unused)) CG023_send_packet(uint8_t bind)
{
	// throttle : 0x00 - 0xFF
	throttle=convert_channel_8b(THROTTLE);
	// rudder
	rudder = convert_channel_16b_limit(RUDDER,0x44,0xBC);	// yaw right : 0x80 (neutral) - 0xBC (right)
	if (rudder<=0x80)
		rudder=0x80-rudder;							// yaw left : 0x00 (neutral) - 0x3C (left)
	// elevator : 0xBB - 0x7F - 0x43
	elevator = convert_channel_16b_limit(ELEVATOR, 0x43, 0xBB); 
	// aileron : 0x43 - 0x7F - 0xBB
	aileron = convert_channel_16b_limit(AILERON, 0x43, 0xBB); 
	
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
	packet[5] = throttle;
	packet[6] = rudder;
	packet[7] = elevator;
	packet[8] = aileron;
	// throttle trim : 0x30 - 0x20 - 0x10
	packet[9] = 0x20; // neutral
	// neutral trims
	packet[10] = 0x20;
	packet[11] = 0x40;
	packet[12] = 0x40;
	if(sub_protocol==CG023)
	{
		// rate
		packet[13] =					  CG023_FLAG_RATE_HIGH
					| GET_FLAG(CH5_SW,CG023_FLAG_FLIP)
					| GET_FLAG(CH6_SW,CG023_FLAG_LED_OFF)
					| GET_FLAG(CH7_SW,CG023_FLAG_STILL)
					| GET_FLAG(CH8_SW,CG023_FLAG_VIDEO)
					| GET_FLAG(CH9_SW,CG023_FLAG_EASY);
	}
	else
	{// YD829
		// rate
		packet[13] =					  YD829_FLAG_RATE_HIGH
					| GET_FLAG(CH5_SW,YD829_FLAG_FLIP)
					| GET_FLAG(CH7_SW,YD829_FLAG_STILL)
					| GET_FLAG(CH8_SW,YD829_FLAG_VIDEO)
					| GET_FLAG(CH9_SW,YD829_FLAG_HEADLESS);
	}
	packet[14] = 0;
	
	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
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

static void __attribute__((unused)) CG023_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t *)"\x26\xA8\x67\x35\xCC", 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower();
}

uint16_t CG023_callback()
{
	if(IS_BIND_DONE)
		CG023_send_packet(0);
	else
	{
		if (bind_counter == 0)
			BIND_DONE;
		else
		{
			CG023_send_packet(1);
			bind_counter--;
		}
	}
	return	packet_period;
}

static void __attribute__((unused)) CG023_initialize_txid()
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
	if(sub_protocol==CG023)
		packet_period=CG023_PACKET_PERIOD;
	else // YD829
		packet_period=YD829_PACKET_PERIOD;
	return	CG023_INITIAL_WAIT+YD829_PACKET_PERIOD;
}

#endif
