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
//  ESky protocol for small models since 2014 (150, 300, 150X, ...)

#if defined(ESKY150_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define ESKY150_PAYLOADSIZE 15
#define ESKY150_TX_ADDRESS_SIZE 4
#define ESKY150_BINDING_PACKET_PERIOD	2000
#define ESKY150_SENDING_PACKET_PERIOD	4800

static void __attribute__((unused)) ESKY150_init()
{
	//Original TX always sets for channelx 0x22 and 0x4a
	// Use channels 2..79
	hopping_frequency[0] = rx_tx_addr[3]%37+2;
	hopping_frequency[1] = hopping_frequency[0] + 40;

	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, (_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO))); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);   // 4-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
	NRF24L01_SetPower();
	NRF24L01_SetBitrate(NRF24L01_BR_2M);
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, ESKY150_PAYLOADSIZE);   // bytes of data payload for pipe 0
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, ESKY150_TX_ADDRESS_SIZE);

	NRF24L01_Activate(0x73);
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
	// Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, _BV(NRF2401_1D_EN_DPL) | _BV(NRF2401_1D_EN_ACK_PAY) | _BV(NRF2401_1D_EN_DYN_ACK));
	NRF24L01_Activate(0x73);
	NRF24L01_FlushTx();
	// Turn radio power on
	NRF24L01_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) ESKY150_bind_init()
{
	uint8_t ESKY150_addr[ESKY150_TX_ADDRESS_SIZE] = { 0x73, 0x73, 0x74, 0x63 }; //This RX address "sstc" is fixed for ESky2

	// Build packet
	packet[0]  = rx_tx_addr[0];
	packet[1]  = rx_tx_addr[1];
	packet[2]  = rx_tx_addr[2];
	packet[3]  = rx_tx_addr[3]; 
	packet[4]  = ESKY150_addr[0];
	packet[5]  = ESKY150_addr[1];
	packet[6]  = ESKY150_addr[2];
	packet[7]  = ESKY150_addr[3];
	packet[8]  = rx_tx_addr[0];
	packet[9]  = rx_tx_addr[1];
	packet[10] = rx_tx_addr[2];
	packet[11] = rx_tx_addr[3]; 
	packet[12] = 0;
	packet[13] = 0;
	packet[14] = 0;

	// Bind address
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, ESKY150_addr, ESKY150_TX_ADDRESS_SIZE);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, ESKY150_addr, ESKY150_TX_ADDRESS_SIZE);
	
	// Bind Channel 1
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 1);
}

static void __attribute__((unused)) ESKY150_send_packet()
{
	// Build packet
	uint16_t throttle=convert_channel_16b_limit(THROTTLE,1000,2000);
	uint16_t aileron=convert_channel_16b_limit(AILERON,1000,2000);
	uint16_t elevator=convert_channel_16b_limit(ELEVATOR,1000,2000);
	uint16_t rudder=convert_channel_16b_limit(RUDDER,1000,2000);
	//set unused channels to zero, for compatibility with older 4 channel models
	uint8_t flight_mode=0;
	uint16_t aux_ch6=0;
	uint8_t aux_ch7=0;
	if(option==1)
	{
		flight_mode=ESKY150_convert_2bit_channel(CH5);
		aux_ch6=convert_channel_16b_limit(CH6,1000,2000);
		aux_ch7=ESKY150_convert_2bit_channel(CH7);
	}
	packet[0]  = hopping_frequency[0];
	packet[1]  = hopping_frequency[1];
	packet[2]  = ((flight_mode << 6) & 0xC0) | ((aux_ch7 << 4) & 0x30) | ((throttle >> 8) & 0xFF);
	packet[3]  = throttle & 0xFF;
	packet[4]  = ((aux_ch6 >> 4) & 0xF0) | ((aileron >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
	packet[5]  = aileron  & 0xFF;
	packet[6]  = (aux_ch6 & 0xF0) | ((elevator >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
	packet[7]  = elevator & 0xFF;
	packet[8]  = ((aux_ch6 << 4) & 0xF0) | ((rudder >> 8) & 0xFF); //and 0xFF works as values are anyways not bigger than 12 bits, but faster code like that
	packet[9]  = rudder & 0xFF;
	// The next 4 Bytes are sint8 trim values (TAER). As trims are already included within normal outputs, these values are set to zero.
	packet[10] = 0x00;
	packet[11] = 0x00;
	packet[12] = 0x00;
	packet[13] = 0x00;
	// Calculate checksum:
	uint8_t sum = 0;
	for (uint8_t i = 0; i < 14; ++i)
		sum += packet[i];
	packet[14] = sum;

	// Hop on 2 channels
	hopping_frequency_no++;
	hopping_frequency_no&=0x01;
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);

	// Clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	// Send packet
	NRF24L01_WritePayload(packet, ESKY150_PAYLOADSIZE);

	//Keep transmit power updated
	NRF24L01_SetPower();
}

uint8_t ESKY150_convert_2bit_channel(uint8_t num)
{
	if(Channel_data[num] > CHANNEL_MAX_COMMAND)
		return 0x03;
	else
		if(Channel_data[num] < CHANNEL_MIN_COMMAND)
			return 0x00;
		else
			if(Channel_data[num] > CHANNEL_SWITCH)
				return 0x02;
	return 0x01;	
}

uint16_t ESKY150_callback()
{
	if(IS_BIND_DONE)
		ESKY150_send_packet();
	else
	{
		NRF24L01_WritePayload(packet, ESKY150_PAYLOADSIZE);
		if (--bind_counter == 0)
		{
			BIND_DONE;
			// Change TX address from bind to normal mode
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, ESKY150_TX_ADDRESS_SIZE);
		}
		return ESKY150_BINDING_PACKET_PERIOD;
	}
	return ESKY150_SENDING_PACKET_PERIOD;
}

uint16_t initESKY150(void)
{
	ESKY150_init();
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter=3000;
		ESKY150_bind_init();
	}
	hopping_frequency_no=0;
	return 10000;
}

#endif