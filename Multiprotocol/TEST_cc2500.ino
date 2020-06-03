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

#if defined(TEST_CC2500_INO)

#include "iface_nrf250k.h"

#define TEST_INITIAL_WAIT    500
#define TEST_PACKET_PERIOD   10000
#define TEST_PAYLOAD_SIZE    10
#define TEST_RF_NUM_CHANNELS 3

uint16_t TEST_callback()
{
	option=1;
	if(phase)
		XN297L_WritePayload(packet, TEST_PAYLOAD_SIZE);
	else
	{
		if(Channel_data[CH5]<CHANNEL_MIN_COMMAND)
			hopping_frequency_no=0;
		else if(Channel_data[CH5]>CHANNEL_MAX_COMMAND)
			hopping_frequency_no=2;
		else
			hopping_frequency_no=1;
		XN297L_Hopping(hopping_frequency_no);
		CC2500_WriteReg(CC2500_3E_PATABLE,convert_channel_8b(CH6));
		debugln("CH:%d, PWR:%d",hopping_frequency_no,convert_channel_8b(CH6));
	}
	phase ^= 1;
	return TEST_PACKET_PERIOD>>1;
}

uint16_t initTEST()
{
	option=1;

	hopping_frequency[0]=0;
	hopping_frequency[1]=40;
	hopping_frequency[2]=80;
	XN297L_Init();
	XN297L_HoppingCalib(TEST_RF_NUM_CHANNELS);	// Calibrate all channels
	XN297L_SetTXAddr((uint8_t*)"RADIO", 5);
	hopping_frequency_no = 0;
	phase=0;
	for(uint8_t i=0; i<TEST_PAYLOAD_SIZE; i++)
		packet[i]= i;
	return TEST_INITIAL_WAIT;
}

#endif
