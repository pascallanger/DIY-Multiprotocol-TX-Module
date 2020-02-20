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
// Last sync with deviation main github branch

#if defined(SLT_NRF24L01_INO)

#include "iface_nrf250k.h"

//#define SLT_Q200_FORCE_ID

// For code readability
#define SLT_PAYLOADSIZE_V1 7
#define SLT_PAYLOADSIZE_V2 11
#define SLT_NFREQCHANNELS 15
#define SLT_TXID_SIZE 4
#define SLT_BIND_CHANNEL 0x50

enum{
	// flags going to packet[6] (Q200)
	FLAG_Q200_FMODE	= 0x20,
	FLAG_Q200_VIDON	= 0x10,
	FLAG_Q200_FLIP	= 0x08,
	FLAG_Q200_VIDOFF= 0x04,
};

enum{
	// flags going to packet[6] (MR100 & Q100)
	FLAG_MR100_FMODE	= 0x20,
	FLAG_MR100_FLIP		= 0x04,
	FLAG_MR100_VIDEO	= 0x02,
	FLAG_MR100_PICTURE	= 0x01,
};

enum {
	SLT_BUILD=0,
	SLT_DATA1,
	SLT_DATA2,
	SLT_DATA3,
	SLT_BIND1,
	SLT_BIND2,
};

static void __attribute__((unused)) SLT_init()
{
	NRF250K_Init();
	NRF250K_SetTXAddr(rx_tx_addr, SLT_TXID_SIZE);
}

static void __attribute__((unused)) SLT_set_freq(void)
{
	// Frequency hopping sequence generation
	for (uint8_t i = 0; i < SLT_TXID_SIZE; ++i)
	{
		uint8_t next_i = (i+1) % SLT_TXID_SIZE; // is & 3 better than % 4 ?
		uint8_t base = i < 2 ? 0x03 : 0x10;
		hopping_frequency[i*4 + 0]  = (rx_tx_addr[i] & 0x3f) + base;
		hopping_frequency[i*4 + 1]  = (rx_tx_addr[i] >> 2) + base;
		hopping_frequency[i*4 + 2]  = (rx_tx_addr[i] >> 4) + (rx_tx_addr[next_i] & 0x03)*0x10 + base;
		hopping_frequency[i*4 + 3]  = (rx_tx_addr[i] >> 6) + (rx_tx_addr[next_i] & 0x0f)*0x04 + base;
	}

	// Unique freq
	uint8_t max_freq=0x50;	//V1 and V2
	if(sub_protocol==Q200)
		max_freq=45;
	for (uint8_t i = 0; i < SLT_NFREQCHANNELS; ++i)
	{
		if(sub_protocol==Q200 && hopping_frequency[i] >= max_freq)
			hopping_frequency[i] = hopping_frequency[i] - max_freq + 0x03;
		uint8_t done = 0;
		while (!done)
		{
			done = 1;
			for (uint8_t j = 0; j < i; ++j)
				if (hopping_frequency[i] == hopping_frequency[j])
				{
					done = 0;
					hopping_frequency[i] += 7;
					if (hopping_frequency[i] >= max_freq)
						hopping_frequency[i] = hopping_frequency[i] - max_freq + 0x03;
				}
		}
	}
	
	//Bind channel
	hopping_frequency[SLT_NFREQCHANNELS]=SLT_BIND_CHANNEL;
	
	//Calib all channels
	NRF250K_HoppingCalib(SLT_NFREQCHANNELS+1);
}

static void __attribute__((unused)) SLT_wait_radio()
{
	if (packet_sent)
		while (!NRF250K_IsPacketSent());
	packet_sent = 0;
}

static void __attribute__((unused)) SLT_send_packet(uint8_t len)
{
	SLT_wait_radio();
	NRF250K_WritePayload(packet, len);
	packet_sent = 1;
}

static void __attribute__((unused)) SLT_build_packet()
{
	static uint8_t calib_counter=0;
	
	// Set radio channel - once per packet batch
	NRF250K_SetFreqOffset();	// Set frequency offset
	NRF250K_Hopping(hopping_frequency_no);
	if (++hopping_frequency_no >= SLT_NFREQCHANNELS)
		hopping_frequency_no = 0;

	// aileron, elevator, throttle, rudder, gear, pitch
	uint8_t e = 0; // byte where extension 2 bits for every 10-bit channel are packed
	for (uint8_t i = 0; i < 4; ++i)
	{
		uint16_t v = convert_channel_10b(CH_AETR[i]);
		if(sub_protocol>SLT_V2 && (i==CH2 || i==CH3) )
			v=1023-v;	// reverse throttle and elevator channels for Q100/Q200/MR100 protocols
		packet[i] = v;
		e = (e >> 2) | (uint8_t) ((v >> 2) & 0xC0);
	}
	// Extra bits for AETR
	packet[4] = e;
	// 8-bit channels
	packet[5] = convert_channel_8b(CH5);
	packet[6] = convert_channel_8b(CH6);
	if(sub_protocol!=SLT_V1)
	{
		if(sub_protocol==Q200)
			packet[6] =  GET_FLAG(CH9_SW , FLAG_Q200_FMODE)
						|GET_FLAG(CH10_SW, FLAG_Q200_FLIP)
						|GET_FLAG(CH11_SW, FLAG_Q200_VIDON)
						|GET_FLAG(CH12_SW, FLAG_Q200_VIDOFF);
		else if(sub_protocol==MR100 || sub_protocol==Q100)
			packet[6] =  GET_FLAG(CH9_SW , FLAG_MR100_FMODE)
						|GET_FLAG(CH10_SW, FLAG_MR100_FLIP)
						|GET_FLAG(CH11_SW, FLAG_MR100_VIDEO)	// Does not exist on the Q100 but...
						|GET_FLAG(CH12_SW, FLAG_MR100_PICTURE);	// Does not exist on the Q100 but...
		packet[7]=convert_channel_8b(CH7);
		packet[8]=convert_channel_8b(CH8);
		packet[9]=0xAA;				//normal mode for Q100/Q200, unknown for V2/MR100
		packet[10]=0x00;			//normal mode for Q100/Q200, unknown for V2/MR100
		if((sub_protocol==Q100 || sub_protocol==Q200) && CH13_SW)
		{//Calibrate
			packet[9]=0x77;			//enter calibration
			if(calib_counter>=20 && calib_counter<=25)	// 7 packets for Q100 / 3 packets for Q200
				packet[10]=0x20;	//launch calibration
			calib_counter++;
			if(calib_counter>250) calib_counter=250;
		}
		else
			calib_counter=0;
	}
}

