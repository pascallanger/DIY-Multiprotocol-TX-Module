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
// compatible with MT99xx, Eachine H7, Yi Zhan i6S and LS114/124
// Last sync with Goebish mt99xx_nrf24l01.c dated 2016-01-29

#if defined(MT99XX_CCNRF_INO)

#include "iface_xn297.h"

#define MT99XX_BIND_COUNT			928
#define MT99XX_PACKET_PERIOD_FY805	2460
#define MT99XX_PACKET_PERIOD_MT		2625
#define MT99XX_PACKET_PERIOD_YZ		3125
#define MT99XX_PACKET_PERIOD_A180	3400	// timing changes between the packets 2 x 27220 then 1x 26080, it seems that it is only on the first RF channel which jitters by 1.14ms but hard to pinpoint with XN297dump
#define MT99XX_PACKET_PERIOD_DRAGON	1038	// there is a pause of 2x1038 between two packets, no idea why and how since it is not even stable on a same dump...
#define MT99XX_PACKET_PERIOD_DRAGON_TELEM	10265	// long pause to receive the telemetry packets, 3 are sent by the RX one after the other
#define MT99XX_PACKET_PERIOD_F949G	3450
#define MT99XX_PACKET_PERIOD_PA18	1338
#define MT99XX_INITIAL_WAIT			500
#define MT99XX_PACKET_SIZE			9

//#define FORCE_A180_ID
//#define FORCE_DRAGON_ID
//#define FORCE_F949G_ID
#define FORCE_PA18_ID1
//#define FORCE_PA18_ID2

enum {
    MT99XX_DATA,
    MT99XX_RX,
    MT99XX_CHECK,
};

enum{
    // flags going to packet[6] (MT99xx, H7)
    FLAG_MT_RATE1   = 0x01, // (H7 & A180 high rate)
    FLAG_MT_RATE2   = 0x02, // (MT9916 only)
    FLAG_MT_VIDEO   = 0x10,
    FLAG_MT_SNAPSHOT= 0x20,
    FLAG_MT_FLIP    = 0x80,
};

enum{
    // flags going to packet[6] (LS)
    FLAG_LS_INVERT  = 0x01,
    FLAG_LS_RATE    = 0x02,
    FLAG_LS_HEADLESS= 0x10,
    FLAG_LS_SNAPSHOT= 0x20,
    FLAG_LS_VIDEO   = 0x40,
    FLAG_LS_FLIP    = 0x80,
};

enum{
    // flags going to packet[7] (FY805)
    FLAG_FY805_HEADLESS= 0x10,
};

enum{
    // flags going to packet[6] (A180)
    FLAG_A180_3D6G	= 0x01,
    FLAG_A180_RATE	= 0x02,
};

enum{
    // flags going to packet[6] (DRAGON)
    FLAG_DRAGON_RATE	= 0x01,
    FLAG_DRAGON_RTH		= 0x80,
    FLAG_DRAGON_UNK		= 0x04,
};

enum{
    // flags going to packet[6] (F949G)
    FLAG_F949G_LIGHT	= 0x01,
    FLAG_F949G_3D6G		= 0x20,
};
enum{
    // flags going to packet[6] (PA18)
	FLAG_PA18_RTH		= 0x08,
	FLAG_PA18_FLIP		= 0x80,
};

const uint8_t h7_mys_byte[] = {
	0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 
	0x05, 0x15, 0x06, 0x16, 0x07, 0x17, 0x00, 0x10
};

const uint8_t ls_mys_byte[] = {
	0x05, 0x15, 0x25, 0x06, 0x16, 0x26,
	0x07, 0x17, 0x27, 0x00, 0x10, 0x20,
	0x01, 0x11, 0x21, 0x02, 0x12, 0x22,
	0x03, 0x13, 0x23, 0x04, 0x14, 0x24
};

const uint8_t yz_p4_seq[] = {0xa0, 0x20, 0x60};

#ifdef MT99XX_HUB_TELEMETRY
	const uint8_t DRAGON_seq[] = {0x20, 0x60, 0x20, 0x80};
#endif

