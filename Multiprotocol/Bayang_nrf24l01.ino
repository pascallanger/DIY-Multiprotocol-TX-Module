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
// compatible with EAchine H8 mini, H10, BayangToys X6/X7/X9, JJRC JJ850 ...

#if defined(BAYANG_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define BAYANG_BIND_COUNT		1000
#define BAYANG_PACKET_PERIOD	2000
#define BAYANG_INITIAL_WAIT		500
#define BAYANG_PACKET_SIZE		15
#define BAYANG_RF_NUM_CHANNELS	4
#define BAYANG_RF_BIND_CHANNEL	0
#define BAYANG_ADDRESS_LENGTH	5

enum BAYANG_FLAGS {
    // flags going to packet[2]
    BAYANG_FLAG_RTH			= 0x01,
    BAYANG_FLAG_HEADLESS	= 0x02, 
    BAYANG_FLAG_FLIP		= 0x08 
};

enum BAYANG_PHASES {
    BAYANG_BIND = 0,
    BAYANG_DATA
};

void BAYANG_send_packet(uint8_t bind)
{
	uint8_t i;
	if (bind)
	{
		packet[0]= 0xA4;
		for(i=0;i<5;i++)
			packet[i+1]=rx_tx_addr[i];
		for(i=0;i<4;i++)
			packet[i+6]=hopping_frequency[i];
		packet[10] = rx_tx_addr[0];
		packet[11] = rx_tx_addr[1];
	}
	else
	{
		uint16_t val;
		packet[0] = 0xA5;
		packet[1] = 0xFA;		// normal mode is 0xf7, expert 0xfa

		//Flags
		packet[2] =0x00;
		if(Servo_data[AUX1] > PPM_SWITCH)
			packet[2] |= BAYANG_FLAG_FLIP;
		if(Servo_data[AUX2] > PPM_SWITCH)
			packet[2] |= BAYANG_FLAG_HEADLESS;
		if(Servo_data[AUX3] > PPM_SWITCH)
			packet[2] |= BAYANG_FLAG_RTH;
		
		packet[3] = 0x00;

		//Aileron
		val = convert_channel_10b(AILERON);
		packet[4] = (val>>8) + ((val>>2) & 0xFC);
		packet[5] = val & 0xFF;
		//Elevator
		val = convert_channel_10b(ELEVATOR);
		packet[6] = (val>>8) + ((val>>2) & 0xFC);
		packet[7] = val & 0xFF;
		//Throttle
		val = convert_channel_10b(THROTTLE);
		packet[8] = (val>>8) + 0x7C;
		packet[9] = val & 0xFF;
		//Rudder
		val = convert_channel_10b(RUDDER);
		packet[10] = (val>>8) + (val>>2 & 0xFC);
		packet[11] = val & 0xFF;
	}
	packet[12] = rx_tx_addr[2];
	packet[13] = 0x0A;
	packet[14] = 0;
    for (uint8_t i=0; i < BAYANG_PACKET_SIZE-1; i++)
		packet[14] += packet[i];

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RF_BIND_CHANNEL);
	else
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no%=BAYANG_RF_NUM_CHANNELS;
	
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, BAYANG_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

void BAYANG_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	XN297_SetTXAddr((uint8_t *)"\x00\x00\x00\x00\x00", BAYANG_ADDRESS_LENGTH);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
	NRF24L01_SetPower();

    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
}

uint16_t BAYANG_callback()
{
	switch (phase)
	{
		case BAYANG_BIND:
			if (bind_counter == 0)
			{
				XN297_SetTXAddr(rx_tx_addr, BAYANG_ADDRESS_LENGTH);
				phase = BAYANG_DATA;
				BIND_DONE;
			}
			else
			{
				BAYANG_send_packet(1);
				bind_counter--;
			}
			break;
		case BAYANG_DATA:
			BAYANG_send_packet(0);
			break;
	}
	return BAYANG_PACKET_PERIOD;
}

void BAYANG_initialize_txid()
{
	// Strange txid, rx_tx_addr and rf_channels could be anything so I will use on rx_tx_addr for all of them...
	// Strange also that there is no check of duplicated rf channels... I think we need to implement that later...
	for(uint8_t i=0; i<BAYANG_RF_NUM_CHANNELS; i++)
		hopping_frequency[i]=rx_tx_addr[i]%42;
	hopping_frequency_no=0;
}

uint16_t initBAYANG(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = BAYANG_BIND_COUNT;
	BAYANG_initialize_txid();
	phase=BAYANG_BIND;
	BAYANG_init();
	return BAYANG_INITIAL_WAIT+BAYANG_PACKET_PERIOD;
}

#endif
