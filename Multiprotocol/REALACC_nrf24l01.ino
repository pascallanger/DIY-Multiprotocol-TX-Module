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
// Compatible with Realacc R11

#if defined(REALACC_NRF24L01_INO)

#include "iface_xn297.h"

//#define FORCE_REALACC_ORIGINAL_ID
//#define FORCE_REALACC_WLV8TX_ID

#define REALACC_INITIAL_WAIT		500
#define REALACC_PACKET_PERIOD		2268
#define REALACC_BIND_RF_CHANNEL		80
#define REALACC_BIND_PAYLOAD_SIZE	10
#define REALACC_PAYLOAD_SIZE		13
#define REALACC_BIND_COUNT			50
#define REALACC_RF_NUM_CHANNELS		5

#ifndef MULTI_AIR
#define WLV8TX_PACKET_PERIOD		16279
#define WLV8TX_BIND2_PAYLOAD_SIZE	12
#define WLV8TX_RX_PAYLOAD_SIZE		6

enum
{
	REALACC_WLV8TX_BIND_TX = 0,
	REALACC_WLV8TX_BIND_RX_SETUP,
	REALACC_WLV8TX_BIND_RX_CHECK,
	REALACC_WLV8TX_DATA
};

#endif
static uint8_t realacc_bind_packet[REALACC_BIND_PAYLOAD_SIZE];

static void __attribute__((unused)) REALACC_send_packet()
{
	#ifndef MULTI_AIR
	if(sub_protocol == REALACC_WLV8TX)
	{
		packet[ 0]= 0xDC;										// DC
		packet[ 1]= 0x80;										// not used
		packet[ 2]= 0x80;										// not used
		packet[ 3]= convert_channel_16b_limit(CH2,0x0F,0xF1);	// Throttle 0F..80..F7 (spring center)
		packet[ 4]= convert_channel_16b_limit(CH1,0xE7,0x19);	// Steering E7..80..1F (reversed)
		packet[ 5]=(convert_channel_8b(CH3) >> 2);				// GY Trim (Rate)    02..20..3D
		packet[ 6]= 0x20; 										// Constant?
		packet[ 7]= (convert_channel_8b(CH4) >> 2);				// TH Trim (Rate)    02..20..3D
		packet[ 8]=((convert_channel_8b(CH5) >> 2) ^ 0x3F);		// ST Trim (R..C..L) 3F..20..01 (reversed)
		packet[ 9]= 0x77;										// Change at each power up? 07 0D 25 50 77 82 89 A5 E4
		packet[10]= 0x0C;										// WL-V8Tx: fixed value = 0C
		packet[11]= 0x00;										// Constant?
		packet[12]= 0x00;										// Constant?
	}
	else
	#endif
	{
		packet[ 0]= 0xDC;							// DC/D6/DE
		packet[ 1]= convert_channel_8b(AILERON);	// 00..80..FF
		packet[ 2]= convert_channel_8b(ELEVATOR);	// 00..80..FF
		packet[ 3]= convert_channel_8b(THROTTLE);	// 00..FF
		packet[ 4]= convert_channel_8b(RUDDER);		// 00..80..FF
		packet[ 5]= 0x20; 							// Trim
		packet[ 6]= 0x20; 							// Trim
		packet[ 7]= 0x20; 							// Trim
		packet[ 8]= 0x20; 							// Trim
		packet[ 9]= 0x88;							// Change at each power up: C5 A2 77 F0 84 58, fixed for the E017 = 88
		packet[10]= 0x04 							// Flag1: R11=04, E017=0C
			| 0x02									//   Rate1=0, Rate2=1, Rate3=2
			| GET_FLAG(CH8_SW, 0x20);				//   Headless
		packet[11]= 0x00 							// Flag2
			| GET_FLAG(CH7_SW, 0x01)				//   Calib
			| GET_FLAG(CH9_SW, 0x20)				//   Return
			| GET_FLAG(CH10_SW,0x80);				//   Throttle cut
		packet[12]= 0x00 							// Flag3
			| GET_FLAG(CH5_SW, 0x01)				//   Flip
			| GET_FLAG(CH11_SW,0x02)				//   Rotating
			| GET_FLAG(CH6_SW, 0x80);				//   Light
	}
	XN297_Hopping(hopping_frequency_no);
	hopping_frequency_no++;
	hopping_frequency_no %= REALACC_RF_NUM_CHANNELS;
	XN297_WriteEnhancedPayload(packet, REALACC_PAYLOAD_SIZE,0);
}

static void __attribute__((unused)) REALACC_send_bind_packet()
{
	#ifndef MULTI_AIR
	if(sub_protocol == REALACC_WLV8TX && rx_id[0])
	{ // WLV8TX bind sent after RX packet B3 acknowledged
		packet[0] = 0xB4;
		packet[1] = rx_id[1];
		packet[2] = rx_id[2];
		memcpy(&packet[3], realacc_bind_packet, 4);	// Original TX ID before XOR
		memcpy(&packet[7], hopping_frequency, 5);	// RF frequencies
		XN297_WriteEnhancedPayload(packet, WLV8TX_BIND2_PAYLOAD_SIZE,1);
	}
	else
	#endif
	{ // initial bind sent for all models
		packet[0] = 0xB1;							// B0/B1
		memcpy(&packet[1],realacc_bind_packet,4);	// Address
		memcpy(&packet[5],hopping_frequency,5);		// RF frequencies
		XN297_WriteEnhancedPayload(packet, REALACC_BIND_PAYLOAD_SIZE,1);
	}
}

