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

#if defined(FRSKY_CC2500_INO)

#include "iface_cc2500.h"

//##########Variables########
//uint32_t state;
//uint8_t len;
uint8_t telemetry_counter=0;

/*
enum {
	FRSKY_BIND		= 0,
	FRSKY_BIND_DONE	= 1000,
	FRSKY_DATA1,
	FRSKY_DATA2,
	FRSKY_DATA3,
	FRSKY_DATA4,
	FRSKY_DATA5
};
*/

uint16_t initFrSky_2way()
{
	if(IS_AUTOBIND_FLAG_on)
	{	   
		frsky2way_init(1);
		state = FRSKY_BIND;//
	}
	else
	{
		frsky2way_init(0);
		state = FRSKY_DATA2;
	}
	return 10000;
}	
		
uint16_t ReadFrSky_2way()
{ 
	if (state < FRSKY_BIND_DONE)
	{
		frsky2way_build_bind_packet();
		cc2500_strobe(CC2500_SIDLE);
		cc2500_writeReg(CC2500_0A_CHANNR, 0x00);
		cc2500_writeReg(CC2500_23_FSCAL3, 0x89);		
		cc2500_strobe(CC2500_SFRX);//0x3A
		cc2500_writeFifo(packet, packet[0]+1);
		state++;
		return 9000;
	}
	if (state == FRSKY_BIND_DONE)
	{
		state = FRSKY_DATA2;
		frsky2way_init(0);
		counter = 0;
		BIND_DONE;
	}
	else
		if (state == FRSKY_DATA5)
		{
			cc2500_strobe(CC2500_SRX);//0x34 RX enable
			state = FRSKY_DATA1;	
			return 9200;
		}
	counter = (counter + 1) % 188;	
	if (state == FRSKY_DATA4)
	{	//telemetry receive
		CC2500_SetTxRxMode(RX_EN);
		cc2500_strobe(CC2500_SIDLE);
		cc2500_writeReg(CC2500_0A_CHANNR, get_chan_num(counter % 47));
		cc2500_writeReg(CC2500_23_FSCAL3, 0x89);
		state++;
		return 1300;
	}
	else
	{
		if (state == FRSKY_DATA1)
		{
			len = cc2500_readReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
			if (len)//20 bytes
			{
				cc2500_readFifo(pkt, len);	//received telemetry packets			
				#if defined(TELEMETRY)
				//parse telemetry packet here
				check_telemetry(pkt,len);	//check if valid telemetry packets and buffer them.
				#endif	
			}			
			CC2500_SetTxRxMode(TX_EN);
			CC2500_SetPower();	// Set tx_power
		}
		cc2500_strobe(CC2500_SIDLE);
		cc2500_writeReg(CC2500_0A_CHANNR, get_chan_num(counter % 47));
		cc2500_writeReg(CC2500_23_FSCAL3, 0x89);
		cc2500_strobe(CC2500_SFRX);        
		frsky2way_data_frame();
		cc2500_writeFifo(packet, packet[0]+1);
		state++;
	}				
	return state == FRSKY_DATA4 ? 7500 : 9000;		
}

#if defined(TELEMETRY)
static void check_telemetry(uint8_t *pkt,uint8_t len)
{
	if(pkt[1] != rx_tx_addr[3] || pkt[2] != rx_tx_addr[2] || len != pkt[0] + 3)
	{//only packets with the required id and packet length
		for(uint8_t i=3;i<6;i++)
			pktt[i]=0;
		return;
	}
	else
	{	   
		for (uint8_t i=3;i<len;i++)
	    pktt[i]=pkt[i];				 
		telemetry_link=1;
		if(pktt[6]>0)
		telemetry_counter=(telemetry_counter+1)%32;		
	}
}

void compute_RSSIdbm(){ 
	RSSI_dBm = (((uint16_t)(pktt[len-2])*18)>>5);
	if(pktt[len-2] >=128)
		RSSI_dBm -= 82;
	else
		RSSI_dBm += 65;
}

#endif

