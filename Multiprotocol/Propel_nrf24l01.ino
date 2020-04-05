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
// Compatible with PROPEL 74-Z Speeder Bike.

#if defined(PROPEL_NRF24L01_INO)

#include "iface_nrf24l01.h"

//#define PROPEL_FORCE_ID

#define PROPEL_INITIAL_WAIT		500
#define PROPEL_PACKET_PERIOD	10000
#define PROPEL_BIND_RF_CHANNEL	0x23
#define PROPEL_PAYLOAD_SIZE		16
#define PROPEL_SEARCH_PERIOD	50	//*10ms
#define PROPEL_BIND_PERIOD		1500
#define PROPEL_PACKET_SIZE		14
#define PROPEL_RF_NUM_CHANNELS	4
#define PROPEL_ADDRESS_LENGTH	5
#define PROPEL_DEFAULT_PERIOD	20

enum {
	PROPEL_BIND1 = 0,
	PROPEL_BIND2,
	PROPEL_BIND3,
	PROPEL_DATA1,
};

static uint16_t __attribute__((unused)) PROPEL_checksum()
{
	typedef union {
		struct {
			uint8_t h:1;
			uint8_t g:1;
			uint8_t f:1;
			uint8_t e:1;
			uint8_t d:1;
			uint8_t c:1;
			uint8_t b:1;
			uint8_t a:1;
		} bits;
		uint8_t byte:8;
	} byte_bits_t;

    uint8_t sum = packet[0];
    for (uint8_t i = 1; i < PROPEL_PACKET_SIZE - 2; i++)
        sum += packet[i];

    byte_bits_t in  = { .byte = sum };
    byte_bits_t out = { .byte = sum };
	out.byte ^= 0x0a;
    out.bits.d = !(in.bits.d ^ in.bits.h);
    out.bits.c = (!in.bits.c && !in.bits.d &&  in.bits.g)
              ||  (in.bits.c && !in.bits.d && !in.bits.g)
              || (!in.bits.c &&  in.bits.g && !in.bits.h)
              ||  (in.bits.c && !in.bits.g && !in.bits.h)
              ||  (in.bits.c &&  in.bits.d &&  in.bits.g &&  in.bits.h)
              || (!in.bits.c &&  in.bits.d && !in.bits.g &&  in.bits.h);
    out.bits.b = (!in.bits.b && !in.bits.c && !in.bits.d)
              ||  (in.bits.b &&  in.bits.c &&  in.bits.g)
              || (!in.bits.b && !in.bits.c && !in.bits.g)
              || (!in.bits.b && !in.bits.d && !in.bits.g)
              || (!in.bits.b && !in.bits.c && !in.bits.h)
              || (!in.bits.b && !in.bits.g && !in.bits.h)
              ||  (in.bits.b &&  in.bits.c &&  in.bits.d &&  in.bits.h)
              ||  (in.bits.b &&  in.bits.d &&  in.bits.g &&  in.bits.h);
    out.bits.a =  (in.bits.a && !in.bits.b)
              ||  (in.bits.a && !in.bits.c && !in.bits.d)
              ||  (in.bits.a && !in.bits.c && !in.bits.g)
              ||  (in.bits.a && !in.bits.d && !in.bits.g)
              ||  (in.bits.a && !in.bits.c && !in.bits.h)
              ||  (in.bits.a && !in.bits.g && !in.bits.h)
              || (!in.bits.a &&  in.bits.b &&  in.bits.c &&  in.bits.g)
              || (!in.bits.a &&  in.bits.b &&  in.bits.c &&  in.bits.d &&  in.bits.h)
              || (!in.bits.a &&  in.bits.b &&  in.bits.d &&  in.bits.g &&  in.bits.h);

    return (sum << 8) | (out.byte & 0xff);
}

static void __attribute__((unused)) PROPEL_bind_packet(bool valid_rx_id)
{
	memset(packet, 0, PROPEL_PACKET_SIZE);

	packet[0] = 0xD0;
	memcpy(&packet[1], rx_tx_addr, 4);  // only 4 bytes sent of 5-byte address
	if (valid_rx_id) memcpy(&packet[5], rx_id, 4);
	packet[9] = rf_ch_num;				// hopping table to be used when switching to normal mode
	packet[11] = 0x05;					// unknown, 0x01 on TX2??

	uint16_t check = PROPEL_checksum();
	packet[12] = check >> 8;
	packet[13] = check & 0xff;

	NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_RX_DR) | _BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WritePayload(packet, PROPEL_PACKET_SIZE);
}

