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

// Known UDI 2.4GHz protocol variants, all using BK2421
//  * UDI U819 coaxial 3ch helicoper
//  * UDI U816/817/818 quadcopters
//    - "V1" with orange LED on TX, U816 RX labeled '' , U817/U818 RX labeled 'UD-U817B'
//    - "V2" with red LEDs on TX, U816 RX labeled '', U817/U818 RX labeled 'UD-U817OG'
//    - "V3" with green LEDs on TX. Did not get my hands on yet.
//  * U830 mini quadcopter with tilt steering ("Protocol 2014")
//  * U839 nano quadcopter ("Protocol 2014")

#if defined(UDI_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_UDI_COUNT 1000

// Timeout for callback in uSec, 4ms=4000us for UDI
// ???
//#define PACKET_UDI_PERIOD 4000

#define BIND_PACKET_UDI_PERIOD 5000
#define PACKET_UDI_PERIOD 15000

#define BIND_PACKETS_UDI_PER_CHANNEL 11
#define PACKETS_UDI_PER_CHANNEL 11

#define NUM_UDI_RF_CHANNELS 16


#define INITIAL_UDI_WAIT 50000

#define PACKET_UDI_CHKTIME 100

// For readability
enum {
    UDI_CAMERA  = 1,
    UDI_VIDEO   = 2,
    UDI_MODE2   = 4,
    UDI_FLIP360 = 8,
    UDI_FLIP    =16,
    UDI_LIGHTS  =32
};

// This is maximum payload size used in UDI protocols
#define UDI_PAYLOADSIZE 16



static uint8_t payload_size;  // Bytes in payload for selected variant
static uint8_t bind_channel;
static uint8_t packets_to_hop;
static uint8_t packets_to_check;  // BIND_RX phase needs to receive/auto-ack more than one packet for RX to switch to next phase, it seems
static uint8_t packets_to_send;   // Number of packets to send / check for in current bind phase
static uint8_t bind_step_success; // Indicates successfull transmission / receive of bind reply during current bind phase
static uint8_t tx_id[3];
static uint8_t rx_id[3];
static uint8_t randoms[3];         // 3 random bytes choosen by TX, sent in BIND packets. Lower nibble of first byte sets index in RF CH table to use for BIND2


//
enum {
    UDI_INIT2 = 0,
    UDI_INIT2_NO_BIND,
    UDI_BIND1_TX,
    UDI_BIND1_RX,
    UDI_BIND2_TX,
    UDI_BIND2_RX,
    UDI_DATA
};

enum {
    PROTOOPTS_FORMAT = 0,
    PROTOOPTS_STARTBIND,
};
enum {
    STARTBIND_NO  = 0,
    STARTBIND_YES = 1,
};

// This are frequency hopping tables for UDI protocols

// uint8_t16 V1 (Orange LED) Bind CH 0x07
// TX ID 0x57, 0x5A, 0x2D
static const uint8_t freq_hopping_uint8_t16_v1[NUM_UDI_RF_CHANNELS] = {
 0x07, 0x21, 0x49, 0x0B, 0x39, 0x10, 0x25, 0x42,
 0x1D, 0x31, 0x35, 0x14, 0x28, 0x3D, 0x18, 0x2D
};

// Protocol 2014 (uint8_t30,uint8_t39,...) BIND CH 0x23 (second entry)
// DATA: hops ~ every 0.361s (0.350 ... 0.372)
static const uint8_t freq_hopping_uint8_t39[NUM_UDI_RF_CHANNELS] = {
 0x08, 0x23, 0x48, 0x0D, 0x3B, 0x12, 0x27, 0x44,
 0x1F, 0x33, 0x37, 0x16, 0x2A, 0x3F, 0x1A, 0x2F
};

// Points to proper table
static const uint8_t * rf_udi_channels = NULL;