static void __attribute__((unused)) SLT_send_bind_packet()
{
	SLT_wait_radio();
	NRF250K_Hopping(SLT_NFREQCHANNELS);	//Bind channel
	BIND_IN_PROGRESS;					//Limit TX power to bind level
	NRF250K_SetPower();
	BIND_DONE;
	NRF250K_SetTXAddr((uint8_t *)"\x7E\xB8\x63\xA9", SLT_TXID_SIZE);
	memcpy((void*)packet,(void*)rx_tx_addr,SLT_TXID_SIZE);
	if(phase==SLT_BIND2)
		SLT_send_packet(SLT_TXID_SIZE);
	else // SLT_BIND1
		SLT_send_packet(SLT_PAYLOADSIZE_V2);
}

#define SLT_TIMING_BUILD		1000
#define SLT_V1_TIMING_PACKET	1000
#define SLT_V2_TIMING_PACKET	2042
#define SLT_V1_TIMING_BIND2		1000
#define SLT_V2_TIMING_BIND1		6507
#define SLT_V2_TIMING_BIND2		2112
uint16_t SLT_callback()
{
	switch (phase)
	{
		case SLT_BUILD:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(sub_protocol==SLT_V1?20000:13730);
			#endif
			SLT_build_packet();
			NRF250K_SetPower();					//Change power level
			NRF250K_SetTXAddr(rx_tx_addr, SLT_TXID_SIZE);
			phase++;
			return SLT_TIMING_BUILD;
		case SLT_DATA1:
		case SLT_DATA2:
			phase++;
			if(sub_protocol==SLT_V1)
			{
				SLT_send_packet(SLT_PAYLOADSIZE_V1);
				return SLT_V1_TIMING_PACKET;
			}
			else //V2
			{
				SLT_send_packet(SLT_PAYLOADSIZE_V2);
				return SLT_V2_TIMING_PACKET;
			}
		case SLT_DATA3:
			if(sub_protocol==SLT_V1)
				SLT_send_packet(SLT_PAYLOADSIZE_V1);
			else //V2
				SLT_send_packet(SLT_PAYLOADSIZE_V2);
			if (++packet_count >= 100)
			{// Send bind packet
				packet_count = 0;
				if(sub_protocol==SLT_V1)
				{
					phase=SLT_BIND2;
					return SLT_V1_TIMING_BIND2;
				}
				else //V2
				{
					phase=SLT_BIND1;
					return SLT_V2_TIMING_BIND1;
				}
			}
			else
			{// Continue to send normal packets
				phase = SLT_BUILD;
				if(sub_protocol==SLT_V1)
					return 20000-SLT_TIMING_BUILD;
				else //V2
					return 13730-SLT_TIMING_BUILD;
			}
		case SLT_BIND1:
			SLT_send_bind_packet();
			phase++;
			return SLT_V2_TIMING_BIND2;
		case SLT_BIND2:
			SLT_send_bind_packet();
			phase = SLT_BUILD;
			if(sub_protocol==SLT_V1)
				return 20000-SLT_TIMING_BUILD-SLT_V1_TIMING_BIND2;
			else //V2
				return 13730-SLT_TIMING_BUILD-SLT_V2_TIMING_BIND1-SLT_V2_TIMING_BIND2;
	}
	return 19000;
}

uint16_t initSLT()
{
	packet_count = 0;
	packet_sent = 0;
	hopping_frequency_no = 0;
	if(sub_protocol==Q200)
	{ //Q200: Force high part of the ID otherwise it won't bind
		rx_tx_addr[0]=0x01;
		rx_tx_addr[1]=0x02;
		#ifdef SLT_Q200_FORCE_ID	// ID taken from TX dumps
			rx_tx_addr[0]=0x01;rx_tx_addr[1]=0x02;rx_tx_addr[2]=0x6A;rx_tx_addr[3]=0x31;
		/*	rx_tx_addr[0]=0x01;rx_tx_addr[1]=0x02;rx_tx_addr[2]=0x0B;rx_tx_addr[3]=0x57;*/
		#endif
	}
	SLT_init();
	SLT_set_freq();
	phase = SLT_BUILD;
	return 50000;
}

#endif
