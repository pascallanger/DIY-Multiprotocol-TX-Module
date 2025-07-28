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
// Compatible with SGF22, ParkTen F22S, KF700 J20

#if defined(SGF22_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_SGF22_ORIGINAL_ID

#define SGF22_PACKET_PERIOD				11950 //10240
#define SGF22_BIND_RF_CHANNEL			78
#define SGF22_PAYLOAD_SIZE				12
#define SGF22_BIND_COUNT				50
#define SGF22_RF_NUM_CHANNELS			4
#define SGF22_F22S_BIND_RF_CHANNEL		10
#define SGF22_J20_BIND_RF_CHANNEL  		28
#define CX10_BIND_RF_CHANNEL			48

//packet[8]
#define SGF22_FLAG_3D					0x00
#define SGF22_FLAG_LIGHT				0x04
#define SGF22_FLAG_ROLL					0x08
#define SGF22_FLAG_VIDEO				0x10
#define SGF22_FLAG_6G					0x40
#define SGF22_FLAG_VERTICAL				0xC0

#define SGF22_J20_FLAG_HORIZONTAL		0x80
//#define SGF22_J20_FLAG_SPEED			0x01	// Up/Down trim, not implemented

#define SGF22_FX922_FLAG_BALANCEHIGH	0x01
#define SGF22_FX922_FLAG_BALANCE		0x02


//packet[9]
#define SGF22_FLAG_TRIMRESET			0x04
#define SGF22_FLAG_PHOTO				0x40	// #define SGF22_J20_FLAG_INVERT  0x40
#define SGF22_J20_FLAG_FIXHEIGHT		0x80

#define SGF22_WRITE_TIME				1000

enum {
	SGF22_DATA1,
	SGF22_DATA2,
	SGF22_DATA3,
	SGF22_RX,
};

static void __attribute__((unused)) SGF22_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
//		packet[ 0] = 0x5B;
		packet[ 0] = 0xC5;
		packet[ 1] = 0x11;
		packet[ 8] = 0x00;								// ??? do they have to be 0 for bind to succeed ?
		packet[ 9] = 0x00;								// ??? do they have to be 0 for bind to succeed ?
		packet[10] = 0xAA;
		packet[11] = 0x55;
	}
	else
	{
		//hop
		XN297_Hopping(packet_sent & 0x03);				// ??? from the dumps I can't really say how hop and seq are sync, there could be an offset (0,1,2,3)...
		//sequence from 02 to 7A by increments of 4, sometimes with a flag 0x80 from 82 to FA, I can't tell from the dumps when the switch happens
		if( (packet_sent & 0x03) == 0x02)
			packet_count = packet_sent;
		packet_sent++;
		if(packet_sent > 0x7B)
			packet_sent = 0;
		//packet
//		packet[0] = 0x1B;
		packet[0] = 0x85;
		packet[1] = packet_count;							// sequence
		packet[8] = SGF22_FLAG_3D           			// CH5 -100%, F22 & F22S - 3D mode, J20 - Gyro off  
			| GET_FLAG(CH6_SW, SGF22_FLAG_ROLL)			// roll
			| GET_FLAG(CH7_SW, SGF22_FLAG_LIGHT)		// push up throttle trim for light in the stock TX
			| GET_FLAG(CH9_SW, SGF22_FLAG_VIDEO)		// push down throttle trim for video in the stock TX
			| GET_FLAG(CH11_SW, SGF22_FX922_FLAG_BALANCE)
			| GET_FLAG(CH12_SW, SGF22_FX922_FLAG_BALANCEHIGH);
		if(Channel_data[CH5] > CHANNEL_MAX_COMMAND)
			packet[8] |= SGF22_FLAG_VERTICAL;     		// CH5 100%,  vertical mode (torque)    
		else if(Channel_data[CH5] > CHANNEL_MIN_COMMAND )
			packet[8] |= ( sub_protocol == SGF22_J20 ? SGF22_J20_FLAG_HORIZONTAL : SGF22_FLAG_6G );     // CH5 0%, F22 & F22S - 6G mode, J20 - Horizontal mode

		// Channel 6 - packet 8 is rate mode or flip mode
		if(CH6_SW)												// rate 3
			flags = 0x02;
		else
			if(Channel_data[CH6] < CHANNEL_MIN_COMMAND)
				flags = 0x00;									// rate 1
			else
				flags = 0x01;
		packet[8] = !CH5_SW ? 0x04 + flags : 0x0C;

		packet[9] = GET_FLAG(CH8_SW, SGF22_FLAG_PHOTO)  // F22: photo, press in throttle trim in the stock TX, J20: invert flight
			| GET_FLAG(CH10_SW, ( sub_protocol == SGF22_J20 ? SGF22_J20_FLAG_FIXHEIGHT : SGF22_FLAG_TRIMRESET )) ;   // F22: Both sticks down inwards in the stock TX, J20: Altitude hold
		packet[10] = 0x42;								// no fine tune
		packet[11] = 0x10;								// no fine tune
	}
	if(sub_protocol == SGF22_F22S)
		packet[0] += 6;
	else if (sub_protocol == SGF22_J20)
    		packet[0] += 3; 
