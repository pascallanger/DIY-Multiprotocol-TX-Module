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
#if defined(CC2500_INSTALLED) || defined(NRF24L01_INSTALLED)

#include "iface_nrf250k.h"

#if defined(CC2500_INSTALLED) && defined(NRF24L01_INSTALLED)
	extern bool xn297_rf;
#endif

uint8_t cc2500_nrf_tx_addr[5], cc2500_nrf_addr_len;

static void __attribute__((unused)) NRF250K_SetTXAddr(uint8_t* addr, uint8_t len)
{
	if (len > 5) len = 5;
	if (len < 3) len = 3;
	#if defined(CC2500_INSTALLED) && defined(NRF24L01_INSTALLED)
		cc2500_nrf_addr_len = len;
		memcpy(cc2500_nrf_tx_addr, addr, len);
		if(xn297_rf == XN297_NRF)
		{
			NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, len);
		}
	#elif defined(CC2500_INSTALLED)
		cc2500_nrf_addr_len = len;
		memcpy(cc2500_nrf_tx_addr, addr, len);
	#elif defined(NRF24L01_INSTALLED)
		NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, addr, len);
	#endif
}

static void __attribute__((unused)) NRF250K_WritePayload(uint8_t* msg, uint8_t len)
{
	#if defined(NRF24L01_INSTALLED)
		#if defined(CC2500_INSTALLED)
			if(xn297_rf == XN297_NRF)
		#endif
		{
			if(len<=32)
			{
				NRF24L01_FlushTx();
				NRF24L01_WriteReg(NRF24L01_07_STATUS, _BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_RX_DR) | _BV(NRF24L01_07_MAX_RT));
				NRF24L01_WritePayload(msg, len);
			}
			#if defined(CC2500_INSTALLED)
				return;
			#endif
		}
	#endif
	#if defined(CC2500_INSTALLED)
		#if defined(ESKY150V2_CC2500_INO)
			uint8_t buf[158];
		#else
			uint8_t buf[35];
		#endif
		uint8_t last = 0;
		uint8_t i;

		//nrf preamble
		if(cc2500_nrf_tx_addr[cc2500_nrf_addr_len - 1] & 0x80)
			buf[0]=0xAA;
		else
			buf[0]=0x55;
		last++;
		// address
		for (i = 0; i < cc2500_nrf_addr_len; ++i)
			buf[last++] = cc2500_nrf_tx_addr[cc2500_nrf_addr_len - i - 1];
		// payload
		for (i = 0; i < len; ++i)
			buf[last++] = msg[i];

		// crc
		crc = 0xffff;
		for (uint8_t i = 1; i < last; ++i)
			crc16_update( buf[i], 8);
		buf[last++] = crc >> 8;
		buf[last++] = crc & 0xff;
		buf[last++] = 0;

		//for(uint8_t i=0;i<last;i++)
		//	debug("%02X ",buf[i]);
		//debugln("");
		// stop TX/RX
		CC2500_Strobe(CC2500_SIDLE);
		// flush tx FIFO
		CC2500_Strobe(CC2500_SFTX);
		// packet length
		CC2500_WriteReg(CC2500_3F_TXFIFO, last);
		// transmit nrf packet
		uint8_t *buff=buf;
		uint8_t status;
		if(last>63)
		{
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 63);
			CC2500_Strobe(CC2500_STX);
			last-=63;
			buff+=63;
			while(last)
			{//Loop until all the data is sent
				do
				{// Wait for the FIFO to become available
					status=CC2500_ReadReg(CC2500_3A_TXBYTES | CC2500_READ_BURST);
				}
				while((status&0x7F)>31 && (status&0x80)==0);
				if(last>31)
				{//Send 31 bytes
					CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, 31);
					last-=31;
					buff+=31;
				}
				else
				{//Send last bytes
					CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
					last=0;
				}
			}
		}
		else
		{//Send packet
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buff, last);
			CC2500_Strobe(CC2500_STX);
		}
	#endif
}

#endif
