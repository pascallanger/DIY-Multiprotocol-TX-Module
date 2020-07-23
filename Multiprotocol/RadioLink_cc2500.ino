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

enum {
	RLINK_DATA	= 0x00,
	RLINK_RX1	= 0x01,
	RLINK_RX2	= 0x02,
};

const PROGMEM uint8_t RLINK_init_values[] = {
  /* 00 */ 0x5B, 0x06, 0x5C, 0x07, 0xAB, 0xCD, 0x40, 0x04,
  /* 08 */ 0x45, 0x00, 0x00, 0x06, 0x00, 0x5C, 0x62, 0x76,
  /* 10 */ 0x7A, 0x7F, 0x13, 0x23, 0xF8, 0x44, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11
};

static void __attribute__((unused)) RLINK_init()
{
	rf_ch_num=(random(0xfefefefe)&0x0F)+1;		// 0x01..0x10
	memcpy(hopping_frequency,"\x12\xBA\x66\x72\x1E\x42\x06\x4E\x36\xAE\x8A\xA2\x2A\x7E\x96\x5A",16);
	hopping_frequency[rf_ch_num]=0xC6;
	#ifdef RLINK_FORCE_ID
		memcpy(rx_tx_addr,"\x3A\x99\x22\x3A",RLINK_TX_ID_LEN);
	#endif
}

static void __attribute__((unused)) RLINK_rf_init()
{
	CC2500_Strobe(CC2500_SIDLE);

	for (uint8_t i = 0; i < 39; ++i)
		CC2500_WriteReg(i, pgm_read_byte_near(&RLINK_init_values[i]));

	prev_option = option;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
}

static void __attribute__((unused)) RLINK_tune_freq()
{
	if ( prev_option != option )
	{
		CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
		prev_option = option ;
	}
}

static void __attribute__((unused)) RLINK_TIMING_RFSEND_packet()
{
	static uint32_t pseudo=0;
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 6;

	CC2500_Strobe(CC2500_SIDLE);

	// packet length
	packet[0] = RLINK_TX_PACKET_LEN;
	// header
	packet[1] = packet_count>3?0x03:0x01;	// packet type: 0x01 normal, 0x03 request telemetry
	//if(RX_num) packet[1] |= ((RX_num+2)<<4)+4;	// RX number limited to 10 values, 0 is a wildcard
	
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
	pseudo=((pseudo * 0xAA) + 3) % 0x7673;	// calc next pseudo random value
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[pseudo &0x0F]);
	packet[28]= pseudo;
	packet[29]= pseudo >> 8;
	packet[30]= 0x00;						// unknown
	packet[31]= 0x00;						// unknown
	packet[32]= rf_ch_num;					// value equal to 0xC6 in the RF table
	
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

	//debug("P:");
	//for(uint8_t i=0;i<RLINK_TX_PACKET_LEN+1;i++)
	//	debug(" 0x%02X",packet[i]);
	//debugln("");
}

#define RLINK_TIMING_PROTO	20000
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
			RLINK_tune_freq();
			RLINK_TIMING_RFSEND_packet();
#if not defined RLINK_HUB_TELEMETRY
				return RLINK_TIMING_PROTO;
#else
			if(packet[1]==0x01)
				return RLINK_TIMING_PROTO;
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
				if(packet_in[0]==RLINK_RX_PACKET_LEN && (packet_in[len-1] & 0x80) && memcmp(&packet[2],rx_tx_addr,RLINK_TX_ID_LEN)==0 && packet_in[6]==0x03)
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
					v_lipo2=packet_in[9]<<1;				//Batt
					telemetry_link=1;						//Send telemetry out
					pps_counter++;
					packet_count=0;
				}
				//debugln("");
			}
			if (millis() - pps_timer >= 2000)
			{//1 packet every 20ms
				pps_timer = millis();
				debugln("%d pps", pps_counter);
				TX_LQI = pps_counter;						//Max=100%
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