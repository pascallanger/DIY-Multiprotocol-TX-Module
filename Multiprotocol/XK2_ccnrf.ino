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
// Compatible with XK TX X4 and model A160S.

#if defined(XK2_CCNRF_INO)

#include "iface_xn297.h"

//#define FORCE_XK2_ID
//#define FORCE_XK2_P10_ID

#define XK2_RF_BIND_CHANNEL	71
#define XK2_P10_RF_BIND_CHANNEL	69
#define XK2_PAYLOAD_SIZE	9
#define XK2_PACKET_PERIOD	4911
#define XK2_RF_NUM_CHANNELS	4

enum {
	XK2_BIND1,
	XK2_BIND2,
	XK2_DATA_PREP,
	XK2_DATA
};

static void __attribute__((unused)) XK2_send_packet()
{
	static uint8_t trim_ch=0;
	
	if(IS_BIND_IN_PROGRESS)
	{
		packet[0] = 0x9D;
		//TXID
		memcpy(&packet[1], rx_tx_addr, 3);
		//RXID
		//memcpy(&packet[4], rx_id     , 3);
		//Unknown
		packet[7] = 0x00;
		//Checksum seed
		packet[8] = 0xC0;
	}
	else
	{
		XN297_Hopping(hopping_frequency_no);
		hopping_frequency_no++;
		hopping_frequency_no &= 0x03;
		//Channels
		packet[0] = convert_channel_16b_limit(AILERON ,0x00,0x64);		//Aileron
		packet[1] = convert_channel_16b_limit(ELEVATOR,0x00,0x64);		//Elevator
		packet[2] = convert_channel_16b_limit(THROTTLE,0x00,0x64);		//Throttle
		packet[3] = convert_channel_16b_limit(RUDDER  ,0x00,0x64);		//Rudder
		//Center the trims
		trim_ch++;
		if(trim_ch > 2) trim_ch = 0;
		packet[4] = 0x20 + 0x40 * trim_ch;								//Trims are A=01..20..3F/E=41..60..7F/R=81..A0..BF, E0 appears when telemetry is received, C1 when p[6] changes from 00->08, C0 when p[6] changes from 08->00
		if(trim_ch == 2)												//Drive rudder trim since otherwise there is no control...
		{
			packet[4] = 0x80 + (convert_channel_8b(RUDDER)>>2);
			if(packet[4] <= 0x81) packet[4] = 0x81;
		}
		//Flags
		packet[5] = GET_FLAG(CH5_SW, 0x01)								//Rate
				  | GET_FLAG(CH6_SW, 0x08)								//Mode
				  | GET_FLAG(CH7_SW, 0x20)								//Hover
				  | GET_FLAG(CH8_SW, 0x40);								//Light
		//Telemetry not received=00, Telemetry received=01 but sometimes switch to 1 even if telemetry is not there...
		packet[6] = 0x00;
		//RXID checksum
		packet[7] = crc8;												//Sum RX_ID[0..2]
		//Checksum seed
		packet[8] = num_ch;												//Based on TX ID
	}
	//Checksum
	for(uint8_t i=0; i<XK2_PAYLOAD_SIZE-1; i++)
		packet[8] += packet[i];
	if(sub_protocol == XK2_P10)
		packet[8] += 0x10;

	// Send
	XN297_SetFreqOffset();
	XN297_SetPower();
	XN297_SetTxRxMode(TX_EN);
	XN297_WritePayload(packet, XK2_PAYLOAD_SIZE);
	#if 0
		debug("P");
		for(uint8_t i=0; i<XK2_PAYLOAD_SIZE; i++)
			debug(" %02X",packet[i]);
		debugln();
	#endif
}

static void __attribute__((unused)) XK2_RF_init()
{
	XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K);
	
	XN297_SetTXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", 5);
	XN297_SetRXAddr((uint8_t*)"\xcc\xcc\xcc\xcc\xcc", XK2_PAYLOAD_SIZE);

	XN297_HoppingCalib(XK2_RF_NUM_CHANNELS);
	XN297_RFChannel(sub_protocol==XK2_X4?XK2_RF_BIND_CHANNEL:XK2_P10_RF_BIND_CHANNEL);
}

