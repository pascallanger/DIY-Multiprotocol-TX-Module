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

#if defined(E016HV2_CC2500_INO)

#include "iface_nrf250k.h"

//#define FORCE_E016HV2_ORIGINAL_ID

#define E016HV2_INITIAL_WAIT		500
#define E016HV2_PACKET_PERIOD		10000
#define E016HV2_RF_BIND_CHANNEL	5
#define E016HV2_PAYLOAD_SIZE		11
#define E016HV2_BIND_COUNT		300	//3sec

static void __attribute__((unused)) E016HV2_send_packet()
{
	//payload length (after this byte)??
	packet[0 ] = 0x0A;
	
	//bind indicator??
	if(IS_BIND_IN_PROGRESS)
	{
		packet[1 ] = 0x02;
		if(bind_counter)
			bind_counter--;
		else
		{
			BIND_DONE;
			XN297L_RFChannel(rf_ch_num);	// Set main channel
		}
	}
	else
	{
		packet[1 ] = 0x20;
		if(prev_option!=option)
		{
			XN297L_RFChannel(option);	// Set main channel
			prev_option=option;
		}
	}
	
	//ID
	packet[2 ] = rx_tx_addr[2];
	packet[3 ] = rx_tx_addr[3];

	//channels TREA
	uint8_t channel;
	if(IS_BIND_IN_PROGRESS)
		channel=0x64;								// Throttle must be centered during bind
	else
		channel=convert_channel_8b_limit_deadband(THROTTLE,0x00,0x64,0xC8, 20);
	packet[4 ]  = channel;
	channel=convert_channel_16b_limit(RUDDER,0x00,0xC8);
	packet[5 ]  = channel;
	channel=convert_channel_16b_limit(ELEVATOR,0x00,0xC8);
	packet[6 ]  = channel;
	channel=convert_channel_16b_limit(AILERON,0x00,0xC8);
	packet[7 ]  = channel;

	//flags
	if(CH8_SW && !phase) //toggle calib flag
		flags ^= 0x40;
	phase=CH8_SW;

	packet[8 ] = GET_FLAG(CH7_SW,  0x01)			// 0x01=Flip
			   | GET_FLAG(CH9_SW,  0x02)			// 0x02=Headless
			   | GET_FLAG(CH10_SW, 0x04)			// 0x04=One Key Return
			   | flags;								// 0x40=Calib

	packet[9 ] = 0x02;								// Speed control 0x00:low, 0x01:medium, 0x02:high
	
	packet[10] = GET_FLAG(CH5_SW, 0x01)				// 0x01=TakeOff/Land  (momentary switch)
			   | GET_FLAG(CH6_SW, 0x04);			// 0x04=Emergeny Stop (momentary switch)

	if(option==0)
		option=1;									// Select the CC2500
	XN297L_SetPower();								// Set tx_power
	XN297L_SetFreqOffset();							// Set frequency offset

	//Build real packet and send it
	static uint8_t pid=0;
	crc=0;
	
	// stop TX/RX
	CC2500_Strobe(CC2500_SIDLE);
	// flush tx FIFO
	CC2500_Strobe(CC2500_SFTX);
	// packet length
	CC2500_WriteReg(CC2500_3F_TXFIFO, 6 + 4 + 1 + 11 + 2);	// preamble + address + packet_control + payload + crc

	// preamble+address
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (uint8_t*)"\xAA\xAA\xAA\xAA\xAA\xAA\xE7\xE7\xE7\xE7", 10);

	// packet control
	CC2500_WriteReg(CC2500_3F_TXFIFO, 0x50+(pid<<2));
	pid++;

	// payload
	debug("P:")
	for (uint8_t i = 0; i < E016HV2_PAYLOAD_SIZE; ++i)
	{
		uint8_t byte = (bit_reverse(packet[i])<<1) | (packet[i+1]&0x01);
		debug(" %02X",byte)
		CC2500_WriteReg(CC2500_3F_TXFIFO,byte);
		crc=crc16_update(crc, byte, 8);
	}

	// crc
	CC2500_WriteReg(CC2500_3F_TXFIFO,crc >> 8);
	CC2500_WriteReg(CC2500_3F_TXFIFO,crc);
	debugln(" %04X",crc)

	// transmit
	CC2500_Strobe(CC2500_STX);
}

uint16_t E016HV2_callback()
{
	E016HV2_send_packet();
	return E016HV2_PACKET_PERIOD;
}

uint16_t initE016HV2()
{
	//Config CC2500
	if(option==0)
		option=1;									// Select the CC2500
	XN297L_Init();
	XN297L_RFChannel(E016HV2_RF_BIND_CHANNEL);		// Set bind channel

	#ifdef FORCE_E016HV2_ORIGINAL_ID
		rx_tx_addr[2]=0x27;
		rx_tx_addr[3]=0x1B;
		//rf_ch_num = 44;
	#endif
	//General ID
	//3F1B -> 68,2C1B -> 49,2B1B -> 48,2A1B -> 47,291B -> 46,281B -> 45,271B -> 44,261B -> 43,251B -> 42
	//241B -> no bind,231B -> no bind,221B -> 71,211B -> 70,201B -> 69,1F1B -> 68,1E1B -> 67,1D1B -> 66,1C1B -> 65,1B1B -> 64,1A1B -> 63,191B -> 62,181B -> 61,171B -> 60,161B -> 59
	//0C1B -> 49,051B -> 42,041B -> no bind,031B -> no bind,021B -> 71,011B -> 70,001B -> no bind
	if(rx_tx_addr[2]<3) rx_tx_addr[2]+=3;			// rx_tx_addr[2]=0 is invalid
	if(rx_tx_addr[3]==0) rx_tx_addr[3]+=64;			// rx_tx_addr[3]=0 is invalid
	rf_ch_num = (rx_tx_addr[2] + rx_tx_addr[3]) % 32 + 42;
	if(rf_ch_num>71)								// channels 72 and 73 are invalid
	{
		rx_tx_addr[2]-=2;
		rf_ch_num-=2;
	}
	
	phase=CH8_SW;
	flags=0;
	bind_counter = E016HV2_BIND_COUNT;
	BIND_IN_PROGRESS;								// Autobind protocol
	return E016HV2_INITIAL_WAIT;
}

#endif
