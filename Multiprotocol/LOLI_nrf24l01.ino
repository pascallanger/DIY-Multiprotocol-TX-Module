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

#if defined(LOLI_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define LOLI_BIND_CHANNEL	33
#define LOLI_PACKET_SIZE	11
#define LOLI_NUM_CHANNELS	5

static void __attribute__((unused)) LOLI_init()
{
	NRF24L01_Initialize();
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      	// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  	// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5-bytes RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t*)"LOVE!", 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"LOVE!", 5);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// No retransmits
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, LOLI_PACKET_SIZE);	// RX FIFO size
	NRF24L01_SetBitrate(NRF24L01_BR_250K);             	// 250Kbps

	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     	// Clear data ready, data sent, and retransmit
	NRF24L01_SetTxRxMode(TX_EN);
}

// flags going to packet[1] for packet type 0xa2 (Rx config)
#define LOLI_FLAG_PWM7   0x02
#define LOLI_FLAG_PWM2   0x04
#define LOLI_FLAG_PWM1   0x08
#define LOLI_FLAG_SBUS   0x40
#define LOLI_FLAG_PPM    0x80

// flags going to packet[2] for packet type 0xa2 (Rx config)
#define LOLI_FLAG_SW8    0x01
#define LOLI_FLAG_SW7    0x02
#define LOLI_FLAG_SW6    0x04
#define LOLI_FLAG_SW5    0x08
#define LOLI_FLAG_SW4    0x10
#define LOLI_FLAG_SW3    0x20
#define LOLI_FLAG_SW2    0x40
#define LOLI_FLAG_SW1    0x80

static void __attribute__((unused)) LOLI_send_packet()
{
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0xa0;
		memcpy(&packet[1], hopping_frequency, LOLI_NUM_CHANNELS);
		memcpy(&packet[6], rx_tx_addr, 5);
		rf_ch_num = LOLI_BIND_CHANNEL;
	}
	else
	{
		if(LOLI_SerialRX)
		{// RX config
			packet[0] = 0xa2;
			packet[1] = LOLI_P1;	// CH1:LOLI_FLAG_PPM || LOLI_FLAG_PWM1, CH2:LOLI_FLAG_PWM2, CH5:LOLI_FLAG_SBUS, CH7:LOLI_FLAG_PWM7
			packet[2] = LOLI_P2;	// CHx switch bit(8-x)=1
			LOLI_SerialRX=false;
		}
		else
		{// Normal packet
			#ifdef FAILSAFE_ENABLE
				packet[0] = IS_FAILSAFE_VALUES_on ? 0xa0 : 0xa1;
			#else
				packet[0] = 0xa1;
			#endif

			//Build channels
			uint8_t ch=0, offset=1;
			uint16_t val;
			for(uint8_t i=0;i<2;i++)
			{
				val = convert_channel_10b(ch++, IS_FAILSAFE_VALUES_on);
				packet[offset++] = val >> 2;
				packet[offset  ] = val << 6;
				val = convert_channel_10b(ch++, IS_FAILSAFE_VALUES_on);
				packet[offset++]|= val >> 4;
				packet[offset  ] = val << 4;
				val = convert_channel_10b(ch++, IS_FAILSAFE_VALUES_on);
				packet[offset++]|= val >> 6;
				packet[offset  ] = val << 2;
				val = convert_channel_10b(ch++, IS_FAILSAFE_VALUES_on);
				packet[offset++]|= val >> 8;
				packet[offset++] = val & 0xff;
			}
			FAILSAFE_VALUES_off;	// Failsafe values are sent if they were available
		}

		if (++hopping_frequency_no > LOLI_NUM_CHANNELS-1)
			hopping_frequency_no = 0;
		rf_ch_num = hopping_frequency[hopping_frequency_no];
	}
	
	#if 0
		debug("P(%02X):",rf_ch_num);
		for(uint8_t i=0; i<LOLI_PACKET_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif
		
	//Send packet
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num);
	NRF24L01_SetPower();
	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0a);  // 8bit CRC, TX
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, LOLI_PACKET_SIZE);
}

enum{
	LOLI_BIND1,
	LOLI_BIND2,
	LOLI_BIND3,
	LOLI_PREP_DATA,
	LOLI_DATA1,
	LOLI_DATA2,
	LOLI_SET_RX_CONFIG,
	LOLI_SET_FAILSAFE
};

uint16_t LOLI_callback()
{
	switch (phase)
	{
		case LOLI_BIND1:
			if(bind_counter)
			{
				bind_counter--;
				if(bind_counter==0)
				{
					phase=LOLI_PREP_DATA;
					break;
				}
			}
			// send bind packet
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(TX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0a);  // 8bit CRC, TX
			LOLI_send_packet();
			phase++;
			return 2000;
		case LOLI_BIND2:
			// switch to RX mode
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_FlushRx();
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x3b);  // 8bit CRC, RX
			phase++;
			packet_count = 0;
			return 2000;
		case LOLI_BIND3:
			// got bind response ?
			if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{
				NRF24L01_ReadPayload(packet, LOLI_PACKET_SIZE);
				if (packet[0] == 'O' && packet[1] == 'K')
				{
					debugln("Bind OK");
					phase++;	// LOLI_PREP_DATA
					break;
				}
			}
			packet_count++;
			if (packet_count > 50)
				phase = LOLI_BIND1;
			return 1000;

		case LOLI_PREP_DATA:
			BIND_DONE;
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			NRF24L01_FlushRx();
			packet_count = 0;
			phase++;

		case LOLI_DATA1:
			#ifdef LOLI_HUB_TELEMETRY
				// Check telemetry
				if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
				{  // RX fifo data ready
					NRF24L01_ReadPayload(packet, LOLI_PACKET_SIZE);
					#if 0
						debug("T:");
						for(uint8_t i=0; i<LOLI_PACKET_SIZE; i++)
							debug(" %02X",packet[i]);
						debugln("");
					#endif
					RX_RSSI = packet[0]<<1;
					v_lipo1 = (packet[1] << 8) | packet[2];
					v_lipo2 = (packet[3] << 8) | packet[4];
					telemetry_link = 1;
					telemetry_counter++;			// TX LQI counter
					if(telemetry_lost)
					{
						telemetry_lost = 0;
						packet_count = 100;
						telemetry_counter = 100;
					}
				}
				//LQI
				packet_count++;
				if(packet_count>=100)
				{
					packet_count=0;
					TX_LQI=telemetry_counter;
					if(telemetry_counter==0)
						telemetry_lost = 1;
					telemetry_counter = 0;
				}
			#endif
			
			// Send data packet
			LOLI_send_packet();
			
	#ifdef LOLI_HUB_TELEMETRY
			phase ++;
			return 2000;
		case LOLI_DATA2:
			// Switch to RX mode
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_FlushRx();
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x3b);  // 8bit CRC, RX
			phase = LOLI_DATA1;
			return 18000;
	#else
			break;
	#endif
	}
	return 20000;
}

uint16_t initLOLI()
{
	rx_tx_addr[1] %= 0x30;
	calc_fh_channels(LOLI_NUM_CHANNELS);
	for (uint8_t i=0; i < LOLI_NUM_CHANNELS; i++)
		if (hopping_frequency[i] == LOLI_BIND_CHANNEL)
			hopping_frequency[i]++;

	if (IS_BIND_IN_PROGRESS)
	{
		bind_counter=250;
		phase = LOLI_BIND1;
	}
	else
		phase = LOLI_PREP_DATA;

	LOLI_init();

	return 500;
}

#endif
