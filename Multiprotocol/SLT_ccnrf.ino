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

#if defined(SLT_CCNRF_INO)

#include "iface_nrf250k.h"

//#define SLT_Q200_FORCE_ID
//#define SLT_V1_4_FORCE_ID

// For code readability
#define SLT_PAYLOADSIZE_V1		7
#define SLT_PAYLOADSIZE_V1_4	5
#define SLT_PAYLOADSIZE_V2		11
#define SLT_NFREQCHANNELS		15
#define SLT_TXID_SIZE			4
#define SLT_BIND_CHANNEL		0x50
#define SLT6_CH_MIN				182		// 10-bit AETR minimum (captures: 180-185, symmetric around 512)
#define SLT6_CH_MAX				842		// 10-bit AETR maximum (captures: 829-843, symmetric around 512)
#define SLT6_SW_THRESHOLD		273		// ~33% of half-range (820/3) for 3-position switch zones

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

// SLT6 sub-cycle states: 3 sub-cycles per triple, each with 2 copies
enum {
	SLT6_BUILD_A=0,		// Build packet, configure for 7B sub-cycle
	SLT6_DATA_A1,		// Send 7B copy 1
	SLT6_DATA_A2,		// Send 7B copy 2, configure for 6B sub-cycle
	SLT6_DATA_B1,		// Send 6B copy 1
	SLT6_DATA_B2,		// Send 6B copy 2, configure for 5B sub-cycle
	SLT6_DATA_C1,		// Send 5B copy 1
	SLT6_DATA_C2,		// Send 5B copy 2
	SLT6_BIND,			// Send bind packet
};

// SLT6 address XOR values for the 3 sub-cycles
#define SLT6_ADDR_XOR_A		0x00	// 7B sub-cycle: base address
#define SLT6_ADDR_XOR_B		0x06	// 6B sub-cycle: byte[0] XOR 0x06
#define SLT6_ADDR_XOR_C		0x09	// 5B sub-cycle: byte[0] XOR 0x09

// SLT6 timing (from capture 12b, in microseconds)
#define SLT6_TIMING_SUBCYCLE	5994	// ~6000us between sub-cycle starts
#define SLT6_TIMING_PAIR		1633	// ~1633us between the two copies within a sub-cycle
#define SLT6_TIMING_BUILD		1000	// Build+config time at start of each triple
#define SLT6_TIMING_TRIPLE		(3 * SLT6_TIMING_SUBCYCLE)	// ~18ms triple period

static void __attribute__((unused)) SLT_RF_init()
{
	NRF250K_Init(option == 0);	// SLT: option==0 uses NRF24L01, option!=0 uses CC2500 with freq tuning
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
	uint8_t max_freq = 0x50;	//V1 and V2
	if(sub_protocol == Q200)
		max_freq=45;
	for (uint8_t i = 0; i < SLT_NFREQCHANNELS; ++i)
	{
		if(sub_protocol == Q200 && hopping_frequency[i] >= max_freq)
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
	#ifdef DEBUG_SERIAL
		debug("CH:");
		for (uint8_t i = 0; i < SLT_NFREQCHANNELS; ++i)
			debug(" %02X(%d)", hopping_frequency[i], hopping_frequency[i]);
		debugln();
	#endif
	
	//Bind channel
	hopping_frequency[SLT_NFREQCHANNELS] = SLT_BIND_CHANNEL;
	
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
		uint16_t v = convert_channel_10b(sub_protocol != SLT_V1_4 ? CH_AETR[i] : i, false);
		if(sub_protocol>SLT_V2 && (i==CH2 || i==CH3) && sub_protocol != SLT_V1_4 && sub_protocol != RF_SIM)
			v=1023-v;	// reverse throttle and elevator channels for Q100/Q200/MR100 protocols
		packet[i] = v;
		e = (e >> 2) | (uint8_t) ((v >> 2) & 0xC0);
	}
	// Extra bits for AETR
	packet[4] = e;

	//->V1_4CH stops here

	// 8-bit channels
	packet[5] = convert_channel_8b(CH5);
	packet[6] = convert_channel_8b(CH6);

	if(sub_protocol == Q200)
		packet[6] =  GET_FLAG(CH9_SW , FLAG_Q200_FMODE)
					|GET_FLAG(CH10_SW, FLAG_Q200_FLIP)
					|GET_FLAG(CH11_SW, FLAG_Q200_VIDON)
					|GET_FLAG(CH12_SW, FLAG_Q200_VIDOFF);
	else if(sub_protocol == MR100 || sub_protocol == Q100)
		packet[6] =  GET_FLAG(CH9_SW , FLAG_MR100_FMODE)
					|GET_FLAG(CH10_SW, FLAG_MR100_FLIP)
					|GET_FLAG(CH11_SW, FLAG_MR100_VIDEO)	// Does not exist on the Q100 but...
					|GET_FLAG(CH12_SW, FLAG_MR100_PICTURE);	// Does not exist on the Q100 but...
	packet[7] = convert_channel_8b(CH7);
	packet[8] = convert_channel_8b(CH8);
	if(sub_protocol == RF_SIM)
	{
		packet[9]  = convert_channel_8b(CH9);
		packet[10] = convert_channel_8b(CH10);
	}
	else
	{
		packet[9]  = 0xAA;		//normal mode for Q100/Q200, unknown for V2/MR100
		packet[10] = 0x00;		//normal mode for Q100/Q200, unknown for V2/MR100
	}
	if((sub_protocol == Q100 || sub_protocol == Q200) && CH13_SW)
	{//Calibrate
		packet[9] = 0x77;			//enter calibration
		if(calib_counter >= 20 && calib_counter <= 25)	// 7 packets for Q100 / 3 packets for Q200
			packet[10] = 0x20;	//launch calibration
		calib_counter++;
		if(calib_counter > 250) calib_counter = 250;
	}
	else
		calib_counter = 0;
}

