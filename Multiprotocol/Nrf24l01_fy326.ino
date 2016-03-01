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


#if defined(FY326_NRF24L01_INO)
#include "iface_nrf24l01.h"

#define INITIAL_WAIT    500
#define FY326_PERIOD   1500  // Timeout for callback in uSec
#define FY326_CHKTIME  300   // Time to wait if packet not yet received or sent
#define FY326_SIZE     15
#define FY326_BIND_COUNT      16

#define CHANNEL_FLIP      AUX1
#define CHANNEL_HEADLESS  AUX2
#define CHANNEL_RTH       AUX3
#define CHANNEL_CALIBRATE AUX4
#define CHANNEL_EXPERT    AUX5

// frequency channel management
#define RF_BIND_CHANNEL    0x17
#define NUM_RF_CHANNELS    5
static uint8_t current_chan;
static uint8_t rf_chans[NUM_RF_CHANNELS];
static uint8_t txid[5];
static uint8_t rxid;

enum {
    FY326_INIT1 = 0,
    FY326_BIND1,
    FY326_BIND2,
    FY326_DATA,
    FY319_INIT1,
    FY319_BIND1,
    FY319_BIND2,
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (PPM_MAX - PPM_MIN)
static uint8_t scale_channel(uint8_t ch, uint8_t destMin, uint8_t destMax)
{
    uint32_t chanval = Servo_data[ch];
    uint32_t range = destMax - destMin;

    if      (chanval < PPM_MIN) chanval = PPM_MIN;
    else if (chanval > PPM_MAX) chanval = PPM_MAX;
    return (range * (chanval - PPM_MIN)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Servo_data[ch] > PPM_MIN_COMMAND ? mask : 0)
#define CHAN_TO_TRIM(chanval) ((uint8_t)(((uint16_t)chanval/10)-10))  // scale to [-10,10]. [-20,20] caused problems.
static void send_packet(uint8_t bind)
{
    packet[0] = txid[3];
    if (bind) {
        packet[1] = 0x55;
    } else {
        packet[1] = GET_FLAG(CHANNEL_HEADLESS,  0x80)
                  | GET_FLAG(CHANNEL_RTH,       0x40)
                  | GET_FLAG(CHANNEL_FLIP,      0x02)
                  | GET_FLAG(CHANNEL_CALIBRATE, 0x01)
                  | GET_FLAG(CHANNEL_EXPERT,    4);
    }
    packet[2]  = 200 - scale_channel(AILERON, 0, 200);  // aileron  1
    packet[3]  = scale_channel(ELEVATOR, 0, 200);        // elevator  2
    packet[4]  = 200 - scale_channel(RUDDER, 0, 200);  // rudder  4
    packet[5]  = scale_channel(THROTTLE, 0, 200);        // throttle  3
    if(sub_protocol == FY319) {
        packet[6] = 255 - scale_channel(AILERON, 0, 255);
        packet[7] = scale_channel(ELEVATOR, 0, 255);
        packet[8] = 255 - scale_channel(RUDDER, 0, 255);
    }
    else {
        packet[6]  = txid[0];
        packet[7]  = txid[1];
        packet[8]  = txid[2];
    }
    packet[9]  = CHAN_TO_TRIM(packet[2]); // aileron_trim;
    packet[10] = CHAN_TO_TRIM(packet[3]); // elevator_trim;
    packet[11] = CHAN_TO_TRIM(packet[4]); // rudder_trim;
    packet[12] = 0; // throttle_trim;
    packet[13] = rxid;
    packet[14] = txid[4];

    if (bind) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    } else {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_chans[current_chan++]);
        current_chan %= NUM_RF_CHANNELS;
    }

    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    NRF24L01_WritePayload(packet, FY326_SIZE);
}