static void __attribute__((unused)) PROPEL_data_packet()
{
	memset(packet, 0, PROPEL_PACKET_SIZE);

	packet[0] = 0xC0;
	packet[1] = convert_channel_16b_limit(THROTTLE, 0x2f, 0xcf);
	packet[2] = convert_channel_16b_limit(RUDDER  , 0xcf, 0x2f);
	packet[3] = convert_channel_16b_limit(ELEVATOR, 0x2f, 0xcf);
	packet[4] = convert_channel_16b_limit(AILERON , 0xcf, 0x2f);
	packet[5] = 0x40;							//might be trims but unsused
	packet[6] = 0x40;							//might be trims but unsused
	packet[7] = 0x40;							//might be trims but unsused
	packet[8] = 0x40;							//might be trims but unsused
	if (bind_phase)
	{//need to send a couple of default packets after bind
		bind_phase--;
		packet[10] = 0x80;	// LEDs
	}
	else
	{
		packet[9] = 0x02    					// Always fast speed, slow=0x00, medium=0x01, fast=0x02, 0x03=flight training mode
					| GET_FLAG( CH14_SW, 0x03)	// Flight training mode
					| GET_FLAG( CH10_SW, 0x04)	// Calibrate
					| GET_FLAG( CH12_SW, 0x08)	// Take off
					| GET_FLAG( CH8_SW,  0x10)	// Fire
					| GET_FLAG( CH11_SW, 0x20)	// Altitude hold=0x20
					| GET_FLAG( CH6_SW,  0x40)	// Roll CW
					| GET_FLAG( CH7_SW,  0x80);	// Roll CCW
		packet[10] =  GET_FLAG( CH13_SW, 0x20)	// Land
					| GET_FLAG( CH9_SW,  0x40)	// Weapon system activted=0x40
					| GET_FLAG(!CH5_SW,  0x80);	// LEDs
	}
	packet[11] = 5;								// unknown, 0x01 on TX2??

	uint16_t check = PROPEL_checksum();
	packet[12] = check >> 8;
	packet[13] = check & 0xff;

	NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
	hopping_frequency_no &= 0x03;
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_RX_DR) | _BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, PROPEL_PACKET_SIZE);
}

static void __attribute__((unused)) PROPEL_init()
{
	NRF24L01_Initialize();
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x7f);
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3f);       // AA on all pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3f);   // Enable all pipes
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);    // 5-byte address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x36);  // retransmit 1ms, 6 times
	NRF24L01_SetBitrate(NRF24L01_BR_1M);              // 1Mbps
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x07);      // ?? match protocol capture
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *)"\x99\x77\x55\x33\x11", PROPEL_ADDRESS_LENGTH);	//Bind address
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (uint8_t *)"\x99\x77\x55\x33\x11", PROPEL_ADDRESS_LENGTH);	//Bind address
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, PROPEL_BIND_RF_CHANNEL);
	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);       // Enable dynamic payload length
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Enable all features
	// Beken 2425 register bank 1 initialized here in stock tx capture
	// Hopefully won't matter for nRF compatibility
	NRF24L01_FlushTx();
    NRF24L01_SetTxRxMode(TX_EN);
}

const uint8_t PROGMEM PROPEL_hopping []= { 0x47,0x36,0x27,0x44,0x33,0x0D,0x3C,0x2E,0x1B,0x39,0x2A,0x18 };
static void __attribute__((unused)) PROPEL_initialize_txid()
{
	//address last byte
	rx_tx_addr[4]=0x11;
	
	//random hopping channel table
	rf_ch_num=random(0xfefefefe)&0x03;
	for(uint8_t i=0; i<3; i++)
		hopping_frequency[i]=pgm_read_byte_near( &PROPEL_hopping[i + 3*rf_ch_num] );
	hopping_frequency[3]=0x23;

#ifdef PROPEL_FORCE_ID
	if(RX_num&1)
		memcpy(rx_tx_addr, (uint8_t *)"\x73\xd3\x31\x30\x11", PROPEL_ADDRESS_LENGTH);	//TX1: 73 d3 31 30 11
	else
		memcpy(rx_tx_addr, (uint8_t *)"\x94\xc5\x31\x30\x11", PROPEL_ADDRESS_LENGTH);	//TX2: 94 c5 31 30 11
	rf_ch_num = 0x03; //TX1
	memcpy(hopping_frequency,(uint8_t *)"\x39\x2A\x18\x23",PROPEL_RF_NUM_CHANNELS);		//TX1: 57,42,24,35
	rf_ch_num = 0x00; //TX2
	memcpy(hopping_frequency,(uint8_t *)"\x47\x36\x27\x23",PROPEL_RF_NUM_CHANNELS); 	//TX2: 71,54,39,35
	rf_ch_num = 0x01; // Manual search
	memcpy(hopping_frequency,(uint8_t *)"\x44\x33\x0D\x23",PROPEL_RF_NUM_CHANNELS); 	//Manual: 68,51,13,35
	rf_ch_num = 0x02; // Manual search
	memcpy(hopping_frequency,(uint8_t *)"\x3C\x2E\x1B\x23",PROPEL_RF_NUM_CHANNELS); 	//Manual: 60,46,27,35
#endif
}