static void __attribute__((unused)) XK2_initialize_txid()
{
	rx_tx_addr[0] = rx_tx_addr[3];				// Use RX_num

	num_ch = 0x21 + rx_tx_addr[0] - rx_tx_addr[1] + rx_tx_addr[2];

	//RF frequencies for X4: 65=0x41, 69=0x45, 73=0x49, 77=0x4D
	//RF frequencies for P10: 67, unknown
	uint8_t start = 65;
	if(sub_protocol == XK2_P10) start += 2;
	for(uint8_t i=0;i<XK2_RF_NUM_CHANNELS;i++)
		hopping_frequency[i] = start + i*4;

	#ifdef FORCE_XK2_ID
		if(rx_tx_addr[3]&1)
		{//Pascal
			rx_tx_addr[0] = 0x66;
			rx_tx_addr[1] = 0x4F;
			rx_tx_addr[2] = 0x47;
			num_ch = 0x7F;
			//hopping frequencies 65=0x41, 69=0x45, 73=0x49, 77=0x4D
		}
		else
		{//Marc
			rx_tx_addr[0] = 0x36;
			rx_tx_addr[1] = 0x49;
			rx_tx_addr[2] = 0x6B;
			num_ch = 0x79;
			//hopping frequencies 65=0x41, 69=0x45, 73=0x49, 77=0x4D
		}
	#endif
	#ifdef FORCE_XK2_P10_ID
		rx_tx_addr[0] = 0xE8;
		rx_tx_addr[1] = 0x25;
		rx_tx_addr[2] = 0x3B;
		num_ch = 0x1F;
		//hopping frequencies 67=0x43, =0x, =0x, =0x
	#endif

	rx_tx_addr[3] = rx_tx_addr[4] = 0xCC;
	debugln("ID: %02X %02X %02X %02X %02X, OFFSET: %02X, HOP: %02X %02X %02X %02X",rx_tx_addr[0],rx_tx_addr[1],rx_tx_addr[2],rx_tx_addr[3],rx_tx_addr[4],num_ch,hopping_frequency[0],hopping_frequency[1],hopping_frequency[2],hopping_frequency[3]);
}

uint16_t XK2_callback()
{
	switch(phase)
	{
		case XK2_BIND1:
			// switch to RX mode
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(RX_EN);
			phase++;
			return 5000;
		case XK2_BIND2:
			if(XN297_IsRX())
			{
				XN297_ReadPayload(packet, XK2_PAYLOAD_SIZE);
				#if 0
					debug("RX");
					for(uint8_t i=0; i<XK2_PAYLOAD_SIZE; i++)
						debug(" %02X",packet[i]);
					debugln("");
				#endif
				crc8 = 0xBF;
				for(uint8_t i=0; i<XK2_PAYLOAD_SIZE-1; i++)
					crc8 += packet[i];
				if(sub_protocol == XK2_P10)
					crc8 += 0x10;
				if(crc8 != packet[8])
				{
					phase = XK2_BIND1;
					return 1000;
				}
				if(packet[0] == 0x9B)
					phase++;
				else
				{
					//checksum of RX_ID
					crc8 = packet[4] + packet[5] + packet[6];
					debugln("W:RX_ID=%02X",crc8);
					eeprom_write_byte((EE_ADDR)(XK2_EEPROM_OFFSET+RX_num),crc8);
					XN297_SetTxRxMode(TXRX_OFF);
					XN297_SetTxRxMode(TX_EN);
					bind_counter = 10;					//send 10 bind end packets
					phase = XK2_DATA;
				}
			}
			return 1000;
		case XK2_DATA_PREP:
			crc8 = eeprom_read_byte((EE_ADDR)(XK2_EEPROM_OFFSET+RX_num));
			debugln("R:RX_ID=%02X",crc8);
			XN297_SetTxRxMode(TXRX_OFF);
			XN297_SetTxRxMode(TX_EN);
			XN297_SetTXAddr(rx_tx_addr, 5);
			BIND_DONE;
			phase++;
		case XK2_DATA:
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(XK2_PACKET_PERIOD);
			#endif
			if(bind_counter)
			{
				bind_counter--;
				if(bind_counter == 0)
				{
					phase = XK2_DATA_PREP;
					//phase = XK2_BIND1;
				}
			}
			XK2_send_packet();
			break;
	}
	return XK2_PACKET_PERIOD;
}

void XK2_init()
{
	//BIND_IN_PROGRESS;	// autobind protocol
	XK2_initialize_txid();
	XK2_RF_init();
	
	if(IS_BIND_IN_PROGRESS)
		phase = XK2_BIND1;
	else
		phase = XK2_DATA_PREP;
	bind_counter = 0;
	hopping_frequency_no = 0;
}

#endif

