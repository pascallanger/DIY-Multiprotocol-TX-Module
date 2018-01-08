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
// compatible with MT99xx, Eachine H7, Yi Zhan i6S and LS114/124
// Last sync with Goebish mt99xx_nrf24l01.c dated 2016-01-29

#if defined(MT99XX_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define MT99XX_BIND_COUNT		928
#define MT99XX_PACKET_PERIOD_FY805 2460
#define MT99XX_PACKET_PERIOD_MT 2625
#define MT99XX_PACKET_PERIOD_YZ 3125
#define MT99XX_INITIAL_WAIT     500
#define MT99XX_PACKET_SIZE		9

#define checksum_offset	rf_ch_num
#define channel_offset	phase

enum{
    // flags going to packet[6] (MT99xx, H7)
    FLAG_MT_RATE1   = 0x01, // (H7 high rate)
    FLAG_MT_RATE2   = 0x02, // (MT9916 only)
    FLAG_MT_VIDEO   = 0x10,
    FLAG_MT_SNAPSHOT= 0x20,
    FLAG_MT_FLIP    = 0x80,
};

enum{
    // flags going to packet[6] (LS)
    FLAG_LS_INVERT  = 0x01,
    FLAG_LS_RATE    = 0x02,
    FLAG_LS_HEADLESS= 0x10,
    FLAG_LS_SNAPSHOT= 0x20,
    FLAG_LS_VIDEO   = 0x40,
    FLAG_LS_FLIP    = 0x80,
};

enum{
    // flags going to packet[7] (FY805)
    FLAG_FY805_HEADLESS= 0x10,
};

enum {
    MT99XX_INIT = 0,
    MT99XX_BIND,
    MT99XX_DATA
};

const uint8_t h7_mys_byte[] = {
	0x01, 0x11, 0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 
	0x05, 0x15, 0x06, 0x16, 0x07, 0x17, 0x00, 0x10
};

static const uint8_t ls_mys_byte[] = {
	0x05, 0x15, 0x25, 0x06, 0x16, 0x26,
	0x07, 0x17, 0x27, 0x00, 0x10, 0x20,
	0x01, 0x11, 0x21, 0x02, 0x12, 0x22,
	0x03, 0x13, 0x23, 0x04, 0x14, 0x24
};

static void __attribute__((unused)) MT99XX_send_packet()
{
	const uint8_t yz_p4_seq[] = {0xa0, 0x20, 0x60};
	static uint8_t yz_seq_num=0;
	static uint8_t ls_counter=0;

	if(sub_protocol != YZ)
	{ // MT99XX & H7 & LS
		packet[0] = convert_channel_16b_limit(THROTTLE,0xE1,0x00); // throttle
		packet[1] = convert_channel_16b_limit(RUDDER  ,0x00,0xE1); // rudder
		packet[2] = convert_channel_16b_limit(AILERON ,0xE1,0x00); // aileron
		packet[3] = convert_channel_16b_limit(ELEVATOR,0x00,0xE1); // elevator
		packet[4] = 0x20; // pitch trim (0x3f-0x20-0x00)
		packet[5] = 0x20; // roll trim (0x00-0x20-0x3f)
		packet[6] = GET_FLAG( CH5_SW, FLAG_MT_FLIP );
		packet[7] = h7_mys_byte[hopping_frequency_no];		// next rf channel index ?

		if(sub_protocol==H7)
			packet[6]|=FLAG_MT_RATE1; // max rate on H7
		else
			if(sub_protocol==MT99)
				packet[6] |= 0x40 | FLAG_MT_RATE2
				  | GET_FLAG( CH7_SW, FLAG_MT_SNAPSHOT )
				  | GET_FLAG( CH8_SW, FLAG_MT_VIDEO );	// max rate on MT99xx
			else
				if(sub_protocol==FY805)
				{
					packet[6]=0x20;
					//Rate 0x01?
					//Flip ?
					packet[7]=0x01
						|GET_FLAG( CH5_SW, FLAG_MT_FLIP )
						|GET_FLAG( CH9_SW, FLAG_FY805_HEADLESS );	//HEADLESS
					checksum_offset=0;
				}
				else //LS
				{
					packet[6] |= FLAG_LS_RATE							// max rate
						| GET_FLAG( CH6_SW, FLAG_LS_INVERT )		//INVERT
						| GET_FLAG( CH7_SW, FLAG_LS_SNAPSHOT )		//SNAPSHOT
						| GET_FLAG( CH8_SW, FLAG_LS_VIDEO )			//VIDEO
						| GET_FLAG( CH9_SW, FLAG_LS_HEADLESS );		//HEADLESS
					packet[7] = ls_mys_byte[ls_counter++];
					if(ls_counter >= sizeof(ls_mys_byte))
						ls_counter=0;
				}

		uint8_t result=checksum_offset;
		for(uint8_t i=0; i<8; i++)
			result += packet[i];
		packet[8] = result;
	}
	else
	{ // YZ
		packet[0] = convert_channel_16b_limit(THROTTLE,0x00,0x64); // throttle
		packet[1] = convert_channel_16b_limit(RUDDER  ,0x64,0x00); // rudder
		packet[2] = convert_channel_16b_limit(ELEVATOR,0x00,0x64); // elevator
		packet[3] = convert_channel_16b_limit(AILERON ,0x64,0x00); // aileron
		if(packet_count++ >= 23)
		{
			yz_seq_num ++;
			if(yz_seq_num > 2)
				yz_seq_num = 0;
			packet_count=0;
		}
		packet[4] = yz_p4_seq[yz_seq_num]; 
		packet[5] = 0x02 // expert ? (0=unarmed, 1=normal)
					| GET_FLAG(CH8_SW, 0x10)		//VIDEO
					| GET_FLAG(CH5_SW, 0x80)		//FLIP
					| GET_FLAG(CH9_SW, 0x04)		//HEADLESS
					| GET_FLAG(CH7_SW, 0x20);		//SNAPSHOT
		packet[6] =   GET_FLAG(CH6_SW, 0x80);		//LED
		packet[7] = packet[0];            
		for(uint8_t idx = 1; idx < MT99XX_PACKET_SIZE-2; idx++)
			packet[7] += packet[idx];
		packet[8] = 0xff;
	}

	if(sub_protocol == LS)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x2D); // LS always transmits on the same channel
	else
		if(sub_protocol==FY805)
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x4B); // FY805 always transmits on the same channel
		else
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no] + channel_offset);
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, MT99XX_PACKET_SIZE);

	hopping_frequency_no++;
	if(sub_protocol == YZ)
		hopping_frequency_no++; // skip every other channel

	if(hopping_frequency_no > 15)
		hopping_frequency_no = 0;

	NRF24L01_SetPower();
}

