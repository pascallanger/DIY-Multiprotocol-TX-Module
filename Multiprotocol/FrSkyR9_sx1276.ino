#if defined(FRSKYR9_SX1276_INO)
#include "iface_sx1276.h"

#define FREQ_MAP_SIZE	29

uint32_t FrSkyR9_freq_map[FREQ_MAP_SIZE];

enum {
	FRSKYR9_FREQ=0,
	FRSKYR9_DATA,
	FRSKYR9_RX1,
	FRSKYR9_RX2,
};

static void __attribute__((unused)) FrSkyR9_build_freq()
{
	uint32_t start_freq=914472960;		// 915
	if(sub_protocol & 0x01 && IS_BIND_DONE)
		start_freq=859504640;			// 868 and bind completed
	for(uint8_t i=0;i<FREQ_MAP_SIZE-2;i++)
	{
		FrSkyR9_freq_map[i]=start_freq;
		debugln("F%d=%lu", i, FrSkyR9_freq_map[i]);
		start_freq+=0x7A000;
	}
    // Last two frequencies determined by FrSkyX_chanskip
	FrSkyR9_freq_map[FREQ_MAP_SIZE-2] = FrSkyR9_freq_map[FrSkyX_chanskip];
	debugln("F%d=%lu", FREQ_MAP_SIZE-2, FrSkyR9_freq_map[FREQ_MAP_SIZE-2]);
	FrSkyR9_freq_map[FREQ_MAP_SIZE-1] = FrSkyR9_freq_map[FrSkyX_chanskip+1];
	debugln("F%d=%lu", FREQ_MAP_SIZE-1, FrSkyR9_freq_map[FREQ_MAP_SIZE-1]);
	hopping_frequency_no = 0;
}

static void __attribute__((unused)) FrSkyR9_build_packet()
{
	//ID
	packet[0] = rx_tx_addr[1];
	packet[1] = rx_tx_addr[2];
	packet[2] = rx_tx_addr[3];

	//Hopping
	packet[3] = hopping_frequency_no;	// current channel index
	packet[4] = FrSkyX_chanskip;		// step size and last 2 channels start index

	//RX number
	packet[5] = RX_num;					// receiver number from OpenTX

	// Set packet[6]=failsafe, packet[7]=0?? and packet[8..19]=channels data
	FrSkyX_channels(6);

	//Bind
	if(IS_BIND_IN_PROGRESS)
	{// 915 0x01=CH1-8_TELEM_ON 0x41=CH1-8_TELEM_OFF 0xC1=CH9-16_TELEM_OFF 0x81=CH9-16_TELEM_ON
		packet[6] = 0x01;				// bind indicator
		if(sub_protocol & 1)
			packet[6] |= 0x20;			// 868
		if(binding_idx&0x01)
			packet[6] |= 0x40;			// telem OFF
		if(binding_idx&0x02)
			packet[6] |= 0x80;			// ch9-16
	}

	//SPort
	packet[20] = FrSkyX_RX_Seq << 4;
	packet[20] |= FrSkyX_TX_Seq ;		//TX=8 at startup
	if ( !(FrSkyX_TX_IN_Seq & 0xF8) )
		FrSkyX_TX_Seq = ( FrSkyX_TX_Seq + 1 ) & 0x03 ;	// Next iteration send next packet
	packet[21] = 0x00;					// length?
	packet[22] = 0x00;					// data1?
	packet[23] = 0x00;					// data2?

	//CRC
	uint16_t crc = FrSkyX_crc(packet, 24);
	packet[24] = crc;					// low byte
	packet[25] = crc >> 8;				// high byte
}

