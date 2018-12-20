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
// Last sync with hexfet new_protocols/flysky_a7105.c dated 2015-09-28

#ifdef AFHDS2A_A7105_INO

#define AFHDS2A_TXPACKET_SIZE	38
#define AFHDS2A_RXPACKET_SIZE	37
#define AFHDS2A_NUMFREQ			16

enum{
	AFHDS2A_PACKET_STICKS,
	AFHDS2A_PACKET_SETTINGS,
	AFHDS2A_PACKET_FAILSAFE,
};

enum{
	AFHDS2A_BIND1,
	AFHDS2A_BIND2,
	AFHDS2A_BIND3,
	AFHDS2A_BIND4,
	AFHDS2A_DATA_INIT,
	AFHDS2A_DATA,
};

static void AFHDS2A_calc_channels()
{
	uint8_t idx = 0;
	uint32_t rnd = MProtocol_id;
	while (idx < AFHDS2A_NUMFREQ)
	{
		uint8_t i;
		uint8_t count_1_42 = 0, count_43_85 = 0, count_86_128 = 0, count_129_168 = 0;
		rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

		uint8_t next_ch = ((rnd >> (idx%32)) % 0xa8) + 1;
		// Keep the distance 2 between the channels - either odd or even
		if (((next_ch ^ MProtocol_id) & 0x01 )== 0)
			continue;
		// Check that it's not duplicate and spread uniformly
		for (i = 0; i < idx; i++)
		{
			if(hopping_frequency[i] == next_ch)
				break;
			if(hopping_frequency[i] <= 42)
				count_1_42++;
			else if (hopping_frequency[i] <= 85)
				count_43_85++;
			else if (hopping_frequency[i] <= 128)
				count_86_128++;
			else
				count_129_168++;
		}
		if (i != idx)
			continue;
		if ((next_ch <= 42 && count_1_42 < 5)
			||(next_ch >= 43 && next_ch <= 85 && count_43_85 < 5)
			||(next_ch >= 86 && next_ch <=128 && count_86_128 < 5)
			||(next_ch >= 129 && count_129_168 < 5))
			hopping_frequency[idx++] = next_ch;
	}
}

#if defined(AFHDS2A_FW_TELEMETRY) || defined(AFHDS2A_HUB_TELEMETRY)
// telemetry sensors ID
enum{
	AFHDS2A_SENSOR_RX_VOLTAGE   = 0x00,
	AFHDS2A_SENSOR_RX_ERR_RATE  = 0xfe,
	AFHDS2A_SENSOR_RX_RSSI      = 0xfc,
	AFHDS2A_SENSOR_RX_NOISE     = 0xfb,
	AFHDS2A_SENSOR_RX_SNR       = 0xfa,
	AFHDS2A_SENSOR_A3_VOLTAGE   = 0x03,
};

static void AFHDS2A_update_telemetry()
{
	// Read TX RSSI
	int16_t temp=256-(A7105_ReadReg(A7105_1D_RSSI_THOLD)*8)/5;		// value from A7105 is between 8 for maximum signal strength to 160 or less
	if(temp<0) temp=0;
	else if(temp>255) temp=255;
	TX_RSSI=temp;
	// AA | TXID | rx_id | sensor id | sensor # | value 16 bit big endian | sensor id ......
	// max 7 sensors per packet
	#ifdef AFHDS2A_FW_TELEMETRY
		if (option & 0x80)
		{
			// forward telemetry to TX, skip rx and tx id to save space
			pkt[0]= TX_RSSI;
			for(int i=9;i < AFHDS2A_RXPACKET_SIZE; i++)
				pkt[i-8]=packet[i];

			telemetry_link=2;
			return;
		}
	#endif
	#ifdef AFHDS2A_HUB_TELEMETRY
		for(uint8_t sensor=0; sensor<7; sensor++)
		{
			// Send FrSkyD telemetry to TX
			uint8_t index = 9+(4*sensor);
			switch(packet[index])
			{
				case AFHDS2A_SENSOR_RX_VOLTAGE:
					//v_lipo1 = packet[index+3]<<8 | packet[index+2];
					v_lipo1 = packet[index+2];
					telemetry_link=1;
					break;
				case AFHDS2A_SENSOR_A3_VOLTAGE:
					v_lipo2 = (packet[index+3]<<5) | (packet[index+2]>>3);	// allows to read voltage up to 4S
					telemetry_link=1;
					break;
				case AFHDS2A_SENSOR_RX_ERR_RATE:
					RX_LQI=packet[index+2];
					break;
				case AFHDS2A_SENSOR_RX_RSSI:
					RX_RSSI = -packet[index+2];
					break;
				case 0xff:
					return;
				/*default:
					// unknown sensor ID
					break;*/
			}
		}
	#endif
}
#endif

