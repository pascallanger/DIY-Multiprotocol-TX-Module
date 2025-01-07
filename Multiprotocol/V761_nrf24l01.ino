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
 
 Thanks to  Goebish ,Ported  from his deviation firmware
 */

#if defined(V761_NRF24L01_INO)

#include "iface_xn297.h"

//#define V761_FORCE_ID

#define V761_PACKET_PERIOD		7060 // Timeout for callback in uSec
#define V761_INITIAL_WAIT		500
#define V761_PACKET_SIZE		8
#define V761_RXPAYLOAD_SIZE		3
#define V761_BIND_COUNT			200
#define V761_BIND_FREQ			0x28
#define V761_RF_NUM_CHANNELS	3
#define TOPRC_BIND_FREQ			0x2A
#define TOPRC_PACKET_PERIOD		14120 // Timeout for callback in uSec

#define V761_WRITE_TIME			450
#define V761_TELEM_PACKET_PERIOD	14088 // Timeout for callback in uSec
//#define V761_TELEM_DEBUG

enum 
{
	V761_BIND1 = 0,
	V761_BIND2,
	V761_RX_CHECK,
	V761_DATA,
	V761_RX
};

static void __attribute__((unused)) V761_set_checksum()
{
	uint8_t checksum = packet[0];
	for(uint8_t i=1; i<V761_PACKET_SIZE-2; i++)
		checksum += packet[i];
	if(phase == V761_BIND1)
	{
		packet[6] = checksum ^ 0xff;
		packet[7] = packet[6];
	}
	else 
	{
		checksum += packet[6];
		packet[7] = checksum ^ 0xff;
	}
}

static void __attribute__((unused)) V761_send_packet()
{
	static bool calib=false, prev_ch6=false;

	if(phase != V761_DATA)
	{
		memcpy(packet, rx_tx_addr, 4);
		packet[4] = hopping_frequency[1];
		packet[5] = hopping_frequency[2];
		if(phase == V761_BIND2)
			packet[6] = 0xF0;							// ?
	}
	else
	{ 
		#ifdef V761_TELEM_DEBUG
			debug_time("");
			debugln(" Ph:%d",hopping_frequency_no);
		#endif
		XN297_Hopping(hopping_frequency_no++);
		if(hopping_frequency_no >= V761_RF_NUM_CHANNELS)
		{
			hopping_frequency_no = 0;
			packet_sent++;
			packet_sent &= 0x03;
		}

		packet[0] = convert_channel_8b(THROTTLE);		// Throttle
		packet[2] = convert_channel_8b(ELEVATOR)>>1;	// Elevator

		if(sub_protocol == V761_4CH || sub_protocol == V761_TOPRC)
		{
			packet[1] = convert_channel_8b(AILERON)>>1;	// Aileron
			packet[3] = convert_channel_8b(RUDDER)>>1;	// Rudder
		}
		else
		{
			packet[1] = convert_channel_8b(RUDDER)>>1;	// Rudder
			packet[3] = convert_channel_8b(AILERON)>>1;	// Aileron
		}

		packet[5] = packet_sent<<6;						// 0X, 4X, 8X, CX
		packet[4] = 0x20;								// Trims 00..20..40, 0X->20 4X->TrAil 8X->TrEle CX->TrRud

		if(CH5_SW)										// Mode Expert Gyro off
			flags = 0x0c;
		else
			if(Channel_data[CH5] < CHANNEL_MIN_COMMAND)
				flags = 0x08;							// Beginer mode (Gyro on, yaw and pitch rate limited)
			else
				flags = 0x0a;							// Mid Mode ( Gyro on no rate limits)        

		if(!prev_ch6 && CH6_SW)							// -100% -> 100% launch gyro calib
			calib=!calib;
		prev_ch6 = CH6_SW;
		if(calib)
			flags |= 0x01;								// Gyro calibration

		packet[5] |= flags;

		packet[6] =  GET_FLAG(CH7_SW,  0x20) 			// Flip
					|GET_FLAG(CH8_SW,  0x08)			// RTH activation
					|GET_FLAG(CH9_SW,  0x10)			// RTH on/off
					|GET_FLAG(CH10_SW, 0x40);			// Beeper on/off
		if(sub_protocol == V761_3CH)
			packet[6] |= 0x80;							// Unknown, set on original V761-1 dump but not on eachine dumps, keeping for compatibility
	}
	V761_set_checksum();

	#if 0
		debug("H:%02X P:",hopping_frequency_no);
		for(uint8_t i=0;i<V761_PACKET_SIZE;i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, V761_PACKET_SIZE);
}

