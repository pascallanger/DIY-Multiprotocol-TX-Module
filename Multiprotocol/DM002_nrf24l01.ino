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
// compatible with DM002

#if defined(DM002_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define DM002_PACKET_PERIOD		6100 // Timeout for callback in uSec
#define DM002_INITIAL_WAIT		500
#define DM002_PACKET_SIZE		12   // packets have 12-byte payload
#define DM002_RF_BIND_CHANNEL	0x27
#define DM002_BIND_COUNT		655  // 4 seconds


enum DM002_FLAGS {
    // flags going to packet[9]
    DM002_FLAG_FLIP		= 0x01, 
    DM002_FLAG_LED		= 0x02, 
    DM002_FLAG_MEDIUM	= 0x04, 
    DM002_FLAG_HIGH		= 0x08, 
    DM002_FLAG_RTH		= 0x10,
    DM002_FLAG_HEADLESS	= 0x20,
    DM002_FLAG_CAMERA1	= 0x40,
    DM002_FLAG_CAMERA2	= 0x80,
};

static void __attribute__((unused)) DM002_send_packet(uint8_t bind)
{
	memcpy(packet+5,(uint8_t *)"\x00\x7F\x7F\x7F\x00\x00\x00",7);
	if(bind)
	{
		packet[0] = 0xAA;
		packet[1] = rx_tx_addr[0]; 
		packet[2] = rx_tx_addr[1];
		packet[3] = rx_tx_addr[2];
		packet[4] = rx_tx_addr[3];
	}
	else
	{
		packet[0]=0x55;
		// Throttle : 0 .. 200
		packet[1]=convert_channel_16b_limit(THROTTLE,0,200);
		// Other channels min 0x57, mid 0x7F, max 0xA7
		packet[2] = convert_channel_16b_limit(RUDDER,0x57,0xA7);
		packet[3] = convert_channel_16b_limit(AILERON, 0x57,0xA7);
		packet[4] = convert_channel_16b_limit(ELEVATOR, 0xA7, 0x57);
		// Features
		packet[9] =   GET_FLAG(CH5_SW,DM002_FLAG_FLIP)
					| GET_FLAG(!CH6_SW,DM002_FLAG_LED)
					| GET_FLAG(CH7_SW,DM002_FLAG_CAMERA1)
					| GET_FLAG(CH8_SW,DM002_FLAG_CAMERA2)
					| GET_FLAG(CH9_SW,DM002_FLAG_HEADLESS)
					| GET_FLAG(CH10_SW,DM002_FLAG_RTH)
					| GET_FLAG(!CH11_SW,DM002_FLAG_HIGH);
		// Packet counter
		if(packet_count&0x03)
		{
			packet_count++;
			hopping_frequency_no++;
			hopping_frequency_no&=4;
		}
		packet_count&=0x0F;
		packet[10] = packet_count;
		packet_count++;
	}
	//CRC
	for(uint8_t i=0;i<DM002_PACKET_SIZE-1;i++)
		packet[11]+=packet[i];
	
	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, DM002_RF_BIND_CHANNEL);
	else
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, DM002_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) DM002_init()
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

uint16_t DM002_callback()
{
	if(IS_BIND_DONE)
		DM002_send_packet(0);
	else
	{
		if (bind_counter == 0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 5);
		}
		else
		{
			DM002_send_packet(1);
			bind_counter--;
		}
	}
	return	DM002_PACKET_PERIOD;
}

static void __attribute__((unused)) DM002_initialize_txid()
{
	// Only 3 IDs/RFs are available, RX_NUM is used to switch between them
	switch(rx_tx_addr[3]%3)
	{
		case 0:
			memcpy(hopping_frequency,(uint8_t *)"\x34\x39\x43\x48",4);
			memcpy(rx_tx_addr,(uint8_t *)"\x47\x93\x00\x00\xD5",5);
			break;
		case 1:
			memcpy(hopping_frequency,(uint8_t *)"\x35\x39\x3B\x3D",4);
			memcpy(rx_tx_addr,(uint8_t *)"\xAC\xA1\x00\x00\xD5",5);
			break;
		case 2:
			memcpy(hopping_frequency,(uint8_t *)"\x32\x37\x41\x46",4);
			memcpy(rx_tx_addr,(uint8_t *)"\x92\x45\x01\x00\xD5",5);
			break;
	}
}

uint16_t initDM002(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = DM002_BIND_COUNT;
	DM002_initialize_txid();
	DM002_init();
	return	DM002_INITIAL_WAIT;
}

#endif
