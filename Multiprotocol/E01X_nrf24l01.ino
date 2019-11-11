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
// compatible with E012 and E015

#if defined(E01X_NRF24L01_INO)

#include "iface_nrf24l01.h"

//Protocols constants
#define E01X_BIND_COUNT			500
#define E01X_INITIAL_WAIT		500
#define E01X_ADDRESS_LENGTH		5

#define E012_PACKET_PERIOD		4525
#define E012_RF_BIND_CHANNEL	0x3c
#define E012_NUM_RF_CHANNELS	4
#define E012_PACKET_SIZE		15

#define E015_PACKET_PERIOD		4500	// stock Tx=9000, but let's send more packets ...
#define E015_RF_CHANNEL			0x2d	// 2445 MHz
#define E015_PACKET_SIZE		10
#define E015_BIND_PACKET_SIZE	9

#define E016H_PACKET_PERIOD 4080
#define E016H_PACKET_SIZE     10
#define E016H_BIND_CHANNEL    80
#define E016H_NUM_CHANNELS    4

//Channels
#define E01X_ARM_SW			CH5_SW
#define E016H_STOP_SW		CH5_SW
#define E01X_FLIP_SW		CH6_SW
#define E01X_LED_SW			CH7_SW
#define E01X_HEADLESS_SW	CH8_SW
#define E01X_RTH_SW			CH9_SW

// E012 flags packet[1]
#define E012_FLAG_FLIP		0x40
#define E012_FLAG_HEADLESS	0x10
#define E012_FLAG_RTH		0x04
// E012 flags packet[7]
#define E012_FLAG_EXPERT	0x02

// E015 flags packet[6]
#define E015_FLAG_DISARM	0x80
#define E015_FLAG_ARM		0x40
// E015 flags packet[7]
#define E015_FLAG_FLIP		0x80
#define E015_FLAG_HEADLESS	0x10
#define E015_FLAG_RTH		0x08
#define E015_FLAG_LED		0x04
#define E015_FLAG_EXPERT	0x02
#define E015_FLAG_INTERMEDIATE 0x01

// E016H flags packet[1]
#define E016H_FLAG_CALIBRATE	0x80
#define E016H_FLAG_STOP		0x20
#define E016H_FLAG_FLIP		0x04
// E016H flags packet[3]
#define E016H_FLAG_HEADLESS	0x10
#define E016H_FLAG_RTH		0x04
// E016H flags packet[7]
#define E016H_FLAG_TAKEOFF	0x80
#define E016H_FLAG_HIGHRATE	0x08

static void __attribute__((unused)) E015_check_arming()
{
	uint8_t arm_channel = E01X_ARM_SW;

	if (arm_channel != arm_channel_previous)
	{
		arm_channel_previous = arm_channel;
		if (arm_channel)
		{
			armed = 1;
			arm_flags ^= E015_FLAG_ARM;
		}
		else
		{
			armed = 0;
			arm_flags ^= E015_FLAG_DISARM;
		}
	}
}

