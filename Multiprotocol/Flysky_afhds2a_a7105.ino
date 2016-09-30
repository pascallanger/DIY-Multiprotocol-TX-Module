// adaptation de https://github.com/goebish/deviation/blob/c5dd9fcc1441fc05fe9effa4c378886aeb3938d4/src/protocol/flysky_afhds2a_a7105.c
#ifdef AFHDS2A_A7105_INO
	#define EEPROMadress	0		// rx ID 32bit
	#define SERVO_HZ		0		//Frequency's servo 0=50	1=400	2=5		
	
	#define TXPACKET_SIZE	38
	#define RXPACKET_SIZE	37
	#define NUMFREQ			16
	#define TXID_SIZE		4
	#define RXID_SIZE		4

	static uint8_t rxid[RXID_SIZE];
	static uint8_t packet_type;
	static uint8_t bind_reply;


	enum{
		PACKET_STICKS,
		PACKET_SETTINGS,
		PACKET_FAILSAFE,
	};

	enum{
		BIND1,
		BIND2,
		BIND3,
		BIND4,
		DATA1,
	};
	enum {
		PWM_IBUS = 0,
		PPM_IBUS,
		PWM_SBUS,
		PPM_SBUS
	};


	static void build_packet(uint8_t type) {
		switch(type) {
			case PACKET_STICKS:		
				packet[0] = 0x58;
				memcpy( &packet[1], rx_tx_addr, 4);
				memcpy( &packet[5], rxid, 4);
				for(uint8_t ch=0; ch<14; ch++) {
					packet[9 +  ch*2] = Servo_data[CH_AETR[ch]]&0xFF;
					packet[10 + ch*2] = (Servo_data[CH_AETR[ch]]>>8)&0xFF;
				}
				packet[37] = 0x00;
				break;
				
			case PACKET_SETTINGS:
				packet[0] = 0xaa;
				memcpy( &packet[1], rx_tx_addr, 4);
				memcpy( &packet[5], rxid, 4);
				packet[9] = 0xfd;
				packet[10]= 0xff;
				packet[11]= SERVO_HZ & 0xff;
				packet[12]= (SERVO_HZ >> 8) & 0xff;
				if(option == PPM_IBUS || option == PPM_SBUS)
					packet[13] = 0x01; // PPM output enabled
				else
					packet[13] = 0x00;
				packet[14]= 0x00;
				for(uint8_t i=15; i<37; i++)
					packet[i] = 0xff;
				packet[18] = 0x05; // ?
				packet[19] = 0xdc; // ?
				packet[20] = 0x05; // ?
				if(option == PWM_SBUS || option == PPM_SBUS)
					packet[21] = 0xdd; // SBUS output enabled
				else
					packet[21] = 0xde;
				packet[37] = 0x00;
				break;
				
			case PACKET_FAILSAFE:
				packet[0] = 0x56;
				memcpy( &packet[1], rx_tx_addr, 4);
				memcpy( &packet[5], rxid, 4);
				for(uint8_t ch=0; ch<14; ch++) {
					if(ch==0) {
		//			if(ch < Model.num_channels && (Model.limits[ch].flags & CH_FAILSAFE_EN)) {
						uint32_t value = ((uint32_t)Servo_data[AUX11] + 100) * 5 + 1000;
						packet[9 + ch*2] = value & 0xff;
						packet[10+ ch*2] = (value >> 8) & 0xff;
					}
					else {
						packet[9 + ch*2] = 0xff;
						packet[10+ ch*2] = 0xff;
					}
				}
				packet[37] = 0x00;
				break;
		}
	}

	#if defined(TELEMETRY)
		// telemetry sensors ID
		enum{
			SENSOR_RX_VOLTAGE   = 0x00,
			SENSOR_RX_ERR_RATE  = 0xfe,
			SENSOR_RX_RSSI      = 0xfc,
			SENSOR_RX_NOISE     = 0xfb,
			SENSOR_RX_SNR       = 0xfa,
		};

		static void update_telemetry() {
			// AA | TXID | RXID | sensor id | sensor # | value 16 bit big endian | sensor id ......
			// max 7 sensors per packet

			for(uint8_t sensor=0; sensor<7; sensor++) {
				uint8_t index = 9+(4*sensor);
				switch(packet[index]) {
					case SENSOR_RX_VOLTAGE:
						v_lipo = packet[index+3]<<8 | packet[index+2];
						telemetry_link=1;
						break;
					case SENSOR_RX_ERR_RATE:
						// packet[index+2];
						break;
					case SENSOR_RX_RSSI:
						RSSI_dBm = -packet[index+2];
						break;
					case 0xff:
						return;
					default:
						// unknown sensor ID
						break;
				}
			}
		}
	#endif

	static void afhds2a_build_bind_packet() {
		uint8_t ch;
		memcpy( &packet[1], rx_tx_addr, 4);
		memset( &packet[5], 0xff, 4);
		packet[10]= 0x00;
		for(ch=0; ch<16; ch++) {
			packet[11+ch] = hopping_frequency[ch];
		}
		memset( &packet[27], 0xff, 10);
		packet[37] = 0x00;
		switch(phase) {
			case BIND1:
				packet[0] = 0xbb;
				packet[9] = 0x01;
				break;
			case BIND2:
			case BIND3:
			case BIND4:
				packet[0] = 0xbc;
				if(phase == BIND4) {
					memcpy( &packet[5], &rxid, 4);
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

	static void calc_afhds_channels(uint32_t seed) {
		int idx = 0;
		uint32_t rnd = seed;
		while (idx < NUMFREQ) {
			int i;
			int count_1_42 = 0, count_43_85 = 0, count_86_128 = 0, count_129_168=0;
			rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

			uint8_t next_ch = ((rnd >> (idx%32)) % 0xa8) + 1;
			// Keep the distance 2 between the channels - either odd or even
			if (((next_ch ^ seed) & 0x01 )== 0)
				continue;
			// Check that it's not duplicate and spread uniformly
			for (i = 0; i < idx; i++) {
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
				{
				hopping_frequency[idx++] = next_ch;
			}
		}
	}

	#define WAIT_WRITE 0x80
	static uint16_t afhds2a_cb() {
		switch(phase) {
			case BIND1:
			case BIND2:
			case BIND3:    
				A7105_Strobe(A7105_STANDBY);
				afhds2a_build_bind_packet();
				A7105_WriteData(38, packet_count%2 ? 0x0d : 0x8c);
				if(A7105_ReadReg(0) == 0x1b) { // todo: replace with check crc+fec
					A7105_Strobe(A7105_RST_RDPTR);
					A7105_ReadData(RXPACKET_SIZE);
					if(packet[0] == 0xbc) {
						for(uint8_t i=0; i<4; i++) {
							rxid[i] = packet[5+i];
						}
						eeprom_write_block((const void*)rxid,(void*)EEPROMadress,4);
						if(packet[9] == 0x01)
							phase = BIND4;
					}
				}
				packet_count++;
				phase |= WAIT_WRITE;
				return 1700;
			case BIND1|WAIT_WRITE:
			case BIND2|WAIT_WRITE:
			case BIND3|WAIT_WRITE:
				A7105_Strobe(A7105_RX);
				phase &= ~WAIT_WRITE;
				phase++;
				if(phase > BIND3)
				phase = BIND1;
				return 2150;
			case BIND4:
				A7105_Strobe(A7105_STANDBY);
				afhds2a_build_bind_packet();
				A7105_WriteData(38, packet_count%2 ? 0x0d : 0x8c);
				packet_count++;
				bind_reply++;
				if(bind_reply>=4) { 
					packet_count=0;
					channel=1;
					phase = DATA1;
					BIND_DONE;
				}                        
				phase |= WAIT_WRITE;
				return 1700;
			case BIND4|WAIT_WRITE:
				A7105_Strobe(A7105_RX);
				phase &= ~WAIT_WRITE;
				return 2150;
			case DATA1:    
				A7105_Strobe(A7105_STANDBY);
				build_packet(packet_type);
				A7105_WriteData(38, hopping_frequency[channel++]);
				if(channel >= 16)
				channel = 0;
				if(!(packet_count % 1313))
					packet_type = PACKET_SETTINGS;
				else if(!(packet_count % 1569))
					packet_type = PACKET_FAILSAFE;
				else
					packet_type = PACKET_STICKS; // todo : check for settings changes
				// got some data from RX ?
				// we've no way to know if RX fifo has been filled
				// as we can't poll GIO1 or GIO2 to check WTR
				// we can't check A7105_MASK_TREN either as we know
				// it's currently in transmit mode.
				if(!(A7105_ReadReg(0) & (1<<5 | 1<<6))) { // FECF+CRCF Ok
					A7105_Strobe(A7105_RST_RDPTR);
					A7105_ReadData(1);
					if(packet[0] == 0xaa) {
						A7105_Strobe(A7105_RST_RDPTR);
						A7105_ReadData(RXPACKET_SIZE);
						if(packet[9] == 0xfc) { // rx is asking for settings
							packet_type=PACKET_SETTINGS;
						}
						else {
							#if defined(TELEMETRY)
								update_telemetry();
							#endif
						}
					}
				}
				packet_count++;
				phase |= WAIT_WRITE;
				return 1700;
			case DATA1|WAIT_WRITE:
				phase &= ~WAIT_WRITE;
				A7105_Strobe(A7105_RX);
				return 2150;
		}
		return 3850; // never reached, please the compiler
	}

	static uint16_t AFHDS2A_setup()
	{
		A7105_Init(INIT_FLYSKY_AFHDS2A);	//flysky_init();

		calc_afhds_channels(MProtocol_id);
		packet_type = PACKET_STICKS;
		packet_count = 0;
		bind_reply = 0;
		if(IS_AUTOBIND_FLAG_on) {
			phase = BIND1;
			BIND_IN_PROGRESS;
		}
		else {
			phase = DATA1;
			eeprom_read_block((void*)rxid,(const void*)EEPROMadress,4);
		}
		channel = 0;
		return 50000;
	}
#endif
