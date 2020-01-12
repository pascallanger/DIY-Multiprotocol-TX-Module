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
// compatible with V911S

#if defined(V911S_NRF24L01_INO)

#include "iface_xn297l.h"

//#define V911S_ORIGINAL_ID

#define V911S_PACKET_PERIOD			5000
#define V911S_BIND_PACKET_PERIOD	3300
#define V911S_INITIAL_WAIT			500
#define V911S_PACKET_SIZE			16
#define V911S_RF_BIND_CHANNEL		35
#define V911S_NUM_RF_CHANNELS		8
#define V911S_BIND_COUNT			200

// flags going to packet[1]
#define	V911S_FLAG_EXPERT	0x04
// flags going to packet[2]
#define	V911S_FLAG_CALIB	0x01

static void __attribute__((unused)) V911S_send_packet(uint8_t bind)
{
	if(bind)
	{
		packet[0] = 0x42;
		packet[1] = 0x4E;
		packet[2] = 0x44;
		for(uint8_t i=0;i<5;i++)
			packet[i+3] = rx_tx_addr[i];
		for(uint8_t i=0;i<8;i++)
			packet[i+8] = hopping_frequency[i];
	}
	else
	{
		uint8_t channel=hopping_frequency_no;
		if(rf_ch_num&1)
		{
			if((hopping_frequency_no&1)==0)
				channel+=8;
			channel>>=1;
		}
		if(rf_ch_num&2)
			channel=7-channel;
		packet[ 0]=(rf_ch_num<<3)|channel;
		packet[ 1]=V911S_FLAG_EXPERT;					// short press on left button
		packet[ 2]=GET_FLAG(CH5_SW,V911S_FLAG_CALIB);	// long  press on right button
		memset(packet+3, 0x00, V911S_PACKET_SIZE - 3);
		//packet[3..6]=trims TAER signed
		uint16_t ch=convert_channel_16b_limit(THROTTLE ,0,0x7FF);
		packet[ 7] = ch;
		packet[ 8] = ch>>8;
		ch=convert_channel_16b_limit(AILERON ,0x7FF,0);
		packet[ 8]|= ch<<3;
		packet[ 9] = ch>>5;
		ch=convert_channel_16b_limit(ELEVATOR,0,0x7FF);
		packet[10] = ch;
		packet[11] = ch>>8;
		ch=convert_channel_16b_limit(RUDDER  ,0x7FF,0);
		packet[11]|= ch<<3;
		packet[12] = ch>>5;
	}
	
	if (!bind)
	{
		XN297L_Hopping(channel);
		hopping_frequency_no++;
		hopping_frequency_no&=7;	// 8 RF channels
	}

	if(sub_protocol==V911S_STD)
		XN297L_WritePayload(packet, V911S_PACKET_SIZE);
	else
		XN297L_WriteEnhancedPayload(packet, V911S_PACKET_SIZE, bind);

	XN297L_SetPower();				// Set tx_power
	XN297L_SetFreqOffset();			// Set frequency offset
}

static void __attribute__((unused)) V911S_init()
{
	XN297L_Init();
	if(sub_protocol==V911S_STD)
		XN297L_SetTXAddr((uint8_t *)"KNBND",5);			// V911S Bind address
	else
		XN297L_SetTXAddr((uint8_t *)"XPBND",5);			// E119 Bind address
	XN297L_HoppingCalib(V911S_NUM_RF_CHANNELS);		// Calibrate all channels
	XN297L_RFChannel(V911S_RF_BIND_CHANNEL);		// Set bind channel
}

static void __attribute__((unused)) V911S_initialize_txid()
{
	//channels
	uint8_t offset=rx_tx_addr[3]%5;				// 0-4
	for(uint8_t i=0;i<V911S_NUM_RF_CHANNELS;i++)
		hopping_frequency[i]=0x10+i*5+offset;
	if(!offset) hopping_frequency[0]++;
	
	// channels order
	rf_ch_num=random(0xfefefefe)&0x03;			// 0-3
}

uint16_t V911S_callback()
{
	if(IS_BIND_DONE)
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(V911S_PACKET_PERIOD);
		#endif
		V911S_send_packet(0);
	}
	else
	{
		if (bind_counter == 0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 5);
			packet_period=V911S_PACKET_PERIOD;
		}
		else
		{
			V911S_send_packet(1);
			bind_counter--;
			if(bind_counter==100)		// same as original TX...
				packet_period=V911S_BIND_PACKET_PERIOD*3;
		}
	}
	return	packet_period;
}

uint16_t initV911S(void)
{
	V911S_initialize_txid();
	#ifdef V911S_ORIGINAL_ID
		if(sub_protocol==V911S_STD)
		{//V911S
			rx_tx_addr[0]=0xA5;
			rx_tx_addr[1]=0xFF;
			rx_tx_addr[2]=0x70;
			rx_tx_addr[3]=0x8D;
			rx_tx_addr[4]=0x76;
			for(uint8_t i=0;i<V911S_NUM_RF_CHANNELS;i++)
				hopping_frequency[i]=0x10+i*5;
			hopping_frequency[0]++;
			rf_ch_num=0;
		}
		else
		{
			//E119
			rx_tx_addr[0]=0x30;
			rx_tx_addr[1]=0xFF;
			rx_tx_addr[2]=0xD1;
			rx_tx_addr[3]=0x2C;
			rx_tx_addr[4]=0x2A;
			for(uint8_t i=0;i<V911S_NUM_RF_CHANNELS;i++)
				hopping_frequency[i]=0x0E + i*5;
			rf_ch_num=0;
		}
	#endif

	V911S_init();

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = V911S_BIND_COUNT;
		packet_period= V911S_BIND_PACKET_PERIOD;
	}
	else
	{
		XN297_SetTXAddr(rx_tx_addr, 5);
		packet_period= V911S_PACKET_PERIOD;
	}
	hopping_frequency_no=0;
	return	V911S_INITIAL_WAIT;
}

#endif
