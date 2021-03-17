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

#if defined(OMP_CCNRF_INO)

#include "iface_xn297.h"

//#define FORCE_OMP_ORIGINAL_ID
//#define OMP_TELEM_DEBUG

#define OMP_INITIAL_WAIT	500
#define OMP_PACKET_PERIOD	5000
#define OMP_RF_BIND_CHANNEL	35
#define OMP_RF_NUM_CHANNELS	8
#define OMP_PAYLOAD_SIZE	16
#define OMP_BIND_COUNT		600	//3sec

static void __attribute__((unused)) OMP_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		memcpy(packet,"BND",3);
		memcpy(&packet[3],rx_tx_addr,5);
		memcpy(&packet[8],hopping_frequency,8);
	}
	else
	{
		memset(packet,0x00,OMP_PAYLOAD_SIZE);

#ifdef OMP_HUB_TELEMETRY
		//RX telem request every 7*5=35ms
		packet_sent++;
		packet_sent %= OMP_RF_NUM_CHANNELS-1;			// Change telem RX channels every time
		if(packet_sent==0)
			packet[0] |= 0x40;							// |0x40 to request RX telemetry
#endif
		
		//hopping frequency
		packet[0 ] |= hopping_frequency_no;
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= OMP_RF_NUM_CHANNELS-1;	// 8 RF channels

		//flags
		packet[1 ] = 0x08								//unknown
				   | GET_FLAG(CH5_SW, 0x20);			// HOLD

		packet[2 ] = 0x40;								//unknown
		
		if(Channel_data[CH6] > CHANNEL_MAX_COMMAND)
			packet[2 ] |= 0x20;							// IDLE2
		else if(Channel_data[CH6] > CHANNEL_MIN_COMMAND)
			packet[1 ] |= 0x40;							// IDLE1

		if(Channel_data[CH7] > CHANNEL_MAX_COMMAND)
			packet[2 ] |= 0x08;							// 3D
		else if(Channel_data[CH7] > CHANNEL_MIN_COMMAND)
			packet[2 ] |= 0x04;							// ATTITUDE

		//trims??
		//packet[3..6]

		//channels TAER packed 11bits
		uint16_t channel=convert_channel_16b_limit(THROTTLE,0,2047);
		packet[7 ]  = channel;
		packet[8 ]  = channel>>8;
		channel=convert_channel_16b_limit(AILERON,2047,0);
		packet[8 ] |= channel<<3;
		packet[9 ]  = channel>>5;
		channel=convert_channel_16b_limit(ELEVATOR,0,2047);
		packet[9] |= channel<<6;
		packet[10]  = channel>>2;
		packet[11]  = channel>>10;
		channel=convert_channel_16b_limit(RUDDER,2047,0);
		packet[11] |= channel<<1;
		packet[12]  = channel>>7;

		//unknown
		//packet[13..15]
		packet[15] = 0x04;
	}

	XN297_SetPower();									// Set tx_power
	XN297_SetFreqOffset();								// Set frequency offset
	XN297_SetTxRxMode(TX_EN);
	XN297_WriteEnhancedPayload(packet, OMP_PAYLOAD_SIZE, packet_sent!=0);
}

static void __attribute__((unused)) OMP_RF_init()
{
	//Config CC2500
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	XN297_SetTXAddr((uint8_t*)"FLPBD", 5);
	XN297_HoppingCalib(OMP_RF_NUM_CHANNELS);	// Calibrate all channels
	XN297_RFChannel(OMP_RF_BIND_CHANNEL);		// Set bind channel

#ifdef OMP_HUB_TELEMETRY
	XN297_SetRXAddr(rx_tx_addr, OMP_PAYLOAD_SIZE);
#endif
}

static void __attribute__((unused)) OMP_initialize_txid()
{
	calc_fh_channels(OMP_RF_NUM_CHANNELS);
	#ifdef FORCE_OMP_ORIGINAL_ID
		rx_tx_addr[0]=0x4E;
		rx_tx_addr[1]=0x72;
		rx_tx_addr[2]=0x8E;
		rx_tx_addr[3]=0x70;
		rx_tx_addr[4]=0x62;
		for(uint8_t i=0; i<OMP_RF_NUM_CHANNELS;i++)
			hopping_frequency[i]=(i+3)*5;
	#endif
}

#ifdef OMP_HUB_TELEMETRY
	static void __attribute__((unused)) OMP_Send_Telemetry(uint8_t v)
	{
		v_lipo1=v;
		telemetry_counter++;	//LQI
		telemetry_link=1;
		if(telemetry_lost)
		{
			telemetry_lost = 0;
			packet_count = 100;
			telemetry_counter = 100;
		}
	}
