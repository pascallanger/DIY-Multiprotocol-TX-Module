#if defined(FRSKYR9_SX1276_INO)
#include "iface_sx1276.h"

#define FREQ_MAP_SIZE	29

uint8_t FrSkyR9_step = 1;
uint32_t FrSkyR9_freq_map[FREQ_MAP_SIZE];

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
    // Last two frequencies determined by FrSkyR9_step
	FrSkyR9_freq_map[FREQ_MAP_SIZE-2] = FrSkyR9_freq_map[FrSkyR9_step];
	debugln("F%d=%lu", FREQ_MAP_SIZE-2, FrSkyR9_freq_map[FREQ_MAP_SIZE-2]);
	FrSkyR9_freq_map[FREQ_MAP_SIZE-1] = FrSkyR9_freq_map[FrSkyR9_step+1];
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
	packet[4] = FrSkyR9_step;			// step size and last 2 channels start index

	//RX number
	packet[5] = RX_num;					// receiver number from OpenTX

	// Set packet[6]=failsafe, packet[7]=0?? and packet[8..19]=channels data
	FrSkyX_channels(6);

	//Bind
	if(IS_BIND_IN_PROGRESS)
	{
		if(sub_protocol & 1)
			packet[6] = 0x61;			// 868
		else
			packet[6] = 0x41;			// 915
	}

	//SPort
	packet[20] = 0x08;					//FrSkyX_TX_Seq=8 at startup
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

	//FrSkyR9_step
	FrSkyR9_step = 1 + (random(0xfefefefe) % 24);
	debugln("Step=%d", FrSkyR9_step);
	
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

	return 20000;						// start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
	static boolean bind=false;
	if(bind && IS_BIND_DONE)
		FrSkyR9_build_freq();			// 868 is binding at 915, need to switch back to 868 once bind is completed
	bind=IS_BIND_IN_PROGRESS;
	
	//Force standby
	SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);

	//SX1276_WriteReg(SX1276_11_IRQFLAGSMASK, 0xbf); // use only RxDone interrupt

	// uint8_t buffer[2];
	// buffer[0] = 0x00;
	// buffer[1] = 0x00;
	// SX1276_WriteRegisterMulti(SX1276_40_DIOMAPPING1, buffer, 2); // RxDone interrupt mapped to DIO0 (the rest are not used because of the REG_IRQ_FLAGS_MASK)

	// SX1276_WriteReg(REG_PAYLOAD_LENGTH, 13);

	// SX1276_WriteReg(REG_FIFO_ADDR_PTR, 0x00);

	// SX1276_WriteReg(SX1276_01_OPMODE, 0x85); // RXCONTINUOUS
	// delay(10); // 10 ms

	// SX1276_WriteReg(SX1276_01_OPMODE, 0x81); // STDBY

	// SX1276_WriteReg(SX1276_09_PACONFIG, 0xF0);

	//Set power
	// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
	// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
	SX1276_SetPaConfig(true, 7, 0);
	
	//Set frequency
	hopping_frequency_no = (hopping_frequency_no + FrSkyR9_step) % FREQ_MAP_SIZE;
	SX1276_SetFrequency(FrSkyR9_freq_map[hopping_frequency_no]); // set current center frequency
	delayMicroseconds(500);		//Frequency settle time

	//Build packet
	FrSkyR9_build_packet();
	
	//Send
	SX1276_WritePayloadToFifo(packet, 26);
	SX1276_SetMode(true, false, SX1276_OPMODE_TX);

	// need to clear RegIrqFlags?

    return 20000;
}

#endif