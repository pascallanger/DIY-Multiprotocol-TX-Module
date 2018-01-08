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

#if defined(Q303_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define Q303_BIND_COUNT			1500
#define Q303_INITIAL_WAIT		500
#define Q303_RF_BIND_CHANNEL	0x02

#define Q303_BTN_TAKEOFF	1
#define Q303_BTN_DESCEND	2
#define Q303_BTN_SNAPSHOT	4
#define Q303_BTN_VIDEO		8
#define Q303_BTN_RTH		16
#define Q303_BTN_VTX		32

static uint8_t __attribute__((unused)) cx10wd_getButtons()
{
	#define CX10WD_FLAG_LAND	0x20
	#define CX10D_FLAG_LAND		0x80
	#define CX10WD_FLAG_TAKEOFF	0x40

	static uint8_t BTN_state;
	static uint8_t command;

	// startup
	if(packet_count < 50)
	{
		BTN_state = 0;
		command = 0;
		packet_count++;
	}
	// auto land
	else if((Channel_data[CH5]<CHANNEL_MIN_COMMAND) && !(BTN_state & Q303_BTN_DESCEND))
	{
		BTN_state |= Q303_BTN_DESCEND;
		BTN_state &= ~Q303_BTN_TAKEOFF;
		switch(sub_protocol)
		{
			case CX10WD:
				command ^= CX10WD_FLAG_LAND;
				break;
			case CX10D:
				command ^= CX10D_FLAG_LAND;
				break;
		}
	}
	// auto take off
	else if(CH5_SW && !(BTN_state & Q303_BTN_TAKEOFF))
	{
		BTN_state |= Q303_BTN_TAKEOFF;
		BTN_state &= ~Q303_BTN_DESCEND;
		command ^= CX10WD_FLAG_TAKEOFF;
	}

	return command;
}

static uint8_t __attribute__((unused))  cx35_lastButton()
{
	#define CX35_CMD_RATE		0x09
	#define CX35_CMD_TAKEOFF	0x0e
	#define CX35_CMD_DESCEND	0x0f
	#define CX35_CMD_SNAPSHOT	0x0b
	#define CX35_CMD_VIDEO		0x0c
	#define CX35_CMD_RTH		0x11
	#define CX35_CMD_VTX		0x10
	
	static uint8_t BTN_state;
	static uint8_t command;
	// simulate 2 keypress on rate button just after bind
	if(packet_count < 50)
	{
		BTN_state = 0;
		packet_count++;
		command = 0x00; // startup
	}
	else if(packet_count < 150)
	{
		packet_count++;
		command = CX35_CMD_RATE; // 1st keypress
	}
	else if(packet_count < 250)
	{
		packet_count++;
		command |= 0x20; // 2nd keypress
	}
	// descend
	else if(!(GET_FLAG(CH5_SW, 1)) && !(BTN_state & Q303_BTN_DESCEND))
	{
		BTN_state |= Q303_BTN_DESCEND;
		BTN_state &= ~Q303_BTN_TAKEOFF;
		command = CX35_CMD_DESCEND;
	}
	// take off
	else if(GET_FLAG(CH5_SW,1) && !(BTN_state & Q303_BTN_TAKEOFF))
	{
		BTN_state |= Q303_BTN_TAKEOFF;
		BTN_state &= ~Q303_BTN_DESCEND;
		command = CX35_CMD_TAKEOFF;
	}
	// RTH
	else if(GET_FLAG(CH10_SW,1) && !(BTN_state & Q303_BTN_RTH))
	{
		BTN_state |= Q303_BTN_RTH;
		if(command == CX35_CMD_RTH)
			command |= 0x20;
		else
			command = CX35_CMD_RTH;
	}
	else if(!(GET_FLAG(CH10_SW,1)) && (BTN_state & Q303_BTN_RTH))
	{
		BTN_state &= ~Q303_BTN_RTH;
		if(command == CX35_CMD_RTH)
			command |= 0x20;
		else
			command = CX35_CMD_RTH;
	}
	// video
	else if(GET_FLAG(CH8_SW,1) && !(BTN_state & Q303_BTN_VIDEO))
	{
		BTN_state |= Q303_BTN_VIDEO;
		if(command == CX35_CMD_VIDEO)
			command |= 0x20;
		else
			command = CX35_CMD_VIDEO;
	}
	else if(!(GET_FLAG(CH8_SW,1)) && (BTN_state & Q303_BTN_VIDEO))
	{
		BTN_state &= ~Q303_BTN_VIDEO;
		if(command == CX35_CMD_VIDEO)
			command |= 0x20;
		else
			command = CX35_CMD_VIDEO;
	}
	// snapshot
	else if(GET_FLAG(CH7_SW,1) && !(BTN_state & Q303_BTN_SNAPSHOT))
	{
		BTN_state |= Q303_BTN_SNAPSHOT;
		if(command == CX35_CMD_SNAPSHOT)
			command |= 0x20;
		else
			command = CX35_CMD_SNAPSHOT;
	}
	// vtx channel
	else if(GET_FLAG(CH6_SW,1) && !(BTN_state & Q303_BTN_VTX))
	{
		BTN_state |= Q303_BTN_VTX;
		if(command == CX35_CMD_VTX)
			command |= 0x20;
		else
			command = CX35_CMD_VTX;
	}

	if(!(GET_FLAG(CH7_SW,1)))
		BTN_state &= ~Q303_BTN_SNAPSHOT;
	if(!(GET_FLAG(CH6_SW,1)))
		BTN_state &= ~Q303_BTN_VTX;

	return command;
}

