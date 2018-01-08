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
// Last sync with hexfet new_protocols/KN_nrf24l01.c dated 2015-11-09

#if defined(KN_NRF24L01_INO)

#include "iface_nrf24l01.h"

// Wait for RX chip stable - 10ms
#define KN_INIT_WAIT_MS  10000

//Payload(16 bytes) plus overhead(10 bytes) is 208 bits, takes about 0.4ms or 0.2ms
//to send for the rate of 500kb/s and 1Mb/s respectively.

// Callback timeout period for sending bind packets, minimum 250
#define KN_BINDING_PACKET_PERIOD  1000

// Timeout for sending data packets, in uSec, KN protocol requires 2ms
#define KN_WL_SENDING_PACKET_PERIOD  2000
// Timeout for sending data packets, in uSec, KNFX protocol requires 1.2 ms
#define KN_FX_SENDING_PACKET_PERIOD  1200

// packets to be sent during binding, last 0.5 seconds in WL Toys and 0.2 seconds in Feilun
#define KN_WL_BIND_COUNT 500
#define KN_FX_BIND_COUNT 200 

#define KN_PAYLOADSIZE 16

//24L01 has 126 RF channels, can we use all of them?
#define KN_MAX_RF_CHANNEL 73

//KN protocol for WL Toys changes RF frequency every 10 ms, repeats with only 4 channels.
//Feilun variant uses only 2 channels, so we will just repeat the hopping channels later
#define KN_RF_CH_COUNT 4

//KN protocol for WL Toys sends 4 data packets every 2ms per frequency, plus a 2ms gap.
#define KN_WL_PACKET_SEND_COUNT 5
//KN protocol for Feilun sends 8 data packets every 1.2ms per frequency, plus a 0.3ms gap.
#define KN_FX_PACKET_SEND_COUNT 8
 
#define KN_TX_ADDRESS_SIZE 5

enum {
    KN_PHASE_PRE_BIND,
    KN_PHASE_BINDING,
    KN_PHASE_PRE_SEND,
    KN_PHASE_SENDING,
};

enum {
	KN_FLAG_DR     = 0x01, // Dual Rate: 1 - full range
	KN_FLAG_TH     = 0x02, // Throttle Hold: 1 - hold
	KN_FLAG_IDLEUP = 0x04, // Idle up: 1 - 3D
	KN_FLAG_RES1   = 0x08,
	KN_FLAG_RES2   = 0x10,
	KN_FLAG_RES3   = 0x20,
	KN_FLAG_GYRO3  = 0x40, // 0 - 6G mode, 1 - 3G mode
	KN_FLAG_GYROR  = 0x80  // Always 0 so far
};

//-------------------------------------------------------------------------------------------------
// This function init 24L01 regs and packet data for binding
// Send tx address, hopping table (for Wl Toys), and data rate to the KN receiver during binding.
// It seems that KN can remember these parameters, no binding needed after power up.
// Bind uses fixed TX address "KNDZK", 1 Mbps data rate and channel 83
//-------------------------------------------------------------------------------------------------
static void __attribute__((unused)) kn_bind_init()
{
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (uint8_t*)"KNDZK", 5);
	packet[0]  = 'K';
	packet[1]  = 'N';
	packet[2]  = 'D';
	packet[3]  = 'Z';
	//Use first four bytes of tx_addr
	packet[4]  = rx_tx_addr[0];
	packet[5]  = rx_tx_addr[1];
	packet[6]  = rx_tx_addr[2];
	packet[7]  = rx_tx_addr[3];

	if(sub_protocol==WLTOYS)
	{
		packet[8]  = hopping_frequency[0];
		packet[9]  = hopping_frequency[1];
		packet[10] = hopping_frequency[2];
		packet[11] = hopping_frequency[3];
	}
	else
	{
		packet[8]  = 0x00;
		packet[9]  = 0x00;
		packet[10] = 0x00;
		packet[11] = 0x00;
	}
	packet[12] = 0x00;
	packet[13] = 0x00;
	packet[14] = 0x00;
	packet[15] = 0x01;	//(USE1MBPS_YES) ? 0x01 : 0x00;

	//Set RF channel
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, 83);
}

