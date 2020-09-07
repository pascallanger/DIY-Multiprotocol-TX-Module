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

#if defined(NANORF_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define NANORF_PACKET_PERIOD		40000
#define NANORF_INITIAL_WAIT			500
#define NANORF_RF_CHANNEL			40
#define NANORF_PAYLOADSIZE			7

static void __attribute__((unused)) NANORF_send_packet()
{
	packet[0] = convert_channel_8b(AILERON);
	packet[1] = convert_channel_8b(ELEVATOR);
	packet[2] = convert_channel_8b(THROTTLE);
	packet[3] = convert_channel_8b(RUDDER);
	packet[4] = convert_channel_8b(CH5);
	packet[5] = convert_channel_8b(CH6);
	packet[6] = 0;
	for (uint8_t i=0; i < NANORF_PAYLOADSIZE-1; i++)
		packet[6] += packet[i];
	packet[6] += 0x55;

	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, NANORF_PAYLOADSIZE);

}

static void __attribute__((unused)) NANORF_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR , (uint8_t *)"Nano1",5);
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable all data pipes (even though not used?)
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // 4mS retransmit t/o, 15 tries (retries w/o AA?)
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, NANORF_RF_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);
	NRF24L01_SetPower();	// Set tx_power
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));  // 
}

uint16_t NANORF_callback()
{
	NANORF_send_packet();
	return NANORF_PACKET_PERIOD;
}

uint16_t initNANORF()
{	
	BIND_DONE;
	NANORF_init();
	return	NANORF_INITIAL_WAIT;
}

#endif
