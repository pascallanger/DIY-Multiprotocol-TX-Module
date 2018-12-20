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

#if defined(NCC1701_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define NCC_WRITE_WAIT      2000
#define NCC_PACKET_INTERVAL 10333
#define NCC_TX_PACKET_LEN	16
#define NCC_RX_PACKET_LEN	13

enum {
	NCC_BIND_TX1=0,
	NCC_BIND_RX1,
	NCC_BIND_TX2,
	NCC_BIND_RX2,
	NCC_TX3,
	NCC_RX3,
};

static void __attribute__((unused)) NCC_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);

	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);     // 5-byte RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t*)"\xE7\xE7\xC7\xD7\x67",5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (uint8_t*)"\xE7\xE7\xC7\xD7\x67",5);
	
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, NCC_RX_PACKET_LEN);	// Enable rx pipe 0
	NRF24L01_SetBitrate(NRF24L01_BR_250K);					// NRF24L01_BR_1M, NRF24L01_BR_2M, NRF24L01_BR_250K
	NRF24L01_SetPower();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to TX mode and disable CRC
										| (1 << NRF24L01_00_CRCO)
										| (1 << NRF24L01_00_PWR_UP)
										| (0 << NRF24L01_00_PRIM_RX));
}

const uint8_t NCC_xor[]={0x80, 0x44, 0x64, 0x75, 0x6C, 0x71, 0x2A, 0x36, 0x7C, 0xF1, 0x6E, 0x52, 0x09, 0x9D};
static void __attribute__((unused)) NCC_Crypt_Packet()
{
	uint16_t crc=0;
	for(uint8_t i=0; i< NCC_TX_PACKET_LEN-2; i++)
	{
		packet[i]^=NCC_xor[i];
		crc=crc16_update(crc, packet[i], 8);
	}
	crc^=0x60DE;
	packet[NCC_TX_PACKET_LEN-2]=crc>>8;
	packet[NCC_TX_PACKET_LEN-1]=crc;
}
static boolean __attribute__((unused)) NCC_Decrypt_Packet()
{
	uint16_t crc=0;
	debug("RX: ");
	for(uint8_t i=0; i< NCC_RX_PACKET_LEN-2; i++)
	{
		crc=crc16_update(crc, packet[i], 8);
		packet[i]^=NCC_xor[i];
		debug("%02X ",packet[i]);
	}
	crc^=0xA950;
	if( (crc>>8)==packet[NCC_RX_PACKET_LEN-2] && (crc&0xFF)==packet[NCC_RX_PACKET_LEN-1] )
	{// CRC match
		debugln("OK");
		return true;
	}
	debugln("NOK");
	return false;
}

static void __attribute__((unused)) NCC_Write_Packet()
{
	packet[0]=0xAA;
	packet[1]=rx_tx_addr[0];
	packet[2]=rx_tx_addr[1];
	packet[3]=rx_id[0];
	packet[4]=rx_id[1];
	packet[5]=convert_channel_8b(THROTTLE)>>2;	// 00-3D
	packet[6]=convert_channel_8b(ELEVATOR);		// original: 61-80-9F but works with 00-80-FF
	packet[7]=convert_channel_8b(AILERON );		// original: 61-80-9F but works with 00-80-FF
	packet[8]=convert_channel_8b(RUDDER  );		// original: 61-80-9F but works with 00-80-FF
	packet[9]=rx_id[2];
	packet[10]=rx_id[3];
	packet[11]=rx_id[4];
	packet[12]=GET_FLAG(CH5_SW, 0x02);			// Warp:0x00 -> 0x02
	packet[13]=packet[5]+packet[6]+packet[7]+packet[8]+packet[12];
	if(phase==NCC_BIND_TX1)
	{
		packet[0]=0xBB;
		packet[5]=0x01;
		packet[6]=rx_tx_addr[2];
		memset((void *)(packet+7),0x55,7);
		hopping_frequency_no^=1;
	}
	else
	{
		hopping_frequency_no++;
		if(hopping_frequency_no>2) hopping_frequency_no=0;
	}
	// change frequency
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
	// switch to TX mode and disable CRC
	NRF24L01_SetTxRxMode(TXRX_OFF);
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
										| (1 << NRF24L01_00_CRCO)
										| (1 << NRF24L01_00_PWR_UP)
										| (0 << NRF24L01_00_PRIM_RX));
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	// send packet
	NCC_Crypt_Packet();
	NRF24L01_WritePayload(packet,NCC_TX_PACKET_LEN);
	NRF24L01_SetPower();
}

