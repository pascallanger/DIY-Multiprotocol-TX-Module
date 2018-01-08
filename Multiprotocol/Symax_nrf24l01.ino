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
// compatible with Syma X5C-1, X11, X11C, X12 and for sub protocol X5C Syma X5C (original), X2

#if defined(SYMAX_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define SYMAX_BIND_COUNT			345		// 1.5 seconds
#define SYMAX_FIRST_PACKET_DELAY	12000
#define SYMAX_PACKET_PERIOD			4000	// Timeout for callback in uSec
#define SYMAX_INITIAL_WAIT			500

#define SYMAX_MAX_RF_CHANNELS    	17

#define SYMAX_FLAG_FLIP				0x01
#define SYMAX_FLAG_VIDEO			0x02
#define SYMAX_FLAG_PICTURE			0x04
#define SYMAX_FLAG_HEADLESS			0x08
#define SYMAX_XTRM_RATES			0x10

#define SYMAX_PAYLOADSIZE			10		// receive data pipes set to this size, but unused
#define SYMAX_MAX_PACKET_LENGTH		16		// X11,X12,X5C-1 10-byte, X5C 16-byte

enum {
	SYMAX_INIT1 = 0,
	SYMAX_BIND2,
	SYMAX_BIND3,
	SYMAX_DATA
};

static uint8_t __attribute__((unused)) SYMAX_checksum(uint8_t *data)
{
	uint8_t sum = data[0];

	for (uint8_t i=1; i < packet_length-1; i++)
	if ( sub_protocol==SYMAX5C )
		sum += data[i];
	else
		sum ^= data[i];

	return sum + ( sub_protocol==SYMAX5C ? 0 : 0x55 );
}

static void __attribute__((unused)) SYMAX_read_controls()
{
	// Protocol is registered AETRF, that is
	// Aileron is channel 1, Elevator - 2, Throttle - 3, Rudder - 4, Flip control - 5
	// Extended (trim-added) Rates - 6, Photo - 7, Video - 8, Headless - 9
	aileron  = convert_channel_s8b(AILERON);
	elevator = convert_channel_s8b(ELEVATOR);
	throttle = convert_channel_8b(THROTTLE);
	rudder   = convert_channel_s8b(RUDDER);

	flags=0;
	// Channel 5
	if (CH5_SW)
		flags = SYMAX_FLAG_FLIP;
	// Channel 6
	if (CH6_SW)
		flags |= SYMAX_XTRM_RATES;
	// Channel 7
	if (CH7_SW)
		flags |= SYMAX_FLAG_PICTURE;
	// Channel 8
	if (CH8_SW)
		flags |= SYMAX_FLAG_VIDEO;
	// Channel 9
	if (CH9_SW)
	{
		flags |= SYMAX_FLAG_HEADLESS;
		flags &= ~SYMAX_XTRM_RATES;	// Extended rates & headless incompatible
	}
}

#define X5C_CHAN2TRIM(X) ((((X) & 0x80 ? 0xff - (X) : 0x80 + (X)) >> 2) + 0x20)

static void __attribute__((unused)) SYMAX_build_packet_x5c(uint8_t bind)
{
	if (bind)
	{
		memset(packet, 0, packet_length);
		packet[7] = 0xae;
		packet[8] = 0xa9;
		packet[14] = 0xc0;
		packet[15] = 0x17;
	}
	else
	{
		SYMAX_read_controls();

		packet[0] = throttle;
		packet[1] = rudder;
		packet[2] = elevator ^ 0x80;  // reversed from default
		packet[3] = aileron;
		if (flags & SYMAX_XTRM_RATES)
		{	// drive trims for extra control range
			packet[4] = X5C_CHAN2TRIM(rudder ^ 0x80);
			packet[5] = X5C_CHAN2TRIM(elevator);
			packet[6] = X5C_CHAN2TRIM(aileron ^ 0x80);
		}
		else
		{
			packet[4] = 0x00;
			packet[5] = 0x00;
			packet[6] = 0x00;
		}
		packet[7] = 0xae;
		packet[8] = 0xa9;
		packet[9] = 0x00;
		packet[10] = 0x00;
		packet[11] = 0x00;
		packet[12] = 0x00;
		packet[13] = 0x00;
		packet[14] =  (flags & SYMAX_FLAG_VIDEO   ? 0x10 : 0x00) 
					| (flags & SYMAX_FLAG_PICTURE ? 0x08 : 0x00)
					| (flags & SYMAX_FLAG_FLIP    ? 0x01 : 0x00)
					| 0x04;// always high rates (bit 3 is rate control)
		packet[15] = SYMAX_checksum(packet);
	}
}

