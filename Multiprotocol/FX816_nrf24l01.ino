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

#if defined(FX816_NRF24L01_INO)

#include "iface_xn297.h"

#define FX816_INITIAL_WAIT    500
#define FX816_PACKET_PERIOD   10000
#define FX816_RF_BIND_CHANNEL 0x28		//40
#define FX816_RF_NUM_CHANNELS 4
#define FX816_PAYLOAD_SIZE    6
#define FX816_BIND_COUNT	  300		//3sec

static void __attribute__((unused)) FX816_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
		packet[0] = 0x55;
	else
	{
		XN297_Hopping(hopping_frequency_no++);
		hopping_frequency_no%=FX816_RF_NUM_CHANNELS;
		packet[0] = 0xAA;
	}
	packet[1] = rx_tx_addr[0];
	packet[2] = rx_tx_addr[1];
	uint8_t val=convert_channel_8b(AILERON);
	#define FX816_SWITCH 20
	if(val>127+FX816_SWITCH)
		packet[3] = 1;
	else if(val<127-FX816_SWITCH)
		packet[3] = 2;
	else
		packet[3] = 0;
	packet[4] = convert_channel_16b_limit(THROTTLE,0,100);
	val=0;
	for(uint8_t i=0;i<FX816_PAYLOAD_SIZE-1;i++)
		val+=packet[i];
	packet[5]=val;

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, FX816_PAYLOAD_SIZE);
}

static void __attribute__((unused)) FX816_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc", 5);
	//XN297_HoppingCalib(FX816_RF_NUM_CHANNELS);
	XN297_RFChannel(FX816_RF_BIND_CHANNEL);
}

static void __attribute__((unused)) FX816_initialize_txid()
{
	//Only 8 IDs: the RX led does not indicate frame loss.
	//I didn't open the plane to find out if I could connect there so this is the best I came up with with few trial and errors...
	rx_tx_addr[0]=0x35+(rx_tx_addr[3]&0x07);							//Original dump=0x35
	rx_tx_addr[1]=0x09;													//Original dump=0x09
	memcpy(hopping_frequency,"\x09\x1B\x30\x42",FX816_RF_NUM_CHANNELS); //Original dump=9=0x09,27=0x1B,48=0x30,66=0x42
	for(uint8_t i=0;i<FX816_RF_NUM_CHANNELS;i++)
		hopping_frequency[i]+=rx_tx_addr[3]&0x07;
}

uint16_t FX816_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(FX816_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
			BIND_DONE;
	FX816_send_packet();
	return FX816_PACKET_PERIOD;
}

void FX816_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	FX816_initialize_txid();
	FX816_RF_init();
	hopping_frequency_no = 0;
	bind_counter=FX816_BIND_COUNT;
}

#endif