//	packet[1] = packet_count;							// sequence
	packet[2] = rx_tx_addr[2];
	packet[3] = rx_tx_addr[3];
	packet[4] = convert_channel_8b(THROTTLE);
	packet[5] = convert_channel_8b(RUDDER);
	packet[6] = convert_channel_8b(ELEVATOR);
	packet[7] = convert_channel_8b(AILERON);

	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
//	XN297_WriteEnhancedPayload(packet, SGF22_PAYLOAD_SIZE,0);
	XN297_WritePayload(packet, SGF22_PAYLOAD_SIZE);
	#if 0
		debug_time("");
		for(uint8_t i=0; i<SGF22_PAYLOAD_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
}

static void __attribute__((unused)) SGF22_initialize_txid()
{
	uint16_t val = ( rx_tx_addr[2] << 8 ) | rx_tx_addr[3];
	if ( rx_tx_addr[2] > ( 0xFF - rx_tx_addr[3]) )
		val--;
	val %= 5;

	const uint8_t hop[5][4] =
	{ { 0x0C, 0x2A, 0x1B, 0x39 },
	  { 0x0F, 0x2D, 0x1E, 0x3D },
	  { 0x12, 0x31, 0x21, 0x41 },
	  { 0x15, 0x34, 0x24, 0x44 },
	  { 0x18, 0x37, 0x27, 0x47 } };
	memcpy(hopping_frequency, &hop[val], SGF22_RF_NUM_CHANNELS);

	/*//Same code sze...
	hopping_frequency[0] = 0x0C + 3 * val;
	hopping_frequency[1] = hopping_frequency[0] + 0x1E;
	if(val > 1) hopping_frequency[1]++;
	hopping_frequency[2] = hopping_frequency[0] + 0x0F;
	hopping_frequency[3] = hopping_frequency[1] + 0x0F;
	if(val    ) hopping_frequency[3]++;*/
	
	#ifdef FORCE_SGF22_ORIGINAL_ID
		rx_tx_addr[2] = 0x1F;	// TX2:27 TX3:2B
		rx_tx_addr[3] = 0x61;	// TX2:51 TX3:0C
		memcpy(hopping_frequency,"\x15\x34\x24\x44", SGF22_RF_NUM_CHANNELS);    //Original dump=>21=0x15,52=0x34,36=0x24,68=0x44
	#endif
	#if 0
		debug("ID: %02X %02X, C: ",rx_tx_addr[2],rx_tx_addr[3]);
		for(uint8_t i=0; i<SGF22_RF_NUM_CHANNELS; i++)
			debug(" %02X",hopping_frequency[i]);
		debugln("");
	#endif

	rx_tx_addr[2] = 0x4C;
	rx_tx_addr[3] = 0xD7;
	memcpy(hopping_frequency, "\x37\x42\x47\x3c", SGF22_RF_NUM_CHANNELS);

/*
	rx_tx_addr[2] = 0x50;
	rx_tx_addr[3] = 0xE1;
	memcpy(hopping_frequency, "\x3b\x4b\x46\x41", SGF22_RF_NUM_CHANNELS);
*/
}