static void __attribute__((unused)) MT99XX_send_packet()
{
	static uint8_t seq_num=0;

	//Set RF freq
	if(sub_protocol == LS)
		XN297_RFChannel(0x2D); 			// LS always transmits on the same channel
	else
		if(sub_protocol==FY805)
			XN297_RFChannel(0x4B);		// FY805 always transmits on the same channel
		else // MT99 & H7 & YZ & A180 & DRAGON & F949G & PA18
			XN297_Hopping(hopping_frequency_no);

	if(IS_BIND_IN_PROGRESS)
	{
		//Bind packet
		packet[0] = 0x20;
		switch(sub_protocol)
		{
			case YZ:
				packet[1] = 0x15;
				packet[2] = 0x05;
				packet[3] = 0x06;
				break;
			case LS:
				packet[1] = 0x14;
				packet[2] = 0x05;
				packet[3] = 0x11;
				break;
			case FY805:
				packet[1] = 0x15;
				packet[2] = 0x12;
				packet[3] = 0x17;
				break;
			default:	// MT99 & H7 & A180 & DRAGON & F949G & PA18
				packet[1] = 0x14;
				packet[2] = 0x03;
				packet[3] = 0x25;
				break;
		}
		packet[4] = rx_tx_addr[0];
		packet[5] = rx_tx_addr[1];
		packet[6] = rx_tx_addr[2];
		if(sub_protocol == PA18+8)
		{
			packet[7] = num_ch;											// checksum offset
			packet[8] = 0x55;											// fixed
		}
		else
		{ // all others
			packet[7] = crc8;											// checksum offset
			if(sub_protocol != F949G)
				packet[8] = 0xAA;										// fixed
			else
				packet[8] = 0x00;
		}
	}
	else
	{
		if(sub_protocol != YZ)
		{ // MT99XX & H7 & LS & FY805 & A180 & DRAGON & F949G & PA18
			packet[0] = convert_channel_16b_limit(THROTTLE,0xE1,0x00);	// throttle
			packet[1] = convert_channel_16b_limit(RUDDER  ,0x00,0xE1);	// rudder
			packet[2] = convert_channel_16b_limit(AILERON ,0xE1,0x00);	// aileron
			packet[3] = convert_channel_16b_limit(ELEVATOR,0x00,0xE1);	// elevator
			packet[4] = 0x20; 											// pitch trim (0x3f-0x20-0x00)
			packet[5] = 0x20; 											// roll trim (0x00-0x20-0x3f)
			packet[6] = GET_FLAG( CH5_SW, FLAG_MT_FLIP );
			if(sub_protocol != PA18+8)
				packet[7] = h7_mys_byte[hopping_frequency_no];			// next rf channel index ?
			else
				packet[7] = (packet[7]&0xBF)|0x20;
			switch(sub_protocol)
			{
				case MT99:
					packet[6] |= 0x40 | FLAG_MT_RATE2					// max rate on MT99xx
					  | GET_FLAG( CH7_SW, FLAG_MT_SNAPSHOT )
					  | GET_FLAG( CH8_SW, FLAG_MT_VIDEO );
					break;
				case H7:
					packet[6] |= FLAG_MT_RATE1; 						// max rate on H7
					break;
				case LS:
					packet[6] |= FLAG_LS_RATE							// max rate
						| GET_FLAG( CH6_SW, FLAG_LS_INVERT )			// invert
						| GET_FLAG( CH7_SW, FLAG_LS_SNAPSHOT )			// snapshot
						| GET_FLAG( CH8_SW, FLAG_LS_VIDEO )				// video
						| GET_FLAG( CH9_SW, FLAG_LS_HEADLESS );			// Headless
					packet[7] = ls_mys_byte[seq_num++];
					if(seq_num >= sizeof(ls_mys_byte))
						seq_num=0;
					break;
				case FY805:
					packet[6]=0x20;
					//Rate 0x01?
					//Flip ?
					packet[7]=0x01
						|GET_FLAG( CH5_SW, FLAG_MT_FLIP )
						|GET_FLAG( CH9_SW, FLAG_FY805_HEADLESS );		//HEADLESS
					crc8=0;
					break;
				case A180:
					packet[6] = GET_FLAG( !CH6_SW,FLAG_A180_RATE)		// 0x02, A180=RATE,  F949S=LED
							   |GET_FLAG( CH5_SW, FLAG_A180_3D6G )		// 0x01, A180=3D_6G, F949S=RATE
							   |GET_FLAG( CH7_SW, 0x20 );				// 0x20, F949S=3D_6G
					packet[7] = 0x00;
					break;
				case DRAGON:
					if(CH5_SW)											// Advanced mode
						packet[5] |= 0x80;
					else
						if(Channel_data[CH5] > CHANNEL_MIN_COMMAND)		// Beginner mode
							packet[5] |= 0x40;
					packet[6] = FLAG_DRAGON_RATE
						| GET_FLAG( CH6_SW, FLAG_DRAGON_RTH );
					
					#ifdef MT99XX_HUB_TELEMETRY
						//Telemetry
						if(hopping_frequency_no == 0)
						{
							seq_num++;
							seq_num &= 0x03;
							packet_count++;
							if(packet_count > 11)
								packet_count = 0;
						}
						if(packet_count > 10)							// Telemetry packet request every 10 or 11 packets
						{
							packet[6] |= 0x04;							// Request telemetry flag
							phase = MT99XX_RX;
						}
						packet[7] = DRAGON_seq[seq_num];				// seq: 20 80 20 60 20 80 20 60... 80 changes to 80+batt from telem
						if(seq_num==3)
							packet[7] |= v_lipo1;
					#else
						packet[7] = 0x20;
					#endif
					break;
				case F949G:
					packet[6] = 0x02
							  | GET_FLAG( CH5_SW, FLAG_F949G_3D6G )
							  | GET_FLAG( CH6_SW, FLAG_F949G_LIGHT );
					packet[7] = 0x00;
					break;
				case PA18+8:
					if(Channel_data[CH5] > CHANNEL_MAX_COMMAND)			// Expert mode
						packet[5] = 0xA0;
					else
						if(Channel_data[CH5] > CHANNEL_MIN_COMMAND)		// Intermediate mode
							packet[5] = 0x60;
					packet[6] = GET_FLAG( CH6_SW, FLAG_PA18_FLIP )		// Flip
							  | GET_FLAG( CH7_SW, FLAG_PA18_RTH );		// RTH
					if(hopping_frequency_no == 0)
						packet[7] ^= 0x40;
					break;
			}
			uint8_t result=crc8;
			for(uint8_t i=0; i<8; i++)
				result += packet[i];
			if(sub_protocol == PA18+8)
				result += num_ch;
			packet[8] = result;
		}
		else
		{ // YZ
			packet[0] = convert_channel_16b_limit(THROTTLE,0x00,0x64);		// throttle
			packet[1] = convert_channel_16b_limit(RUDDER  ,0x64,0x00);		// rudder
			packet[2] = convert_channel_16b_limit(ELEVATOR,0x00,0x64);		// elevator
			packet[3] = convert_channel_16b_limit(AILERON ,0x64,0x00);		// aileron
			if(packet_count++ >= 23)
			{
				seq_num ++;
				if(seq_num > 2)
					seq_num = 0;
				packet_count=0;
			}
			packet[4] = yz_p4_seq[seq_num];
			packet[5] = 0x02 // expert ? (0=unarmed, 1=normal)
						| GET_FLAG(CH8_SW, 0x10)							//VIDEO
						| GET_FLAG(CH5_SW, 0x80)							//FLIP
						| GET_FLAG(CH9_SW, 0x04)							//HEADLESS
						| GET_FLAG(CH7_SW, 0x20);							//SNAPSHOT
			packet[6] =   GET_FLAG(CH6_SW, 0x80);							//LED
			packet[7] = packet[0];            
			for(uint8_t idx = 1; idx < MT99XX_PACKET_SIZE-2; idx++)
				packet[7] += packet[idx];
			packet[8] = 0xff;
		}
	}

	//Hopp
	hopping_frequency_no++;
	if(sub_protocol == YZ || sub_protocol == A180 || sub_protocol == DRAGON || sub_protocol == F949G || sub_protocol == PA18+8)
		hopping_frequency_no++; // skip every other channel
	if(hopping_frequency_no > 15)
		hopping_frequency_no = 0;

	// Send
	XN297_SetPower();
	XN297_SetFreqOffset();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, MT99XX_PACKET_SIZE);
	
	#if 0
		for(uint8_t i=0; i<MT99XX_PACKET_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) MT99XX_RF_init()
{
    if(sub_protocol == YZ)
		XN297_Configure(XN297_CRCEN, XN297_UNSCRAMBLED, XN297_250K);
	else if(sub_protocol == F949G)
		XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	else
		XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
    XN297_SetTXAddr((uint8_t *)"\xCC\xCC\xCC\xCC\xCC", 5);
	XN297_HoppingCalib(16);
	#ifdef MT99XX_HUB_TELEMETRY
		XN297_SetRXAddr(rx_tx_addr, MT99XX_PACKET_SIZE);
	#endif

}