uint16_t initFrSkyR9()
{
	set_rx_tx_addr(MProtocol_id_master);

	//FrSkyX_chanskip
	FrSkyX_chanskip = 1 + (random(0xfefefefe) % 24);
	debugln("Step=%d", FrSkyX_chanskip);
	
	//Frequency table
	FrSkyR9_build_freq();
	
	//Set FrSkyFormat
	if((sub_protocol & 0x02) == 0)
		FrSkyFormat=0;					// 16 channels
	else
		FrSkyFormat=1;					// 8 channels
	debugln("%dCH", FrSkyFormat&1 ? 8:16);
	
	//SX1276 Init
	SX1276_SetMode(true, false, SX1276_OPMODE_SLEEP);
	SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);

	// uint8_t buffer[2];
	// buffer[0] = 0x00;
	// buffer[1] = 0x00;
	// SX1276_WriteRegisterMulti(SX1276_40_DIOMAPPING1, buffer, 2);

	SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF6);
	SX1276_ConfigModem1(SX1276_MODEM_CONFIG1_BW_500KHZ, SX1276_MODEM_CONFIG1_CODING_RATE_4_5, true);
	SX1276_ConfigModem2(6, false, false);
	SX1276_ConfigModem3(false, false);
	SX1276_SetPreambleLength(9);
	SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF6);
	SX1276_SetLna(1, true);
	SX1276_SetHopPeriod(0);				// 0 = disabled, we hop frequencies manually
	SX1276_SetPaDac(true);
	SX1276_SetTxRxMode(TX_EN);			// Set RF switch to TX

	FrSkyX_telem_init();
	
	phase=FRSKYR9_FREQ;
	return 20000;						// start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
	static boolean bind=false;
	switch (phase)
	{
		case FRSKYR9_FREQ:
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//Set frequency
			//868 is binding at 915, need to switch back to 868 once bind is completed
			if(bind && IS_BIND_DONE)
				FrSkyR9_build_freq();
			bind=IS_BIND_IN_PROGRESS;
			hopping_frequency_no = (hopping_frequency_no + FrSkyX_chanskip) % FREQ_MAP_SIZE;
			SX1276_SetFrequency(FrSkyR9_freq_map[hopping_frequency_no]); // set current center frequency
			//Set power
			// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
			// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
			SX1276_SetPaConfig(true, 7, 0);
			//Build packet
			FrSkyR9_build_packet();
			phase++;
			return 460;					// Frequency settle time
		case FRSKYR9_DATA:
			//Set RF switch to TX
			SX1276_SetTxRxMode(TX_EN);
			//Send packet
			SX1276_WritePayloadToFifo(packet, 26);
			SX1276_SetMode(true, false, SX1276_OPMODE_TX);
#if not defined TELEMETRY
			phase=FRSKYR9_FREQ;
			return 20000-460;
#else
			phase++;
			return 11140;				// Packet send time
		case FRSKYR9_RX1:
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//RX packet size is 13
			SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 13);
			//Reset pointer
			SX1276_WriteReg(SX1276_0D_FIFOADDRPTR, 0x00);
			//Set RF switch to RX
			SX1276_SetTxRxMode(RX_EN);
			//Switch to RX
			SX1276_WriteReg(SX1276_01_OPMODE, 0x85);
			phase++;
			return 7400;
		case FRSKYR9_RX2:
			if( (SX1276_ReadReg(SX1276_12_REGIRQFLAGS)&0xF0) == (_BV(SX1276_REGIRQFLAGS_RXDONE) | _BV(SX1276_REGIRQFLAGS_VALIDHEADER)) )
			{
				if(SX1276_ReadReg(SX1276_13_REGRXNBBYTES)==13)
				{
					SX1276_ReadRegisterMulti(SX1276_00_FIFO,packet_in,13);
					if( packet_in[9]==rx_tx_addr[1] && packet_in[10]==rx_tx_addr[2] && FrSkyX_crc(packet_in, 11, rx_tx_addr[1]+(rx_tx_addr[2]<<8))==(packet_in[11]+(packet_in[12]<<8)) )
					{
						if(packet_in[0]&0x80)
							RX_RSSI=packet_in[0]<<1;
						else
							v_lipo1=(packet_in[0]<<1)+1;
						//TX_LQI=~(SX1276_ReadReg(SX1276_19_PACKETSNR)>>2)+1;
						TX_RSSI=SX1276_ReadReg(SX1276_1A_PACKETRSSI)-157;
						for(uint8_t i=0;i<9;i++)
							packet[4+i]=packet_in[i];		//Adjust buffer to match FrSkyX
						frsky_process_telemetry(packet,len);	//Check and parse telemetry packets
						pps_counter++;
						if(TX_LQI==0)
							TX_LQI++;						//Recover telemetry right away
					}
				}
			}
			if (millis() - pps_timer >= 1000)
			{//50pps @20ms
				pps_timer = millis();
				debugln("%d pps", pps_counter);
				TX_LQI = pps_counter<<1;					// Max=100pps
				pps_counter = 0;
			}
			if(TX_LQI==0)
				FrSkyX_telem_init();
			else
				telemetry_link=1;							// Send telemetry out anyway
			//Clear all flags
			SX1276_WriteReg(SX1276_12_REGIRQFLAGS,0xFF);
			phase=FRSKYR9_FREQ;
			break;
#endif
	}
	return 1000;
}

#endif