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
#if defined(MOULDKG_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_MOULDKG_ORIGINAL_ID

#define MOULDKG_PACKET_PERIOD		5000
#define MOULDKG_BIND_PACKET_PERIOD	12000
#define MOULDKG_TX_BIND_CHANNEL		11
#define MOULDKG_RX_BIND_CHANNEL		76
#define MOULDKG_PAYLOAD_SIZE_DIGIT	5
#define MOULDKG_PAYLOAD_SIZE_ANALOG	10
#define MOULDKG_BIND_PAYLOAD_SIZE	7
#define MOULDKG_BIND_COUNT			300
#define MOULDKG_RF_NUM_CHANNELS		4

enum {
	MOULDKG_BINDTX=0,
	MOULDKG_BINDRX,
	MOULDKG_PREP_DATA,
	MOULDKG_PREP_DATA1,
	MOULDKG_DATA,
};

uint8_t MOULDKG_RX_id[4*3];

static void __attribute__((unused)) MOULDKG_send_packet()
{
	uint8_t len = MOULDKG_BIND_PAYLOAD_SIZE;
	memcpy(&packet[1],rx_tx_addr,3);
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xC0;
		memset(&packet[4], 0x00, 3);
	}
	else
	{
		uint8_t n = num_ch<<2;
		if(sub_protocol == MOULDKG_ANALOG)
		{
			packet[0] = 0x36;
			uint8_t ch[]={ 1,0,2,3,4,5 };
			for(uint8_t i=0;i<6;i++)
				packet[i+4] = convert_channel_8b(ch[i]+n);
			len = MOULDKG_PAYLOAD_SIZE_ANALOG;
		}
		else
		{//DIGIT
			len = MOULDKG_PAYLOAD_SIZE_DIGIT;
			uint8_t val=0;
			if(packet_count&1)
			{
				packet[0] = 0x31;
				//Button B
				if(Channel_data[CH2+n]>CHANNEL_MAX_COMMAND) val |= 0x40;
				else if(Channel_data[CH2+n]<CHANNEL_MIN_COMMAND) val |= 0x80;
				//Button C
				if(Channel_data[CH3+n]>CHANNEL_MAX_COMMAND) val |= 0x10;
				else if(Channel_data[CH3+n]<CHANNEL_MIN_COMMAND) val |= 0x20;
			}
			else
			{
				packet[0] = 0x30;
				val = 0x60;
				//	| GET_FLAG(CH5_SW, 0x80)	// Button E
				//	| GET_FLAG(CH6_SW, 0x10);	// Button F
			}
			//Button A
			if(Channel_data[CH1+n]>CHANNEL_MAX_COMMAND) val |= 0x01;
			else if(Channel_data[CH1+n]<CHANNEL_MIN_COMMAND) val |= 0x02;
			//Button D
			if(Channel_data[CH4+n]>CHANNEL_MAX_COMMAND) val |= 0x04;
			else if(Channel_data[CH4+n]<CHANNEL_MIN_COMMAND) val |= 0x08;
			packet[4]= val;
		}
	}

	// Send
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, len);
	#if 0
		for(uint8_t i=0; i < len; i++)
			debug("%02X ", packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) MOULDKG_initialize_txid()
{
	//rx_tx_addr[0] = rx_tx_addr[3];	// Use RX_num;

	#ifdef FORCE_MOULDKG_ORIGINAL_ID
		rx_tx_addr[0]=0x57;
		rx_tx_addr[1]=0x1B;
		rx_tx_addr[2]=0xF8;
	#endif
}

static void __attribute__((unused)) MOULDKG_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"KDH", 3);
	XN297_SetRXAddr((uint8_t*)"KDH", MOULDKG_BIND_PAYLOAD_SIZE);
}

