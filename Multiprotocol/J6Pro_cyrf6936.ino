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

#if defined(J6PRO_CYRF6936_INO)

#include "iface_cyrf6936.h"

enum PktState {
    J6PRO_BIND,
    J6PRO_BIND_01,
    J6PRO_BIND_03_START,
    J6PRO_BIND_03_CHECK,
    J6PRO_BIND_05_1,
    J6PRO_BIND_05_2,
    J6PRO_BIND_05_3,
    J6PRO_BIND_05_4,
    J6PRO_BIND_05_5,
    J6PRO_BIND_05_6,
    J6PRO_CHANSEL,
    J6PRO_CHAN_1,
    J6PRO_CHAN_2,
    J6PRO_CHAN_3,
    J6PRO_CHAN_4,
};

const uint8_t PROGMEM j6pro_bind_sop_code[] = {0x62, 0xdf, 0xc1, 0x49, 0xdf, 0xb1, 0xc0, 0x49};
const uint8_t j6pro_data_code[] = {0x02, 0xf9, 0x93, 0x97, 0x02, 0xfa, 0x5c, 0xe3, 0x01, 0x2b, 0xf1, 0xdb, 0x01, 0x32, 0xbe, 0x6f};

static void __attribute__((unused)) j6pro_build_bind_packet()
{
    packet[0] = 0x01;  //Packet type
    packet[1] = 0x01;  //FIXME: What is this? Model number maybe?
    packet[2] = 0x56;  //FIXME: What is this?
    packet[3] = cyrfmfg_id[0];
    packet[4] = cyrfmfg_id[1];
    packet[5] = cyrfmfg_id[2];
    packet[6] = cyrfmfg_id[3];
    packet[7] = cyrfmfg_id[4];
    packet[8] = cyrfmfg_id[5];
}

static void __attribute__((unused)) j6pro_build_data_packet()
{
    uint8_t i;
    uint32_t upperbits = 0;
    uint16_t value;
    packet[0] = 0xaa; //FIXME what is this?
    for (i = 0; i < 12; i++)
    {
        value = convert_channel_10b(CH_AETR[i]);
        packet[i+1] = value & 0xff;
        upperbits |= (value >> 8) << (i * 2);
    }
    packet[13] = upperbits & 0xff;
    packet[14] = (upperbits >> 8) & 0xff;
    packet[15] = (upperbits >> 16) & 0xff;
}

static void __attribute__((unused)) j6pro_cyrf_init()
{
    /* Initialise CYRF chip */
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3c);
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
    //CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
    //CYRF_SetPower(0x05);
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);
    CYRF_SetPower(0x28);
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0e);
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xee);
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x00);
    CYRF_ConfigDataCode(j6pro_data_code, 16);
    CYRF_WritePreamble(0x333302);

    CYRF_GetMfgData(cyrfmfg_id);
	//Model match
	cyrfmfg_id[3]+=RX_num;
}

static void __attribute__((unused)) cyrf_bindinit()
{
    /* Use when binding */
    CYRF_SetPower(0x28); //Deviation using max power, replaced by bind power...
    //CYRF_ConfigRFChannel(0x52);
    CYRF_PROGMEM_ConfigSOPCode(j6pro_bind_sop_code);
    CYRF_ConfigCRCSeed(0x0000);
    //CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);
    //CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);
    //CYRF_ConfigRFChannel(0x52);
    //CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
    //CYRF_WriteRegister(CYRF_02_TX_CTRL, 0x40);
    j6pro_build_bind_packet();
}

static void __attribute__((unused)) cyrf_datainit()
{
    /* Use when already bound */
    uint8_t sop_idx = (0xff & (cyrfmfg_id[0] + cyrfmfg_id[1] + cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[5])) % 19;
    uint16_t crc =  (0xff & (cyrfmfg_id[1] - cyrfmfg_id[4] + cyrfmfg_id[5])) |
                   ((0xff & (cyrfmfg_id[2] + cyrfmfg_id[3] - cyrfmfg_id[4] + cyrfmfg_id[5])) << 8);
    //CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
    CYRF_PROGMEM_ConfigSOPCode(DEVO_j6pro_sopcodes[sop_idx]);
    CYRF_ConfigCRCSeed(crc);
}