static void __attribute__((unused)) SYMAX_build_packet(uint8_t bind)
{
	if (bind)
	{
		packet[0] = rx_tx_addr[4];
		packet[1] = rx_tx_addr[3];
		packet[2] = rx_tx_addr[2];
		packet[3] = rx_tx_addr[1];
		packet[4] = rx_tx_addr[0];
		packet[5] = 0xaa;
		packet[6] = 0xaa;
		packet[7] = 0xaa;
		packet[8] = 0x00;
	}
	else
	{
		SYMAX_read_controls();
		packet[0] = throttle;
		packet[1] = elevator;
		packet[2] = rudder;
		packet[3] = aileron;
		packet[4] = (flags & SYMAX_FLAG_VIDEO   ? 0x80 : 0x00) | (flags & SYMAX_FLAG_PICTURE ? 0x40 : 0x00);
		packet[5] = 0xc0;	//always high rates (bit 7 is rate control)
		packet[6] = flags & SYMAX_FLAG_FLIP ? 0x40 : 0x00;
		packet[7] = flags & SYMAX_FLAG_HEADLESS ? 0x80 : 0x00;
		if (flags & SYMAX_XTRM_RATES)
		{	// use trims to extend controls
			packet[5] |= elevator >> 2;
			packet[6] |= rudder >> 2;
			packet[7] |= aileron >> 2;
		}
		packet[8] = 0x00;
	}
	packet[9] = SYMAX_checksum(packet);
}

static void __attribute__((unused)) SYMAX_send_packet(uint8_t bind)
{
	if (sub_protocol==SYMAX5C)
		SYMAX_build_packet_x5c(bind);
	else
		SYMAX_build_packet(bind);
		
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x2e);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
	NRF24L01_FlushTx();

	NRF24L01_WritePayload(packet, packet_length);

	if (packet_count++ % 2)	// use each channel twice
		hopping_frequency_no = (hopping_frequency_no + 1) % rf_ch_num;

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) symax_init()
{
	NRF24L01_Initialize();
	//
	NRF24L01_SetTxRxMode(TX_EN);
	//	
	NRF24L01_ReadReg(NRF24L01_07_STATUS);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes (even though not used?)
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xff); // 4mS retransmit t/o, 15 tries (retries w/o AA?)
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);

	if (sub_protocol==SYMAX5C)
	{
		NRF24L01_SetBitrate(NRF24L01_BR_1M);
		packet_length = 16;
	}
	else
	{
		NRF24L01_SetBitrate(NRF24L01_BR_250K);
		packet_length = 10;
	}
	//
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_08_OBSERVE_TX, 0x00);
	NRF24L01_WriteReg(NRF24L01_09_CD, 0x00);
	NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
	NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
	NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
	NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, SYMAX_PAYLOADSIZE);   // bytes of data payload for pipe 1
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, SYMAX_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, SYMAX_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, SYMAX_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, SYMAX_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, SYMAX_PAYLOADSIZE);
	NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here

	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR , sub_protocol==SYMAX5C ? (uint8_t *)"\x6D\x6A\x73\x73\x73" : (uint8_t *)"\xAB\xAC\xAD\xAE\xAF" ,5);

	NRF24L01_ReadReg(NRF24L01_07_STATUS);
	NRF24L01_FlushTx();
	NRF24L01_ReadReg(NRF24L01_07_STATUS);
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x0e);
	NRF24L01_ReadReg(NRF24L01_00_CONFIG); 
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0c); 
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0e);  // power on
}