#endif

enum {
	OMP_BIND		= 0x00,
	OMP_PREPDATA	= 0x01,
	OMP_DATA		= 0x02,
	OMP_RX			= 0x03,
};

#define OMP_WRITE_TIME 850

uint16_t OMP_callback()
{
	bool rx;
	
	switch(phase)
	{
		case OMP_BIND:
			if(--bind_counter==0)
				phase++;						// OMP_PREPDATA
			OMP_send_packet();
			return OMP_PACKET_PERIOD;
		case OMP_PREPDATA:
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 5);
			phase++;							// OMP_DATA
		case OMP_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(OMP_PACKET_PERIOD);
			#endif
			#ifdef OMP_HUB_TELEMETRY
				rx = XN297_IsRX();				// Needed for the NRF24L01 since otherwise the bit gets cleared
			#endif
			OMP_send_packet();
			#ifdef OMP_HUB_TELEMETRY
				if(packet_sent == 0)
				{
					phase++;					// OMP_RX
					return OMP_WRITE_TIME;
				}
				else if(packet_sent == 1)
				{
					if( rx )
					{ // a packet has been received
						#ifdef OMP_TELEM_DEBUG
							debug("RX :");
						#endif
						if(XN297_ReadEnhancedPayload(packet_in, OMP_PAYLOAD_SIZE) == OMP_PAYLOAD_SIZE)
						{ // packet with good CRC and length
							#ifdef OMP_TELEM_DEBUG
								debug("OK :");
								for(uint8_t i=0;i<OMP_PAYLOAD_SIZE;i++)
									debug(" %02X",packet_in[i]);
							#endif
							// packet_in = 01 00 98 2C 03 19 19 F0 49 02 00 00 00 00 00 00
							// all bytes are fixed and unknown except 2 and 3 which represent the battery voltage: packet_in[3]*256+packet_in[2]=lipo voltage*100 in V
							uint16_t v=((packet_in[3]<<8)+packet_in[2]-400)/50;
							if(v>255) v=255;
							v_lipo2=v;
							OMP_Send_Telemetry(v);
						}
						else
						{ // As soon as the motor spins the telem packets are becoming really bad and the CRC throws most of them in error as it should but...
							#ifdef OMP_TELEM_DEBUG
								debug("NOK:");
								for(uint8_t i=0;i<OMP_PAYLOAD_SIZE;i++)
										debug(" %02X",packet_in[i]);
							#endif
							if(packet_in[0]==0x01 && packet_in[1]==0x00)
							{// the start of the packet looks ok...
								uint16_t v=((packet_in[3]<<8)+packet_in[2]-400)/50;
								if(v<260 && v>180)
								{ //voltage is less than 13V and more than 9V (3V/element)
									if(v>255) v=255;
									uint16_t v1=v-v_lipo2;
									if(v1&0x8000) v1=-v1;
									if(v1<20) // the batt voltage is within 1V from a good reading...
									{
										OMP_Send_Telemetry(v);	// ok to send
										#ifdef OMP_TELEM_DEBUG
											debug(" OK");
										#endif
									}
								}
							}
							else
								telemetry_counter++;	//LQI
						}
						#ifdef OMP_TELEM_DEBUG
							debugln("");
						#endif
					}
					XN297_SetTxRxMode(TXRX_OFF);
					packet_count++;
					if(packet_count>=100)
					{//LQI calculation
						packet_count=0;
						TX_LQI=telemetry_counter;
						RX_RSSI=telemetry_counter;
						if(telemetry_counter==0)
							telemetry_lost = 1;
						telemetry_counter = 0;
					}
				}
			#endif
			return OMP_PACKET_PERIOD;
	#ifdef OMP_HUB_TELEMETRY
		case OMP_RX:
			{
				uint16_t start=(uint16_t)micros();
				while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)
				{
					if(XN297_IsPacketSent())
						break;
				}
			}
			XN297_SetTxRxMode(RX_EN);
			phase = OMP_DATA;
			return OMP_PACKET_PERIOD-OMP_WRITE_TIME;
	#endif
	}
	return OMP_PACKET_PERIOD;
}

void OMP_init()
{
	OMP_initialize_txid();
	OMP_RF_init();
	hopping_frequency_no = 0;
	packet_sent = 0;
	#ifdef OMP_HUB_TELEMETRY
		packet_count = 0;
		telemetry_lost = 1;
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = OMP_BIND_COUNT;
		phase = OMP_BIND;
	}
	else
		phase = OMP_PREPDATA;
}

#endif
