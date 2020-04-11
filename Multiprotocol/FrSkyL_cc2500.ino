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


#if defined(FRSKYL_CC2500_INO)

#include "iface_cc2500.h"

//#define FRSKYL_FORCE_ID
#define FRSKYL_PACKET_LEN	256
#define FRSKYL_PERIOD		18000

uint8_t FrSkyL_buffer[FRSKYL_PACKET_LEN];

static void __attribute__((unused)) FrSkyL_build_bind_packet()
{
	//Header
	packet[0] = 0x4E;				// Unknown but constant
	//Bind packet
	memset(&packet[1],0x00,3);
	//ID
	packet[4 ] = rx_tx_addr[3];		// ID
	packet[5 ] = rx_tx_addr[2];		// ID
	int idx = ((state -FRSKY_BIND) % 10) * 5;
	packet[6 ] = idx;
	packet[7 ] = hopping_frequency[idx++];
	packet[8 ] = hopping_frequency[idx++];
	packet[9 ] = hopping_frequency[idx++];
	packet[10] = hopping_frequency[idx++];
	packet[11] = hopping_frequency[idx++];
	packet[12] = rx_tx_addr[1];		// ID or hw ver?
	packet[13] = RX_num;
	packet[14] = 0x00;				// Unknown but constant
	//CRC
	uint16_t lcrc = FrSkyX_crc(&packet[1], 14);
	packet[15] = lcrc >> 8;
	packet[16] = lcrc;
	//Debug
/*	debug("Bind:");
	for(uint8_t i=0;i<17;i++)
		debug(" %02X",packet[i]);
	debugln("");*/
}

static void __attribute__((unused)) FrSkyL_build_packet()
{
	static uint8_t chan_offset=0;
	uint16_t chan_0,chan_1;

	//Header
	packet[0 ] = 0x4E;				// Unknown but constant
	//ID
	packet[1 ] = rx_tx_addr[3];		// ID
	packet[2 ] = rx_tx_addr[2];		// ID
	packet[3 ] = rx_tx_addr[1];		// ID or hw ver?
	//skip_hop
	packet[4 ] = (FrSkyX_chanskip<<6)|hopping_frequency_no; 
	packet[5 ] = FrSkyX_chanskip>>2;
	//Channels
	uint8_t startChan = chan_offset;
	for(uint8_t i = 0; i <9 ; i+=3)
	{//9 bytes of channel data
		chan_0 = FrSkyX_scaleForPXX(startChan,6);
		startChan++;
		//
		chan_1 = FrSkyX_scaleForPXX(startChan,6);
		startChan++;
		//
		packet[6+i] = lowByte(chan_0);									//3 bytes*4
		packet[6+i+1]=(((chan_0>>8) & 0x0F)|(chan_1 << 4));
		packet[6+i+2]=chan_1>>4;
	}
	if(sub_protocol & 0x01 )											//6ch mode only??
		chan_offset = 0 ;
	else
		chan_offset^=0x06;
	//CRC
	uint16_t lcrc = FrSkyX_crc(&packet[1], 14, RX_num);
	packet[15] = lcrc >> 8;
	packet[16] = lcrc;
	//Debug
	/*debug("Norm:");
	for(uint8_t i=0;i<17;i++)
		debug(" %02X",packet[i]);
	debugln("");*/
}

