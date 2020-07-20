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

#if defined(FRSKYX_CC2500_INO)

#include "iface_cc2500.h"

static void __attribute__((unused)) FrSkyX_build_bind_packet()
{
	uint8_t packet_size = 0x1D;
	if(protocol==PROTO_FRSKYX && (FrSkyFormat & 2 ))
		packet_size=0x20;				// FrSkyX V1 LBT
	//Header
	packet[0] = packet_size;			// Number of bytes in the packet (after this one)
	packet[1] = 0x03;					// Bind packet
	packet[2] = 0x01;					// Bind packet

	//ID
	packet[3] = rx_tx_addr[3];			// ID
	packet[4] = rx_tx_addr[2];			// ID

	if(protocol==PROTO_FRSKYX)
	{
		int idx = ((state -FRSKY_BIND) % 10) * 5;
		packet[5] = idx;
		packet[6] = hopping_frequency[idx++];
		packet[7] = hopping_frequency[idx++];
		packet[8] = hopping_frequency[idx++];
		packet[9] = hopping_frequency[idx++];
		packet[10] = hopping_frequency[idx++];
		packet[11] = rx_tx_addr[1];		// Unknown but constant ID?
		packet[12] = RX_num;
		//
		memset(&packet[13], 0, packet_size - 14);
		if(binding_idx&0x01)
			memcpy(&packet[13],(void *)"\x55\xAA\x5A\xA5",4);	// Telem off
		if(binding_idx&0x02)
			memcpy(&packet[17],(void *)"\x55\xAA\x5A\xA5",4);	// CH9-16
	}
	else
	{
		//packet 1D 03 01 0E 1C 02 00 00 32 0B 00 00 A8 26 28 01 A1 00 00 00 3E F6 87 C7 00 00 00 00 C9 C9
		packet[5] = rx_tx_addr[1];		// Unknown but constant ID?
		packet[6] = RX_num;
		//Bind flags
		packet[7]=0;
		if(binding_idx&0x01)
			packet[7] |= 0x40;				// Telem off
		if(binding_idx&0x02)
			packet[7] |= 0x80;				// CH9-16
		//Unknown bytes
		memcpy(&packet[8],"\x32\x0B\x00\x00\xA8\x26\x28\x01\xA1\x00\x00\x00\x3E\xF6\x87\xC7",16);
		packet[20]^= 0x0E ^ rx_tx_addr[3];	// Update the ID
		packet[21]^= 0x1C ^ rx_tx_addr[2];	// Update the ID
		//Xor
		for(uint8_t i=3; i<packet_size-1; i++)
			packet[i] ^= 0xA7;
	}
	//CRC
	uint16_t lcrc = FrSkyX_crc(&packet[3], packet_size-4);
	packet[packet_size-1] = lcrc >> 8;
	packet[packet_size] = lcrc;

	/*//Debug
	debug("Bind:");
	for(uint8_t i=0;i<=packet_size;i++)
		debug(" %02X",packet[i]);
	debugln("");*/
}

static void __attribute__((unused)) FrSkyX_build_packet()
{
	//0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
    // data frames sent every 9ms; failsafe every 9 seconds
	//
	uint8_t packet_size = 0x1D;
	if(protocol==PROTO_FRSKYX && (FrSkyFormat & 2 ))
		packet_size=0x20;				// FrSkyX V1 LBT
	//Header
	packet[0] = packet_size;			// Number of bytes in the packet (after this one)
	packet[1] = rx_tx_addr[3];			// ID
	packet[2] = rx_tx_addr[2];			// ID
	packet[3] = rx_tx_addr[1];			// Unknown but constant ID?
	//  
	packet[4] = (FrSkyX_chanskip<<6)|hopping_frequency_no; 
	packet[5] = FrSkyX_chanskip>>2;
	packet[6] = RX_num;

	//Channels
	FrSkyX_channels(7);					// Set packet[7]=failsafe, packet[8]=0?? and packet[9..20]=channels data
	
	//Sequence and send SPort
	FrSkyX_seq_sport(21,packet_size-2);	//21=RX|TXseq, 22=bytes count, 23..packet_size-2=data

	//CRC
	uint16_t lcrc = FrSkyX_crc(&packet[3], packet_size-4);
	packet[packet_size-1] = lcrc >> 8;
	packet[packet_size] = lcrc;

	/*//Debug
	debug("Norm:");
	for(uint8_t i=0;i<=packet_size;i++)
		debug(" %02X",packet[i]);
	debugln("");*/
}