static void AFHDS2A_build_bind_packet()
{
	uint8_t ch;
	memcpy( &packet[1], rx_tx_addr, 4);
	memset( &packet[5], 0xff, 4);
	packet[10]= 0x00;
	for(ch=0; ch<AFHDS2A_NUMFREQ; ch++)
		packet[11+ch] = hopping_frequency[ch];
	memset( &packet[27], 0xff, 10);
	packet[37] = 0x00;
	switch(phase)
	{
		case AFHDS2A_BIND1:
			packet[0] = 0xbb;
			packet[9] = 0x01;
			break;
		case AFHDS2A_BIND2:
		case AFHDS2A_BIND3:
		case AFHDS2A_BIND4:
			packet[0] = 0xbc;
			if(phase == AFHDS2A_BIND4)
			{
				memcpy( &packet[5], &rx_id, 4);
				memset( &packet[11], 0xff, 16);
			}
			packet[9] = phase-1;
			if(packet[9] > 0x02)
				packet[9] = 0x02;
			packet[27]= 0x01;
			packet[28]= 0x80;
			break;
	}
}

static void AFHDS2A_build_packet(uint8_t type)
{
	uint16_t val;
	memcpy( &packet[1], rx_tx_addr, 4);
	memcpy( &packet[5], rx_id, 4);
	switch(type)
	{
		case AFHDS2A_PACKET_STICKS:		
			packet[0] = 0x58;
			for(uint8_t ch=0; ch<14; ch++)
			{
				uint16_t channelMicros = convert_channel_ppm(CH_AETR[ch]);
				packet[9 +  ch*2] = channelMicros&0xFF;
				packet[10 + ch*2] = (channelMicros>>8)&0xFF;
			}
			#ifdef AFHDS2A_LQI_CH
				// override channel with LQI
				val = 2000 - 10*RX_LQI;
				packet[9+((AFHDS2A_LQI_CH-1)*2)] = val & 0xff;
				packet[10+((AFHDS2A_LQI_CH-1)*2)] = (val >> 8) & 0xff;
			#endif
			break;
		case AFHDS2A_PACKET_FAILSAFE:
			packet[0] = 0x56;
			for(uint8_t ch=0; ch<14; ch++)
			{
				#ifdef FAILSAFE_ENABLE
					uint16_t failsafeMicros = Failsafe_data[CH_AETR[ch]];
					failsafeMicros = (((failsafeMicros<<2)+failsafeMicros)>>3)+860;
					if( failsafeMicros!=FAILSAFE_CHANNEL_HOLD+860)
					{ // Failsafe values
						packet[9 + ch*2] =  failsafeMicros & 0xff;
						packet[10+ ch*2] = ( failsafeMicros >> 8) & 0xff;
					}
					else
				#endif
					{ // no values
						packet[9 + ch*2] = 0xff;
						packet[10+ ch*2] = 0xff;
					}
			}
			break;
		case AFHDS2A_PACKET_SETTINGS:
			packet[0] = 0xaa;
			packet[9] = 0xfd;
			packet[10]= 0xff;
			val=5*(option & 0x7f)+50;	// option value should be between 0 and 70 which gives a value between 50 and 400Hz
			if(val<50 || val>400) val=50;	// default is 50Hz
			packet[11]= val;
			packet[12]= val >> 8;
			if(sub_protocol == PPM_IBUS || sub_protocol == PPM_SBUS)
				packet[13] = 0x01;	// PPM output enabled
			else
				packet[13] = 0x00;
			packet[14]= 0x00;
			for(uint8_t i=15; i<37; i++)
				packet[i] = 0xff;
			packet[18] = 0x05;		// ?
			packet[19] = 0xdc;		// ?
			packet[20] = 0x05;		// ?
			if(sub_protocol == PWM_SBUS || sub_protocol == PPM_SBUS)
				packet[21] = 0xdd;	// SBUS output enabled
			else
				packet[21] = 0xde;	// IBUS
			break;
	}
	packet[37] = 0x00;
}