static uint8_t packet_udi_ack()
{
	switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT))) {
		case BV(NRF24L01_07_TX_DS):		return PKT_ACKED;
		case BV(NRF24L01_07_MAX_RT):	return PKT_TIMEOUT;
	}
	return PKT_PENDING;
}

static void UDI_init()
{
    NRF24L01_Initialize();
    //NRF24L01_SetTxRxMode(TX_EN);
    
    switch (sub_protocol) {
	    case U816_V1:
	        rf_udi_channels = freq_hopping_uint8_t16_v1;
	        payload_size = 8;
	        break;

	    case U816_V2:
	        rf_udi_channels = NULL; // NO HOPPING !
	        payload_size = 7;
	        break;

	    case U839_2014:
	        // UDI 2014 Protocol (uint8_t30, uint8_t39, all other new products ?)
	        rf_udi_channels = freq_hopping_uint8_t39;
	        payload_size = 8;
	        break;
    }
    
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, payload_size);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x07);     // Clear status bits
    
    if ((sub_protocol == U816_V1) || (sub_protocol == U816_V2)) {
        NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x27);   // 
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x3A); // 
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);   // 3 byte address
        NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 
        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto-acknowledge on all data pipers, same as YD
        if (sub_protocol == U816_V1) {
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x7F);     // 
        } else {
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x7A);     // 
        }
    } else
    if (sub_protocol == U839_2014) {
        NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, 0x0F);   // 2Mbps air rate, 5dBm RF output power, high LNA gain
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A); // 500uS retransmit t/o, 10 tries (same as YD)
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);   // 3 byte address
        NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 
        NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto-acknowledge on all data pipers, same as YD
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);     // Enable CRC, 2 byte CRC, PWR UP, PRIMARY RX
    }
    
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    uint8_t status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, status);
    
    status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_FlushTx();
    status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, status);

    // Implicit delay in callback
    // delayMicroseconds(120)
}

static void UDI_init2()
{
    NRF24L01_FlushTx();
    bind_step_success = 0;
    packet_sent = 0;

    switch (sub_protocol) {
    case U816_V1:
        rf_ch_num = 0;
        bind_channel = rf_udi_channels[rf_ch_num++];
        break;
    case U816_V2:
        rf_ch_num = 0x07; // This is actual channel. No hopping here
        bind_channel = 0;
        break;
    case U839_2014:
        rf_ch_num = 1;
        bind_channel = rf_udi_channels[rf_ch_num++];
        break;
    }
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind_channel);

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t *) "\xe7\x7e\xe7", 3);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t *) "\xe7\x7e\xe7", 3);

    // Turn radio power on
    NRF24L01_SetTxRxMode(TX_EN);
    uint8_t config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
}

static void set_tx_id(uint32_t id)
{
    tx_id[0] = (id >> 16) & 0xFF;
    tx_id[1] = (id >> 8) & 0xFF;
    tx_id[2] = (id >> 0) & 0xFF;
    
/*    
	uint32_t val = rand32();	randoms[0] = val & 0xff;	randoms[1] = (val >> 8 ) & 0xff;	randoms[2] = (val >> 16 ) & 0xff;
*/    
	// FIXME
	// This one has been observed, leads to RF CH 0x1F (#08) used for BIND2
	randoms[0] = 0x98;	randoms[1] = 0x80;	randoms[2] = 0x5B;
}

static void add_pkt_checksum()
{
  // CHECKSUM was introduced with 2014 protocol
  if (sub_protocol < U839_2014) return;
  uint8_t sum = 0;
  for (uint8_t i = 0; i < payload_size-1;  ++i) sum += packet[i];
  packet[payload_size-1] = sum & 0x3f; // *sick*
}


