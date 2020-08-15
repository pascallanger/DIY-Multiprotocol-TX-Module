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

const PROGMEM uint8_t RLINK_hopping[][8] = {
  /* 4C494E4B */ { 0xBC, 0x5A, 0x70, 0x4E, 0xDF, 0x32, 0x16, 0x89 },
  /* 4D494E4B */ { 0x4C, 0xF3, 0xEA, 0x5B, 0x62, 0x9D, 0x01, 0x87 },
  /* 4E494E4B */ { 0x86, 0xEA, 0xD0, 0xC9, 0x2B, 0x53, 0x7F, 0x41 },
  /* 4F494E4B */ { 0xAC, 0x91, 0x7D, 0x48, 0xE0, 0xB5, 0x32, 0xF6 },
  /* 50494E4B */ { 0xD6, 0x7C, 0xA4, 0x93, 0x5F, 0xE1, 0x02, 0xB8 },
  /* 51494E4B */ { 0xED, 0x04, 0x73, 0xC8, 0x56, 0xB9, 0x1F, 0xA2 },
  /* 52494E4B */ { 0xA7, 0xF0, 0x36, 0xB2, 0x95, 0x4E, 0x1C, 0xD8 },
  /* 53494E4B */ { 0x76, 0x8B, 0xA0, 0x3E, 0x51, 0x4C, 0x9D, 0x2F },
  /* 54494E4B */ { 0x07, 0x23, 0x16, 0xFD, 0xC9, 0x5B, 0x84, 0xAE },
  /* 55494E4B */ { 0xD3, 0xA0, 0x69, 0xBF, 0x12, 0x8C, 0x4E, 0x57 },
  /* 56494E4B */ { 0xA6, 0xBE, 0x91, 0xD3, 0x7C, 0x4F, 0x82, 0x50 },
  /* 57494E4B */ { 0x91, 0xDA, 0xBC, 0x75, 0x82, 0x36, 0x4E, 0xF0 },
  /* 58494E4B */ { 0x9A, 0x27, 0x5C, 0xF4, 0xD8, 0xB0, 0x36, 0xE1 },
  /* 59494E4B */ { 0x92, 0xF1, 0x34, 0xA7, 0x5B, 0x0C, 0xED, 0x86 },
  /* 5A494E4B */ { 0x8C, 0x2B, 0x51, 0xF9, 0x3E, 0x4A, 0x67, 0xD0 },
  /* 5B494E4B */ { 0x5E, 0x3D, 0x67, 0x9B, 0xA2, 0x84, 0xFC, 0x01 },
  /* 5C494E4B */ { 0xF9, 0x35, 0xBD, 0x78, 0x26, 0x1C, 0x0E, 0xA4 },
  /* 5D494E4B */ { 0xD9, 0x7B, 0x48, 0x0E, 0x2A, 0xCF, 0x13, 0x65 },
  /* 5E494E4B */ { 0x07, 0xE4, 0xF9, 0x8A, 0x3C, 0x21, 0xB5, 0xD6 },
  /* 5F494E4B */ { 0xEB, 0xFA, 0x29, 0xD1, 0x54, 0x3C, 0x07, 0x86 },
  /* 60494E4B */ { 0xDF, 0xCE, 0x0A, 0x32, 0x71, 0x5B, 0x96, 0x48 },
  /* 61494E4B */ { 0x19, 0x86, 0xF5, 0x3A, 0x27, 0xDC, 0x0E, 0xB4 },
  /* 62494E4B */ { 0xF8, 0x47, 0x9C, 0xE0, 0x2D, 0xBA, 0x15, 0x36 },
  /* 63494E4B */ { 0xED, 0x78, 0x01, 0xA3, 0x2B, 0x6C, 0x45, 0xF9 },
  /* 64494E4B */ { 0xE0, 0xA2, 0xD4, 0x6B, 0xF5, 0x18, 0x3C, 0x79 },
  /* 65494E4B */ { 0x26, 0x90, 0x8B, 0x5D, 0x31, 0xCF, 0xE7, 0x4A },
  /* 66494E4B */ { 0x7B, 0x12, 0xA8, 0x4F, 0xC0, 0x65, 0xD9, 0x3E },
  /* 67494E4B */ { 0x35, 0xA2, 0x14, 0xBE, 0x06, 0x7D, 0x98, 0xFC },
  /* 68494E4B */ { 0xD2, 0xA9, 0x7E, 0x40, 0x6F, 0x83, 0x5C, 0xB1 },
  /* 69494E4B */ { 0xE5, 0xB9, 0xC1, 0x04, 0x83, 0x27, 0xA6, 0xFD },
  /* 6A494E4B */ { 0x8E, 0x0C, 0x4A, 0x51, 0xFB, 0x63, 0x92, 0x7D },
  /* 6B494E4B */ { 0xC7, 0x1D, 0x02, 0x93, 0x45, 0xF8, 0xA6, 0xBE },
  /* 6C494E4B */ { 0xC1, 0x64, 0x59, 0x0A, 0x7D, 0x3F, 0x28, 0xEB },
  /* 6D494E4B */ { 0xEF, 0x75, 0xAB, 0x94, 0xD2, 0x83, 0x1C, 0x60 },
  /* 6E494E4B */ { 0xA1, 0x20, 0x54, 0x68, 0x9E, 0x7B, 0x3D, 0xFC },
  /* 6F494E4B */ { 0x3E, 0x60, 0x28, 0xFC, 0xDA, 0x45, 0x9B, 0x71 },
  /* 70494E4B */ { 0xB7, 0x0E, 0xA8, 0x23, 0xFC, 0x65, 0x4D, 0x91 },
  /* 71494E4B */ { 0x29, 0x34, 0x51, 0x7C, 0xB8, 0xFD, 0x0E, 0x6A },
  /* 72494E4B */ { 0x1B, 0x06, 0x3C, 0x89, 0xF5, 0x2A, 0x7E, 0xD4 },
  /* 73494E4B */ { 0xF2, 0xC9, 0x63, 0x57, 0x0A, 0xB1, 0x48, 0xDE },
  /* 74494E4B */ { 0x24, 0xAE, 0x0C, 0x19, 0x53, 0x7B, 0xF6, 0x8D },
  /* 75494E4B */ { 0xEC, 0xD8, 0xF2, 0x4B, 0xA3, 0x51, 0x09, 0x76 },
  /* 76494E4B */ { 0x98, 0x71, 0x5E, 0xAD, 0xFC, 0x04, 0x3B, 0x62 },
  /* 77494E4B */ { 0xAE, 0xF6, 0xB7, 0x01, 0x52, 0x34, 0x9D, 0x8C },
  /* 78494E4B */ { 0x69, 0x48, 0xF1, 0x3C, 0xDB, 0x0E, 0x25, 0xA7 },
  /* 79494E4B */ { 0xCF, 0x60, 0x94, 0xAD, 0xB1, 0x82, 0x73, 0xE5 },
  /* 7A494E4B */ { 0xFA, 0xC1, 0xD7, 0xB0, 0x53, 0x92, 0x6E, 0x48 },
  /* 7B494E4B */ { 0xAC, 0x7D, 0xE5, 0x8B, 0x41, 0x96, 0x2F, 0x30 },
  /* 7C494E4B */ { 0xFD, 0xC1, 0xB9, 0x02, 0xE4, 0x87, 0x56, 0x3A },
  /* 7D494E4B */ { 0x30, 0xDA, 0x4F, 0x8E, 0x5C, 0xB9, 0x26, 0x71 },
  /* 7E494E4B */ { 0xDC, 0xF9, 0x57, 0x30, 0x82, 0x1E, 0x6A, 0x4B },
  /* 7F494E4B */ { 0x84, 0x1D, 0x7E, 0x29, 0x3C, 0x65, 0xA0, 0xBF },
  /* 80494E4B */ { 0x01, 0xA3, 0xF6, 0xE2, 0x4C, 0x8B, 0x5D, 0x79 },
  /* 81494E4B */ { 0xA1, 0x32, 0xE7, 0x08, 0x4D, 0x5B, 0x9F, 0x6C },
  /* 82494E4B */ { 0x31, 0x40, 0x67, 0x8F, 0xBA, 0x95, 0x2C, 0xED },
  /* 83494E4B */ { 0x91, 0x76, 0xFA, 0x83, 0x20, 0x4B, 0xEC, 0x5D },
  /* 84494E4B */ { 0xDB, 0x54, 0xC2, 0x61, 0xF0, 0xA9, 0x87, 0x3E },
  /* 85494E4B */ { 0xC0, 0xB4, 0x61, 0xD3, 0x7A, 0x5F, 0x82, 0x9E },
  /* 86494E4B */ { 0xD6, 0xCF, 0x9B, 0x75, 0xE1, 0x42, 0x3A, 0x80 },
  /* 87494E4B */ { 0xFE, 0xA2, 0xB4, 0x9C, 0x10, 0x7D, 0x56, 0x83 },
  /* 88494E4B */ { 0xD2, 0x79, 0x54, 0xEF, 0xC8, 0x0B, 0x36, 0xA1 },
  /* 89494E4B */ { 0x8D, 0xCF, 0x23, 0x64, 0xE5, 0x0B, 0x1A, 0x97 },
  /* 8A494E4B */ { 0x07, 0xC4, 0xEF, 0x9A, 0x61, 0xD8, 0xB3, 0x52 },
  /* 8B494E4B */ { 0x45, 0x6E, 0xBF, 0x8C, 0x9A, 0x2D, 0x31, 0x70 },
#ifdef RLINK_FORCE_ID
  /* 3A99223A */ { 0x1F, 0x89, 0x25, 0x06, 0x4E, 0xBD, 0x3A, 0xC7 },
  /* FC110D20 */ { 0xBC, 0xFE, 0x59, 0x84, 0x37, 0xA1, 0xD0, 0x62 }
 #endif
};

