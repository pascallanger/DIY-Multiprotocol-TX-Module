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
// Radiolink surface protocol. TXs: RC4GS,RC6GS. Compatible RXs:R7FG(Std),R6FG,R6F,R8EF,R8FM,R8F,R4FGM 

#if defined(RLINK_CC2500_INO)

#include "iface_cc2500.h"

//#define RLINK_FORCE_ID

#define RLINK_TX_PACKET_LEN	33
#define RLINK_RX_PACKET_LEN	15
#define RLINK_TX_ID_LEN		4
#define RLINK_HOP			16

enum {
	RLINK_DATA	= 0x00,
	RLINK_RX1	= 0x01,
	RLINK_RX2	= 0x02,
};

uint32_t RLINK_rand1;
uint32_t RLINK_rand2;

static uint32_t __attribute__((unused)) RLINK_prng_next(uint32_t r)
{
	return 0xA5E2A705 * r + 0x754DB79B;
}

static void __attribute__((unused)) RLINK_init_random(uint32_t id)
{
	uint32_t result = id;

	RLINK_rand2 = result;
	for (uint8_t i=0; i<31; i++)
		result = RLINK_prng_next(result);
	RLINK_rand1 = result;
}

static uint8_t __attribute__((unused)) RLINK_next_random_swap()
{
	uint8_t result = (RLINK_rand2 >> 16) + RLINK_rand2 + (RLINK_rand1 >> 16) + RLINK_rand1;

	RLINK_rand2 = RLINK_prng_next(RLINK_rand2);
	RLINK_rand1 = RLINK_prng_next(RLINK_rand1);

	return result & 0x0F;
}

static uint32_t __attribute__((unused)) RLINK_compute_start_id(uint32_t id)
{
	return id * 0xF65EF9F9u + 0x2EDDF6CAu;
}

static void __attribute__((unused)) RLINK_shuffle_freqs(uint32_t seed)
{
	RLINK_init_random(seed);

	for(uint8_t i=0; i<RLINK_HOP; i++)
	{
		uint8_t r   = RLINK_next_random_swap();
		uint8_t tmp = hopping_frequency[r];
		hopping_frequency[r] = hopping_frequency[i];
		hopping_frequency[i] = tmp;
	}
}

static void __attribute__((unused)) RLINK_hop()
{
	uint8_t inc=3*(rx_tx_addr[0]&3);
	
	// init hop table
	for(uint8_t i=0; i<RLINK_HOP; i++)
		hopping_frequency[i] = (12*i) + inc;

	// shuffle
	RLINK_shuffle_freqs(RLINK_compute_start_id(rx_tx_addr[0] + (rx_tx_addr[1] << 8)));
	RLINK_shuffle_freqs(RLINK_compute_start_id(rx_tx_addr[2] + (rx_tx_addr[3] << 8)));

	// replace one of the channel randomely
	rf_ch_num=random(0xfefefefe)%0x11;		// 0x00..0x10
	if(inc==9) inc=6;						// frequency exception
	hopping_frequency[rf_ch_num]=12*16+inc;
}

static void __attribute__((unused)) RLINK_init()
{
	#ifdef RLINK_FORCE_ID
		//surface RC6GS
		memcpy(rx_tx_addr,"\x3A\x99\x22\x3A",RLINK_TX_ID_LEN);
		//air T8FB
		//memcpy(rx_tx_addr,"\xFC\x11\x0D\x20",RLINK_TX_ID_LEN);
	#endif
	// channels order depend on ID
	RLINK_hop();

 	#if 0
		debug("ID:");
		for(uint8_t i=0;i<RLINK_TX_ID_LEN;i++)
			debug(" 0x%02X",rx_tx_addr[i]);
		debugln("");
		debug("Hop(%d):", rf_ch_num);
		for(uint8_t i=0;i<RLINK_HOP;i++)
			debug(" 0x%02X",hopping_frequency[i]);
		debugln("");
	#endif
 }

const PROGMEM uint8_t RLINK_init_values[] = {
  /* 00 */ 0x5B, 0x06, 0x5C, 0x07, 0xAB, 0xCD, 0x40, 0x04,
  /* 08 */ 0x45, 0x00, 0x00, 0x06, 0x00, 0x5C, 0x62, 0x76,
  /* 10 */ 0x7A, 0x7F, 0x13, 0x23, 0xF8, 0x44, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11
};

static void __attribute__((unused)) RLINK_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&RLINK_init_values[i]));

	if(sub_protocol==RLINK_DUMBORC)
	{
		CC2500_WriteReg(4, 0xBA);
		CC2500_WriteReg(5, 0xDC);
	}

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
}

static void __attribute__((unused)) RLINK_tune_freq()
{
	if ( prev_option != option )
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		prev_option = option ;
	}
}

