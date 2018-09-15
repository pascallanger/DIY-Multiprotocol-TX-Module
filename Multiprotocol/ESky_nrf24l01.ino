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
// Last sync with hexfet new_protocols/esky_nrf24l01.c dated 2015-02-13

#if defined(ESKY_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define ESKY_BIND_COUNT		1000
#define ESKY_PACKET_PERIOD	3333
#define ESKY_PAYLOAD_SIZE	13
#define ESKY_PACKET_CHKTIME	100 // Time to wait for packet to be sent (no ACK, so very short)

static void __attribute__((unused)) ESKY_set_data_address()
{
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);     // 4-byte RX/TX address for regular packets
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 4);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr, 4);
}

static void __attribute__((unused)) ESKY_init()
{
	NRF24L01_Initialize();

	// 2-bytes CRC, radio off
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);            // No Auto Acknowledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);        // Enable data pipe 0
	if (IS_BIND_IN_PROGRESS)
	{
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);     // 3-byte RX/TX address for bind packets
		NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t*)"\x00\x00\x00", 3);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (uint8_t*)"\x00\x00\x00", 3);
	}
	else
		ESKY_set_data_address();
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);          // No auto retransmission
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 50);              // Channel 50 for bind packets
	NRF24L01_SetBitrate(NRF24L01_BR_1M);                   // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);           // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, ESKY_PAYLOAD_SIZE);  // bytes of data payload for pipe 0
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, ESKY_PAYLOAD_SIZE);
	NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, ESKY_PAYLOAD_SIZE);
	NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, ESKY_PAYLOAD_SIZE);
	NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, ESKY_PAYLOAD_SIZE);
	NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, ESKY_PAYLOAD_SIZE);
	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00);      // Just in case, no real bits to write here
}

static void __attribute__((unused)) ESKY_init2()
{
	NRF24L01_FlushTx();
	hopping_frequency_no = 0;
	uint16_t channel_ord = rx_tx_addr[0] % 74;
	hopping_frequency[12] = 10 + (uint8_t)channel_ord;	//channel_code
	uint8_t channel1, channel2;
	channel1 = 10 + (uint8_t)((37 + channel_ord*5) % 74);
	channel2 = 10 + (uint8_t)((     channel_ord*5) % 74) ;

	hopping_frequency[0] = channel1;
	hopping_frequency[1] = channel1;
	hopping_frequency[2] = channel1;
	hopping_frequency[3] = channel2;
	hopping_frequency[4] = channel2;
	hopping_frequency[5] = channel2;

	//end_bytes
	hopping_frequency[6] = 6;
	hopping_frequency[7] = channel1*2;
	hopping_frequency[8] = channel2*2;
	hopping_frequency[9] = 6;
	hopping_frequency[10] = channel1*2;
	hopping_frequency[11] = channel2*2;

	// Turn radio power on
	NRF24L01_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) ESKY_send_packet(uint8_t bind)
{
	uint8_t rf_ch = 50; // bind channel
	if (bind)
	{
		// Bind packet
		packet[0]  = rx_tx_addr[2];
		packet[1]  = rx_tx_addr[1];
		packet[2]  = rx_tx_addr[0];
		packet[3]  = hopping_frequency[12]; // channel_code encodes pair of channels to transmit on
		packet[4]  = 0x18;
		packet[5]  = 0x29;
		packet[6]  = 0;
		packet[7]  = 0;
		packet[8]  = 0;
		packet[9]  = 0;
		packet[10] = 0;
		packet[11] = 0;
		packet[12] = 0;
	}
	else
	{
		// Regular packet
		// Each data packet is repeated 3 times on one channel, and 3 times on another channel
		// For arithmetic simplicity, channels are repeated in rf_channels array
		if (hopping_frequency_no == 0)
			for (uint8_t i = 0; i < 6; i++)
			{
				uint16_t val=convert_channel_ppm(CH_AETR[i]);
				packet[i*2]   = val>>8;		//high byte of servo timing(1000-2000us)
				packet[i*2+1] = val&0xFF;	//low byte of servo timing(1000-2000us)
			}
		rf_ch = hopping_frequency[hopping_frequency_no];
		packet[12] = hopping_frequency[hopping_frequency_no+6];	// end_bytes
		hopping_frequency_no++;
		if (hopping_frequency_no > 6) hopping_frequency_no = 0;
	}
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, ESKY_PAYLOAD_SIZE);
	NRF24L01_SetPower();	//Keep transmit power updated
}

uint16_t ESKY_callback()
{
	if(IS_BIND_DONE)
		ESKY_send_packet(0);
	else
	{
		ESKY_send_packet(1);
		if (--bind_counter == 0)
		{
			ESKY_set_data_address();
			BIND_DONE;
		}
	}
	return ESKY_PACKET_PERIOD;
}

uint16_t initESKY(void)
{
	bind_counter = ESKY_BIND_COUNT;
	rx_tx_addr[2] = rx_tx_addr[3];	// Model match
	rx_tx_addr[3] = 0xBB;
	ESKY_init();
	ESKY_init2();
	return 50000;
}

#endif