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
// Compatible with KF606 plane.

#if defined(KF606_NRF24L01_INO)

#include "iface_nrf24l01.h"

//#define FORCE_KF606_ORIGINAL_ID

#define KF606_INITIAL_WAIT    500
#define KF606_PACKET_PERIOD   3000
#define KF606_RF_BIND_CHANNEL 7
#define KF606_PAYLOAD_SIZE    4
#define KF606_BIND_COUNT	  857	//3sec

static void __attribute__((unused)) KF606_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xAA;
		memcpy(&packet[1],rx_tx_addr,3);
	}
	else
	{
		packet[0]= 0x55;
		packet[1]= convert_channel_8b(THROTTLE);					// 0..255
		packet[2]= convert_channel_16b_limit(AILERON,0x20,0xE0);	// Low:50..80..AF High:3E..80..C1 
		packet[3]= convert_channel_16b_limit(CH5,0xC1,0xDF);		// Trim on a separated channel C1..D0..DF
	}
	// Power on, TX mode, CRC enabled
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if(IS_BIND_DONE)
	{
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
		hopping_frequency_no ^= 1;			// 2 RF channels
	}

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, KF606_PAYLOAD_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) KF606_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t*)"\xe7\xe7\xe7\xe7\xe7", 5);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, KF606_RF_BIND_CHANNEL);	// Bind channel
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);		// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);	// Enable data pipe 0 only
	NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
	NRF24L01_SetPower();
}

static void __attribute__((unused)) KF606_initialize_txid()
{
	rx_tx_addr[0]=rx_tx_addr[3];	// Use RX_num;
	hopping_frequency[0]=(rx_tx_addr[0]&0x3F)+9;
	hopping_frequency[1]=hopping_frequency[0]+3;
	#ifdef FORCE_KF606_ORIGINAL_ID
		//TX1
		rx_tx_addr[0]=0x57;
		rx_tx_addr[1]=0x02;
		rx_tx_addr[2]=0x00;
		hopping_frequency[0]=0x20;
		hopping_frequency[0]=0x23;
		//TX2
		rx_tx_addr[0]=0x25;
		rx_tx_addr[1]=0x04;
		rx_tx_addr[2]=0x00;
		hopping_frequency[0]=0x2E;
		hopping_frequency[0]=0x31;
	#endif
}

uint16_t KF606_callback()
{
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 3);
		}
	KF606_send_packet();
	return KF606_PACKET_PERIOD;
}

uint16_t initKF606()
{
	BIND_IN_PROGRESS;	// autobind protocol
	KF606_initialize_txid();
	KF606_init();
	hopping_frequency_no = 0;
	bind_counter=KF606_BIND_COUNT;
	return KF606_INITIAL_WAIT;
}

#endif