static void __attribute__((unused)) MT99XX_initialize_txid()
{
	num_ch = rx_tx_addr[1];			// PA18
	rx_tx_addr[1] = rx_tx_addr[3];	// RX_Num
	switch(sub_protocol)
	{
		case YZ:
			rx_tx_addr[0] = 0x53;	// test (SB id)
			rx_tx_addr[1] = 0x00;
			rx_tx_addr[2] = 0x00;
			break;
		case FY805:
			rx_tx_addr[0] = 0x81;	// test (SB id)
			rx_tx_addr[1] = 0x0F;
			rx_tx_addr[2] = 0x00;
			break;
		case LS:
			rx_tx_addr[0] = 0xCC;
			break;
	#ifdef FORCE_A180_ID
		case A180:
			rx_tx_addr[0] = 0x84;	// MikeHRC ID
			rx_tx_addr[1] = 0x62;
			rx_tx_addr[2] = 0x4A;
			//crc8 = 0x30
			//channel_offset  = 0x03;
			break;
	#endif
	#ifdef FORCE_DRAGON_ID
		case DRAGON:
			rx_tx_addr[0] = 0x6C;	// Laurie ID
			rx_tx_addr[1] = 0x00;
			rx_tx_addr[2] = 0x22;
			//crc8 = 0x8E
			//channel_offset  = 0x06
			break;
	#endif
	#ifdef FORCE_F949G_ID
		case F949G:
			rx_tx_addr[0] = 0x7E;	// LilTeo14 ID
			rx_tx_addr[1] = 0x2F;
			rx_tx_addr[2] = 0x29;
			//crc8 = 0xD6
			//channel_offset  = 0x03
			break;
	#endif
	#ifdef FORCE_PA18_ID1
		case PA18+8:
			rx_tx_addr[0] = 0xC9;	// zebble ID
			rx_tx_addr[1] = 0x02;
			rx_tx_addr[2] = 0x13;
			num_ch = 0x89;			// additional crc init. How is this calculated? or could it be random?
			//crc8 = 0xDE
			//channel_offset  = 0x03
			//1Mb C=5 S=Y A= C9 02 13 CC CC P(9)= E1 70 70 70 20 20 00 20 1A -> 0x91 + 0x89 => 0x1A
			//        S=Y A= C9 02 13 CC CC P(9)= E1 70 70 70 20 A0 00 20 9A -> 0x11 + 0x89 => 0x9A
			//bind    S=Y A= CC CC CC CC CC P(9)= 20 14 03 25 C9 02 13 89 55
			break;
	#endif
	#ifdef FORCE_PA18_ID2
		case PA18+8:
			rx_tx_addr[0] = 0x0E;
			rx_tx_addr[1] = 0x05;
			rx_tx_addr[2] = 0x13;
			num_ch = 0xD1;			// additional crc init. How is this calculated? or could it be random?
			//crc8 = 0x28
			//channel_offset  = 0x00
			//1Mb C=2 S=Y A= 0E 05 13 CC CC P(9)= E1 70 70 70 20 60 00 60 E2 -> 0x11 + 0xD1 => 0xE2
			//bind    S=Y A= CC CC CC CC CC P(9)= 20 14 03 25 0E 05 13 D1 55
			break;
	#endif
		default: //MT99 & H7 & A180 & DRAGON & F949G & PA18
			rx_tx_addr[2] = 0x00;
			if(sub_protocol == PA18+8)
				rx_tx_addr[2] = 0x13;
			break;
	}
	
	rx_tx_addr[3] = 0xCC;
	rx_tx_addr[4] = 0xCC;

	crc8 = rx_tx_addr[0] + rx_tx_addr[1] + rx_tx_addr[2];

	//memcpy(hopping_frequency,"\x02\x48\x0C\x3e\x16\x34\x20\x2A\x2A\x20\x34\x16\x3e\x0c\x48\x02",16);
	for(uint8_t i=0; i<8; i++)
	{
		hopping_frequency[(i<<1)  ]=0x02 + (10*i);
		hopping_frequency[(i<<1)+1]=0x48 - (10*i);
	}
	hopping_frequency_no=0;
}

