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


#if defined(HonTai_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define BIND_HT_COUNT 80
#define PACKET_HT_PERIOD    13500 // Timeout for callback in uSec
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf

#define INITIAL_HT_WAIT       500
#define BIND_HT_PACKET_SIZE   10
#define PACKET_HT_SIZE        12
#define RF_BIND_HT_CHANNEL    0

enum {
    FORMAT_HONTAI = 0,
    FORMAT_JJRCX1,
};


#define CHANNEL_LED         AUX1
#define CHANNEL_ARM         AUX1    // for JJRC X1
#define CHANNEL_FLIP        AUX2
#define CHANNEL_PICTURE     AUX3
#define CHANNEL_VIDEO       AUX4
#define CHANNEL_HEADLESS    AUX5
#define CHANNEL_RTH         AUX6
#define CHANNEL_CALIBRATE   AUX7

enum {
    HonTai_INIT1 = 0,
    HonTai_BIND2,
    HonTai_DATA
};

static uint8_t ht_txid[5];

static uint8_t rf_chan = 0; 
static uint8_t rf_channels[][3] = {{0x05, 0x19, 0x28},     // Hontai
                                    {0x0a, 0x1e, 0x2d}};    // JJRC X1
static uint8_t rx_tx_ht_addr[] = {0xd2, 0xb5, 0x99, 0xb3, 0x4a};
static uint8_t addr_vals[4][16] = {
                    {0x24, 0x26, 0x2a, 0x2c, 0x32, 0x34, 0x36, 0x4a, 0x4c, 0x4e, 0x54, 0x56, 0x5a, 0x64, 0x66, 0x6a},
                    {0x92, 0x94, 0x96, 0x9a, 0xa4, 0xa6, 0xac, 0xb2, 0xb4, 0xb6, 0xca, 0xcc, 0xd2, 0xd4, 0xd6, 0xda},
                    {0x93, 0x95, 0x99, 0x9b, 0xa5, 0xa9, 0xab, 0xad, 0xb3, 0xb5, 0xc9, 0xcb, 0xcd, 0xd3, 0xd5, 0xd9},
                    {0x25, 0x29, 0x2b, 0x2d, 0x33, 0x35, 0x49, 0x4b, 0x4d, 0x59, 0x5b, 0x65, 0x69, 0x6b, 0x6d, 0x6e}};

// proudly swiped from http://www.drdobbs.com/implementing-the-ccitt-cyclical-redundan/199904926
#define POLY 0x8408
static uint16_t crc16(uint8_t *data_p, uint32_t length)
{
    uint8_t i;
    uint32_t data;
    uint32_t crc;
     
    crc = 0xffff;
     
    if (length == 0) return (~crc);
     
    length -= 2;
    do {
        for (i = 0, data = (uint8_t)0xff & *data_p++;
             i < 8;
             i++, data >>= 1) {
                 if ((crc & 0x0001) ^ (data & 0x0001))
                     crc = (crc >> 1) ^ POLY;
                 else
                     crc >>= 1;
        }
    } while (--length);
     
    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xFF);
    *data_p++ = crc >> 8;
    *data_p   = crc & 0xff;
    return crc;
}

#define CHAN_RANGE (PPM_MAX - PPM_MIN)
static uint8_t scale_HT_channel(uint8_t ch, uint8_t start, uint8_t end)
{
    uint32_t range = end - start;
    uint32_t chanval = Servo_data[ch];

    if      (chanval < PPM_MIN) chanval = PPM_MIN;
    else if (chanval > PPM_MAX) chanval = PPM_MAX;

    uint32_t round = range < 0 ? 0 : CHAN_RANGE / range;   // channels round up
    if (start < 0) round = CHAN_RANGE / range / 2;    // trims zero centered around zero
    return (range * (chanval - PPM_MIN + round)) / CHAN_RANGE + start;
}

