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

#ifndef _IFACE_CYRF6936_H_
#define _IFACE_CYRF6936_H_
enum {
    CYRF_00_CHANNEL        = 0x00,
    CYRF_01_TX_LENGTH      = 0x01,
    CYRF_02_TX_CTRL        = 0x02,
    CYRF_03_TX_CFG         = 0x03,
    CYRF_04_TX_IRQ_STATUS  = 0x04,
    CYRF_05_RX_CTRL        = 0x05,
    CYRF_06_RX_CFG         = 0x06,
    CYRF_07_RX_IRQ_STATUS  = 0x07,
    CYRF_08_RX_STATUS      = 0x08,
    CYRF_09_RX_COUNT       = 0x09,
    CYRF_0A_RX_LENGTH      = 0x0A,
    CYRF_0B_PWR_CTRL       = 0x0B,
    CYRF_0C_XTAL_CTRL      = 0x0C,
    CYRF_0D_IO_CFG         = 0x0D,
    CYRF_0E_GPIO_CTRL      = 0x0E,
    CYRF_0F_XACT_CFG       = 0x0F,
    CYRF_10_FRAMING_CFG    = 0x10,
    CYRF_11_DATA32_THOLD   = 0x11,
    CYRF_12_DATA64_THOLD   = 0x12,
    CYRF_13_RSSI           = 0x13,
    CYRF_14_EOP_CTRL       = 0x14,
    CYRF_15_CRC_SEED_LSB   = 0x15,
    CYRF_16_CRC_SEED_MSB   = 0x16,
    CYRF_17_TX_CRC_LSB     = 0x17,
    CYRF_18_TX_CRC_MSB     = 0x18,
    CYRF_19_RX_CRC_LSB     = 0x19,
    CYRF_1A_RX_CRC_MSB     = 0x1A,
    CYRF_1B_TX_OFFSET_LSB  = 0x1B,
    CYRF_1C_TX_OFFSET_MSB  = 0x1C,
    CYRF_1D_MODE_OVERRIDE  = 0x1D,
    CYRF_1E_RX_OVERRIDE    = 0x1E,
    CYRF_1F_TX_OVERRIDE    = 0x1F,
    /*Register Files */
    CYRF_20_TX_BUFFER      = 0x20,
    CYRF_21_RX_BUFFER      = 0x21,
    CYRF_22_SOP_CODE       = 0x22,
    CYRF_23_DATA_CODE      = 0x23,
    CYRF_24_PREAMBLE       = 0x24,
    CYRF_25_MFG_ID         = 0x25,
    /*****************/
    CYRF_26_XTAL_CFG       = 0x26,
    CYRF_27_CLK_OVERRIDE   = 0x27,
    CYRF_28_CLK_EN         = 0x28,
    CYRF_29_RX_ABORT       = 0x29,
    CYRF_32_AUTO_CAL_TIME  = 0x32,
    CYRF_35_AUTOCAL_OFFSET = 0x35,
    CYRF_39_ANALOG_CTRL    = 0x39,
};

enum CYRF_PWR {
    CYRF_PWR_100MW,
    CYRF_PWR_10MW,
    CYRF_PWR_DEFAULT,
};



/* SPI CYRF6936 */
/*
void CYRF_Initialize();
int CYRF_Reset();
void CYRF_GetMfgData(u8 data[]);

void CYRF_SetTxRxMode(enum TXRX_State);
void CYRF_ConfigRFChannel(u8 ch);
void CYRF_SetPower(u8 power);
void CYRF_ConfigCRCSeed(u16 crc);
static void CYRF_StartReceive();
void CYRF_ConfigSOPCode(const u8 *sopcodes);
void CYRF_ConfigDataCode(const u8 *datacodes, u8 len);
static u8 CYRF_ReadRSSI(u32 dodummyread);
static void CYRF_ReadDataPacket(u8 dpbuffer[]); 
void CYRF_WriteDataPacket(const u8 dpbuffer[]);
static void CYRF_WriteDataPacketLen(const u8 dpbuffer[], u8 len);
void CYRF_WriteRegister(u8 address, u8 data);
u8 CYRF_ReadRegister(u8 address);
void CYRF_WritePreamble(u32 preamble);
u8 CYRF_MaxPower();
void CYRF_FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 minchan, u8 maxchan);
*/
#endif
