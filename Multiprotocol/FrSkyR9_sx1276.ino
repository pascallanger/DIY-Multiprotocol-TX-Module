#if defined(FRSKYR9_SX1276_INO)
#include "iface_sx1276.h"

#define FREQ_MAP_SIZE				29

// TODO the channel spacing is equal, consider calculating the new channel instead of using lookup tables (first_chan + index * step)

static uint32_t _freq_map_915[FREQ_MAP_SIZE] =
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

	// last two determined by _step
	0,
	0
};

static uint32_t _freq_map_868[FREQ_MAP_SIZE] =
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

	// last two determined by _step
	0,
	0
};

static uint8_t _step = 1;
static uint32_t* _freq_map = _freq_map_915;

uint16_t initFrSkyR9()
{
	set_rx_tx_addr(MProtocol_id_master);

	if(sub_protocol == 0) // 915MHz
		_freq_map = _freq_map_915;
	else if(sub_protocol == 1) // 868MHz
		_freq_map = _freq_map_868;

	_step = 1 + (random(0xfefefefe) % 24);
    _freq_map[27] = _freq_map[_step];
	_freq_map[28] = _freq_map[_step+1];

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

	// TODO this can probably be shorter
	return 20000; // start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
    static uint16_t freq_hop_index = 0;
	
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
	SX1276_SetFrequency(_freq_map[freq_hop_index]); // set current center frequency
	
	delayMicroseconds(500);

	uint8_t payload[26];

	payload[0] = 0x3C; // ????
	payload[1] = rx_tx_addr[3]; // unique radio id
	payload[2] = rx_tx_addr[2]; // unique radio id
	payload[3] = freq_hop_index; // current channel index
	payload[4] = _step; // step size and last 2 channels start index
	payload[5] = RX_num; // receiver number from OpenTX

	// binding mode: 0x00 regular / 0x41 bind?
	if(IS_BIND_IN_PROGRESS)
		payload[6] = 0x41; 
	else
		payload[6] = 0x00;

	// TODO
	payload[7] = 0x00; // fail safe related (looks like the same sequence of numbers as FrskyX protocol)

	// two channel are spread over 3 bytes.
	// each channel is 11 bit + 1 bit (msb) that states whether
	// it's part of the upper channels (9-16) or lower (1-8) (0 - lower 1 - upper)

	const int payload_offset = 8;
	const bool is_upper = false;

	int chan_index = 0;

	for(int i = 0; i < 8; i += 3)
	{
		// map channel values (0-2047) to (64-1984)
		uint16_t ch1 = 64 + (uint16_t)((1920.0f / 2047.0f) * Channel_data[chan_index]);
		uint16_t ch2 = 64 + (uint16_t)((1920.0f / 2047.0f) * Channel_data[chan_index + 1]);

		chan_index += 2;

		payload[payload_offset + i] = ch1;

		if(is_upper)
		{
			payload[payload_offset + i + 1] = ((ch1 >> 8) | 0b1000) | (ch2 << 4);
			payload[payload_offset + i + 2] = (ch2 >> 4) | 0b10000000;
		}
		else
		{
			payload[payload_offset + i + 1] = ((ch1 >> 8) & 0b0111) | (ch2 << 4);
			payload[payload_offset + i + 2] = (ch2 >> 4) & 0b01111111;
		}
	}
	
	payload[20] = 0x08; // ????
	payload[21] = 0x00; // ????
	payload[22] = 0x00; // ????
	payload[23] = 0x00; // ????

	uint16_t crc = FrSkyX_crc(payload, 24);

	payload[24] = crc; // low byte
	payload[25] = crc >> 8; // high byte

	SX1276_WritePayloadToFifo(payload, 26);

	freq_hop_index = (freq_hop_index + _step) % FREQ_MAP_SIZE;

	SX1276_SetMode(true, false, SX1276_OPMODE_TX);

	// need to clear RegIrqFlags?

    return 19400;
}

#endif