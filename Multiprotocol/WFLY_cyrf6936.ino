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

#if defined(WFLY_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define WFLY_FORCE_ID
#define WFLY_BIND_COUNT 1500	// around 15s
#define WFLY_NUM_FREQUENCE 4
#define WFLY_BIND_CHANNEL 0x09

enum {
	WFLY_BIND_TX=0,
	WFLY_BIND_PREP_RX,
	WFLY_BIND_RX,
	WFLY_PREP_DATA,
	WFLY_DATA,
};

const uint8_t PROGMEM WFLY_sop_bind[]={ 0x5A, 0xCC, 0xAE, 0x46, 0xB6, 0x31, 0xAE, 0x46 };
const uint8_t PROGMEM WFLY_sop_data[]={ 0xEF, 0x64, 0xB0, 0x2A, 0xD2, 0x8F, 0xB1, 0x2A };

//Most of the bytes are unknown... 1C A7 looks to be the bind ID, BF 13 is the TX ID, 15 is the channel used to send the hopping frequencies.
const uint8_t PROGMEM WFLY_bind_packet[]={ 0x1C, 0xA7, 0x60, 0x04, 0x04, 0xBF, 0x13, 0x15, 0xC5, 0x40, 0x8A, 0x37, 0xE0, 0xE8, 0x03, 0xA3 };


const uint8_t PROGMEM WFLY_init_vals[][2] = {
	//Init from dump
	{CYRF_1D_MODE_OVERRIDE, 0x19},			// Reset
	{CYRF_32_AUTO_CAL_TIME, 0x3C},			// Default init value
	{CYRF_35_AUTOCAL_OFFSET, 0x14},			// Default init value
	{CYRF_1B_TX_OFFSET_LSB, 0x55},			// Default init value
	{CYRF_1C_TX_OFFSET_MSB, 0x05},			// Default init value
	{CYRF_06_RX_CFG, 0x48 | 0x02},			// LNA enabled, Fast Turn Mode enabled, adding overwrite enable to not lockup RX
	{CYRF_10_FRAMING_CFG, 0xE8},			// SOP enable
	{CYRF_03_TX_CFG, 0x08 | CYRF_BIND_POWER},	// Original=0x0F, 8DR Mode, 32 chip codes
	{CYRF_0C_XTAL_CTRL, 0xC4},				// Enable XOUT as GPIO
	{CYRF_0D_IO_CFG, 0x04},					// Enable PACTL as GPIO
	{CYRF_0F_XACT_CFG, 0x21},				// Abort current operation
	{CYRF_1E_RX_OVERRIDE, 0x00},			// Accept packets with 0 seed for bind
	{CYRF_15_CRC_SEED_LSB, 0x00},			// CRC seed for bind
	{CYRF_16_CRC_SEED_MSB, 0x00},			// CRC seed for bind
};

static void __attribute__((unused)) WFLY_cyrf_bind_config()
{
	for(uint8_t i = 0; i < sizeof(WFLY_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&WFLY_init_vals[i][0]), pgm_read_byte_near(&WFLY_init_vals[i][1]));

    CYRF_PROGMEM_ConfigSOPCode(WFLY_sop_bind);
	CYRF_ConfigRFChannel(WFLY_BIND_CHANNEL);
	CYRF_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) WFLY_cyrf_data_config()
{
	for(uint8_t i = 0; i < (sizeof(WFLY_init_vals) / 2)-3; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&WFLY_init_vals[i][0]), pgm_read_byte_near(&WFLY_init_vals[i][1]));

	//CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x08);	// Do not accept CRC with 0 seed but not needed since the RX is not sending any data...
	CYRF_WriteRegister(CYRF_15_CRC_SEED_LSB, rx_tx_addr[2]);
	CYRF_WriteRegister(CYRF_16_CRC_SEED_MSB, rx_tx_addr[3]);
	
    CYRF_PROGMEM_ConfigSOPCode(WFLY_sop_data);
	CYRF_SetTxRxMode(TX_EN);
}

static uint16_t __attribute__((unused)) WFLY_send_data_packet()
{
	packet_count++;
	packet[0] = rx_tx_addr[2];
	packet[1] = rx_tx_addr[3];
	if(packet_count%4==3)
	{	// Send the hopping frequencies
		packet[2]=0x70;		// packet type
		packet[3]=0x04;		// unknown
		packet[4]=0x00;		// unknown
		packet[5]=0x04;		// unknown
		packet[6]=hopping_frequency[0];
		packet[7]=hopping_frequency[0];
		packet[8]=hopping_frequency[1];
		packet[9]=hopping_frequency[2];
		len=10;				// packet[10] contains the checksum
	}
	else
	{	// Send sticks packet
		uint8_t nbr_ch=option;
		if(nbr_ch<4) nbr_ch=9;			// 4 channels min can be sent, default to 9
		if(nbr_ch>9) nbr_ch=9;			// 9 channels max can be sent
		packet[2]=nbr_ch-3;				// nbr of channels to follow
		packet[3]=packet_count>>2;		// packet counter 0x00..0x3F
		len=4;
		for(uint8_t i=0;i<3;i++)
		{ // Channels
			uint16_t ch = convert_channel_16b_nolimit(i*4+0,151,847);
			uint8_t offset=i*5;
			packet[3+offset]|=ch<<6;
			packet[4+offset]=ch>>2;
			len++;
			if(--nbr_ch==0) break;
			ch = convert_channel_16b_nolimit(i*4+1,151,847);
			packet[5+offset]=ch;
			packet[6+offset]=ch>>8;
			len+=2;
			if(--nbr_ch==0) break;
			ch = convert_channel_16b_nolimit(i*4+2,151,847);
			packet[6+offset]|=ch<<2;
			packet[7+offset]=ch>>6;
			len++;
			if(--nbr_ch==0) break;
			ch = convert_channel_16b_nolimit(i*4+3,151,847);
			packet[7+offset]|=ch<<4;
			packet[8+offset]=ch>>4;
			len++;
			if(--nbr_ch==0) break;
		}
	}

	uint8_t sum=0;
	for(uint8_t i = 0; i < len; i++)
		sum += packet[i];
	packet[len] = sum;

	CYRF_ConfigRFChannel(hopping_frequency[(packet_count)%4]);
	CYRF_SetPower(0x08);
	CYRF_WriteDataPacketLen(packet, len+1);

	switch(packet_count%4)
	{
		case 0:
			return 1393;
		case 1:
			return 1330;
		case 2:
			return 1555;
	}
	return 1093;	// case 3
}