static void __attribute__((unused)) V761_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
}

static void __attribute__((unused)) V761_initialize_txid()
{
	#ifdef V761_FORCE_ID
		if(sub_protocol == V761_TOPRC)
		{ //Dump from air on TopRCHobby TX
			memcpy(rx_tx_addr,(uint8_t *)"\xD5\x01\x00\x00",4);
			memcpy(hopping_frequency,(uint8_t *)"\x2E\x41",2);
		}
		else
		{
			switch(RX_num%5)
			{
				case 1:	//Dump from air on Protonus TX
					memcpy(rx_tx_addr,(uint8_t *)"\xE8\xE4\x45\x09",4);
					memcpy(hopping_frequency,(uint8_t *)"\x0D\x21",2);
					break;
				case 2:	//Dump from air on mshagg2 TX
					memcpy(rx_tx_addr,(uint8_t *)"\xAE\xD1\x45\x09",4);
					memcpy(hopping_frequency,(uint8_t *)"\x13\x1D",2);
					break;
				case 3:	//Dump from air on MikeHRC Eachine TX
					memcpy(rx_tx_addr,(uint8_t *)"\x08\x03\x00\xA0",4);
					memcpy(hopping_frequency,(uint8_t *)"\x0D\x21",2);
					break;
				case 4:	//Dump from air on Crashanium Eachine TX
					memcpy(rx_tx_addr,(uint8_t *)"\x58\x08\x00\xA0",4);
					memcpy(hopping_frequency,(uint8_t *)"\x0D\x31",2);
					break;
				default: //Dump from SPI
					memcpy(rx_tx_addr,(uint8_t *)"\x6f\x2c\xb1\x93",4);
					memcpy(hopping_frequency,(uint8_t *)"\x14\x1e",2);
					break;
			}
		}
	#else
		//Tested with Eachine RX
		rx_tx_addr[0]+=RX_num;
		hopping_frequency[0]=(rx_tx_addr[0]&0x0F)+0x05;
		hopping_frequency[1]=hopping_frequency[0]+0x05+(RX_num%0x2D);
	#endif

	hopping_frequency[2]=hopping_frequency[0]+0x37;

	debugln("ID: %02X %02X %02X %02X , HOP: %02X %02X %02X",rx_tx_addr[0],rx_tx_addr[1],rx_tx_addr[2],rx_tx_addr[3],hopping_frequency[0],hopping_frequency[1],hopping_frequency[2]);
}