static void __attribute__((unused)) REALACC_initialize_txid()
{
	rx_tx_addr[3] &= 0x3F;
	calc_fh_channels(REALACC_RF_NUM_CHANNELS);
	#ifdef FORCE_REALACC_ORIGINAL_ID
		if(RX_num==0)
		{//TX1
			rx_tx_addr[0]=0x99;
			rx_tx_addr[1]=0x06;
			rx_tx_addr[2]=0x00;
			rx_tx_addr[3]=0x00;	// 00..3F:OK, 40..:NOK
			hopping_frequency[0]=0x55;
			hopping_frequency[1]=0x59;
			hopping_frequency[2]=0x5A;
			hopping_frequency[3]=0x5A;
			hopping_frequency[4]=0x62;
		}
		else
		{//TX2
			rx_tx_addr[0]=0x4F;
			rx_tx_addr[1]=0xB9;
			rx_tx_addr[2]=0xA1;
			rx_tx_addr[3]=0x17;
			hopping_frequency[0]=0x45;
			hopping_frequency[1]=0x38;
			hopping_frequency[2]=0x3C;
			hopping_frequency[3]=0x41;
			hopping_frequency[4]=0x3F;
		}
	#endif
	#ifdef FORCE_REALACC_WLV8TX_ID
	{
		rx_tx_addr[0]=0x32;
		rx_tx_addr[1]=0xE9;
		rx_tx_addr[2]=0xDE;
		rx_tx_addr[3]=0x0A;
		hopping_frequency[0]=0x45;
		hopping_frequency[1]=0x42;
		hopping_frequency[2]=0x38;
		hopping_frequency[3]=0x3C;
		hopping_frequency[4]=0x4A;
	}
	#endif
	memcpy(realacc_bind_packet, rx_tx_addr, 4);
	#if 0
		debug("ID: %02X %02X %02X %02X, C: ",rx_tx_addr[0],rx_tx_addr[1],rx_tx_addr[2],rx_tx_addr[3]);
		for(uint8_t i=0; i<REALACC_RF_NUM_CHANNELS; i++)
			debug(" %02X",hopping_frequency[i]);
		debugln("");
	#endif
}

static void __attribute__((unused)) REALACC_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_1M);
	XN297_SetTXAddr((uint8_t*)"MAIN", 4);
	XN297_RFChannel(REALACC_BIND_RF_CHANNEL);	// Set bind channel
}

#ifndef MULTI_AIR
static void __attribute__((unused)) REALACC_wlv8tx_process_rx()
{
	if(!XN297_IsRX())
		return;
	uint8_t len = XN297_ReadEnhancedPayload(packet_in, WLV8TX_RX_PAYLOAD_SIZE);
	if(len != 3)
		return;

	if(packet_in[0] == 0xB3 && !rx_id[0])
	{
		rx_id[0] = 0x01;
		rx_id[1] = packet_in[1];
		rx_id[2] = packet_in[2];
		rx_tx_addr[2] ^= packet_in[1];
		rx_tx_addr[3] ^= packet_in[2];
		rx_tx_addr[3] |= 0x80;
	}
	else if(packet_in[0] == 0xB5 && rx_id[0])
	{
		BIND_DONE;
		XN297_SetTXAddr(rx_tx_addr, 4);
		XN297_SetTxRxMode(TXRX_OFF);
		bind_phase = REALACC_WLV8TX_DATA;
	}
}
#endif

uint16_t REALACC_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(REALACC_PACKET_PERIOD);
	#endif
	XN297_SetPower();

	#ifdef MULTI_AIR
		if(sub_protocol == REALACC_WLV8TX)
		{
			SUB_PROTO_INVALID;
			return 10000;
		}
	#else
	if(sub_protocol == REALACC_WLV8TX)
	{
		if(bind_phase == REALACC_WLV8TX_DATA)
		{
			XN297_SetTxRxMode(TX_EN);
			REALACC_send_packet();
			return WLV8TX_PACKET_PERIOD;
		}

		if(bind_phase == REALACC_WLV8TX_BIND_RX_SETUP)
		{
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			bind_phase = REALACC_WLV8TX_BIND_RX_CHECK;
			return WLV8TX_PACKET_PERIOD/3;
		}

		if(bind_phase == REALACC_WLV8TX_BIND_RX_CHECK)
		{
			REALACC_wlv8tx_process_rx();
			if(bind_phase == REALACC_WLV8TX_DATA)
				return WLV8TX_PACKET_PERIOD;
			bind_phase = REALACC_WLV8TX_BIND_TX;
			return WLV8TX_PACKET_PERIOD/3;
		}
		//Bind Phase
		XN297_SetTxRxMode(TXRX_OFF);
		XN297_SetTxRxMode(TX_EN);
		REALACC_send_bind_packet();
		bind_phase = REALACC_WLV8TX_BIND_RX_SETUP;
		return WLV8TX_PACKET_PERIOD/3;
	}
	#endif

	XN297_SetTxRxMode(TX_EN);
	if(IS_BIND_IN_PROGRESS)
	{
		REALACC_send_bind_packet();
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr, 4);
		}
	}
	else
		REALACC_send_packet();
	return REALACC_PACKET_PERIOD;
}

