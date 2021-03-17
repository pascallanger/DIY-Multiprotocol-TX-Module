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

#include "iface_xn297.h"

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

static void __attribute__((unused)) DM002_send_packet()
{
	memcpy(packet+5,(uint8_t *)"\x00\x7F\x7F\x7F\x00\x00\x00",7);
	if(IS_BIND_IN_PROGRESS)
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
		XN297_Hopping(hopping_frequency_no);
	}
	//CRC
	for(uint8_t i=0;i<DM002_PACKET_SIZE-1;i++)
		packet[11]+=packet[i];
	
	//Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, DM002_PACKET_SIZE);
}

static void __attribute__((unused)) DM002_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t *)"\x26\xA8\x67\x35\xCC", 5);
	XN297_RFChannel(DM002_RF_BIND_CHANNEL);
}

uint16_t DM002_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(DM002_PACKET_PERIOD);
	#endif
	if (bind_counter)
	{
		bind_counter--;
		if (bind_counter == 0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 5);
		}
	}
	DM002_send_packet();
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

void DM002_init(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = DM002_BIND_COUNT;
	DM002_initialize_txid();
	DM002_RF_init();
}

#endif