//-------------------------------------------------------------------------------------------------
// Update control data to be sent
// Do it once per frequency, so the same values will be sent 4 times
// KN uses 4 10-bit data channels plus a 8-bit switch channel
//
// The packet[0] is used for pitch/throttle, the relation is hard coded, not changeable.
// We can change the throttle/pitch range though.
//
// How to use trim? V977 stock controller can trim 6-axis mode to eliminate the drift.
//-------------------------------------------------------------------------------------------------
static void __attribute__((unused)) kn_update_packet_control_data()
{
	uint16_t value;
	value = convert_channel_10b(THROTTLE);
	packet[0]  = (value >> 8) & 0xFF;
	packet[1]  = value & 0xFF;
	value = convert_channel_10b(AILERON);
	packet[2]  = (value >> 8) & 0xFF;
	packet[3]  = value & 0xFF;
	value = convert_channel_10b(ELEVATOR);
	packet[4]  = (value >> 8) & 0xFF;
	packet[5]  = value & 0xFF;
	value = convert_channel_10b(RUDDER);
	packet[6]  = (value >> 8) & 0xFF;
	packet[7]  = value & 0xFF;
	// Trims, middle is 0x64 (100) range 0-200
	packet[8]  = convert_channel_16b_limit(CH9,0,200); // 0x64; // T
	packet[9]  = convert_channel_16b_limit(CH10,0,200); // 0x64; // A
	packet[10] = convert_channel_16b_limit(CH11,0,200); // 0x64; // E
	packet[11] = 0x64; // R

	flags=0;
	if (CH5_SW)
		flags = KN_FLAG_DR;
	if (CH6_SW)
		flags |= KN_FLAG_TH;
	if (CH7_SW)
		flags |= KN_FLAG_IDLEUP;
	if (CH8_SW)
		flags |= KN_FLAG_GYRO3;

	packet[12] = flags;

	packet[13] = 0x00;
	if(sub_protocol==WLTOYS)
		packet[13] = (packet_sent << 5) | (hopping_frequency_no << 2);

	packet[14] = 0x00;
	packet[15] = 0x00;

	NRF24L01_SetPower();
}


//-------------------------------------------------------------------------------------------------
// This function generate RF TX packet address
// V977 can remember the binding parameters; we do not need rebind when power up.
// This requires the address must be repeatable for a specific RF ID at power up.
//-------------------------------------------------------------------------------------------------
static void __attribute__((unused)) kn_calculate_tx_addr()
{
	if(sub_protocol==FEILUN)
	{
		uint8_t addr2;
		// Generate TXID with sum of minimum 256 and maximum 256+MAX_RF_CHANNEL-32
		rx_tx_addr[1] = 1 + rx_tx_addr[0] % (KN_MAX_RF_CHANNEL-33);
		addr2 = 1 + rx_tx_addr[2] % (KN_MAX_RF_CHANNEL-33);
		if ((uint16_t)(rx_tx_addr[0] + rx_tx_addr[1]) < 256)
			rx_tx_addr[2] = addr2;
		else
			rx_tx_addr[2] = 0x00;
		rx_tx_addr[3] = 0x00;
		while((uint16_t)(rx_tx_addr[0] + rx_tx_addr[1] + rx_tx_addr[2] + rx_tx_addr[3]) < 257)
			rx_tx_addr[3] += addr2;
	}
    //The 5th byte is a constant, must be 'K'
    rx_tx_addr[4] = 'K';
}

//-------------------------------------------------------------------------------------------------
// This function generates "random" RF hopping frequency channel numbers.
// These numbers must be repeatable for a specific seed
// The generated number range is from 0 to MAX_RF_CHANNEL. No repeat or adjacent numbers
//
// For Feilun variant, the channels are calculated from TXID, and since only 2 channels are used
// we copy them to fill up to MAX_RF_CHANNEL
//-------------------------------------------------------------------------------------------------
static void __attribute__((unused)) kn_calculate_freqency_hopping_channels()
{
	if(sub_protocol==WLTOYS)
	{
		uint8_t idx = 0;
		uint32_t rnd = MProtocol_id;
		while (idx < KN_RF_CH_COUNT)
		{
			uint8_t i;
			rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

			// Use least-significant byte. 73 is prime, so channels 76..77 are unused
			uint8_t next_ch = ((rnd >> 8) % KN_MAX_RF_CHANNEL) + 2;
			// Keep the distance 2 between the channels - either odd or even
			if (((next_ch ^ MProtocol_id) & 0x01 )== 0)
				continue;
			// Check that it's not duplicate and spread uniformly
			for (i = 0; i < idx; i++)
				if(hopping_frequency[i] == next_ch)
					break;
			if (i != idx)
				continue;
			hopping_frequency[idx++] = next_ch;
		}
	}
	else
	{//FEILUN
		hopping_frequency[0] = rx_tx_addr[0] + rx_tx_addr[1] + rx_tx_addr[2] + rx_tx_addr[3]; // - 256; ???
		hopping_frequency[1] = hopping_frequency[0] + 32;
		hopping_frequency[2] = hopping_frequency[0];
		hopping_frequency[3] = hopping_frequency[1];
	}
}

