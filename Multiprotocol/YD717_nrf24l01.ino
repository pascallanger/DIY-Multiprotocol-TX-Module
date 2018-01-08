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
// Last sync with hexfet new_protocols/yd717_nrf24l01.c dated 2015-09-28

#if defined(YD717_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define YD717_BIND_COUNT		120
#define YD717_PACKET_PERIOD		8000	// Timeout for callback in uSec, 8ms=8000us for YD717
#define YD717_INITIAL_WAIT		50000	// Initial wait before starting callbacks

// Stock tx fixed frequency is 0x3C. Receiver only binds on this freq.
#define YD717_RF_CHANNEL 0x3C

#define YD717_FLAG_FLIP     0x0F
#define YD717_FLAG_LIGHT    0x80
#define YD717_FLAG_PICTURE  0x40
#define YD717_FLAG_VIDEO    0x20
#define YD717_FLAG_HEADLESS 0x10

#define YD717_PAYLOADSIZE 8				// receive data pipes set to this size, but unused

static void __attribute__((unused)) yd717_send_packet(uint8_t bind)
{
	uint8_t rudder_trim, elevator_trim, aileron_trim;
	if (bind)
	{
		packet[0]= rx_tx_addr[0]; // send data phase address in first 4 bytes
		packet[1]= rx_tx_addr[1];
		packet[2]= rx_tx_addr[2];
		packet[3]= rx_tx_addr[3];
		packet[4] = 0x56;
		packet[5] = 0xAA;
		packet[6] = (sub_protocol == NIHUI) ? 0x00 : 0x32;
		packet[7] = 0x00;
	}
	else
	{
		// Throttle
		packet[0] = convert_channel_8b(THROTTLE);
		// Rudder
		if( sub_protocol==XINXUN )
		{
			rudder = convert_channel_8b(RUDDER);
			rudder_trim = (0xff - rudder) >> 1;
		}
		else
		{
			rudder = 0xff - convert_channel_8b(RUDDER);
			rudder_trim = rudder >> 1;
		}
		packet[1] = rudder;
		// Elevator
		elevator = convert_channel_8b(ELEVATOR);
		elevator_trim = elevator >> 1;
		packet[3] = elevator;
		// Aileron
		aileron = 0xff - convert_channel_8b(AILERON);
		aileron_trim = aileron >> 1;
		packet[4] = aileron;
		// Trims
		if( sub_protocol == YD717 )
		{
			packet[2] = elevator_trim;
			packet[5] = aileron_trim;
			packet[6] = rudder_trim;
		}
		else
		{
			packet[2] = rudder_trim;
			packet[5] = elevator_trim;
			packet[6] = aileron_trim;
		}
		// Flags
		flags=0;
		// Channel 5
		if (CH5_SW)	flags = YD717_FLAG_FLIP;
		// Channel 6
		if (CH6_SW)	flags |= YD717_FLAG_LIGHT;
		// Channel 7
		if (CH7_SW)	flags |= YD717_FLAG_PICTURE;
		// Channel 8
		if (CH8_SW)	flags |= YD717_FLAG_VIDEO;
		// Channel 9
		if (CH9_SW)	flags |= YD717_FLAG_HEADLESS;
		packet[7] = flags;
	}

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();

	if( sub_protocol == YD717 )
		NRF24L01_WritePayload(packet, 8);
	else
	{
		packet[8] = packet[0];  // checksum
		for(uint8_t i=1; i < 8; i++)
			packet[8] += packet[i];
		packet[8] = ~packet[8];
		NRF24L01_WritePayload(packet, 9);
	}

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) yd717_init()
{
	NRF24L01_Initialize();

	// CRC, radio on
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_PWR_UP)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);				// Enable Acknowledgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);			// Enable all data pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);			// 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A);		// 500uS retransmit t/o, 10 tries
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, YD717_RF_CHANNEL);	// Channel 3C
	NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent and retransmit

	NRF24L01_Activate(0x73);								// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);				// Enable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);			// Set feature bits on
	NRF24L01_Activate(0x73);

	// for bind packets set address to prearranged value known to receiver
	uint8_t bind_rx_tx_addr[5];
	uint8_t offset=5;
	if( sub_protocol==SYMAX4 )
		offset=0;
	else
		if( sub_protocol==NIHUI )
			offset=4;
	for(uint8_t i=0; i < 5; i++)
		bind_rx_tx_addr[i]  = 0x60 + offset;
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_rx_tx_addr, 5);
}

uint16_t yd717_callback()
{
	if(IS_BIND_DONE)
		yd717_send_packet(0);
	else
	{
		if (bind_counter == 0)
		{
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);	// set address
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
			yd717_send_packet(0);
			BIND_DONE;							// bind complete
		}
		else
		{
			yd717_send_packet(1);
			bind_counter--;
		}
	}
	return YD717_PACKET_PERIOD;						// Packet every 8ms
}

uint16_t initYD717()
{
	BIND_IN_PROGRESS;			// autobind protocol
	rx_tx_addr[4] = 0xC1;		// always uses first data port
	yd717_init();
	bind_counter = YD717_BIND_COUNT;

	// Call callback in 50ms
	return YD717_INITIAL_WAIT;
}

#endif