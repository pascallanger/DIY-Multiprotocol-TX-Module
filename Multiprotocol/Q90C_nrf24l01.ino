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
// Compatible with Q90C quad.

#if defined(Q90C_NRF24L01_INO)

#include "iface_nrf250k.h"

//#define FORCE_Q90C_ORIGINAL_ID

#define Q90C_BIND_COUNT			250
#define Q90C_PACKET_PERIOD		7336 // 6200 on saimat's TX...
#define Q90C_INITIAL_WAIT		500
#define Q90C_PACKET_SIZE		12
#define Q90C_RF_BIND_CHANNEL	0x33
#define Q90C_RF_NUM_CHANNELS	3
#define Q90C_ADDRESS_LENGTH		5

bool Q90C_VTX;

int16_t Q90C_channel(uint8_t num, int16_t in_min,int16_t in_max, int16_t out_min,int16_t out_max)
{
	int32_t val=Channel_data[num];
	if(val<in_min) val=in_min;
	else if(val>in_max) val=in_max;
	val=(val-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;
	return (uint16_t)val;
}

static void __attribute__((unused)) Q90C_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(packet, rx_tx_addr, 4);
		memcpy(&packet[4], hopping_frequency, 3);
		//packet[7]  = 0x1e;	// 2e on Saimat 1???
		packet[10] = 0x4B;
		packet[11] = 0x4E;
	}
	else
	{
		XN297L_Hopping(hopping_frequency_no++);				// RF Freq
		hopping_frequency_no %= Q90C_RF_NUM_CHANNELS;
		packet[0]= convert_channel_8b(THROTTLE);			// 0..255
		// A,E,R have weird scaling, 0x00-0xff range (unsigned) but center isn't 7f or 80
		// rudder ff-7a-00
		if (Channel_data[RUDDER] <= CHANNEL_MID)
			packet[1] = Q90C_channel(RUDDER,   CHANNEL_MIN_100, CHANNEL_MID, 0xff, 0x7a );
		else
			packet[1] = Q90C_channel(RUDDER,   CHANNEL_MID, CHANNEL_MAX_100, 0x7a, 0x00 );
		// elevator 00-88-ff
		if (Channel_data[ELEVATOR] <= CHANNEL_MID)
			packet[2] = Q90C_channel(ELEVATOR, CHANNEL_MIN_100, CHANNEL_MID, 0x00, 0x88);
		else
			packet[2] = Q90C_channel(ELEVATOR, CHANNEL_MID, CHANNEL_MAX_100, 0x88, 0xff);
		// aileron ff-88-00
		if (Channel_data[AILERON] <= CHANNEL_MID)
			packet[3] = Q90C_channel(AILERON,  CHANNEL_MIN_100, CHANNEL_MID, 0xff, 0x88);
		else
			packet[3] = Q90C_channel(AILERON,  CHANNEL_MID, CHANNEL_MAX_100, 0x88, 0x00);
		// required to "arm" (low throttle + aileron to the right)
		if (packet[0] < 5 && packet[3] < 25) {
			packet[1] = 0x7a;
			packet[2] = 0x88;
		}
		packet[4] = 0x1e;									// T trim 00-1e-3c
		packet[5] = 0x1e;									// R trim 3c-1e-00
		packet[6] = 0x1e;									// E trim 00-1e-3c
		//packet[7] = 0x1e;									// A trim 00-1e-3c
		packet[8] |= 0x02;									// Rudder rate 0=min,1,2=max
		if(state!=Channel_data[CH5])
		{
			state=Channel_data[CH5];
			if(state<CHANNEL_MIN_COMMAND)
				packet[8] ^= 0x04;							// Angle
			else if(state>CHANNEL_MAX_COMMAND)
				packet[8] ^= 0x10;							// Acro
			else
				packet[8] ^= 0x08;							// Horizon
		}
		if(!Q90C_VTX && CH6_SW)
			packet[8] ^= 0x20;								// VTX+
		Q90C_VTX=CH6_SW;

		debugln("8=%02X",packet[8]);
		packet[10] = packet_count++;
	}
	packet[7]  = 0x1e;	// bind 1e or 2e, normal: A trim 00-1e-3c

	// checksum
	if(IS_BIND_DONE)
	{
	    uint8_t sum=0;
		for (uint8_t i = 0; i < Q90C_PACKET_SIZE - 1; i++)
			sum += packet[i];
		packet[11] = sum ^ crc8;
	}

	XN297L_SetFreqOffset();									// Set frequency offset
	XN297L_SetPower();										// Set tx_power
	XN297L_WriteEnhancedPayload(packet, Q90C_PACKET_SIZE, 0);
}

static void __attribute__((unused)) Q90C_initialize_txid()
{
	calc_fh_channels(Q90C_RF_NUM_CHANNELS);
	rx_tx_addr[4]=0x4B;
	#ifdef FORCE_Q90C_ORIGINAL_ID
		//24 03 01 82 18 26 37 1E 00 00 4B 4E
		memcpy(rx_tx_addr, (uint8_t*)"\x24\x03\x01\x82\x4B", Q90C_ADDRESS_LENGTH);	//Goebish
		memcpy(hopping_frequency, (uint8_t*)"\x18\x26\x37", Q90C_RF_NUM_CHANNELS);
		//4C 0A 02 01 17 24 36 2E 00 00 4B 4E 
		memcpy(rx_tx_addr, (uint8_t*)"\x4C\x0A\x02\x01\x4B", Q90C_ADDRESS_LENGTH);	//Saimat 1
		memcpy(hopping_frequency, (uint8_t*)"\x17\x24\x36", Q90C_RF_NUM_CHANNELS);
		//34 13 02 01 18 26 37 1E 00 00 4B 4E 
		memcpy(rx_tx_addr, (uint8_t*)"\x34\x13\x02\x01\x4B", Q90C_ADDRESS_LENGTH);	//Saimat 2
		memcpy(hopping_frequency, (uint8_t*)"\x18\x26\x37", Q90C_RF_NUM_CHANNELS);
	#endif
	crc8=rx_tx_addr[0]^rx_tx_addr[1]^rx_tx_addr[2]^rx_tx_addr[3];
}

static void __attribute__((unused)) Q90C_init()
{
	XN297L_Init();
	if(IS_BIND_IN_PROGRESS)
		XN297L_SetTXAddr((uint8_t*)"\x4F\x43\x54\x81\x81", Q90C_ADDRESS_LENGTH);
	else
		XN297L_SetTXAddr(rx_tx_addr, Q90C_ADDRESS_LENGTH);
	XN297L_HoppingCalib(Q90C_RF_NUM_CHANNELS);				// Calibrate all channels
	XN297L_RFChannel(Q90C_RF_BIND_CHANNEL);					// Set bind channel
}

uint16_t Q90C_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(Q90C_PACKET_PERIOD);
	#endif
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297L_SetTXAddr(rx_tx_addr, Q90C_ADDRESS_LENGTH);
		}
	Q90C_send_packet();
	return Q90C_PACKET_PERIOD;
}

uint16_t initQ90C()
{
	Q90C_initialize_txid();
	Q90C_init();
	hopping_frequency_no = 0;
	packet_count = 0;
	bind_counter=Q90C_BIND_COUNT;

	//features
	state=Channel_data[CH5];
	Q90C_VTX=CH6_SW;
	packet[8]  = 0x00;
	packet[9]  = 0x00;
	return Q90C_INITIAL_WAIT;
}

#endif