static void __attribute__((unused)) Q303_send_packet(uint8_t bind)
{
	uint16_t aileron, elevator, throttle, rudder, slider;
	if(bind)
	{
		packet[0] = 0xaa;
		memcpy(&packet[1], rx_tx_addr + 1, 4);
		memset(&packet[5], 0, packet_length-5);
	}
	else
	{
		packet[0] = 0x55;
		// sticks
		switch(sub_protocol)
		{
			case Q303:
			case CX35:
				aileron  = convert_channel_16b_limit(AILERON,  0, 1000);
				elevator = convert_channel_16b_limit(ELEVATOR, 1000, 0);
				throttle = convert_channel_16b_limit(THROTTLE, 0, 1000);
				rudder   = convert_channel_16b_limit(RUDDER,   1000, 0);
				if(sub_protocol == CX35)
					aileron = 1000 - aileron;
				packet[1] = aileron >> 2;			// 8 bits
				packet[2] = (aileron & 0x03) << 6	// 2 bits
							| (elevator >> 4);		// 6 bits
				packet[3] = (elevator & 0x0f) << 4	// 4 bits
							| (throttle >> 6);		// 4 bits
				packet[4] = (throttle & 0x3f) << 2	// 6 bits 
							| (rudder >> 8);		// 2 bits
				packet[5] = rudder & 0xff;			// 8 bits
				break;
			case CX10D:
			case CX10WD:
				aileron  = convert_channel_16b_limit(AILERON,  2000, 1000);
				elevator = convert_channel_16b_limit(ELEVATOR, 2000, 1000);
				throttle = convert_channel_16b_limit(THROTTLE, 1000, 2000);
				rudder   = convert_channel_16b_limit(RUDDER,   1000, 2000);
				packet[1] = aileron & 0xff;
				packet[2] = aileron >> 8;
				packet[3] = elevator & 0xff;
				packet[4] = elevator >> 8;
				packet[5] = throttle & 0xff;
				packet[6] = throttle >> 8;
				packet[7] = rudder & 0xff;
				packet[8] = rudder >> 8;
				break;
		}

		// buttons
		switch(sub_protocol)
		{
			case Q303:
				packet[6] = 0x10;					// trim(s) ?
				packet[7] = 0x10;					// trim(s) ?
				packet[8] = 0x03					// high rate (0-3)
					| GET_FLAG(CH5_SW,   0x40)
					| GET_FLAG(CH10_SW,	 0x80);
				packet[9] = 0x40					// always set
					| GET_FLAG(CH9_SW,0x08)
					| GET_FLAG(CH6_SW,	0x80)
					| GET_FLAG(CH7_SW,0x10)
					| GET_FLAG(CH8_SW,   0x01);
				if(Channel_data[CH11] < CHANNEL_MIN_COMMAND)
					packet[9] |= 0x04;				// gimbal down
				else if(CH11_SW)
						packet[9] |= 0x20;			// gimbal up
				break;

			case CX35:
				slider = convert_channel_16b_limit(CH11, 731, 342);
				packet[6] = slider >> 2;
				packet[7] = ((slider & 3) << 6)
					| 0x3e;							// ?? 6 bit left (always 111110 ?)
				packet[8] = 0x80;					// always set
				packet[9] = cx35_lastButton();
				break;

			case CX10D:
				packet[8] |= GET_FLAG(CH6_SW, 0x10);
				packet[9] = 0x02; // rate (0-2)
				packet[10]= cx10wd_getButtons();	// auto land / take off management
				break;

			case CX10WD:
				packet[8] |= GET_FLAG(CH6_SW, 0x10);
				packet[9]  = 0x02  // rate (0-2)
						| cx10wd_getButtons();		// auto land / take off management
				packet[10] = 0x00;
				break;
		}
	}

	// Power on, TX mode, CRC enabled
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? Q303_RF_BIND_CHANNEL : hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no %= rf_ch_num;

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	XN297_WritePayload(packet, packet_length);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) Q303_init()
{
	const uint8_t bind_address[] = {0xcc,0xcc,0xcc,0xcc,0xcc};

	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	switch(sub_protocol)
	{
		case CX35:
		case CX10D:
		case CX10WD:
			XN297_SetScrambledMode(XN297_SCRAMBLED);
			NRF24L01_SetBitrate(NRF24L01_BR_1M);
			break;
		case Q303:
			XN297_SetScrambledMode(XN297_UNSCRAMBLED);
			NRF24L01_SetBitrate(NRF24L01_BR_250K);
			break;
	}
	XN297_SetTXAddr(bind_address, 5);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);		// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// no retransmits
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);		// Set feature bits on
	NRF24L01_Activate(0x73);
}