/*
XK A160 Piper CUB

Bind
----
Plane sends these packets:
RX:     0us C=71 S=Y A= CC CC CC CC CC P(9)= 9C BB CC DD 38 12 10 00 19
P[0] = 9C bind phase 1
P[1] = Dummy TX_ID
P[2] = Dummy TX_ID
P[3] = Dummy TX_ID
P[4] = RX_ID[0]
P[5] = RX_ID[1]
P[6] = RX_ID[2]
P[7] = 00
P[8] = sum P[0..7] + BF

TX responds to plane:
RX 9D 66 4F 47 38 12 10 00 B3
P[0] = 9D bind phase 2
P[1] = TX_ID[0]
P[2] = TX_ID[1]
P[3] = TX_ID[2]
P[4] = RX_ID[0]
P[5] = RX_ID[1]
P[6] = RX_ID[2]
P[7] = 00
P[8] = sum P[0..7] + C0

Planes ack:
RX:  4299us C=71 S=Y A= CC CC CC CC CC P(9)= 9B 66 4F 47 38 12 10 00 B0
RX: 26222us C=71 S=Y A= CC CC CC CC CC P(9)= 9B 66 4F 47 38 12 10 00 B0
RX:  8743us C=71 S=Y A= CC CC CC CC CC P(9)= 9B 66 4F 47 38 12 10 00 B0
P[0] = 9B bind phase 3
P[1] = TX_ID[0]
P[2] = TX_ID[1]
P[3] = TX_ID[2]
P[4] = RX_ID[0]
P[5] = RX_ID[1]
P[6] = RX_ID[2]
P[7] = 00
P[8] = sum P[0..7] + BF

Normal
------
TX sends
C=65,69,73,77 -> only one channel when telemetry is working
250K C=69 S=Y A= 66 4F 47 CC CC P(9)= 32 32 00 32 E0 00 01 5A 50
P[0] = A 00..32..64
P[1] = E 00..32..64
P[2] = T 00..64
P[3] = R 00..32..64
P[4] = alternates 20,60,A0,E0
       trims
		A 01..20..3F
		E 41..60..7F
		R 81..A0..BF
	   telemetry
	    E0 present when the telemetry works
	   6g/3d
		C1 few times if P[6] flag 00->08
		C0 few times if P[6] = flag 08->00
P[5] = flags
        01=high rate
		20=hover=long_press_left
		40=light -> temporary
		08=6g/3d=short_press_right sequece also switches for a few packets to C1 if 8 C0 if 0
P[6] = 00 telemetry nok
       01 telemetry ok but sometimes switch to 1 also when telemetry is nok...
P[7] = 5A -> sum RX_ID[0..2] 
P[8] = sum P[0..7] + TX_ID[0] - TX_ID[1] + TX_ID[2] + 21

Telemetry
RX on channel: 69, Time:  3408us P: 66 4F 47 00 00 00 00 00 C8
P[0] = TX_ID[0]
P[1] = TX_ID[1]
P[2] = TX_ID[2]
P[8] = sum P[0..7] + CC

Timing when plane is not detected:
RF
2469 110713 0
2473 114560 3847
2477 120291 5731
2465 135684 15393
2469 142138 6454
2473 145984 3846
2477 151753 5769
2465 155330 3577

*/
/* P10 Piper CUB
Bind
----
Phase 1
Plane sends these packets:
250K C=69 S=Y A= CC CC CC CC CC P(9)= 9C BB CC DD 84 24 20 00 97
P[0] = 9C bind phase 1
P[1] = Dummy TX_ID
P[2] = Dummy TX_ID
P[3] = Dummy TX_ID
P[4] = RX_ID[0]
P[5] = RX_ID[1]
P[6] = RX_ID[2]
P[7] = 00
P[8] = sum P[0..7] + BF + 10

Normal
------
TX sends
C=67 -> only one channel when telemetry is working
A= E8 25 3B CC CC P(9)= 32 32 00 32 A0 40 01 C8 6E
P[0] = A 00..32..64
P[1] = E 00..32..64
P[2] = T 00..64
P[3] = R 00..32..64
P[4] = alternates 20,60,A0,E0
       trims
		A 01..20..3F
		E 41..60..7F
		R 81..A0..BF
	   telemetry
	    E0 present when the telemetry works
	   6g/3d
		C1 few times if P[6] flag 00->08
		C0 few times if P[6] = flag 08->00
P[5] = flags
        01=high rate
		20=hover=long_press_left
		40=light -> temporary
		08=6g/3d=short_press_right sequece also switches for a few packets to C1 if 8 C0 if 0
P[6] = 00 telemetry nok
       01 telemetry ok but sometimes switch to 1 also when telemetry is nok...
P[7] = C8 -> sum RX_ID[0..2] 
P[8] = sum P[0..7] + TX_ID[0] - TX_ID[1] + TX_ID[2] + 21 +10
*/