static void __attribute__((unused))  FrSkyL_encode_packet(bool type)
{
	#define FRSKYL_BIT0 0xED
	#define FRSKYL_BIT1 0x712
	
	uint32_t bits = 0;
	uint8_t bitsavailable = 0;
	uint8_t idx = 0,len=6;
	if(type)
	{//just replace packet content
		idx=66;
		len=17;
	}

	//debugln("Encode:");
	for (uint8_t i = 0; i < len; i++)
	{
		uint8_t tmp=packet[i];
		//debug("%02X =",tmp);
		for(uint8_t j=0;j<8;j++)
		{
			bits <<= 11;
			if(tmp&0x01)
				bits |= FRSKYL_BIT1;
			else
				bits |= FRSKYL_BIT0;
			tmp >>=1;
			bitsavailable += 11;
			while (bitsavailable >= 8) {
				uint32_t bits_tmp=bits>>(bitsavailable-8);
				bitsavailable -= 8;
				FrSkyL_buffer[idx] = bits_tmp;
				//debug(" %02X",FrSkyL_buffer[idx]);
				idx++;
			}
		}
		//debugln("");
	}
}

uint16_t ReadFrSkyL()
{
	static uint8_t written=0, send=0;
	switch(send)
	{
		case 1:
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_Strobe(CC2500_SFTX);
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, FrSkyL_buffer, 64);
			CC2500_Strobe(CC2500_STX);
			CC2500_Strobe(CC2500_SIDLE);	// This cancels the current transmission???
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, FrSkyL_buffer, 64);
			CC2500_Strobe(CC2500_SFTX);		// This just clears what we've written???
			CC2500_Strobe(CC2500_STX);
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, FrSkyL_buffer, 64);
			written=64;
			send++;
			return 2623;
		case 2:
			len=FRSKYL_PACKET_LEN-written;
			if(len>31)
				len=31;
			CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, FrSkyL_buffer+written, len);
			written+=len;
			if(len!=31)			//everything has been sent
			{
				send=0;
				return 2936;
			}
			return 1984;
	}
	
	switch(state)
	{	
		default: 
			//Bind
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(9000);
			#endif
			FrSkyX_set_start(47);
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			FrSkyL_build_bind_packet();
			FrSkyL_encode_packet(true);

			CC2500_Strobe(CC2500_SIDLE);
			if(IS_BIND_DONE)
				state = FRSKY_BIND_DONE;
			else
			{
				state++;
				send=1;
			}
			return 537;
		case FRSKY_BIND_DONE:
			FrSkyX_initialize_data(0);
			hopping_frequency_no=0;
			BIND_DONE;
			state++;													//FRSKY_DATA1
			break;

		case FRSKY_DATA1:
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);				//Frequency offset hack 
				prev_option = option ;
			}
			FrSkyX_set_start(hopping_frequency_no);
			FrSkyL_build_packet();
			FrSkyL_encode_packet(true);
			CC2500_SetPower();
			hopping_frequency_no = (hopping_frequency_no+FrSkyX_chanskip)%47;
			send=1;
			return 537;
	}		
	return 1;		
}

uint16_t initFrSkyL()
{
	set_rx_tx_addr(MProtocol_id_master);
	rx_tx_addr[1]=0x02;		// ID related, hw version?

	#ifdef FRSKYL_FORCE_ID
		rx_tx_addr[3]=0x0E;
		rx_tx_addr[2]=0x1C;
		rx_tx_addr[1]=0x02;
	#endif
	FrSkyX2_init_hop();
	
	while(!FrSkyX_chanskip)
		FrSkyX_chanskip=random(0xfefefefe)%47;

	FrSkyX_init();

	//Prepare frame
	memset(FrSkyL_buffer,0x00,FRSKYL_PACKET_LEN-3);
	memset(&FrSkyL_buffer[FRSKYL_PACKET_LEN-3],0x55,3);
	memset(packet,0xAA,6);
	FrSkyL_encode_packet(false);
	/*debugln("Frame:");
	for(uint16_t i=0;i<FRSKYL_PACKET_LEN;i++)
	{
		debug(" %02X",FrSkyL_buffer[i]);
		if(i%11==10)
			debugln("");
	}
	debugln("");*/
	
	if(IS_BIND_IN_PROGRESS)
	{	   
		state = FRSKY_BIND;
		FrSkyX_initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		FrSkyX_initialize_data(0);
	}
	return 10000;
}	
#endif
