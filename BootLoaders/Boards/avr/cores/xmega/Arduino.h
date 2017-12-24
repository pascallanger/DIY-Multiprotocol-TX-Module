/*
  Arduino.h - standard definitions for Arduino build environment
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis [duplicated from pins_arduino.h, not present in original]

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
  for the XMegaForArduino project - http://github.com/XMegaForArduino

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/


#ifndef Arduino_h
#define Arduino_h

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "binary.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define HIGH 0x1
#define LOW  0x0

#define INPUT                0x0 /* totem poll, input */
#define OUTPUT               0x1 /* totem poll, output */
#define INPUT_BUS_KEEPER     0x2 /* weak pull up/down to maintain state when switched to or in input mode */
#define INPUT_PULLUP         0x3 /* pullup resistor on input */
#define INPUT_PULLDOWN       0x4 /* pulldown resistor on input */
#define OUTPUT_OR            0x5 /* output open drain 'or', no pulldown */
#define OUTPUT_AND           0x6 /* output open drain 'and', no pullup */
#define INPUT_OR_PULLDOWN    0x7 /* output open drain 'or' with pulldown */
#define INPUT_AND_PULLUP     0x8 /* output open drain 'and' with pullup */
#define OUTPUT_OR_PULLDOWN   0x9 /* output open drain 'or' with pulldown */
#define OUTPUT_AND_PULLUP    0xa /* output open drain 'and' with pullup */
#define INPUT_OUTPUT_MASK    0xf /* mask for INPUT/OUTPUT flags */
#define INPUT_SENSE_DEFAULT  0    /* input sense default - currently 'BOTH' */
#define INPUT_SENSE_RISING   0x10 /* just rising */
#define INPUT_SENSE_FALLING  0x20 /* just falling */
#define INPUT_SENSE_BOTH     0x30 /* rising AND falling */
#define INPUT_SENSE_LEVEL    0x40 /* high level (or low if I invert it) */
#define INPUT_SENSE_DISABLED 0x50 /* buffered input disabled (most pins won't be able to use 'IN' if you do this) */
#define INPUT_SENSE_MASK     0x70 /* mask for 'input sense' bits */
#define INPUT_OUTPUT_INVERT  0x80 /* bit for 'inverted' I/O - note that digitalRead and digitalWrite will re-invert to maintain consistency */

// NOTE:  'INPUT_OUTPUT_INVER' is primarily there to support LOW LEVEL interrupts.  if you specify this flag for normal
//        digital I/O, there will be no 'visible effect' since digitalRead and digitalWrite will "re-invert" the bit value
//        and act as if the invert flag weren't set.  That way, if you select 'LOW LEVEL' interrupt, you will read the
//        low level as a '0' (as it should be) via digitalRead, even though the value MUST be inverted for this to work.

// NOTE:  the values of 'true' and 'false' should be defined by C++ already
#define true /*0x1*/(!0) /* rather than '1' true is defined as '!0' - it's logically accurate */
#define false 0x0

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

/* I do not know what these next 4 #defines do, but all 4 seem to be WRONG - bf */
#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

// INTERRUPT TYPE - LOW, HIGH, CHANGE, FALLING, RISING  (compatibility with DUE etc.)
// 'LOW' is defined as '0' already
// 'HIGH' is defined as '1' already
#define CHANGE 2
#define FALLING 3
#define RISING 4

// definitions for atmega328 etc. carried forward - not sure what this is for
#define INTERNAL 3
#define DEFAULT 1
#define EXTERNAL 0

// undefine stdlib's abs if encountered (from 'arduino' version)
#ifdef abs
#undef abs
#endif

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define interrupts() sei()
#define noInterrupts() cli()

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))


typedef unsigned int word;

#define bit(b) (1UL << (b))

typedef uint8_t boolean;
typedef uint8_t byte;

void init(void);
void initVariant(void);

int atexit(void (*func)()) __attribute__((weak));

void adc_setup(void); // implemented in wiring_analog.c - configures ADC for analogRead()
// adc_setup must be called whenever exiting SLEEP MODE or ADC will malfunction
// It is automatically called from 'init()' but sleep mode typically resets the controller

void pinMode(uint8_t, uint8_t);
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void analogReference(uint8_t mode); // somewhat different for xmega (default is Vcc/2) - see 'enum _analogReference_', below
                                    // pass only one of THOSE values as 'mode'
void analogWrite(uint8_t, int);

// special XMEGA-specific functions for the analog inputs