static void __attribute__((unused)) RLINK_load_hopp(uint8_t num)
{
	uint8_t inc=3*(rx_tx_addr[0]&3);
	
	for (uint8_t i = 0; i < RLINK_HOP>>1; i++)
	{
		uint8_t val=pgm_read_byte_near(&RLINK_hopping[num][i]);
		hopping_frequency[ i<<1   ]=12*(val>>4  )+inc;
		hopping_frequency[(i<<1)+1]=12*(val&0x0F)+inc;
	}

	// replace one of the channel randomely
	rf_ch_num=random(0xfefefefe)%0x11;		// 0x00..0x10
	if(inc==9) inc=6;						// frequency exception
	hopping_frequency[rf_ch_num]=12*16+inc;
}

static void __attribute__((unused)) RLINK_init()
{
	// channels order depend on ID and currently unknown so using a table of 64 entries...
	uint8_t id=rx_tx_addr[3]&0x3F;
	memcpy(rx_tx_addr,"\x4C\x49\x4E\x4B",RLINK_TX_ID_LEN);
	rx_tx_addr[0] += id;
	RLINK_load_hopp(id);
	
	#ifdef RLINK_FORCE_ID
		//surface RC6GS
		memcpy(rx_tx_addr,"\x3A\x99\x22\x3A",RLINK_TX_ID_LEN);
		RLINK_load_hopp(64);
		//air T8FB
		//memcpy(rx_tx_addr,"\xFC\x11\x0D\x20",RLINK_TX_ID_LEN);
		//RLINK_load_hopp(65);
	#endif

/* 	debug("ID:");
	for(uint8_t i=0;i<RLINK_TX_ID_LEN;i++)
		debug(" 0x%02X",rx_tx_addr[i]);
	debugln("");
	debug("Hop(%d):", rf_ch_num);
	for(uint8_t i=0;i<RLINK_HOP;i++)
		debug(" 0x%02X",hopping_frequency[i]);
	debugln("");
 */
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
	if(sub_protocol)
		packet[1] = 0x21;					//air 0x21 on dump but it looks to support telemetry at least RSSI
	else
	{//surface
		packet[1] = 0x01;
		//radiolink additionnal ID which is working only on a small set of RXs
		//if(RX_num) packet[1] |= ((RX_num+2)<<4)+4;	// RX number limited to 10 values, 0 is a wildcard
	}
	if(packet_count>3)
		packet[1] |= 0x02;					// 0x02 telemetry request flag
	
	// ID
	memcpy(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN);

	// pack 16 channels on 11 bits values between 170 and 1876, 1023 middle. The last 8 channels are failsafe values associated to the first 8 values.
	for (uint8_t i = 0; i < 16; i++)
	{
		uint32_t val = convert_channel_16b_nolimit(i,170,1876);		// allow extended limits
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