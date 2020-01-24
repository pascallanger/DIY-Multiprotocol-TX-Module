#if defined(FRSKYR9_SX1276_INO)
#include "iface_sx1276.h"

#define FREQ_MAP_SIZE				29

// TODO the channel spacing is equal, consider calculating the new channel instead of using lookup tables (first_chan + index * step)

static uint32_t FrSkyR9_freq_map_915[FREQ_MAP_SIZE] =
{
	914472960,
	914972672,
	915472384,
	915972096,
	916471808,
	916971520,
	917471232,
	917970944,
	918470656,
	918970368,
	919470080,
	919969792,
	920469504,
	920969216,
	921468928,
	921968640,
	922468352,
	922968064,
	923467776,
	923967488,
	924467200,
	924966912,
	925466624,
	925966336,
	926466048,
	926965760,
	927465472,

	// last two determined by FrSkyR9_step
	0,
	0
};

static uint32_t FrSkyR9_freq_map_868[FREQ_MAP_SIZE] =
{
	859504640,
	860004352,
	860504064,
	861003776,
	861503488,
	862003200,
	862502912,
	863002624,
	863502336,
	864002048,
	864501760,
	865001472,
	865501184,
	866000896,
	866500608,
	867000320,
	867500032,
	867999744,
	868499456,
	868999168,
	869498880,
	869998592,
	870498304,
	870998016,
	871497728,
	871997440,
	872497152,

	// last two determined by FrSkyR9_step
	0,
	0
};

static uint8_t FrSkyR9_step = 1;
static uint32_t* FrSkyR9_freq_map = FrSkyR9_freq_map_915;

uint16_t initFrSkyR9()
{
	set_rx_tx_addr(MProtocol_id_master);

	if(sub_protocol & 0x01)
		FrSkyR9_freq_map = FrSkyR9_freq_map_868;
	else
		FrSkyR9_freq_map = FrSkyR9_freq_map_915;

	FrSkyR9_step = 1 + (random(0xfefefefe) % 24);
    FrSkyR9_freq_map[27] = FrSkyR9_freq_map[FrSkyR9_step];
	FrSkyR9_freq_map[28] = FrSkyR9_freq_map[FrSkyR9_step+1];

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
	SX1276_SetHopPeriod(0); // 0 = disabled, we hope frequencies manually
	SX1276_SetPaDac(true);

	hopping_frequency_no = 0;

	// TODO this can probably be shorter
	return 20000; // start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
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

	//SX1276_WriteReg(SX1276_09_PACONFIG, 0xF0);

	// max power: 15dBm (10.8 + 0.6 * MaxPower [dBm])
	// output_power: 2 dBm (17-(15-OutputPower) (if pa_boost_pin == true))
	SX1276_SetPaConfig(true, 7, 0);
	SX1276_SetFrequency(FrSkyR9_freq_map[hopping_frequency_no]); // set current center frequency
	
	delayMicroseconds(500);

	packet[0] = 0x3C; // ????
	packet[1] = rx_tx_addr[3]; // unique radio id
	packet[2] = rx_tx_addr[2]; // unique radio id
	packet[3] = hopping_frequency_no; // current channel index
	packet[4] = FrSkyR9_step; // step size and last 2 channels start index
	packet[5] = RX_num; // receiver number from OpenTX

	// binding mode: 0x00 regular / 0x41 bind?
	if(IS_BIND_IN_PROGRESS)
		packet[6] = 0x41; 
	else
		packet[6] = 0x00;

	// TODO
	packet[7] = 0x00; // fail safe related (looks like the same sequence of numbers as FrskyX protocol)

	// two channel are spread over 3 bytes.
	// each channel is 11 bit + 1 bit (msb) that states whether
	// it's part of the upper channels (9-16) or lower (1-8) (0 - lower 1 - upper)

	#define CH_POS 8
	static uint8_t chan_start=0;
	uint8_t chan_index = chan_start;

	for(int i = 0; i < 12; i += 3)
	{
		// map channel values (0-2047) to (64-1984)
		uint16_t ch1 = FrSkyX_scaleForPXX(chan_index);
		uint16_t ch2 = FrSkyX_scaleForPXX(chan_index + 1);

		packet[CH_POS + i] = ch1;
		packet[CH_POS + i + 1] = (ch1 >> 8) | (ch2 << 4);
		packet[CH_POS + i + 2] = (ch2 >> 4);

		chan_index += 2;
	}
	
	if((sub_protocol & 0x02) == 0)
		chan_start ^= 0x08;		// Alternate between lower and upper when 16 channels is used
	
	packet[20] = 0x08; // ????
	packet[21] = 0x00; // ????
	packet[22] = 0x00; // ????
	packet[23] = 0x00; // ????

	uint16_t crc = FrSkyX_crc(packet, 24);

	packet[24] = crc; // low byte
	packet[25] = crc >> 8; // high byte

	SX1276_WritePayloadToFifo(packet, 26);

	hopping_frequency_no = (hopping_frequency_no + FrSkyR9_step) % FREQ_MAP_SIZE;

	SX1276_SetMode(true, false, SX1276_OPMODE_TX);

	// need to clear RegIrqFlags?

    return 19400;
}

#endif