int analogReadDeltaWithGain(uint8_t pin, uint8_t negpin, uint8_t gain);
// typically 'pin' can be A0 through An, 'negpin' may be restricted but typically A4-A7 or 'ANALOG_READ_DELTA_USE_GND' to use GND
// NOTE:  On the A-series processors it is NOT possible to use 'diff input with gain' on MORE than A0-A7
//        On later processors (like D series) it _IS_ possible.


#define ANALOG_READ_DELTA_USE_GND 0xff

// there is a bug in several headers for ADC_REFSEL_gm - should be 0x70, not 0x30 (and it gets re-defined, too)
#ifdef ADC_REFSEL_gm
#undef ADC_REFSEL_gm
#endif // ADC_REFSEL_gm
#define ADC_REFSEL_gm 0x70

enum _analogReference_ // pass to 'analogReference' function - see D manual section 22.14.3, or 28.16.3 in 'AU' manual
{
  analogReference_INT1V = (ADC_REFSEL_INT1V_gc),
  analogReference_PORTA0 = (ADC_REFSEL_AREFA_gc),   // PORT A pin 0 is the AREF

#if !defined (__AVR_ATxmega8E5__) && !defined (__AVR_ATxmega16E5__) && !defined (__AVR_ATxmega32E5__)
  // these 2 aren't valid for 'E' series
  analogReference_PORTB0 = (ADC_REFSEL_AREFB_gc),   // PORT B pin 0 is the AREF

  analogReference_VCC = (ADC_REFSEL0_bm) /* (ADC_REFSEL_VCC_gc)*/,        // VCC / 10, actually
  // NOTE:  'ADC_REFSEL_VCC_gc' exists for some headers, and others 'ADC_REFSEL_INTVCC_gc'
  //        to avoid compile problems I use the bitmask instead.
#endif // E series

#if defined(__AVR_ATxmega64d4__) || defined(__AVR_ATxmega64a1u__) || defined(__AVR_ATxmega128a1u__)
  analogReference_VCCDIV2 = (ADC_REFSEL_VCCDIV2_gc) // using THIS forces gain to 1/2, so it's rail-rail
#else
  analogReference_VCCDIV2 = (0x04<<4) // (ADC_REFSEL_VCCDIV2_gc)
  // NOTE that for some processor headers, ADC_REFSEL_VCCDIV2_gc is not properly defined
  // this definition '(0x04<<4)' is taken from the 64d4 header.  it's also THE DEFAULT for max compatibility
#endif // processors that define ADC_REFSEL_VCCDIV2_gc correctly
};  

// NOTE: this constant isn't always defined, either
#ifndef ADC_CH_GAIN_gm
#define ADC_CH_GAIN_gm  0x1C  /* Gain Factor group mask. */
#endif // ADC_CH_GAIN_gm


unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long);
void delayMicroseconds(unsigned int us);
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout);

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);

// XMEGA specific
void wait_for_interrupt(void); // uses 'IDLE' sleep mode to wait for an interrupt, then returns
void low_power_delay(unsigned long ms); // similar to 'delay' but goes into low power 'IDLE sleep' state


// X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A
//
// NOTE:  for 'attachInterrupt' the 'mode' parameter indicates non-default input pins
//        and the interrupt mode, as well as the interrupt priority.  If the interrupt
//        priority is INT_MODE_PRI_DEFAULT (0), it will be assigned a 'default' value.
//        Default interrupt pin is pin 2 except for PORTR (since it doesn't have a pin 2)
//        when it has not been specified.
//
//        usage:
//          attachInterrupt(PORTD_INT0,                                     the specific interrupt vector - see pins_arduino.h
//                          my_callback,                                    user-defined callback function
//                          RISING                                          interrupt mode (can be LOW, HIGH, RISING, FALLING, CHANGE)
//                          | INT_MODE_PIN_DEFAULT                          the pin(s) to assign to this interrupt, or default pin 2 (optional)
//                          | INT_MODE_PRI_DEFAULT);                        interrupt priority, default is 'high' (optional)
//
//        Additional note, the 'pin' constants (see below) refer to the port's pin number, and
//        NOT the 'digital I/O pin' number.  See 'pins_arduino.h' for more on this.
//
// for compatibility with newer arduino environment, attachInterrupt 'interruptNum' parameter
// can use the return value from 'digitalPinToInterrupt(pin)'
//
// X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A   X M E G A

#ifdef __cplusplus
void attachInterrupt(uint8_t interruptNum, void (*)(void), int mode = 0); // default 'mode' param is:  LOW | INT_MODE_PRI_DEFAULT | INT_MODE_PIN_DEFAULT
#else // not __cplusplus
void attachInterrupt(uint8_t interruptNum, void (*)(void), int mode);
#endif // __cplusplus

