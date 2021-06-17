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

#if defined(MULTI_CONFIG_INO)

#ifdef CYRF6936_INSTALLED
#include "iface_cyrf6936.h"
#endif

void CONFIG_write_GID(uint32_t id)
{
	for(uint8_t i=0;i<4;i++)
		eeprom_write_byte((EE_ADDR)EEPROM_ID_OFFSET+i,id >> (i*8));
	//eeprom_write_byte((EE_ADDR)(EEPROM_ID_OFFSET+10),0xf0);
}

void CONFIG_write_CID(uint8_t *data)
{
	for(uint8_t i=0;i<6;i++)
		eeprom_write_byte((EE_ADDR)EEPROM_CID_OFFSET+i, data[i]);
	//eeprom_write_byte((EE_ADDR)EEPROM_CID_INIT_OFFSET, 0xf0);
}
uint16_t CONFIG_callback()
{
	static uint8_t line=0, page=0;
	uint32_t id=0;
	// [0] = page<<4|line number
	// [1..6] = max 6 bytes
	if(CONFIG_SerialRX)
	{
		debug("config");
		for(uint8_t i=0; i<7; i++)
			debug("%02X ",CONFIG_SerialRX_val[i]);
		debugln("");
		CONFIG_SerialRX = false;
		switch(CONFIG_SerialRX_val[0]&0x0F)
		{
			//case 0:
				// Page change
			//	break;
			case 1:
				for(uint8_t i=0; i<4; i++)
				{
					id <<= 8;
					id |= CONFIG_SerialRX_val[i+1];
				}
				debugln("Update ID to %lx", id);
				CONFIG_write_GID(id);
				break;
			case 2:
				if(CONFIG_SerialRX_val[1]==0xAA)
				{
					#define STM32_UUID ((uint32_t *)0x1FFFF7E8)
					id = STM32_UUID[0] ^ STM32_UUID[1] ^ STM32_UUID[2];
					debugln("Reset GID to %lx", id);
					CONFIG_write_GID(id);
				}
				break;
#ifdef CYRF6936_INSTALLED
			case 4:
				debug("Update CID to ");
				for(uint8_t i=0; i<6; i++)
					debug("%02X ",CONFIG_SerialRX_val[i+1]);
				debugln("");
				CONFIG_write_CID(&CONFIG_SerialRX_val[1]);
			case 5:
				if(CONFIG_SerialRX_val[1]==0xAA)
				{
					uint8_t data[6];
					CYRF_WriteRegister(CYRF_25_MFG_ID, 0xFF);	/* Fuses power on */
					CYRF_ReadRegisterMulti(CYRF_25_MFG_ID, data, 6);
					CYRF_WriteRegister(CYRF_25_MFG_ID, 0x00);	/* Fuses power off */
					debug("Reset CID to ");
					for(uint8_t i=0; i<6; i++)
						debug("%02X ",data[i]);
					debugln("");
					CONFIG_write_CID(data);
				}
				break;
#endif
			case 7:
				if(CONFIG_SerialRX_val[1]==0xAA)
				{
					debugln("Format EE");
					#if defined(STM32_BOARD)
						EEPROM.format();
					#else
						for (uint16_t i = 0; i < 512; i++)
    						EEPROM.write(i, 0xFF);
					#endif
				}
				break;
		}
	}

	if(	telemetry_link )
		return 10000;
	// [0] = page<<4|line number
	// line=0: VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_PATCH_LEVEL, Channel order:RUDDER<<6|THROTTLE<<4|ELEVATOR<<2|AILERON
	// [1..21] = max 20 characters, any displayable chars followed by:
	//    0x00    : end of line
	//    0x80+len:  selectable text to follow
	//    0x90+len:  selectable text to follow with "Are you sure?"
	//    0xA0+len:  not editable dec value
	//    0xB0+len:  editable dec value
	//    0xC0+len:  not editable hex value
	//    0xD0+len:  editable hex value
	memset(&packet_in[1],0,20);
	do
	{
		packet_in[0] = (page<<4) | line;
		switch(line)
		{
			case 0:
				packet_in[1]=VERSION_MAJOR;
				packet_in[2]=VERSION_MINOR;
				packet_in[3]=VERSION_REVISION;
				packet_in[4]=VERSION_PATCH_LEVEL;
				packet_in[5]=RUDDER<<6|THROTTLE<<4|ELEVATOR<<2|AILERON;
				break;
			case 1:
				//Global ID
				#ifndef FORCE_GLOBAL_ID
					memcpy(&packet_in[1],"Global ID",9);
					packet_in[10] = 0xD0 + 4;
				#else
					memcpy(&packet_in[1],"Fixed ID ",9);
					packet_in[10] = 0xC0 + 4;
				#endif
				MProtocol_id_master = random_id(EEPROM_ID_OFFSET,false);
				set_rx_tx_addr(MProtocol_id_master);
				for(uint8_t i=0; i<4; i++)
					packet_in[11+i]=rx_tx_addr[i];
				break;
			#if defined(STM32_BOARD) && not defined(FORCE_GLOBAL_ID)
			case 2:
				//Reset global ID
				packet_in[1] = 0x90+9;
				memcpy(&packet_in[2],"Reset GID",9);
				break;
			#endif
#ifdef CYRF6936_INSTALLED
			case 4:
				//Cyrf ID
				#ifndef FORCE_CYRF_ID
					memcpy(&packet_in[1],"Cyrf ID",7);
					packet_in[8] = 0xD0 + 6;
					CYRF_GetMfgData(&packet_in[9]);
				#else
					memcpy(&packet_in[1],"Fixed CID",9);
					packet_in[10] = 0xC0 + 6;
					CYRF_GetMfgData(&packet_in[11]);
				#endif
				break;
			#ifndef FORCE_CYRF_ID
			case 5:
				//Reset Cyrf ID
				packet_in[1] = 0x90+9;
				memcpy(&packet_in[2],"Reset CID",9);
				break;
			#endif
#endif
			case 7:
				packet_in[1] = 0x90+13;
				memcpy(&packet_in[2],"Format EEPROM",13);
				break;
		}
		line++;
		line %= 8;
	}
	while(packet_in[1]==0);	// next line if empty
	telemetry_link = 1;
	return 10000;
}

void CONFIG_init()
{
}

#endif