uint16_t PROPEL_callback()
{
	uint8_t status;

	switch (phase)
	{
		case PROPEL_BIND1:
			PROPEL_bind_packet(false);		//rx_id unknown
			phase++;						//BIND2
			return PROPEL_BIND_PERIOD;

		case PROPEL_BIND2:
			status=NRF24L01_ReadReg(NRF24L01_07_STATUS);
			if (status & _BV(NRF24L01_07_MAX_RT))
			{// Max retry (6) reached
				phase = PROPEL_BIND1;
				return PROPEL_BIND_PERIOD;
			}
			if (!(_BV(NRF24L01_07_RX_DR) & status))
				return PROPEL_BIND_PERIOD;		// nothing received
			// received frame, got rx_id, save it
			NRF24L01_ReadPayload(packet_in, PROPEL_PACKET_SIZE);
			memcpy(rx_id, &packet_in[1], 4);
			PROPEL_bind_packet(true);		//send bind packet with rx_id
			phase++;						//BIND3
			break;

		case PROPEL_BIND3:
			if (_BV(NRF24L01_07_RX_DR) & NRF24L01_ReadReg(NRF24L01_07_STATUS))
			{
				NRF24L01_ReadPayload(packet_in, PROPEL_PACKET_SIZE);
				if (packet_in[0] == 0xa3 && memcmp(&packet_in[1],rx_id,4)==0)
				{//confirmation from the model
					phase++;				//PROPEL_DATA1
					bind_phase=PROPEL_DEFAULT_PERIOD;
					packet_count=0;
					BIND_DONE;
					break;
				}
			}
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, PROPEL_ADDRESS_LENGTH);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, PROPEL_ADDRESS_LENGTH);
			PROPEL_bind_packet(true);		//send bind packet with rx_id
			break;

		case PROPEL_DATA1:
			if (_BV(NRF24L01_07_RX_DR) & NRF24L01_ReadReg(NRF24L01_07_STATUS))
			{// data received from the model
				NRF24L01_ReadPayload(packet_in, PROPEL_PACKET_SIZE);
				if (packet_in[0] == 0xa3 && memcmp(&packet_in[1],rx_id,3)==0)
				{
					telemetry_counter++;	//LQI
					v_lipo1=packet[5];		//number of life left?
					v_lipo2=packet[4];		//bit mask: 0x80=flying, 0x08=taking off, 0x04=landing, 0x00=landed/crashed
					if(telemetry_lost==0)
						telemetry_link=1;
				}
			}
			PROPEL_data_packet();
			packet_count++;
			if(packet_count>=100)
			{//LQI calculation
				packet_count=0;
				TX_LQI=telemetry_counter;
				RX_RSSI=telemetry_counter;
				telemetry_counter = 0;
				telemetry_lost=0;
			}
			break;
	}
	return PROPEL_PACKET_PERIOD;
}

uint16_t initPROPEL()
{
	BIND_IN_PROGRESS;	// autobind protocol
	PROPEL_initialize_txid();
	PROPEL_init();
	hopping_frequency_no = 0;
	phase=PROPEL_BIND1;
	return PROPEL_INITIAL_WAIT;
}

#endif
// equations for checksum check byte from truth table
// (1)  z =  a && !b
//       ||  a && !c && !d
//       ||  a && !c && !g
//       ||  a && !d && !g
//       ||  a && !c && !h
//       ||  a && !g && !h
//       || !a &&  b &&  c &&  g
//       || !a &&  b &&  c &&  d &&  h
//       || !a &&  b &&  d &&  g &&  h;
//
// (2)  y = !b && !c && !d
//       ||  b &&  c &&  g
//       || !b && !c && !g
//       || !b && !d && !g
//       || !b && !c && !h
//       || !b && !g && !h
//       ||  b &&  c &&  d &&  h
//       ||  b &&  d &&  g &&  h;
//
// (3)  x = !c && !d &&  g
//       ||  c && !d && !g
//       || !c &&  g && !h
//       ||  c && !g && !h
//       ||  c &&  d &&  g &&  h
//       || !c &&  d && !g &&  h;
//
// (4)  w =  d &&  h
//       || !d && !h;
//
// (5)  v = !e;
//
// (6)  u =  f;
//
// (7)  t = !g;
//
// (8)  s =  h;