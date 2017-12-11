/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A

  The xmega code mods make a considerable number of assumptions
  about the pin number assignments (as defined here):

  DEFAULT MAPPING ('DIGITAL_IO_PIN_SHIFT' NOT DEFINED)
  ----------------------------------------------------
  PORTD - digital 0-7
  PORTC - digital 8-15
  PORTE - digital 16-20
  PORTR - digital 20-21 (built-in LED on PORTR pin 1, aka '21')
  PORTA - analog 0-7, digital 22-29
  PORTB - analog 8-11, digital 30-33

  SPI is assumed to be on PORTC (pins 4-7)
  Serial is implemented on PORTD, Serial2 on PORTC, both using pins 2,3 (no flow control)
  PORTR pin 1 is assumed to be connected to an LED.  Pin 1 is the 'built-in' LED, defined
  as 'LED_BUILTIN', and is active HIGH.

  Your Mileage May Vary, depending on your board layout.  Some boards shift the
  digital pin assignments by 2 so that digital I/O pin 0 is PORTD Rx, pin 13 is PORTC SCK, just
  like the Arduino ATmega board.  Then they align the physical pins so that a regular Arduino shield
  will work, and assign PORTD 0-1 to 2 different digital I/O pin numbers (such as 20 and 21).

  To facilitate that specific change, uncomment the #define for 'DIGITAL_IO_PIN_SHIFT', below.
  Alternately you can create a separate version of this file with a different variant name,
  such as 'xmega-compat', with the #define uncommented, stored in an appropriately named directory.

  ============================
  HARDWARE SERIAL FLOW CONTROL
  ============================

  This version of the xmega Arduino startup+support files supports HARDWARE FLOW CONTROL on BOTH serial ports via
  RTS (output) and CTS (input).  CTS is implemented as input from remote device's DTR.  RTS is implemented as DTR output.

  To implement RTS/CTS, use definitions similar to the following in your version of this header file

  NOTE: RTS(DTR) will be configured as an output, active low (high tells sender to stop sending data to the UART)
        CTS will be configured as an input, active low (high stops data from being sent out via UART)

  CTS high to low transition causes an interrupt that may result in serial I/O (for faster response time).

  // RTS(DTR) as GPIO 0 (port D pin 0)
  #define SERIAL_0_RTS_PORT_NAME PORTD
  #define SERIAL_0_RTS_PIN_INDEX 0

  // CTS as GPIO 1 (port D pin 1)
  #define SERIAL_0_CTS_PORT_NAME PORTD
  #define SERIAL_0_CTS_PIN_INDEX 1

  use similar definitions for serial 1, aka 'SERIAL_1_CTS_PORT'

  NOTE:  you can even use PORTA or PORTB pins for this, if you don't need to measure analog volts on those pins

  X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A

*/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

// for now, the default xmega uses a simple assignment of digital pin numbers, beginning with port D
// to accomodate a useful "compatibility" shield design, these pins can be shifted so that the pin
// that maps to 'digitalRead(0)' would be D2 rather than D0.  This also puts 'Serial' on pins 0 and 1
// exactly like the Arduino UNO.  For any other mapping, create your own 'pins_arduino.h' file.
//
#define DIGITAL_IO_PIN_SHIFT /* COMMENT THIS to disable the shifting of digital pin assignments for Arduino shield compatibility */

// default two-wire port is TWIC.  '#define'ing USE_TWIC maps it to digital pins 20 and 21.  Only valid with DIGITAL_IO_PIN_SHIFT '#define'd
#define USE_TWIC /* define this to re-map TWIC to digital pins 20 and 21, similar to an Arduino Mega2560.  requires DIGITAL_IO_PIN_SHIFT */


#define NUM_DIGITAL_PINS            22
#define NUM_ANALOG_INPUTS           12
#define analogInputToDigitalPin(p)  ((p < 12) ? (p) + 22 : -1)
#ifdef DIGITAL_IO_PIN_SHIFT
#ifdef USE_TWIC
#define digitalPinHasPWM(p)         ((p) < 18 || (p) == 20 || (p) == 21) /* PORTC pins 0 and 1 are 20 and 21, respectively */
#else // USE_TWIC
#define digitalPinHasPWM(p)         ((p) < 18 || (p) == 20 || (p) == 21) /* PORTD pins 0 and 1 are 20 and 21, respectively */
#endif // USE_TWIC
#else // no digital I/O pin shift
#define digitalPinHasPWM(p)         ((p) < 20) /* port E pin 3 is the highest one that has PWM */
#endif // DIGITAL_IO_PIN_SHIFT