// SLT6: build 7-byte data packet from current channel values
static void __attribute__((unused)) SLT6_build_packet()
{
	// aileron, elevator, throttle, rudder (10-bit, limited range)
	uint8_t e = 0;
	for (uint8_t i = 0; i < 4; ++i)
	{
		uint16_t v = convert_channel_16b_limit(CH_AETR[i], SLT6_CH_MIN, SLT6_CH_MAX);
		packet[i] = v;
		e = (e >> 2) | (uint8_t) ((v >> 2) & 0xC0);
	}
	packet[4] = e;

	// Flight mode: 3 positions at ~33% each
	if(Channel_data[CH5] > CHANNEL_MID + SLT6_SW_THRESHOLD)
		packet[5] = 0xD0;
	else if(Channel_data[CH5] < CHANNEL_MID - SLT6_SW_THRESHOLD)
		packet[5] = 0x30;
	else
		packet[5] = 0x80;

	// Panic: only active when CH6 is below -33%, center and up = no panic
	if(Channel_data[CH6] < CHANNEL_MID - SLT6_SW_THRESHOLD)
		packet[6] = 0x30;
	else
		packet[6] = 0xD0;
}

// SLT6: configure radio for a sub-cycle (set address and channel)
static void __attribute__((unused)) SLT6_configure_radio(uint8_t addr_xor, uint8_t hop_offset)
{
	SLT_wait_radio();
	// Set TX address with XOR on byte[0]
	uint8_t addr[SLT_TXID_SIZE];
	memcpy(addr, rx_tx_addr, SLT_TXID_SIZE);
	addr[0] ^= addr_xor;
	NRF250K_SetTXAddr(addr, SLT_TXID_SIZE);
	// Set RF channel
	NRF250K_Hopping((hopping_frequency_no + hop_offset) % SLT_NFREQCHANNELS);
}

static void __attribute__((unused)) SLT_send_bind_packet()
{
	SLT_wait_radio();
	NRF250K_Hopping(SLT_NFREQCHANNELS);	//Bind channel
	BIND_IN_PROGRESS;					//Limit TX power to bind level
	NRF250K_SetPower();
	BIND_DONE;
	NRF250K_SetTXAddr((uint8_t *)"\x7E\xB8\x63\xA9", SLT_TXID_SIZE);
	memcpy((void*)packet, (void*)rx_tx_addr, SLT_TXID_SIZE);
	if(phase == SLT_BIND2 || phase == SLT6_BIND)
		SLT_send_packet(SLT_TXID_SIZE);
	else // SLT_BIND1
		SLT_send_packet(SLT_PAYLOADSIZE_V2);
}

