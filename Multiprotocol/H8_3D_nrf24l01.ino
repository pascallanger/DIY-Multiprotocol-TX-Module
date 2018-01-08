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
// compatible with EAchine 3D X4, CG023/CG031, Attop YD-822/YD-829/YD-829C and H8_3D/JJRC H20/H22
// Merged CG023 and H8_3D protocols
// Last sync with hexfet new_protocols/cg023_nrf24l01.c dated 2015-10-03
// Last sync with hexfet new_protocols/h8_3d_nrf24l01.c dated 2015-11-18

#if defined(H8_3D_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define H8_3D_PACKET_PERIOD		1800
#define H20H_PACKET_PERIOD		9340
#define H20MINI_PACKET_PERIOD	3100
#define H8_3D_INITIAL_WAIT		500
#define H8_3D_PACKET_SIZE		20
#define H8_3D_RF_NUM_CHANNELS	4
#define H20H_BIND_RF			0x49
#define H8_3D_BIND_COUNT		1000

enum H8_3D_FLAGS {
    // flags going to packet[17]
    H8_3D_FLAG_FLIP      = 0x01,
    H8_3D_FLAG_RATE_MID  = 0x02,
    H8_3D_FLAG_RATE_HIGH = 0x04,
    H8_3D_FLAG_LIGTH	 = 0x08, // Light on H22
    H8_3D_FLAG_HEADLESS  = 0x10, // RTH + headless on H8, headless on JJRC H20, RTH on H22
    H8_3D_FLAG_RTH		 = 0x20, // 360 flip mode on H8 3D and H22, RTH on JJRC H20
};

enum H8_3D_FLAGS_2 {
    // flags going to packet[18]
    H8_3D_FLAG_VIDEO      = 0x80,
    H8_3D_FLAG_PICTURE    = 0x40,
    H8_3D_FLAG_CALIBRATE1 = 0x20,  // H8 3D acc calibration, H20,H20H headless calib
    H8_3D_FLAG_CALIBRATE2 = 0x10,  // H11D, H20, H20H acc calibration
    H8_3D_FLAG_CAM_DN     = 0x08,
    H8_3D_FLAG_CAM_UP     = 0x04,
};

static void __attribute__((unused)) H8_3D_send_packet(uint8_t bind)
{
	if(sub_protocol==H20H)
		packet[0] = 0x14;
	else // H8_3D, H20MINI, H30MINI
		packet[0] = 0x13;

	packet[1] = rx_tx_addr[0]; 
	packet[2] = rx_tx_addr[1];
	packet[3] = rx_tx_addr[2];
	packet[4] = rx_tx_addr[3];
	packet[8] = rx_tx_addr[0]+rx_tx_addr[1]+rx_tx_addr[2]+rx_tx_addr[3]; // txid checksum
	memset(&packet[9], 0, 10);
	if (bind)
	{    
		packet[5] = 0x00;
		packet[6] = 0x00;
		packet[7] = 0x01;
	}
	else
	{
		packet[5] = hopping_frequency_no;
		packet[7] = 0x03;

		rudder = convert_channel_16b_limit(RUDDER,0x44,0xBC);			// yaw right : 0x80 (neutral) - 0xBC (right)
		if(sub_protocol!=H20H)
		{ // H8_3D, H20MINI, H30MINI
			packet[6] = 0x08;
			packet[9] = convert_channel_8b(THROTTLE);					// throttle  : 0x00 - 0xFF
			packet[15] = 0x20;	// trims
			packet[16] = 0x20;	// trims
			if (rudder<=0x80)
				rudder=0x80-rudder;										// yaw left  : 0x00 (neutral) - 0x3C (left)
			if(rudder==0x01 || rudder==0x81)
				rudder=0x00;	// Small deadband
		}
		else
		{ //H20H
			packet[6] = hopping_frequency_no == 0 ? 8 - packet_count : 16 - packet_count;
			packet[9] = convert_channel_16b_limit(THROTTLE, 0x43, 0xBB);	// throttle : 0x43 - 0x7F - 0xBB
			packet[15]= 0x40;	// trims
			packet[16]= 0x40;	// trims
			rudder--;													// rudder : 0x43 - 0x7F - 0xBB
			if (rudder>=0x7F-1 && rudder<=0x7F+1)
				rudder=0x7F;	// Small deadband
		}
		packet[10] = rudder;
		packet[11] = convert_channel_16b_limit(ELEVATOR, 0x43, 0xBB);	// elevator : 0x43 - 0x7F - 0xBB
		packet[12] = convert_channel_16b_limit(AILERON,  0x43, 0xBB);	// aileron  : 0x43 - 0x7F - 0xBB
		// neutral trims
		packet[13] = 0x20;
		packet[14] = 0x20;
		// flags
		packet[17] = 					  H8_3D_FLAG_RATE_HIGH
					| GET_FLAG(CH5_SW,H8_3D_FLAG_FLIP)
					| GET_FLAG(CH6_SW,H8_3D_FLAG_LIGTH) //H22 light
					| GET_FLAG(CH9_SW,H8_3D_FLAG_HEADLESS)
					| GET_FLAG(CH10_SW,H8_3D_FLAG_RTH); // 180/360 flip mode on H8 3D
		packet[18] =  GET_FLAG(CH7_SW,H8_3D_FLAG_PICTURE)
					| GET_FLAG(CH8_SW,H8_3D_FLAG_VIDEO)
					| GET_FLAG(CH11_SW,H8_3D_FLAG_CALIBRATE1)
					| GET_FLAG(CH12_SW,H8_3D_FLAG_CALIBRATE2);
		if(Channel_data[CH13]<CHANNEL_MIN_COMMAND)
			packet[18] |= H8_3D_FLAG_CAM_DN;
		if(CH13_SW)
			packet[18] |= H8_3D_FLAG_CAM_UP;
	}
	uint8_t  sum = packet[9];
	for (uint8_t i=10; i < H8_3D_PACKET_SIZE-1; i++)
		sum += packet[i];
	packet[19] = sum; // data checksum
	
	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | _BV(NRF24L01_00_PWR_UP));
	if(sub_protocol!=H20H)
	{ // H8_3D, H20MINI, H30MINI
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? hopping_frequency[0] : hopping_frequency[hopping_frequency_no++]);
		hopping_frequency_no %= H8_3D_RF_NUM_CHANNELS;
	}
	else
	{ //H20H
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? H20H_BIND_RF : hopping_frequency[packet_count>>3]);  
		if(!bind)
		{
			packet_count++;
			if(packet_count>15)
			{
				packet_count = 0;
				hopping_frequency_no = 0;
			}
			else
				if(packet_count > 7)
					hopping_frequency_no = 1;
		}
	}
	
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, H8_3D_PACKET_SIZE);

	NRF24L01_SetPower();	// Set tx_power
}

