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

//#define FORCE_FX620_ID

static void __attribute__((unused)) FX_send_packet()
{
	//Hopp
	if(IS_BIND_DONE)
	{
		XN297_Hopping(hopping_frequency_no++);
		hopping_frequency_no &= 0x03;
	}

	memset(packet,0x00,packet_length);

	//Channels
	uint8_t offset=sub_protocol == FX816 ? FX816_CH_OFFSET:FX620_CH_OFFSET;
	uint8_t val=convert_channel_8b(AILERON);
	if(val>127+FX_SWITCH)
		packet[offset] = sub_protocol == FX816 ? 1:0xFF;
	else if(val<127-FX_SWITCH)
		packet[offset] = sub_protocol == FX816 ? 2:0x00;
	else
		packet[offset] = sub_protocol == FX816 ? 0:0x7F;
	packet[offset+1] = convert_channel_16b_limit(THROTTLE,0,100);	//FX816:0x00..0x63, FX620:0x00..0x5E but that should work

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
	else //FX620
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

	//Check
	val=0;
	for(uint8_t i=0;i<packet_length-1;i++)
		val+=packet[i];
	packet[packet_length-1]=val;

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
	else //FX620
	{
		XN297_SetTXAddr((uint8_t *)"\xaa\xbb\xcc", 3);
		XN297_RFChannel(FX620_BIND_CHANNEL);
		packet_period = FX620_BIND_PACKET_PERIOD;
		packet_length = FX620_PAYLOAD_SIZE;
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
	else//FX620
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