#define SLT_TIMING_BUILD		1000
#define SLT_V1_TIMING_PACKET	1000
#define SLT_V1_4_TIMING_PACKET	1643
#define SLT_V2_TIMING_PACKET	2042
#define SLT_V1_TIMING_BIND2		1000
#define SLT_V2_TIMING_BIND1		6507
#define SLT_V2_TIMING_BIND2		2112

// SLT6 callback: triple-address state machine
// Each triple: 3 sub-cycles (7B, 6B, 5B), each sent twice, with different addresses and channels
static uint16_t __attribute__((unused)) SLT6_callback()
{
	switch (phase)
	{
		case SLT6_BUILD_A:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(SLT6_TIMING_TRIPLE);
			#endif
			SLT6_build_packet();
			NRF250K_SetPower();
			SLT6_configure_radio(SLT6_ADDR_XOR_A, 0);	// 7B sub-cycle: base address, hop+0
			phase = SLT6_DATA_A1;
			return SLT6_TIMING_BUILD;
		case SLT6_DATA_A1:
			SLT_send_packet(7);
			phase = SLT6_DATA_A2;
			return SLT6_TIMING_PAIR;					// 1633us between copies
		case SLT6_DATA_A2:
			SLT_send_packet(7);
			SLT6_configure_radio(SLT6_ADDR_XOR_B, 3);	// 6B sub-cycle: XOR 0x06 address, hop+3
			phase = SLT6_DATA_B1;
			return SLT6_TIMING_SUBCYCLE - SLT6_TIMING_PAIR;	// 4361us to next sub-cycle TX
		case SLT6_DATA_B1:
			SLT_send_packet(6);
			phase = SLT6_DATA_B2;
			return SLT6_TIMING_PAIR;
		case SLT6_DATA_B2:
			SLT_send_packet(6);
			SLT6_configure_radio(SLT6_ADDR_XOR_C, 6);	// 5B sub-cycle: XOR 0x09 address, hop+6
			phase = SLT6_DATA_C1;
			return SLT6_TIMING_SUBCYCLE - SLT6_TIMING_PAIR;	// 4361us
		case SLT6_DATA_C1:
			SLT_send_packet(5);
			phase = SLT6_DATA_C2;
			return SLT6_TIMING_PAIR;
		case SLT6_DATA_C2:
			SLT_send_packet(5);
			// Advance hopping for next triple
			if (++hopping_frequency_no >= SLT_NFREQCHANNELS)
				hopping_frequency_no = 0;
			if (++packet_count >= 100)
			{// Send bind packet periodically
				packet_count = 0;
				phase = SLT6_BIND;
				return SLT_V1_TIMING_BIND2;
			}
			phase = SLT6_BUILD_A;
			return SLT6_TIMING_SUBCYCLE - SLT6_TIMING_PAIR - SLT6_TIMING_BUILD;	// Gap before next build
		case SLT6_BIND:
			SLT_send_bind_packet();
			phase = SLT6_BUILD_A;
			return SLT6_TIMING_SUBCYCLE - SLT6_TIMING_PAIR - SLT6_TIMING_BUILD;
	}
	return SLT6_TIMING_TRIPLE;
}

