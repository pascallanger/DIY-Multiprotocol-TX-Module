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

#if defined(FAKE_NRF24L01_INO)

#include "iface_nrf250k.h"

static void __attribute__((unused)) FAKE_send_packet()
{
	for(uint8_t i=0;i<5;i++)
		packet[i]=i;
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_RX_DR) | _BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, 5);
}

static void __attribute__((unused)) FAKE_init()
{
//	BIND_IN_PROGRESS;

	//CC2500
	option=1;
	XN297L_Init();
	CC2500_WriteReg(CC2500_07_PKTCTRL1,	0x01);   // Packet Automation Control
	CC2500_WriteReg(CC2500_08_PKTCTRL0,	0x00);   // Packet Automation Control
	CC2500_WriteReg(CC2500_12_MDMCFG2,	0x12);   // Modem Configuration
	CC2500_WriteReg(CC2500_13_MDMCFG1,	0x13);   // Modem Configuration
	CC2500_WriteReg(CC2500_04_SYNC1,	0x11);
	CC2500_WriteReg(CC2500_05_SYNC0,	0x33);
	CC2500_WriteReg(CC2500_09_ADDR,     0x99);
	CC2500_WriteReg(CC2500_06_PKTLEN,	10);

	CC2500_SetTxRxMode(RX_EN);
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_Strobe(CC2500_SRX);
	//CC2500_SetTxRxMode(TX_EN);
	XN297L_RFChannel(0);

	//NRF
/*	option=0;
	PE1_on;							//NRF24L01 antenna RF3 by default
	PE2_off;						//NRF24L01 antenna RF3 by default
	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x7f);
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);//0x3f);       // AA on all pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3f);   // Enable all pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);    // 5-byte address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x36);  // retransmit 1ms, 6 times
	NRF24L01_SetBitrate(NRF24L01_BR_250K);              // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *)"\x99\x33\x11\xAA\xAA", 5);	//Bind address
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (uint8_t *)"\x99\x33\x11\xAA\xAA", 5);	//Bind address
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0);
	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);       // Enable dynamic payload length
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Enable all features
*/	
	/*NRF24L01_FlushTx();
    NRF24L01_SetTxRxMode(TX_EN);*/
}

uint16_t FAKE_callback()
{
	len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
	if(len) debug("L=%d, ",len);
	if(len && len < sizeof(packet_in))
	{
		CC2500_ReadData(packet_in, len);
		debug("P:");
		for(uint8_t i=0;i<len;i++)
			debug(" %02X", packet_in[i]);
	}
	if(len) debugln("");
	CC2500_Strobe(CC2500_SFRX);
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_Strobe(CC2500_SRX);

	option=0;
	//FAKE_send_packet();
	
	PE1_off;	//antenna RF2
	PE2_on;
	/*packet[0]=0x99;
	for(uint8_t i=1;i<5;i++)
		packet[i]=i;
	CC2500_WriteData(packet, 5);*/
	return 10000;
}

uint16_t initFAKE()
{
	FAKE_init();
	return 5000;
}

#endif
