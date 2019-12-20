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
// Compatible with Tiger Drone 1400782.

#if defined(TIGER_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define TIGER_FORCE_ID

#define TIGER_INITIAL_WAIT    500
#define TIGER_PACKET_PERIOD   3940
#define TIGER_RF_NUM_CHANNELS 4
#define TIGER_BIND_RF_NUM_CHANNELS 8
#define TIGER_PAYLOAD_SIZE    16
#define TIGER_BIND_COUNT	  761		//3sec


static uint8_t __attribute__((unused)) TIGER_convert_channel(uint8_t num)
{
	uint8_t val=convert_channel_8b(num);
	// 7F..01=left, 00=center, 80..FF=right
	if(val==0x80)
		val=0;				// 0
	else
		if(val>0x80)
			val--;			// 80..FE
		else
		{
			val=0x80-val;	// 80..01
			if(val==0x80)
				val--;		// 7F..01
		}
	return val;
}

static void __attribute__((unused)) TIGER_send_packet()
{
	if(IS_BIND_DONE)
	{
		//Channels
		packet[0]=convert_channel_8b(THROTTLE);		// 00..FF
		packet[1]=TIGER_convert_channel(RUDDER);	// 7F..01=left, 00=center, 80..FF=right
		packet[2]=TIGER_convert_channel(ELEVATOR);	// 7F..01=down, 00=center, 80..FF=up
		packet[3]=TIGER_convert_channel(AILERON);	// 7F..01=left, 00=center, 80..FF=right
		//Flags
		packet[14]= GET_FLAG(CH5_SW, 0x04)			//FLIP
				  | GET_FLAG(CH6_SW, 0x10);			//LIGHT
	}
	//Check
	crc8=0;
	for(uint8_t i=0;i<TIGER_PAYLOAD_SIZE-1;i++)
		crc8+=packet[i];
	packet[TIGER_PAYLOAD_SIZE-1]=crc8;

	//Hopping frequency
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no>>1]);
	hopping_frequency_no++;
	if(IS_BIND_IN_PROGRESS)
	{
		if(hopping_frequency_no>=2*TIGER_BIND_RF_NUM_CHANNELS)
			hopping_frequency_no=0;
	}
	else
	{
		if(hopping_frequency_no>=2*(TIGER_BIND_RF_NUM_CHANNELS+TIGER_RF_NUM_CHANNELS))
			hopping_frequency_no=2*TIGER_BIND_RF_NUM_CHANNELS;
	}

	//Clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	//Send packet
	XN297_WritePayload(packet, TIGER_PAYLOAD_SIZE);
	//Set tx_power
	NRF24L01_SetPower();
}

static void __attribute__((unused)) TIGER_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t *)"\x68\x94\xA6\xD5\xC3", 5);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// No retransmits
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
	// Power on, TX mode, 2byte CRC
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
}

static void __attribute__((unused)) TIGER_initialize_txid()
{
#ifdef TIGER_FORCE_ID
	rx_tx_addr[0]=0x64;
	rx_tx_addr[1]=0x39;
	rx_tx_addr[2]=0x12;
	rx_tx_addr[3]=0x00;
	rx_tx_addr[4]=0x00;
	memcpy(hopping_frequency,"\x0E\x39\x1C\x07\x24\x3E\x2B\x47",TIGER_BIND_RF_NUM_CHANNELS);
	memcpy(&hopping_frequency[TIGER_BIND_RF_NUM_CHANNELS],"\x36\x41\x37\x4E",TIGER_RF_NUM_CHANNELS);
#endif
	//prepare bind packet
	memset(&packet[0], 0x00, 4);
	memset(&packet[4], 0x40, 10);
	memcpy(&packet[7], rx_tx_addr, 5);
	packet[14]=0xC0;
}

uint16_t TIGER_callback()
{
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(TIGER_PACKET_PERIOD);
	#endif
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr((uint8_t *)"\x49\xA6\x83\xEB\x4B", 5);
		}
	TIGER_send_packet();
	return TIGER_PACKET_PERIOD;
}

uint16_t initTIGER()
{
	BIND_IN_PROGRESS;	// autobind protocol
	TIGER_initialize_txid();
	TIGER_init();
	hopping_frequency_no = 0;
	bind_counter=TIGER_BIND_COUNT;
	return TIGER_INITIAL_WAIT;
}

#endif
/*Bind
- RF setup: 1Mbps, scrambled, CRC
- TX addr: 0x68 0x94 0xA6 0xD5 0xC3
- 8 RF channels: 0x0E 0x39 0x1C 0x07 0x24 0x3E 0x2B 0x47
- 2 packets per RF channel, 3940µs between packets
- payload 16 bytes: 0x00 0x00 0x00 0x00 0x40 0x40 0x40 0x64 0x39 0x12 0x00 0x00 0x40 0x40 0xC0 0xAF
- payload[15]=sum of payload[0..14]
- the only difference with normal packets is the payload[14]=0xC0
- ??? payload[7..11] TX ID ???

Normal
- RF setup: 1Mbps
- TX addr: 0x49 0xA6 0x83 0xEB 0x4B
- 4 RF channels: 0x36 0x41 0x37 0x4E
- 2 packets per RF channel, 3940µs between packets
- payload 16 bytes: 0x00 0x00 0x00 0x00 0x40 0x40 0x40 0x64 0x39 0x12 0x00 0x00 0x40 0x40 0x00 0xEF
- payload[15]=sum of payload[0..14]
- throttle is on payload[0] 00..FF
- rudder is on payload[1] 00=center, 80..FF=right, 01..7F=left
- elevator is on payload[2] 00=center, 80..FF=up, 01..7F=down
- aileron is on payload[3] 00=center, 80..FF=right, 01..7F=left
- trims payload[4..6]
- ??? payload[7..11] TX ID ???
- ??? payload[12..13] ???
- flip is on payload[14] and flag 0x04
- light is on payload[14] and flag 0x10
*/