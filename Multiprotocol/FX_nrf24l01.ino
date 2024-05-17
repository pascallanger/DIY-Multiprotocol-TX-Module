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
// Compatible with FEI XIONG P38 plane.

#if defined(FX_NRF24L01_INO)

#include "iface_xn297.h"

#define FX_BIND_COUNT				300		//3sec
#define FX_SWITCH					20
#define FX_NUM_CHANNELS				4

#define FX816_PACKET_PERIOD			10000
#define FX816_BIND_CHANNEL			40
#define FX816_PAYLOAD_SIZE			6
#define FX816_CH_OFFSET				3

#define FX620_PACKET_PERIOD 		3250
#define FX620_BIND_PACKET_PERIOD	4500
#define FX620_BIND_CHANNEL			18
#define FX620_PAYLOAD_SIZE			7
#define FX620_CH_OFFSET				1

#define FX9630_PACKET_PERIOD		8124	//8156 on QIDI-560
#define FX9630_BIND_PACKET_PERIOD	8124
#define FX9630_BIND_CHANNEL			51
#define FX9630_PAYLOAD_SIZE			8
#define FX9630_NUM_CHANNELS			3

//#define FORCE_FX620_ID
//#define FORCE_FX9630_ID
//#define FORCE_QIDI_ID

static void __attribute__((unused)) FX_send_packet()
{
	static uint8_t trim_ch = 0;

	//Hopp
	if(IS_BIND_DONE)
	{
		XN297_Hopping(hopping_frequency_no++);
		if(sub_protocol >= FX9630)
		{ // FX9630 & FX_Q560
			XN297_SetTXAddr(rx_tx_addr, 4);
			if (hopping_frequency_no >= FX9630_NUM_CHANNELS)
			{
				hopping_frequency_no = 0;
				if(sub_protocol == FX9630)
				{
					trim_ch++;
					if(trim_ch > 3) trim_ch = 0;
				}
				else // FX_Q560
					trim_ch = 0;
			}
		}
		else // FX816 and FX620
		{
			hopping_frequency_no &= 0x03;
		}
	}

	memset(packet,0x00,packet_length);

	//Channels
	uint8_t val;
	if (sub_protocol >= FX9630)
	{ // FX9630 & FX_Q560
		packet[0] = convert_channel_8b(THROTTLE);
		packet[1] = convert_channel_8b(AILERON);
		packet[2] = 0xFF - convert_channel_8b(ELEVATOR);
		packet[3] = convert_channel_8b(RUDDER);
		val = trim_ch==0 ? 0x20 : (convert_channel_8b(trim_ch + CH6) >> 2);	// no trim on Throttle
		packet[4] = val;			// Trim for channel x 0C..20..34
		packet[5] = (trim_ch << 4)	// channel x << 4
					| GET_FLAG(CH5_SW, 0x01)  // DR toggle swich: 0 small throw, 1 large throw
					// FX9630  =>0:6G small throw, 1:6G large throw, 2:3D
					// QIDI-550=>0:3D, 1:6G, 2:Torque
					| (Channel_data[CH6] < CHANNEL_MIN_COMMAND ? 0x00 : (Channel_data[CH6] > CHANNEL_MAX_COMMAND ? 0x04 : 0x02));
		if(sub_protocol == FX_Q560)
			packet[5] |= GET_FLAG(CH7_SW, 0x10);
	}
	else // FX816 and FX620
	{
		uint8_t offset=sub_protocol == FX816 ? FX816_CH_OFFSET:FX620_CH_OFFSET;
		val=convert_channel_8b(AILERON);
		if(val>127+FX_SWITCH)
			packet[offset] = sub_protocol == FX816 ? 1:0xFF;
		else if(val<127-FX_SWITCH)
			packet[offset] = sub_protocol == FX816 ? 2:0x00;
		else
			packet[offset] = sub_protocol == FX816 ? 0:0x7F;
		packet[offset+1] = convert_channel_16b_limit(THROTTLE,0,100);	//FX816:0x00..0x63, FX620:0x00..0x5E but that should work
	}

	//Bind and specifics
	if(sub_protocol == FX816)
	{
		if(IS_BIND_IN_PROGRESS)
			packet[0] = 0x55;
		else
			packet[0] = 0xAA;
		packet[1] = rx_tx_addr[0];
		packet[2] = rx_tx_addr[1];
	}
	else if(sub_protocol == FX620)
	{
		if(IS_BIND_IN_PROGRESS)
		{
			memcpy(packet,rx_tx_addr,3);
			packet[3] = hopping_frequency[0];
			if(bind_counter > (FX_BIND_COUNT >> 1))
				packet[5] = 0x78;
		}
		else
		{
			packet[0] = 0x1F;	// Is it based on ID??
			packet[5] = 0xAB;	// Is it based on ID??
		}
	}
	else // FX9630 & FX_Q560
	{
		if(IS_BIND_IN_PROGRESS)
		{
			memcpy(packet,rx_tx_addr, 4);
			packet[4] = hopping_frequency[1];
			packet[5] = hopping_frequency[2];
			packet[7] = 0x55;
		}
	}

	//Check
	uint8_t last_packet_idx = packet_length-1;
	if (sub_protocol == FX9630 && IS_BIND_IN_PROGRESS)
		last_packet_idx--;
	val=0;
	for(uint8_t i=0;i<last_packet_idx;i++)
		val+=packet[i];
	if (sub_protocol == FX9630)
		val = val ^ 0xFF;
	packet[last_packet_idx]=val;

	//Debug
	#if 0
		for(uint8_t i=0;i<packet_length;i++)
			debug("%02X ",packet[i]);
		debugln("");
	#endif
	
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, packet_length);
}