//-------------------------------------------------------------------------------------------------
// This function setup 24L01
// V977 uses one way communication, receiving only. 24L01 RX is never enabled.
// V977 needs payload length in the packet. We should configure 24L01 to enable Packet Control Field(PCF)
//   Some RX reg settings are actually for enable PCF
//-------------------------------------------------------------------------------------------------
static void __attribute__((unused)) kn_init()
{
	kn_calculate_tx_addr();
	kn_calculate_freqency_hopping_channels();

	NRF24L01_Initialize();

	NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO)); 
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0);    // Disable retransmit
	NRF24L01_SetPower();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x20);   // bytes of data payload for pipe 0


	NRF24L01_Activate(0x73);
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 1); // Dynamic payload for data pipe 0
	// Enable: Dynamic Payload Length to enable PCF
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, _BV(NRF2401_1D_EN_DPL));

	NRF24L01_SetPower();

	NRF24L01_FlushTx();
	// Turn radio power on
	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_SetBitrate(NRF24L01_BR_1M);	//USE1MBPS_YES ? NRF24L01_BR_1M : NRF24L01_BR_250K;
}
  
//================================================================================================
// Private Functions
//================================================================================================
uint16_t initKN()
{
	if(sub_protocol==WLTOYS)
	{
		packet_period = KN_WL_SENDING_PACKET_PERIOD;
		bind_counter  = KN_WL_BIND_COUNT;
		packet_count  = KN_WL_PACKET_SEND_COUNT;
	}
	else
	{
		packet_period = KN_FX_SENDING_PACKET_PERIOD;
		bind_counter  = KN_FX_BIND_COUNT;
		packet_count  = KN_FX_PACKET_SEND_COUNT;
	}
	kn_init();
	phase = IS_BIND_IN_PROGRESS ? KN_PHASE_PRE_BIND : KN_PHASE_PRE_SEND;

	return KN_INIT_WAIT_MS;
}

uint16_t kn_callback()
{
	switch (phase)
	{
		case KN_PHASE_PRE_BIND:
			kn_bind_init();
			phase=KN_PHASE_BINDING;
			//Do once, no break needed
		case KN_PHASE_BINDING:
			if(bind_counter>0)
			{
				bind_counter--;
				NRF24L01_WritePayload(packet, KN_PAYLOADSIZE);
				return KN_BINDING_PACKET_PERIOD;
			}
			BIND_DONE;
			//Continue
		case KN_PHASE_PRE_SEND:
			packet_sent = 0;
			hopping_frequency_no = 0;
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, KN_TX_ADDRESS_SIZE);
			phase = KN_PHASE_SENDING;
			//Do once, no break needed
		case KN_PHASE_SENDING:
			if(packet_sent >= packet_count)
			{
				packet_sent = 0;
				hopping_frequency_no++;
				if(hopping_frequency_no >= KN_RF_CH_COUNT) hopping_frequency_no = 0;
				kn_update_packet_control_data();
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
			}
			else
			{
				// Update sending count and RF channel index.
				// The protocol sends 4 data packets every 2ms per frequency, plus a 2ms gap.
				// Each data packet need a packet number and RF channel index
				packet[13] = 0x00;
				if(sub_protocol==WLTOYS)
					packet[13] = (packet_sent << 5) | (hopping_frequency_no << 2);
			}
			NRF24L01_WritePayload(packet, KN_PAYLOADSIZE);
			packet_sent++;
			return packet_period;
	} //switch
 
    //Bad things happened, reset
    packet_sent = 0;
    phase = KN_PHASE_PRE_SEND;
    return packet_period;
}

#endif
