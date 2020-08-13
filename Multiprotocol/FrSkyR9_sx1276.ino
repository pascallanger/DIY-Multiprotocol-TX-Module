#if defined(FRSKYR9_SX1276_INO)
#include "iface_sx1276.h"

#define DISP_FREQ_TABLE

#define FLEX_FREQ	29
#define FCC_FREQ	43
#define EU_FREQ		19

enum {
	FRSKYR9_FREQ=0,
	FRSKYR9_DATA,
	FRSKYR9_RX1,
	FRSKYR9_RX2,
};

void FrSkyR9_set_frequency()
{
	uint8_t data[3];
	uint16_t num=0;
	hopping_frequency_no += FrSkyX_chanskip;
	switch(sub_protocol & 0xFD)
	{
		case R9_868:
			if(IS_BIND_DONE)							// if bind is in progress use R9_915 instead
			{
				hopping_frequency_no %= FLEX_FREQ;
				num=hopping_frequency_no;
				if(hopping_frequency_no>=FLEX_FREQ-2)
					num+=FrSkyX_chanskip-FLEX_FREQ+2;	// the last 2 values are FrSkyX_chanskip and FrSkyX_chanskip+1
				num <<= 5;
				num += 0xD700;
				break;
			}//else use R9_915
		case R9_915:
			hopping_frequency_no %= FLEX_FREQ;
			num=hopping_frequency_no;
			if(hopping_frequency_no>=FLEX_FREQ-2)
				num+=FrSkyX_chanskip-FLEX_FREQ+2;		// the last 2 values are FrSkyX_chanskip and FrSkyX_chanskip+1
			num <<= 5;
			num += 0xE4C0;
			break;
		case R9_FCC:
			hopping_frequency_no %= FCC_FREQ;
			num=hopping_frequency_no;
			num <<= 5;
			num += 0xE200;
			break;
		case R9_EU:
			hopping_frequency_no %= EU_FREQ;
			num=hopping_frequency_no;
			num <<= 4;
			num += 0xD7D0;
			break;
	}
	data[0] = num>>8;
	data[1] = num&0xFF;
	data[2] = 0x00;

	#ifdef DISP_FREQ_TABLE
		if(phase==0xFF)
			debugln("F%d=%02X%02X%02X=%lu", hopping_frequency_no, data[0], data[1], data[2], (uint32_t)((data[0]<<16)+(data[1]<<8)+data[2])*61);
	#endif
	SX1276_WriteRegisterMulti(SX1276_06_FRFMSB, data, 3);
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

	//Channels
	FrSkyX_channels(6);					// Set packet[6]=failsafe, packet[7]=0?? and packet[8..19]=channels data

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

	//Sequence and send SPort
	FrSkyX_seq_sport(20,23);			//20=RX|TXseq, 21=bytes count, 22&23=data

	//CRC
	uint16_t crc = FrSkyX_crc(packet, 24);
	packet[24] = crc;					// low byte
	packet[25] = crc >> 8;				// high byte
}

static uint8_t __attribute__((unused)) FrSkyR9_CRC8(uint8_t *p, uint8_t l)
{
	uint8_t crc = 0xFF;
	for (uint8_t i = 0; i < l; i++)
    {
		crc = crc ^ p[i];
		for ( uint8_t j = 0; j < 8; j++ ) 
			if ( crc & 0x80 )
			{
				crc <<= 1;
				crc ^= 0x07;
			}
			else
				crc <<= 1;
	}
	return crc;
}

static void __attribute__((unused)) FrSkyR9_build_EU_packet()
{
	//ID
	packet[0] = rx_tx_addr[1];
	packet[1] = rx_tx_addr[2];
	packet[2] = rx_tx_addr[3];

	//Hopping
	packet[3] = FrSkyX_chanskip;		// step size and last 2 channels start index

	//RX number
	packet[4] = RX_num;					// receiver number from OpenTX

	//Channels
	//TODO FrSkyX_channels(5,4);			// Set packet[5]=failsafe and packet[6..11]=4 channels data

	//Bind
	if(IS_BIND_IN_PROGRESS)
	{
		packet[5] = 0x01;				// bind indicator
		if((sub_protocol & 2) == 0)
			packet[5] |= 0x10;			// 16CH
		// if(sub_protocol & 1)
			// packet[5] |= 0x20;			// 868
		if(binding_idx&0x01)
			packet[5] |= 0x40;			// telem OFF
		if(binding_idx&0x02)
			packet[5] |= 0x80;			// ch9-16
	}

	//Sequence and send SPort
	packet[12] = (FrSkyX_RX_Seq << 4)|0x08;	//TX=8 at startup

	//CRC
	packet[13] = FrSkyR9_CRC8(packet, 13);
}