static void __attribute__((unused)) j6pro_set_radio_channels()
{
    //FIXME: Query free channels
    //lowest channel is 0x08, upper channel is 0x4d?
    CYRF_FindBestChannels(hopping_frequency, 3, 5, 8, 77);
    hopping_frequency[3] = hopping_frequency[0];
}

uint16_t ReadJ6Pro()
{
    uint16_t start;

    switch(phase)
    {
        case J6PRO_BIND:
            cyrf_bindinit();
            phase = J6PRO_BIND_01;
            //no break because we want to send the 1st bind packet now
        case J6PRO_BIND_01:
            CYRF_ConfigRFChannel(0x52);
            CYRF_SetTxRxMode(TX_EN);
            //CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
            CYRF_WriteDataPacketLen(packet, 0x09);
            phase = J6PRO_BIND_03_START;
            return 3000; //3msec
        case J6PRO_BIND_03_START:
            start=(uint16_t)micros();
            while ((uint16_t)((uint16_t)micros()-(uint16_t)start) < 500)				// Wait max 500µs
				if((CYRF_ReadRegister(CYRF_02_TX_CTRL) & 0x80) == 0x00)
					break;										// Packet transmission complete
			CYRF_ConfigRFChannel(0x53);
            CYRF_SetTxRxMode(RX_EN);
            //CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4a);
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);
            phase = J6PRO_BIND_03_CHECK;
            return 30000; //30msec
        case J6PRO_BIND_03_CHECK:
            {
            uint8_t rx = CYRF_ReadRegister(CYRF_07_RX_IRQ_STATUS);
            if((rx & 0x1a) == 0x1a) {
                rx = CYRF_ReadRegister(CYRF_0A_RX_LENGTH);
                if(rx == 0x0f) {
                    rx = CYRF_ReadRegister(CYRF_09_RX_COUNT);
                    if(rx == 0x0f) {
                        //Expected and actual length are both 15
                        CYRF_ReadDataPacketLen(packet, rx);
                        if (packet[0] == 0x03 &&
                            packet[3] == cyrfmfg_id[0] &&
                            packet[4] == cyrfmfg_id[1] &&
                            packet[5] == cyrfmfg_id[2] &&
                            packet[6] == cyrfmfg_id[3] &&
                            packet[7] == cyrfmfg_id[4] &&
                            packet[8] == cyrfmfg_id[5])
                        {
                            //Send back Ack
                            packet[0] = 0x05;
                            CYRF_ConfigRFChannel(0x54);
                            CYRF_SetTxRxMode(TX_EN);
                            phase = J6PRO_BIND_05_1;
                            return 2000; //2msec
                         }
                    }
                }
            }
            phase = J6PRO_BIND_01;
            return 500;
            }
        case J6PRO_BIND_05_1:
        case J6PRO_BIND_05_2:
        case J6PRO_BIND_05_3:
        case J6PRO_BIND_05_4:
        case J6PRO_BIND_05_5:
        case J6PRO_BIND_05_6:
            //CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x24);
            CYRF_WriteDataPacketLen(packet, 0x0f);
            phase = phase + 1;
            return 4600; //4.6msec
        case J6PRO_CHANSEL:
            BIND_DONE;
            j6pro_set_radio_channels();
            cyrf_datainit();
            phase = J6PRO_CHAN_1;
        case J6PRO_CHAN_1:
            //Keep transmit power updated
            CYRF_SetPower(0x28);
            j6pro_build_data_packet();
            //return 3400;
        case J6PRO_CHAN_2:
            //return 3500;
        case J6PRO_CHAN_3:
            //return 3750
        case J6PRO_CHAN_4:
            CYRF_ConfigRFChannel(hopping_frequency[phase - J6PRO_CHAN_1]);
            CYRF_SetTxRxMode(TX_EN);
            CYRF_WriteDataPacket(packet);
            if (phase == J6PRO_CHAN_4) {
                phase = J6PRO_CHAN_1;
                return 13900;
            }
            phase = phase + 1;
            return 3550;
    }
    return 0;
}

uint16_t initJ6Pro()
{
    j6pro_cyrf_init();

	if(IS_BIND_IN_PROGRESS)
        phase = J6PRO_BIND;
    else
        phase = J6PRO_CHANSEL;
    return 2400;
}

#endif