void detachInterrupt(uint8_t interruptNum); // NOTE:  detaches ALL interrupts for that port (special exceptions for serial flow control)


// this next function reads data from the calibration row, including the serial # info.
// This is often referred to as the 'PRODUCT SIGNATURE ROW'.  It is xmega-specific.
uint8_t readCalibrationData(uint16_t iIndex);


// INTERRUPT MODE FLAGS - for attachInterrupt 'mode' parameter

#define INT_MODE_MODE_MASK 0x003f
#define INT_MODE_PRI_MASK  0x00c0
#define INT_MODE_PRI_DEFAULT    0
#define INT_MODE_PRI_LOW   0x0040
#define INT_MODE_PRI_MED   0x0080
#define INT_MODE_PRI_HIGH  0x00c0
#define INT_MODE_PRI_SHIFT      6 /* shift right 6 bits to get a 0, 1, 2 or 3 for priority (0 is 'default') */
#define INT_MODE_PIN_MASK  0xff00
#define INT_MODE_PIN0      0x0100
#define INT_MODE_PIN1      0x0200
#define INT_MODE_PIN2      0x0400
#define INT_MODE_PIN3      0x0800
#define INT_MODE_PIN4      0x1000
#define INT_MODE_PIN5      0x2000
#define INT_MODE_PIN6      0x4000
#define INT_MODE_PIN7      0x8000
#define INT_MODE_PIN_DEFAULT    0 /* no 'pin bits' set implies 'default' which is pin 2 on each capable port */
#define INT_MODE_PIN_SHIFT      8 /* shift right 8 bits to get the pin bits in a single byte */

// NOTE:  the 'pin' constants refer to the port's pin number, and not the digital I/O pin
//        The default 'pin 2' refers to the port's pin 2. See 'pins_arduino.h' for more on this.
//        Multiple pins may be specified, so it is a bit mask.  If a pin is specified by using
//        digitalPinToInterrupt() and you also specify pins using the 'INT_MODE_PINx' flags, the
//        pin specified in the 'interruptNum' parameter will be 'or'd with the pins specified in
//        'mode'.  This can result in some unpredictable outcomes, so you should either use
//        'digitalPinToInterrupt' for 'interruptNum', or specify the port as 'interruptNum' and
//        then specfify the pin info in 'mode'.


#define NOT_AN_INTERRUPT (-1) /* a placeholder for various arrays, etc. */




// SETUP and LOOP (no changes from Arduino classic)

void setup(void);
void loop(void);


// hardware flow control 'helpers'
void serial_0_cts_callback(void);
void serial_1_cts_callback(void);
void InitSerialFlowControlInterrupts(void);


// On the xmega, the addresses of the port registers are
// greater than 255, so we can't store them in uint8_t's.
extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];
extern const uint16_t PROGMEM digital_pin_to_control_PGM[];

// these contain index values so they CAN be uint8_t's
extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_timer_PGM[];
// extern const uint8_t PROGMEM digital_pin_to_bit_PGM[];  not used on xmega
extern const uint16_t PROGMEM port_to_input_PGM[];

// Get the bit location within the hardware port of the given virtual pin.
// This comes from the pins_*.c file for the active board configuration.
//
// These perform slightly better as macros compared to inline functions
//
#define digitalPinToPort(P) ( pgm_read_byte( digital_pin_to_port_PGM + (P) ) )
#define digitalPinToBitMask(P) ( pgm_read_byte( digital_pin_to_bit_mask_PGM + (P) ) )
#define digitalPinToTimer(P) ( pgm_read_byte( digital_pin_to_timer_PGM + (P) ) )
// note pins_arduino.h may need to override this next one, depending
#define analogInPinToBit(P) ((P) & 7) /* analog pin 0 = 0 (PORTA), analog pin 8 = 0 (PORTB) */
#define portOutputRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_output_PGM + (P))) )
#define portInputRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_input_PGM + (P))) )
#define portModeRegister(P) ( (volatile uint8_t *)( pgm_read_word( port_to_mode_PGM + (P))) )
#define pinControlRegister(P) ( (volatile uint8_t *)( pgm_read_word( digital_pin_to_control_PGM + (P))) )

