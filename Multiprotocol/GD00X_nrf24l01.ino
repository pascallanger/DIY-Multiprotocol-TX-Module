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
// Compatible with GD005 C-17 and GD006 DA62 planes.

#if defined(GD00X_NRF24L01_INO)

#include "iface_nrf24l01.h"

//#define FORCE_GD00X_ORIGINAL_ID

#define GD00X_INITIAL_WAIT    500
#define GD00X_PACKET_PERIOD   3500
#define GD00X_RF_BIND_CHANNEL 2
#define GD00X_PAYLOAD_SIZE    15
#define GD00X_BIND_COUNT	  857	//3sec

// flags going to packet[11]
#define	GD00X_FLAG_DR		0x08
#define	GD00X_FLAG_LIGHT	0x04

static void __attribute__((unused)) GD00X_send_packet()
{
	packet[0] = IS_BIND_IN_PROGRESS?0xAA:0x55;
	memcpy(packet+1,rx_tx_addr,4);
	uint16_t channel=convert_channel_ppm(AILERON);
	packet[5 ] = channel;
	packet[6 ] = channel>>8;
	channel=convert_channel_ppm(THROTTLE);
	packet[7 ] = channel;
	packet[8 ] = channel>>8;
	channel=convert_channel_ppm(CH5);		// TRIM
	packet[9 ] = channel;
	packet[10] = channel>>8;
	packet[11] = GD00X_FLAG_DR						// Force high rate
			   | GET_FLAG(CH6_SW, GD00X_FLAG_LIGHT);
	packet[12] = 0x00;
	packet[13] = 0x00;
	packet[14] = 0x00;

	// Power on, TX mode, CRC enabled
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if(IS_BIND_DONE)
	{
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
		hopping_frequency_no &= 3;	// 4 RF channels
	}

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, GD00X_PAYLOAD_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) GD00X_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", 5);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, GD00X_RF_BIND_CHANNEL);	// Bind channel
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);		// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);	// Enable data pipe 0 only
	NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps
	NRF24L01_SetPower();
}

static void __attribute__((unused)) GD00X_initialize_txid()
{
	uint8_t start=76+(rx_tx_addr[0]&0x03);
	for(uint8_t i=0; i<4;i++)
		hopping_frequency[i]=start-(i<<1);
	#ifdef FORCE_GD00X_ORIGINAL_ID
		rx_tx_addr[0]=0x1F;					// or 0xA5 or 0x26
		rx_tx_addr[1]=0x39;					// or 0x37 or 0x35
		rx_tx_addr[2]=0x12;					// Constant on 3 TXs
		rx_tx_addr[3]=0x13;					// Constant on 3 TXs
		for(uint8_t i=0; i<4;i++)
			hopping_frequency[i]=79-(i<<1);	// or 77 or 78
	#endif
}

uint16_t GD00X_callback()
{
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
			BIND_DONE;
	GD00X_send_packet();
	return GD00X_PACKET_PERIOD;
}

uint16_t initGD00X()
{
	BIND_IN_PROGRESS;	// autobind protocol
	GD00X_initialize_txid();
	GD00X_init();
	hopping_frequency_no = 0;
	bind_counter=GD00X_BIND_COUNT;
	return GD00X_INITIAL_WAIT;
}

#endif
