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

#if defined(DSM_RX_CYRF6936_INO)

#include "iface_cyrf6936.h"

//#define DSM_DEBUG_RF
//#define DSM_DEBUG_CH

uint8_t DSM_rx_type;

enum {
	DSM_RX_BIND1 = 0,
	DSM_RX_BIND2,
	DSM_RX_DATA_PREP,
	DSM2_RX_SCAN,
	DSM_RX_DATA_CH1,
	DSM_RX_DATA_CH2,
};

static void __attribute__((unused)) DSM_Rx_init()
{
	DSM_cyrf_config();
	rx_disable_lna = IS_POWER_FLAG_on;
	if(IS_BIND_IN_PROGRESS)
	{
		//64 SDR Mode is configured so only the 8 first values are needed but need to write 16 values...
		uint8_t code[16];
		DSM_read_code(code,0,8,8);
		CYRF_ConfigDataCode(code, 16);
		CYRF_ConfigRFChannel(1);
		CYRF_SetTxRxMode(RX_EN);									// Force end state read
		CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);					// Prepare to receive
	}
	else
	{
		DSM_cyrf_configdata();
		CYRF_WriteRegister(CYRF_06_RX_CFG, rx_disable_lna ? 0x0A:0x4A);	// AGC disabled, LNA disabled/enabled, Attenuator disabled, RX override enabled, Fast turn mode enabled, RX is 1MHz below TX
	}
}

uint16_t convert_channel_DSM_nolimit(int32_t val)
{
	val=(val-0x150)*(CHANNEL_MAX_100-CHANNEL_MIN_100)/(0x6B0-0x150)+CHANNEL_MIN_100;
	if(val<0)
		val=0;
	else
		if(val>2047)
			val=2047;
	return (uint16_t)val;
}

static uint8_t __attribute__((unused)) DSM_Rx_check_packet()
{
 	uint8_t rx_status=CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
	if((rx_status & 0x03) == 0x02)  									// RXC=1, RXE=0 then 2nd check is required (debouncing)
		rx_status |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
	if((rx_status & 0x07) == 0x02)
	{ // data received with no errors
		len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
		#ifdef DSM_DEBUG_RF
			debugln("l=%d",len);
		#endif
		if(len>=2 && len<=16)
		{
			// Read packet
			CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);		// Need to set RXOW before data read
			CYRF_ReadDataPacketLen(packet, len);

			// Check packet ID
			if ((DSM_rx_type&0x80) == 0)
			{//DSM2
				packet[0] ^= 0xff;
				packet[1] ^= 0xff;
			}
			#ifdef DSM_DEBUG_CH
				for(uint8_t i=0;i<len;i++)
					debug("%02X ",packet[i]);
				debugln("");
			#endif
			if(packet[0] == cyrfmfg_id[2] && packet[1] == cyrfmfg_id[3])
				return 0x02;										// Packet ok
		}
		return 0x00;												// Wrong size or ID -> nothing received
	}
	return rx_status;												// Return error code
}

static void __attribute__((unused)) DSM_Rx_build_telemetry_packet()
{
	uint8_t nbr_bits = 11;
	if((DSM_rx_type&0xF0) == 0x00)
		nbr_bits=10;												// Only DSM_22 is using a resolution of 1024

	// Use packet length to calculate the number of channels
	len -= 2;														// Remove header length
	len >>= 1;														// Channels are on 2 bytes
	if(len==0) return;												// No channels...

	// Extract channels
	uint8_t idx;
	for (uint8_t i = 0; i < len; i++)
	{
		uint16_t value=(packet[i*2+2]<<8) | packet[i*2+3];
		if(value!=0xFFFF)
		{
			idx=(value&0x7FFF)>>nbr_bits;							// retrieve channel index
			#ifdef DSM_DEBUG_CH
				debugln("i=%d,v=%d,u=%X",idx,value&0x7FF,value&0x8000);
			#endif
			if(idx<13)
			{
				if(nbr_bits==10) value <<= 1;						// switch to 11 bits
				value &= 0x7FF;
				rx_rc_chan[CH_TAER[idx]]=convert_channel_DSM_nolimit(value);
			}
		}
	}

	// Buid telemetry packet
	idx=0;
	packet_in[idx++] = RX_LQI;
	packet_in[idx++] = RX_LQI;
	packet_in[idx++] = 0;  											// start channel
	packet_in[idx++] = 12; 											// number of channels in packet

	// Pack channels
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	for (uint8_t i = 0; i < 12; i++)
	{
		bits |= ((uint32_t)rx_rc_chan[i]) << bitsavailable;
		bitsavailable += 11;
		while (bitsavailable >= 8)
		{
			packet_in[idx++] = bits & 0xff;
			bits >>= 8;
			bitsavailable -= 8;
		}
	}
	if(bitsavailable)
		packet_in[idx++] = bits & 0xff;
	// Send telemetry
	telemetry_link = 1;
}

