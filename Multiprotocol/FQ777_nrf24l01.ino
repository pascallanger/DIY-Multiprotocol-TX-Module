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
// Last sync with bikemike FQ777-124.ino

#if defined(FQ777_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define FQ777_INITIAL_WAIT		500
#define FQ777_PACKET_PERIOD		2000
#define FQ777_PACKET_SIZE		8
#define FQ777_BIND_COUNT		1000
#define FQ777_NUM_RF_CHANNELS	4

enum {
	FQ777_FLAG_RETURN     = 0x40,  // 0x40 when not off, !0x40 when one key return
	FQ777_FLAG_HEADLESS   = 0x04,
	FQ777_FLAG_EXPERT     = 0x01,
	FQ777_FLAG_FLIP       = 0x80,
};

const uint8_t ssv_xor[] = {0x80,0x44,0x64,0x75,0x6C,0x71,0x2A,0x36,0x7C,0xF1,0x6E,0x52,0x9,0x9D,0x1F,0x78,0x3F,0xE1,0xEE,0x16,0x6D,0xE8,0x73,0x9,0x15,0xD7,0x92,0xE7,0x3,0xBA};
uint8_t FQ777_bind_addr []   = {0xe7,0xe7,0xe7,0xe7,0x67};

static void __attribute__((unused)) ssv_pack_dpl(uint8_t addr[], uint8_t pid, uint8_t* len, uint8_t* payload, uint8_t* packed_payload)
{
	uint8_t i = 0;

	uint16_t pcf = (*len & 0x3f) << 3;
	pcf |= (pid & 0x3) << 1;
	pcf |= 0x00; // noack field
	
	uint8_t header[7] = {0};
	header[6] = pcf;
	header[5] = (pcf >> 7) | (addr[0] << 1);
	header[4] = (addr[0] >> 7) | (addr[1] << 1);
	header[3] = (addr[1] >> 7) | (addr[2] << 1);
	header[2] = (addr[2] >> 7) | (addr[3] << 1);
	header[1] = (addr[3] >> 7) | (addr[4] << 1);
	header[0] = (addr[4] >> 7);

	// calculate the crc
	union 
	{
		uint8_t bytes[2];
		uint16_t val;
	} crc;

	crc.val=0x3c18;
	for (i = 0; i < 7; ++i)
		crc.val=crc16_update(crc.val,header[i],8);
	for (i = 0; i < *len; ++i)
		crc.val=crc16_update(crc.val,payload[i],8);

	// encode payload and crc
	// xor with this:
	for (i = 0; i < *len; ++i)
		payload[i] ^= ssv_xor[i];
	crc.bytes[1] ^= ssv_xor[i++];
	crc.bytes[0] ^= ssv_xor[i++];

	// pack the pcf, payload, and crc into packed_payload
	packed_payload[0] = pcf >> 1;
	packed_payload[1] = (pcf << 7) | (payload[0] >> 1);
	
	for (i = 0; i < *len - 1; ++i)
		packed_payload[i+2] = (payload[i] << 7) | (payload[i+1] >> 1);

	packed_payload[i+2] = (payload[i] << 7) | (crc.val >> 9);
	++i;
	packed_payload[i+2] = (crc.val >> 1 & 0x80 ) | (crc.val >> 1 & 0x7F);
	++i;
	packed_payload[i+2] = (crc.val << 7);

	*len += 4;
}

static void __attribute__((unused)) FQ777_send_packet(uint8_t bind)
{
	uint8_t packet_len = FQ777_PACKET_SIZE;
	uint8_t packet_ori[8];
	if (bind)
	{
		// 4,5,6 = address fields
		// last field is checksum of address fields
		packet_ori[0] = 0x20;
		packet_ori[1] = 0x15;
		packet_ori[2] = 0x05;
		packet_ori[3] = 0x06;
		packet_ori[4] = rx_tx_addr[0];
		packet_ori[5] = rx_tx_addr[1];
		packet_ori[6] = rx_tx_addr[2];
		packet_ori[7] = packet_ori[4] + packet_ori[5] + packet_ori[6];
	}
	else
	{
		// throt, yaw, pitch, roll, trims, flags/left button,00,right button
		//0-3 0x00-0x64
		//4 roll/pitch/yaw trims. cycles through one trim at a time - 0-40 trim1, 40-80 trim2, 80-C0 trim3 (center:  A0 20 60)
		//5 flags for throttle button, two buttons above throttle - def: 0x40
		//6 00 ??
		//7 checksum - add values in other fields 

		
		// Trims are usually done through the radio configuration but leaving the code here just in case...
		uint8_t trim_mod  = packet_count % 144;
		uint8_t trim_val  = 0;
		if (36 <= trim_mod && trim_mod < 72) // yaw
			trim_val  = 0x20; // don't modify yaw trim
		else
			if (108 < trim_mod && trim_mod) // pitch
				trim_val = 0xA0;
			else // roll
				trim_val = 0x60;

		packet_ori[0] = convert_channel_16b_limit(THROTTLE,0,0x64);
		packet_ori[1] = convert_channel_16b_limit(RUDDER,0,0x64);
		packet_ori[2] = convert_channel_16b_limit(ELEVATOR,0,0x64);
		packet_ori[3] = convert_channel_16b_limit(AILERON,0,0x64);
		packet_ori[4] = trim_val; // calculated above
		packet_ori[5] = GET_FLAG(CH5_SW, FQ777_FLAG_FLIP)
				  | GET_FLAG(CH7_SW, FQ777_FLAG_HEADLESS)
				  | GET_FLAG(!CH6_SW, FQ777_FLAG_RETURN)
				  | GET_FLAG(CH8_SW,FQ777_FLAG_EXPERT);
		packet_ori[6] = 0x00;
		// calculate checksum
		uint8_t checksum = 0;
		for (int i = 0; i < 7; ++i)
			checksum += packet_ori[i];
		packet_ori[7] = checksum;

		packet_count++;
	}

	ssv_pack_dpl( (0 == bind) ? rx_tx_addr : FQ777_bind_addr, hopping_frequency_no, &packet_len, packet_ori, packet);
	
	NRF24L01_WriteReg(NRF24L01_00_CONFIG,_BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no %= FQ777_NUM_RF_CHANNELS;
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, packet_len);
	NRF24L01_WritePayload(packet, packet_len);
	NRF24L01_WritePayload(packet, packet_len);
}

static void __attribute__((unused)) FQ777_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, FQ777_bind_addr, 5);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowledgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x00);
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
	NRF24L01_SetBitrate(NRF24L01_BR_250K);
	NRF24L01_SetPower();
    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
}

uint16_t FQ777_callback()
{
	if(bind_counter!=0)
	{
		FQ777_send_packet(1);
		bind_counter--;
		if (bind_counter == 0)
		{
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
			BIND_DONE;
		}
	}
	else
		FQ777_send_packet(0);
	return FQ777_PACKET_PERIOD;
}

uint16_t initFQ777(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
	bind_counter = FQ777_BIND_COUNT;
	packet_count=0;
	hopping_frequency[0] = 0x4D;
	hopping_frequency[1] = 0x43;
	hopping_frequency[2] = 0x27;
	hopping_frequency[3] = 0x07;
	hopping_frequency_no=0;
	rx_tx_addr[2] = 0x00;
	rx_tx_addr[3] = 0xe7;
	rx_tx_addr[4] = 0x67;
	FQ777_init();
	return	FQ777_INITIAL_WAIT;
}

#endif
