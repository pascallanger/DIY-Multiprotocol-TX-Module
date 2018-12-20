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
// compatible with MJX Bugs 3 Mini and Bugs 3H

#if defined(BUGSMINI_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define BUGSMINI_INITIAL_WAIT    500
#define BUGSMINI_PACKET_INTERVAL 6840
#define BUGSMINI_WRITE_WAIT      2000
#define BUGSMINI_TX_PAYLOAD_SIZE 24
#define BUGSMINI_RX_PAYLOAD_SIZE 16
#define BUGSMINI_NUM_RF_CHANNELS 15
#define BUGSMINI_ADDRESS_SIZE    5

static uint8_t BUGSMINI_txid[3];
static uint8_t BUGSMINI_txhash;

enum {
    BUGSMINI_BIND1,
    BUGSMINI_BIND2,
    BUGSMINI_DATA1,
    BUGSMINI_DATA2
};

#define BUGSMINI_CH_SW_ARM		CH5_SW
#define BUGSMINI_CH_SW_ANGLE	CH6_SW
#define BUGSMINI_CH_SW_FLIP		CH7_SW
#define BUGSMINI_CH_SW_PICTURE	CH8_SW
#define BUGSMINI_CH_SW_VIDEO	CH9_SW
#define BUGSMINI_CH_SW_LED		CH10_SW

// flags packet[12]
#define BUGSMINI_FLAG_FLIP    0x08    // automatic flip
#define BUGSMINI_FLAG_MODE    0x04    // low/high speed select (set is high speed)
#define BUGSMINI_FLAG_VIDEO   0x02    // toggle video
#define BUGSMINI_FLAG_PICTURE 0x01    // toggle picture

// flags packet[13]
#define BUGSMINI_FLAG_LED     0x80    // enable LEDs
#define BUGSMINI_FLAG_ARM     0x40    // arm (toggle to turn on motors)
#define BUGSMINI_FLAG_DISARM  0x20    // disarm (toggle to turn off motors)
#define BUGSMINI_FLAG_ANGLE   0x02    // angle/acro mode (set is angle mode)

static void __attribute__((unused)) BUGSMINI_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, BUGSMINI_RX_PAYLOAD_SIZE); // bytes of data payload for rx pipe 1
	NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x07);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);
	NRF24L01_SetPower();
	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);     // Set feature bits on
}

static void __attribute__((unused)) BUGSMINI_check_arming()
{
	uint8_t arm_channel = BUGSMINI_CH_SW_ARM;

	if (arm_channel != arm_channel_previous)
	{
		arm_channel_previous = arm_channel;
		if (arm_channel)
		{
			armed = 1;
			arm_flags ^= BUGSMINI_FLAG_ARM;
		}
		else
		{
			armed = 0;
			arm_flags ^= BUGSMINI_FLAG_DISARM;
		}
	}
}

static void __attribute__((unused)) BUGSMINI_send_packet(uint8_t bind)
{
	BUGSMINI_check_arming();  // sets globals arm_flags and armed

	uint16_t aileron  = convert_channel_16b_limit(AILERON,500,0);
	uint16_t elevator = convert_channel_16b_limit(ELEVATOR,0,500);
	uint16_t throttle = armed ? convert_channel_16b_limit(THROTTLE,0,500) : 0;
	uint16_t rudder   = convert_channel_16b_limit(RUDDER,500,0);

	packet[1] = BUGSMINI_txid[0];
	packet[2] = BUGSMINI_txid[1];
	packet[3] = BUGSMINI_txid[2];
	if(bind)
	{
		packet[4] = 0x00;
		packet[5] = 0x7d;
		packet[6] = 0x7d;
		packet[7] = 0x7d;
		packet[8] = 0x20;
		packet[9] = 0x20;
		packet[10]= 0x20;
		packet[11]= 0x40;
		packet[12]^= 0x40;	 // alternating freq hopping flag
		packet[13]= 0x60;
		packet[14]= 0x00;
		packet[15]= 0x00;
	}
	else
	{
		packet[4] = throttle >> 1;
		packet[5] = rudder >> 1;
		packet[6] = elevator >> 1;
		packet[7] = aileron >> 1;
		packet[8] = 0x20 | (aileron << 7);
		packet[9] = 0x20 | (elevator << 7);
		packet[10]= 0x20 | (rudder << 7);
		packet[11]= 0x40 | (throttle << 7);
		packet[12]= 0x80 | (packet[12] ^ 0x40) // bugs 3 H doesn't have 0x80 ?
			| BUGSMINI_FLAG_MODE
			| GET_FLAG(BUGSMINI_CH_SW_PICTURE, BUGSMINI_FLAG_PICTURE)
			| GET_FLAG(BUGSMINI_CH_SW_VIDEO, BUGSMINI_FLAG_VIDEO);
		if(armed)
			packet[12] |= GET_FLAG(BUGSMINI_CH_SW_FLIP, BUGSMINI_FLAG_FLIP);
		packet[13] = arm_flags
			| GET_FLAG(BUGSMINI_CH_SW_LED, BUGSMINI_FLAG_LED)
			| GET_FLAG(BUGSMINI_CH_SW_ANGLE, BUGSMINI_FLAG_ANGLE);

		packet[14] = 0;
		packet[15] = 0; // 0x53 on bugs 3 H ?
	}
	uint8_t checksum = 0x6d;
	for(uint8_t i=1; i < BUGSMINI_TX_PAYLOAD_SIZE; i++)
	checksum ^= packet[i];
	packet[0] = checksum;

	if(!(packet[12]&0x40))
	{
		hopping_frequency_no++;
		if(hopping_frequency_no >= BUGSMINI_NUM_RF_CHANNELS)
			hopping_frequency_no = 0;
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? hopping_frequency[hopping_frequency_no+BUGSMINI_NUM_RF_CHANNELS] : hopping_frequency[hopping_frequency_no]);
	}

	// Power on, TX mode, 2byte CRC
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, BUGSMINI_TX_PAYLOAD_SIZE);
	NRF24L01_SetPower();
}