static bool __attribute__((unused)) DSM_Rx_bind_check_validity()
{
	uint16_t sum = 384 - 0x10;//
	for(uint8_t i = 0; i < 8; i++)
		sum += packet_in[i];
	if( packet_in[8] != (sum>>8) || packet_in[9] != (sum&0xFF))		//Checksum
		return false;
	for(uint8_t i = 8; i < 14; i++)
		sum += packet_in[i];
	if( packet_in[14] != (sum>>8) || packet_in[15] != (sum&0xFF))	//Checksum
		return false;
	if(memcmp(packet_in,packet_in+4,4))								//Check ID
		return false;
	return true;
}

static void __attribute__((unused)) DSM_Rx_build_bind_packet()
{
	uint16_t sum = 384 - 0x10;//
	packet[0] = 0xff ^ cyrfmfg_id[0];								// ID
	packet[1] = 0xff ^ cyrfmfg_id[1];
	packet[2] = 0xff ^ cyrfmfg_id[2];
	packet[3] = 0xff ^ cyrfmfg_id[3];
	packet[4] = 0x01;												// RX version
	packet[5] = num_ch;												// Number of channels
	packet[6] = DSM_rx_type;										// DSM type, let's just send back whatever the TX gave us...
	packet[7] = 0x00;												// Unknown
	for(uint8_t i = 0; i < 8; i++)
		sum += packet[i];
	packet[8] = sum >> 8;
	packet[9] = sum & 0xff;
}

static void __attribute__((unused)) DSM_abort_channel_rx(uint8_t ch)
{
	CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);						// Abort RX operation
	CYRF_SetTxRxMode(IS_POWER_FLAG_on ? TXRX_OFF:RX_EN);			// Force end state read
	if (rx_disable_lna != IS_POWER_FLAG_on && IS_BIND_DONE)
	{
		rx_disable_lna = IS_POWER_FLAG_on;
		CYRF_WriteRegister(CYRF_06_RX_CFG, rx_disable_lna ? 0x0A:0x4A);	// AGC disabled, LNA disabled/enabled, Attenuator disabled, RX override enabled, Fast turn mode enabled, RX is 1MHz below TX
	}
	if(ch&0x02) DSM_set_sop_data_crc(true ,DSM_rx_type&0x80);		// Set sop data,crc seed and rf channel using CH1, DSM2/X
	if(ch&0x01) DSM_set_sop_data_crc(false,DSM_rx_type&0x80);		// Set sop data,crc seed and rf channel using CH1, DSM2/X
	CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);						// Clear abort RX operation
	CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x83);						// Prepare to receive
}

