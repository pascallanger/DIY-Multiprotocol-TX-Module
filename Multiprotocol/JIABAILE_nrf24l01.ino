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
#if defined(JIABAILE_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_JIABAILE_ORIGINAL_ID

#define JIABAILE_PAYLOAD_SIZE			8
#define JIABAILE_RX_PAYLOAD_SIZE		7
#define JIABAILE_RF_NUM_CHANNELS		3
#define JIABAILE_BIND_PACKET_PERIOD		12700
#define JIABAILE_PACKET_PERIOD			2408
#define JIABAILE_BIND_COUNT				2000
#define JIABAILE_WRITE_TIME				1000

enum {
	JIABAILE_BIND=0,
	JIABAILE_RX,
	JIABAILE_DATA,
};

static uint8_t __attribute__((unused)) JIABAILE_channel(uint8_t num)
{
	uint8_t val=convert_channel_16b_limit(num,0,100);
	if(val>50)
	{
		packet[3] |= 0x01;
		return val-50;
	}
	if(val<50)
	{
		packet[3] |= 0x02;
		return 50-val;
	}
	return 0;
}

static void __attribute__((unused)) JIABAILE_send_packet()
{
	hopping_frequency_no++;
	if(hopping_frequency_no > 2)
		hopping_frequency_no = 0;
	XN297_Hopping(hopping_frequency_no);

	memcpy(packet,rx_tx_addr,3);
	memset(&packet[3], 0x00, 4);
	if(IS_BIND_DONE)
	{//Normal
		packet[4] = convert_channel_16b_limit(RUDDER,0,50)-25;	//ST Trim
		packet[6] = JIABAILE_channel(AILERON);					//ST
		packet[3] ^= 0x03;										//Reverse ST channel
		packet[3] <<= 2;										//Move ST channel where it should be
		packet[5] = JIABAILE_channel(ELEVATOR);					//TH
		packet[3] |= GET_FLAG(CH5_SW,  0x20)					//Low speed
					|GET_FLAG(CH7_SW,  0x40)					//Flash light
					|GET_FLAG(!CH6_SW,  0x80);					//Light
		if(!CH5_SW && Channel_data[CH5] > CHANNEL_MIN_COMMAND)
			packet[3] |= 0x10;									//Mid speed
	}
	packet[7] = 0x55 + hopping_frequency[hopping_frequency_no];
	for(uint8_t i=0;i<JIABAILE_PAYLOAD_SIZE-1;i++)
		packet[7] += packet[i];
	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, JIABAILE_PAYLOAD_SIZE);
	#ifdef ADEBUG_SERIAL
		debug("H%d RF%d",hopping_frequency_no,hopping_frequency[hopping_frequency_no]);
		for(uint8_t i=0; i < JIABAILE_PAYLOAD_SIZE; i++)
			debug(" %02X", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) JIABAILE_initialize_txid()
{
	rx_tx_addr[0] = rx_tx_addr[3];
	#ifdef FORCE_JIABAILE_ORIGINAL_ID
		memcpy(rx_tx_addr,(uint8_t *)"\xCB\x03\xA5",3);
		//memcpy(rx_tx_addr,(uint8_t *)"\x3D\x08\xA2",3);
		//Normal frequencies are calculated from the car ID...
		//memcpy(&hopping_frequency[3],(uint8_t *)"\x23\x2D\x4B",3);	//35,45,75
		memcpy(&hopping_frequency[3],(uint8_t *)"\x24\x43\x4C",3);	//36,67,76
	#endif
	//Bind frequencies
	memcpy(hopping_frequency,(uint8_t *)"\x07\x27\x45",3);	//7,39,69
}

static void __attribute__((unused)) JIABAILE_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	//Bind address
	memcpy(rx_id,(uint8_t*)"\xA7\x07\x57\xA7\x26", 5);
	XN297_SetTXAddr(rx_id, 5);
	XN297_SetRXAddr(rx_id, JIABAILE_RX_PAYLOAD_SIZE);
}

uint16_t JIABAILE_callback()
{
	switch(phase)
	{
		case JIABAILE_BIND:
			if(XN297_IsRX())
			{
				if(XN297_ReadPayload(packet_in, JIABAILE_RX_PAYLOAD_SIZE))
				{//CRC OK
					#ifdef DEBUG_SERIAL
						debug("RX");
						for(uint8_t i=0; i < JIABAILE_RX_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
						debugln();
					#endif
					//RX: CB 03 A5 9D 05 A2 68
					if(memcmp(packet_in,rx_tx_addr,3)==0)
					{//TX ID match
						//Check packet
						uint8_t sum=0xAA + hopping_frequency[hopping_frequency_no];
						for(uint8_t i=0; i < JIABAILE_RX_PAYLOAD_SIZE-1; i++)
							sum+=packet_in[i];
						if(sum==packet_in[6])
						{
							//Write the RXID in the address
							memcpy(&rx_id[1],&packet_in[3],3);
							XN297_SetTxRxMode(TXRX_OFF);
							XN297_SetTXAddr(rx_id, 5);
							//Set the normal frequencies
							sum=packet_in[3]&0x07;
							hopping_frequency[0] = (sum>4?30:8) + sum;
							if(sum==4 || sum ==7)
								hopping_frequency[0]++;
							hopping_frequency[1] = 40 + sum;
							if((sum & 0x06) == 0x06)
								hopping_frequency[1] += 21;
							hopping_frequency[2] = 70 + sum;
							#ifdef DEBUG_SERIAL
								debug("RF");
								for(uint8_t i=0; i < 3; i++)
									debug(" %d", hopping_frequency[i]);
								debugln();
							#endif
							//Switch to normal mode
							BIND_DONE;
							phase = JIABAILE_DATA;
						}
			#ifdef DEBUG_SERIAL
						else
							debug("Wrong Sum");
					}
					else
						debug("Wrong TX ID");
				}
				else
					debug("Bad CRC");
				debugln("");
			#else
					}
				}
			#endif
			}
			XN297_SetTxRxMode(TXRX_OFF);
			JIABAILE_send_packet();
			phase++;
			return JIABAILE_WRITE_TIME;
		case JIABAILE_RX:
			//Wait for the packet transmission to finish
			while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase = JIABAILE_BIND;
			return JIABAILE_BIND_PACKET_PERIOD - JIABAILE_WRITE_TIME;
		default:	//JIABAILE_DATA
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(JIABAILE_PACKET_PERIOD);
			#endif
			JIABAILE_send_packet();
			break;
	}
	return JIABAILE_PACKET_PERIOD;
}