static void fy326_init()
{
  uint8_t rx_tx_addr[] = {0x15, 0x59, 0x23, 0xc6, 0x29};

    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    if(sub_protocol == FY319)
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // Five-byte rx/tx address
    else
        NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);   // Three-byte rx/tx address
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr, sizeof(rx_tx_addr));
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, sizeof(rx_tx_addr));
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, FY326_SIZE);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
    NRF24L01_SetPower();
    
    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3f);
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
}

static uint16_t fy326_callback()
{
    uint8_t i;
    switch (phase) {
    case FY319_INIT1:
        NRF24L01_SetTxRxMode(TXRX_OFF);
        NRF24L01_FlushRx();
        NRF24L01_SetTxRxMode(RX_EN);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
        phase = FY319_BIND1;
		BIND_IN_PROGRESS;
        return FY326_CHKTIME;
        break;
        
    case FY319_BIND1:
        if(NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
            NRF24L01_ReadPayload(packet, FY326_SIZE);
            rxid = packet[13];
            packet[0] = txid[3];
            packet[1] = 0x80;
            packet[14]= txid[4];
            bind_counter = FY326_BIND_COUNT;
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
            NRF24L01_FlushTx();
            bind_counter = 255;
            for(i=2; i<6; i++)
                packet[i] = rf_chans[0];
            phase = FY319_BIND2;
        }
        return FY326_CHKTIME;
        break;
    
    case FY319_BIND2:
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
        NRF24L01_FlushTx();
        NRF24L01_WritePayload(packet, FY326_SIZE);
        if(bind_counter == 250)
            packet[1] = 0x40;
        if(--bind_counter == 0) {
            BIND_DONE;
            phase = FY326_DATA;
        }
        break;
    
    case FY326_INIT1:
        bind_counter = FY326_BIND_COUNT;
        phase = FY326_BIND2;
        send_packet(1);
        return FY326_CHKTIME;
        break;

    case FY326_BIND1:
        if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) { // RX fifo data ready
            NRF24L01_ReadPayload(packet, FY326_SIZE);
            rxid = packet[13];
            txid[0] = 0xaa;
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            BIND_DONE;
            phase = FY326_DATA;
        } else if (bind_counter-- == 0) {
            bind_counter = FY326_BIND_COUNT;
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            send_packet(1);
            phase = FY326_BIND2;
            return FY326_CHKTIME;
        }
        break;

    case FY326_BIND2:
        if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_TX_DS)) { // TX data sent
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            phase = FY326_BIND1;
        } else {
            return FY326_CHKTIME;
        }
        break;

    case FY326_DATA:
        send_packet(0);
        break;
    }
    return FY326_PERIOD;
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void fy_txid()
{
    txid[0] = (MProtocol_id_master >> 24) & 0xFF;
    txid[1] = ((MProtocol_id_master >> 16) & 0xFF);
    txid[2] = (MProtocol_id_master >> 8) & 0xFF;
    txid[3] = MProtocol_id_master & 0xFF;
//    for (uint8_t i = 0; i < sizeof(MProtocol_id_master); ++i) rand32_r(&MProtocol_id_master, 0);
    txid[4] = MProtocol_id_master & 0xFF;

    rf_chans[0] =         txid[0] & 0x0F;
    rf_chans[1] = 0x10 + (txid[0] >> 4);
    rf_chans[2] = 0x20 + (txid[1] & 0x0F);
    rf_chans[3] = 0x30 + (txid[1] >> 4);
    rf_chans[4] = 0x40 + (txid[2] >> 4);
  
    if(sub_protocol == FY319) {        
        for(uint8_t i=0; i<5; i++)
            rf_chans[i] = txid[0] & ~0x80;
    }
}

static uint16_t FY326_setup()
{
    BIND_IN_PROGRESS;
    rxid = 0xaa;
    if(sub_protocol == FY319)
        phase = FY319_INIT1;
    else
        phase = FY326_INIT1;
    bind_counter = FY326_BIND_COUNT;
    fy_txid();
    fy326_init();
    return INITIAL_WAIT;
}
#endif

