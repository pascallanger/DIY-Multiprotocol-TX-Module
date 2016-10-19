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

static void joysway_build_packet()
{
    int i;
    //-100% =~ 0x03e8
    //+100% =~ 0x07ca
    //Calculate:
    //Center = 0x5d9
    //1 %    = 5
    packet[0] = phase == 0 ? 0xdd : 0xff;
    packet[1] = (binding_idx >> 24) & 0xff;
    packet[2] = (binding_idx >> 16) & 0xff;
    packet[3] = (binding_idx >>  8) & 0xff;
    packet[4] = (binding_idx >>  0) & 0xff;
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
        A7105_WriteID(binding_idx);
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
	binding_idx = MProtocol_id_master | 0xf8000000;
	A7105_Init(INIT_JOYSWAY);
	return 2400;
}
#endif