static void __attribute__((unused)) Q303_initialize_txid()
{
	uint8_t i,offset;

	rx_tx_addr[0] = 0x55;

	switch(sub_protocol)
	{
		case Q303:
		case CX10WD:
			offset = rx_tx_addr[1] & 3;
			for(i=0; i<4; i++)
				hopping_frequency[i] = 0x46 + i*2 + offset;
			break;
		case CX35:
		case CX10D:
			// not thoroughly figured out rx_tx_addr/channels mapping yet
			// for now 5 msb of rx_tx_addr[1] must be cleared
			rx_tx_addr[1] &= 7;
			offset = 6+(rx_tx_addr[1]*3);
			hopping_frequency[0] = 0x14; // works only if rx_tx_addr[1] < 8
			for(i=1; i<16; i++)
			{
				hopping_frequency[i] = hopping_frequency[i-1] + offset;
				if(hopping_frequency[i] > 0x41)
					hopping_frequency[i] -= 0x33;
				if(hopping_frequency[i] < 0x14)
					hopping_frequency[i] += offset;
			}
			// CX35 tx uses only 4 of those channels (#0,3,6,9)
			if(sub_protocol == CX35)
				for(i=0; i<4; i++)
					hopping_frequency[i] = hopping_frequency[i*3];
			break;
	}
}

uint16_t Q303_callback()
{
	if(IS_BIND_DONE)
		Q303_send_packet(0);
	else
	{
		if (bind_counter == 0)
		{
			XN297_SetTXAddr(rx_tx_addr, 5);
			packet_count = 0;
			BIND_DONE;
		}
		else
		{
			Q303_send_packet(1);
			bind_counter--;
		}
	}
	return packet_period;
}

uint16_t initQ303()
{
	Q303_initialize_txid();
	Q303_init();
	bind_counter = Q303_BIND_COUNT;
	switch(sub_protocol)
	{
		case Q303:
			packet_period = 1500;
			packet_length = 10;
			rf_ch_num = 4;
			break;
		case CX35:
			packet_period = 3000;
			packet_length = 10;
			rf_ch_num = 4;
			break;
		case CX10D:
			packet_period = 3000;
			packet_length = 11;
			rf_ch_num = 16;
			break;
		case CX10WD:
			packet_period = 3000;
			packet_length = 11;
			rf_ch_num = 4;
		break;
	}
	hopping_frequency_no = 0;
	BIND_IN_PROGRESS;	// autobind protocol
	return Q303_INITIAL_WAIT;
}

#endif