uint16_t SLT_callback()
{
	// SLT6 has its own state machine
	if(sub_protocol == SLT6TX)
		return SLT6_callback();

	switch (phase)
	{
		case SLT_BUILD:
			//debugln_time("b ");
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(packet_period);
			#endif
			SLT_build_packet();
			NRF250K_SetPower();					//Change power level
			NRF250K_SetTXAddr(rx_tx_addr, SLT_TXID_SIZE);
			phase++;
			return SLT_TIMING_BUILD;
		case SLT_DATA1:
		case SLT_DATA2:
			phase++;
			SLT_send_packet(packet_length);
			if(sub_protocol == SLT_V1)
				return SLT_V1_TIMING_PACKET;
			if(sub_protocol == SLT_V1_4)
			{
				phase++;						//Packets are sent two times only
				return SLT_V1_4_TIMING_PACKET;
			}
			//V2
			return SLT_V2_TIMING_PACKET;
		case SLT_DATA3:
			SLT_send_packet(packet_length);
			if (++packet_count >= 100)
			{// Send bind packet
				packet_count = 0;
				if(sub_protocol == SLT_V1 || sub_protocol == SLT_V1_4)
				{
					phase = SLT_BIND2;
					return SLT_V1_TIMING_BIND2;
				}
				//V2
				phase = SLT_BIND1;
				return SLT_V2_TIMING_BIND1;
			}
			else
			{// Continue to send normal packets
				phase = SLT_BUILD;
				if(sub_protocol == SLT_V1)
					return 20000 - SLT_TIMING_BUILD;
				if(sub_protocol==SLT_V1_4)
					return 18000 - SLT_TIMING_BUILD - SLT_V1_4_TIMING_PACKET;
				//V2
				return 13730 - SLT_TIMING_BUILD;
			}
		case SLT_BIND1:
			SLT_send_bind_packet();
			phase++;
			return SLT_V2_TIMING_BIND2;
		case SLT_BIND2:
			SLT_send_bind_packet();
			phase = SLT_BUILD;
			if(sub_protocol == SLT_V1)
				return 20000 - SLT_TIMING_BUILD - SLT_V1_TIMING_BIND2;
			if(sub_protocol == SLT_V1_4)
				return 18000 - SLT_TIMING_BUILD - SLT_V1_TIMING_BIND2 - SLT_V1_4_TIMING_PACKET;
			//V2
			return 13730 - SLT_TIMING_BUILD - SLT_V2_TIMING_BIND1 - SLT_V2_TIMING_BIND2;
	}
	return 19000;
}

void SLT_init()
{
	BIND_DONE;	// Not a TX bind protocol
	packet_count = 0;
	packet_sent = 0;
	hopping_frequency_no = 0;

	if(sub_protocol == SLT6TX)
	{
		hopping_frequency_no = 1;	// SLT6 starts hopping at index 1 (verified from captures)
		// packet_length not used for SLT6 (lengths vary per sub-cycle)
		#ifdef MULTI_SYNC
			packet_period = SLT6_TIMING_TRIPLE;
		#endif
	}
	else if(sub_protocol == SLT_V1)
	{
		packet_length = SLT_PAYLOADSIZE_V1;
		#ifdef MULTI_SYNC
			packet_period = 20000+2*SLT_V1_TIMING_PACKET;		//22ms
		#endif
	}
	else if(sub_protocol == SLT_V1_4)
	{
		packet_length = SLT_PAYLOADSIZE_V1_4;
		#ifdef MULTI_SYNC
			packet_period = 18000;								//18ms
		#endif
		 //Force high part of the ID otherwise the RF frequencies do not match, only tested the 2 last bytes...
		rx_tx_addr[0]=0xF4;
		rx_tx_addr[1]=0x71;
		#ifdef SLT_V1_4_FORCE_ID	// ID taken from TX dumps
			memcpy(rx_tx_addr,"\xF4\x71\x8D\x01",SLT_TXID_SIZE);
		#endif
	}
	else //V2
	{
		packet_length = SLT_PAYLOADSIZE_V2;
		#ifdef MULTI_SYNC
			packet_period = 13730+2*SLT_V2_TIMING_PACKET;		//~18ms
		#endif
	}

	if(sub_protocol == Q200)
	{ //Q200: Force high part of the ID otherwise it won't bind
		rx_tx_addr[0]=0x01;
		rx_tx_addr[1]=0x02;
		#ifdef SLT_Q200_FORCE_ID	// ID taken from TX dumps
			rx_tx_addr[0]=0x01;rx_tx_addr[1]=0x02;rx_tx_addr[2]=0x6A;rx_tx_addr[3]=0x31;
		/*	rx_tx_addr[0]=0x01;rx_tx_addr[1]=0x02;rx_tx_addr[2]=0x0B;rx_tx_addr[3]=0x57;*/
		#endif
	}

	SLT_RF_init();
	SLT_set_freq();

	if(sub_protocol == SLT6TX)
		phase = SLT6_BUILD_A;
	else
		phase = SLT_BUILD;
}

#endif
//SLT v1_4ch timing
//268363 + 1643 / 15 = 18000
