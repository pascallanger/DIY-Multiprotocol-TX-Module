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
 
 Works with Traxxas 6519 receivers https://traxxas.com/sites/default/files/24CompGuide-2016.jpg .
 */

#if defined(TRAXXAS_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define TRAXXAS_FORCE_ID

#define TRAXXAS_CHANNEL 0x05
#define TRAXXAS_BIND_CHANNEL 0x2B
#define TRAXXAS_PACKET_SIZE 16

enum {
	TRAXXAS_BIND_PREP_RX=0,
	TRAXXAS_BIND_RX,
	TRAXXAS_BIND_TX1,
	TRAXXAS_PREP_DATA,
	TRAXXAS_DATA,
};

const uint8_t PROGMEM TRAXXAS_sop_bind[] ={ 0x3C, 0x37, 0xCC, 0x91, 0xE2, 0xF8, 0xCC, 0x91 };
const uint8_t PROGMEM TRAXXAS_sop_data[] ={ 0xA1, 0x78, 0xDC, 0x3C, 0x9E, 0x82, 0xDC, 0x3C };
//const uint8_t PROGMEM TRAXXAS_sop_check[]={ 0x97, 0xE5, 0x14, 0x72, 0x7F, 0x1A, 0x14, 0x72 };

const uint8_t PROGMEM TRAXXAS_init_vals[][2] = {
	//Init from dump
	{CYRF_0B_PWR_CTRL, 0x00},					// PMU
	{CYRF_32_AUTO_CAL_TIME, 0x3C},				// Default init value
	{CYRF_35_AUTOCAL_OFFSET, 0x14},				// Default init value
	{CYRF_1B_TX_OFFSET_LSB, 0x55},				// Default init value
	{CYRF_1C_TX_OFFSET_MSB, 0x05},				// Default init value
	{CYRF_28_CLK_EN, 0x02},						// Force Receive Clock Enable
	{CYRF_06_RX_CFG, 0x88 | 0x02},				// AGC enabled, Fast Turn Mode enabled, adding overwrite enable to not lockup RX
	{CYRF_1E_RX_OVERRIDE, 0x08},				// Reject packets with 0 seed
	{CYRF_03_TX_CFG, 0x08 | CYRF_BIND_POWER},	// 8DR Mode, 32 chip codes
};

static void __attribute__((unused)) TRAXXAS_cyrf_bind_config()
{
	CYRF_PROGMEM_ConfigSOPCode(TRAXXAS_sop_bind);
	CYRF_WriteRegister(CYRF_15_CRC_SEED_LSB, 0x5A);
	CYRF_WriteRegister(CYRF_16_CRC_SEED_MSB, 0x5A);
	CYRF_ConfigRFChannel(TRAXXAS_BIND_CHANNEL);
}

static void __attribute__((unused)) TRAXXAS_cyrf_data_config()
{
	CYRF_PROGMEM_ConfigSOPCode(TRAXXAS_sop_data);
	#ifdef TRAXXAS_FORCE_ID					// data taken from TX dump
		CYRF_WriteRegister(CYRF_15_CRC_SEED_LSB, 0x1B);
		CYRF_WriteRegister(CYRF_16_CRC_SEED_MSB, 0x3F);
	#else
		CYRF_WriteRegister(CYRF_15_CRC_SEED_LSB, cyrfmfg_id[0]+0xB6);
		CYRF_WriteRegister(CYRF_16_CRC_SEED_MSB, cyrfmfg_id[1]+0x5D);
	#endif
	CYRF_ConfigRFChannel(TRAXXAS_CHANNEL);
	CYRF_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) TRAXXAS_send_data_packet()
{
	packet[0] = 0x01;
	memset(&packet[1],0x00,TRAXXAS_PACKET_SIZE-1);
	//Steering
	uint16_t ch = convert_channel_16b_nolimit(RUDDER,500,1000);
	packet[2]=ch>>8;
	packet[3]=ch;
	//Throttle
	ch = convert_channel_16b_nolimit(THROTTLE,500,1000);
	packet[4]=ch>>8;
	packet[5]=ch;
	//AUX3
	ch = convert_channel_16b_nolimit(AILERON,500,1000);
	packet[6]=ch>>8;
	packet[7]=ch;
	//AUX4???
	ch = convert_channel_16b_nolimit(ELEVATOR,500,1000);
	packet[12]=ch>>8;
	packet[13]=ch;

	CYRF_SetPower(0x08);
	CYRF_WriteDataPacketLen(packet, TRAXXAS_PACKET_SIZE);
}