static uint8_t convert_channel(uint8_t num, uint8_t chn_max, uint8_t sign_ofs)
{
    uint32_t ch = Servo_data[num];
    if (ch < PPM_MIN) {
        ch = PPM_MIN;
    } else if (ch > PPM_MAX) {
        ch = PPM_MAX;
    }
    uint32_t chn_val;
    if (sign_ofs) chn_val = (((ch * chn_max / PPM_MAX) + sign_ofs) >> 1);
    else chn_val = (ch * chn_max / PPM_MAX);
    if (chn_val < 0) chn_val = 0;
    else if (chn_val > chn_max) chn_val = chn_max;
    return (uint8_t) chn_val;
}


static void read_controls(uint8_t* throttle, uint8_t* rudder, uint8_t* elevator, uint8_t* aileron,
                          uint8_t* flags)
{
    // Protocol is registered AETRG, that is
    // Aileron is channel 0, Elevator - 1, Throttle - 2, Rudder - 3
    // Sometimes due to imperfect calibration or mixer settings
    // throttle can be less than PPM_MIN or larger than
    // PPM_MAX. As we have no space here, we hard-limit
    // channels values by min..max range

    // Channel 3: throttle is 0-100
    *throttle = convert_channel(THROTTLE, 0x64, 0);

    // Channel 4
    *rudder = convert_channel(RUDDER, 0x3f, 0x20);

    // Channel 2
    *elevator = convert_channel(ELEVATOR, 0x3f, 0x20);

    // Channel 1
    *aileron = convert_channel(AILERON, 0x3f, 0x20);

    // Channel 5
    if (Servo_data[AUX1] <= 0) *flags &= ~UDI_FLIP360;
    else *flags |= UDI_FLIP360;

    // Channel 6
    if (Servo_data[AUX2] <= 0) *flags &= ~UDI_FLIP;
    else *flags |= UDI_FLIP;

    // Channel 7
    if (Servo_data[AUX3] <= 0) *flags &= ~UDI_CAMERA;
    else *flags |= UDI_CAMERA;

    // Channel 8
    if (Servo_data[AUX4] <= 0) *flags &= ~UDI_VIDEO;
    else *flags |= UDI_VIDEO;

    // Channel 9
    if (Servo_data[AUX5] <= 0) *flags &= ~UDI_LIGHTS;
    else *flags |= UDI_LIGHTS;

    // Channel 10
    if (Servo_data[AUX6] <= 0) *flags &= ~UDI_MODE2;
    else *flags |= UDI_MODE2;
}