uint16_t ReadFrSkyX()
{
	switch(state)
	{	
		default: 
			FrSkyX_set_start(47);		
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			FrSkyX_build_bind_packet();
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteData(packet, packet[0]+1);
			if(IS_BIND_DONE)
				state = FRSKY_BIND_DONE;
			else
				state++;
			return 9000;
		case FRSKY_BIND_DONE:
			FrSkyX_initialize_data(0);
			hopping_frequency_no=0;
			BIND_DONE;
			state++;														//FRSKY_DATA1
			break;

		case FRSKY_DATA1:
			CC2500_Strobe(CC2500_SIDLE);
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);					//Frequency offset hack 
				prev_option = option ;
			}
			FrSkyX_set_start(hopping_frequency_no);
			FrSkyX_build_packet();
			if(FrSkyFormat & 2)
			{// LBT
				CC2500_Strobe(CC2500_SRX);									//Acquire RSSI
				state++;
				return 400;		// LBT
			}
		case FRSKY_DATA2:
			if(FrSkyFormat & 2)
			{
				uint16_t rssi=0;
				for(uint8_t i=0;i<4;i++)
					rssi += CC2500_ReadReg(CC2500_34_RSSI | CC2500_READ_BURST);	// 0.5 db/count, RSSI value read from the RSSI status register is a 2's complement number
				rssi>>=2;
				#if 0
					uint8_t rssi_level=convert_channel_8b(CH16)>>1;			//CH16 0..127
					if ( rssi > rssi_level && rssi < 128)					//test rssi level dynamically
				#else
					if ( rssi > 14 && rssi < 128)							//if RSSI above -65dBm (12=-70) => ETSI requirement
				#endif
				{
					LBT_POWER_on;											//Reduce to low power before transmitting
					debugln("Busy %d %d",hopping_frequency_no,rssi);
				}
			}
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFTX);										//Flush the TXFIFO
			CC2500_SetTxRxMode(TX_EN);
			CC2500_SetPower();
			hopping_frequency_no = (hopping_frequency_no+FrSkyX_chanskip)%47;
			CC2500_WriteData(packet, packet[0]+1);
			state=FRSKY_DATA3;
			if(FrSkyFormat & 2)
				return 4000;	// LBT
			else
				return 5200;	// FCC
		case FRSKY_DATA3:
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFRX);										//Flush the RXFIFO
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SRX);
			state++;
			if(FrSkyFormat & 2)
				return 4200;	// LBT
			else
				return 3400;	// FCC
		case FRSKY_DATA4:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(9000);
			#endif
			#if defined TELEMETRY
				len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
				if (len && len <= 17)										//Telemetry frame is 17 bytes
				{
					//debug("Telem:");
					CC2500_ReadData(packet_in, len);						//Read what has been received so far
					if(len<17)
					{//not all bytes were received
						uint8_t last_len=CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
						if(last_len==17)									//All bytes received
						{
							CC2500_ReadData(packet_in+len, last_len-len);	//Finish to read
							len=17;
						}
					}
					if(len==17 && (protocol==PROTO_FRSKYX || (protocol==PROTO_FRSKYX2 && (packet_in[len-1] & 0x80))) )
					{//Telemetry received with valid crc for FRSKYX2
						//Debug
						//for(uint8_t i=0;i<len;i++)
						//	debug(" %02X",packet_in[i]);
						if(frsky_process_telemetry(packet_in,len))			//Check and process telemetry packet
						{//good packet received
							pps_counter++;
							if(TX_LQI==0)
								TX_LQI++;									//Recover telemetry right away
						}
					}
					//debugln("");
				}
				if (millis() - pps_timer >= 900)
				{//1 packet every 9ms
					pps_timer = millis();
					debugln("%d pps", pps_counter);
					TX_LQI = pps_counter;									//Max=100%
					pps_counter = 0;
				}
				if(TX_LQI==0)
					FrSkyX_telem_init();									//Reset telemetry
				else
					telemetry_link=1;										//Send telemetry out anyway
			#endif
			state = FRSKY_DATA1;
			return 400;	// FCC & LBT
	}
	return 1;		
}

uint16_t initFrSkyX()
{
	set_rx_tx_addr(MProtocol_id_master);
	FrSkyFormat = sub_protocol;
	
	if (sub_protocol==XCLONE_16||sub_protocol==XCLONE_8)
		Frsky_init_clone();
	else if(protocol==PROTO_FRSKYX)
	{
		Frsky_init_hop();
		rx_tx_addr[1]=0x02;		// ID related, hw version?
	}
	else
	{
		#ifdef FRSKYX2_FORCE_ID
			rx_tx_addr[3]=0x0E;
			rx_tx_addr[2]=0x1C;
			FrSkyX_chanskip=18;
		#endif
		rx_tx_addr[1]=0x02;		// ID related, hw version?
		FrSkyX2_init_hop();
	}
	
	packet_count=0;
	while(!FrSkyX_chanskip)
		FrSkyX_chanskip=random(0xfefefefe)%47;

	FrSkyX_init();

	if(IS_BIND_IN_PROGRESS)
	{	   
		state = FRSKY_BIND;
		FrSkyX_initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		FrSkyX_initialize_data(0);
	}
	FrSkyX_telem_init();
	return 10000;
}
#endif
