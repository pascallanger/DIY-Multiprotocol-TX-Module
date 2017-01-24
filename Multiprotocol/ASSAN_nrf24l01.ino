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

#if defined(ASSAN_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define ASSAN_PACKET_SIZE		20
#define ASSAN_RF_BIND_CHANNEL	0x03
#define ASSAN_ADDRESS_LENGTH	4

enum {
    ASSAN_BIND0=0,
    ASSAN_BIND1,
    ASSAN_BIND2,
    ASSAN_DATA0,
    ASSAN_DATA1,
    ASSAN_DATA2,
    ASSAN_DATA3,
    ASSAN_DATA4,
    ASSAN_DATA5
};

void ASSAN_init()
{
    NRF24L01_Initialize();
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);			// 4 bytes rx/tx address
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *)"\x80\x80\x80\xB8", ASSAN_ADDRESS_LENGTH);		// Bind address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *)"\x80\x80\x80\xB8", ASSAN_ADDRESS_LENGTH);	// Bind address
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, ASSAN_PACKET_SIZE);
    NRF24L01_SetPower();
}

void ASSAN_send_packet()
{
	uint16_t temp;
	for(uint8_t i=0;i<8;i++)
	{
		temp=Servo_data[i]<<3;
		packet[2*i]=temp>>8;
		packet[2*i+1]=temp;
	}
	for(uint8_t i=0;i<ASSAN_ADDRESS_LENGTH;i++)
		packet[16+i]=packet[23-i];
 	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);		// Clear data ready, data sent, and retransmit
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, ASSAN_PACKET_SIZE);
}

uint16_t ASSAN_callback()
{
	switch (phase)
	{
	// Bind
		case ASSAN_BIND0:
			//Config RX @1M
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, ASSAN_RF_BIND_CHANNEL);
			NRF24L01_SetBitrate(NRF24L01_BR_1M);					// 1Mbps
			NRF24L01_SetTxRxMode(RX_EN);
			phase++;
		case ASSAN_BIND1:
			//Wait for receiver to send the frames
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ //Something has been received
				NRF24L01_ReadPayload(packet, ASSAN_PACKET_SIZE);
				if(packet[19]==0x13)
				{ //Last frame received
					phase++;
					//Switch to TX
					NRF24L01_SetTxRxMode(TXRX_OFF);
					NRF24L01_SetTxRxMode(TX_EN);
					//Prepare bind packet
					memset(packet,0x05,ASSAN_PACKET_SIZE-5);
					packet[15]=0x99;
					for(uint8_t i=0;i<ASSAN_ADDRESS_LENGTH;i++)
						packet[16+i]=packet[23-i];
					packet_count=0;
					delayMilliseconds(260);
					return 10000;	// Wait 270ms in total...
				}
			}
			return 1000;
		case ASSAN_BIND2:
			// Send 20 packets
			packet_count++;
			if(packet_count==20)
				packet[15]=0x13;	// different value for last packet
			NRF24L01_WritePayload(packet, ASSAN_PACKET_SIZE);
			if(packet_count==20)
			{
				phase++;
				delayMilliseconds(2165);
			}
			return 22520;
	// Normal operation
		case ASSAN_DATA0:
			// Bind Done
			BIND_DONE;
			NRF24L01_SetBitrate(NRF24L01_BR_250K);					// 250Kbps
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(TX_EN);
		case ASSAN_DATA1:
		case ASSAN_DATA4:
			// Change ID and RF channel
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,packet+20+4*hopping_frequency_no, ASSAN_ADDRESS_LENGTH);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			hopping_frequency_no^=0x01;
			NRF24L01_SetPower();
			phase=ASSAN_DATA2;
			return 2000;
		case ASSAN_DATA2:
		case ASSAN_DATA3:
			ASSAN_send_packet();
			phase++;	// DATA 3 or 4
			return 5000;
	}
	return 0;
}

static void __attribute__((unused)) ASSAN_initialize_txid()
{
/*	//Renaud TXID with Freq=36 and alternate Freq 67 or 68 or 69 or 70 or 71 or 73 or 74 or 75 or 78 and may be more...
	packet[23]=0x22;
	packet[22]=0x37;
	packet[21]=0xFA;
	packet[20]=0x53; */
	// Using packet[20..23] to store the ID1 and packet[24..27] to store the ID2
	uint8_t freq=0,freq2;
	for(uint8_t i=0;i<ASSAN_ADDRESS_LENGTH;i++)
	{
		uint8_t temp=rx_tx_addr[i];
		packet[i+20]=temp;
		packet[i+24]=temp+1;
		freq+=temp;
	}	

	// Main frequency
	freq=((freq%25)+2)<<1;
	if(freq&0x02)	freq|=0x01;
	hopping_frequency[0]=freq;
	// Alternate frequency has some random
	do
	{
		freq2=random(0xfefefefe)%9;
		freq2+=freq*2-5;
	}
	while( (freq2>118) || (freq2<freq+1) || (freq2==2*freq) );
	hopping_frequency[1]=freq2;
}

uint16_t initASSAN()
{
	ASSAN_initialize_txid();
	ASSAN_init();
	hopping_frequency_no = 0;

	if(IS_AUTOBIND_FLAG_on)
		phase=ASSAN_BIND0;
	else 
		phase=ASSAN_DATA0;
	return 1000;
}

#endif