static void __attribute__((unused)) SGF22_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"\xC7\x95\x3C\xBB\xA5", 5);
	#ifdef SGF22_HUB_TELEMETRY
		XN297_SetRXAddr((uint8_t*)"\xC7\x95\x3C\xBB\xA5", SGF22_PAYLOAD_SIZE);
	#endif

//	const uint8_t bind_chan[] = {SGF22_BIND_RF_CHANNEL, SGF22_F22S_BIND_RF_CHANNEL, SGF22_J20_BIND_RF_CHANNEL};
	const uint8_t bind_chan[] = {CX10_BIND_RF_CHANNEL, SGF22_F22S_BIND_RF_CHANNEL, SGF22_J20_BIND_RF_CHANNEL};
	XN297_RFChannel(bind_chan[sub_protocol]);	// Set bind channel
}

uint16_t SGF22_callback()
{
	#ifdef SGF22_HUB_TELEMETRY
		bool rx = false;
		static uint8_t telem_count = 0;
	#endif

	switch(phase)
	{
		case SGF22_DATA1:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(SGF22_PACKET_PERIOD);
			#endif
	#ifdef SGF22_HUB_TELEMETRY
			rx = XN297_IsRX();
			XN297_SetTxRxMode(TXRX_OFF);
	#endif
			SGF22_send_packet();
			if(IS_BIND_IN_PROGRESS)
			{
				if(--bind_counter==0)
					BIND_DONE;
			}
	#ifdef SGF22_HUB_TELEMETRY
			if(rx)
			{
				uint8_t p_len = XN297_ReadEnhancedPayload(packet_in, SGF22_PAYLOAD_SIZE);
				if(p_len == 3 && packet_in[0] == rx_tx_addr[2] && packet_in[1] == rx_tx_addr[3])
				{//packets: 00 0B 00 -> 00 0B 01
					telemetry_link = 1;
					v_lipo1 = packet_in[2] ? 0 : 255;	//2.9V for 1S, 7.0V for 2S
					telemetry_lost = 0;
					telem_count = 0;
				}
				#if 0
					debug("L %d ",p_len);
					debug("RX");
					for(uint8_t i=0; i<SGF22_PAYLOAD_SIZE; i++)
						debug(" %02X",packet_in[i]);
					debugln("");
				#endif
			}
			if(telem_count > 4*63)				// Around 3.5sec with no telemetry
				telemetry_lost = 1;
			else
			{
				telem_count++;
				if(!telemetry_lost && (telem_count & 0x3F) == 0)
				{// Should have received a telem packet but... Send telem to the radio to keep it alive
					telemetry_link = 1;
					#if 0
						debugln("Miss");
					#endif
				}
			}

	#endif
			phase++;
			break;
		case SGF22_DATA2:
		case SGF22_DATA3:
			//send 3 times in total the same packet
			XN297_ReSendPayload();
			phase++;
			break;
		default: //SGF22_RX
	#ifdef SGF22_HUB_TELEMETRY
			/*{ // Wait for packet to be sent before switching to receive mode
				uint16_t start=(uint16_t)micros(), count=0;
				while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)
				{
					if(XN297_IsPacketSent())
						break;
					count++;
				}
				debugln("%d",count);
			}*/
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
	#endif
			phase = SGF22_DATA1;
			return SGF22_PACKET_PERIOD - 3*1550;
	}
	return 1550;
}

void SGF22_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	SGF22_initialize_txid();
	SGF22_RF_init();
	bind_counter=SGF22_BIND_COUNT;
	packet_sent = packet_count = 0x26;	// TX2:26 TX3:26
	phase = SGF22_DATA1;
	#ifdef SGF22_HUB_TELEMETRY
		RX_RSSI = 100;		// Dummy value
		telemetry_lost = 1;
	#endif
}

#endif