static void __attribute__((unused)) RLINK_send_packet()
{
	static uint32_t pseudo=0;
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 6;

	CC2500_Strobe(CC2500_SIDLE);

	// packet length
	packet[0] = RLINK_TX_PACKET_LEN;
	// header
	if(packet_count>3)
		packet[1] = 0x02;					// 0x02 telemetry request flag
	switch(sub_protocol)
	{
		case RLINK_SURFACE:
			packet[1] |= 0x01;
			//radiolink additionnal ID which is working only on a small set of RXs
			//if(RX_num) packet[1] |= ((RX_num+2)<<4)+4;	// RX number limited to 10 values, 0 is a wildcard
			break;
		case RLINK_AIR:
			packet[1] |= 0x21;					//air 0x21 on dump but it looks to support telemetry at least RSSI
			break;
		case RLINK_DUMBORC:
			packet[1]  = 0x00;					//always 0x00 on dump
			break;
	}
	
	// ID
	memcpy(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN);

	// pack 16 channels on 11 bits values between 170 and 1876, 1023 middle. The last 8 channels are failsafe values associated to the first 8 values.
	for (uint8_t i = 0; i < 16; i++)
	{
		uint32_t val = convert_channel_16b_nolimit(i,170,1876,false);		// allow extended limits
		if (val & 0x8000)
			val = 0;
		else if (val > 2047)
			val=2047;

		bits |= val << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8) {
			packet[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
	
	// hop
	pseudo=((pseudo * 0xAA) + 0x03) % 0x7673;	// calc next pseudo random value
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[pseudo & 0x0F]);
	packet[28]= pseudo;
	packet[29]= pseudo >> 8;
	packet[30]= 0x00;						// unknown
	packet[31]= 0x00;						// unknown
	packet[32]= rf_ch_num;					// index of value changed in the RF table
	
	// check
	uint8_t sum=0;
	for(uint8_t i=1;i<33;i++)
		sum+=packet[i];
	packet[33]=sum;

	// send packet
	CC2500_WriteData(packet, RLINK_TX_PACKET_LEN+1);
	
	// packets type
	packet_count++;
	if(packet_count>5) packet_count=0;

	//debugln("C= 0x%02X",hopping_frequency[pseudo & 0x0F]);
	//debug("P=");
	//for(uint8_t i=1;i<RLINK_TX_PACKET_LEN+1;i++)
	//	debug(" 0x%02X",packet[i]);
	//debugln("");
}

#define RLINK_TIMING_PROTO	20000-100		// -100 for compatibility with R8EF
#define RLINK_TIMING_RFSEND	10500
#define RLINK_TIMING_CHECK	2000
uint16_t RLINK_callback()
{
	switch(phase)
	{
		case RLINK_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(RLINK_TIMING_PROTO);
			#endif
			CC2500_SetPower();
			RLINK_tune_freq();
			RLINK_send_packet();
#if not defined RLINK_HUB_TELEMETRY
			return RLINK_TIMING_PROTO;
#else
			if(!(packet[1]&0x02))
				return RLINK_TIMING_PROTO;					//Normal packet
															//Telemetry packet
			phase++;										// RX1
			return RLINK_TIMING_RFSEND;
		case RLINK_RX1:
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFRX);
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SRX);
			phase++;										// RX2
			return RLINK_TIMING_PROTO-RLINK_TIMING_RFSEND-RLINK_TIMING_CHECK;
		case RLINK_RX2:
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len == RLINK_RX_PACKET_LEN + 1 + 2)			//Telemetry frame is 15 bytes + 1 byte for length + 2 bytes for RSSI&LQI&CRC
			{
				//debug("Telem:");
				CC2500_ReadData(packet_in, len);
				if(packet_in[0]==RLINK_RX_PACKET_LEN && (packet_in[len-1] & 0x80) && memcmp(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN)==0 && packet_in[6]==packet[1])
				{//Correct telemetry received: length, CRC, ID and type
					//Debug
					//for(uint8_t i=0;i<len;i++)
					//	debug(" %02X",packet_in[i]);
					TX_RSSI = packet_in[len-2];
					if(TX_RSSI >=128)
						TX_RSSI -= 128;
					else
						TX_RSSI += 128;
					RX_RSSI=packet_in[7];					//Should be packet_in[7]-256 but since it's an uint8_t...
					v_lipo1=packet_in[8]<<1;				//RX Batt
					v_lipo2=packet_in[9];					//Batt
					telemetry_link=1;						//Send telemetry out
					pps_counter++;
					packet_count=0;
				}
				//debugln("");
			}
			if (millis() - pps_timer >= 2000)
			{//1 telemetry packet every 100ms
				pps_timer = millis();
				if(pps_counter<20)
					pps_counter*=5;
				else
					pps_counter=100;
				debugln("%d pps", pps_counter);
				TX_LQI = pps_counter;						//0..100%
				pps_counter = 0;
			}
			CC2500_SetTxRxMode(TX_EN);
			phase=RLINK_DATA;								// DATA
			return RLINK_TIMING_CHECK;
#endif
	}
	return 0;
}

uint16_t initRLINK()
{
	BIND_DONE;	// Not a TX bind protocol
	RLINK_init();
	RLINK_rf_init();
	packet_count = 0;
	phase = RLINK_DATA;
	return 10000;
}

#endif