// compute final address for the rxid received during bind
// thanks to Pascal for the function!
const uint8_t PROGMEM BUGSMINI_end []= {
	0x2d,0x9e ,0x95,0xa4 ,0x9c,0x5c ,0xb4,0xa6 ,0xa9,0xce ,0x56,0x2b ,0x3e,0x73 ,0xb8,0x95 ,0x6a,0x82,
	0x94,0x37 ,0x3d,0x5a ,0x4b,0xb2 ,0x69,0x49 ,0xc2,0x24 ,0x6b,0x3d ,0x23,0xc6 ,0x9e,0xa3 ,0xa4,0x98,
	0x5c,0x9e ,0xa6,0x52 ,0xce,0x76 ,0x2b,0x4b ,0x73,0x3a };
static void __attribute__((unused)) BUGSMINI_make_address()
{
    uint8_t start, length, index;

	//read rxid
	uint8_t base_adr=BUGSMINI_EEPROM_OFFSET+RX_num*2;
    uint8_t rxid_high = eeprom_read_byte((EE_ADDR)(base_adr+0));
    uint8_t rxid_low  = eeprom_read_byte((EE_ADDR)(base_adr+1));
    
    if(rxid_high==0x00 || rxid_high==0xFF)
        rx_tx_addr[0]=0x52;
    else
        rx_tx_addr[0]=rxid_high;
    
    rx_tx_addr[1]=BUGSMINI_txhash;
    
    if(rxid_low==0x00 || rxid_low==0xFF)
        rx_tx_addr[2]=0x66;
    else
        rx_tx_addr[2]=rxid_low;
    
    for(uint8_t end_idx=0;end_idx<23;end_idx++)
    {
        //calculate sequence start
        if(end_idx<=7)
            start=end_idx;
        else
            start=(end_idx-7)*16+7;
        //calculate sequence length
        if(end_idx>6)
        {
            if(end_idx>15)
                length=(23-end_idx)<<1;
            else
                length=16;
        }
        else
            length=(end_idx+1)<<1;
        //calculate first index
        index=start-rxid_high;
        //scan for a possible match using the current end
        for(uint8_t i=0;i<length;i++)
        {
            if(index==rxid_low)
            { //match found
                rx_tx_addr[3]=pgm_read_byte_near( &BUGSMINI_end[end_idx<<1] );
                rx_tx_addr[4]=pgm_read_byte_near( &BUGSMINI_end[(end_idx<<1)+1] );
                return;
            }
            index+=i&1?7:8;	//increment index
        }
    }
    // Something wrong happened if we arrive here....
}

static void __attribute__((unused)) BUGSMINI_update_telemetry()
{
#if defined(BUGS_HUB_TELEMETRY)
	uint8_t checksum = 0x6d;
	for(uint8_t i=1; i<12; i++)
		checksum += packet[i];
	if(packet[0] == checksum)
	{
		RX_RSSI = packet[3];
		if(packet[11] & 0x80)
			v_lipo1 = 0xff; // Ok
		else if(packet[11] & 0x40)
			v_lipo1 = 0x80; // Warning
		else
			v_lipo1 = 0x00; // Critical
	}
#endif
}

