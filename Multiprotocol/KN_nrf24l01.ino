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

#if defined(KN_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define KN_BIND_COUNT 1000 // for KN 2sec every 2ms - 1000 packets
// Timeout for callback in uSec, 2ms=2000us for KN
#define KN_PACKET_PERIOD 2000
#define KN_PACKET_CHKTIME  100 // Time to wait for packet to be sent (no ACK, so very short)

//#define PAYLOADSIZE 16
#define NFREQCHANNELS 4
#define KN_TXID_SIZE 4


enum {
	KN_FLAG_DR     = 0x01, // Dual Rate
	KN_FLAG_TH     = 0x02, // Throttle Hold
	KN_FLAG_IDLEUP = 0x04, // Idle up
	KN_FLAG_RES1   = 0x08,
	KN_FLAG_RES2   = 0x10,
	KN_FLAG_RES3   = 0x20,
	KN_FLAG_GYRO3  = 0x40, // 00 - 6G mode, 01 - 3G mode
	KN_FLAG_GYROR  = 0x80  // Always 0 so far
};

//
enum {
	KN_INIT2 = 0,
	KN_INIT2_NO_BIND,
	KN_BIND,
	KN_DATA
};

/*enum {
	USE1MBPS_NO  = 0,
	USE1MBPS_YES = 1,
};*/

// 2-bytes CRC
#define CRC_CONFIG (BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO))

void kn_init()
{
	NRF24L01_Initialize();

	NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x20);   // bytes of data payload for pipe 0


    NRF24L01_Activate(0x73);                         // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
	// Enable: Dynamic Payload Length, Payload with ACK , W_TX_PAYLOAD_NOACK
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, BV(NRF2401_1D_EN_DPL) | BV(NRF2401_1D_EN_ACK_PAY) | BV(NRF2401_1D_EN_DYN_ACK));
    NRF24L01_Activate(0x73);
}

uint16_t kn_init2()
{
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	packet_sent = 0;
	packet_count = 0;
	hopping_frequency_no = 0;

	// Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, CRC_CONFIG | BV(NRF24L01_00_PWR_UP));
	return 150;
}

void set_tx_for_bind()
{
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 83);
	NRF24L01_SetBitrate(NRF24L01_BR_1M); // 1Mbps for binding
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *) "KNDZK", 5);
}

void set_tx_for_data()
{
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}

void kn_calc_fh_channels(uint32_t seed)
{
	uint8_t idx = 0;
	uint32_t rnd = seed;
	while (idx < NFREQCHANNELS) {
		uint8_t i;
		rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

		// Use least-significant byte. 73 is prime, so channels 76..77 are unused
		uint8_t next_ch = ((rnd >> 8) % 73) + 2;
		// Keep the distance 2 between the channels - either odd or even
		if (((next_ch ^ seed) & 0x01 )== 0)
			continue;
		// Check that it's not duplicate and spread uniformly
		for (i = 0; i < idx; i++) {
			if(hopping_frequency[i] == next_ch)
			break;
		}
		if (i != idx)
			continue;
		hopping_frequency[idx++] = next_ch;
	}
}

void kn_initialize_tx_id()
{
	rx_tx_addr[4] = 'K';
	kn_calc_fh_channels(MProtocol_id);
}

