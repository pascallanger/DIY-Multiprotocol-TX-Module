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

#if defined(ESKY150V2_CC2500_INO)

#include "iface_nrf250k.h"

//#define ESKY150V2_FORCE_ID

#define ESKY150V2_PAYLOADSIZE 40
#define ESKY150V2_BINDPAYLOADSIZE 150
#define ESKY150V2_NFREQCHANNELS 70
#define ESKY150V2_TXID_SIZE 4
#define ESKY150V2_BIND_CHANNEL 0x00
#define ESKY150V2_PACKET_PERIOD 10000
#define ESKY150V2_BINDING_PACKET_PERIOD 57000

#ifdef ESKY150V2_FORCE_ID
const uint8_t PROGMEM ESKY150V2_hop[ESKY150V2_NFREQCHANNELS]= {
	0x07, 0x47, 0x09, 0x27, 0x0B, 0x42, 0x0D, 0x35, 0x17, 0x40, 0x26, 0x3D, 0x16, 0x43, 0x06, 0x2A, 0x24, 0x44,
	0x0E, 0x38, 0x20, 0x48, 0x22, 0x2D, 0x2B, 0x39, 0x0F, 0x36, 0x23, 0x46, 0x14, 0x3B, 0x1A, 0x41, 0x10, 0x2E,
	0x1E, 0x28, 0x0C, 0x49, 0x1D, 0x3E, 0x29, 0x2C, 0x25, 0x30, 0x1C, 0x2F, 0x1B, 0x33, 0x13, 0x31, 0x0A, 0x37,
	0x12, 0x3C, 0x18, 0x4B, 0x11, 0x45, 0x21, 0x4A, 0x1F, 0x3F, 0x15, 0x32, 0x08, 0x3A, 0x19, 0x34 };
/*const uint8_t PROGMEM ESKY150V2_hop2[40]= {
	0x19, 0x23, 0x13, 0x1B, 0x09, 0x22, 0x14, 0x27, 0x06, 0x26, 0x16, 0x24, 0x0B, 0x2A, 0x0E, 0x1C, 0x11, 0x1E,
	0x08, 0x29, 0x0D, 0x28, 0x18, 0x2D, 0x12, 0x20, 0x0C, 0x1A, 0x10, 0x1D, 0x07, 0x2C, 0x0A, 0x2B, 0x0F, 0x25,
	0x15, 0x1F, 0x17, 0x21 };*/
#endif

static void __attribute__((unused)) ESKY150V2_set_freq(void)
{
	calc_fh_channels(ESKY150V2_NFREQCHANNELS);
	
	#ifdef ESKY150V2_FORCE_ID
		for(uint8_t i=0; i<ESKY150V2_NFREQCHANNELS; i++)
			hopping_frequency[i]=pgm_read_byte_near( &ESKY150V2_hop[i] );
	#endif
	
	//Bind channel
	hopping_frequency[ESKY150V2_NFREQCHANNELS]=ESKY150V2_BIND_CHANNEL;
	
	//Calib all channels
	NRF250K_SetFreqOffset();	// Set frequency offset
	NRF250K_HoppingCalib(ESKY150V2_NFREQCHANNELS+1);
}

static void __attribute__((unused)) ESKY150V2_send_packet()
{
	NRF250K_SetFreqOffset();	// Set frequency offset
	NRF250K_Hopping(hopping_frequency_no);
	if (++hopping_frequency_no >= ESKY150V2_NFREQCHANNELS)
		hopping_frequency_no = 0;
	NRF250K_SetPower();			//Set power level

	packet[0] = 0xFA;		// Unknown
	packet[1] = 0x41;		// Unknown
	packet[2] = 0x08;		// Unknown
	packet[3] = 0x00;		// Unknown
	for(uint8_t i=0;i<16;i++)
	{
		uint16_t channel=convert_channel_16b_limit(CH_TAER[i],200,1000);
		packet[4+2*i] = channel;
		packet[5+2*i] = channel>>8;
	}
	NRF250K_WritePayload(packet, ESKY150V2_PAYLOADSIZE);
}

uint16_t ESKY150V2_callback()
{
	if(option==0) option=1; 	//Trick the RF component auto select system
	if(IS_BIND_DONE)
	{
		#ifdef MULTI_SYNC
			telemetry_set_input_sync(ESKY150V2_PACKET_PERIOD);
		#endif
		ESKY150V2_send_packet();
	}
	else
	{
		BIND_DONE;					//Need full power for bind to work...
		NRF250K_SetPower();			//Set power level
		BIND_IN_PROGRESS;
		NRF250K_WritePayload(packet, ESKY150V2_BINDPAYLOADSIZE);
		if (--bind_counter == 0)
		{
			BIND_DONE;
			// Change TX address from bind to normal mode
			NRF250K_SetTXAddr(rx_tx_addr, ESKY150V2_TXID_SIZE);
			memset(packet,0x00,ESKY150V2_PAYLOADSIZE);
		}
		return 30000; //ESKY150V2_BINDING_PACKET_PERIOD;
	}
	return ESKY150V2_PACKET_PERIOD;
}

uint16_t initESKY150V2()
{
	if(option==0) option=1;		 // Trick the RF component auto select system
	NRF250K_Init();
	ESKY150V2_set_freq();
	hopping_frequency_no = 0;

	#ifdef ESKY150V2_FORCE_ID	// ID taken from TX dump
		rx_tx_addr[0]=0x87;rx_tx_addr[1]=0x5B;rx_tx_addr[2]=0x2C;rx_tx_addr[3]=0x5D;
	#endif

	memset(packet,0x00,ESKY150V2_BINDPAYLOADSIZE);

	if(IS_BIND_IN_PROGRESS)
	{
		NRF250K_SetTXAddr((uint8_t *)"\x73\x73\x74\x63", ESKY150V2_TXID_SIZE);	//Bind address
		NRF250K_Hopping(ESKY150V2_NFREQCHANNELS);	//Bind channel
		memcpy(packet,"\x73\x73\x74\x63", ESKY150V2_TXID_SIZE);
		memcpy(&packet[ESKY150V2_TXID_SIZE],rx_tx_addr, ESKY150V2_TXID_SIZE);
		packet[8]=0x41;								//Unknown
		packet[9]=0x88;								//Unknown
		packet[10]=0x41;							//Unknown
		memset(&packet[11],0xAA,4);					//Unknown
		memcpy(&packet[15],hopping_frequency,ESKY150V2_NFREQCHANNELS);	// hop table
		//for(uint8_t i=0; i<40; i++)				// Does not seem to be needed
		//	packet[i+85]=pgm_read_byte_near( &ESKY150V2_hop2[i] );
		bind_counter=100;
	}
	else
		NRF250K_SetTXAddr(rx_tx_addr, ESKY150V2_TXID_SIZE);
	return 50000;
}

#endif