// this returns the DEFAULT INTERRUPT (in this case, interrupt 0) for any digital or analog pin
// If you choose a port's pin 2, it will be the same as using 'PORTn_INT0'
#define digitalPinToInterrupt(p) \
  ( pgm_read_byte(&port_to_int0_PGM[pgm_read_byte(&digital_pin_to_port_PGM[p])]) | \
    ( ((pgm_read_byte(&digital_pin_to_bit_mask_PGM[p]) - 2) & 7) << 5 ) )


// xmega-specific - Interrupt 'vector number' assignments:

// Interrupts are PORT-SPECIFIC, not pin specific.
// pin 2 on any port is always asynchronous (except for 'R' which doesn't have a pin 2)
// all other pins can manage synchronous interrupts.  'wakeup' from sleep mode
// and other async interrupts must be on a 'pin 2', on ports A through E
//
// Each port has 2 separate interrupt vectors.  They can be assigned different pins.
// The same pin can also be assigned to both vectors on the same port, if desired.

#define PORTD_INT0  0
#define PORTD_INT1  1
#define PORTC_INT0  2
#define PORTC_INT1  3
#define PORTE_INT0  4
#define PORTE_INT1  5
#define PORTA_INT0  6
#define PORTA_INT1  7
#define PORTB_INT0  8
#define PORTB_INT1  9
#define PORTR_INT0  10
#define PORTR_INT1  11

#define EXTERNAL_NUM_INTERRUPTS 12 /* defined here instead of wiring_private.h - max value is 32 */

// was in wiring_external.h, moved here
#define EXTERNAL_INT_0  0
#define EXTERNAL_INT_1  1
#define EXTERNAL_INT_2  2
#define EXTERNAL_INT_3  3
#define EXTERNAL_INT_4  4
#define EXTERNAL_INT_5  5
#define EXTERNAL_INT_6  6
#define EXTERNAL_INT_7  7
#define EXTERNAL_INT_8  8
#define EXTERNAL_INT_9  9
#define EXTERNAL_INT_10 10
#define EXTERNAL_INT_11 11

// xmega 'D' series has 2 sets of UART and SPI.
// The default UART is assigned on Port D, pins PD2-3
// The default SPI is assigned on Port C, pins PC4-7
//
// Also there are multiple 2-wire ports, the default being assigned to PC0-1
// see definition for DEFAULT_TWI and USE_TWIC
//
// Standard GPIO pins are assigned as follows:
// PD0-7 Digital 0-7
// PC0-7 Digital 8-15
// PE0-3 digital 16-19
// PR0-1 digital 20-21
// PA0-7 analog A0-A7
// PB0-3 analog A8-A11
//
// '#define'ing DIGITAL_IO_PIN_SHIFT shifts this down by 2, and places PD0-1 on 20-21
// This is for Arduino 'atmega' compatibility with respect to existing shields, so that
// you don't have to re-map pin numbers with #defines in existing software that hard-codes them
// or makes assumptions about pin numbers vs functionality [except TWI won't ever match up]
//
// '#define'ing USE_TWIC puts PC0-1 on 20-21 (corresponding to TWI pins on later Arduinos)
//
//
// ALL PORT REMAP registers must be assigned to 0 (default mappings for pins)
// this puts PWM output on pins 0-3 for PORT E (the timers are split for C and D)
// Additionally, CLKOUT should be 0 (no clock outputs on any port/pin).
//
// TIMERS
// Timer 0 should be configured as 'Tx2' (for 8 PWM outputs) by default, essentially
// as a dual 8-bit timer, more or less compatible with the Arduino's 3 timers and
// supporting all 8 pins on ports C and D for PWM output.  Port C's timer supports
// the system clock.
//
// See 'D' manual chapter 13 for more on this


// ------------------------------------------
// DEFINITIONS FOR SERIAL PORTS AND TWI PORTS
// ------------------------------------------