uint16_t NCC_callback()
{
	switch(phase)
	{
		case NCC_BIND_TX1:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
				if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1])

				{
					rx_id[0]=packet[3];
					rx_id[1]=packet[4];
					NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
					phase=NCC_BIND_TX2;
					return NCC_PACKET_INTERVAL;
				}
			}
			NCC_Write_Packet();
			phase = NCC_BIND_RX1;
			return NCC_WRITE_WAIT;
		case NCC_BIND_RX1:
			// switch to RX mode and disable CRC
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
			NRF24L01_FlushRx();
			phase = NCC_BIND_TX1;
			return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
		case NCC_BIND_TX2:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
				if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1] && packet[3]==rx_id[0] && packet[4]==rx_id[1])
				{
					rx_id[2]=packet[8];
					rx_id[3]=packet[9];
					rx_id[4]=packet[10];
					NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
					BIND_DONE;
					phase=NCC_TX3;
					return NCC_PACKET_INTERVAL;
				}
			}
			NCC_Write_Packet();
			phase = NCC_BIND_RX2;
			return NCC_WRITE_WAIT;
		case NCC_BIND_RX2:
			// switch to RX mode and disable CRC
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
			NRF24L01_FlushRx();
			phase = NCC_BIND_TX2;
			return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
		case NCC_TX3:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
				if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1] && packet[3]==rx_id[0] && packet[4]==rx_id[1])
				{
					//Telemetry
					//packet[5] and packet[7] roll angle
					//packet[6] crash detect: 0x00 no crash, 0x02 crash
					#ifdef NCC1701_HUB_TELEMETRY
						v_lipo1 = packet[6]?0xFF:0x00;	// Crash indication
						v_lipo2 = 0x00;
						RX_RSSI = 0x7F;					// Dummy RSSI
						TX_RSSI = 0x7F;					// Dummy RSSI
						telemetry_link=1;
					#endif
				}
			}
			NCC_Write_Packet();
			phase = NCC_RX3;
			return NCC_WRITE_WAIT;
		case NCC_RX3:
			// switch to RX mode and disable CRC
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
			NRF24L01_FlushRx();
			phase = NCC_TX3;
			return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
	}
	return 0;
}

const uint8_t PROGMEM NCC_TX_DATA[][6]= {
	{ 0x6D, 0x97, 0x04, 0x48, 0x43, 0x26 }, 
	{ 0x35, 0x4B, 0x80, 0x44, 0x4C, 0x0B },
	{ 0x50, 0xE2, 0x32, 0x2D, 0x4B, 0x0A },
	{ 0xBF, 0x34, 0xF3, 0x45, 0x4D, 0x0D },
	{ 0xDD, 0x7D, 0x5A, 0x46, 0x28, 0x23 },
	{ 0xED, 0x19, 0x06, 0x2C, 0x4A, 0x09 },
	{ 0xE9, 0xA8, 0x91, 0x2B, 0x49, 0x07 },
	{ 0x66, 0x17, 0x7D, 0x48, 0x43, 0x26 },
	{ 0xC2, 0x93, 0x55, 0x44, 0x4C, 0x0B },
};

uint16_t initNCC(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	
	// Load TX data
	uint8_t rand=rx_tx_addr[3]%9;
	for(uint8_t i=0; i<3; i++)
	{
		rx_tx_addr[i]=pgm_read_byte_near(&NCC_TX_DATA[rand][i]);
		hopping_frequency[i]=pgm_read_byte_near(&NCC_TX_DATA[rand][i+3]);
	}

	// RX data is acquired during bind
	rx_id[0]=0x00;
	rx_id[1]=0x00;
	rx_id[2]=0x20;
	rx_id[3]=0x20;
	rx_id[4]=0x20;

	hopping_frequency[4]=0x08;	// bind channel 1
	hopping_frequency[5]=0x2A;	// bind channel 2
	hopping_frequency_no=4;		// start with bind
	NCC_init();
	phase=NCC_BIND_TX1;
	#ifdef NCC1701_HUB_TELEMETRY
		init_frskyd_link_telemetry();
	#endif
	return 10000;
}

#endif
