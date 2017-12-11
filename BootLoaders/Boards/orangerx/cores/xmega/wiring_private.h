/*
  wiring_private.h - Internal header file.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

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

  $Id: wiring.h 239 2007-01-12 17:58:39Z mellis $
*/

#ifndef WiringPrivate_h
#define WiringPrivate_h

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>

#include "Arduino.h"

// things that need to be defined in order for the code to compile

// definitions for ADC-related product signature row entries
#ifndef PRODSIGNATURES_ADCACAL0 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_ADCACAL0  _SFR_MEM8(0x0020)
#endif // PRODSIGNATURES_ADCACAL0

#ifndef PRODSIGNATURES_ADCACAL1 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_ADCACAL1  _SFR_MEM8(0x0021)
#endif // PRODSIGNATURES_ADCACAL1

#ifndef PRODSIGNATURES_ADCBCAL0 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_ADCBCAL0  _SFR_MEM8(0x0024)
#endif // PRODSIGNATURES_ADCBCAL0

#ifndef PRODSIGNATURES_ADCBCAL1 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_ADCBCAL1  _SFR_MEM8(0x0025)
#endif // PRODSIGNATURES_ADCBCAL1

// definitions for USB-related product signature row entries
#ifndef PRODSIGNATURES_USBCAL0 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_USBCAL0  _SFR_MEM8(0x001a)
#endif // PRODSIGNATURES_USBCAL0

#ifndef PRODSIGNATURES_USBCAL1 /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_USBCAL1  _SFR_MEM8(0x001b)
#endif // PRODSIGNATURES_USBCAL1

#ifndef PRODSIGNATURES_USBRCOSC /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_USBRCOSC  _SFR_MEM8(0x001c)
#endif // PRODSIGNATURES_USBRCOSC

#ifndef PRODSIGNATURES_USBRCOSCA /* _MOST_ headers don't define this properly - see ATMel Studio headers */
#define PRODSIGNATURES_USBRCOSCA  _SFR_MEM8(0x001d)
#endif // PRODSIGNATURES_USBRCOSCA



#ifndef ADCA_CH0_SCAN           /* A1 headers don't define this properly */
#define ADCA_CH0_SCAN  _SFR_MEM8(0x0226)
#define ADC_REFSEL_INTVCC2_gc (0x04<<4)
#define ADC_CH_GAIN_DIV2_gc (0x07<<2)
#endif // ADCA_CH0_SCAN

#ifndef ADC_REFSEL2_bm          /* A1 headers don't define this properly, but A1U headers do [and others] */
#define ADC_REFSEL2_bm  (1<<6)  /* Reference Selection bit 2 mask. */
#define ADC_REFSEL2_bp  6       /* Reference Selection bit 2 position. */
#endif // ADC_REFSEL2_bm

#ifndef PORT_USART0_bm
#define PORT_USART0_bm  0x10  /* Usart0 bit mask for port remap register. */
#endif // PORT_USART0_bm


#ifdef __cplusplus
extern "C"{
#endif

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// NOTE:  xmega is SO different that these need to be in pins_arduino.h
//        some xmegas have 2 per port, others 1 per port, and the # of ports vary greatly
//#define EXTERNAL_INT_0 0
//#define EXTERNAL_INT_1 1
//#define EXTERNAL_INT_2 2
//#define EXTERNAL_INT_3 3
//#define EXTERNAL_INT_4 4
//#define EXTERNAL_INT_5 5
//#define EXTERNAL_INT_6 6
//#define EXTERNAL_INT_7 7

// CPU-specific definitions go into the 'pins_arduino.h' files anyway
//
//#if defined(SOME_CPU)
//#define EXTERNAL_NUM_INTERRUPTS 8
//#endif

typedef void (*voidFuncPtr)(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif


