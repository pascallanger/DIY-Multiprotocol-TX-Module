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

#if defined JOYSWAY_A7105_INO
#include "iface_a7105.h"


#define EVEN_ODD 0x00
//#define EVEN_ODD 0x01

static uint8_t PROGMEM A7105_regs[] = {
    0x00, 0x62,   0xFF, 0x0f, 0x00,  0xFF ,  0xFF , 0x00,     0x00, 0x05, 0x00, 0x01, 0x00, 0xf5, 0x00, 0x15,
    0x9e, 0x4b, 0x00, 0x03, 0x56, 0x2b, 0x12, 0x4a,     0x02, 0x80, 0x80, 0x00, 0x0e, 0x91, 0x03, 0x0f,
    0x16, 0x2a, 0x00,  0xFF,    0xFF,   0xFF, 0x3a, 0x06,     0x1f, 0x47, 0x80, 0x01, 0x05, 0x45, 0x18, 0x00,
    0x01, 0x0f, 0x00
};
static void joysway_build_packet()
{
    int i;
    //-100% =~ 0x03e8
    //+100% =~ 0x07ca
    //Calculate:
    //Center = 0x5d9
    //1 %    = 5
    packet[0] = phase == 0 ? 0xdd : 0xff;
    packet[1] = (MProtocol_id >> 24) & 0xff;
    packet[2] = (MProtocol_id >> 16) & 0xff;
    packet[3] = (MProtocol_id >>  8) & 0xff;
    packet[4] = (MProtocol_id >>  0) & 0xff;
    packet[5] = 0x00;
    static const int chmap[4] = {6, 7, 10, 11};
    for (i = 0; i < 4; i++) {
//        if (i >= Model.num_channels) {            packet[chmap[i]] = 0x64;            continue;        }
        packet[chmap[i]] = map(limit_channel_100(i),servo_min_100,servo_max_100,0,204);
    }
    packet[8] = 0x64;
    packet[9] = 0x64;
    packet[12] = 0x64;
    packet[13] = 0x64;
    packet[14] = phase == 0 ? 0x30 : 0xaa;
    uint8_t value = 0;
    for (int i = 0; i < 15; i++) {        value += packet[i];    }
    packet[15] = value;
}

static uint16_t joysway_cb()
{
    if (phase == 254) {
        phase = 0;
        A7105_WriteID(0x5475c52a);
        hopping_frequency_no = 0x0a;
    } else if (phase == 2) {
        A7105_WriteID(MProtocol_id);
        hopping_frequency_no = 0x30;
    } else {
        if ((phase & 0x01) ^ EVEN_ODD) {
            hopping_frequency_no = 0x30;
        } else {
            hopping_frequency_no = rf_ch_num;
        }
    }
    if (! ((phase & 0x01) ^ EVEN_ODD)) {
        rf_ch_num++;
        if (rf_ch_num == 0x45)
            rf_ch_num = 0x30;
    }
    joysway_build_packet();
    A7105_Strobe(A7105_STANDBY);
    A7105_WriteData(16, hopping_frequency_no);
    phase++;
    return 6000;
}

static uint16_t JOYSWAY_Setup() {
    int i;
    u8 if_calibration1;
    //u8 vco_calibration0;
    //u8 vco_calibration1;

    counter = 0;
    next_ch = 0x30;

    for (i = 0; i < 0x33; i++) {
		uint8_t val=pgm_read_byte_near(&A7105_Regs[i]);
		if( val != 0xFF)
			A7105_WriteReg(i, val);
	}
    A7105_WriteID(0x5475c52a);

    A7105_Strobe(A7105_PLL);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
	while(A7105_ReadReg(A7105_02_CALC));			// Wait for calibration to end
    A7105_Strobe(A7105_STANDBY);

    //VCO Current Calibration
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet
    A7105_WriteReg(0x25, 0x09); //Recomended calibration from A7105 Datasheet

    A7105_WriteID(MProtocol_id);
    A7105_Strobe(A7105_PLL);
    A7105_WriteReg(0x02, 1);
	while(A7105_ReadReg(A7105_02_CALC));			// Wait for calibration to end
    A7105_Strobe(A7105_STANDBY);
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet
    A7105_WriteReg(0x25, 0x09); //Recomended calibration from A7105 Datasheet

    A7105_SetTxRxMode(TX_EN);
    A7105_SetPower();

    A7105_Strobe(A7105_STANDBY);
	return 2400;
}
#endif