static void __attribute__((unused)) symax_init1()
{
	// duplicate stock tx sending strange packet (effect unknown)
	uint8_t first_packet[] = {0xf9, 0x96, 0x82, 0x1b, 0x20, 0x08, 0x08, 0xf2, 0x7d, 0xef, 0xff, 0x00, 0x00, 0x00, 0x00};
	uint8_t chans_bind[] = {0x4b, 0x30, 0x40, 0x20};
	uint8_t chans_bind_x5c[] = {0x27, 0x1b, 0x39, 0x28, 0x24, 0x22, 0x2e, 0x36,
								0x19, 0x21, 0x29, 0x14, 0x1e, 0x12, 0x2d, 0x18};

	NRF24L01_FlushTx();
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);
	NRF24L01_WritePayload(first_packet, 15);

	if (sub_protocol==SYMAX5C)
	{
		rf_ch_num = sizeof(chans_bind_x5c);
		memcpy(hopping_frequency, chans_bind_x5c, rf_ch_num);
	}
	else
	{
		rx_tx_addr[4] = 0xa2; // this is constant in ID
		rf_ch_num = sizeof(chans_bind);
		memcpy(hopping_frequency, chans_bind, rf_ch_num);
	}
	hopping_frequency_no = 0;
	packet_count = 0;
}

// channels determined by last byte of tx address
static void __attribute__((unused)) symax_set_channels(uint8_t address)
{
	static const uint8_t start_chans_1[] = {0x0a, 0x1a, 0x2a, 0x3a};
	static const uint8_t start_chans_2[] = {0x2a, 0x0a, 0x42, 0x22};
	static const uint8_t start_chans_3[] = {0x1a, 0x3a, 0x12, 0x32};

	uint8_t laddress = address & 0x1f;
	uint8_t i;
	uint32_t *pchans = (uint32_t *)hopping_frequency;   // avoid compiler warning

	rf_ch_num = 4;

	if (laddress < 0x10)
	{
		if (laddress == 6)
			laddress = 7;
		for(i=0; i < rf_ch_num; i++)
			hopping_frequency[i] = start_chans_1[i] + laddress;
	}
	else
		if (laddress < 0x18)
		{
			for(i=0; i < rf_ch_num; i++)
				hopping_frequency[i] = start_chans_2[i] + (laddress & 0x07);
			if (laddress == 0x16)
			{
				hopping_frequency[0]++;
				hopping_frequency[1]++;
			}
		}
		else
			if (laddress < 0x1e)
			{
				for(i=0; i < rf_ch_num; i++)
					hopping_frequency[i] = start_chans_3[i] + (laddress & 0x07);
			}
			else
				if (laddress == 0x1e)
					*pchans = 0x38184121;
				else
					*pchans = 0x39194121;
}

static void __attribute__((unused)) symax_init2()
{
static	uint8_t chans_data_x5c[] = {0x1d, 0x2f, 0x26, 0x3d, 0x15, 0x2b, 0x25, 0x24,
								0x27, 0x2c, 0x1c, 0x3e, 0x39, 0x2d, 0x22};

	if (sub_protocol==SYMAX5C)
	{
		rf_ch_num = sizeof(chans_data_x5c);
		memcpy(hopping_frequency, chans_data_x5c, rf_ch_num);
	}
	else
	{
		symax_set_channels(rx_tx_addr[0]);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
	}
	hopping_frequency_no = 0;
	packet_count = 0;
}

uint16_t symax_callback()
{
	switch (phase)
	{
		case SYMAX_INIT1:
			symax_init1();
			phase = SYMAX_BIND2;
			return SYMAX_FIRST_PACKET_DELAY;
			break;
		case SYMAX_BIND2:
			bind_counter = SYMAX_BIND_COUNT;
			phase = SYMAX_BIND3;
			SYMAX_send_packet(1);
			break;
		case SYMAX_BIND3:
			if (bind_counter == 0)
			{
				symax_init2();
				phase = SYMAX_DATA;
				BIND_DONE;
			}
			else
			{
				SYMAX_send_packet(1);
				bind_counter--;
			}
			break;
		case SYMAX_DATA:
			SYMAX_send_packet(0);
			break;
	}
	return SYMAX_PACKET_PERIOD;
}

uint16_t initSymax()
{	
	packet_count = 0;
	flags = 0;
	BIND_IN_PROGRESS;	// autobind protocol
	symax_init();
	phase = SYMAX_INIT1;
	return	SYMAX_INITIAL_WAIT;
}

#endif
