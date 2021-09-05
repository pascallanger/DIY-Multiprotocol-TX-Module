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
// compatible with XERALL

#if defined(XERALL_NRF24L01_INO)

#include "iface_xn297.h"

//#define XERALL_ORIGINAL_ID

#define XERALL_PACKET_PERIOD		2500	//2046
#define XERALL_PACKET_SIZE			10
#define XERALL_NUM_RF_CHANNELS		4
#define XERALL_BIND_COUNT			15000	//about 30sec

// flags going to packet[6]
#define	XERALL_FLAG_VIDEO			0x80
#define	XERALL_FLAG_PHOTO			0x40
// flags going to packet[7]
#define	XERALL_FLAG_RATE			0x80
#define	XERALL_FLAG_FLIGHT_GROUND	0x20
#define	XERALL_FLAG_HEADING_HOLD	0x04
#define	XERALL_FLAG_ONE_BUTTON		0x02

enum {
	XERALL_DATA,
	XERALL_RX,
	XERALL_CHECK,
};

static void __attribute__((unused)) XERALL_send_packet()
{
	if(bind_phase)
		bind_phase--;
	else
	{	// Hopping frequency
		if(packet_sent==0)
		{
			XN297_Hopping(hopping_frequency_no);
			hopping_frequency_no++;
			hopping_frequency_no &= (XERALL_NUM_RF_CHANNELS-1);
		}
		packet_sent++;
		if(IS_BIND_IN_PROGRESS)
		{
			if(packet_sent > 24)
			packet_sent=0;													// Hopp after 25 packets
		}
		else
		{
			if(packet_sent > 18)
				packet_sent = 0;											// Hopp after 19 packets
		}

		// Packet
		if(IS_BIND_IN_PROGRESS && (bind_counter&0x10))						// Alternate bind and normal packets
		{ // Bind packet: 01 56 06 23 00 13 20 40 02 00 and 01 F9 58 31 00 13 20 40 05 00
			if(packet[0] != 0x01)
			{
				XN297_SetTXAddr((uint8_t *)"\x01\x01\x01\x01\x09", 5);		// Bind address
				XN297_SetRXAddr((uint8_t *)"\x01\x01\x01\x01\x09", XERALL_PACKET_SIZE);
			}
			packet[0] = 0x01;
			for(uint8_t i=0;i<5;i++)
				packet[i+1] = rx_tx_addr[i];
			packet[9] = 0;
		}
		else
		{
			if(packet[0] != 0x08)
			{
				XN297_SetTXAddr(rx_tx_addr, 5);
				XN297_SetRXAddr(rx_tx_addr, XERALL_PACKET_SIZE);
			}
			// Normal packet: 08 32 7C 1C 20 20 20 40 0A 00
			packet[0] = 0x08;
			//Throttle
			packet[1] = convert_channel_16b_limit(THROTTLE ,0,0x32)<<1; 	//00..64 but only even values
			//Rudder
			packet[2] = (0xFF-convert_channel_8b(RUDDER))&0xF8;				//F8..00 -> 5 bits
			//Elevator
			uint8_t ch = convert_channel_8b(ELEVATOR)>>3;
			packet[2] |= ch>>2;												//00..07 -> 3 bits high
			packet[3]  = ch<<6;												//00,40,80,C0 -> 2 bits low
			//Aileron
			packet[3] |= ((0xFF-convert_channel_8b(AILERON))>>3)<<1;		//5 bits

			//Trim Rudder 0x00..0x20..0x3F
			packet[4] = convert_channel_8b(CH11)>>2;
			//Trim Elevator 0x00..0x20..0x3F
			packet[5] = convert_channel_8b(CH12)>>2;
		}
	}
	
	// Flags + Trim Aileron
	//packet[6]
	// 0x20 -> 0x60 short press photo/video => |0x40 -> momentary
	// 0x20 -> 0xA0 long press photo/video  => |0x80 -> toggle
	// 0xA0 -> 0xE0 short press photo/video => |0x40 -> momentary
	// 0x20 -> 0x00..0x20..0x3F Trim Aileron
	packet[6] = (convert_channel_8b(CH13)>>2)
			  | GET_FLAG(CH9_SW,XERALL_FLAG_PHOTO)
			  | GET_FLAG(CH10_SW,XERALL_FLAG_VIDEO);

	// Flags
	// 0x40 -> 0x44 Heading hold mode => |0x04 -> toggle
	// 0x40 -> 0xC0 High/low speed => |0x80 -> toggle
	// 0x40 -> 0x42 One button takeoff/landing/emergency => |0x02 -> toggle
	// 0x40 -> 0x60 Flight/Ground => |0x20 -> toggle
	packet[7] = 0x40
			  | GET_FLAG(CH5_SW,XERALL_FLAG_FLIGHT_GROUND)
			  | GET_FLAG(CH6_SW,XERALL_FLAG_ONE_BUTTON)
			  | GET_FLAG(CH7_SW,XERALL_FLAG_RATE)
			  | GET_FLAG(CH8_SW,XERALL_FLAG_HEADING_HOLD);

	// CRC
	uint8_t sum = 0;
	for(uint8_t i=1;i<8;i++)
		sum += packet[i];
	packet[8] = sum & 0x0F;

	//0x00 -> 0x1A on first telemetry packet received
	//packet[9] = 0x00;
	
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TXRX_OFF);
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, XERALL_PACKET_SIZE, 0);
	#if 0
		debug("H:%d,P:",hopping_frequency_no);
		for(uint8_t i=0; i<XERALL_PACKET_SIZE; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) XERALL_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
}