static void frsky2way_init(uint8_t bind)
{
	// Configure cc2500 for tx mode
	CC2500_Reset();
	//
	cc2500_writeReg(CC2500_02_IOCFG0, 0x06);		
	cc2500_writeReg(CC2500_00_IOCFG2, 0x06);
	cc2500_writeReg(CC2500_17_MCSM1, 0x0c);
	cc2500_writeReg(CC2500_18_MCSM0, 0x18);
	cc2500_writeReg(CC2500_06_PKTLEN, 0x19);
	cc2500_writeReg(CC2500_07_PKTCTRL1, 0x04);
	cc2500_writeReg(CC2500_08_PKTCTRL0, 0x05);
	cc2500_writeReg(CC2500_3E_PATABLE, 0xff);
	cc2500_writeReg(CC2500_0B_FSCTRL1, 0x08);
	cc2500_writeReg(CC2500_0C_FSCTRL0, option);
	//base freq              FREQ = 0x5C7627 (F = 2404MHz)
	cc2500_writeReg(CC2500_0D_FREQ2, 0x5c);	
	cc2500_writeReg(CC2500_0E_FREQ1, 0x76);
	cc2500_writeReg(CC2500_0F_FREQ0, 0x27);
	//		
	cc2500_writeReg(CC2500_10_MDMCFG4, 0xAA);		
	cc2500_writeReg(CC2500_11_MDMCFG3, 0x39);
	cc2500_writeReg(CC2500_12_MDMCFG2, 0x11);
	cc2500_writeReg(CC2500_13_MDMCFG1, 0x23);
	cc2500_writeReg(CC2500_14_MDMCFG0, 0x7a);
	cc2500_writeReg(CC2500_15_DEVIATN, 0x42);
	cc2500_writeReg(CC2500_19_FOCCFG, 0x16);
	cc2500_writeReg(CC2500_1A_BSCFG, 0x6c);	
	cc2500_writeReg(CC2500_1B_AGCCTRL2, bind ? 0x43 : 0x03);
	cc2500_writeReg(CC2500_1C_AGCCTRL1,0x40);
	cc2500_writeReg(CC2500_1D_AGCCTRL0,0x91);
	cc2500_writeReg(CC2500_21_FREND1, 0x56);
	cc2500_writeReg(CC2500_22_FREND0, 0x10);
	cc2500_writeReg(CC2500_23_FSCAL3, 0xa9);
	cc2500_writeReg(CC2500_24_FSCAL2, 0x0A);
	cc2500_writeReg(CC2500_25_FSCAL1, 0x00);
	cc2500_writeReg(CC2500_26_FSCAL0, 0x11);
	cc2500_writeReg(CC2500_29_FSTEST, 0x59);
	cc2500_writeReg(CC2500_2C_TEST2, 0x88);
	cc2500_writeReg(CC2500_2D_TEST1, 0x31);
	cc2500_writeReg(CC2500_2E_TEST0, 0x0B);
	cc2500_writeReg(CC2500_03_FIFOTHR, 0x07);
	cc2500_writeReg(CC2500_09_ADDR, 0x00);
	//
	CC2500_SetTxRxMode(TX_EN);
	CC2500_SetPower();
	
	cc2500_strobe(CC2500_SIDLE);	

	cc2500_writeReg(CC2500_09_ADDR, bind ? 0x03 : rx_tx_addr[3]);
	cc2500_writeReg(CC2500_07_PKTCTRL1, 0x05);
	cc2500_strobe(CC2500_SIDLE);	// Go to idle...
	//
	cc2500_writeReg(CC2500_0A_CHANNR, 0x00);
	cc2500_writeReg(CC2500_23_FSCAL3, 0x89);
	cc2500_strobe(CC2500_SFRX);
	//#######END INIT########		
}
	
static uint8_t get_chan_num(uint16_t idx)
{
	uint8_t ret = (idx * 0x1e) % 0xeb;
	if(idx == 3 || idx == 23 || idx == 47)
		ret++;
	if(idx > 47)
		return 0;
	return ret;
}

static void frsky2way_build_bind_packet()
{
	//11 03 01 d7 2d 00 00 1e 3c 5b 78 00 00 00 00 00 00 01
	//11 03 01 19 3e 00 02 8e 2f bb 5c 00 00 00 00 00 00 01
	packet[0] = 0x11;                
	packet[1] = 0x03;                
	packet[2] = 0x01;                
	packet[3] = rx_tx_addr[3];
	packet[4] = rx_tx_addr[2];
	uint16_t idx = ((state -FRSKY_BIND) % 10) * 5;
	packet[5] = idx;
	packet[6] = get_chan_num(idx++);
	packet[7] = get_chan_num(idx++);
	packet[8] = get_chan_num(idx++);
	packet[9] = get_chan_num(idx++);
	packet[10] = get_chan_num(idx++);
	packet[11] = 0x00;
	packet[12] = 0x00;
	packet[13] = 0x00;
	packet[14] = 0x00;
	packet[15] = 0x00;
	packet[16] = 0x00;
	packet[17] = 0x01;
}



static void frsky2way_data_frame()
{//pachet[4] is telemetry user frame counter(hub)
	//11 d7 2d 22 00 01 c9 c9 ca ca 88 88 ca ca c9 ca 88 88
	//11 57 12 00 00 01 f2 f2 f2 f2 06 06 ca ca ca ca 18 18
	packet[0] = 0x11;             //Length
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = counter;//	
	packet[4]=telemetry_counter;	

	packet[5] = 0x01;
	//
	packet[10] = 0;
	packet[11] = 0;
	packet[16] = 0;
	packet[17] = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		uint16_t value;
			value = convert_channel_frsky(i);
		if(i < 4)
		{
			packet[6+i] = value & 0xff;
			packet[10+(i>>1)] |= ((value >> 8) & 0x0f) << (4 *(i & 0x01));
		} 
		else
		{
			packet[8+i] = value & 0xff;
			packet[16+((i-4)>>1)] |= ((value >> 8) & 0x0f) << (4 * ((i-4) & 0x01));
		}
	}
} 

#endif
