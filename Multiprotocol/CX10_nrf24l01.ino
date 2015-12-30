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
// compatible with Cheerson CX-10 blue & newer red pcb, CX-10A, CX11, CX-10 green pcb, DM007, Floureon FX-10, CX-Stars

#if defined(CX10_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CX10_BIND_COUNT 4360   // 6 seconds
#define CX10_PACKET_SIZE 15
#define CX10A_PACKET_SIZE 19       // CX10 blue board packets have 19-byte payload
#define CX10_PACKET_PERIOD   1316  // Timeout for callback in uSec
#define CX10A_PACKET_PERIOD  6000

#define INITIAL_WAIT     500

// flags
#define CX10_FLAG_FLIP       0x10 // goes to rudder channel
#define CX10_FLAG_MODE_MASK  0x03
#define CX10_FLAG_HEADLESS   0x04
// flags2
#define CX10_FLAG_VIDEO      0x02
#define CX10_FLAG_SNAPSHOT   0x04

// frequency channel management
#define RF_BIND_CHANNEL 0x02
#define NUM_RF_CHANNELS    4

enum {
    CX10_INIT1 = 0,
    CX10_BIND1,
    CX10_BIND2,
    CX10_DATA
};

void CX10_Write_Packet(uint8_t bind)
{
	uint8_t offset = 0;
	if(sub_protocol == CX10_BLUE)
		offset = 4;
	packet[0] = bind ? 0xAA : 0x55;
	packet[1] = rx_tx_addr[0];
	packet[2] = rx_tx_addr[1];
	packet[3] = rx_tx_addr[2];
	packet[4] = rx_tx_addr[3];
	// packet[5] to [8] (aircraft id) is filled during bind for blue board
	packet[5+offset] = lowByte(Servo_data[AILERON]);
	packet[6+offset]= highByte(Servo_data[AILERON]);
	packet[7+offset]= lowByte(Servo_data[ELEVATOR]);
	packet[8+offset]= highByte(Servo_data[ELEVATOR]);
	packet[9+offset]= lowByte(Servo_data[THROTTLE]);
	packet[10+offset]= highByte(Servo_data[THROTTLE]);
	packet[11+offset]= lowByte(Servo_data[RUDDER]);
	packet[12+offset]= highByte(Servo_data[RUDDER]);
	
    // Channel 5 - flip flag
	if(Servo_data[AUX1] > PPM_SWITCH)
		packet[12+offset] |= CX10_FLAG_FLIP; // flip flag

    // Channel 6 - mode
	if(Servo_data[AUX2] > PPM_MAX_COMMAND)		// mode 3 / headless on CX-10A
		packet[13+offset] = 0x02;
	else
		if(Servo_data[AUX2] < PPM_MIN_COMMAND)
			packet[13+offset] = 0x00;			// mode 1
		else
			packet[13+offset] = 0x01;			// mode 2

	flags=0;
	if(sub_protocol == DM007)
	{
		// Channel 7 - snapshot
		if(Servo_data[AUX3] > PPM_SWITCH)
			flags |= CX10_FLAG_SNAPSHOT;
		// Channel 8 - video
		if(Servo_data[AUX4] > PPM_SWITCH)
			flags |= CX10_FLAG_VIDEO;
		// Channel 9 - headless
		if(Servo_data[AUX5] > PPM_SWITCH)
			packet[13+offset] |= CX10_FLAG_HEADLESS;
	}
	packet[14+offset] = flags;

	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
	else
	{
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
		hopping_frequency_no %= NUM_RF_CHANNELS;
	}
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	XN297_WritePayload(packet, packet_length);
	NRF24L01_SetPower();
}

void CX10_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc",5);
	XN297_SetRXAddr((uint8_t *)"\xcc\xcc\xcc\xcc\xcc",5);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_length);	// rx pipe 0 (used only for blue board)
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
	NRF24L01_SetPower();
}

uint16_t CX10_callback() {
	switch (phase) {
		case CX10_INIT1:
			phase = bind_phase;
			break;
		case CX10_BIND1:
			if (bind_counter == 0)
			{
				phase = CX10_DATA;
				BIND_DONE;
			}
			else
			{
				CX10_Write_Packet(1);
				bind_counter--;
			}
			break;
		case CX10_BIND2:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				XN297_ReadPayload(packet, packet_length);
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				if(packet[9] == 1)
					phase = CX10_BIND1;
			}
			else
			{
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				CX10_Write_Packet(1);
				delay(1);						// used to be 300µs in deviation but not working so 1ms now
				// switch to RX mode
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_FlushRx();
				NRF24L01_SetTxRxMode(RX_EN);
				XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP) | BV(NRF24L01_00_PRIM_RX));
			}
			break;
		case CX10_DATA:
			CX10_Write_Packet(0);
			break;
	}
	return packet_period;
}

void initialize_txid()
{
	rx_tx_addr[1]%= 0x30;
    hopping_frequency[0] = 0x03 + (rx_tx_addr[0] & 0x0F);
    hopping_frequency[1] = 0x16 + (rx_tx_addr[0] >> 4);
    hopping_frequency[2] = 0x2D + (rx_tx_addr[1] & 0x0F);
    hopping_frequency[3] = 0x40 + (rx_tx_addr[1] >> 4);
}

uint16_t initCX10(void)
{
	switch(sub_protocol)
	{
		case CX10_GREEN:
        case DM007:
            packet_length = CX10_PACKET_SIZE;
            packet_period = CX10_PACKET_PERIOD;
            bind_phase = CX10_BIND1;
            bind_counter = CX10_BIND_COUNT;
			break;
		case CX10_BLUE:
            packet_length = CX10A_PACKET_SIZE;
            packet_period = CX10A_PACKET_PERIOD;
            bind_phase = CX10_BIND2;
            bind_counter=0;
			for(uint8_t i=0; i<4; i++)
				packet[5+i] = 0xff; // clear aircraft id
            packet[9] = 0;
			break;
	}
	initialize_txid();
	CX10_init();
	phase = CX10_INIT1;
	BIND_IN_PROGRESS;	// autobind protocol
	return INITIAL_WAIT;
}

#endif