uint16_t ReadWFLY()
{
	uint8_t status,len,sum=0,check=0;
	uint8_t start;
	static uint8_t retry;

	switch(phase)
	{
		case WFLY_BIND_TX:
			CYRF_SetTxRxMode(TX_EN);
			CYRF_WriteDataPacketLen(packet, sizeof(WFLY_bind_packet));
			debug("P=");
			for(uint8_t i=0;i<sizeof(WFLY_bind_packet);i++)
				debug(" %02X",packet[i]);
			debugln(" , L=%02X", sizeof(WFLY_bind_packet));
			phase++;
			if(--bind_counter==0)
			{ // Switch to normal mode
				BIND_DONE;
				phase=WFLY_PREP_DATA;
			}
			return 2500;
		case WFLY_BIND_PREP_RX:
			start=micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 200)				// Wait max 200µs for TX to finish
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;										// Packet transmission complete
			CYRF_SetTxRxMode(RX_EN);							//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);			//Prepare to receive
			retry=10;									//Timeout for RX
			phase=WFLY_BIND_RX;
			return 700;
		case WFLY_BIND_RX:
			//Read data from RX
			status = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((status & 0x03) == 0x02)  						// RXC=1, RXE=0 then 2nd check is required (debouncing)
				status |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// need to set RXOW before data read
			if((status & 0x07) == 0x02)
			{ // Data received with no errors
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				debugln("L=%02X",len)
				if(len==0x10)
				{
					CYRF_ReadDataPacketLen(pkt, len);
					debug("RX=");
					for(uint8_t i=0;i<0x0F;i++)
					{
						debug(" %02X",pkt[i]);
						if(pkt[i]==packet[i])
							check++;							// Verify quickly the content
						sum+=pkt[i];
					}
					debugln(" %02X",pkt[15]);
					if(sum==pkt[15] && check>=10)
					{ // Good packet received
						if(pkt[2]==0x64)
						{ // Switch to normal mode
							BIND_DONE;
							phase=WFLY_PREP_DATA;
							return 10000;
						}
						memcpy((void *)packet,(void *)pkt,0x10);	// Send back to the RX what we've just received with no modifications
					}
					phase=WFLY_BIND_TX;							
					return 200;
				}
			}
			if(status & 0x85 || --retry == 0)
			{ // RX error or no answer
				debugln("Abort");
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);		// Enable RX abort
				CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x21);		// Force end state
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);		// Disable RX abort
				phase=WFLY_BIND_TX;								// Retry sending bind packet
			}
			return 700;
		case WFLY_PREP_DATA:
			WFLY_cyrf_data_config();
			packet_count=0;
			phase++;
		case WFLY_DATA:
			start=micros();
			while ((uint8_t)((uint8_t)micros()-(uint8_t)start) < 200)
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;										// Packet transmission complete
			return WFLY_send_data_packet();
	}
	return 1000;
}

uint16_t initWFLY()
{ 
	//Random start channel
	uint8_t ch=0x0A+random(0xfefefefe)%0x0E;
	if(ch%3==0)
		ch++;								// remove these channels as they seem to not be working...
	rf_ch_num=0x0C+(rx_tx_addr[1]%4)*3;		// use the start channels which do not seem to work to send the hopping table instead
	
	#ifdef WFLY_FORCE_ID					// data taken from TX dump
		rx_tx_addr[2]=0xBF;					// ID
		rx_tx_addr[3]=0x13;					// ID
		ch=0x16;							// value seen between 0x0A and 0x17
		rc_ch_num=0x15						// RF channel to send the current hopping table
	#endif

	debug("ID:")
	for(uint8_t i=0;i<2;i++)
		debug(" %02X", rx_tx_addr[2+i]);
	debugln("");

	hopping_frequency[0]=ch;
	hopping_frequency[1]=ch+0x1E;
	hopping_frequency[2]=ch+0x2D;
	hopping_frequency[3]=rf_ch_num;			// RF channel used to send the current hopping table
	
	debug("RF Channels:")
	for(uint8_t i=0;i<WFLY_NUM_FREQUENCE;i++)
		debug(" %02X", hopping_frequency[i]);
	debugln("");

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter=WFLY_BIND_COUNT;
		WFLY_cyrf_bind_config();
		for(uint8_t i=0;i<sizeof(WFLY_bind_packet);i++)
			packet[i]=pgm_read_byte_near(&WFLY_bind_packet[i]);
		packet[5]=rx_tx_addr[2];
		packet[6]=rx_tx_addr[3];
		packet[7]=rf_ch_num;
		uint8_t sum=0;
		for(uint8_t i = 0; i < 15; i++)
			sum += packet[i];
		packet[15] = sum;
		phase=WFLY_BIND_TX;
	}
	else
		phase = WFLY_PREP_DATA;
	return 10000;
}

#endif