// TWI ports
#define DEFAULT_TWI TWIC /* note see definitions for SDA and SCL, below - alter accordingly */

// the XMega64D4 has two TWI ports
#define TWI_PORT0 TWIC
#define TWI_VECTOR_S0 TWIC_TWIS_vect
#define TWI_VECTOR_M0 TWIC_TWIM_vect
#define TWI_PORT1 TWIE
#define TWI_VECTOR_S1 TWIE_TWIS_vect
#define TWI_VECTOR_M1 TWIE_TWIM_vect

#define TWI_INTERFACE_COUNT 2


// obsolete - consider removal in all of them
//#define TWIC_VECT_ENABLE /* use this to select the correct interrupt vectors for default */


// serial port 0
#define SERIAL_0_PORT_NAME PORTD
#define SERIAL_0_USART_NAME USARTD0
#define SERIAL_0_USART_DATA USARTD0_DATA
#define SERIAL_0_RXC_ISR ISR(USARTD0_RXC_vect)
#define SERIAL_0_DRE_ISR ISR(USARTD0_DRE_vect)
//#define SERIAL_0_REMAP PORTD_REMAP /* define THIS to re-map the pins from 0-3 to 4-7 on serial port 0 */
#define SERIAL_0_REMAP_BIT 4    /* the bit needed to remap the port if SERIAL_0_REMAP is defined */
#define SERIAL_0_RX_PIN_INDEX 2 /* the pin number on the port, not the mapped digital pin number */
#define SERIAL_0_TX_PIN_INDEX 3 /* the pin number on the port, not the mapped digital pin number */
#define USARTD0_VECTOR_EXISTS

// serial port 1
#define SERIAL_1_PORT_NAME PORTC
#define SERIAL_1_USART_NAME USARTC0
#define SERIAL_1_USART_DATA USARTC0_DATA
#define SERIAL_1_RXC_ISR ISR(USARTC0_RXC_vect)
#define SERIAL_1_DRE_ISR ISR(USARTC0_DRE_vect)
//#define SERIAL_1_REMAP PORTC_REMAP /* define THIS to re-map the pins from 0-3 to 4-7 on serial port 1 */
#define SERIAL_1_REMAP_BIT 4    /* the bit needed to remap the port if SERIAL_1_REMAP is defined */
#define SERIAL_1_RX_PIN_INDEX 2 /* the pin number on the port, not the mapped digital pin number */
#define SERIAL_1_TX_PIN_INDEX 3 /* the pin number on the port, not the mapped digital pin number */
#define USARTC0_VECTOR_EXISTS



// For atmega/Arduino shield compatibility, with DIGITAL_IO_PIN_SHIFT defined
// typical board/pin layout might be like this (for shield pins):
//
// NOTE:  this design/layout assumes USE_TWIC is defined, so TWI is on TWIC (port C pins 0/1)
//
//               M M
//             S I O   T R
//         A   C S S S x x
//     S S R G K O I S 2 2              T R
//     C D E N 1 1 1 1                  x x
//     L A F D 3 2 1 0 9 8  7 6 5 4 3 2 1 0
// ----o-o-o-o-o-o-o-o-o-o--o-o-o-o-o-o-o-o----
//     P P     P P P P P P  P P P P P P P P
//     C C     C C C C C C  C C D D D D D D
//     1 0     7 6 5 4 3 2  1 0 7 6 5 4 3 2
//
//
//               T O P   V I E W
//
//
//                              P P P P P P
//                              A A A A A A
//                              5 4 3 2 1 0
// ------------o-o-o-o-o-o-o-o--o-o-o-o-o-o----
//             G I R 3 5 G G V  A A A A A A
//             N O E . V N N i  5 4 3 2 1 0
//             D R S 3   D D n
//               E E V
//               F T
//
// As with the MEGA2560 and other 'mega' Arduino boards, additional connectors would
// break out the additional pins, with appropriate labeling.  Additionally, there should
// be an LED on PORTR pin 1 for 'LED_BUILTIN'.
//
// This layout is based on the 'Rev 3' Arduino.
//
// NOTE - NO AREF:  AREF is not connected.  AREF is a bit of an issue on xmega because
// it DOES! NOT! WORK! THE! SAME! as it does on the ATmegaXXX and so you would need to
// (literally) steal one of the additional analog input pins to implement it. It's not
// impossible, or even THAT difficult.  I'm just not doing it here.