static void __attribute__((unused)) E01X_send_packet(uint8_t bind)
{
    uint8_t can_flip = 0, calibrate = 1;
	if(sub_protocol==E012)
	{
		packet_length=E012_PACKET_SIZE;
		packet[0] = rx_tx_addr[1];
		if(bind)
		{
			packet[1] = 0xaa;
			memcpy(&packet[2], hopping_frequency, E012_NUM_RF_CHANNELS);
			memcpy(&packet[6], rx_tx_addr, E01X_ADDRESS_LENGTH);
			rf_ch_num=E012_RF_BIND_CHANNEL;
		}
		else
		{
			packet[1] = 0x01
				| GET_FLAG(E01X_RTH_SW,			E012_FLAG_RTH)
				| GET_FLAG(E01X_HEADLESS_SW,	E012_FLAG_HEADLESS)
				| GET_FLAG(E01X_FLIP_SW,		E012_FLAG_FLIP);
			packet[2] = convert_channel_16b_limit(AILERON,  0xc8, 0x00); // aileron
			packet[3] = convert_channel_16b_limit(ELEVATOR, 0x00, 0xc8); // elevator
			packet[4] = convert_channel_16b_limit(RUDDER,   0xc8, 0x00); // rudder
			packet[5] = convert_channel_16b_limit(THROTTLE, 0x00, 0xc8); // throttle
			packet[6] = 0xaa;
			packet[7] = E012_FLAG_EXPERT;	// rate (0-2)
			packet[8] = 0x00;
			packet[9] = 0x00;
			packet[10]= 0x00;
			rf_ch_num=hopping_frequency[hopping_frequency_no++];
			hopping_frequency_no %= E012_NUM_RF_CHANNELS;
		}
		packet[11] = 0x00;
		packet[12] = 0x00;
		packet[13] = 0x56; 
		packet[14] = rx_tx_addr[2];
	}
	else if(sub_protocol==E015)
	{ // E015
		if(bind)
		{
			packet[0] = 0x18;
			packet[1] = 0x04;
			packet[2] = 0x06;
			// data phase address
			memcpy(&packet[3], rx_tx_addr, E01X_ADDRESS_LENGTH);
			// checksum
			packet[8] = packet[3];
			for(uint8_t i=4; i<8; i++)
				packet[8] += packet[i];
			packet_length=E015_BIND_PACKET_SIZE;
		}
		else
		{
			E015_check_arming();
			packet[0] = convert_channel_16b_limit(THROTTLE,	0, 225); // throttle
			packet[1] = convert_channel_16b_limit(RUDDER,   225, 0); // rudder
			packet[2] = convert_channel_16b_limit(AILERON,  0, 225); // aileron
			packet[3] = convert_channel_16b_limit(ELEVATOR, 225, 0); // elevator
			packet[4] = 0x20; // elevator trim
			packet[5] = 0x20; // aileron trim
			packet[6] = arm_flags;
			packet[7] = E015_FLAG_EXPERT
				| GET_FLAG(E01X_FLIP_SW,	E015_FLAG_FLIP)
				| GET_FLAG(E01X_LED_SW,		E015_FLAG_LED)
				| GET_FLAG(E01X_HEADLESS_SW,E015_FLAG_HEADLESS)
				| GET_FLAG(E01X_RTH_SW,		E015_FLAG_RTH);
			packet[8] = 0;
			// checksum
			packet[9] = packet[0];
			for(uint8_t i=1; i<9; i++)
				packet[9] += packet[i];
			packet_length=E015_PACKET_SIZE;
		}
	}
	else
	{ // E016H
		packet_length=E016H_PACKET_SIZE;
		if(bind)
		{
			rf_ch_num=E016H_BIND_CHANNEL;
			memcpy(packet, &rx_tx_addr[1], 4);
			memcpy(&packet[4], hopping_frequency, 4);
			packet[8] = 0x23;
		}
		else
		{
			// trim commands
			packet[0] = 0;
			// aileron
			uint16_t val = convert_channel_16b_limit(AILERON, 0, 0x3ff);
			can_flip |= (val < 0x100) || (val > 0x300);
			packet[1] = val >> 8;
			packet[2] = val & 0xff;
			if(val < 0x300) calibrate = 0;
			// elevator
			val = convert_channel_16b_limit(ELEVATOR, 0x3ff, 0);
			can_flip |= (val < 0x100) || (val > 0x300);
			packet[3] = val >> 8;
			packet[4] = val & 0xff;
			if(val < 0x300) calibrate = 0;
			// throttle
			val = convert_channel_16b_limit(THROTTLE, 0, 0x3ff);
			packet[5] = val >> 8;
			packet[6] = val & 0xff;
			if(val > 0x100) calibrate = 0;
			// rudder
			val = convert_channel_16b_limit(RUDDER, 0, 0x3ff);
			packet[7] = val >> 8;
			packet[8] = val & 0xff;
			if(val > 0x100) calibrate = 0;
			// flags
			packet[1] |= GET_FLAG(E016H_STOP_SW, E016H_FLAG_STOP)
					  |  (can_flip ? GET_FLAG(E01X_FLIP_SW, E016H_FLAG_FLIP) : 0)
					  |  (calibrate ? E016H_FLAG_CALIBRATE : 0);
			packet[3] |= GET_FLAG(E01X_HEADLESS_SW, E016H_FLAG_HEADLESS)
					  |  GET_FLAG(E01X_RTH_SW, E016H_FLAG_RTH);
			packet[7] |= E016H_FLAG_HIGHRATE;
			// frequency hopping
			rf_ch_num=hopping_frequency[hopping_frequency_no++ & 0x03];
		}
		// checksum
		packet[9] = packet[0];
		for (uint8_t i=1; i < E016H_PACKET_SIZE-1; i++)
			packet[9] += packet[i];
	}

	// Power on, TX mode, CRC enabled
	if(sub_protocol==E016H)
		XN297_Configure( _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	else //E012 & E015
		HS6200_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num);

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	if(sub_protocol==E016H)
		XN297_WritePayload(packet, packet_length);
	else
		HS6200_WritePayload(packet, packet_length);

	// Check and adjust transmission power. We do this after
	// transmission to not bother with timeout after power
	// settings change -  we have plenty of time until next
	// packet.
	NRF24L01_SetPower();
}

