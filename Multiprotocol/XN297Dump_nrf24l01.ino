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

// sub_protocol: 0=250Kbps, 1=1Mbps, 2=2Mbps. Other values default to 1Mbps.
// RX_num = address length 3 or 4 or 5. Other values default to 5.
// option = RF channel number 0..84 and -1 = scan all channels. Other values default to RF channel 0.

#ifdef XN297DUMP_NRF24L01_INO

#include "iface_nrf24l01.h"

// Parameters which can be modified
#define XN297DUMP_PERIOD_DUMP		25000
#define XN297DUMP_MAX_RF_CHANNEL	84		// Default 84

// Do not touch from there
#define XN297DUMP_INITIAL_WAIT		500
#define XN297DUMP_MAX_PACKET_LEN	32
#define XN297DUMP_CRC_LENGTH		2

uint8_t address_length;
uint16_t over;
boolean scramble;

static void __attribute__((unused)) XN297Dump_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(RX_EN);

	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);				// No Auto Acknowledgment on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);			// Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);			// 3 bytes RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t*)"\x55\x0F\x71", 3);	// set up RX address to xn297 preamble
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, XN297DUMP_MAX_PACKET_LEN);	// Enable rx pipe 0

	debug("XN297 dump, address length=%d, speed=",address_length);
	switch(sub_protocol)
	{
		case 0:
			NRF24L01_SetBitrate(NRF24L01_BR_250K);
			debugln("250K");
			break;
		case 2:
			NRF24L01_SetBitrate(NRF24L01_BR_2M);
			debugln("2M");
			break;
		default:
			NRF24L01_SetBitrate(NRF24L01_BR_1M);
			debugln("1M");
			break;

	}
    NRF24L01_Activate(0x73);								// Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);				// Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
	NRF24L01_SetPower();
}

static boolean __attribute__((unused)) XN297Dump_process_packet(void)
{
	uint16_t crcxored;
	uint8_t packet_sc[XN297DUMP_MAX_PACKET_LEN], packet_un[XN297DUMP_MAX_PACKET_LEN];

	// init crc
	crc = 0xb5d2;
	
	// address
	for (uint8_t i = 0; i < address_length; i++)
	{
		crc = crc16_update(crc, packet[i], 8);
		packet_un[address_length-1-i]=packet[i];
		packet_sc[address_length-1-i]=packet[i] ^ xn297_scramble[i];
	}
	
	// payload
	for (uint8_t i = address_length; i < XN297DUMP_MAX_PACKET_LEN-XN297DUMP_CRC_LENGTH; i++)
	{
		crc = crc16_update(crc, packet[i], 8);
		packet_sc[i] = bit_reverse(packet[i]^xn297_scramble[i]);
		packet_un[i] = bit_reverse(packet[i]);
		// check crc
		crcxored = crc ^ pgm_read_word(&xn297_crc_xorout[i+1 - 3]);
		if( (crcxored >> 8) == packet[i + 1] && (crcxored & 0xff) == packet[i + 2])
		{
			packet_length=i+1;
			memcpy(packet,packet_un,packet_length);
			scramble=false;
			return true;
		}
		crcxored = crc ^ pgm_read_word(&xn297_crc_xorout_scrambled[i+1 - 3]);
		if( (crcxored >> 8) == packet[i + 1] && (crcxored & 0xff) == packet[i + 2])
		{
			packet_length=i+1;
			memcpy(packet,packet_sc,packet_length);
			scramble=true;
			return true;
		}
	}
	return false;
}

static void __attribute__((unused)) XN297Dump_overflow()
{
	if(TIMER2_BASE->SR & TIMER_SR_UIF)
	{ // timer overflow
		if(over!=0xFFFF)
			over++;
		TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_UIF;	// Clear Timer2 overflow flag
	}
}
static uint16_t XN297Dump_callback()
{
	static uint16_t time=0;
	while(1)
	{
		if(option==0xFF && bind_counter>XN297DUMP_PERIOD_DUMP)
		{	// Scan frequencies
			hopping_frequency_no++;
			bind_counter=0;
		}
		if(hopping_frequency_no!=rf_ch_num)
		{	// Channel has changed
			if(hopping_frequency_no>XN297DUMP_MAX_RF_CHANNEL)
				hopping_frequency_no=0;	// Invalid channel 0 by default
			rf_ch_num=hopping_frequency_no;
			debugln("Channel=%d,0x%02X",hopping_frequency_no,hopping_frequency_no)
			NRF24L01_WriteReg(NRF24L01_05_RF_CH,hopping_frequency_no);
			// switch to RX mode
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to RX mode and disable CRC
											| (1 << NRF24L01_00_CRCO)
											| (1 << NRF24L01_00_PWR_UP)
											| (1 << NRF24L01_00_PRIM_RX));
			over=0xFFFF;
		}
		XN297Dump_overflow();
		
		if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
		{ // RX fifo data ready
			if(NRF24L01_ReadReg(NRF24L01_09_CD) || option != 0xFF)
			{
				NRF24L01_ReadPayload(packet,XN297DUMP_MAX_PACKET_LEN);
				uint16_t time_TCNT1=TCNT1;
				uint32_t time32=0;
				time=time_TCNT1-time;
				if(over!=0xFFFF)
					time32=(over<<16)+time;
				debug("RX: %5luus C=%d ", time32>>1 , hopping_frequency_no);
				time=time_TCNT1;
				over=0;
				if(XN297Dump_process_packet())
				{ // valid crc found
					debug("S=%c A=",scramble?'Y':'N');
					for(uint8_t i=0; i<address_length; i++)
					{
						debug(" %02X",packet[i]);
					}
					debug(" P(%d)=",packet_length-address_length);
					for(uint8_t i=address_length; i<packet_length; i++)
					{
						debug(" %02X",packet[i]);
					}
					debugln("");
				}
				else
				{
					debugln("Bad CRC");
				}
			}
			
			XN297Dump_overflow();
			// restart RX mode
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to RX mode and disable CRC
											| (1 << NRF24L01_00_CRCO)
											| (1 << NRF24L01_00_PWR_UP)
											| (1 << NRF24L01_00_PRIM_RX));
			XN297Dump_overflow();
		}
		bind_counter++;
		if(IS_RX_FLAG_on)					// Let the radio update the protocol
		{
			if(Update_All()) return 10000;	// New protocol selected
			if(prev_option!=option)
			{	// option has changed
				hopping_frequency_no=option;
				prev_option=option;
			}
		}
		XN297Dump_overflow();
	}
	return 100;
}

uint16_t initXN297Dump(void)
{
	BIND_DONE;
	over=0xFFFF;
	address_length=RX_num;
	if(address_length<3||address_length>5)
		address_length=5;	//default
	XN297Dump_init();
	bind_counter=0;
	rf_ch_num=0xFF;
	prev_option=option^0x55;
	return XN297DUMP_INITIAL_WAIT;
}

#endif