#ifdef DIGITAL_IO_PIN_SHIFT // aka digital I/O pin 0 is PORTD pin 2

// SHIFTED I/O pins (atmega compatibility) - see xmega mod description in comment at top

// default SPI
static const uint8_t SS   = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 12;
static const uint8_t SCK  = 13;

// primary SPI on PC4-7
static const uint8_t SS0   = 10;
static const uint8_t MOSI0 = 11;
static const uint8_t MISO0 = 12;
static const uint8_t SCK0  = 13;

// secondary SPI on PD4-7
static const uint8_t SS1   = 2;
static const uint8_t MOSI1 = 3;
static const uint8_t MISO1 = 4;
static const uint8_t SCK1  = 5;

// default 2-wire on PC0,PC1 - TWIC
#ifdef USE_TWIC
static const uint8_t SDA = 20;
static const uint8_t SCL = 21;
#else // !USE_TWIC
static const uint8_t SDA = 6;
static const uint8_t SCL = 7;
#endif // USE_TWIC

// port-specific 2-wire
#ifdef USE_TWIC
static const uint8_t SDA0 = 20;
static const uint8_t SCL0 = 21;
#else // !USE_TWIC
static const uint8_t SDA0 = 6;
static const uint8_t SCL0 = 7;
#endif // USE_TWIC
static const uint8_t SDA1 = 14;
static const uint8_t SCL1 = 15;


// keep track of the indices for port R since its control register
// settings should be slightly different - D manual table 11-6
#define PR0 18
#define PR1 19

#else // no digital I/O pin shifting, PORTD pin 0 is digital I/O pin 0 (as it should be)

// default "no shift" pin assignments

// default SPI
static const uint8_t SS   = 12;
static const uint8_t MOSI = 13;
static const uint8_t MISO = 14;
static const uint8_t SCK  = 15;

// primary SPI on PC4-7
static const uint8_t SS0   = 12;
static const uint8_t MOSI0 = 13;
static const uint8_t MISO0 = 14;
static const uint8_t SCK0  = 15;

// secondary SPI on PD4-7
static const uint8_t SS1   = 4;
static const uint8_t MOSI1 = 5;
static const uint8_t MISO1 = 6;
static const uint8_t SCK1  = 7;

// default 2-wire on PC0,PC1 - TWIC (TWIE appears to be broken)
static const uint8_t SDA = 8;
static const uint8_t SCL = 9;
static const uint8_t SDA0 = 8;
static const uint8_t SCL0 = 9;
static const uint8_t SDA1 = 16;
static const uint8_t SCL1 = 17;

// keep track of the indices for port R since its control register
// settings should be slightly different - D manual table 11-6
#define PR0 20
#define PR1 21

#endif // DIGITAL_IO_PIN_SHIFT


// default 'status' LED on PR1
//static const uint8_t LED_BUILTIN = PR1;
#define LED_BUILTIN PR1 /* Arduino 1.06 uses #define, not a const uint8_t */

static const uint8_t A0 = 22;
static const uint8_t A1 = 23;
static const uint8_t A2 = 24;
static const uint8_t A3 = 25;
static const uint8_t A4 = 26;
static const uint8_t A5 = 27;
static const uint8_t A6 = 28;
static const uint8_t A7 = 29;
static const uint8_t A8 = 30;
static const uint8_t A9 = 31;
static const uint8_t A10 = 32;
static const uint8_t A11 = 33;

// on the xmega64d4, PA2, PB2, PC2, PD2, and PE2 are asynchronous ints.  Others are 'synchronous' which means
// that they must be held in their 'interrupt state' long enough for the system to detect them.  In any case
// all digital input pins can be use as interrupts, synchronous or otherwise.



#ifdef ARDUINO_MAIN


const uint16_t PROGMEM port_to_mode_PGM[] = {
  NOT_A_PORT,             // 0
  (uint16_t) &PORTA_DIR,       // PA
  (uint16_t) &PORTB_DIR,       // PB
  (uint16_t) &PORTC_DIR,       // PC
  (uint16_t) &PORTD_DIR,       // PD
  (uint16_t) &PORTE_DIR,       // PE
  (uint16_t) &PORTR_DIR,       // PR
};