uint16_t initFrSkyR9()
{
	//Check frequencies
	#ifdef DISP_FREQ_TABLE
		phase=0xFF;
		FrSkyX_chanskip=1;
		hopping_frequency_no=0xFF;
		for(uint8_t i=0;i<FCC_FREQ;i++)
			FrSkyR9_set_frequency();
	#endif

	//Reset ID
	set_rx_tx_addr(MProtocol_id_master);

	//FrSkyX_chanskip
	FrSkyX_chanskip = 1 + (random(0xfefefefe) % 24);
	debugln("chanskip=%d", FrSkyX_chanskip);
	
	//Set FrSkyFormat
	if((sub_protocol & 0x02) == 0)
		FrSkyFormat=0;											// 16 channels
	else
		FrSkyFormat=1;											// 8 channels
	debugln("%dCH", FrSkyFormat&1 ? 8:16);
	
	//EU packet length
	if( (sub_protocol & 0xFD) == R9_EU )
		packet_length=14;
	else
		packet_length=26;

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
	SX1276_SetHopPeriod(0);										// 0 = disabled, we hop frequencies manually
	SX1276_SetPaDac(true);
	SX1276_SetTxRxMode(TX_EN);									// Set RF switch to TX
	//Enable all IRQ flags
	SX1276_WriteReg(SX1276_11_IRQFLAGSMASK,0x00);
	FrSkyX_telem_init();
	
	hopping_frequency_no=0;
	phase=FRSKYR9_FREQ;
	return 20000;												// Start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
	switch (phase)
	{
		case FRSKYR9_FREQ:
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//Set frequency
			FrSkyR9_set_frequency(); 							// Set current center frequency
			//Set power
			// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
			// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
			SX1276_SetPaConfig(true, 7, 0);						// Lowest power for the T18
			//Build packet
			if( packet_length == 26 )
				FrSkyR9_build_packet();
			else
				FrSkyR9_build_EU_packet();
			phase++;
			return 460;											// Frequency settle time
		case FRSKYR9_DATA:
			//Set RF switch to TX
			SX1276_SetTxRxMode(TX_EN);
			//Send packet
			SX1276_WritePayloadToFifo(packet, packet_length);
			SX1276_SetMode(true, false, SX1276_OPMODE_TX);
#if not defined TELEMETRY
			phase=FRSKYR9_FREQ;
			return 20000-460;
#else
			phase++;
			return 11140;										// Packet send time
		case FRSKYR9_RX1:
			//Force standby
			SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
			//RX packet size is 13
			SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 13);
			//Reset pointer
			SX1276_WriteReg(SX1276_0D_FIFOADDRPTR, 0x00);
			//Set RF switch to RX
			SX1276_SetTxRxMode(RX_EN);
			//Clear all IRQ flags
			SX1276_WriteReg(SX1276_12_REGIRQFLAGS,0xFF);
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
							packet[4+i]=packet_in[i];			// Adjust buffer to match FrSkyX
						frsky_process_telemetry(packet,len);	// Process telemetry packet
						pps_counter++;
						if(TX_LQI==0)
							TX_LQI++;							// Recover telemetry right away
					}
				}
			}
			if (millis() - pps_timer >= 1000)
			{//1 packet every 20ms
				pps_timer = millis();
				debugln("%d pps", pps_counter);
				TX_LQI = pps_counter<<1;						// Max=100%
				pps_counter = 0;
			}
			if(TX_LQI==0)
				FrSkyX_telem_init();							// Reset telemetry
			else
				telemetry_link=1;								// Send telemetry out anyway
			phase=FRSKYR9_FREQ;
			break;
#endif
	}
	return 1000;
}

#endif