void JIABAILE_init()
{
	BIND_IN_PROGRESS;	// Autobind protocol
	JIABAILE_initialize_txid();
	JIABAILE_RF_init();
	phase = JIABAILE_BIND;
	hopping_frequency_no = 0;
}

#endif
/*
// CAR RX debug code

static void __attribute__((unused)) JIABAILE_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	//Bind address
	memcpy(rx_id,(uint8_t*)"\xA7\x07\x57\xA7\x26", 5);
	XN297_SetTXAddr(rx_id, 5);
	XN297_SetRXAddr(rx_id, JIABAILE_PAYLOAD_SIZE);
	XN297_RFChannel(7);
	rx_tx_addr[0] = 0x00;
	rx_tx_addr[1] = RX_num;
	rx_tx_addr[2] = 0x00;
}

uint16_t JIABAILE_callback()
{
	switch(phase)
	{
		case JIABAILE_BIND:
			if(XN297_IsRX())
			{
				if(XN297_ReadPayload(packet_in, JIABAILE_PAYLOAD_SIZE))
				{//CRC OK
					XN297_SetTxRxMode(TXRX_OFF);
					#ifdef DEBUG_SERIAL
						debug("RX Bind");
						for(uint8_t i=0; i < JIABAILE_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
						debugln();
					#endif
					memcpy(packet,packet_in,3);
					memcpy(&packet[3],rx_tx_addr,3);
					packet[6]=0xAA + 7;
					for(uint8_t i=0; i < JIABAILE_RX_PAYLOAD_SIZE-1; i++)
						packet[6]+=packet[i];
					XN297_SetTxRxMode(TX_EN);
					memcpy(&rx_id[1],rx_tx_addr,3);
					bind_counter = 10;
					phase = JIABAILE_RX;
				}
			}
			return JIABAILE_WRITE_TIME;
		case JIABAILE_RX:
			if(bind_counter)
			{
				bind_counter--;
				XN297_WritePayload(packet, JIABAILE_RX_PAYLOAD_SIZE);
				#ifdef DEBUG_SERIAL
					debug("TX Bind");
					for(uint8_t i=0; i < JIABAILE_RX_PAYLOAD_SIZE; i++)
						debug(" %02X", packet[i]);
					debugln();
				#endif
				return JIABAILE_PACKET_PERIOD;
			}
			//Wait for the packet transmission to finish
			//while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetRXAddr(rx_id, JIABAILE_PAYLOAD_SIZE);
			hopping_frequency_no++;
			if(hopping_frequency_no>84)
				hopping_frequency_no = 0;
			debug(".");
			XN297_RFChannel(hopping_frequency_no);
			XN297_SetTxRxMode(RX_EN);
			phase = JIABAILE_DATA;
			return JIABAILE_BIND_PACKET_PERIOD;
		case JIABAILE_DATA:
			if(XN297_IsRX())
			{
				debugln("");
				if(XN297_ReadPayload(packet_in, JIABAILE_PAYLOAD_SIZE))
				{//CRC OK
					#ifdef DEBUG_SERIAL
						debug("CH=%d=%02X P:",hopping_frequency_no,hopping_frequency_no);
						for(uint8_t i=0; i < JIABAILE_PAYLOAD_SIZE; i++)
							debug(" %02X", packet_in[i]);
					#endif
					//Check packet
					uint8_t sum=0x55 + hopping_frequency_no;
					for(uint8_t i=0; i < JIABAILE_PAYLOAD_SIZE-1; i++)
						sum+=packet_in[i];
					if(sum==packet_in[7])
					{
						debug("Good channel");
					}
					else
						debug("Wrong Sum");
				}
				else
					debug("Bad CRC");
				debugln("");
			}
			phase = JIABAILE_RX;
			break;
	}
	return JIABAILE_PACKET_PERIOD;
}

void JIABAILE_init()
{
	JIABAILE_RF_init();
	XN297_SetTxRxMode(TXRX_OFF);
	XN297_SetTxRxMode(RX_EN);
	phase = JIABAILE_BIND;
	hopping_frequency_no = 7;
}
*/
