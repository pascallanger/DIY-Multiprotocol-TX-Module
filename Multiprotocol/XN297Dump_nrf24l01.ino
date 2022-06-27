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

// sub_protocol: 0=250Kbps, 1=1Mbps, 2=2Mbps. Other values default to 1Mbps.
// RX_num = address length 3 or 4 or 5. Other values default to 5.
// option = RF channel number 0..84 and -1 = scan all channels. Other values default to RF channel 0.

#ifdef XN297DUMP_NRF24L01_INO

#include "iface_xn297.h"

// Parameters which can be modified
#define XN297DUMP_PERIOD_SCAN		50000 	// 25000
#define XN297DUMP_MAX_RF_CHANNEL	127		// Default 84

// Do not touch from there
#define XN297DUMP_INITIAL_WAIT		500
#define XN297DUMP_MAX_PACKET_LEN	32
#define XN297DUMP_CRC_LENGTH		2

uint8_t address_length;
uint16_t timeH=0;
boolean scramble;
boolean enhanced;
boolean ack;
uint8_t pid;
uint8_t bitrate;

static void __attribute__((unused)) XN297Dump_RF_init()
{
	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(RX_EN);

	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);			// 3 bytes RX/TX address
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (uint8_t*)"\x55\x0F\x71", 3);	// set up RX address to xn297 preamble
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, XN297DUMP_MAX_PACKET_LEN);	// Enable rx pipe 0

	debug("XN297 dump, address length=%d, bitrate=",address_length);
	switch(bitrate)
	{
		case XN297DUMP_250K:
			NRF24L01_SetBitrate(NRF24L01_BR_250K);
			debugln("250K");
			break;
		case XN297DUMP_2M:
			NRF24L01_SetBitrate(NRF24L01_BR_2M);
			debugln("2M");
			break;
		default:
			NRF24L01_SetBitrate(NRF24L01_BR_1M);
			debugln("1M");
			break;

	}
}

extern const uint8_t xn297_scramble[];
extern const uint16_t PROGMEM xn297_crc_xorout_scrambled[];
extern const uint16_t PROGMEM xn297_crc_xorout[];
extern const uint16_t PROGMEM xn297_crc_xorout_scrambled_enhanced[];
extern const uint16_t xn297_crc_xorout_enhanced[];

