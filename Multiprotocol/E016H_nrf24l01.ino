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
// compatible with E016H

#if defined(E016H_NRF24L01_INO)

#include "iface_xn297.h"

//Protocols constants
#define E016H_BIND_COUNT		500
#define E016H_ADDRESS_LENGTH	5

#define E016H_PACKET_PERIOD		4080
#define E016H_PACKET_SIZE		10
#define E016H_BIND_CHANNEL		80
#define E016H_NUM_CHANNELS		4

//Channels
#define E016H_STOP_SW			CH5_SW
#define E016H_FLIP_SW			CH6_SW
#define E016H_HEADLESS_SW		CH8_SW
#define E016H_RTH_SW			CH9_SW

// E016H flags packet[1]
#define E016H_FLAG_CALIBRATE	0x80
#define E016H_FLAG_STOP			0x20
#define E016H_FLAG_FLIP			0x04
// E016H flags packet[3]
#define E016H_FLAG_HEADLESS		0x10
#define E016H_FLAG_RTH			0x04
// E016H flags packet[7]
#define E016H_FLAG_TAKEOFF		0x80
#define E016H_FLAG_HIGHRATE		0x08

static void __attribute__((unused)) E016H_send_packet()
{
    uint8_t can_flip = 0, calibrate = 1;

	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(packet, &rx_tx_addr[1], 4);
		memcpy(&packet[4], hopping_frequency, 4);
		packet[8] = 0x23;
	}
	else
	{
		// trim commands
		packet[0] = 0;
		// aileron
		uint16_t val = convert_channel_16b_limit(AILERON, 0, 0x3ff);
		can_flip |= (val < 0x100) || (val > 0x300);
		packet[1] = val >> 8;
		packet[2] = val & 0xff;
		if(val < 0x300) calibrate = 0;
		// elevator
		val = convert_channel_16b_limit(ELEVATOR, 0x3ff, 0);
		can_flip |= (val < 0x100) || (val > 0x300);
		packet[3] = val >> 8;
		packet[4] = val & 0xff;
		if(val < 0x300) calibrate = 0;
		// throttle
		val = convert_channel_16b_limit(THROTTLE, 0, 0x3ff);
		packet[5] = val >> 8;
		packet[6] = val & 0xff;
		if(val > 0x100) calibrate = 0;
		// rudder
		val = convert_channel_16b_limit(RUDDER, 0, 0x3ff);
		packet[7] = val >> 8;
		packet[8] = val & 0xff;
		if(val > 0x100) calibrate = 0;
		// flags
		packet[1] |= GET_FLAG(E016H_STOP_SW, E016H_FLAG_STOP)
				  |  (can_flip ? GET_FLAG(E016H_FLIP_SW, E016H_FLAG_FLIP) : 0)
				  |  (calibrate ? E016H_FLAG_CALIBRATE : 0);
		packet[3] |= GET_FLAG(E016H_HEADLESS_SW, E016H_FLAG_HEADLESS)
				  |  GET_FLAG(E016H_RTH_SW, E016H_FLAG_RTH);
		packet[7] |= E016H_FLAG_HIGHRATE;
		// frequency hopping
		XN297_Hopping(hopping_frequency_no++ & 0x03);
	}
	// checksum
	packet[9] = packet[0];
	for (uint8_t i=1; i < E016H_PACKET_SIZE-1; i++)
		packet[9] += packet[i];

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, E016H_PACKET_SIZE);
}

static void __attribute__((unused)) E016H_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t *)"\x5a\x53\x46\x30\x31", 5);  // bind address
	//XN297_HoppingCalib(E016H_NUM_CHANNELS);
	XN297_RFChannel(E016H_BIND_CHANNEL);
}

uint16_t E016H_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(packet_period);
	#endif
	if(bind_counter)
	{
		bind_counter--;
		if (bind_counter == 0)
		{
			XN297_SetTXAddr(rx_tx_addr, E016H_ADDRESS_LENGTH);
			BIND_DONE;
		}
	}
	E016H_send_packet();
	return E016H_PACKET_PERIOD;
}

static void __attribute__((unused)) E016H_initialize_txid()
{
	// tx id
    rx_tx_addr[0] = 0xa5;
    rx_tx_addr[1] = 0x00;

    // rf channels
    uint32_t lfsr=random(0xfefefefe);
    for(uint8_t i=0; i<E016H_NUM_CHANNELS; i++)
    {
		hopping_frequency[i] = (lfsr & 0xFF) % 80; 
		lfsr>>=8;
	}
}

void E016H_init()
{
	BIND_IN_PROGRESS;
	E016H_initialize_txid();
	E016H_RF_init();
	bind_counter = E016H_BIND_COUNT;
	hopping_frequency_no = 0;
}

#endif
