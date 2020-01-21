#if defined(FRSKYR9_SX1276_INO)

#define REG_IRQ_FLAGS_MASK			0x11

#define REG_PAYLOAD_LENGTH			0x22


#define REG_FIFO_ADDR_PTR			0x0D
#define REG_FIFO_TX_BASE_ADDR		0x0E

#define REG_OP_MODE 				0x01
#define REG_DETECT_OPTIMIZE 		0x31
#define REG_DIO_MAPPING1			0x40
#define REG_VERSION 				0x42


#define REG_MODEM_CONFIG1			0x1D
#define REG_MODEM_CONFIG2			0x1E
#define REG_MODEM_CONFIG3			0x26

#define REG_PREAMBLE_LSB			0x21
#define REG_DETECTION_THRESHOLD		0x37

#define REG_LNA						0x0C

#define REG_HOP_PERIOD				0x24

#define REG_PA_DAC					0x4D

#define REG_PA_CONFIG				0x09

#define REG_FRF_MSB					0x06

#define REG_FIFO					0x00

#define REG_OCP						0x0B

static uint32_t _freq_map[] =
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

static uint8_t _step = 1;

static uint16_t  __attribute__((unused)) FrSkyX_scaleForPXX_temp( uint8_t i )
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	uint16_t chan_val=convert_channel_frsky(i)-1226;
	if(i>7) chan_val|=2048;   // upper channels offset
	return chan_val;
}

uint16_t initFrSkyR9()
{
	set_rx_tx_addr(MProtocol_id_master);

	_step = 1 + (random(0xfefefefe) % 24);
    _freq_map[27] = _freq_map[_step];
	_freq_map[28] = _freq_map[_step+1];

	SX1276_WriteReg(REG_OP_MODE, 0x80); // sleep
	SX1276_WriteReg(REG_OP_MODE, 0x81); // standby

	uint8_t buffer[2];
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	SX1276_WriteRegisterMulti(REG_DIO_MAPPING1, buffer, 2);

	uint8_t val = SX1276_ReadReg(REG_DETECT_OPTIMIZE);
	val = (val & 0b11111000) | 0b00000101;
	SX1276_WriteReg(REG_DETECT_OPTIMIZE, val);

	// val = SX1276_ReadReg(REG_MODEM_CONFIG2);
	// val = (val & 0b00011111) | 0b11000000;
	// writeRegister(REG_MODEM_CONFIG2, val);

	SX1276_WriteReg(REG_MODEM_CONFIG1, 0x93);

	SX1276_WriteReg(REG_MODEM_CONFIG2, 0x60);

	val = SX1276_ReadReg(REG_MODEM_CONFIG3);
	val = (val & 0b11110011);
	SX1276_WriteReg(REG_MODEM_CONFIG3, val);

	SX1276_WriteReg(REG_PREAMBLE_LSB, 9);

	SX1276_WriteReg(REG_DETECTION_THRESHOLD, 0x0C);

	SX1276_WriteReg(REG_LNA, 0x23);

	SX1276_WriteReg(REG_HOP_PERIOD, 0x00);

	val = SX1276_ReadReg(REG_PA_DAC);
	val = (val & 0b11111000) | 0b00000111;
	SX1276_WriteReg(REG_PA_DAC, val);

	// TODO this can probably be shorter
	return 20000; // start calling FrSkyR9_callback in 20 milliseconds
}

uint16_t FrSkyR9_callback()
{
    static uint16_t index = 0;
	uint8_t buffer[3];

	SX1276_WriteReg(REG_OP_MODE, 0x81); // STDBY
	SX1276_WriteReg(REG_IRQ_FLAGS_MASK, 0xbf); // use only RxDone interrupt

	buffer[0] = 0x00;
	buffer[1] = 0x00;
	SX1276_WriteRegisterMulti(REG_DIO_MAPPING1, buffer, 2); // RxDone interrupt mapped to DIO0 (the rest are not used because of the REG_IRQ_FLAGS_MASK)

	// SX1276_WriteReg(REG_PAYLOAD_LENGTH, 13);

	// SX1276_WriteReg(REG_FIFO_ADDR_PTR, 0x00);

	// SX1276_WriteReg(REG_OP_MODE, 0x85); // RXCONTINUOUS
	// delay(10); // 10 ms

	// SX1276_WriteReg(REG_OP_MODE, 0x81); // STDBY

	SX1276_WriteReg(REG_PA_CONFIG, 0xF0);

	uint32_t freq = _freq_map[index] / 61;
	
	buffer[0] = (freq & (0xFF << 16)) >> 16;
	buffer[1] = (freq & (0xFF << 8)) >> 8;
	buffer[2] = freq & 0xFF;
	
	SX1276_WriteRegisterMulti(REG_FRF_MSB, buffer, 3); // set current center frequency
	
	delayMicroseconds(500);

	SX1276_WriteReg(REG_PAYLOAD_LENGTH, 26);
	SX1276_WriteReg(REG_FIFO_TX_BASE_ADDR, 0x00);
	SX1276_WriteReg(REG_FIFO_ADDR_PTR, 0x00);

	uint8_t payload[26];

	payload[0] = 0x3C; // ????
	payload[1] = rx_tx_addr[3]; // unique radio id
	payload[2] = rx_tx_addr[2]; // unique radio id
	payload[3] = index; // current channel index
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

	// write payload to fifo
	SX1276_WriteRegisterMulti(REG_FIFO, payload, 26);

	index = (index + _step) % 29;

	SX1276_WriteReg(REG_OP_MODE, 0x83); // TX

	// need to clear RegIrqFlags?

    return 19400;
}

#endif