static void __attribute__((unused)) E01X_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	if(sub_protocol==E012)
		HS6200_SetTXAddr((uint8_t *)"\x55\x42\x9C\x8F\xC9", E01X_ADDRESS_LENGTH);
	else if(sub_protocol==E015)
		HS6200_SetTXAddr((uint8_t *)"\x62\x54\x79\x38\x53", E01X_ADDRESS_LENGTH);
	else //E016H
		XN297_SetTXAddr((uint8_t *)"\x5a\x53\x46\x30\x31", 5);  // bind address
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1 Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
	NRF24L01_Activate(0x73);
}

uint16_t E01X_callback()
{
	if(IS_BIND_IN_PROGRESS)
	{
		if (bind_counter == 0)
		{
			if(sub_protocol==E016H)
				XN297_SetTXAddr(rx_tx_addr, E01X_ADDRESS_LENGTH);
			else
				HS6200_SetTXAddr(rx_tx_addr,  E01X_ADDRESS_LENGTH);
			BIND_DONE;
		}
		else
		{
			E01X_send_packet(1);
			bind_counter--;
		}
	}
	else
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(packet_period);
		#endif
		E01X_send_packet(0);
	}
	return packet_period;
}

static void __attribute__((unused)) E012_initialize_txid()
{
    // rf channels
    uint32_t lfsr=random(0xfefefefe);
    for(uint8_t i=0; i<E012_NUM_RF_CHANNELS; i++)
    {
		hopping_frequency[i] = 0x10 + ((lfsr & 0xff) % 0x32); 
		lfsr>>=8;
	}
}

static void __attribute__((unused)) E016H_initialize_txid()
{
	// tx id
    rx_tx_addr[0] = 0xa5;
    rx_tx_addr[1] = 0x00;

    // rf channels
    uint32_t lfsr=random(0xfefefefe);
    for(uint8_t i=0; i<E016H_NUM_CHANNELS; i++)
    {
		hopping_frequency[i] = (lfsr & 0xFF) % 80; 
		lfsr>>=8;
	}
}

uint16_t initE01X()
{
	BIND_IN_PROGRESS;
	if(sub_protocol==E012)
	{
		E012_initialize_txid();
		packet_period=E012_PACKET_PERIOD;
	}
	else if(sub_protocol==E015)
	{
		packet_period=E015_PACKET_PERIOD;
		rf_ch_num=E015_RF_CHANNEL;
		armed = 0;
		arm_flags = 0;
		arm_channel_previous = E01X_ARM_SW;
	}
	else
	{ // E016H
		E016H_initialize_txid();
		packet_period=E016H_PACKET_PERIOD;
	}
	E01X_init();
	bind_counter = E01X_BIND_COUNT;
	hopping_frequency_no = 0;
	return E01X_INITIAL_WAIT;
}

#endif