const uint16_t PROGMEM port_to_output_PGM[] = {
  NOT_A_PORT,              // 0
  (uint16_t) &PORTA_OUT,       // PA
  (uint16_t) &PORTB_OUT,       // PB
  (uint16_t) &PORTC_OUT,       // PC
  (uint16_t) &PORTD_OUT,       // PD
  (uint16_t) &PORTE_OUT,       // PE
  (uint16_t) &PORTR_OUT,       // PR
};

const uint16_t PROGMEM port_to_input_PGM[] = {
  NOT_A_PORT,             // 0
  (uint16_t) &PORTA_IN,       // PA
  (uint16_t) &PORTB_IN,       // PB
  (uint16_t) &PORTC_IN,       // PC
  (uint16_t) &PORTD_IN,       // PD
  (uint16_t) &PORTE_IN,       // PE
  (uint16_t) &PORTR_IN,       // PR
};

const uint8_t PROGMEM port_to_int0_PGM[] = {
  NOT_AN_INTERRUPT,           // 0
  PORTA_INT0,                 // PA
  PORTB_INT0,                 // PB
  PORTC_INT0,                 // PC
  PORTD_INT0,                 // PD
  PORTE_INT0,                 // PE
  PORTR_INT0,                 // PR
};

// xmega has a per-pin config register as well.  Normally these will be 00000111 for analog, 00000000 for digital 'totem pole'
// for 'INPUT_PULLUP' these will be 00011111
//   bits 2:0 (trigger)  000 both edges  001 rising  010 falling  011 level  111 input buffer disabled
//            note:  'input buffer disabled' required to use the 'IN' register (so default here)
//                   also port R does not support 'INTPUT_DISABLED' (sic) so use BOTHEDGES [0] instead
//   bits 5:3 (out/pull) 000 TOTEM [normal], 001 bus keeper [sticky], 010 pulldown, 011 pullup,
//                       100 wired 'or', 101 wired 'and', 110 wired 'or' pulldown, 111 wired 'and' pullup
//   bit 6:  "invert logic" (0 = normal, 1 = inverted)
//   bit 7:  unused, must be zero
// NOTE:  PORTA through PORTE (PORTF?) support 'input buffer disabled' and this setting is recommended
//        for analog inputs.  PORTR apparently does NOT support this (set to zero?)