static void send_udi_packet(uint8_t bind)
{
    packet[7] = 0x4A;
    if (bind == 1) {
        // Bind phase 1
        // MAGIC
        packet[0] = 0x5A;  // NOTE: Also 0xF3, when RX does not ACK packets (uint8_t39, only TX on) ...
        // Current Address / TX ID
        if (sub_protocol == U839_2014) {
            // uint8_t39: Current RX/TX Addr
            packet[1] = 0xE7;
            packet[2] = 0x7E;
            packet[3] = 0xE7;
        } else {
            // uint8_t16: ID Fixed per TX
            packet[1] = tx_id[0];
            packet[2] = tx_id[1];
            packet[3] = tx_id[2];
        }
        // Pseudo random values (lower nibble of packet[4] determines index of RF CH used in BIND2)
        packet[4] = randoms[0];
        packet[5] = randoms[1];
        packet[6] = randoms[2];
        if (sub_protocol == U839_2014) {
            packet[7] = (packet_counter < 4) ? 0x3f : 0x04; // first four packets use 0x3f here, then 0x04
        }
    } else if (bind == 2) {
        // Bind phase 2
        // MAGIC
        packet[0] = 0xAA;
        // Current Address (RX "ID", pseudorandom again)
        packet[1] = rx_id[0];
        packet[2] = rx_id[1];
        packet[3] = rx_id[2];
        // Pseudo random values
        packet[4] = randoms[0];
        packet[5] = randoms[1];
        packet[6] = randoms[2];
        if (sub_protocol == U839_2014) {
            packet[7] = 0x04;
        }
    } else {
        // regular packet
        // Read channels (converts to required ranges)
        read_controls(&throttle, &rudder, &elevator, &aileron, &flags);
        // MAGIC
        packet[0] = 0x55;
        packet[1] = throttle; // throttle is 0-0x64
        // 3 Channels packed into 2 bytes (5bit per channel)
        uint16_t encoded = (rudder << 11) | (elevator << 6) | (aileron << 1);
        packet[2] = (encoded >> 8) & 0xff;
        packet[3] = encoded & 0xff;
        // Trims and flags (0x20 = center)
        packet[4] = 0x20; // rudder trim 6bit
        packet[5] = 0x20; // elev   trim 6bit
        packet[6] = 0x20; // ail    trim 6bit
        
        if (flags & UDI_FLIP) packet[4] |= 0x80;    // "Directional" flip
        if (flags & UDI_LIGHTS) packet[4] |= 0x40;  // Light on/off

        if (flags & UDI_MODE2) packet[5] |= 0x80;   // High rate ("Mode2")
        if (flags & UDI_FLIP360) packet[5] |= 0x40; // 360 degree flip

        if (flags & UDI_VIDEO) packet[6] |= 0x80;   // Video recording on/off
        if (flags & UDI_CAMERA) packet[6] |= 0x40;  // Take picture

        // NOTE: Only newer protocols have this (handled by routine)
        add_pkt_checksum();
    }

    uint8_t status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_WriteReg(NRF24L01_07_STATUS,status);

    if (packet_sent && bind && (status & BV(NRF24L01_07_TX_DS))) {
        bind_step_success = 1;
    }

    packet_sent = 0;
    
    // Check if its time to change channel
    // This seems to be done by measuring time,
    // not by counting packets, on UDI transmitters
    // NOTE: Seems even in bind phase channels are changed
    
    // NOTE: Only hop in TX mode ???
    if (rf_udi_channels && (bind == 0) && (packets_to_hop-- == 0)) {
        uint8_t rf_ch = rf_udi_channels[rf_ch_num];
        rf_ch_num++;
        rf_ch_num %= NUM_UDI_RF_CHANNELS;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch);
        
        packets_to_hop = bind ? BIND_PACKETS_UDI_PER_CHANNEL : PACKETS_UDI_PER_CHANNEL;
    }
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, payload_size);
    ++packet_counter;
    packet_sent = 1;
}


