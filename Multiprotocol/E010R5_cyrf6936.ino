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

#if defined(E010R5_CYRF6936_INO)

#include "iface_cyrf6936.h"

#define E010R5_FORCE_ID

static void __attribute__((unused)) E010R5_build_data_packet()
{
	uint8_t buf[16];

	//Build the packet
	buf[ 0] = 0x0D;									// Packet length
	buf[ 1] = convert_channel_8b(THROTTLE);
	buf[ 2] = convert_channel_s8b(RUDDER);
	buf[ 3] = convert_channel_s8b(ELEVATOR);
	buf[ 4] = convert_channel_s8b(AILERON);
	buf[ 5] = 0x20;									// Trim Rudder
	buf[ 6] = 0x20;									// Trim Elevator
	buf[ 7] = 0x20;									// Trim Aileron
	buf[ 8] = 0x01									// Flags: high=0x01, low=0x00
			| GET_FLAG(CH5_SW, 0x04)				//        flip=0x04
			| GET_FLAG(CH6_SW, 0x08)				//        led=0x08
			| GET_FLAG(CH8_SW, 0x10)				//		  headless=0x10
			| GET_FLAG(CH9_SW, 0x20);				//		  one key return=0x20
	buf[ 9] = IS_BIND_IN_PROGRESS ? 0x80 : 0x00		// Flags: bind=0x80
			| GET_FLAG(CH7_SW, 0x20)				//        calib=0x20
			| GET_FLAG(CH10_SW, 0x01);				//		  strange effect=0x01=long press on right button
	buf[10] = rx_tx_addr[0];
	buf[11] = rx_tx_addr[1];
	buf[12] = rx_tx_addr[2];
	buf[13] = 0x9D;									// Check
	for(uint8_t i=0;i<13;i++)
		buf[13] += buf[i];

	//Add CRC
	crc=0x00;
	for(uint8_t i=0;i<14;i++)
		crc16_update(bit_reverse(buf[i]),8);
	buf[14] = bit_reverse(crc>>8);
	buf[15] = bit_reverse(crc);

	#if 0
		debug("B:");
		for(uint8_t i=0; i<16; i++)
			debug(" %02X",buf[i]);
		debugln("");
	#endif

	//Build the payload
	memcpy(packet,"\x0E\x54\x96\xEE\xC3\xC3",6);	// 4 bytes of address followed by 5 FEC encoded
	memset(&packet[6],0x00,70-6);
	
	//FEC encode
	for(uint8_t i=0; i<16; i++)
	{
		for(uint8_t j=0;j<8;j++)
		{
			uint8_t offset=6 + (i<<2) + (j>>1);
			packet[offset] <<= 4;
			if( (buf[i]>>j) & 0x01 )
				packet[offset] |= 0x0C;
			else
				packet[offset] |= 0x03;
		}
	}

	#if 0
		debug("E:");
		for(uint8_t i=0; i<70; i++)
			debug(" %02X",packet[i]);
		debugln("");
	#endif

	//CYRF wants LSB first
	for(uint8_t i=0;i<71;i++)
		packet[i]=bit_reverse(packet[i]);
}

