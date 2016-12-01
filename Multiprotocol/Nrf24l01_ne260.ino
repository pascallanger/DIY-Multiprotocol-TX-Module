/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */
/* This code is based upon code from:
   http://www.rcgroups.com/forums/showthread.php?t=1564343
   Author  : Ferenc Szili (kile at the rcgroups.net forum)
*/


#if defined(NE260_NRF24L01_INO)
#include "iface_nrf24l01.h"

////////////////////////////////////////////////////////////
///////////////////////
// register bits
///////////////////////

// CONFIG
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0

// EN_AA
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0

// EN_RXADDR
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0

// RF_SETUP
#define CONT_WAVE   7
#define RF_DR_LOW   5
#define PLL_LOCK    4
#define RF_DR_HIGH  3
#define RF_PWR_HIGH 2
#define RF_PWR_LOW  1
#define LNA_HCURR   0	// obsolete in nRF24L01+

// STATUS
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define TX_FULL     0

// FIFO_STATUS
#define TX_REUSE        6
#define FIFO_TX_FULL    5
#define TX_EMPTY        4
#define RX_FULL         1
#define RX_EMPTY        0

///////////////////////
// register bit values
///////////////////////

// CONFIG
#define vMASK_RX_DR		(1<<(MASK_RX_DR))
#define vMASK_TX_DS		(1<<(MASK_TX_DS))
#define vMASK_MAX_RT	(1<<(MASK_MAX_RT))
#define vEN_CRC			(1<<(EN_CRC))
#define vCRCO			(1<<(CRCO))
#define vPWR_UP			(1<<(PWR_UP))
#define vPRIM_RX		(1<<(PRIM_RX))

// EN_AA
#define vENAA_P5		(1<<(ENAA_P5))
#define vENAA_P4		(1<<(ENAA_P4))
#define vENAA_P3		(1<<(ENAA_P3))
#define vENAA_P2		(1<<(ENAA_P2))
#define vENAA_P1		(1<<(ENAA_P1))
#define vENAA_P0		(1<<(ENAA_P0))

// EN_RXADDR
#define vERX_P5			(1<<(ERX_P5))
#define vERX_P4			(1<<(ERX_P4))
#define vERX_P3			(1<<(ERX_P3))
#define vERX_P2			(1<<(ERX_P2))
#define vERX_P1			(1<<(ERX_P1))
#define vERX_P0			(1<<(ERX_P0))

// SETUP_AW -- address widths in bytes
#define vAW_3			1
#define vAW_4			2
#define vAW_5			3

// RF_SETUP
#define vCONT_WAVE		(1<<(CONT_WAVE))
#define vRF_DR_LOW		(1<<(RF_DR_LOW))
#define vPLL_LOCK		(1<<(PLL_LOCK))
#define vRF_DR_HIGH		(1<<(RF_DR_HIGH))
#define vRF_PWR_HIGH	(1<<(RF_PWR_HIGH))
#define vRF_PWR_LOW		(1<<(RF_PWR_LOW))
#define vLNA_HCURR		(1<<(LNA_HCURR))	// obsolete in nRF24L01+

#define vRF_DR_1MBPS	0
#define vRF_DR_2MBPS	(1<<(RF_DR_HIGH))
#define vRF_DR_250KBPS	(1<<(RF_DR_LOW))

#define vRF_PWR_M18DBM	0x00
#define vRF_PWR_M12DBM	0x02
#define vRF_PWR_M6DBM	0x04
#define vRF_PWR_0DBM	0x06

#define vARD_250us		0x00
#define vARD_500us		0x10
#define vARD_750us		0x20
#define vARD_1000us		0x30
#define vARD_1250us		0x40
#define vARD_1500us		0x50
#define vARD_1750us		0x60
#define vARD_2000us		0x70
#define vARD_2250us		0x80
#define vARD_2500us		0x90
#define vARD_2750us		0xA0
#define vARD_3000us		0xB0
#define vARD_3250us		0xC0
#define vARD_3500us		0xD0
#define vARD_3750us		0xE0
#define vARD_4000us		0xF0

// STATUS
#define vRX_DR			(1<<(RX_DR))
#define vTX_DS			(1<<(TX_DS))
#define vMAX_RT			(1<<(MAX_RT))
#define vTX_FULL		(1<<(TX_FULL))

#define RX_P_NO(stat)			((stat >> 1) & 7)
#define HAS_RX_PAYLOAD(stat)	((stat & 0b1110) < 0b1100)

