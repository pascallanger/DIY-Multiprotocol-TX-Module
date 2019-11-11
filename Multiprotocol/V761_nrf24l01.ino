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
 
 Thanks to  Goebish ,Ported  from his deviation firmware
 */

#if defined(V761_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define V761_PACKET_PERIOD		7060 // Timeout for callback in uSec
#define V761_INITIAL_WAIT		500
#define V761_PACKET_SIZE		8
#define V761_BIND_COUNT			200

//Fx chan management
#define V761_BIND_FREQ			0x28
#define V761_RF_NUM_CHANNELS	3

enum 
   {
    V761_BIND1 = 0,
    V761_BIND2,
    V761_DATA
   };

static void __attribute__((unused)) V761_set_checksum()
{
	uint8_t checksum = packet[0];
	for(uint8_t i=1; i<V761_PACKET_SIZE-2; i++)
		checksum += packet[i];
	if(phase == V761_BIND1)
	{
		packet[6] = checksum ^ 0xff;
		packet[7] = packet[6];
	}
	else 
	{
		checksum += packet[6];
		packet[7] = checksum ^ 0xff;
	}
}


static void __attribute__((unused)) V761_send_packet()
{
	if(phase != V761_DATA)
	{
		packet[0] = rx_tx_addr[0];
		packet[1] = rx_tx_addr[1];
		packet[2] = rx_tx_addr[2];
		packet[3] = rx_tx_addr[3];  
		packet[4] = hopping_frequency[1];
		packet[5] = hopping_frequency[2];
		if(phase == V761_BIND2) 
			packet[6] = 0xf0; // ?
	}
	else
	{ 
		packet[0] = convert_channel_8b(THROTTLE);		// throttle       
		packet[1] = convert_channel_8b(RUDDER)>>1;		// rudder
		packet[2] = convert_channel_8b(ELEVATOR)>>1;	// elevator
		packet[3] = convert_channel_8b(AILERON)>>1;		// aileron
		packet[5] = (packet_count++ / 3)<<6;
		packet[4] = (packet[5] == 0x40) ? 0x1a : 0x20;

		// Channel 5 - Gyro mode is packet 5
		if(CH5_SW)    // Mode Expert Gyro off
			flags = 0x0c;
		else
			if(Channel_data[CH5] < CHANNEL_MIN_COMMAND)
				flags = 0x08;     // Beginer mode (Gyro on, yaw and pitch rate limited)
			else
				flags = 0x0a;     // Mid Mode ( Gyro on no rate limits)        
		packet[5] |= flags;
		packet[6] = 0x80; // unknown 

		//packet counter
		if(packet_count >= 12)
			packet_count = 0;
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
		if(hopping_frequency_no >= V761_RF_NUM_CHANNELS)
			hopping_frequency_no = 0;
	}
	V761_set_checksum();
	// Power on, TX mode, 2byte CRC
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, V761_PACKET_SIZE);
	NRF24L01_SetPower();
}

static void __attribute__((unused)) V761_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);		// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);		// set address length (4 bytes)
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);				// 1Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
}

static void __attribute__((unused)) V761_initialize_txid()
{
	// TODO: try arbitrary rx_tx_addr & frequencies (except hopping_frequency[0])
	switch(RX_num%3)
	{
		case 1:	//Dump from air on Protonus TX
			memcpy(rx_tx_addr,(uint8_t *)"\xE8\xE4\x45\x09",4);
			memcpy(hopping_frequency,(uint8_t *)"\x0D\x21\x44",3);
			break;
		case 2:	//Dump from air on mshagg2 TX
			memcpy(rx_tx_addr,(uint8_t *)"\xAE\xD1\x45\x09",4);
			memcpy(hopping_frequency,(uint8_t *)"\x13\x1D\x4A",3);
			break;
		default: //Dump from SPI
			memcpy(rx_tx_addr,(uint8_t *)"\x6f\x2c\xb1\x93",4);
			memcpy(hopping_frequency,(uint8_t *)"\x14\x1e\x4b",3);
			break;
	}
}

uint16_t V761_callback()
{
	switch(phase)
	{
		case V761_BIND1:
			if(bind_counter) 
				bind_counter--;
			packet_count ++;
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, V761_BIND_FREQ);
			XN297_SetTXAddr((uint8_t*)"\x34\x43\x10\x10", 4);
			V761_send_packet();
			if(packet_count >= 20) 
			{
				packet_count = 0;
				phase = V761_BIND2;
			}
			return 15730;
		case V761_BIND2:
			if(bind_counter) 
				bind_counter--;
			packet_count ++;
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[0]);
			XN297_SetTXAddr(rx_tx_addr, 4);
			V761_send_packet();
			if(bind_counter == 0) 
			{
				phase = V761_DATA;
				BIND_DONE;
			}
			else if(packet_count >= 20) 
			{
				packet_count = 0;
				phase = V761_BIND1;
			}
			return 15730;
		case V761_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(V761_PACKET_PERIOD);
			#endif
			V761_send_packet();
			break;
	}
	return V761_PACKET_PERIOD;
}

uint16_t initV761(void)
{
	BIND_IN_PROGRESS;
	bind_counter = V761_BIND_COUNT;
	V761_initialize_txid();
	phase = V761_BIND1;
	V761_init();
	hopping_frequency_no = 0;
	packet_count = 0;
	return	V761_INITIAL_WAIT;
}

#endif