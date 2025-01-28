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
#if defined(H36_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_H36_ORIGINAL_ID

#define H36_PAYLOAD_SIZE		13
#define H36_RF_NUM_CHANNELS		4
#define H36_BIND_PACKET_PERIOD	10285
#define H36_BIND_COUNT			648		//3sec

enum {
	H36_DATA1=0,
	H36_DATA2,
	H36_DATA3,
	H36_DATA4,
};

static void __attribute__((unused)) H36_send_packet()
{
	if(IS_BIND_DONE && phase == H36_DATA1)
	{
		hopping_frequency_no++;
		hopping_frequency_no&=3;
		XN297_Hopping(hopping_frequency_no);
	}

	packet[0] = 0x2E; 							// constant?
	memcpy(&packet[2],rx_tx_addr,3);
	if(IS_BIND_IN_PROGRESS)
	{//Bind
		memcpy(&packet[5],hopping_frequency,4);
		memset(&packet[9], 0x00, 3);
		packet[12] = 0xED; 						// constant?
		bind_counter--;
		if(bind_counter == 0)
			BIND_DONE;
	}
	else
	{//Normal
		packet[5] = convert_channel_8b(THROTTLE);
		packet[6] = convert_channel_8b(RUDDER);
		packet[7] = convert_channel_8b(ELEVATOR);
		packet[8] = convert_channel_8b(AILERON);
		packet[9] = GET_FLAG(CH6_SW,  0x02)		//Headless
				   |GET_FLAG(CH7_SW,  0x04);	//RTH(temporary)
		packet[10] = 0x20; 						//Trim A centered(0x20)
		packet[11] = CH5_SW?0x60:0x20;			//Flip(0x40)|Trim E centered(0x20)
		packet[12] = 0xA0; 						//High(0x80)/Low(0x40) rates|Trim R centered(0x20)?
	}
	//crc
	packet[1]=0xAA;
	for(uint8_t i=5;i<12;i++)
		packet[1] ^= packet[i];
	//Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, H36_PAYLOAD_SIZE);
	#ifdef DEBUG_SERIAL
		debug("H%d P",hopping_frequency_no);
		for(uint8_t i=0; i < H36_PAYLOAD_SIZE; i++)
			debug(" %02X", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) H36_initialize_txid()
{
	rx_tx_addr[0] = rx_tx_addr[3];
	calc_fh_channels(4);
	#ifdef FORCE_H36_ORIGINAL_ID
		if(!RX_num)
		{
			memcpy(rx_tx_addr,(uint8_t *)"\x00\x11\x00",3);
			memcpy(hopping_frequency,(uint8_t *)"\x36\x3A\x31\x2B",4);	//54, 58, 49, 43 
		}
	#endif
}

static void __attribute__((unused)) H36_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\xCC\x6C\x47\x90\x53", 5);
	XN297_RFChannel(50);		//Bind channel
}

uint16_t H36_callback()
{
	H36_send_packet();
	switch(phase)
	{
		case H36_DATA1:
			phase++;
			return 1830;
		case H36_DATA2:
		case H36_DATA3:
			phase++;
			return 3085;
		default://DATA4
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(18500);
			#endif
			phase = H36_DATA1;
			break;
	}
	return 10500;
}

void H36_init()
{
	BIND_IN_PROGRESS;	// Autobind protocol
	H36_initialize_txid();
	H36_RF_init();
	phase = H36_DATA1;
	hopping_frequency_no = 0;
	bind_counter = H36_BIND_COUNT;
}
#endif
