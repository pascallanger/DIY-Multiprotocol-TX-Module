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

#include "iface_nrf250k.h"

//#define FORCE_KF606_ORIGINAL_ID

#define KF606_INITIAL_WAIT    500
#define KF606_PACKET_PERIOD   3000
#define KF606_RF_BIND_CHANNEL 7
#define KF606_PAYLOAD_SIZE    4
#define KF606_BIND_COUNT	  857	//3sec
#define KF606_RF_NUM_CHANNELS 2

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
		// Deadband is needed on aileron, 40 gives +-6%
		packet[2]=convert_channel_8b_limit_deadband(AILERON,0x20,0x80,0xE0,40);	// Aileron: Max values:20..80..E0, Low rates:50..80..AF, High rates:3E..80..C1
		// Aileron trim must be on a separated channel C1..D0..DF
		packet[3]= convert_channel_16b_limit(CH5,0xC1,0xDF);
	}
	if(IS_BIND_DONE)
	{
		XN297L_Hopping(hopping_frequency_no);
		hopping_frequency_no ^= 1;			// 2 RF channels
	}

	XN297L_WritePayload(packet, KF606_PAYLOAD_SIZE);

	XN297L_SetPower();		// Set tx_power
	XN297L_SetFreqOffset();	// Set frequency offset
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

static void __attribute__((unused)) KF606_init()
{
	XN297L_Init();
	XN297L_SetTXAddr((uint8_t*)"\xe7\xe7\xe7\xe7\xe7", 5);
	XN297L_HoppingCalib(KF606_RF_NUM_CHANNELS);	// Calibrate all channels
	XN297L_RFChannel(KF606_RF_BIND_CHANNEL);	// Set bind channel
}

uint16_t KF606_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(KF606_PACKET_PERIOD);
	#endif
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