void REALACC_init()
{
	BIND_IN_PROGRESS;	// autobind protocol
	REALACC_initialize_txid();
	REALACC_RF_init();
	#ifndef MULTI_AIR
	if(sub_protocol == REALACC_WLV8TX)
	{
		rx_id[0] = 0;	// realacc_wlv8tx_rx_b3
		rx_id[1] = 0;
		rx_id[2] = 0;
		bind_phase = REALACC_WLV8TX_BIND_TX;
		memcpy(packet, realacc_bind_packet, 4);
		packet[3] |= 0x80;
		XN297_SetRXAddr(packet, 4);
	}
	else
	#endif
		bind_counter=REALACC_BIND_COUNT;
	hopping_frequency_no=0;
}

#endif

// XN297 speed 1Mb, scrambled, enhanced
// Bind
//   Address = 4D 41 49 4E = 'MAIN'
//   Channel = 80 (most likely from dump)
//   TX1
//   ---
//     P(10) = B1 99 06 00 00 55 59 5A 5A 62
//     Bx indicates bind packet, why x=1?
//     99 06 00 00 = ID = address of normal packets
//     55 59 5A 5A 62 = 85, 89, 90, 90, 98 = RF channels to be used (kind of match previous dumps)
//   TX2
//   ---
//     P(10) = B0 4F B9 A1 17 45 38 3C 41 3F
//     Bx indicates bind packet, why x=0?
//     4F B9 A1 17 = ID = address of normal packets
//     45 38 3C 41 3F  = 69, 56, 60, 65, 63 = RF channels to be used
//   WLtoys Car 284019A V8-Tx
// ------> TX sends
//     P(10)= B1 32 E9 DE 0A 45 42 38 3C 4A
//     B1 indicates bind packet
//     32 E9 DE 0A = ID -> address of normal TX packets before bind
//     45 42 38 3C 4A  = 69, 66, 56, 60, 74 -> RF channels to be used
//     P(12)= B4 D4 E9 32 E9 DE 0A 45 42 38 3C 4A -> B4 designates TX found RX?,  P(10) to P(12), TX inserts packet[1] and [2] from RX
//     32 E9 0A E3 = ID -> address of normal TX packets after bind (XOR rx_tx_addr[2],[3] with values from RX)
// ------> RX returns
//     32 E9 DE 8A = -> RX ID address returning bind info, same as TX address except (packet[3]|0x80)
//     P(3)= B3 D4 E9 = B3=bind initiates Tx found, D4=XOR with rx_tx_addr[2], E9=XOR with rx_tx_addr[3]
//     P(3)= B5 D4 E9 = B5=bind completed
//     P(6)= 40 83 00 00 00 00 = RX returns on some hopping channels - unknown? telemetry? not used on this model?
// Normal
//   TX1
//   ---
//     Address = 99 06 00 00
//     Channels = 84, 89, 90, 90, 98 (guess from bind)
//     P(13)= DC 80 80 32 80 20 20 20 20 58 04 00 00
//     Dx = normal packet, why C ?
//     80 80 32 80 : AETR 00..80..FF
//     20 20 20 20 : Trims
//     58 : changing every time the TX restart
//     04 : |0x20=headless, |0x01=rate2, |0x02=rate3
//     00 : |0x01=calib, |0x20=return, |0x80=unknown
//     00 : |0x80=light, |0x01=flip
//   TX2
//   ---
//     Address = 4F B9 A1 17
//     P(13)= D6/DE 80 80 80 80 20 20 20 20 88 0C 00 00
//     Dx = normal packet, why 6/E ?
//     80 80 32 80 : AETR 00..80..FF
//     20 20 20 20 : Trims
//     88 : not changing unknown
//     0C : |0x20=headless, |0x01=rate2, |0x02=rate3
//     00 : |0x01=calib, |0x20=return, |0x80=unknown
//     00 : |0x80=light, |0x01=flip, |0x02=Rotating
//   WLtoys Car 284019A V8-Tx
//   ---
//     Address = 32 E9 DE 0A (before bind), 32 E9 0A E3 (after bind and XOR)
//     P(13)= DC 80 80 80 80 20 20 20 20 77 0C 00 00
//     Dx = normal packet, why C ?
//     80 80 80 80 : AE=80 TR 00..80..FF
//     20 20 20 20 : Gyro Rate, Constant?, Throttle Rate, Steer Trim
//     77 : changing every time the TX restart
//     0C : Constant?
//     00 : Constant?
//     00 : Constant?
