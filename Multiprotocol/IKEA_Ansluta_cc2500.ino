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
// This module makes it possible to bind to and control the IKEA "Ansluta" line
// of remote-controlled lights.
// To bind, first switch the receiver into binding mode, then the TX.
// Once bound, the TX can send one of three commands:
//   lights off, lights dimmed 50% and lights on.
// Those are mapped to throttle ranges 0..0x55, 0x56..0xAA, 0xAB..0xFF.
#if defined(IKEAANSLUTA_CC2500_INO)

#include "iface_cc2500.h"

#define IKEAANSLUTA_BIND_COUNT	30  // ~ 2sec autobind/65ms per loop = 30 binding packets

// Commands
#define IKEAANSLUTA_LIGHT_OFF 0x01
#define IKEAANSLUTA_LIGHT_DIM 0x02  // 50% dimmed light
#define IKEAANSLUTA_LIGHT_ON  0x03
#define IKEAANSLUTA_PAIR 0xFF

void IKEAANSLUTA_send_command(uint8_t command){
      CC2500_Strobe(CC2500_SIDLE);
      packet[4] = option;
      packet[5] = command;
      CC2500_WriteData(packet, 8);
}

uint16_t IKEAANSLUTA_callback(void)
{
    if (bind_counter) {
        IKEAANSLUTA_send_command(IKEAANSLUTA_PAIR);
        if(--bind_counter == 0) {
            BIND_DONE;
            CC2500_SetPower();
        }
    }
    else {
        uint8_t throttle = convert_channel_8b(THROTTLE);
        uint8_t cmd = throttle <= 0x55 ? IKEAANSLUTA_LIGHT_OFF : 
                      throttle <= 0xAA ? IKEAANSLUTA_LIGHT_DIM :
                      IKEAANSLUTA_LIGHT_ON;
        IKEAANSLUTA_send_command(cmd);
    }
    return 65535; // 65ms loop cycle is more than enough here (we could make it even longer if not for uint16_t)
}

// Register initialization values as a continuous memory block (to save on flash memory)
const PROGMEM uint8_t IKEAANSLUTA_init_values[] = {
  0xFF, // CC2500_06_PKTLEN
  0x04, // CC2500_07_PKTCTRL1
  0x05, // CC2500_08_PKTCTRL0
  0x00, // CC2500_09_ADDR  (unused, default)
  0x10, // CC2500_0A_CHANNR
  0x09, // CC2500_0B_FSCTRL1
  0x00, // CC2500_0C_FSCTRL0
  0x5D, // CC2500_0D_FREQ2
  0x93, // CC2500_0E_FREQ1
  0xB1, // CC2500_0F_FREQ0
  0x2D, // CC2500_10_MDMCFG4
  0x3B, // CC2500_11_MDMCFG3
  0x73, // CC2500_12_MDMCFG2
  0xA2, // CC2500_13_MDMCFG1
  0xF8, // CC2500_14_MDMCFG0
  0x01, // CC2500_15_DEVIATN
  0x07, // CC2500_16_MCSM2
  0x30, // CC2500_17_MCSM1
  0x18, // CC2500_18_MCSM0
  0x1D, // CC2500_19_FOCCFG
  0x1C, // CC2500_1A_BSCFG
  0xC7, // CC2500_1B_AGCCTRL2
  0x00, // CC2500_1C_AGCCTRL1
  0xB2, // CC2500_1D_AGCCTRL0
  0x87, // CC2500_1E_WOREVT1 (unused, default)
  0x6b, // CC2500_1F_WOREVT0 (unused, default)
  0xf8, // CC2500_20_WORCTRL (unused, default)
  0xB6, // CC2500_21_FREND1
  0x10, // CC2500_22_FREND0
  0xEA, // CC2500_23_FSCAL3
  0x0A, // CC2500_24_FSCAL2
  0x00, // CC2500_25_FSCAL1
  0x11, // CC2500_26_FSCAL0
  0x41, // CC2500_27_RCCTRL1
  0x00, // CC2500_28_RCCTRL0
};

void IKEAANSLUTA_init(void)
{
    if (IS_BIND_DONE) bind_counter = 0;
    else bind_counter = IKEAANSLUTA_BIND_COUNT;

    // All packets we send are the same
    packet[0] = 0x06;
    packet[1] = 0x55;
    packet[2] = 0x01;
    // Bytes 3&4 are the transmitter address (to which the RX binds)
    // Byte 5 is the command.
    packet[3] = rx_tx_addr[3];  // <pseudorandom tx-fixed value> + <rx_num>
    // packet[4] = option;
    // packet[5] = 0x00; // Command goes here
    packet[6] = 0xAA;
    packet[7] = 0xFF;

    // Configure & initialize CC2500
	for (uint8_t i = 0; i <= CC2500_28_RCCTRL0-CC2500_06_PKTLEN; ++i)
		CC2500_WriteReg(CC2500_06_PKTLEN+i, pgm_read_byte_near(&IKEAANSLUTA_init_values[i]));    
    CC2500_SetTxRxMode(TX_EN);
    CC2500_SetPower();
}

#endif