const uint16_t PROGMEM digital_pin_to_control_PGM[] = {
#ifndef DIGITAL_IO_PIN_SHIFT
  (uint16_t) &PORTD_PIN0CTRL,  // PD 0 ** 0 **
  (uint16_t) &PORTD_PIN1CTRL,  // PD 1 ** 1 **
#endif // DIGITAL_IO_PIN_SHIFT
// subtract 2 from the digital pin number if DIGITAL_IO_PIN_SHIFT is defined
  (uint16_t) &PORTD_PIN2CTRL,  // PD 2 ** 2 ** USARTD_RX     ASYNC
  (uint16_t) &PORTD_PIN3CTRL,  // PD 3 ** 3 ** USARTD_TX
  (uint16_t) &PORTD_PIN4CTRL,  // PD 4 ** 4 **
  (uint16_t) &PORTD_PIN5CTRL,  // PD 5 ** 5 **
  (uint16_t) &PORTD_PIN6CTRL,  // PD 6 ** 6 **
  (uint16_t) &PORTD_PIN7CTRL,  // PD 7 ** 7 **
#if defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  (uint16_t) &PORTD_PIN0CTRL,  // PD 0 ** 8 ** map PORTD pins 0/1 here if TWIC is used
  (uint16_t) &PORTD_PIN1CTRL,  // PD 1 ** 9 **
#else
  (uint16_t) &PORTC_PIN0CTRL,  // PC 0 ** 8 ** SDA
  (uint16_t) &PORTC_PIN1CTRL,  // PC 1 ** 9 ** SCL
#endif // defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  (uint16_t) &PORTC_PIN2CTRL,  // PC 2 ** 10 **              ASYNC
  (uint16_t) &PORTC_PIN3CTRL,  // PC 3 ** 11 **
  (uint16_t) &PORTC_PIN4CTRL,  // PC 4 ** 12 ** SPI_SS
  (uint16_t) &PORTC_PIN5CTRL,  // PC 5 ** 13 ** SPI_MOSI
  (uint16_t) &PORTC_PIN6CTRL,  // PC 6 ** 14 ** SPI_MISO
  (uint16_t) &PORTC_PIN7CTRL,  // PC 7 ** 15 ** SPI_SCK
  (uint16_t) &PORTE_PIN0CTRL,  // PE 0 ** 16 ** SDA1
  (uint16_t) &PORTE_PIN1CTRL,  // PE 1 ** 17 ** SCL1
  (uint16_t) &PORTE_PIN2CTRL,  // PE 2 ** 18 **              ASYNC
  (uint16_t) &PORTE_PIN3CTRL,  // PE 3 ** 19 **
  (uint16_t) &PORTR_PIN0CTRL,  // PR 0 ** 20 **
  (uint16_t) &PORTR_PIN1CTRL,  // PR 1 ** 21 ** default LED
#ifdef DIGITAL_IO_PIN_SHIFT
#ifdef USE_TWIC
  (uint16_t) &PORTC_PIN0CTRL,  // PC 0 ** the new 20 ** SDA, SDA0
  (uint16_t) &PORTC_PIN1CTRL,  // PC 1 ** the new 21 ** SCL, SCL0
#else
  (uint16_t) &PORTD_PIN0CTRL,  // PD 0 ** the new 20 **
  (uint16_t) &PORTD_PIN1CTRL,  // PD 1 ** the new 21 **
#endif // USE_TWIC
#endif // DIGITAL_IO_PIN_SHIFT
  (uint16_t) &PORTA_PIN0CTRL,  // PA 0 ** 22 ** A0
  (uint16_t) &PORTA_PIN1CTRL,  // PA 1 ** 23 ** A1
  (uint16_t) &PORTA_PIN2CTRL,  // PA 2 ** 24 ** A2           ASYNC
  (uint16_t) &PORTA_PIN3CTRL,  // PA 3 ** 25 ** A3
  (uint16_t) &PORTA_PIN4CTRL,  // PA 4 ** 26 ** A4
  (uint16_t) &PORTA_PIN5CTRL,  // PA 5 ** 27 ** A5
  (uint16_t) &PORTA_PIN6CTRL,  // PA 6 ** 28 ** A6
  (uint16_t) &PORTA_PIN7CTRL,  // PA 7 ** 29 ** A7
  (uint16_t) &PORTB_PIN0CTRL,  // PB 0 ** 30 ** A8
  (uint16_t) &PORTB_PIN1CTRL,  // PB 1 ** 31 ** A9
  (uint16_t) &PORTB_PIN2CTRL,  // PB 2 ** 32 ** A10         ASYNC
  (uint16_t) &PORTB_PIN3CTRL,  // PB 3 ** 33 ** A11
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
  // PORTLIST
  // -------------------------------------------
#ifndef DIGITAL_IO_PIN_SHIFT
  _PD,  // PD 0 ** 0 **
  _PD,  // PD 1 ** 1 **
#endif // DIGITAL_IO_PIN_SHIFT
// subtract 2 from the digital pin number if DIGITAL_IO_PIN_SHIFT is defined
  _PD,  // PD 2 ** 2 ** USARTD_RX
  _PD,  // PD 3 ** 3 ** USARTD_TX
  _PD,  // PD 4 ** 4 **
  _PD,  // PD 5 ** 5 **
  _PD,  // PD 6 ** 6 **
  _PD,  // PD 7 ** 7 **
#if defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  _PD,  // PD 0 ** 8 **
  _PD,  // PD 1 ** 9 **
#else
  _PC,  // PC 0 ** 8 ** SDA
  _PC,  // PC 1 ** 9 ** SCL
#endif // defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  _PC,  // PC 2 ** 10 **
  _PC,  // PC 3 ** 11 **
  _PC,  // PC 4 ** 12 ** SPI_SS
  _PC,  // PC 5 ** 13 ** SPI_MOSI
  _PC,  // PC 6 ** 14 ** SPI_MISO
  _PC,  // PC 7 ** 15 ** SPI_SCK
  _PE,  // PE 0 ** 16 ** SDA
  _PE,  // PE 1 ** 17 ** SCL
  _PE,  // PE 2 ** 18 **
  _PE,  // PE 3 ** 19 **
  _PR,  // PR 0 ** 20 **
  _PR,  // PR 1 ** 21 ** default LED
#ifdef DIGITAL_IO_PIN_SHIFT
#ifdef USE_TWIC
  _PC,  // PC 0 ** the new 20 ** SDA
  _PC,  // PC 1 ** the new 21 ** SCL
#else
  _PD,  // PD 0 ** the new 20 **
  _PD,  // PD 1 ** the new 21 **
#endif // USE_TWIC
#endif // DIGITAL_IO_PIN_SHIFT
  _PA,  // PA 0 ** 22 ** A0
  _PA,  // PA 1 ** 23 ** A1
  _PA,  // PA 2 ** 24 ** A2
  _PA,  // PA 3 ** 25 ** A3
  _PA,  // PA 4 ** 26 ** A4
  _PA,  // PA 5 ** 27 ** A5
  _PA,  // PA 6 ** 28 ** A6
  _PA,  // PA 7 ** 29 ** A7
  _PB,  // PB 0 ** 30 ** A8
  _PB,  // PB 1 ** 31 ** A9
  _PB,  // PB 2 ** 32 ** A10
  _PB,  // PB 3 ** 33 ** A11
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
  // PIN IN PORT
  // -------------------------------------------
#ifndef DIGITAL_IO_PIN_SHIFT
  _BV( 0 ),  // PD 0 ** 0 **
  _BV( 1 ),  // PD 1 ** 1 **
#endif // DIGITAL_IO_PIN_SHIFT
// subtract 2 from the digital pin number if DIGITAL_IO_PIN_SHIFT is defined
  _BV( 2 ),  // PD 2 ** 2 ** USARTD_RX
  _BV( 3 ),  // PD 3 ** 3 ** USARTD_TX
  _BV( 4 ),  // PD 4 ** 4 **
  _BV( 5 ),  // PD 5 ** 5 **
  _BV( 6 ),  // PD 6 ** 6 **
  _BV( 7 ),  // PD 7 ** 7 **
#if defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  _BV( 0 ),  // PD 0 ** 8 **
  _BV( 1 ),  // PD 1 ** 9 **
#else
  _BV( 0 ),  // PC 0 ** 8 ** SDA
  _BV( 1 ),  // PC 1 ** 9 ** SCL
#endif // defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  _BV( 2 ),  // PC 2 ** 10 **
  _BV( 3 ),  // PC 3 ** 11 **
  _BV( 4 ),  // PC 4 ** 12 ** SPI_SS
  _BV( 5 ),  // PC 5 ** 13 ** SPI_MOSI
  _BV( 6 ),  // PC 6 ** 14 ** SPI_MISO
  _BV( 7 ),  // PC 7 ** 15 ** SPI_SCK
  _BV( 0 ),  // PE 0 ** 16 ** SDA
  _BV( 1 ),  // PE 1 ** 17 ** SCL
  _BV( 2 ),  // PE 2 ** 18 **
  _BV( 3 ),  // PE 3 ** 19 **
  _BV( 0 ),  // PR 0 ** 20 **
  _BV( 1 ),  // PR 1 ** 21 ** default LED
#ifdef DIGITAL_IO_PIN_SHIFT
#ifdef USE_TWIC
  _BV( 0 ),  // PC 0 ** the new 20 ** SDA
  _BV( 1 ),  // PC 1 ** the new 21 ** SCL
#else
  _BV( 0 ),  // PD 0 ** the new 20 **
  _BV( 1 ),  // PD 1 ** the new 21 **
#endif // USE_TWIC
#endif // DIGITAL_IO_PIN_SHIFT
  _BV( 0 ),  // PA 0 ** 22 ** A0
  _BV( 1 ),  // PA 1 ** 23 ** A1
  _BV( 2 ),  // PA 2 ** 24 ** A2
  _BV( 3 ),  // PA 3 ** 25 ** A3
  _BV( 4 ),  // PA 4 ** 26 ** A4
  _BV( 5 ),  // PA 5 ** 27 ** A5
  _BV( 6 ),  // PA 6 ** 28 ** A6
  _BV( 7 ),  // PA 7 ** 29 ** A7
  _BV( 0 ),  // PB 0 ** 30 ** A8
  _BV( 1 ),  // PB 1 ** 31 ** A9
  _BV( 2 ),  // PB 2 ** 32 ** A10
  _BV( 3 ),  // PB 3 ** 33 ** A11
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
  // TIMERS
  // -------------------------------------------
  // for now 'NOT_ON_TIMER' for all - later, assign timers based
  // on pins 0-3 being enabled as PWM out for ports A through E
  // corresponding to timers A through D (see D manual sections 11.12.14,
  // also see D manual sect 13.6 for using the 'compare' channel on 'TCx2' to generate
  // a PWM output.  Must select pin as output, _AND_ enable the 'compare' output
  // for the appropriate pin.  LCMPENx/HCMPENx registers to enable it.

#ifndef DIGITAL_IO_PIN_SHIFT
  TIMERD2,       // PD 0 ** 0 **
  TIMERD2,       // PD 1 ** 1 **
#endif // DIGITAL_IO_PIN_SHIFT
// subtract 2 from the digital pin number if DIGITAL_IO_PIN_SHIFT is defined
  TIMERD2,       // PD 2 ** 2 ** USARTD_RX
  TIMERD2,       // PD 3 ** 3 ** USARTD_TX
  TIMERD2,       // PD 4 ** 4 **
  TIMERD2,       // PD 5 ** 5 **
  TIMERD2,       // PD 6 ** 6 **
  TIMERD2,       // PD 7 ** 7 **
#if defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  TIMERD2,       // PD 0 ** 8 **
  TIMERD2,       // PD 1 ** 9 **
#else
  TIMERC2,       // PC 0 ** 8 ** SDA
  TIMERC2,       // PC 1 ** 9 ** SCL
#endif // defined(DIGITAL_IO_PIN_SHIFT) && defined(USE_TWIC)
  TIMERC2,       // PC 2 ** 10 **
  TIMERC2,       // PC 3 ** 11 **
  TIMERC2,       // PC 4 ** 12 ** SPI_SS
  TIMERC2,       // PC 5 ** 13 ** SPI_MOSI
  TIMERC2,       // PC 6 ** 14 ** SPI_MISO
  TIMERC2,       // PC 7 ** 15 ** SPI_SCK
  TIMERE0,       // PE 0 ** 16 ** SDA
  TIMERE0,       // PE 1 ** 17 ** SCL
  TIMERE0,       // PE 2 ** 18 **
  TIMERE0,       // PE 3 ** 19 **
  NOT_ON_TIMER,  // PR 0 ** 20 **
  NOT_ON_TIMER,  // PR 1 ** 21 ** default LED
#ifdef DIGITAL_IO_PIN_SHIFT
#ifdef USE_TWIC
  TIMERC2,       // PC 0 ** the new 20 ** SDA
  TIMERC2,       // PC 1 ** the new 21 ** SCL
#else
  TIMERD2,       // PD 0 ** the new 20 **
  TIMERD2,       // PD 1 ** the new 21 **
#endif // USE_TWIC
#endif // DIGITAL_IO_PIN_SHIFT
  NOT_ON_TIMER,  // PA 0 ** 22 ** A0
  NOT_ON_TIMER,  // PA 1 ** 23 ** A1
  NOT_ON_TIMER,  // PA 2 ** 24 ** A2
  NOT_ON_TIMER,  // PA 3 ** 25 ** A3
  NOT_ON_TIMER,  // PA 4 ** 26 ** A4
  NOT_ON_TIMER,  // PA 5 ** 27 ** A5
  NOT_ON_TIMER,  // PA 6 ** 28 ** A6
  NOT_ON_TIMER,  // PA 7 ** 29 ** A7
  NOT_ON_TIMER,  // PB 0 ** 30 ** A8
  NOT_ON_TIMER,  // PB 1 ** 31 ** A9
  NOT_ON_TIMER,  // PB 2 ** 32 ** A10
  NOT_ON_TIMER,  // PB 3 ** 33 ** A11
};

#endif


// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_MONITOR   Serial
#define SERIAL_PORT_HARDWARE  Serial
#define SERIAL_HARDWARE_OPEN  Serial2

#endif