uint16_t MOULDKG_callback()
{
	uint8_t rf,n;
	uint16_t addr;
	switch(phase)
	{
		case MOULDKG_BINDTX:
			if(XN297_IsRX())
			{
				//Example:	TX: C=11 S=Y A= 4B 44 48 P(7)= C0 37 02 4F 00 00 00
				//			RX: C=76 S=Y A= 4B 44 48 P(7)= 5A 37 02 4F 03 0D 8E
				// 03 0D 8E => RF channels 0F,1C,39,3C
				XN297_ReadPayload(packet_in, MOULDKG_BIND_PAYLOAD_SIZE);
				for(uint8_t i=0; i < MOULDKG_BIND_PAYLOAD_SIZE; i++)
					debug("%02X ", packet_in[i]);
				debugln();
				//Not sure if I should test packet_in[0]
				if(memcmp(&packet_in[1],&packet[1],3)==0)
				{//TX ID match
					if(option == 0)
					{
						memcpy(MOULDKG_RX_id,&packet_in[4],3);
						phase = MOULDKG_PREP_DATA1;
					}
					else
					{// Store RX ID
						addr=MOULDKG_EEPROM_OFFSET+RX_num*3;
						for(uint8_t i=0;i<3;i++)
							eeprom_write_byte((EE_ADDR)(addr+i),packet_in[4+i]);
						phase = MOULDKG_PREP_DATA;
					}
					break;
				}
			}
			XN297_RFChannel(MOULDKG_TX_BIND_CHANNEL);	// Set TX bind channel
			XN297_SetTxRxMode(TXRX_OFF);
			MOULDKG_send_packet();
			phase++;
			return 600;
		case MOULDKG_BINDRX:
			//Wait for the packet transmission to finish
			while(XN297_IsPacketSent()==false);
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_RFChannel(MOULDKG_RX_BIND_CHANNEL);	// Set RX bind channel
			XN297_SetTxRxMode(RX_EN);
			phase = MOULDKG_BINDTX;
			return MOULDKG_BIND_PACKET_PERIOD-600;
		case MOULDKG_PREP_DATA:
			addr=MOULDKG_EEPROM_OFFSET+RX_num*3;
			debug("RXID: ");
			for(uint8_t i=0;i<3*4;i++)
			{ // load 4 consecutive RX IDs
				MOULDKG_RX_id[i]=eeprom_read_byte((EE_ADDR)(addr+i));
				debug(" %02X",MOULDKG_RX_id[i]);
			}
			debugln("");
		case MOULDKG_PREP_DATA1:
			//Switch to normal mode
			BIND_DONE;
			XN297_SetTxRxMode(TXRX_OFF);
			phase = MOULDKG_DATA;
			break;
		case MOULDKG_DATA:
			#ifdef MULTI_SYNC
				if(num_ch==0)
					telemetry_set_input_sync(MOULDKG_PACKET_PERIOD);
			#endif
			if(option == 0) option++;
			if(num_ch<option)
			{
				//Set RX ID address
				n = num_ch*3;
				XN297_SetTXAddr(&MOULDKG_RX_id[n], 3);
				//Set frequency based on current RX ID and packet number
				rf = 0x0C;
				if(packet_count & 0x04)
				{
					n++;
					rf += 0x20;
				}
				if(packet_count & 0x02)
					rf += 0x10 + (MOULDKG_RX_id[n] >> 4);
				else
					rf += MOULDKG_RX_id[n] & 0x0F;
				XN297_RFChannel(rf);
				#if 1
					debugln("num_ch=%d,packet_count=%d,rf=%02X,ID=%02X %02X %02X",num_ch,packet_count,rf,MOULDKG_RX_id[num_ch*3],MOULDKG_RX_id[num_ch*3+1],MOULDKG_RX_id[num_ch*3+2]);
				#endif
				MOULDKG_send_packet();
				if(num_ch==0)
					packet_count++;
			}
			num_ch++;
			num_ch &= 0x03;
			break;
	}
	return MOULDKG_PACKET_PERIOD/4;
}

void MOULDKG_init()
{
	if(option == 0)
		BIND_IN_PROGRESS;
	MOULDKG_initialize_txid();
	MOULDKG_RF_init();
	bind_counter = MOULDKG_BIND_COUNT;
	if(IS_BIND_IN_PROGRESS)
		phase = MOULDKG_BINDTX;
	else
		phase = MOULDKG_PREP_DATA;
	packet_count = 0;
	num_ch = 0;
}

#endif

// Analog
// Bind TX: C=11 S=Y A= 4B 44 48 P(7)= C0 46 01 00 00 00 00
// Bind RX: 5A 46 01 00 63 82 4E
// Norm: C=15 S=Y A= 63 82 4E P(10)= 36 46 01 00 80 80 80 80 00 00