uint16_t MT99XX_callback()
{
	switch(phase)
	{
		case MT99XX_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(packet_period);
			#endif
			if(bind_counter)
			{
				bind_counter--;
				if (bind_counter == 0)
				{
					// set tx address for data packets
					XN297_SetTXAddr(rx_tx_addr, 5);
					// set rf channels
					uint8_t channel_offset = ((crc8>>4) + (crc8 & 0x0f)) % 8;
					for(uint8_t i=0;i<16;i++)
						hopping_frequency[i] += channel_offset;
					XN297_HoppingCalib(16);
					BIND_DONE;
				}
			}
			MT99XX_send_packet();
			break;
		#ifdef MT99XX_HUB_TELEMETRY
		case MT99XX_RX:
			//Switch to RX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase++;
			return MT99XX_PACKET_PERIOD_DRAGON_TELEM - MT99XX_PACKET_PERIOD_DRAGON - 500;
		case MT99XX_CHECK:
			//Check telem
			if(XN297_IsRX())
			{
				//debug("RX");
				if(XN297_ReadPayload(packet_in, MT99XX_PACKET_SIZE))
				{
					// C=48 S=Y A= 6C 00 22 CC CC P(9)= 6C 00 22 27 00 00 00 00 60
					// C=48 S=Y A= 6C 00 22 CC CC P(9)= 6C 00 22 28 00 00 00 00 61
					// C=18 S=Y A= 6C 00 22 CC CC P(9)= 6C 00 22 24 00 00 00 00 5D
					// 6C 00 22 = TX address, 27/28/24=vbatt, check = sum(P[0..7]) + AB
					// D2 EE 00 25 00 00 00 00 90 -> check also + AB
					//for(uint8_t i=0; i<MT99XX_PACKET_SIZE; i++)
					//	debug(" %02X",packet_in[i]);
					uint8_t check=0xAB;
					for(uint8_t i=0; i<8; i++)
						check += packet_in[i];
					if(packet_in[8] == check && packet_in[0] == rx_tx_addr[0] && packet_in[1] == rx_tx_addr[1] && packet_in[2] == rx_tx_addr[2])
					{ // checksum and address are ok
					//	debug(" OK");
						v_lipo1 = packet_in[3] & 0x7F;	// Batt
						v_lipo2 = packet_in[3] & 0x80;	// Low batt flag
						RX_RSSI=100;
						telemetry_link = 1;
					}
				}
				//debugln("");
			}
			//Switch to TX
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(TX_EN);
			phase=MT99XX_DATA;
			return 500;
		#endif
	}
	return packet_period;
}

void MT99XX_init(void)
{
	if(protocol == PROTO_MT99XX2)
		sub_protocol|=0x08;		// Increase the number of sub_protocols for MT99XX

    bind_counter = MT99XX_BIND_COUNT;
	if(IS_BIND_DONE)
	{		
		if(sub_protocol != A180 && sub_protocol != DRAGON && sub_protocol != F949G && sub_protocol != PA18+8)
			BIND_IN_PROGRESS;	// autobind protocol
		else
		    bind_counter = 1;
	}

	MT99XX_initialize_txid();
	MT99XX_RF_init();

	switch(sub_protocol)
	{
		case YZ:
			packet_period = MT99XX_PACKET_PERIOD_YZ;
			break;
		case FY805:
			packet_period = MT99XX_PACKET_PERIOD_FY805;
			break;
		case F949G:
		case A180:
			packet_period = MT99XX_PACKET_PERIOD_A180;
			break;
		case PA18+8:
			packet_period = MT99XX_PACKET_PERIOD_PA18;
			break;
		default:
			packet_period = MT99XX_PACKET_PERIOD_MT;
			break;
	}

	packet_count=0;
	phase=MT99XX_DATA;
}
#endif
