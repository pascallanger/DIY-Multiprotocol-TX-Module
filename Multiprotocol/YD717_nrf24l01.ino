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

#define YD717_BIND_COUNT		60
#define YD717_PACKET_PERIOD		8000	// Timeout for callback in uSec, 8ms=8000us for YD717
#define YD717_INITIAL_WAIT		50000	// Initial wait before starting callbacks
#define YD717_PACKET_CHKTIME	500		// Time to wait if packet not yet acknowledged or timed out    

// Stock tx fixed frequency is 0x3C. Receiver only binds on this freq.
#define YD717_RF_CHANNEL 0x3C

#define YD717_FLAG_FLIP     0x0F
#define YD717_FLAG_LIGHT    0x80
#define YD717_FLAG_PICTURE  0x40
#define YD717_FLAG_VIDEO    0x20
#define YD717_FLAG_HEADLESS 0x10

#define YD717_PAYLOADSIZE 8				// receive data pipes set to this size, but unused

enum {
	YD717_INIT1 = 0,
	YD717_BIND2,
	YD717_BIND3,
	YD717_DATA
};

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
		if (Servo_AUX1)	flags = YD717_FLAG_FLIP;
		// Channel 6
		if (Servo_AUX2)	flags |= YD717_FLAG_LIGHT;
		// Channel 7
		if (Servo_AUX3)	flags |= YD717_FLAG_PICTURE;
		// Channel 8
		if (Servo_AUX4)	flags |= YD717_FLAG_VIDEO;
		// Channel 9
		if (Servo_AUX5)	flags |= YD717_FLAG_HEADLESS;
		packet[7] = flags;
	}

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
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
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_PWR_UP)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto Acknoledgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A); // 500uS retransmit t/o, 10 tries
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, YD717_RF_CHANNEL);      // Channel 3C
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
	NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
	NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
	NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, YD717_PAYLOADSIZE);   // bytes of data payload for pipe 1
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, YD717_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, YD717_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, YD717_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, YD717_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, YD717_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

	NRF24L01_Activate(0x73);						  // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Set feature bits on
	NRF24L01_Activate(0x73);

	// set tx id
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}

static void __attribute__((unused)) YD717_init1()
{
	// for bind packets set address to prearranged value known to receiver
	uint8_t bind_rx_tx_addr[] = {0x65, 0x65, 0x65, 0x65, 0x65};
	uint8_t i;
	if( sub_protocol==SYMAX4 )
		for(i=0; i < 5; i++)
			bind_rx_tx_addr[i]  = 0x60;
	else
		if( sub_protocol==NIHUI )
			for(i=0; i < 5; i++)
				bind_rx_tx_addr[i]  = 0x64;

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_rx_tx_addr, 5);
}

static void __attribute__((unused)) YD717_init2()
{
    // set rx/tx address for data phase
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}

uint16_t yd717_callback()
{
	switch (phase)
	{
		case YD717_INIT1:
			yd717_send_packet(0);	// receiver doesn't re-enter bind mode if connection lost...check if already bound
			phase = YD717_BIND3;
			break;
		case YD717_BIND2:
			if (counter == 0)
			{
				if (NRF24L01_packet_ack() == PKT_PENDING)
					return YD717_PACKET_CHKTIME;	// packet send not yet complete
				YD717_init2();						// change to data phase rx/tx address
				yd717_send_packet(0);
				phase = YD717_BIND3;
			}
			else
			{
				if (NRF24L01_packet_ack() == PKT_PENDING)
					return YD717_PACKET_CHKTIME;	// packet send not yet complete;
				yd717_send_packet(1);
				counter--;
			}
			break;
		case YD717_BIND3:
			switch (NRF24L01_packet_ack())
			{
				case PKT_PENDING:
					return YD717_PACKET_CHKTIME;	// packet send not yet complete
				case PKT_ACKED:
					phase = YD717_DATA;
					BIND_DONE;							// bind complete
					break;
				case PKT_TIMEOUT:
					YD717_init1();					// change to bind rx/tx address
					counter = YD717_BIND_COUNT;
					phase = YD717_BIND2;
					yd717_send_packet(1);
			}
			break;
		case YD717_DATA:
			if (NRF24L01_packet_ack() == PKT_PENDING)
				return YD717_PACKET_CHKTIME;		// packet send not yet complete
			yd717_send_packet(0);
			break;
	}	
	return YD717_PACKET_PERIOD;						// Packet every 8ms
}

uint16_t initYD717()
{
	rx_tx_addr[4] = 0xC1;	// always uses first data port
	yd717_init();
	phase = YD717_INIT1;	
	BIND_IN_PROGRESS;		// autobind protocol

	// Call callback in 50ms
	return YD717_INITIAL_WAIT;
}

#endif