#define GET_FLAG(ch, mask) (Servo_data[ch] > 0 ? mask : 0)
static void send_HT_packet(uint8_t bind)
{
    if (bind) {
      memcpy(packet, ht_txid, 5);
      memset(&packet[5], 0, 3);
    } else {
      if (sub_protocol == FORMAT_HONTAI) {
          packet[0] = 0x0b;
      } else {
          packet[0] = GET_FLAG(CHANNEL_ARM, 0x02);
      }
      packet[1] = 0x00;
      packet[2] = 0x00;
    packet[3] = (scale_HT_channel(THROTTLE, 0, 127) << 1)    // throttle
                | GET_FLAG(CHANNEL_PICTURE, 0x01);
    packet[4] = scale_HT_channel(AILERON, 63, 0);            // aileron
      if (sub_protocol == FORMAT_HONTAI) {
          packet[4] |= GET_FLAG(CHANNEL_RTH, 0x80)
                     | GET_FLAG(CHANNEL_HEADLESS, 0x40);
      } else {
          packet[4] |= 0x80;                                // not sure what this bit does
      }
      packet[5] = scale_channel(CHANNEL2, 0, 63)            // elevator
                | GET_FLAG(CHANNEL_CALIBRATE, 0x80)
                | GET_FLAG(CHANNEL_FLIP, 0x40);
    packet[6] = scale_HT_channel(RUDDER, 0, 63)            // rudder
                | GET_FLAG(CHANNEL_VIDEO, 0x80);
      packet[7] = scale_HT_channel(AILERON, -16, 16);         // aileron trim
      if (sub_protocol == FORMAT_HONTAI) {
          packet[8] = scale_HT_channel(RUDDER, -16, 16);         // rudder trim
      } else {
          packet[8] = 0xc0    // always in expert mode
                    | GET_FLAG(CHANNEL_RTH, 0x02)
                    | GET_FLAG(CHANNEL_HEADLESS, 0x01);
      }
      packet[9] = scale_HT_channel(ELEVATOR, -16, 16);         // elevator trim
    }
  crc16(packet, bind ? BIND_HT_PACKET_SIZE : PACKET_HT_SIZE);
    
    // Power on, TX mode, 2byte CRC
    if (sub_protocol == FORMAT_HONTAI) {
        XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    } else {
        NRF24L01_SetTxRxMode(TX_EN);
    }

  NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? RF_BIND_HT_CHANNEL : rf_channels[sub_protocol][rf_chan++]);
  rf_chan %= sizeof(rf_channels);

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    if (sub_protocol == FORMAT_HONTAI) {
        XN297_WritePayload(packet, bind ? BIND_HT_PACKET_SIZE : PACKET_HT_SIZE);
    } else {
        NRF24L01_WritePayload(packet, bind ? BIND_HT_PACKET_SIZE : PACKET_HT_SIZE);
    }

  NRF24L01_SetPower();
}

static void ht_init()
{
    NRF24L01_Initialize();

    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5); 
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7); 
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7); 

    if (sub_protocol == FORMAT_HONTAI) {
        XN297_SetTXAddr(rx_tx_ht_addr, sizeof(rx_tx_ht_addr));
    } else {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_ht_addr, sizeof(rx_tx_ht_addr));
    }

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower();
    NRF24L01_Activate(0x73);                              // Activate feature register
    if (sub_protocol == FORMAT_HONTAI) {
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);  // no retransmits
        NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
        NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x00);
        NRF24L01_Activate(0x73);                          // Deactivate feature register
    } else {
        NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xff);  // JJRC uses dynamic payload length
        NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);       // match other stock settings even though AA disabled...
        NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
    }
}

static void ht_init2()
{
    uint8_t data_tx_addr[] = {0x2a, 0xda, 0xa5, 0x25, 0x24};

    data_tx_addr[0] = addr_vals[0][ ht_txid[3]       & 0x0f];
    data_tx_addr[1] = addr_vals[1][(ht_txid[3] >> 4) & 0x0f];
    data_tx_addr[2] = addr_vals[2][ ht_txid[4]       & 0x0f];
    data_tx_addr[3] = addr_vals[3][(ht_txid[4] >> 4) & 0x0f];

    if (sub_protocol == FORMAT_HONTAI) {
        XN297_SetTXAddr(data_tx_addr, sizeof(data_tx_addr));
    } else {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, data_tx_addr, sizeof(data_tx_addr));
    }
}

static uint16_t ht_callback()
{
    switch (phase) {
    case HonTai_INIT1:
        phase = HonTai_BIND2;
        break;
    case HonTai_BIND2:
        if (counter == 0) {
            ht_init2();
            phase = HonTai_DATA;
              BIND_DONE;
        } else {
              send_HT_packet(1);
            counter -= 1;
        }
        break;

    case HonTai_DATA:
          send_HT_packet(0);
        break;
    }
    return PACKET_HT_PERIOD;
}

static uint16_t ht_setup()
{
    counter = BIND_HT_COUNT;

    if (sub_protocol == FORMAT_HONTAI) {
        ht_txid[0] = 0x4c; // first three bytes some kind of model id? - set same as stock tx
        ht_txid[1] = 0x4b;
        ht_txid[2] = 0x3a;
    } else {
        ht_txid[0] = 0x4b; // JJRC X1
        ht_txid[1] = 0x59;
        ht_txid[2] = 0x3a;
    }
    ht_txid[3] = (MProtocol_id >> 8 ) & 0xff;
    ht_txid[4] = MProtocol_id & 0xff;
  
    ht_init();
    phase = HonTai_INIT1;

    return INITIAL_HT_WAIT;
}
#endif

