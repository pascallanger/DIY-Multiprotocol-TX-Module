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
// Last sync with hexfet new_protocols/fy326_nrf24l01.c dated 2015-07-29

#if defined(FY326_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define FY326_INITIAL_WAIT		500
#define FY326_PACKET_PERIOD		1500
#define FY326_PACKET_CHKTIME	300
#define FY326_PACKET_SIZE		15
#define FY326_BIND_COUNT		16
#define FY326_RF_BIND_CHANNEL	0x17
#define FY326_NUM_RF_CHANNELS	5

enum {
    FY326_BIND1=0,
    FY326_BIND2,
    FY326_DATA,
    FY319_BIND1,
    FY319_BIND2,
};

#define rxid channel

#define CHAN_TO_TRIM(chanval) ((chanval/10)-10)
static void __attribute__((unused)) FY326_send_packet(uint8_t bind)
{
	packet[0] = rx_tx_addr[3];
	if(bind)
		packet[1] = 0x55;
	else
		packet[1] =	  GET_FLAG(CH7_SW,	0x80)	// Headless
					| GET_FLAG(CH6_SW,	0x40)	// RTH
					| GET_FLAG(CH5_SW,	0x02)	// Flip
					| GET_FLAG(CH9_SW,	0x01)	// Calibrate
					| GET_FLAG(CH8_SW,	0x04);	// Expert
	packet[2]  = convert_channel_16b_limit(AILERON, 0, 200);	// aileron
	packet[3]  = convert_channel_16b_limit(ELEVATOR, 0, 200);		// elevator
	packet[4]  = convert_channel_16b_limit(RUDDER, 0, 200);	// rudder
	packet[5]  = convert_channel_16b_limit(THROTTLE, 0, 200);		// throttle
	if(sub_protocol==FY319)
	{
		packet[6] = convert_channel_8b(AILERON);
		packet[7] = convert_channel_8b(ELEVATOR);
		packet[8] = convert_channel_8b(RUDDER);
	}
	else
	{
		packet[6]  = rx_tx_addr[0];
		packet[7]  = rx_tx_addr[1];
		packet[8]  = rx_tx_addr[2];
	}
	packet[9]  = CHAN_TO_TRIM(packet[2]);	// aileron_trim;
	packet[10] = CHAN_TO_TRIM(packet[3]);	// elevator_trim;
	packet[11] = CHAN_TO_TRIM(packet[4]);	// rudder_trim;
	packet[12] = 0;							// throttle_trim;
	packet[13] = rxid;
	packet[14] = rx_tx_addr[4];

	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, FY326_RF_BIND_CHANNEL);
	else
	{
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
		hopping_frequency_no %= FY326_NUM_RF_CHANNELS;
	}

	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();

	NRF24L01_WritePayload(packet, FY326_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) FY326_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	if(sub_protocol==FY319)
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // Five-byte rx/tx address
	else
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);   // Three-byte rx/tx address
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (uint8_t *)"\x15\x59\x23\xc6\x29", 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *)"\x15\x59\x23\xc6\x29", 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowledgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, FY326_PACKET_SIZE);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, FY326_RF_BIND_CHANNEL);
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
    NRF24L01_SetPower();

	NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
	NRF24L01_Activate(0x73);

	//Switch to RX
	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_FlushRx();
	NRF24L01_SetTxRxMode(RX_EN);
}

uint16_t FY326_callback()
{
	switch (phase)
	{
		case FY319_BIND1:
			if(NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{
				NRF24L01_ReadPayload(packet, FY326_PACKET_SIZE);
				rxid = packet[13];
				packet[0] = rx_tx_addr[3];
				packet[1] = 0x80;
				packet[14]= rx_tx_addr[4];
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
				NRF24L01_FlushTx();
				bind_counter = 255;
				for(uint8_t i=2; i<6; i++)
					packet[i] = hopping_frequency[0];
				phase = FY319_BIND2;
			}
			return FY326_PACKET_CHKTIME;
			break;
		case FY319_BIND2:
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			NRF24L01_FlushTx();
			NRF24L01_WritePayload(packet, FY326_PACKET_SIZE);
			if(bind_counter == 250)
				packet[1] = 0x40;
			if(--bind_counter == 0)
			{
				BIND_DONE;
				phase = FY326_DATA;
			}
			break;
		case FY326_BIND1:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				NRF24L01_ReadPayload(packet, FY326_PACKET_SIZE);
				rxid = packet[13];
				rx_tx_addr[0] = 0xAA;
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				BIND_DONE;
				phase = FY326_DATA;
			}
			else
				if (bind_counter-- == 0)
				{
					bind_counter = FY326_BIND_COUNT;
					NRF24L01_SetTxRxMode(TXRX_OFF);
					NRF24L01_SetTxRxMode(TX_EN);
					FY326_send_packet(1);
					phase = FY326_BIND2;
					return FY326_PACKET_CHKTIME;
				}
			break;
		case FY326_BIND2:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_TX_DS))
			{ // TX data sent -> switch to RX mode
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_FlushRx();
				NRF24L01_SetTxRxMode(RX_EN);
				phase = FY326_BIND1;
			}
			else
				return FY326_PACKET_CHKTIME;
			break;
		case FY326_DATA:
			FY326_send_packet(0);
			break;
	}
	return FY326_PACKET_PERIOD;
}

static void __attribute__((unused)) FY326_initialize_txid()
{
	hopping_frequency[0] = 		  (rx_tx_addr[0]&0x0f);
	hopping_frequency[1] = 0x10 + (rx_tx_addr[0] >> 4);
	hopping_frequency[2] = 0x20 + (rx_tx_addr[1]&0x0f);
	hopping_frequency[3] = 0x30 + (rx_tx_addr[1] >> 4);
	hopping_frequency[4] = 0x40 + (rx_tx_addr[2] >> 4);
	if(sub_protocol==FY319)
		for(uint8_t i=0;i<5;i++)
			hopping_frequency[i]=rx_tx_addr[0] & ~0x80;
}

uint16_t initFY326(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    rxid = 0xAA;
	bind_counter = FY326_BIND_COUNT;
	FY326_initialize_txid();
	FY326_init();
	if(sub_protocol==FY319)
	{
		phase=FY319_BIND1;
	}
	else
		phase=FY326_BIND1;
	return	FY326_INITIAL_WAIT;
}

#endif