#define AFHDS2A_WAIT_WRITE 0x80
uint16_t ReadAFHDS2A()
{
	static uint8_t packet_type;
	static uint16_t packet_counter;
	uint8_t data_rx;
	uint16_t start;
	#ifndef FORCE_AFHDS2A_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	switch(phase)
	{
		case AFHDS2A_BIND1:
		case AFHDS2A_BIND2:
		case AFHDS2A_BIND3:
			AFHDS2A_build_bind_packet();
			A7105_WriteData(AFHDS2A_TXPACKET_SIZE, packet_count%2 ? 0x0d : 0x8c);
			if(!(A7105_ReadReg(A7105_00_MODE) & (1<<5 | 1<<6)))
			{ // FECF+CRCF Ok
				A7105_ReadData(AFHDS2A_RXPACKET_SIZE);
				if(packet[0] == 0xbc && packet[9] == 0x01)
				{
					uint8_t temp=AFHDS2A_EEPROM_OFFSET+RX_num*4;
					for(uint8_t i=0; i<4; i++)
					{
						rx_id[i] = packet[5+i];
						eeprom_write_byte((EE_ADDR)(temp+i),rx_id[i]);
					}
					phase = AFHDS2A_BIND4;
					packet_count++;
					return 3850;
				}
			}
			packet_count++;
			phase |= AFHDS2A_WAIT_WRITE;
			return 1700;
		case AFHDS2A_BIND1|AFHDS2A_WAIT_WRITE:
		case AFHDS2A_BIND2|AFHDS2A_WAIT_WRITE:
		case AFHDS2A_BIND3|AFHDS2A_WAIT_WRITE:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)micros()-start < 700)			// Wait max 700µs, using serial+telemetry exit in about 120µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetPower();
			A7105_SetTxRxMode(TXRX_OFF);					// Turn LNA off since we are in near range and we want to prevent swamping
			A7105_Strobe(A7105_RX);
			phase &= ~AFHDS2A_WAIT_WRITE;
			phase++;
			if(phase > AFHDS2A_BIND3)
				phase = AFHDS2A_BIND1;
			return 2150;
		case AFHDS2A_BIND4:
			AFHDS2A_build_bind_packet();
			A7105_WriteData(AFHDS2A_TXPACKET_SIZE, packet_count%2 ? 0x0d : 0x8c);
			packet_count++;
			bind_phase++;
			if(bind_phase>=4)
			{ 
				hopping_frequency_no=1;
				phase = AFHDS2A_DATA_INIT;
				BIND_DONE;
			}
			return 3850;
		case AFHDS2A_DATA_INIT:
			packet_counter=0;
			packet_type = AFHDS2A_PACKET_STICKS;
			phase = AFHDS2A_DATA;
		case AFHDS2A_DATA:
			AFHDS2A_build_packet(packet_type);
			if((A7105_ReadReg(A7105_00_MODE) & 0x01))		// Check if something has been received...
				data_rx=0;
			else
				data_rx=1;									// Yes
			A7105_WriteData(AFHDS2A_TXPACKET_SIZE, hopping_frequency[hopping_frequency_no++]);
			if(hopping_frequency_no >= AFHDS2A_NUMFREQ)
				hopping_frequency_no = 0;
			if(!(packet_counter % 1313))
				packet_type = AFHDS2A_PACKET_SETTINGS;
			else
			{
				#ifdef FAILSAFE_ENABLE
					if(!(packet_counter % 1569) && IS_FAILSAFE_VALUES_on)
						packet_type = AFHDS2A_PACKET_FAILSAFE;
					else
				#endif
						packet_type = AFHDS2A_PACKET_STICKS;		// todo : check for settings changes
			}
			if(!(A7105_ReadReg(A7105_00_MODE) & (1<<5 | 1<<6)) && data_rx==1)
			{ // RX+FECF+CRCF Ok
				A7105_ReadData(AFHDS2A_RXPACKET_SIZE);
				if(packet[0] == 0xaa)
				{
					if(packet[9] == 0xfc)
						packet_type=AFHDS2A_PACKET_SETTINGS;	// RX is asking for settings
					else
					{
						#ifdef AFHDS2A_LQI_CH
							for(uint8_t sensor=0; sensor<7; sensor++)
							{//read LQI value for RX output
								uint8_t index = 9+(4*sensor);
								if(packet[index]==AFHDS2A_SENSOR_RX_ERR_RATE)
									RX_LQI=packet[index+2];
							}
						#endif
						#if defined(AFHDS2A_FW_TELEMETRY) || defined(AFHDS2A_HUB_TELEMETRY)
							AFHDS2A_update_telemetry();
						#endif
					}
				}
			}
			packet_counter++;
			phase |= AFHDS2A_WAIT_WRITE;
			return 1700;
		case AFHDS2A_DATA|AFHDS2A_WAIT_WRITE:
			//Wait for TX completion
			start=micros();
			while ((uint16_t)micros()-start < 700)			// Wait max 700µs, using serial+telemetry exit in about 120µs
				if(!(A7105_ReadReg(A7105_00_MODE) & 0x01))
					break;
			A7105_SetPower();
			A7105_SetTxRxMode(RX_EN);
			A7105_Strobe(A7105_RX);
			phase &= ~AFHDS2A_WAIT_WRITE;
			return 2150;
	}
	return 3850; // never reached, please the compiler
}

uint16_t initAFHDS2A()
{
	A7105_Init();

	AFHDS2A_calc_channels();
	packet_count = 0;
	bind_phase = 0;
	if(IS_BIND_IN_PROGRESS)
		phase = AFHDS2A_BIND1;
	else
	{
		phase = AFHDS2A_DATA_INIT;
		//Read RX ID from EEPROM based on RX_num, RX_num must be uniq for each RX
		uint8_t temp=AFHDS2A_EEPROM_OFFSET+RX_num*4;
		for(uint8_t i=0;i<4;i++)
			rx_id[i]=eeprom_read_byte((EE_ADDR)(temp+i));
	}
	hopping_frequency_no = 0;
#if defined(AFHDS2A_FW_TELEMETRY) || defined(AFHDS2A_HUB_TELEMETRY)
	init_frskyd_link_telemetry();
#endif
	return 50000;
}
#endif