uint16_t DSM_Rx_callback()
{
	uint8_t rx_status;
	static uint8_t read_retry=0;

	switch (phase)
	{
		case DSM_RX_BIND1:
			if(IS_BIND_DONE)										// Abort bind
			{
				phase = DSM_RX_DATA_PREP;
				break;
			}
			if(packet_count==0)
				read_retry=0;
			//Check received data
			rx_status = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_status & 0x03) == 0x02)  						// RXC=1, RXE=0 then 2nd check is required (debouncing)
				rx_status |= CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
			if((rx_status & 0x07) == 0x02)
			{ // data received with no errors
				CYRF_WriteRegister(CYRF_07_RX_IRQ_STATUS, 0x80);	// Need to set RXOW before data read
				len=CYRF_ReadRegister(CYRF_09_RX_COUNT);
				debugln("RX:%d, CH:%d",len,hopping_frequency_no);
				if(len==16)
				{
					CYRF_ReadDataPacketLen(packet_in, 16);
					if(DSM_Rx_bind_check_validity())
					{
						// store tx info into eeprom
						uint16_t temp = DSM_RX_EEPROM_OFFSET;
						debug("ID=");
						for(uint8_t i=0;i<4;i++)
						{
							cyrfmfg_id[i]=packet_in[i]^0xFF;
							eeprom_write_byte((EE_ADDR)temp++, cyrfmfg_id[i]);
							debug(" %02X", cyrfmfg_id[i]);
						}
						// check num_ch
						num_ch=packet_in[11];
						if(num_ch>12) num_ch=12;
						//check DSM_rx_type
						/*packet[12]     1 byte -> max DSM type allowed:
							0x01 => 22ms 1024 DSM2 1 packet => number of channels is <8
							0x02 => 22ms 1024 DSM2 2 packets => either a number of channel >7
							0x12 => 11ms 2048 DSM2 2 packets => can be any number of channels
							0xA2 => 22ms 2048 DSMX 1 packet => number of channels is <8
							0xB2 => 11ms 2048 DSMX => can be any number of channels
							(0x01 or 0xA2) and num_ch < 7 => 22ms else 11ms
							&0x80 => false=DSM2, true=DSMX
							&0xF0 => false=1024, true=2048 */
						DSM_rx_type=packet_in[12];
						switch(DSM_rx_type)
						{
							case 0x01:
								if(num_ch>7) DSM_rx_type = 0x02;	// Can't be 0x01 with this number of channels
								break;
							case 0xA2:
								if(num_ch>7) DSM_rx_type = 0xB2;	// Can't be 0xA2 with this number of channels
								break;
							case 0x02:
							case 0x12:
							case 0xB2:
								break;
							default:								// Unknown type, default to DSMX 11ms
								DSM_rx_type = 0xB2;
								break;
						}
						eeprom_write_byte((EE_ADDR)temp, DSM_rx_type);
						debugln(", num_ch=%d, type=%02X",num_ch, DSM_rx_type);
						CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x20);	// Abort RX operation
						CYRF_SetTxRxMode(TX_EN);					// Force end state TX
						CYRF_ConfigDataCode((const uint8_t *)"\x98\x88\x1B\xE4\x30\x79\x03\x84", 16);
						CYRF_WriteRegister(CYRF_29_RX_ABORT, 0x00);	// Clear abort RX
						DSM_Rx_build_bind_packet();
						bind_counter=500;
						phase++;									// DSM_RX_BIND2;
						return 1000;
					}
				}
				DSM_abort_channel_rx(0);							// Abort RX operation and receive
				if(read_retry==0)
					read_retry=8;
			}
			else
				if(rx_status & 0x02)								// RX error
					DSM_abort_channel_rx(0);						// Abort RX operation and receive
			packet_count++;
			if(packet_count>12)
			{
				packet_count=1;
				if(read_retry)
					read_retry--;
				if(read_retry==0)
				{
					packet_count=0;
					hopping_frequency_no++;								// Change channel
					hopping_frequency_no %= 0x50;
					hopping_frequency_no |= 0x01;						// Odd channels only
					CYRF_ConfigRFChannel(hopping_frequency_no);
					DSM_abort_channel_rx(0);							// Abort RX operation and receive
				}
			}
			return 1000;
		case DSM_RX_BIND2:
			//Transmit settings back
			CYRF_WriteDataPacketLen(packet,10);						// Send packet
			if(bind_counter--==0)
			{
				BIND_DONE;
				phase++;											// DSM_RX_DATA_PREP
			}
			break;
		case DSM_RX_DATA_PREP:
			hopping_frequency_no = 0;
			read_retry=0;
			rx_data_started = false;
			pps_counter = 0;
			RX_LQI = 100;
			DSM_cyrf_configdata();
			pps_timer=millis();
			sop_col = (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + 2) & 0x07;
			seed = (cyrfmfg_id[0] << 8) + cyrfmfg_id[1];
			if(DSM_rx_type&0x80)
			{ // DSMX
				DSM_calc_dsmx_channel();							// Build hop table
				DSM_abort_channel_rx(1);							// Abort RX operation, set sop&data&seed&rf using CH1, DSM2/X and receive
				phase=DSM_RX_DATA_CH1;
			}
			else
			{ // DSM2
				rf_ch_num=0;
				hopping_frequency_no = 0;
				hopping_frequency[0] = 3;
				hopping_frequency[1] = 0;
				DSM_abort_channel_rx(1);							// Abort RX operation, set sop&data&seed&rf using CH1, DSM2/X and receive
				phase=DSM2_RX_SCAN;
			}
			break;
		case DSM2_RX_SCAN:											// Scan for DSM2 frequencies
			//Received something ?
			rx_status = DSM_Rx_check_packet();
			if(rx_status == 0x02)
			{ // data received with no errors
				debugln("CH%d:Found %d",rf_ch_num+1,hopping_frequency[rf_ch_num]);
				read_retry=0;
				if(rf_ch_num)
				{													// Both CH1 and CH2 found
					read_retry=0;
					hopping_frequency_no=0;
					DSM_abort_channel_rx(1);						// Abort RX operation, set sop&data&seed&rf using CH1, DSM2/X and receive
					pps_timer=millis();
					phase++;										// DSM_RX_DATA_CH1
				}
				else
				{
					rf_ch_num++;									// CH1 found, scan for CH2
					hopping_frequency_no = 1;
					if(hopping_frequency[1] < 3)					// If no CH2 keep then restart from current
						hopping_frequency[1]=hopping_frequency[0]+1;
					DSM_abort_channel_rx(2);						// Abort RX operation, set sop&data&seed&rf using CH2, DSM2/X and receive
				}
			}
			else
			{
				read_retry++;
				if(read_retry>50)									// After 50ms
				{ // Try next channel
					debugln("CH%d:Next channel",rf_ch_num+1);
					read_retry=0;
					hopping_frequency_no = rf_ch_num;
					hopping_frequency[rf_ch_num]++;
					if(hopping_frequency[rf_ch_num] > 73) hopping_frequency[rf_ch_num] = 3;
					DSM_abort_channel_rx(rf_ch_num+1);				// Abort RX operation, set sop&data&seed&rf using CH1/2, DSM2/X and receive
				}
				else if(rx_status & 0x02)
				{ // data received with errors
					if((rx_status & 0x01) && rf_ch_num==0)
						hopping_frequency[1] = hopping_frequency[0];// Might be CH2 since it's a CRC error so keep it
					debugln("CH%d:RX error",rf_ch_num+1);
					DSM_abort_channel_rx(0);						// Abort RX operation and receive
				}
			}
			return 1000;
		case DSM_RX_DATA_CH1:
			//Packets per second
			if (millis() - pps_timer >= 1000)
			{//182pps @11ms, 91pps @22ms
				pps_timer = millis();
				if(DSM_rx_type!=0xA2 && DSM_rx_type!=0x01)			// if 11ms
					pps_counter >>=1;								// then /2
				debugln("%d pps", pps_counter);
				RX_LQI = pps_counter;								// max=91pps
				pps_counter = 0;
			}
			//Received something ?
			rx_status = DSM_Rx_check_packet();
			if(rx_status == 0x02)
			{ // data received with no errors
				#ifdef DSM_DEBUG_RF
					debugln("CH1:RX");
				#endif
				DSM_Rx_build_telemetry_packet();
				rx_data_started = true;
				pps_counter++;
				DSM_abort_channel_rx(2);							// Abort RX operation, set sop&data&seed&rf using CH2, DSM2/X and receive
				phase++;
				return 5000;
			}
			else
			{
				read_retry++;
				if(rx_data_started && read_retry>6)					// After 6*500=3ms
				{ // skip to CH2
					#ifdef DSM_DEBUG_RF
						debugln("CH1:Skip to CH2");
					#endif
					DSM_abort_channel_rx(2);						// Abort RX operation, set sop&data&seed&rf using CH2, DSM2/X and receive
					phase++;
					return 4000;
				}
				if(rx_data_started && RX_LQI==0)
				{  // communication lost
					#ifdef DSM_DEBUG_RF
						debugln("CH1:Restart...");
					#endif
					phase=DSM_RX_DATA_PREP;
					return 1000;
				}
				if(read_retry>250)
				{ // move to next RF channel
					#ifdef DSM_DEBUG_RF
						debugln("CH1:Scan");
					#endif
					DSM_abort_channel_rx(3);						// Abort RX operation, set sop&data&seed&rf using CH2 then CH1, DSM2/X and receive
					read_retry=0;
				}
				else if(rx_status & 0x02)
				{ // data received with errors
					#ifdef DSM_DEBUG_RF
						debugln("CH1:RX error %02X",rx_status);
					#endif
					DSM_abort_channel_rx(0);						// Abort RX operation and receive
				}
			}
			return 500;
		case DSM_RX_DATA_CH2:
			rx_status = DSM_Rx_check_packet();
			if(rx_status == 0x02)
			{ // data received with no errors
				#ifdef DSM_DEBUG_RF
					debugln("CH2:RX");
				#endif
				DSM_Rx_build_telemetry_packet();
				pps_counter++;
			}
			#ifdef DSM_DEBUG_RF
				else
					debugln("CH2:No RX");
			#endif
			DSM_abort_channel_rx(1);								// Abort RX operation, set sop&data&seed&rf using CH1, DSM2/X and receive
			read_retry=0;
			phase=DSM_RX_DATA_CH1;
			if(DSM_rx_type==0xA2) //|| DSM_rx_type==0x01 -> not needed for DSM2 since we are ok to listen even if there will be nothing
				return 15000;										//22ms
			else
				return 4000;										//11ms
	}
	return 10000;
}

uint16_t initDSM_Rx()
{
	DSM_Rx_init();
	hopping_frequency_no = 0;
	
	if (IS_BIND_IN_PROGRESS)
	{
		packet_count=0;
		phase = DSM_RX_BIND1;
	}
	else
	{
		uint16_t temp = DSM_RX_EEPROM_OFFSET;
		debug("ID=");
		for(uint8_t i=0;i<4;i++)
		{
			cyrfmfg_id[i]=eeprom_read_byte((EE_ADDR)temp++);
			debug(" %02X", cyrfmfg_id[i]);
		}
		DSM_rx_type=eeprom_read_byte((EE_ADDR)temp);
		debugln(", type=%02X", DSM_rx_type);
		phase = DSM_RX_DATA_PREP;
	}
	return 15000;
}

#endif
