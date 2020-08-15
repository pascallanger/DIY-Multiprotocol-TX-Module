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

#if defined(OMP_NRF24L01_INO)

#include "iface_nrf250k.h"

#define FORCE_OMP_ORIGINAL_ID

#define OMP_INITIAL_WAIT	500
#define OMP_PACKET_PERIOD	5000
#define OMP_RF_BIND_CHANNEL	35
#define OMP_RF_NUM_CHANNELS	8
#define OMP_PAYLOAD_SIZE	16
#define OMP_BIND_COUNT		600	//3sec

static void __attribute__((unused)) OMP_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(packet,"BND",3);
		memcpy(&packet[3],rx_tx_addr,5);
		memcpy(&packet[8],hopping_frequency,8);
	}
	else
	{
		memset(packet,0x00,OMP_PAYLOAD_SIZE);

		//hopping frequency
		packet[0 ] = hopping_frequency_no;	// |0x40 to request RX telemetry
		XN297L_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= OMP_RF_NUM_CHANNELS-1;	// 8 RF channels

		//flags
		packet[1 ] = 0x08								//unknown
				   | GET_FLAG(CH5_SW, 0x20);			// HOLD

		packet[2 ] = 0x40;								//unknown
		
		if(Channel_data[CH6] > CHANNEL_MAX_COMMAND)
			packet[2 ] |= 0x20;							// IDLE2
		else if(Channel_data[CH6] > CHANNEL_MIN_COMMAND)
			packet[1 ] |= 0x40;							// IDLE1

		if(Channel_data[CH7] > CHANNEL_MAX_COMMAND)
			packet[2 ] |= 0x08;							// 3D
		else if(Channel_data[CH7] > CHANNEL_MIN_COMMAND)
			packet[2 ] |= 0x04;							// ATTITUDE

		//trims??
		//packet[3..6]

		//channels TAER packed 11bits
		uint16_t channel=convert_channel_16b_limit(THROTTLE,0,2047);
		packet[7 ]  = channel;
		packet[8 ]  = channel>>8;
		channel=convert_channel_16b_limit(AILERON,0,2047);
		packet[8 ] |= channel<<3;
		packet[9 ]  = channel>>5;
		channel=convert_channel_16b_limit(ELEVATOR,0,2047);
		packet[10] |= channel<<6;
		packet[11]  = channel>>2;
		packet[12]  = channel>>10;
		channel=convert_channel_16b_limit(RUDDER,0,2047);
		packet[12] |= channel<<1;
		packet[13]  = channel>>8;

		//unknown
		//packet[13] = 0x00;
		//packet[14] = 0x00;
		packet[15] = 0x04;
	}

	XN297L_SetPower();		// Set tx_power
	XN297L_SetFreqOffset();	// Set frequency offset
	XN297L_WriteEnhancedPayload(packet, OMP_PAYLOAD_SIZE, IS_BIND_IN_PROGRESS);
}

static void __attribute__((unused)) OMP_init()
{
	XN297L_Init();
	XN297L_SetTXAddr((uint8_t*)"FLPBD", 5);
	XN297L_HoppingCalib(OMP_RF_NUM_CHANNELS);	// Calibrate all channels
	XN297L_RFChannel(OMP_RF_BIND_CHANNEL);		// Set bind channel
}

static void __attribute__((unused)) OMP_initialize_txid()
{
	calc_fh_channels(OMP_RF_NUM_CHANNELS);
	#ifdef FORCE_OMP_ORIGINAL_ID
		rx_tx_addr[0]=0x4E;
		rx_tx_addr[1]=0x72;
		rx_tx_addr[2]=0x8E;
		rx_tx_addr[3]=0x70;
		rx_tx_addr[4]=0x62;
		for(uint8_t i=0; i<OMP_RF_NUM_CHANNELS;i++)
			hopping_frequency[i]=(i+3)*5;
	#endif
}

uint16_t OMP_callback()
{
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
			BIND_DONE;
	OMP_send_packet();
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(OMP_PACKET_PERIOD);
	#endif
	return OMP_PACKET_PERIOD;
}

uint16_t initOMP()
{
	OMP_initialize_txid();
	OMP_init();
	hopping_frequency_no = 0;
	bind_counter=OMP_BIND_COUNT;
	return OMP_INITIAL_WAIT;
}

#endif