static uint16_t UDI_callback() {
	switch (phase) {
	case UDI_INIT2:
		UDI_init2();
		phase = UDI_BIND1_TX;
		return 120;
		break;
	case UDI_INIT2_NO_BIND:
		// Do nothing (stay forever)
		// Cannot re-bind on UDI protocol since IDs are random
		return 10000; // 10ms
		break;
	case UDI_BIND1_TX:
		if (packet_sent && packet_udi_ack() == PKT_ACKED) {	bind_step_success = 1;	}
		if (bind_step_success) {
			// All fine, wait for reply of receiver
			phase = UDI_BIND1_RX;

			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();

			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);
			bind_step_success = 0;
			//packets_to_check = 12; // according to SPI traces on uint8_t17B RX it receives 12 packets (and answers with 5)
			packets_to_check = 3;
		} else {
			send_udi_packet(1);
		}
		return BIND_PACKET_UDI_PERIOD;
		break;
	case UDI_BIND1_RX:
		// Check if data has been received
		if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR) ) {
			uint8_t data[UDI_PAYLOADSIZE];
			NRF24L01_ReadPayload(data, payload_size);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x4E); // On original TX this is done on LAST packet check only !
			NRF24L01_FlushRx();

			// Verify MAGIC and Random ID
			// (may be reply to bind packet from other TX)
			if ((data[0] == 0xA5) &&
			(data[4] == randoms[0]) &&
			(data[5] == randoms[1]) &&
			(data[6] == randoms[2]) &&
			(data[7] == randoms[2])) {
				rx_id[0] = data[1];
				rx_id[1] = data[2];
				rx_id[2] = data[3];
				if (sub_protocol != U816_V2) {
					rf_ch_num = randoms[0] & 0x0f;
				}
				bind_step_success = 1;
			}
		}
		// RX seems to need more than one ACK
		if (packets_to_check) packets_to_check--;
		//NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);
		if (bind_step_success && !packets_to_check) {
			// All fine, switch address and RF channel,
			// send bind packets with channel hopping now
			phase = UDI_BIND2_TX;

			packet_sent = 0;
			packets_to_send = 4;
			bind_step_success = 0;

			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_id, 3);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_id, 3);

			if (sub_protocol != U816_V2) {
				// Switch RF channel
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_udi_channels[rf_ch_num++]);
				rf_ch_num %= NUM_UDI_RF_CHANNELS;
			}

			NRF24L01_FlushTx();
			NRF24L01_FlushRx();
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x7E);

			NRF24L01_SetTxRxMode(TX_EN);
			//NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0E)

			return 10; // 10 µs (start sending immediately)
		}
		return BIND_PACKET_UDI_PERIOD;
		break;

	case UDI_BIND2_TX:
		if (packet_sent && packet_udi_ack() == PKT_ACKED) {
			bind_step_success = 1;
		}
		send_udi_packet(2);
		if (packets_to_send) --packets_to_send;
		if (bind_step_success || !packets_to_send) {
			// Seems the original TX ignores AACK, too !
			// U816 V1: 3 packets send, U839: 4 packets send
			// All fine, wait for reply of receiver
			phase = UDI_BIND2_RX;

			NRF24L01_SetTxRxMode(RX_EN);
			NRF24L01_FlushRx();

			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);
			bind_step_success = 0;
			packets_to_check = 14; // ???
		}
		return bind_step_success ? 4000 : 12000; // 4ms if no packed acked yet, 12ms afterwards
		//        return 120; // FIXME: Varies for first three packets !!!

		break;

	case UDI_BIND2_RX:
		// Check if data has been received
		if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR) ) {
			uint8_t data[UDI_PAYLOADSIZE];
			NRF24L01_ReadPayload(data, payload_size);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x4E);
			NRF24L01_FlushRx();

			// Verify MAGIC, RX Addr, Random ID
			// (may be reply to bind packet from other TX)
			if ((data[0] == 0xDD) &&
			(data[1] == rx_id[0]) &&
			(data[2] == rx_id[1]) &&
			(data[3] == rx_id[2]) &&
			(data[4] == randoms[0]) &&
			(data[5] == randoms[1]) &&
			(data[6] == randoms[2]) &&
			(data[7] == randoms[2])) {
				bind_step_success = 1;
			}
		}
		// RX seems to need more than one ACK
		if (packets_to_check) packets_to_check--;
		//NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);
		if (bind_step_success && !packets_to_check) {
			phase = UDI_DATA;
			NRF24L01_SetTxRxMode(TX_EN);
			NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x7E);
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0E);
			NRF24L01_FlushTx();

			// Switch RF channel
			if (sub_protocol == U816_V2) {
				// FIXED RF Channel
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch_num);
			} else {
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_udi_channels[rf_ch_num++]);
				rf_ch_num %= NUM_UDI_RF_CHANNELS;
			}

			flags = 0;
			BIND_DONE;
		}
		return BIND_PACKET_UDI_PERIOD;
		break;
	case UDI_DATA:
		if (packet_sent && packet_udi_ack() != PKT_ACKED) {
			return PACKET_UDI_CHKTIME;
		}
		send_udi_packet(0);
		break;
	}
	// Packet every 15ms
	return PACKET_UDI_PERIOD;
}

static uint16_t UDI_setup()
{
    packet_counter = 0;
    UDI_init();
    phase = UDI_INIT2;
    
    // observed on U839 TX
    set_tx_id(0x457C27);

    return INITIAL_UDI_WAIT;
}
#endif