uint16_t BUGSMINI_callback()
{
	uint8_t base_adr;
	switch(phase)
	{
		case BUGSMINI_BIND1:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				XN297_ReadPayload(packet, BUGSMINI_RX_PAYLOAD_SIZE);
				base_adr=BUGSMINI_EEPROM_OFFSET+RX_num*2;
				eeprom_write_byte((EE_ADDR)(base_adr+0),packet[1]);	// Save rxid in EEPROM
				eeprom_write_byte((EE_ADDR)(base_adr+1),packet[2]);	// Save rxid in EEPROM
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(TX_EN);
				BUGSMINI_make_address();
				XN297_SetTXAddr(rx_tx_addr, 5);
				XN297_SetRXAddr(rx_tx_addr, 5);
				phase = BUGSMINI_DATA1;
				BIND_DONE;
				return BUGSMINI_PACKET_INTERVAL;
			}
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(TX_EN);
			BUGSMINI_send_packet(1);
			phase = BUGSMINI_BIND2;
			return BUGSMINI_WRITE_WAIT;
		case BUGSMINI_BIND2:
			// switch to RX mode
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();
			XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) 
						  | _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
			phase = BUGSMINI_BIND1;
			return BUGSMINI_PACKET_INTERVAL - BUGSMINI_WRITE_WAIT;
		case BUGSMINI_DATA1:
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready => read only 12 bytes to not overwrite channel change flag
				XN297_ReadPayload(packet, 12);
				BUGSMINI_update_telemetry();
			}
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(TX_EN);
			BUGSMINI_send_packet(0);
			phase = BUGSMINI_DATA2;
			return BUGSMINI_WRITE_WAIT;
		case BUGSMINI_DATA2:
			// switch to RX mode
			NRF24L01_SetTxRxMode(TXRX_OFF);
			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();
			XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) 
							| _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX));
			phase = BUGSMINI_DATA1;
			return BUGSMINI_PACKET_INTERVAL - BUGSMINI_WRITE_WAIT;
	}
	return BUGSMINI_PACKET_INTERVAL;
}

#define BUGSMINI_NUM_TX_RF_MAPS 4
// haven't figured out BUGSMINI_txid<-->rf channel mapping yet
const uint8_t PROGMEM BUGSMINI_RF_chans[BUGSMINI_NUM_TX_RF_MAPS][BUGSMINI_NUM_RF_CHANNELS] = {
	{0x22,0x2f,0x3a,0x14,0x20,0x2d,0x38,0x18,0x26,0x32,0x11,0x1d,0x29,0x35,0x17},
	{0x3d,0x34,0x2b,0x22,0x19,0x40,0x37,0x2e,0x25,0x1c,0x3a,0x31,0x28,0x1f,0x16},
	{0x12,0x20,0x2f,0x1a,0x28,0x38,0x14,0x23,0x32,0x1c,0x2c,0x3b,0x17,0x26,0x34},
	{0x13,0x25,0x37,0x1F,0x31,0x17,0x28,0x3A,0x1C,0x2E,0x22,0x33,0x19,0x2B,0x3D} };
const uint8_t PROGMEM BUGSMINI_bind_chans[BUGSMINI_NUM_RF_CHANNELS] = {
	0x1A,0x23,0x2C,0x35,0x3E,0x17,0x20,0x29,0x32,0x3B,0x14,0x1D,0x26,0x2F,0x38}; // bugs 3 mini bind channels
const uint8_t PROGMEM BUGSMINI_tx_id[BUGSMINI_NUM_TX_RF_MAPS][3] = {
	{0xA8,0xE6,0x32},
	{0xdd,0xab,0xfd},
	{0x90,0x9e,0x4a},
	{0x20,0x28,0xBA} };
const uint8_t PROGMEM BUGSMINI_tx_hash[BUGSMINI_NUM_TX_RF_MAPS] = { // 2nd byte of final address
	0x6c,0x9e,0x3d,0xb3};
	
static void __attribute__((unused)) BUGSMINI_initialize_txid()
{
	// load hopping_frequency with tx channels in low part and bind channels in high part
	for(uint8_t i=0; i<BUGSMINI_NUM_RF_CHANNELS;i++)
	{
		hopping_frequency[i]=pgm_read_byte_near( &BUGSMINI_RF_chans[rx_tx_addr[3]%BUGSMINI_NUM_TX_RF_MAPS][i] );
		hopping_frequency[i+BUGSMINI_NUM_RF_CHANNELS]=pgm_read_byte_near( &BUGSMINI_bind_chans[i] );
	}
	// load txid
	for(uint8_t i=0; i<sizeof(BUGSMINI_txid);i++)
		BUGSMINI_txid[i]=pgm_read_byte_near( &BUGSMINI_tx_id[rx_tx_addr[3]%BUGSMINI_NUM_TX_RF_MAPS][i] );
	//load tx_hash
	BUGSMINI_txhash = pgm_read_byte_near( &BUGSMINI_tx_hash[rx_tx_addr[3]%BUGSMINI_NUM_TX_RF_MAPS] );
}

uint16_t initBUGSMINI()
{
	BUGSMINI_initialize_txid();
	memset(packet, (uint8_t)0, BUGSMINI_TX_PAYLOAD_SIZE);
	BUGSMINI_init();
	if(IS_BIND_IN_PROGRESS)
	{
		XN297_SetTXAddr((const uint8_t*)"mjxRC", 5);
		XN297_SetRXAddr((const uint8_t*)"mjxRC", 5);
		phase = BUGSMINI_BIND1;
	}
	else
	{
		BUGSMINI_make_address();
		XN297_SetTXAddr(rx_tx_addr, 5);
		XN297_SetRXAddr(rx_tx_addr, 5);
		phase = BUGSMINI_DATA1;
	}
	armed = 0;
	arm_flags = BUGSMINI_FLAG_DISARM;    // initial value from captures
	arm_channel_previous = BUGSMINI_CH_SW_ARM;
	#ifdef BUGS_HUB_TELEMETRY
		init_frskyd_link_telemetry();
	#endif
	return BUGSMINI_INITIAL_WAIT;
}

#endif