uint16_t ReadTRAXXAS()
{
	uint8_t status;
	
	switch(phase)
	{
		case TRAXXAS_BIND_PREP_RX:
			TRAXXAS_cyrf_bind_config();
			CYRF_SetTxRxMode(RX_EN);								//Receive mode
			CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);				//Prepare to receive
			packet_count=100;										//Timeout for RX
			phase=TRAXXAS_BIND_RX;
			return 700;
		case TRAXXAS_BIND_RX:
			//Read data from RX
			status = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((status & 0x03) == 0x02)  							// RXC=1, RXE=0 then 2nd check is required (debouncing)
				status |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			debugln("s=%02X",status);
			CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);		// need to set RXOW before data read
			if((status & 0x07) == 0x02)
			{ // Data received with no errors
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				debugln("L=%02X",len)
				if(len==TRAXXAS_PACKET_SIZE)
				{
					CYRF_ReadDataPacketLen(packet, TRAXXAS_PACKET_SIZE);
					debug("RX=");
					for(uint8_t i=0;i<TRAXXAS_PACKET_SIZE;i++)
						debug(" %02X",packet[i]);
					debugln("");
					for(uint8_t i=0;i<6;i++)
						packet[i+1]=cyrfmfg_id[i];
					packet[10]=0x01;
					packet_count=12;
					CYRF_SetTxRxMode(TX_EN);
					phase=TRAXXAS_BIND_TX1;
					return 200;
				}
			}
			if( --packet_count == 0 )
			{ // Retry RX
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);			// Enable RX abort
				CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);			// Force end state
				CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);			// Disable RX abort
				if(--bind_counter != 0)
					phase=TRAXXAS_BIND_PREP_RX;						// Retry receiving bind packet
				else
					phase=TRAXXAS_PREP_DATA;						// Abort binding
			}
			return 700;
		case TRAXXAS_BIND_TX1:
			CYRF_WriteDataPacketLen(packet, TRAXXAS_PACKET_SIZE);
			debug("P=");
			for(uint8_t i=0;i<TRAXXAS_PACKET_SIZE;i++)
				debug(" %02X",packet[i]);
			debugln("");
			if(--packet_count==0)	// Switch to normal mode
				phase=TRAXXAS_PREP_DATA;
			break;
		case TRAXXAS_PREP_DATA:
			BIND_DONE;
			TRAXXAS_cyrf_data_config();
			phase++;
		case TRAXXAS_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(13940);
			#endif
			TRAXXAS_send_data_packet();
			break;
	}
	return 13940;
}

uint16_t initTRAXXAS()
{ 
	CYRF_Reset();

	//Config CYRF registers
	for(uint8_t i = 0; i < sizeof(TRAXXAS_init_vals) / 2; i++)	
		CYRF_WriteRegister(pgm_read_byte_near(&TRAXXAS_init_vals[i][0]), pgm_read_byte_near(&TRAXXAS_init_vals[i][1]));

	//Read CYRF ID
	CYRF_GetMfgData(cyrfmfg_id);
	cyrfmfg_id[0]+=RX_num;

	#ifdef TRAXXAS_FORCE_ID					// data taken from TX dump
		cyrfmfg_id[0]=0x65;					// CYRF MFG ID
		cyrfmfg_id[1]=0xE2;
		cyrfmfg_id[2]=0x5E;
		cyrfmfg_id[3]=0x55;
		cyrfmfg_id[4]=0x4D;
		cyrfmfg_id[5]=0xFE;
	#endif

	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter=100;
		phase = TRAXXAS_BIND_PREP_RX;
	}
	else
		phase = TRAXXAS_PREP_DATA;
	return 1000;
}

/*
Bind phase 1
CHANNEL:	0x2B
SOP_CODE:	0x3C	0x37	0xCC	0x91	0xE2	0xF8	0xCC	0x91
CRC_SEED_LSB:	0x5A
CRC_SEED_MSB:	0x5A
RX1:	0x02	0x4A	0xA3	0x2D	0x1A	0x49	0xFE	0x06	0x00	0x00	0x02	0x01	0x06	0x06	0x00	0x00
TX1:	0x02	0x65	0xE2	0x5E	0x55	0x4D	0xFE	0xEE	0x00	0x00	0x01	0x01	0x06	0x05	0x00	0x00
Note: RX cyrfmfg_id is 0x4A,0xA3,0x2D,0x1A,0x49,0xFE and TX cyrfmfg_id is 0x65,0xE2,0x5E,0x55,0x4D,0xFE

Bind phase 2 (looks like normal mode?)
CHANNEL:	0x05
SOP_CODE:	0xA1	0x78	0xDC	0x3C	0x9E	0x82	0xDC	0x3C
CRC_SEED_LSB:	0x1B
CRC_SEED_MSB:	0x3F
RX2:	0x03	0x4A	0xA3	0x2D	0x1A	0x49	0xFE	0x06	0x00	0x00	0x02	0x01	0x06	0x06	0x00	0x00
TX2:	0x01	0x65	0x01	0xF4	0x03	0xE7	0x02	0x08	0x00	0x00	0x01	0x01	0x02	0xEE	0x00	0x00
Note: TX2 is nearly a normal packet at the exception of the 2nd byte equal to cyrfmfg_id[0]

Bind phase 3 (check?)
CHANNEL:	0x22
SOP_CODE:	0x97	0xE5	0x14	0x72	0x7F	0x1A	0x14	0x72
CRC_SEED_LSB:	0xA5
CRC_SEED_MSB:	0xA5
RX3:	0x04	0x4A	0xA3	0x2D	0x1A	0x49	0xFE	0x06	0x00	0x00	0x02	0x01	0x06	0x06	0x00	0x00

Switch to normal mode
CHANNEL: 	0x05
SOP_CODE:	0xA1	0x78	0xDC	0x3C	0x9E	0x82	0xDC	0x3C
CRC_SEED_LSB:	0x1B
CRC_SEED_MSB:	0x3F
TX3:	0x01	0x00	0x02	0xA8	0x03	0xE7	0x02	0x08	0x00	0x00	0x01	0x01	0x02	0xEE	0x00	0x00
*/
#endif
