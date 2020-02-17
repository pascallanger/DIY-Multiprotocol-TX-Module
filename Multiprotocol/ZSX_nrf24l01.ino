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
// Compatible with JJRC ZSX-280 plane.

#if defined(ZSX_NRF24L01_INO)

#include "iface_nrf250k.h"

//#define FORCE_ZSX_ORIGINAL_ID

#define ZSX_INITIAL_WAIT	500
#define ZSX_PACKET_PERIOD	10093
#define ZSX_RF_BIND_CHANNEL	7
#define ZSX_PAYLOAD_SIZE	6
#define ZSX_BIND_COUNT		50
#define ZSX_RF_NUM_CHANNELS	1

static void __attribute__((unused)) ZSX_send_packet()
{
	memcpy(&packet[1],rx_tx_addr,3);
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xAA;
		packet[4] = 0x00;
		packet[5] = 0x00;
	}
	else
	{
		packet[0]= 0x55;
		packet[4]= 0xFF-convert_channel_8b(RUDDER);		// FF..80..01
		packet[5]= convert_channel_8b(THROTTLE)>>1		// 0..7F
				| GET_FLAG(CH5_SW, 0x80);				// Light
	}

	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, ZSX_PAYLOAD_SIZE);

	NRF24L01_SetPower();		// Set tx_power
}

static void __attribute__((unused)) ZSX_initialize_txid()
{
	rx_tx_addr[0]=rx_tx_addr[3];	// Use RX_num;
	#ifdef FORCE_ZSX_ORIGINAL_ID
		//TX1
		rx_tx_addr[0]=0x03;
		rx_tx_addr[1]=0x01;
		rx_tx_addr[2]=0xC3;
	#endif
}

static void __attribute__((unused)) ZSX_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);		// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);	// Enable data pipe 0 only
	NRF24L01_SetBitrate(NRF24L01_BR_1M);			// 1Mbps
	NRF24L01_SetPower();
	XN297_SetTXAddr((uint8_t*)"\xc1\xc2\xc3", 3);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, ZSX_RF_BIND_CHANNEL);	// Set bind channel
}

uint16_t ZSX_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(ZSX_PACKET_PERIOD);
	#endif
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 3);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x00);
		}
	ZSX_send_packet();
	return ZSX_PACKET_PERIOD;
}

uint16_t initZSX()
{
	BIND_IN_PROGRESS;	// autobind protocol
	ZSX_initialize_txid();
	ZSX_init();
	bind_counter=ZSX_BIND_COUNT;
	return ZSX_INITIAL_WAIT;
}

#endif

// XN297 spped 1Mb, scrambled
// Bind
//   channel 7
//   address: C1 C2 C3
//   P(6)= AA 03 01 C3 00 00
//   03 01 C3 <- normal address
// Normal
//   channel 0 and seems to be fixed
//   address: 03 01 C3
//   P(6)= 55 03 01 C3 80 00
//   03 01 C3 <- normal address
//   80 <- rudder FF..80..01
//   00 <- throttle 00..7F, light flag 0x80