static void __attribute__((unused)) H8_3D_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
	if(sub_protocol==H20H)
		XN297_SetTXAddr((uint8_t *)"\xEE\xDD\xCC\xBB\x11", 5);
	else // H8_3D, H20MINI, H30MINI
		XN297_SetTXAddr((uint8_t *)"\xC4\x57\x09\x65\x21", 5);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower();
}

uint16_t H8_3D_callback()
{
	if(IS_BIND_DONE)
		H8_3D_send_packet(0);
	else
	{
		if (bind_counter == 0)
		{
			BIND_DONE;
			packet_count=0;
		}
		else
		{
			H8_3D_send_packet(1);
			bind_counter--;
		}
	}
	return	packet_period;
}

// captured from H20H stock transmitters
const uint8_t PROGMEM h20h_tx_rf_map[3][6] = {{/*ID*/0x83, 0x3c, 0x60, 0x00, /*RF*/0x47, 0x3e},
											  {/*ID*/0x5c, 0x2b, 0x60, 0x00, /*RF*/0x4a, 0x3c},
											  {/*ID*/0x57, 0x07, 0x00, 0x00, /*RF*/0x41, 0x48} };
// captured from H20 Mini / H30 Mini stock transmitters
const uint8_t PROGMEM h20mini_tx_rf_map[4][8] =  {{/*ID*/0xb4, 0xbb, 0x09, 0x00, /*RF*/0x3e, 0x45, 0x47, 0x4a},
												  {/*ID*/0x94, 0x9d, 0x0b, 0x00, /*RF*/0x3e, 0x43, 0x49, 0x4a},
												  {/*ID*/0xd1, 0xd0, 0x00, 0x00, /*RF*/0x3f, 0x42, 0x46, 0x4a},
												  {/*ID*/0xcb, 0xcd, 0x04, 0x00, /*RF*/0x41, 0x43, 0x46, 0x4a}};
static void __attribute__((unused)) H8_3D_initialize_txid()
{
	uint8_t id_num=rx_tx_addr[4];
	switch(sub_protocol)
	{
		case H8_3D:
            for(uint8_t i=0; i<4; i++)
                hopping_frequency[i] = 6 + (0x0f*i) + (((rx_tx_addr[i] >> 4) + (rx_tx_addr[i] & 0x0f)) % 0x0f);
			break;
		case H20H:
            id_num%=3; // 3 different IDs
			for(uint8_t i=0; i<4; i++)
			{
				rx_tx_addr[i] = pgm_read_byte_near(&h20h_tx_rf_map[id_num][i]);
				if(i<2)
					hopping_frequency[i] = pgm_read_byte_near(&h20h_tx_rf_map[id_num][i+4]);
			}
			break;
		case H20MINI:
		case H30MINI:
            id_num%=4; // 4 different IDs
			for(uint8_t i=0; i<4; i++)
			{
				rx_tx_addr[i] = pgm_read_byte_near(&h20mini_tx_rf_map[id_num][i]);
				hopping_frequency[i] = pgm_read_byte_near(&h20mini_tx_rf_map[id_num][i+4]);
			}
			break;
	}
}

uint16_t initH8_3D(void)
{
	BIND_IN_PROGRESS;	// autobind protocol
    bind_counter = H8_3D_BIND_COUNT;
	H8_3D_initialize_txid();
	H8_3D_init();
	switch(sub_protocol)
	{
        case H8_3D:
			packet_period=H8_3D_PACKET_PERIOD;
			break;
		case H20H:
			packet_period=H20H_PACKET_PERIOD;
			break;
		case H20MINI:
		case H30MINI:
			packet_period=H20MINI_PACKET_PERIOD;
			break;
	}
	return	H8_3D_INITIAL_WAIT;
}

#endif