static void __attribute__((unused)) MT99XX_init()
{
    NRF24L01_Initialize();
    if(sub_protocol == YZ)
		XN297_SetScrambledMode(XN297_UNSCRAMBLED);
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_FlushTx();
    XN297_SetTXAddr((uint8_t *)"\xCC\xCC\xCC\xCC\xCC", 5);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);		// Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);		// Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);		// 5 bytes address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);	// no auto retransmit
    if(sub_protocol == YZ)
        NRF24L01_SetBitrate(NRF24L01_BR_250K);			// 250Kbps (nRF24L01+ only)
    else
        NRF24L01_SetBitrate(NRF24L01_BR_1M);          // 1Mbps
    NRF24L01_SetPower();
	
    XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP) );
	
}

static void __attribute__((unused)) MT99XX_initialize_txid()
{
	rx_tx_addr[3] = 0xCC;
	rx_tx_addr[4] = 0xCC;
    if(sub_protocol == YZ)
	{
		rx_tx_addr[0] = 0x53; // test (SB id)
		rx_tx_addr[1] = 0x00;
		rx_tx_addr[2] = 0x00;
	}
	else
		if(sub_protocol == FY805)
		{
			rx_tx_addr[0] = 0x81; // test (SB id)
			rx_tx_addr[1] = 0x0F;
			rx_tx_addr[2] = 0x00;
		}
		else
			if(sub_protocol == LS)
				rx_tx_addr[0] = 0xCC;
			else //MT99 & H7
				rx_tx_addr[2] = 0x00;
	checksum_offset = rx_tx_addr[0] + rx_tx_addr[1] + rx_tx_addr[2];
	channel_offset = (((checksum_offset & 0xf0)>>4) + (checksum_offset & 0x0f)) % 8;
}

uint16_t MT99XX_callback()
{
	if(IS_BIND_DONE)
		MT99XX_send_packet();
	else
	{
		if (bind_counter == 0)
		{
            // set tx address for data packets
            XN297_SetTXAddr(rx_tx_addr, 5);
			BIND_DONE;
		}
		else
		{
			if(sub_protocol == LS)
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x2D); // LS always transmits on the same channel
			else
				if(sub_protocol==FY805)
					NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x4B); // FY805 always transmits on the same channel
				else
					NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
			NRF24L01_FlushTx();
			XN297_WritePayload(packet, MT99XX_PACKET_SIZE); // bind packet
			hopping_frequency_no++;
			if(sub_protocol == YZ)
				hopping_frequency_no++; // skip every other channel
			if(hopping_frequency_no > 15)
				hopping_frequency_no = 0;
			bind_counter--;
		}
	}

    return packet_period;
}

uint16_t initMT99XX(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = MT99XX_BIND_COUNT;

	memcpy(hopping_frequency,"\x02\x48\x0C\x3e\x16\x34\x20\x2A\x2A\x20\x34\x16\x3e\x0c\x48\x02",16);
	hopping_frequency_no=0;
	
	MT99XX_initialize_txid();
	MT99XX_init();

	packet[0] = 0x20;
	packet_period = MT99XX_PACKET_PERIOD_MT;
	switch(sub_protocol)
	{ // MT99 & H7
		case MT99:
		case H7:
			packet[1] = 0x14;
			packet[2] = 0x03;
			packet[3] = 0x25;
			break;
		case YZ:
			packet_period = MT99XX_PACKET_PERIOD_YZ;
			packet[1] = 0x15;
			packet[2] = 0x05;
			packet[3] = 0x06;
			break;
		case LS:
			packet[1] = 0x14;
			packet[2] = 0x05;
			packet[3] = 0x11;
			break;
		case FY805:
			packet_period = MT99XX_PACKET_PERIOD_FY805;
			packet[1] = 0x15;
			packet[2] = 0x12;
			packet[3] = 0x17;
			break;
	}
	packet[4] = rx_tx_addr[0];
	packet[5] = rx_tx_addr[1];
	packet[6] = rx_tx_addr[2];
    packet[7] = checksum_offset; // checksum offset
    packet[8] = 0xAA;			// fixed
	packet_count=0;
	return	MT99XX_INITIAL_WAIT+MT99XX_PACKET_PERIOD_MT;
}
#endif