static boolean __attribute__((unused)) XN297Dump_process_packet(void)
{
	uint16_t crcxored;
	uint8_t packet_sc[XN297DUMP_MAX_PACKET_LEN], packet_un[XN297DUMP_MAX_PACKET_LEN];
	enhanced=false;
	// init crc
	crc = 0xb5d2;
	
	/*debug("P: 71 0F 55 ");
	for(uint8_t i=0; i<XN297DUMP_MAX_PACKET_LEN; i++)
		debug("%02X ",packet[i]);
	debugln("");*/
	//Try normal payload
	// address
	for (uint8_t i = 0; i < address_length; i++)
	{
		crc16_update( packet[i], 8);
		packet_un[address_length-1-i]=packet[i];
		packet_sc[address_length-1-i]=packet[i] ^ xn297_scramble[i];
	}
	
	// payload
	for (uint8_t i = address_length; i < XN297DUMP_MAX_PACKET_LEN-XN297DUMP_CRC_LENGTH; i++)
	{
		crc16_update( packet[i], 8);
		packet_sc[i] = bit_reverse(packet[i]^xn297_scramble[i]);
		packet_un[i] = bit_reverse(packet[i]);
		// check crc
		crcxored = crc ^ pgm_read_word(&xn297_crc_xorout[i+1 - 3]);
		if( (crcxored >> 8) == packet[i + 1] && (crcxored & 0xff) == packet[i + 2])
		{
			packet_length=i+1;
			memcpy(packet,packet_un,packet_length);
			scramble=false;
			return true;
		}
		crcxored = crc ^ pgm_read_word(&xn297_crc_xorout_scrambled[i+1 - 3]);
		if( (crcxored >> 8) == packet[i + 1] && (crcxored & 0xff) == packet[i + 2])
		{
			packet_length=i+1;
			memcpy(packet,packet_sc,packet_length);
			scramble=true;
			return true;
		}
	}

	//Try enhanced payload
	uint16_t crc_save = 0xb5d2;
	packet_length=0;
	for (uint8_t i = 0; i < XN297DUMP_MAX_PACKET_LEN-XN297DUMP_CRC_LENGTH; i++) 
	{
		packet_sc[i]=packet[i]^xn297_scramble[i];
		crc = crc_save;
		crc16_update( packet[i], 8);
		crc_save = crc;
		crc16_update( packet[i+1] & 0xC0, 2);
		crcxored=(packet[i+1]<<10)|(packet[i+2]<<2)|(packet[i+3]>>6) ;
		if(i>=3)
		{
			if((crc ^ pgm_read_word(&xn297_crc_xorout_scrambled_enhanced[i - 3])) == crcxored)
			{ // Found a valid CRC for the enhanced payload mode
				packet_length=i;
				scramble=true;
				i++;
				packet_sc[i]=packet[i]^xn297_scramble[i];
				memcpy(packet_un,packet_sc,packet_length+2); // unscramble packet
				break;
			}
			if((crc ^ pgm_read_word(&xn297_crc_xorout_enhanced[i - 3])) == crcxored)
			{ // Found a valid CRC for the enhanced payload mode
				packet_length=i;
				scramble=false;
				memcpy(packet_un,packet,packet_length+2); 	// packet is unscrambled
				break;
			}
		}
	}
	if(packet_length!=0)
	{ // Found a valid CRC for the enhanced payload mode
		enhanced=true;
		//check selected address length
		if((packet_un[address_length]>>1)!=packet_length-address_length)
		{
			for(uint8_t i=3;i<=5;i++)
				if((packet_un[i]>>1)==packet_length-i)
					address_length=i;
			debugln("Detected wrong address length, using %d intead", address_length );
		}
		pid=((packet_un[address_length]&0x01)<<1)|(packet_un[address_length+1]>>7);
		ack=(packet_un[address_length+1]>>6)&0x01;
		// address
		for (uint8_t i = 0; i < address_length; i++)
			packet[address_length-1-i]=packet_un[i];
		// payload
		for (uint8_t i = address_length; i < packet_length; i++)
			packet[i] = bit_reverse((packet_un[i+1]<<2)|(packet_un[i+2]>>6));
		return true;
	}

	return false;
}

