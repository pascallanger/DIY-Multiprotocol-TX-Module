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

#if defined(SCORPIO_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define SCORPIO_FORCE_ID

#define SCORPIO_PACKET_PERIOD		12000
#define SCORPIO_PACKETCH_PERIOD		2580
#define SCORPIO_BINDPAYLOAD_SIZE	8
#define SCORPIO_PAYLOAD_SIZE		10
#define SCORPIO_BIND_COUNT			1000
#define SCORPIO_RF_NUM_CHANNELS		3

static uint16_t __attribute__((unused)) SCORPIO_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0x88;					//FIXME: What is this?
		packet[1] = 0x55;					//FIXME: What is this?
		packet[2] = crc;					//CRC_low  for normal packets
		packet[3] = crc >> 8;				//CRC_high for normal packets
		packet[4] = hopping_frequency[0];	//RF freq 0
		packet[5] = hopping_frequency[1];	//RF freq 1
		packet[6] = hopping_frequency[2];	//RF freq 2
		packet[7] = 0x80;					//FIXME: What is this?
		//SendPacket
		CYRF_WriteDataPacketLen(packet, SCORPIO_BINDPAYLOAD_SIZE);
		return SCORPIO_PACKET_PERIOD;
	}
	CYRF_ConfigRFChannel(hopping_frequency[hopping_frequency_no]);
	CYRF_SetPower(0x28);					//Update power
	delayMicroseconds(100);					//Max frequency settle time
	packet[0] = hopping_frequency[0];
	packet[1] = hopping_frequency[1];
	packet[2] = hopping_frequency[2];
	packet[3] = convert_channel_8b(THROTTLE);
	packet[4] = 0xFF - convert_channel_8b(RUDDER);
	packet[5] = convert_channel_8b(ELEVATOR);
	packet[6] = convert_channel_8b(AILERON);
	packet[7] = 0x55;						//FIXME: What is this?
	packet[8] = 0x00;						//FIXME: What is this?
	packet[9] = 0x00;						//FIXME: What is this?
	CYRF_WriteDataPacketLen(packet, SCORPIO_PAYLOAD_SIZE);
	hopping_frequency_no++;
	if(hopping_frequency_no >= SCORPIO_RF_NUM_CHANNELS)
	{
		hopping_frequency_no = 0;
		return SCORPIO_PACKET_PERIOD - 2*SCORPIO_PACKETCH_PERIOD;
	}
	return SCORPIO_PACKETCH_PERIOD;
}

static void __attribute__((unused)) SCORPIO_RF_init()
{
	/* Initialise CYRF chip */
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME,	0x3C);
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET,	0x14);
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB,	0x55);
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB,	0x05);
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG,		0xE8);
	CYRF_SetPower(0x28);
	CYRF_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) SCORPIO_TX_init()
{
	calc_fh_channels(3);	// select 3 frequencies between 2 and 77. FIXME: Could they be choosen on the spot finding empty frequencies?
	crc = (rx_tx_addr[0] ^ rx_tx_addr[1] ^ RX_num) + ((rx_tx_addr[2] ^ rx_tx_addr[3] ^ RX_num) << 8);

	#ifdef SCORPIO_FORCE_ID
		crc = 0x689C;
		hopping_frequency[0] = 0x26;
		hopping_frequency[1] = 0x49;
		hopping_frequency[2] = 0x2E;
	#endif
	//debugln("C0:%02X, C1:%02X, C2:%02X, CRC:%04X", hopping_frequency[0], hopping_frequency[1], hopping_frequency[2], crc);
	CYRF_ConfigRFChannel(hopping_frequency[0]);	// Use first RF channel for bind
}

uint16_t SCORPIO_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(SCORPIO_PACKET_PERIOD);
	#endif
	if(bind_counter)
		if(--bind_counter==0)
		{
			CYRF_ConfigCRCSeed(crc);
			BIND_DONE;
		}
	return SCORPIO_send_packet();
}

void SCORPIO_init()
{
	SCORPIO_RF_init();
	SCORPIO_TX_init();
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter = SCORPIO_BIND_COUNT;
		CYRF_ConfigCRCSeed(0x0001);
	}
	else
	{
		bind_counter = 0;
		CYRF_ConfigCRCSeed(crc);
	}
	hopping_frequency_no = 0;
}

#endif