// use THIS macro to convert a _BV(n) value into 'n'
#define pinBitValueToIndex(B) ( (B)==_BV(0) ? 0 : (B)==_BV(1) ? 1 : (B)==_BV(2) ? 2 : (B)==_BV(3) ? 3 : \
                                (B)==_BV(4) ? 4 : (B)==_BV(5) ? 5 : (B)==_BV(6) ? 6 : (B)==_BV(7) ? 7 : 0 )

#define NOT_A_PIN 0
#define NOT_A_PORT 0

#ifdef ARDUINO_MAIN
// use of '_' prefix to prevent collisions with iox64d#.h and for consistency
#define _PA 1
#define _PB 2
#define _PC 3
#define _PD 4
#define _PE 5
#define _PR 6 /* was PF */
#define _PF 7
#define _PH 8
#define _PJ 9
#define _PK 10
#define _PQ 11
#endif

// modified timer definitions for xmega
// TCD2 --> TIMERD2
// TCC2 --> TIMERC2
// TCE0 --> TIMERE0 - 'D' series which has only 4 pins on PORTE */
// TCE2 --> TIMERE2 - A series and others that use all 8 pins for port E
// TCF2 --> TIMERF2 - A series and others that have PORT F
#define NOT_ON_TIMER 0
#define TIMERD2 1
#define TIMERC2 2
#define TIMERE0 3
#define TIMERE2 4
#define TIMERF2 5
#define TIMERC4 6
#define TIMERD5 7
// not using TCD0,1 nor TCC0,1
// The first 16 IO pins (PD0-PD7, PC0-PC7) will be PWM capable, as are PE0-PE3 (or PE0-PE7) and PF0-PF7 (when there)


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifdef __cplusplus
#include "WCharacter.h"
#include "WString.h"
#include "HardwareSerial.h"

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

#if 0
// these are not currently implemented - TODO implement them

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout = 1000000L);
#endif // 0

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration = 0);
void noTone(uint8_t _pin);


// WMath prototypes
long random(long);
long random(long, long);
void randomSeed(unsigned int);
long map(long, long, long, long, long);

#endif // __cplusplus

// at this point we include the pin definitions from 'pins_arduino.h'
// you can customize 'pins_arduino.h' for your specific hardware

#include "pins_arduino.h"

// The default SPI interface is SPIC if not already defined
#ifndef DEFAULT_SPI
#define DEFAULT_SPI SPIC
#endif // DEFAULT_SPI

// the default TWI interface is TWIC if not already defined
#ifndef DEFAULT_TWI
#define DEFAULT_TWI TWIC
#endif // DEFAULT_TWI


// added support for hardware serial flow control - spans multiple files

#if defined(SERIAL_0_RTS_PORT_NAME) && defined(SERIAL_0_RTS_PIN_INDEX)
#define SERIAL_0_RTS_ENABLED
#define SERIAL_0_RTS_PORT (&SERIAL_0_RTS_PORT_NAME)
#define SERIAL_0_RTS_PIN _BV(SERIAL_0_RTS_PIN_INDEX)
#endif // defined(SERIAL_0_RTS_PORT) && defined(SERIAL_0_RTS_PIN)

#if defined(SERIAL_1_RTS_PORT_NAME) && defined(SERIAL_1_RTS_PIN_INDEX)
#define SERIAL_1_RTS_ENABLED
#define SERIAL_1_RTS_PORT (&SERIAL_1_RTS_PORT_NAME)
#define SERIAL_1_RTS_PIN _BV(SERIAL_1_RTS_PIN_INDEX)
#endif // defined(SERIAL_1_RTS_PORT) && defined(SERIAL_1_RTS_PIN)

#if defined(SERIAL_0_CTS_PORT_NAME) && defined(SERIAL_0_CTS_PIN_INDEX)
#define SERIAL_0_CTS_ENABLED
#define SERIAL_0_CTS_PORT (&SERIAL_0_CTS_PORT_NAME)
#define SERIAL_0_CTS_PIN _BV(SERIAL_0_CTS_PIN_INDEX)
#endif // defined(SERIAL_0_CTS_PORT) && defined(SERIAL_0_CTS_PIN)

#if defined(SERIAL_1_CTS_PORT_NAME) && defined(SERIAL_1_CTS_PIN_INDEX)
#define SERIAL_1_CTS_ENABLED
#define SERIAL_1_CTS_PORT (&SERIAL_1_CTS_PORT_NAME)
#define SERIAL_1_CTS_PIN _BV(SERIAL_1_CTS_PIN_INDEX)
#endif // defined(SERIAL_1_CTS_PORT) && defined(SERIAL_1_CTS_PIN)


#endif // Arduino_h