#define PACKET_COUNT_SHIFT 5
#define RF_CHANNEL_SHIFT 2
void kn_send_packet(uint8_t bind)
{
	uint8_t rf_ch;
	if (bind) {
		rf_ch = 83;
		packet[0]  = 'K';
		packet[1]  = 'N';
		packet[2]  = 'D';
		packet[3]  = 'Z';
		packet[4]  = rx_tx_addr[0];
		packet[5]  = rx_tx_addr[1];
		packet[6]  = rx_tx_addr[2];
		packet[7]  = rx_tx_addr[3];
		packet[8]  = hopping_frequency[0];
		packet[9]  = hopping_frequency[1];
		packet[10] = hopping_frequency[2];
		packet[11] = hopping_frequency[3];
		packet[12] = 0x00;
		packet[13] = 0x00;
		packet[14] = 0x00;
		packet[15] = 0x01; //mode_bitrate == USE1MBPS_YES ? 0x01 : 0x00;
	} else {
		rf_ch = hopping_frequency[hopping_frequency_no];

		// Each packet is repeated 4 times on the same channel
		// We're not strictly repeating them, rather we
		// send new packet on the same frequency, so the
		// receiver gets the freshest command. As receiver
		// hops to a new frequency as soon as valid packet
		// received it does not matter that the packet is
		// not the same one repeated twice - nobody checks this

		// NB! packet_count overflow is handled and used in
		// callback.
		if (++packet_count == 4)
			hopping_frequency_no = (hopping_frequency_no + 1) & 0x03;

		uint16_t kn_throttle, kn_rudder, kn_elevator, kn_aileron;
		kn_throttle = convert_channel_10b(THROTTLE);
		kn_aileron  = convert_channel_10b(AILERON);
		kn_elevator = convert_channel_10b(ELEVATOR);
		kn_rudder   = convert_channel_10b(RUDDER);
		packet[0]  = (kn_throttle >> 8) & 0xFF;
		packet[1]  = kn_throttle & 0xFF;
		packet[2]  = (kn_aileron >> 8) & 0xFF;
		packet[3]  = kn_aileron  & 0xFF;
		packet[4]  = (kn_elevator >> 8) & 0xFF;
		packet[5]  = kn_elevator & 0xFF;
		packet[6]  = (kn_rudder >> 8) & 0xFF;
		packet[7]  = kn_rudder & 0xFF;
		// Trims, middle is 0x64 (100) 0-200
		packet[8]  = 0x64; // T
		packet[9]  = 0x64; // A
		packet[10] = 0x64; // E
		packet[11] = 0x64; // R

		if (Servo_data[AUX1] > PPM_SWITCH)
			flags |= KN_FLAG_DR;
		else
			flags=0;
		if (Servo_data[AUX2] > PPM_SWITCH)
			flags |= KN_FLAG_TH;
		if (Servo_data[AUX3] > PPM_SWITCH)
			flags |= KN_FLAG_IDLEUP;
		if (Servo_data[AUX4] > PPM_SWITCH)
			flags |= KN_FLAG_GYRO3;

		packet[12] = flags;

		packet[13] = (packet_count << PACKET_COUNT_SHIFT) | (hopping_frequency_no << RF_CHANNEL_SHIFT);

		packet[14] = 0x00;
		packet[15] = 0x00;
	}

	packet_sent = 0;
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
	NRF24L01_FlushTx();
	NRF24L01_WritePayload(packet, 16);
	//++total_packets;
	packet_sent = 1;

	if (! hopping_frequency_no) {
		//Keep transmit power updated
		NRF24L01_SetPower();
	}
}

uint16_t kn_callback()
{
	uint16_t timeout = KN_PACKET_PERIOD;
	switch (phase)
	{
		case KN_INIT2:
			bind_counter = KN_BIND_COUNT;
			timeout = kn_init2();
			phase = KN_BIND;
			set_tx_for_bind();
			break;
		case KN_INIT2_NO_BIND:
			timeout = kn_init2();
			phase = KN_DATA;
			set_tx_for_data();
			break;
		case KN_BIND:
			if (packet_sent && NRF24L01_packet_ack() != PKT_ACKED)
				return KN_PACKET_CHKTIME;
			kn_send_packet(1);
			if (--bind_counter == 0) {
				phase = KN_DATA;
				set_tx_for_data();
				BIND_DONE;			
			}
			break;
		case KN_DATA:
			if (packet_count == 4)
				packet_count = 0;
			else {
				if (packet_sent && NRF24L01_packet_ack() != PKT_ACKED)
					return KN_PACKET_CHKTIME;
				kn_send_packet(0);
			}
			break;
	}
	return timeout;
}

uint16_t initKN(){	
	//total_packets = 0;
	//mode_bitrate=USE1MBPS_YES;
	kn_init();
	phase = IS_AUTOBIND_FLAG_on ? KN_INIT2 : KN_INIT2_NO_BIND;
	kn_initialize_tx_id();

	// Call callback in 50ms
	return 50000;
}

#endif