const uint8_t PROGMEM E010R5_init_vals[][2] = {
	{CYRF_02_TX_CTRL, 0x00},		// transmit err & complete interrupts disabled
	{CYRF_05_RX_CTRL, 0x00},		// receive err & complete interrupts disabled
	{CYRF_28_CLK_EN, 0x02},			// Force Receive Clock Enable, MUST be set
	{CYRF_32_AUTO_CAL_TIME, 0x3c},	// must be set to 3C
	{CYRF_35_AUTOCAL_OFFSET, 0x14},	// must be  set to 14
	{CYRF_06_RX_CFG, 0x48},			// LNA manual control, Rx Fast Turn Mode Enable
	{CYRF_1B_TX_OFFSET_LSB, 0x00},	// Tx frequency offset LSB
	{CYRF_1C_TX_OFFSET_MSB, 0x00},	// Tx frequency offset MSB
	{CYRF_0F_XACT_CFG, 0x24},		// Force End State, transaction end state = idle
	{CYRF_03_TX_CFG, 0x00},			// GFSK mode
	{CYRF_12_DATA64_THOLD, 0x0a},	// 64 Chip Data PN Code Correlator Threshold = 10
	{CYRF_0F_XACT_CFG, 0x04},		// Transaction End State = idle
	{CYRF_39_ANALOG_CTRL, 0x01},	// synth setting time for all channels is the same as for slow channels
	{CYRF_0F_XACT_CFG, 0x24},		//Force IDLE
	{CYRF_29_RX_ABORT, 0x00},		//Clear RX abort
	{CYRF_12_DATA64_THOLD, 0x0a},	//set pn correlation threshold
	{CYRF_10_FRAMING_CFG, 0x4a},	//set sop len and threshold
	{CYRF_29_RX_ABORT, 0x0f},		//Clear RX abort?
	{CYRF_03_TX_CFG, 0x00},			// GFSK mode
	{CYRF_10_FRAMING_CFG, 0x4a},	// 0b11000000 //set sop len and threshold
	{CYRF_1F_TX_OVERRIDE, 0x04},	//disable tx CRC
	{CYRF_1E_RX_OVERRIDE, 0x14},	//disable rx crc
	{CYRF_14_EOP_CTRL, 0x00},		//set EOP sync == 0
	{CYRF_01_TX_LENGTH, 70 },		// payload length
};

static void __attribute__((unused)) E010R5_cyrf_init()
{
	for(uint8_t i = 0; i < sizeof(E010R5_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&E010R5_init_vals[i][0]), pgm_read_byte_near(&E010R5_init_vals[i][1]));
	CYRF_WritePreamble(0xAAAA02);
	CYRF_SetTxRxMode(TX_EN);
}

uint16_t ReadE010R5()
{
	//Bind
	if(bind_counter)
	{
		bind_counter--;
		if(bind_counter==0)
			BIND_DONE;
	}

	//Send packet of 71 bytes...
	uint8_t *buffer=packet;
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x40);
	CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, 16);
	CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x80);
	buffer+=16;
	for(uint8_t i=0;i<6;i++)
	{
		while((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)&0x10) == 0);
		CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, 8);
		buffer+=8;
	}
	while((CYRF_ReadRegister(CYRF_04_TX_IRQ_STATUS)&0x10) == 0);
	CYRF_WriteRegisterMulti(CYRF_20_TX_BUFFER, buffer, 6);
	
	//Timing and hopping
	packet_count++;
	switch(packet_count)
	{
		case 1:
		case 2:
		case 4:
		case 5:
			return 1183;
		default:
			hopping_frequency_no++;
			hopping_frequency_no &= 3;
			if(IS_BIND_IN_PROGRESS)
				rf_ch_num = 0x30 + (hopping_frequency_no<<3);
			else
				rf_ch_num = hopping_frequency[hopping_frequency_no];
			CYRF_ConfigRFChannel(rf_ch_num);
			debugln("%d",hopping_frequency[hopping_frequency_no]);
			CYRF_SetPower(0x00);
			packet_count = 0;
		case 3:
			E010R5_build_data_packet();
			return 3400;
	}
	return 0;
}

uint16_t initE010R5()
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = 2600;

    E010R5_cyrf_init();
    
    #ifdef E010R5_FORCE_ID
        hopping_frequency[0]=0x30;	//48
        hopping_frequency[1]=0x45;	//69
        hopping_frequency[2]=0x40;	//64
        hopping_frequency[3]=0x35;	//53
        rx_tx_addr[0]=0x00;
        rx_tx_addr[1]=0x45;
        rx_tx_addr[2]=0x46;
    #endif

	E010R5_build_data_packet();
	CYRF_ConfigRFChannel(hopping_frequency[0]);
	hopping_frequency_no=0;
	packet_count=0;

    return 3400;
}

#endif
