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

#if defined(POTENSIC_NRF24L01_INO)

#include "iface_nrf24l01.h"

//#define FORCE_POTENSIC_ORIGINAL_ID

#define POTENSIC_PACKET_PERIOD		4100 // Timeout for callback in uSec
#define POTENSIC_INITIAL_WAIT		500
#define POTENSIC_PACKET_SIZE		10
#define POTENSIC_BIND_COUNT			400
#define POTENSIC_RF_NUM_CHANNELS	4

static void __attribute__((unused)) POTENSIC_set_checksum()
{
	uint8_t checksum = packet[1];
	for(uint8_t i=2; i<POTENSIC_PACKET_SIZE-2; i++)
		checksum += packet[i];
	packet[8] |= checksum & 0x0f;
}

static void __attribute__((unused)) POTENSIC_send_packet()
{
	packet[8]=0;
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0x61;
		memcpy(&packet[1],rx_tx_addr,5);
		packet[6] = 0x20;
		packet[7] = 0xC0;
	}
	else
	{ 
		packet[0] = 0x64;
		// Deadband is needed on throttle to emulate the spring to neutral otherwise the quad behaves weirdly, 160 gives +-20%
		packet[1] = convert_channel_8b_limit_deadband(THROTTLE,0x00,0x19,0x32,160)<<1;	// Throttle 00..19..32 *2
		uint8_t elevator=convert_channel_8b(ELEVATOR)>>3;			
		packet[2] = ((255-convert_channel_8b(RUDDER))&0xF8)|(elevator>>2);
		packet[3] = (elevator<<6)|(((255-convert_channel_8b(AILERON))>>2)&0xFE);
		packet[4] = 0x20;	// Trim
		packet[5] = 0x20	// Trim
					| GET_FLAG(CH7_SW, 0x80);			// High: +100%
		packet[6] = 0x20;	// Trim
		packet[7] = 0x40								// Low: -100%
					| GET_FLAG((Channel_data[CH7] > CHANNEL_MIN_COMMAND && !CH7_SW), 0x80)	// Medium: 0%
					| GET_FLAG((CH5_SW||CH6_SW), 0x02)	// Momentary Take off/Landing + Emergency
					| GET_FLAG(CH8_SW, 0x04);			// Headless: -100%=off,+100%=on
		packet[8] = GET_FLAG(CH6_SW, 0x80);				// Emergency
	}
	POTENSIC_set_checksum();
	packet[9] = hopping_frequency_no;
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no&0x03]);
	hopping_frequency_no++;
	// Power on, TX mode, 2byte CRC
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, POTENSIC_PACKET_SIZE);
	NRF24L01_SetPower();
}

static void __attribute__((unused)) POTENSIC_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	if(IS_BIND_IN_PROGRESS)
		XN297_SetTXAddr((uint8_t*)"\x01\x01\x01\x01\x06", 5);	// Bind address
	else
		XN297_SetTXAddr(rx_tx_addr,5);							// Normal address
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);		// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// set address length (5 bytes)
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);				// 1Mbps
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);			// Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
	NRF24L01_Activate(0x73);
}

static void __attribute__((unused)) POTENSIC_initialize_txid()
{
	#ifdef FORCE_POTENSIC_ORIGINAL_ID
		memcpy(rx_tx_addr,(uint8_t *)"\xF6\xE0\x20\x00\x0E",5);
	#endif
	memcpy(hopping_frequency,(uint8_t *)"\x32\x3E\x3A\x36",POTENSIC_RF_NUM_CHANNELS); //50, 62, 58, 54
}

uint16_t POTENSIC_callback()
{
	if(IS_BIND_IN_PROGRESS)
		if(--bind_counter==0)
		{
			BIND_DONE;
			XN297_SetTXAddr(rx_tx_addr,5);
		}
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(POTENSIC_PACKET_PERIOD);
	#endif
	POTENSIC_send_packet();
	return POTENSIC_PACKET_PERIOD;
}

uint16_t initPOTENSIC(void)
{
	bind_counter = POTENSIC_BIND_COUNT;
	POTENSIC_initialize_txid();
	POTENSIC_init();
	hopping_frequency_no = 0;
	return	POTENSIC_INITIAL_WAIT;
}

#endif
