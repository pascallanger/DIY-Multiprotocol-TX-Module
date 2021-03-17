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
// Compatible with Global Drone GW008 protocol.
// There are 3 versions of this small quad, this protocol is for the one with a XNS104 IC in the stock Tx and PAN159CY IC in the quad (SOCs with built-in xn297 compatible RF).
// The xn297 version is compatible with the CX10 protocol (green pcb).
// The LT8910 version is not supported yet.

#if defined(GW008_NRF24L01_INO)

#include "iface_xn297.h"

#define GW008_INITIAL_WAIT    500
#define GW008_PACKET_PERIOD   2400
#define GW008_RF_BIND_CHANNEL 2
#define GW008_PAYLOAD_SIZE    15

enum {
	GW008_BIND1,
	GW008_BIND2,
	GW008_DATA
};

static void __attribute__((unused)) GW008_send_packet()
{
	packet[0] = rx_tx_addr[0];
	if(IS_BIND_IN_PROGRESS)
	{
		packet[1] = 0x55;
		packet[2] = hopping_frequency[0];
		packet[3] = hopping_frequency[1];
		packet[4] = hopping_frequency[2];
		packet[5] = hopping_frequency[3];
		memset(&packet[6], 0, 7);
		packet[13] = 0xaa;
	}
	else
	{
		XN297_Hopping((hopping_frequency_no++)/2);
		hopping_frequency_no %= 8;

		packet[1] = 0x01 | GET_FLAG(CH5_SW, 0x40); // flip
		packet[2] = convert_channel_16b_limit(AILERON , 200, 0); // aileron
		packet[3] = convert_channel_16b_limit(ELEVATOR, 0, 200); // elevator
		packet[4] = convert_channel_16b_limit(RUDDER  , 200, 0); // rudder
		packet[5] = convert_channel_16b_limit(THROTTLE, 0, 200); // throttle
		packet[6] = 0xaa;
		packet[7] = 0x02; // max rate
		packet[8] = 0x00;
		packet[9] = 0x00;
		packet[10]= 0x00;
		packet[11]= 0x00;
		packet[12]= 0x00;
		packet[13]= rx_tx_addr[2];
	}
	packet[14] = rx_tx_addr[1];

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, GW008_PAYLOAD_SIZE, 0);
}

static void __attribute__((unused)) GW008_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	
	XN297_SetTXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", 5);
	XN297_SetRXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", GW008_PAYLOAD_SIZE);

	//XN297_HoppingCalib(8);
	XN297_RFChannel(GW008_RF_BIND_CHANNEL);
}

static void __attribute__((unused)) GW008_initialize_txid()
{
	uint32_t lfsr = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
    for(uint8_t i=0; i<4; i++)
        hopping_frequency[i] = 0x10 + ((lfsr >> (i*8)) % 0x37);
}

uint16_t GW008_callback()
{
	switch(phase)
	{
		case GW008_BIND1:
			if(XN297_IsRX() &&	// RX fifo data ready
				XN297_ReadEnhancedPayload(packet, GW008_PAYLOAD_SIZE) == GW008_PAYLOAD_SIZE &&	// check payload size
				packet[0] == rx_tx_addr[0] && packet[14] == rx_tx_addr[1])			// check tx id
			{
				XN297_SetTxRxMode(TXRX_OFF);
				XN297_SetTxRxMode(TX_EN);
				rx_tx_addr[2] = packet[13];
				BIND_DONE;
				phase = GW008_DATA;
			}
			else
			{
				XN297_SetTxRxMode(TXRX_OFF);
				XN297_SetTxRxMode(TX_EN);
				GW008_send_packet();
				phase = GW008_BIND2;
				return 850; // minimum value 750 for STM32
			}
			break;
		case GW008_BIND2:
			// switch to RX mode
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = GW008_BIND1;
			return 5000;
		case GW008_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(GW008_PACKET_PERIOD);
			#endif
			GW008_send_packet();
			break;
	}
	return GW008_PACKET_PERIOD;
}

void GW008_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	GW008_initialize_txid();
	phase = GW008_BIND1;
	GW008_RF_init();
	hopping_frequency_no = 0;
}

#endif
