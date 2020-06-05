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

#if defined(V2X2_RX_NRF24L01_INO)

#define V2X2_RX_PACKET_SIZE 16
#define V2X2_RX_RF_BIND_CHANNEL 0x08
#define V2X2_RX_RF_NUM_CHANNELS 5

enum {
	V2X2_RX_BIND,
	V2X2_RX_DATA
};

static void __attribute__((unused)) V2X2_Rx_init_nrf24l01()
{
	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5-byte RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, (uint8_t*)"\x66\x88\x68\x68\x68", 5);
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  	// Enable all data pipes
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, V2X2_RX_PACKET_SIZE);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, BAYANG_RX_RF_BIND_CHANNEL);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);             	// 1Mbps
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);		// reset registers status
	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_SetTxRxMode(RX_EN);
	// switch to RX mode
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
}

static uint8_t __attribute__((unused)) V2X2_Rx_check_validity()
{
	// check transmitter id

	// checksum

}

static void __attribute__((unused)) V2X2_Rx_build_telemetry_packet()
{

}

uint16_t initV2X2_Rx()
{
	V2X2_Rx_init_nrf24l01();
	
	phase = V2X2_RX_BIND;
	return 1000;
}

uint16_t V2X2_Rx_callback()
{
	switch (phase) {
		case V2X2_RX_BIND:
			// V2X2_set_tx_id();
			break;
		case V2X2_RX_DATA:

			break;
	}
	return 0;
}

#endif