static void __attribute__((unused)) XERALL_initialize_txid()
{
	rx_tx_addr[0] = rx_tx_addr[3];
	#ifdef XERALL_ORIGINAL_ID
		// Pascal
		if(RX_num)
		{
			rx_tx_addr[0]=0x56;
			rx_tx_addr[1]=0x06;
			rx_tx_addr[2]=0x23;
			rx_tx_addr[3]=0x00;
			rx_tx_addr[4]=0x13;
		}
		else
		{
			// Alfons
			rx_tx_addr[0]=0xF9;
			rx_tx_addr[1]=0x58;
			rx_tx_addr[2]=0x31;
			rx_tx_addr[3]=0x00;
			rx_tx_addr[4]=0x13;
		}
	#endif
	rx_tx_addr[3] = 0x00;
	rx_tx_addr[4] = 0x13;
	hopping_frequency[0] = 56;	// 0x38
	hopping_frequency[1] = 46;	// 0x2E
	hopping_frequency[2] = 61;	// 0x3D
	hopping_frequency[3] = 51;	// 0x33
}

#define XERALL_WRITE_WAIT 600
#define XERALL_CHECK_WAIT 300
uint16_t XERALL_callback()
{
	static uint8_t wait = 0;
	switch(phase)
	{
		case XERALL_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(XERALL_PACKET_PERIOD);
			#endif
			if (bind_counter == 0)
				BIND_DONE;
			else
				bind_counter--;
			XERALL_send_packet();
			phase++;
			return XERALL_WRITE_WAIT;
		case XERALL_RX:
			// switch to RX mode
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase++;
			return XERALL_PACKET_PERIOD-XERALL_WRITE_WAIT-XERALL_CHECK_WAIT;
		case XERALL_CHECK:
			if( XN297_IsRX() )
			{ // RX fifo data ready
				uint8_t len = XN297_ReadEnhancedPayload(packet_in, XERALL_PACKET_SIZE);
				if(len != 255) // CRC OK
				{
					#if 0
						debug("RX(%d):",len);
						for(uint8_t i=0; i<len; i++)
							debug("%02X ", packet_in[i]);
						debugln();
					#endif
					if(len == XERALL_PACKET_SIZE && packet_in[0] == 0x11)
					{ // Request for ack packet
						// Ack the telem packet
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(TX_EN);
						XN297_WriteEnhancedPayload(packet, 0, 0);

						packet[9] = packet_in[9];
						if(packet[0] == 0x01)				// Last packet sent was a bind packet
						{// Build bind response
							packet[0] = 0x02;
							for(uint8_t i=1; i<5; i++)
								packet[i] = packet_in[i];	// Tank ID???
							bind_phase = 14;
							XN297_SetTXAddr(rx_tx_addr, 5);
						}
						wait = 0;
						phase = XERALL_DATA;
						break;
					}
					else if(len == XERALL_PACKET_SIZE && packet_in[0] == 0x12)
					{
						BIND_DONE;
						bind_phase = 0;
						wait = 0;
					}
					else if(len == 0)
						wait = 5;							// The quad wants to talk let's pause sending data...
				}
				if(wait)
				{ // switch to RX mode
					XN297_SetTxRxMode(TXRX_OFF);
					XN297_SetTxRxMode(RX_EN);
				}
			}
			if(wait)
			{
				wait--;
				break;
			}
			phase = XERALL_DATA;
			return XERALL_CHECK_WAIT;
	}
	return XERALL_PACKET_PERIOD;
}

void XERALL_init(void)
{
	XERALL_initialize_txid();

	XERALL_RF_init();

	if(IS_BIND_IN_PROGRESS)
		bind_counter = XERALL_BIND_COUNT;
	hopping_frequency_no=0;
	packet_sent = 0;
	bind_phase = 0;
	memset(packet, 0, XERALL_PACKET_SIZE);
	phase = XERALL_DATA;
}

#endif