// FIFO_STATUS
#define vTX_REUSE		(1<<(TX_REUSE))
#define vTX_FULL		(1<<(TX_FULL))
#define vTX_EMPTY		(1<<(TX_EMPTY))
#define vRX_FULL		(1<<(RX_FULL))
#define vRX_EMPTY		(1<<(RX_EMPTY))
////////////////////////////////////////////////////////////
uint8_t		neChannel = 10;
uint8_t		neChannelOffset = 0;
#define PACKET_NE_LENGTH	7

static uint16_t model_id = 0xA04A;

uint8_t NE_ch[]={THROTTLE, RUDDER, ELEVATOR, AILERON,  AUX1};
uint8_t NEAddr[] = {0x34, 0x43, 0x10, 0x10, 0x01};
enum {
	NE260_BINDTX,
	NE260_BINDRX,
	NE260_DATA1,
	NE260_DATA2,
	NE260_DATA3,
};


static void ne260_init() {
	NRF24L01_Initialize();

	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, NEAddr, 5);   // write the address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, NEAddr, 5);

	NRF24L01_WriteReg(NRF24L01_01_EN_AA, vENAA_P0);                  // enable auto acknoledge
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, vARD_500us);   // ARD=500us, ARC=disabled
	NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, vRF_DR_250KBPS | vLNA_HCURR | vRF_PWR_0DBM);     // data rate, output power and noise cancel
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PACKET_NE_LENGTH);  // RX payload length
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, vERX_P0);               // enable RX address
	NRF24L01_WriteReg(NRF24L01_07_STATUS, vRX_DR | vTX_DS | vMAX_RT);        // reset the IRQ flags
}

static void send_ne_data_packet() {
	for(int i = 0; i < 4; i++) {
		uint32_t value = (uint32_t)map(limit_channel_100(NE_ch[i]),servo_min_100,servo_max_100,0,80);
		if (value > 0x7f)
			value = 0x7f;
		else if(value < 0)
			value = 0;
		packet[i] = value;
	}
	packet[4] = 0x55;
	packet[5] = model_id & 0xff;
	packet[6] = (model_id >> 8) & 0xff;

	NRF24L01_FlushTx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, vMAX_RT);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, neChannel + neChannelOffset);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, vEN_CRC | vCRCO | vPWR_UP);
	// send a fresh packet to the nRF
	NRF24L01_WritePayload((uint8_t*) packet, PACKET_NE_LENGTH);
}

static void send_ne_bind_packet() {
	packet[0] = 0xAA; //throttle
	packet[1] = 0xAA; //rudder
	packet[2] = 0xAA; //elevator
	packet[3] = 0xAA; //aileron
	packet[4] = 0xAA; //command
	packet[5] = model_id & 0xff;
	packet[6] = (model_id >> 8) & 0xff;

	NRF24L01_WriteReg(NRF24L01_07_STATUS, vRX_DR | vTX_DS | vMAX_RT);        // reset the status flags
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, neChannel + neChannelOffset);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload((uint8_t*) &packet, PACKET_NE_LENGTH);      // send the bind packet
}

static uint16_t ne260_cb() {
	if (state == NE260_BINDTX) {
		// do we have a packet?
		if ((NRF24L01_ReadReg(NRF24L01_07_STATUS) & vRX_DR) != 0) {
			// read the packet contents
			NRF24L01_ReadPayload(packet, PACKET_NE_LENGTH);
			
			// is this the bind response packet?
			if (strncmp("\x55\x55\x55\x55\x55", (char*) (packet + 1), 5) == 0 &&  *((uint16_t*)(packet + 6)) == model_id) {
				// exit the bind loop
				state = NE260_DATA1;
				NRF24L01_FlushTx();
				NRF24L01_FlushRx();
				NRF24L01_SetTxRxMode(TX_EN);
				return 2000;
			}
		}
		NRF24L01_SetTxRxMode(TX_EN);
		send_ne_bind_packet();
		state = NE260_BINDRX;
		return 500;
	} else if (state == NE260_BINDRX) {
		// switch to RX mode
		while (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & (vMAX_RT | vTX_DS))) ;
		NRF24L01_WriteReg(NRF24L01_07_STATUS, vTX_DS);
		
		NRF24L01_SetTxRxMode(RX_EN);
		NRF24L01_FlushRx();
		state = NE260_BINDTX;
		return 2000;
	}
	else if (state == NE260_DATA1) {	neChannel = 10;	state = NE260_DATA2;	}
	else if (state == NE260_DATA2) {	neChannel = 30;	state = NE260_DATA3;	}
	else if (state == NE260_DATA3) {	neChannel = 50;	state = NE260_DATA1;	}
	send_ne_data_packet();
	return 2500;
}

static uint16_t NE260_setup() {
	ne260_init();
	state = NE260_BINDTX;
	
	return 10000;
}
#endif
