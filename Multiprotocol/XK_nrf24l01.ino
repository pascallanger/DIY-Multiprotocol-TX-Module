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
// Compatible with X450 and X420 plane.

#if defined(XK_NRF24L01_INO)

#include "iface_xn297l.h"

#define FORCE_XK_ORIGINAL_ID

#define XK_INITIAL_WAIT		500
#define XK_PACKET_PERIOD		4000
#define XK_RF_BIND_NUM_CHANNELS 8
#define XK_RF_NUM_CHANNELS	4
#define XK_PAYLOAD_SIZE		16
#define XK_BIND_COUNT			750	//3sec

static uint16_t __attribute__((unused)) XK_convert_channel(uint8_t num)
{
	uint16_t val=convert_channel_10b(num);
	// 1FF..01=left, 00=center, 200..3FF=right
	if(val==0x200)
		val=0;				// 0
	else
		if(val>0x200)
			val--;			// 200..3FE
		else
		{
			val=0x200-val;	// 200..01
			if(val==0x200)
				val--;		// 1FF..01
		}
	return val;
}

static void __attribute__((unused)) XK_send_packet()
{
	memset(packet,0x00,16);
	
	memcpy(&packet[7],rx_tx_addr,3);
	packet[12]=0x40;
	packet[13]=0x40;
	if(IS_BIND_IN_PROGRESS)
		packet[14] = 0xC0;
	else
	{
		uint16_t val=convert_channel_10b(THROTTLE);
		packet[0] = val>>2;						// 0..255
		packet[12] |= val & 2;
		val=XK_convert_channel(RUDDER);
		packet[1] = val>>2;
		packet[12] |= (val & 2)<<2;
		val=XK_convert_channel(ELEVATOR);
		packet[2] = val>>2;
		packet[13] |= val & 2;
		val=XK_convert_channel(AILERON);
		packet[3] = val>>2;
		packet[13] |= (val & 2)<<2;
		
		memset(&packet[4],0x40,3);				// Trims
		
		if(Channel_data[CH5] > CHANNEL_MAX_COMMAND)
			packet[10] = 0x10; 					// V-Mode
		else
			if(Channel_data[CH5] > CHANNEL_MIN_COMMAND)
				packet[10] = 0x04; 				// 6G-Mode
		//0x00 default M-Mode
		packet[10] |= GET_FLAG(CH7_SW,0x80);	// Emergency stop momentary switch

		packet[11]  = GET_FLAG(CH8_SW,0x03)		// 3D/6G momentary switch
					 |GET_FLAG(CH6_SW,0x40);	// Take off momentary switch
		packet[14]  = GET_FLAG(CH9_SW,0x01);	// Photo momentary switch
	}

	crc=packet[0];
	for(uint8_t i=1; i<XK_PAYLOAD_SIZE-1;i++)
		crc+=packet[i];
	packet[15]=crc;

	//debug("C: %02X, P:",hopping_frequency[(IS_BIND_IN_PROGRESS?0:XK_RF_BIND_NUM_CHANNELS)+(hopping_frequency_no>>1)]);
	XN297L_Hopping((IS_BIND_IN_PROGRESS?0:XK_RF_BIND_NUM_CHANNELS)+(hopping_frequency_no>>1));
	hopping_frequency_no++;
	if(hopping_frequency_no >= (IS_BIND_IN_PROGRESS?XK_RF_BIND_NUM_CHANNELS*2:XK_RF_NUM_CHANNELS*2))
		hopping_frequency_no=0;

	XN297L_WritePayload(packet, XK_PAYLOAD_SIZE);
	//for(uint8_t i=0; i<XK_PAYLOAD_SIZE; i++)
	//	debug(" %02X",packet[i]);
	//debugln("");
	
	XN297L_SetPower();		// Set tx_power
	XN297L_SetFreqOffset();	// Set frequency offset
}

const uint8_t PROGMEM XK_bind_hop[XK_RF_BIND_NUM_CHANNELS]= { 0x07, 0x24, 0x3E, 0x2B, 0x47, 0x0E, 0x39, 0x1C };	// Bind

static void __attribute__((unused)) XK_initialize_txid()
{
	//bind hop
	for(uint8_t i=0; i<XK_RF_BIND_NUM_CHANNELS; i++)
		hopping_frequency[i]=pgm_read_byte_near( &XK_bind_hop[i] );

	//#ifdef FORCE_XK_ORIGINAL_ID
		if(sub_protocol==X450)
		{
			//TX1 X8 X450
			rx_tx_addr[0]=0x04;
			rx_tx_addr[1]=0x15;
			rx_tx_addr[2]=0x22;
			memcpy(&hopping_frequency[XK_RF_BIND_NUM_CHANNELS],"\x3B\x48\x40\x49", XK_RF_NUM_CHANNELS);	// freq and order verified
			//Normal packet address \x2C\x96\x2A\xA9\x32

			//TX2 X450 X8?
			//rx_tx_addr[0]=0x24;
			//rx_tx_addr[1]=0x14;
			//rx_tx_addr[2]=0x22;
			//memcpy(&hopping_frequency[XK_RF_BIND_NUM_CHANNELS],"\x3A\x3B\x42\x48", XK_RF_NUM_CHANNELS);	// freq unsure and order unknown
			//Normal packet address \x2C\x96\x2A\xA9\x32
		}
		else
		{
			//TX3 X420
			rx_tx_addr[0]=0x13;
			rx_tx_addr[1]=0x24;
			rx_tx_addr[2]=0x18;
			memcpy(&hopping_frequency[XK_RF_BIND_NUM_CHANNELS],"\x36\x37\x41\x4E", XK_RF_NUM_CHANNELS);	// freq ok but order unknown
			//Normal packet address \xA6\x83\xEB\x4B\xC9
		}
	//#endif
}

static void __attribute__((unused)) XK_init()
{
	XN297L_Init();
	XN297L_SetTXAddr((uint8_t*)"\x68\x94\xA6\xD5\xC3", 5);					// Bind address
	XN297L_HoppingCalib(XK_RF_BIND_NUM_CHANNELS+XK_RF_NUM_CHANNELS);	// Calibrate all channels
}

uint16_t XK_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(XK_PACKET_PERIOD);
	#endif
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			if(sub_protocol==X450)
				XN297L_SetTXAddr((uint8_t*)"\x2C\x96\x2A\xA9\x32", 5);			// Normal address X450
			else
				XN297L_SetTXAddr((uint8_t*)"\xA6\x83\xEB\x4B\xC9", 5);			// Normal address X420
		}
	XK_send_packet();
	return XK_PACKET_PERIOD;
}

uint16_t initXK()
{
	BIND_IN_PROGRESS;	// autobind protocol
	XK_initialize_txid();
	XK_init();
	hopping_frequency_no = 0;
	bind_counter=XK_BIND_COUNT;
	return XK_INITIAL_WAIT;
}

#endif