static void __attribute__((unused)) XN297Dump_overflow()
{
	if(TIMER2_BASE->SR & TIMER_SR_UIF)
	{ // timer overflow
		timeH++;
		TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_UIF;	// Clear Timer2 overflow flag
	}
}
static uint16_t XN297Dump_callback()
{
	static uint32_t time=0,*time_rf;

	//!!!Blocking mode protocol!!!
	TX_MAIN_PAUSE_off;
	tx_resume();
	while(1)
	{
		if(sub_protocol<XN297DUMP_AUTO)
		{
			if(option==0xFF && bind_counter>XN297DUMP_PERIOD_SCAN)
			{	// Scan frequencies
				hopping_frequency_no++;
				bind_counter=0;
			}
			if(hopping_frequency_no!=rf_ch_num)
			{	// Channel has changed
				if(hopping_frequency_no>XN297DUMP_MAX_RF_CHANNEL)
					hopping_frequency_no=0;	// Invalid channel 0 by default
				rf_ch_num=hopping_frequency_no;
				debugln("Channel=%d,0x%02X",hopping_frequency_no,hopping_frequency_no);
				NRF24L01_WriteReg(NRF24L01_05_RF_CH,hopping_frequency_no);
				// switch to RX mode
				NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(RX_EN);
				NRF24L01_FlushRx();
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to RX mode and disable CRC
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
				phase=0;				// init timer
			}
			XN297Dump_overflow();
			
			if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
			{ // RX fifo data ready
				if(NRF24L01_ReadReg(NRF24L01_09_CD) || option != 0xFF)
				{
					NRF24L01_ReadPayload(packet,XN297DUMP_MAX_PACKET_LEN);
					XN297Dump_overflow();
					uint16_t timeL=TCNT1;
					if(TIMER2_BASE->SR & TIMER_SR_UIF)
					{//timer just rolled over...
						XN297Dump_overflow();
						timeL=0;
					}
					if((phase&0x01)==0)
					{
						phase=1;
						time=0;
					}
					else
						time=(timeH<<16)+timeL-time;
					if(XN297Dump_process_packet())
					{ // valid crc found
						debug("RX: %5luus C=%d ", time>>1 , hopping_frequency_no);
						time=(timeH<<16)+timeL;
						if(enhanced)
						{
							debug("Enhanced ");
							debug("pid=%d ",pid);
							if(ack) debug("ack ");
						}
						debug("S=%c A=",scramble?'Y':'N');
						for(uint8_t i=0; i<address_length; i++)
						{
							debug(" %02X",packet[i]);
						}
						debug(" P(%d)=",packet_length-address_length);
						for(uint8_t i=address_length; i<packet_length; i++)
						{
							debug(" %02X",packet[i]);
						}
						debugln("");
					}
					else
					{
						debugln("RX: %5luus C=%d Bad CRC", time>>1 , hopping_frequency_no);
					}
				}
				
				XN297Dump_overflow();
				// restart RX mode
				NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(RX_EN);
				NRF24L01_FlushRx();
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to RX mode and disable CRC
												| (1 << NRF24L01_00_CRCO)
												| (1 << NRF24L01_00_PWR_UP)
												| (1 << NRF24L01_00_PRIM_RX));
				XN297Dump_overflow();
			}
		}
		else if(sub_protocol==XN297DUMP_AUTO)
		{
			switch(phase)
			{
				case 0:
					debugln("------------------------");
					debugln("Detecting XN297 packets.");
					XN297Dump_RF_init();
					debug("Trying RF channel: 0");
					hopping_frequency_no=0;
					bitrate=0;
					phase++;
					break;
				case 1:
					if(bind_counter>XN297DUMP_PERIOD_SCAN)
					{	// Scan frequencies
						hopping_frequency_no++;
						bind_counter=0;
						if(hopping_frequency_no>XN297DUMP_MAX_RF_CHANNEL)
						{
							hopping_frequency_no=0;
							bitrate++;
							bitrate%=3;
							debugln("");
							XN297Dump_RF_init();
							debug("Trying RF channel: 0");
						}
						if(hopping_frequency_no)
							debug(",%d",hopping_frequency_no);
						NRF24L01_WriteReg(NRF24L01_05_RF_CH,hopping_frequency_no);
						// switch to RX mode
						NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
						NRF24L01_SetTxRxMode(TXRX_OFF);
						NRF24L01_SetTxRxMode(RX_EN);
						NRF24L01_FlushRx();
						NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to RX mode and disable CRC
														| (1 << NRF24L01_00_CRCO)
														| (1 << NRF24L01_00_PWR_UP)
														| (1 << NRF24L01_00_PRIM_RX));
					}
					if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
					{ // RX fifo data ready
						if(NRF24L01_ReadReg(NRF24L01_09_CD))
						{
							NRF24L01_ReadPayload(packet,XN297DUMP_MAX_PACKET_LEN);
							if(XN297Dump_process_packet())
							{ // valid crc found
								debug("\r\n\r\nPacket detected: bitrate=");
								switch(bitrate)
								{
									case XN297DUMP_250K:
										XN297_Configure(XN297_CRCEN, scramble?XN297_SCRAMBLED:XN297_UNSCRAMBLED, XN297_250K, true);
										debug("250K");
										break;
									case XN297DUMP_2M:
										XN297_Configure(XN297_CRCEN, scramble?XN297_SCRAMBLED:XN297_UNSCRAMBLED, XN297_1M);
										NRF24L01_SetBitrate(NRF24L01_BR_2M);
										debug("2M");
										break;
									default:
										XN297_Configure(XN297_CRCEN, scramble?XN297_SCRAMBLED:XN297_UNSCRAMBLED, XN297_1M);
										debug("1M");
										break;

								}
								debug(" C=%d ", hopping_frequency_no);
								if(enhanced)
								{
									debug("Enhanced ");
									debug("pid=%d ",pid);
									if(ack) debug("ack ");
								}
								debug("S=%c A=",scramble?'Y':'N');
								for(uint8_t i=0; i<address_length; i++)
								{
									debug(" %02X",packet[i]);
									rx_tx_addr[i]=packet[i];
								}
								debug(" P(%d)=",packet_length-address_length);
								for(uint8_t i=address_length; i<packet_length; i++)
								{
									debug(" %02X",packet[i]);
								}
								packet_length=packet_length-address_length;
								debugln("\r\n--------------------------------");
								phase=2;
								debugln("Identifying all RF channels in use.");
								bind_counter=0;
								hopping_frequency_no=0;
								rf_ch_num=0;
								packet_count=0;
								debug("Trying RF channel: 0");

								XN297_SetTXAddr(rx_tx_addr,address_length);
								XN297_SetRXAddr(rx_tx_addr,packet_length);
								XN297_RFChannel(0);
								XN297_SetTxRxMode(TXRX_OFF);
								XN297_SetTxRxMode(RX_EN);
							}
						}
					}
					break;
				case 2:
					if(bind_counter>XN297DUMP_PERIOD_SCAN)
					{	// Scan frequencies
						hopping_frequency_no++;
						bind_counter=0;
						if(packet_count && packet_count<=20)
							debug("\r\nTrying RF channel: ");
						packet_count=0;
						if(hopping_frequency_no>XN297DUMP_MAX_RF_CHANNEL)
						{
							debug("\r\n\r\n%d RF channels identified:",rf_ch_num);
							for(uint8_t i=0;i<rf_ch_num;i++)
								debug(" %d",hopping_frequency[i]);
							time_rf=(uint32_t*)malloc(rf_ch_num*sizeof(time));
							if(time_rf==NULL)
							{
								debugln("\r\nCan't allocate memory for next phase!!!");
								phase=0;
								break;
							}
							debugln("\r\n--------------------------------");
							debugln("Identifying RF channels order.");
							hopping_frequency_no=1;
							phase=3;
							packet_count=0;
							bind_counter=0;
							debugln("Time between CH:%d and CH:%d",hopping_frequency[0],hopping_frequency[hopping_frequency_no]);
							time_rf[hopping_frequency_no]=0xFFFFFFFF;
							XN297_RFChannel(hopping_frequency[0]);
							uint16_t timeL=TCNT1;
							if(TIMER2_BASE->SR & TIMER_SR_UIF)
							{//timer just rolled over...
								XN297Dump_overflow();
								timeL=0;
							}
							time=(timeH<<16)+timeL;
							XN297_SetTxRxMode(TXRX_OFF);
							XN297_SetTxRxMode(RX_EN);
							XN297Dump_overflow();
							break;
						}
						debug(",%d",hopping_frequency_no);
						XN297_RFChannel(hopping_frequency_no);
						// switch to RX mode
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(RX_EN);
					}
					if( XN297_IsRX() )
					{ // RX fifo data ready
						//	if(NRF24L01_ReadReg(NRF24L01_09_CD))
						{
							uint8_t res;
							if(enhanced)
							{
								res=XN297_ReadEnhancedPayload(packet, packet_length);
								res++;
							}
							else
								res=XN297_ReadPayload(packet, packet_length);
							if(res)
							{ // valid crc found
								XN297Dump_overflow();
								uint16_t timeL=TCNT1;
								if(TIMER2_BASE->SR & TIMER_SR_UIF)
								{//timer just rolled over...
									XN297Dump_overflow();
									timeL=0;
								}
								if(packet_count==0)
								{//save channel
									hopping_frequency[rf_ch_num]=hopping_frequency_no;
									rf_ch_num++;
									time=0;
								}
								else
									time=(timeH<<16)+timeL-time;
								debug("\r\nRX on channel: %d, Time: %5luus P:",hopping_frequency_no, time>>1);
								time=(timeH<<16)+timeL;
								for(uint8_t i=0;i<packet_length;i++)
									debug(" %02X",packet[i]);
								packet_count++;
								if(packet_count>20)
								{//change channel
									bind_counter=XN297DUMP_PERIOD_SCAN+1;
									debug("\r\nTrying RF channel: ");
								}
							}
						}
						// restart RX mode
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(RX_EN);
					}
					XN297Dump_overflow();
					break;
				case 3:
					if(bind_counter>XN297DUMP_PERIOD_SCAN)
					{	// Scan frequencies
						hopping_frequency_no++;
						bind_counter=0;
						if(hopping_frequency_no>=rf_ch_num)
						{
							uint8_t next=0;
							debugln("\r\n\r\nChannel order:");
							debugln("%d:     0us",hopping_frequency[0]);
							uint8_t i=1;
							do
							{
								time=time_rf[i];
								if(time!=0xFFFFFFFF)
								{
									next=i;
									for(uint8_t j=2;j<rf_ch_num;j++)
										if(time>time_rf[j])
										{
											next=j;
											time=time_rf[j];
										}
									time_rf[next]=-1;
									debugln("%d: %5luus",hopping_frequency[next],time);
									i=0;
								}
								i++;
							}
							while(i<rf_ch_num);
							free(time_rf);
							debugln("\r\n--------------------------------");
							debugln("Identifying Sticks and features.");
							phase=4;
							hopping_frequency_no=0;
							break;
						}
						debugln("Time between CH:%d and CH:%d",hopping_frequency[0],hopping_frequency[hopping_frequency_no]);
						time_rf[hopping_frequency_no]=-1;
						XN297_RFChannel(hopping_frequency[0]);
						uint16_t timeL=TCNT1;
						if(TIMER2_BASE->SR & TIMER_SR_UIF)
						{//timer just rolled over...
							XN297Dump_overflow();
							timeL=0;
						}
						time=(timeH<<16)+timeL;
						// switch to RX mode
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(RX_EN);
					}
					if( XN297_IsRX() )
					{ // RX fifo data ready
						//if(NRF24L01_ReadReg(NRF24L01_09_CD))
						{
							uint8_t res;
							if(enhanced)
							{
								res=XN297_ReadEnhancedPayload(packet, packet_length);
								res++;
							}
							else
								res=XN297_ReadPayload(packet, packet_length);
							if(res)
							{ // valid crc found
								XN297Dump_overflow();
								uint16_t timeL=TCNT1;
								if(TIMER2_BASE->SR & TIMER_SR_UIF)
								{//timer just rolled over...
									XN297Dump_overflow();
									timeL=0;
								}
								if(packet_count&1)
								{
									time=(timeH<<16)+timeL-time;
									if(time_rf[hopping_frequency_no] > (time>>1))
										time_rf[hopping_frequency_no]=time>>1;
									debugln("Time: %5luus", time>>1);
									XN297_RFChannel(hopping_frequency[0]);
								}
								else
								{
									time=(timeH<<16)+timeL;
									XN297_RFChannel(hopping_frequency[hopping_frequency_no]);
								}
								packet_count++;
								if(packet_count>24)
								{
									bind_counter=XN297DUMP_PERIOD_SCAN+1;
									packet_count=0;
								}
							}
						}
						// restart RX mode
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(RX_EN);
					}
					XN297Dump_overflow();
					break;
				case 4:
					if( XN297_IsRX() )
					{ // RX fifo data ready
						//if(NRF24L01_ReadReg(NRF24L01_09_CD))
						{
							uint8_t res;
							if(enhanced)
							{
								res=XN297_ReadEnhancedPayload(packet, packet_length);
								res++;
							}
							else
								res=XN297_ReadPayload(packet, packet_length);
							if(res)
							{ // valid crc found
								if(memcmp(packet_in,packet,packet_length))
								{
									debug("P:");
									for(uint8_t i=0;i<packet_length;i++)
										debug(" %02X",packet[i]);
									debugln("");
									memcpy(packet_in,packet,packet_length);
								}
							}
						}
						// restart RX mode
						XN297_SetTxRxMode(TXRX_OFF);
						XN297_SetTxRxMode(RX_EN);
					}
					break;
			}
		}
		else if(sub_protocol == XN297DUMP_NRF)
		{
			if(phase==0)
			{
				address_length=3;
				memcpy(rx_tx_addr, (uint8_t *)"\xBD\x54\x78", address_length); //"\x62\xE6\xBD\x54\x78"

				bitrate=XN297DUMP_1M;
				packet_length=7;
				hopping_frequency_no=40; //bind ?, normal 40
				
				NRF24L01_Initialize();
				NRF24L01_SetTxRxMode(TXRX_OFF);
				NRF24L01_SetTxRxMode(RX_EN);
				NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, address_length-2);			// RX/TX address length
				NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, address_length);	// set up RX address
				NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, packet_length);				// Enable rx pipe 0
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, option);	//hopping_frequency_no);

				debug("NRF dump, len=%d, rf=%d, address length=%d, bitrate=",packet_length,hopping_frequency_no,address_length);
				switch(bitrate)
				{
					case XN297DUMP_250K:
						NRF24L01_SetBitrate(NRF24L01_BR_250K);
						debugln("250K");
						break;
					case XN297DUMP_2M:
						NRF24L01_SetBitrate(NRF24L01_BR_2M);
						debugln("2M");
						break;
					default:
						NRF24L01_SetBitrate(NRF24L01_BR_1M);
						debugln("1M");
						break;

				}
				NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX)); //_BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) | 
				phase++;
			}
			else
			{
				if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR))
				{ // RX fifo data ready
					if(NRF24L01_ReadReg(NRF24L01_09_CD))
					{
						NRF24L01_ReadPayload(packet, packet_length);
						//bool ok=true;
						uint8_t buffer[40];
						memcpy(buffer,packet,packet_length);
						//if(memcmp(&packet_in[0],&packet[0],packet_length))
						{
							debug("P:");
							for(uint8_t i=0;i<packet_length;i++)
								debug(" %02X",packet[i]);
							debugln("");
							memcpy(packet_in,packet,packet_length);
						}
						/*//realign bits
							for(uint8_t i=0; i<packet_length; i++)
								buffer[i]=buffer[i+2];
							//for(uint8_t i=0; i<packet_length; i++)
							//	buffer[i]=(buffer[i]<<4)+(buffer[i+1]>>4);
							
							//check for validity and decode
							memset(packet_in,0,packet_length);
							for(uint8_t i=0; i<packet_length-2; i++)
							{
								for(uint8_t j=0;j<2;j++)
								{
									packet_in[i>>2] >>= 1;
									if( (buffer[i]&0xC0) == 0xC0 && (buffer[i]&0x30) == 0x00 )
										packet_in[i>>2] |= 0x80;
									else if( (buffer[i]&0xC0) == 0x00 && (buffer[i]&0x30) == 0x30 )
										packet_in[i>>2] |= 0x00;
									else
										ok=false;	// error
									buffer[i] <<= 4;
								}
							}
							if(ok)
							{
								debug("P:(%02X,%02X):",packet[0],packet[1]);
								for(uint8_t i=0;i<packet_length/4;i++)
									debug(" %02X",packet_in[i]);
								debugln("");
								memcpy(packet_in,packet,packet_length);
							}
						}*/
						/*crc=0;
						for (uint8_t i = 1; i < 12; ++i)
							crc16_update( packet[i], 8);
						if(packet[12]==((crc>>8)&0xFF) && packet[13]==(crc&0xFF))
							if(memcmp(&packet_in[1],&packet[1],packet_length-1))
							{
								debug("P:");
								for(uint8_t i=0;i<packet_length;i++)
									debug(" %02X",packet[i]);
								debug(" CRC: %04X",crc);
								debugln("");
								debug("P(%02X):",packet[0]);
								for(uint8_t i=1;i<packet_length-2;i++)
									debug(" %02X",((bit_reverse(packet[i])<<1)|(bit_reverse(packet[i-1])>>7))&0xFF);
								debugln("");
								memcpy(packet_in,packet,packet_length);
							}*/
					}
					// restart RX mode
					NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);			// Clear data ready, data sent, and retransmit
					NRF24L01_SetTxRxMode(TXRX_OFF);
					NRF24L01_SetTxRxMode(RX_EN);
					NRF24L01_FlushRx();
					NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_PWR_UP) | _BV(NRF24L01_00_PRIM_RX)); //  _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_CRCO) |
				}
				NRF24L01_WriteReg(NRF24L01_05_RF_CH, option);	//hopping_frequency_no);
			}
		}
		else if(sub_protocol == XN297DUMP_CC2500)
		{
		#if defined (CC2500_INSTALLED)
			if(phase==0)
			{
				address_length=5;
				switch(RX_num)
				{
					case 0:
						memcpy(rx_tx_addr, (uint8_t *)"\xAE\xD2\x71\x79\x46", address_length);
						break;
					case 1:
						memcpy(rx_tx_addr, (uint8_t *)"\x5D\xA4\xE2\xF2\x8C", address_length);
						break;
					case 2:
						memcpy(rx_tx_addr, (uint8_t *)"\xBB\x49\xC5\xE5\x18", address_length);
						break;
					case 3:
						memcpy(rx_tx_addr, (uint8_t *)"\x76\x93\x8B\xCA\x30", address_length);
						break;
					case 4:
						memcpy(rx_tx_addr, (uint8_t *)"\xED\x27\x17\x94\x61", address_length);
						break;
					case 5:
						memcpy(rx_tx_addr, (uint8_t *)"\xDA\x4E\x2F\x28\xC2", address_length);
						break;
					case 6:
						memcpy(rx_tx_addr, (uint8_t *)"\xAB\xB4\x9C\x5E\x51", address_length);
						break;
					case 7:
						memcpy(rx_tx_addr, (uint8_t *)"\x57\x69\x38\xBC\xA3", address_length);
						break;
				}
				packet_length=38;
				hopping_frequency_no=54; //bind 30, normal 54
				debugln("CC2500 dump, len=%d, rf=%d, address length=%d, bitrate=250K",packet_length,hopping_frequency_no,address_length);
				
				//Config CC2500
				CC2500_250K_Init();
				CC2500_SetFreqOffset();
				CC2500_WriteReg(CC2500_04_SYNC1, rx_tx_addr[0]);	// Sync word, high byte
				CC2500_WriteReg(CC2500_05_SYNC0, rx_tx_addr[1]);	// Sync word, low byte
				CC2500_WriteReg(CC2500_09_ADDR,  rx_tx_addr[2]);	// Set addr
				CC2500_WriteReg(CC2500_12_MDMCFG2,  0x12);			// Modem Configuration, GFSK, 16/16 Sync Word TX&RX
				CC2500_WriteReg(CC2500_06_PKTLEN, packet_length);	// Packet len

				//2.4001GHz: offfset of 100KHz
				CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C);   // Frequency Control Word, High Byte
				CC2500_WriteReg(CC2500_0E_FREQ1,    0x4F);   // Frequency Control Word, Middle Byte
				CC2500_WriteReg(CC2500_0F_FREQ0,    0xC1);   // Frequency Control Word, Low Byte

				CC2500_250K_RFChannel(hopping_frequency_no);

				CC2500_SetTxRxMode(RX_EN);
				CC2500_Strobe(CC2500_SFRX);
				CC2500_Strobe(CC2500_SRX);
				phase++;
			}
			else
			{
				CC2500_SetFreqOffset();
				if((CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F) == packet_length + 2) // 2 = RSSI + LQI
				{ // RX fifo data ready
					//debugln("f_off=%02X", CC2500_ReadReg(0x32 | CC2500_READ_BURST));
					CC2500_ReadData(packet, packet_length+2);
					bool ok=true;
					//filter address
					if(rx_tx_addr[2]!=packet[0] || rx_tx_addr[3]!=packet[1] || rx_tx_addr[4]!=packet[2] )
						ok=false;
					//filter constants
					if(RX_num == 0 && ok)
					{
						if (packet[3] != 0x10 || (packet[4] & 0xFC) != 0x54 || packet[5] != 0x64)
							ok=false;
						else if(packet[6] != 0x10 && packet[6] != 0x25)
								ok=false;
						else if(memcmp(&packet[9],"\xC6\xE7\x50\x02\xAA\x49",6)!=0)
								ok=false;
					}
					else if(RX_num == 4 && ok)
					{
						if (packet[3] != 0x05 || (packet[4] & 0xCF) != 0x46)
							ok=false;
						else if(packet[5] != 0x41 && packet[5] != 0x42)
								ok=false;
						else if((packet[6]&0xF0) != 0x50 && (packet[6]&0xF0) != 0x00)
								ok=false;
						else if((packet[8]&0x0F) != 0x0C)
								ok=false;
						else if(memcmp(&packet[9],"\x6E\x75\x00\x2A\xA4\x94\xA4\x6F",8)!=0)
								ok=false;
					}
					if(ok)
					{
						//uint8_t buffer[100];
						//memcpy(buffer,packet,packet_length);
						//if(memcmp(&packet_in[0],&packet[0],packet_length))
						{
							debug("P:");
							for(uint8_t i=0;i<packet_length;i++)
								debug(" %02X",packet[i]);
							debugln("");
							memcpy(packet_in,packet,packet_length);
						}
					}
					CC2500_SetTxRxMode(TXRX_OFF);
					CC2500_SetTxRxMode(RX_EN);
					CC2500_Strobe(CC2500_SFRX);
					CC2500_Strobe(CC2500_SRX);
				}
			}
		#endif
		}
		bind_counter++;
		if(IS_RX_FLAG_on)					// Let the radio update the protocol
		{
			if(Update_All()) return 10000;	// New protocol selected
			if(prev_option!=option && sub_protocol<XN297DUMP_AUTO)
			{	// option has changed
				hopping_frequency_no=option;
				prev_option=option;
			}
		}
		XN297Dump_overflow();
	}
	return 100;
}

void XN297Dump_init(void)
{
	BIND_DONE;
	if(sub_protocol<XN297DUMP_AUTO)
		bitrate=sub_protocol;
	else
		bitrate=0;
	address_length=RX_num;
	if(address_length<3||address_length>5)
		address_length=5;	//default
	XN297Dump_RF_init();
	bind_counter=0;
	rf_ch_num=0xFF;
	prev_option=option^0x55;
	phase=0;				// init
}

#endif