uint16_t V761_callback()
{
	static bool rx = false;
	static uint8_t packet_telem = 0;

	switch(phase)
	{
		case V761_BIND1:
			if(bind_counter) 
				bind_counter--;
			packet_sent ++;
			XN297_RFChannel(sub_protocol == V761_TOPRC ? TOPRC_BIND_FREQ : V761_BIND_FREQ);
			XN297_SetTXAddr(rx_id, 4);
			V761_send_packet();
			if(packet_sent >= 20) 
			{
				packet_sent = 0;
				phase = V761_BIND2;
			}
			return 15730;
		case V761_BIND2:
			if(bind_counter) 
				bind_counter--;
			packet_sent ++;
			XN297_Hopping(0);
			XN297_SetTXAddr(rx_tx_addr, 4);
			V761_send_packet();
			if(bind_counter == 0) 
			{
				packet_sent = 0;
				BIND_DONE;
				phase = V761_DATA;
				#ifdef V761_HUB_TELEMETRY
					XN297_SetRXAddr(rx_tx_addr, V761_RXPAYLOAD_SIZE);
				#endif
			}
			else if(packet_sent >= 20) 
			{
				packet_sent = 0;
				phase = V761_BIND1;
			}
			return 15730;
	#ifdef V761_HUB_TELEMETRY
		case V761_RX_CHECK:
			rx = XN297_IsRX();					// Needed for the NRF24L01 since otherwise the bit gets cleared
			XN297_SetTxRxMode(TXRX_OFF);
			if(packet_count > 4*63)				// Around 3.5sec with no telemetry
			{
				telemetry_lost = 1;
				packet_period = V761_PACKET_PERIOD;
			}
			else
			{
				packet_count++;
				if(!telemetry_lost && !rx && (packet_count%64) == 0)
				{// Should have received a telem packet but... Send telem to the radio to keep it alive
					telemetry_link = 1;
					#ifdef V761_TELEM_DEBUG
						debugln("Miss");
					#endif
				}
			}
			phase++;
	#endif
		case V761_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(packet_period);
			#endif
			V761_send_packet();
	#ifdef V761_HUB_TELEMETRY
			if(sub_protocol == V761_TOPRC)
				break;
			if(rx)
			{ // Check if a packet has been received
				#ifdef V761_TELEM_DEBUG
					debug("RX ");
				#endif
				if(XN297_ReadPayload(packet_in, V761_RXPAYLOAD_SIZE))
				{ // packet with good CRC and length
					#ifdef V761_TELEM_DEBUG
						debug("OK:");
						for(uint8_t i=0;i<V761_RXPAYLOAD_SIZE;i++)
							debug(" %02X",packet_in[i]);
						debug(" pps:%d", packet_count);
					#endif
					// packet_in[] = AA 00 55 -> battery ok
					// packet_in[] = 55 00 AA -> low battery
					crc8 = 0;
					for(uint8_t i=0;i<V761_RXPAYLOAD_SIZE;i++)
						crc8 ^= packet_in[i];
					if(crc8 == 0xFF)
					{
						v_lipo1 = packet_in[0] >> 1;
						telemetry_link = 1;
						telemetry_lost = 0;
						packet_count = 0;
						packet_period = V761_TELEM_PACKET_PERIOD;
					}
					#ifdef V761_TELEM_DEBUG
					else // Bad packet
						debug(" NOK");
					#endif
				}
				#ifdef V761_TELEM_DEBUG
				else // Bad packet
					debug("NOK");
				debugln("");
				#endif
				rx = false;
			}
			phase++;
			return V761_WRITE_TIME;
		case V761_RX:
			{ // Wait for packet to be sent before switching to receive mode
				uint16_t start=(uint16_t)micros();
				while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)
					if(XN297_IsPacketSent())
						break;
			}
			XN297_SetTxRxMode(RX_EN);
			phase = V761_RX_CHECK;
			return packet_period - V761_WRITE_TIME;
	#else
			break;
	#endif
	}
	return packet_period;
}

void V761_init(void)
{
	V761_initialize_txid();
	V761_RF_init();
	if(sub_protocol == V761_TOPRC)
	{
		memcpy(rx_id,(uint8_t*)"\x20\x21\x05\x0A",4);
		packet_period = TOPRC_PACKET_PERIOD;
	}
	else
	{
		memcpy(rx_id,(uint8_t*)"\x34\x43\x10\x10",4);
		packet_period = V761_PACKET_PERIOD;
	}

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = V761_BIND_COUNT;
		phase = V761_BIND1;
	}
	else
	{
		XN297_SetTXAddr(rx_tx_addr, 4);
		#ifdef V761_HUB_TELEMETRY
			XN297_SetRXAddr(rx_tx_addr, V761_RXPAYLOAD_SIZE);
		#endif
		phase = V761_DATA;
	}

	hopping_frequency_no = 0;
	packet_sent = 0;
	#ifdef V761_HUB_TELEMETRY
		packet_count = 0;
		telemetry_lost = 1;
		RX_RSSI = 100;		// Dummy value
	#endif
}

#endif