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

#ifndef _IFACE_SX1276_H_
#define _IFACE_SX1276_H_

enum
{
    SX1276_00_FIFO                  = 0x00,        
    SX1276_01_OPMODE                = 0x01,
    SX1276_06_FRFMSB                = 0x06,
    SX1276_09_PACONFIG              = 0x09,
    SX1276_0B_OCP                   = 0x0B,
    SX1276_0C_LNA                   = 0x0C,
    SX1276_0D_FIFOADDRPTR           = 0x0D,
    SX1276_0E_FIFOTXBASEADDR        = 0x0E,
    SX1276_11_IRQFLAGSMASK          = 0x11,
    SX1276_1D_MODEMCONFIG1          = 0x1D,
    SX1276_1E_MODEMCONFIG2          = 0x1E,
    SX1276_20_PREAMBLEMSB           = 0x20,
    SX1276_22_PAYLOAD_LENGTH        = 0x22,
    SX1276_24_HOPPERIOD             = 0x24,
    SX1276_26_MODEMCONFIG3          = 0x26,
    SX1276_31_DETECTOPTIMIZE        = 0x31,
    SX1276_37_DETECTIONTHRESHOLD    = 0x37,
    SX1276_40_DIOMAPPING1           = 0x40,
    SX1276_42_VERSION               = 0x42,
    SX1276_4D_PADAC                 = 0x4D
};

enum
{
    SX1276_OPMODE_SLEEP = 0,
    SX1276_OPMODE_STDBY,
    SX1276_OPMODE_FSTX,
    SX1276_OPMODE_TX,
    SX1276_OPMODE_FSRX,
    SX1276_OPMODE_RXCONTINUOUS,
    SX1276_OPMODE_RXSINGLE,
    SX1276_OPMODE_CAD
};

enum
{
    SX1276_DETECT_OPTIMIZE_SF7_TO_SF12 = 0x03,
    SX1276_DETECT_OPTIMIZE_SF6 = 0x05
};

enum
{
    SX1276_MODEM_CONFIG1_BW_7_8KHZ = 0,
    SX1276_MODEM_CONFIG1_BW_10_4KHZ,
    SX1276_MODEM_CONFIG1_BW_15_6KHZ,
    SX1276_MODEM_CONFIG1_BW_20_8KHZ,
    SX1276_MODEM_CONFIG1_BW_31_25KHZ,
    SX1276_MODEM_CONFIG1_BW_41_7KHZ,
    SX1276_MODEM_CONFIG1_BW_62_5KHZ,
    SX1276_MODEM_CONFIG1_BW_125KHZ,
    SX1276_MODEM_CONFIG1_BW_250KHZ,
    SX1276_MODEM_CONFIG1_BW_500KHZ
};

enum
{
    SX1276_MODEM_CONFIG1_CODING_RATE_4_5 = 1,
    SX1276_MODEM_CONFIG1_CODING_RATE_4_6,
    SX1276_MODEM_CONFIG1_CODING_RATE_4_7,
    SX1276_MODEM_CONFIG1_CODING_RATE_4_8
};

enum
{
    SX1276_MODEM_DETECTION_THRESHOLD_SF7_TO_SF12 = 0x0A,
    SX1276_MODEM_DETECTION_THRESHOLD_SF6 = 0x0C,

};

#endif
