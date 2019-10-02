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

/************************/
/** Firmware Signature **/
/************************/

/*
The firmware signature is appended to the compiled binary image in order to provide information
about the options used to compile the firmware file.  This information is then used by Multi-module 
flashing tools to verify that the image is correct / valid.

In order for the build process to determine the options used to build the firmware this file conditionally
declares 'flag' variables for the options we are interested in.

When the pre-compiler parses the source code these variables are either present or not in the parsed cpp file,
typically '$build_dir$/preproc/ctags_target_for_gcc_minus_e.cpp'.

Once the .bin file is created an additional command-line build tool scans the parsed cpp file, detects the
flags, assembles the signature, and finally appends the signature to the end of the binary file.

The signature is 24 bytes long:
multi-x[8-byte hex code]-[8-byte version number]

For example:
multi-x1234abcd-01020199

The 8-byte hex code is a 32-bit bitmask value indicating the configuration options, currently:

Bit(s)  Bitmask    Option                  Comment
1-2     0x3        Module type             Read as a two-bit value indicating a number from 0-3 which maps to a module type (AVR, STM32, OrangeRX)
3-7     0x7C       Channel order           Read as a five-bit value indicating a number from 0-23 which maps to as channel order (AETR, TAER, RETA, etc) (right-shift two bits to read)
8       0x80       Bootloader support      Indicates whether or not the firmware was built with support for the bootloader
9       0x100      CHECK_FOR_BOOTLOADER    Indicates if CHECK_FOR_BOOTLOADER is defined
10      0x200      INVERT_TELEMETRY        Indicates if INVERT_TELEMETRY is defined
11      0x400      MULTI_STATUS            Indicates if MULTI_STATUS is defined
12      0x800      MULTI_TELEMETRY         Indicates if MULTI_TELEMETRY is defined
13      0x1000     DEBUG_SERIAL            Indicates if DEBUG_SERIAL is defined

The 8-byte version number is the version number zero-padded to a fixed width of two-bytes per segment and no separator.  
E.g. 1.2.3.45 becomes 01020345.

Multi Telemetery Type can be read from bits 11 and 12 using the bitmask 0xC00 and right-shifting ten bits:
Telemetry Type    Decimal Value   Binary Value
Undefined         0               00
erSkyTX           1               01
OpenTX            2               10

Module types are mapped to the following decimal / binary values:
Module Type       Decimal Value   Binary Valsue
AVR               0               00
STM32             1               01
OrangeRX          2               10

Channel orders are mapped to the following decimal / binary values:
Channel Order	  Decimal Value	  Binary Value
AETR	          0	              00000
AERT	          1	              00001
ARET	          2	              00010
ARTE	          3	              00011
ATRE	          4	              00100
ATER	          5	              00101
EATR	          6	              00110
EART	          7	              00111
ERAT	          8	              01000
ERTA	          9	              01001
ETRA	          10	          01010
ETAR	          11	          01011
TEAR	          12	          01100
TERA	          13	          01101
TREA	          14	          01110
TRAE	          15	          01111
TARE	          16	          10000
TAER	          17	          10001
RETA	          18	          10010
REAT	          19	          10011
RAET	          20	          10100
RATE	          21	          10101
RTAE	          22	          10110
RTEA	          23	          10111
*/
 
// Set the flags for detecting and writing the firmware signature
#if defined (CHECK_FOR_BOOTLOADER)
    bool firmwareFlag_CHECK_FOR_BOOTLOADER = true;
#endif
#if defined (INVERT_TELEMETRY)
    bool firmwareFlag_INVERT_TELEMETRY = true;
#endif
#if defined (MULTI_STATUS)
    bool firmwareFlag_MULTI_STATUS = true;
#endif
#if defined (MULTI_TELEMETRY)
    bool firmwareFlag_MULTI_TELEMETRY = true;
#endif
#if defined (DEBUG_SERIAL)
    bool firmwareFlag_DEBUG_SERIAL = true;
#endif

// Channel order flags
#if defined (AETR)
    bool firmwareFlag_ChannelOrder_AETR = true;
#endif
#if defined (AERT)
    bool firmwareFlag_ChannelOrder_AERT = true;
#endif
#if defined (ARET)
    bool firmwareFlag_ChannelOrder_ARET = true;
#endif
#if defined (ARTE)
    bool firmwareFlag_ChannelOrder_ARTE = true;
#endif
#if defined (ATRE)
    bool firmwareFlag_ChannelOrder_ATRE = true;
#endif
#if defined (ATER)
    bool firmwareFlag_ChannelOrder_ATER = true;
#endif
#if defined (EATR)
    bool firmwareFlag_ChannelOrder_EATR = true;
#endif
#if defined (EART)
    bool firmwareFlag_ChannelOrder_EART = true;
#endif
#if defined (ERAT)
    bool firmwareFlag_ChannelOrder_ERAT = true;
#endif
#if defined (ERTA)
    bool firmwareFlag_ChannelOrder_ERTA = true;
#endif
#if defined (ETRA)
    bool firmwareFlag_ChannelOrder_ETRA = true;
#endif
#if defined (ETAR)
    bool firmwareFlag_ChannelOrder_ETAR = true;
#endif
#if defined (TEAR)
    bool firmwareFlag_ChannelOrder_TEAR = true;
#endif
#if defined (TERA)
    bool firmwareFlag_ChannelOrder_TERA = true;
#endif
#if defined (TREA)
    bool firmwareFlag_ChannelOrder_TREA = true;
#endif
#if defined (TRAE)
    bool firmwareFlag_ChannelOrder_TRAE = true;
#endif
#if defined (TARE)
    bool firmwareFlag_ChannelOrder_TARE = true;
#endif
#if defined (TAER)
    bool firmwareFlag_ChannelOrder_TAER = true;
#endif
#if defined (RETA)
    bool firmwareFlag_ChannelOrder_RETA = true;
#endif
#if defined (REAT)
    bool firmwareFlag_ChannelOrder_REAT = true;
#endif
#if defined (RAET)
    bool firmwareFlag_ChannelOrder_RAET = true;
#endif
#if defined (RATE)
    bool firmwareFlag_ChannelOrder_RATE = true;
#endif
#if defined (RTAE)
    bool firmwareFlag_ChannelOrder_RTAE = true;
#endif
#if defined (RTEA)
    bool firmwareFlag_ChannelOrder_RTEA = true;
#endif