static void __attribute__((unused)) FX_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	if(sub_protocol == FX816)
	{
		XN297_SetTXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc", 5);
		XN297_RFChannel(FX816_BIND_CHANNEL);
		packet_period = FX816_PACKET_PERIOD;
		packet_length = FX816_PAYLOAD_SIZE;
	}
	else if(sub_protocol == FX620)
	{
		XN297_SetTXAddr((uint8_t *)"\xaa\xbb\xcc", 3);
		XN297_RFChannel(FX620_BIND_CHANNEL);
		packet_period = FX620_BIND_PACKET_PERIOD;
		packet_length = FX620_PAYLOAD_SIZE;
	}
	else // FX9630 & FX_Q560
	{
		XN297_SetTXAddr((uint8_t *)"\x56\x78\x90\x12", 4);
		XN297_RFChannel(FX9630_BIND_CHANNEL);
		packet_period = FX9630_BIND_PACKET_PERIOD;
		packet_length = FX9630_PAYLOAD_SIZE;
	}
}

static void __attribute__((unused)) FX_initialize_txid()
{
	if(sub_protocol == FX816)
	{
		//Only 8 IDs: the RX led does not indicate frame loss.
		//I didn't open the plane to find out if I could connect there so this is the best I came up with with few trial and errors...
		rx_tx_addr[0]=0x35+(rx_tx_addr[3]&0x07);							//Original dump=0x35
		rx_tx_addr[1]=0x09;													//Original dump=0x09
		memcpy(hopping_frequency,"\x09\x1B\x30\x42",FX_NUM_CHANNELS);		//Original dump=9=0x09,27=0x1B,48=0x30,66=0x42
		for(uint8_t i=0;i<FX_NUM_CHANNELS;i++)
			hopping_frequency[i]+=rx_tx_addr[3]&0x07;
	}
	else if(sub_protocol == FX620)
	{
		rx_tx_addr[0] = rx_tx_addr[3];
		hopping_frequency[0] = 0x18 + rx_tx_addr[3]&0x07;	// just to try something
		#ifdef FORCE_FX620_ID
			memcpy(rx_tx_addr,(uint8_t*)"\x34\xA9\x32",3);
			hopping_frequency[0] = 0x18;	//on dump: 24 34 44 54
		#endif
		for(uint8_t i=1;i<FX_NUM_CHANNELS;i++)
			hopping_frequency[i] = i*10 + hopping_frequency[0];
	}
	else // FX9630 & FX_Q560
	{
		#ifdef FORCE_FX9630_ID
			memcpy(rx_tx_addr,(uint8_t*)"\xCE\x31\x9B\x73", 4);
			memcpy(hopping_frequency,"\x13\x1A\x38", FX9630_NUM_CHANNELS);		//Original dump=>19=0x13,26=0x1A,56=0x38
		#else
			hopping_frequency[0] = 0x13; // constant???
			hopping_frequency[1] = RX_num & 0x0F + 0x1A;
			hopping_frequency[2] = rx_tx_addr[3] & 0x0F + 0x38;
		#endif
		#ifdef FORCE_QIDI_ID
			memcpy(rx_tx_addr,(uint8_t*)"\x23\xDC\x76\xA2", 4);
			memcpy(hopping_frequency,"\x08\x25\x33", FX9630_NUM_CHANNELS);		//Original dump=>08=0x08,37=0x25,51=0x33

			//QIDI-560 #1
			//memcpy(rx_tx_addr,(uint8_t*)"\x38\xC7\x6D\x8D", 4);
			//memcpy(hopping_frequency,"\x0D\x20\x3A", FX9630_NUM_CHANNELS);
		#endif
		//??? Need to find out how the first RF channel is calculated ???
	}
}

uint16_t FX_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(packet_period);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
		{
			BIND_DONE;
			if(sub_protocol == FX620)
			{
				XN297_SetTXAddr(rx_tx_addr, 3);
				packet_period = FX620_PACKET_PERIOD;
			}
		}
	FX_send_packet();
	return packet_period;
}

void FX_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	FX_initialize_txid();
	FX_RF_init();
	hopping_frequency_no = 0;
	bind_counter=FX_BIND_COUNT;
}

#endif
