/* -*- mode: jde; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  Part of the Wiring project - http://wiring.uniandes.edu.co

  Copyright (c) 2004-05 Hernando Barragan

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

  Modified 24 November 2006 by David A. Mellis
  Modified 1 August 2010 by Mark Sproul

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "wiring_private.h"

// interrupts on the xmega are handled differently than the mega
// there are 2 interrupt vectors for each port.  Typical xmega
// will use ports A, B, C, D, E, and R.  The vectors for those
// are as follows:
//
//   PORTn_INT0_vect
//   - and -
//   PORTn_INT1_vect
//
// where 'n' is A, B, C, D, E, or R
//
// Additional vectors are:
//
//   OSC_XOSCF_vect  (external oscillator failure, NMI)
//
//   RTC_OVF_vect    (real-time clock overflow)
//   RTC_COMP_vect   (real-time clock compare)
//
//   TWIC_TWIS_vect  (2-wire slave on port C)
//   TWIC_TWIM_vect  (2-wire master on port C)
//   TWIE_TWIS_vect  (2-wire slave on port E)
//   TWIE_TWIM_vect  (2-wire master on port E)
//
//   timers - 'n' is C or D
//   TCn0_OVF_vect   (n timer 0 overflow)
//   TCn1_OVF_vect   (n timer 1 overflow)
//   TCn2_LUNF_vect  (n timer 2 low byte underflow)
//   TCn2_HUNF_vect  (n timer 2 high byte underflow)
//   TCE0_OVF_vect   (E timer 0 overflow)
//   TCn0_ERR_vect   (n timer 0 error)
//   TCn1_ERR_vect   (n timer 1 error)
//   TCE0_ERR_vect   (E timer 0 error)
//   TCn0_CCA_vect   (n timer 0 compare or capture A)
//   TCn1_CCA_vect   (n timer 1 compare or capture A)
//   TCn2_LCMPA_vect (n timer 2 low-byte compare or capture A)
//   TCE0_CCA_vect   (E timer 0 compare or capture A)
//   TCn0_CCB_vect   (n timer 0 compare or capture B)
//   TCn1_CCB_vect   (n timer 1 compare or capture B)
//   TCn2_LCMPB_vect (n timer 2 low-byte compare or capture B)
//   TCE0_CCB_vect   (E timer 0 compare or capture B)
//   TCn0_CCC_vect   (n timer 0 compare or capture C)
//   TCn2_LCMPC_vect (n timer 2 low-byte compare or capture C)
//   TCE0_CCC_vect   (E timer 0 compare or capture C)
//   TCn0_CCD_vect   (n timer 0 compare or capture D)
//   TCn2_LCMPD_vect (n timer 2 low-byte compare or capture D)
//   TCE0_CCD_vect   (E timer 0 compare or capture D)
//
//   SPIn_INT_vect   (SPI C or D)
//
//   USARTn0_RXC_vect (USART 'n' [C or D] receive complete)
//   USARTn0_DRE_vect (USART 'n' [C or D] data reg empty)
//   USARTn0_TXC_vect (USART 'n' [C or D] transmit complete)
//
// NOTE:  a 'USARTE' interrupt vector also exists, but isn't
// implemented on the D4 series
//
// ASYNC interrupts are only possible on pin 2 for each of the 5
// ports ('R' only has 2 pins, 0 and 1, so no async interrupt).
// Sleep modes typically need ASYNC interrupts to wake up.

// The interrupt will be handled for a particular port, and not for
// a particular interrupt.  The 'attachInterrupt' function will default
// to pin 2 (asynchronous interrupt) unless otherwise specified in the
// 'mode' parameter.

// BOOTLOADER NOTES
//
// Bit 6 of the CTRL reg must be assigned to '0'.  Bit 7 can be assigned
// to '1' to enable 'round robin' scheduling using the priority bits.
// Bits 0-2 (HILVLEN, MEDLVLEN, LOLVLEN) should also be assigned to '1'
// to allow all 3 interrupt levels to execute.  ('D' manual 10.8.3 pg 102)
// The CTRL reg can be assigned to b10000111 to accomplish this.  Bit 6
// needs to use the "configuration change protection" method to change it
// and may need to be assigned separately.



// interrupt mode - predefined values are LOW, CHANGE, RISING, FALLING, HIGH
// additional bits are 'or'd with mode

static volatile voidFuncPtr intFunc[EXTERNAL_NUM_INTERRUPTS];
static volatile uint8_t intPins[EXTERNAL_NUM_INTERRUPTS]; // added - store pins for this interrupt
// volatile static voidFuncPtr twiIntFunc;


// NOTE: I _HATE_ K&R style so I'll make it Allman style as I go along...

void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode)
{
uint8_t iPinBits, iPriBits, iModeBits, iInv, iNum, iMask;
uint8_t oldSREG;
uint8_t intInfo;
PORT_t *port;


  // for compatibility with newer IDE, 'interruptNum' can be encoded with pin information.
  // if it is, then the pin info will be derived from pin info in 'mode' and 'interruptNum'
  // pin info will be incorporated into it.

  intInfo = ((interruptNum & 0xe0) >> 5);  // is an int pin encoded here by digitalPinToInterrupt ?
  interruptNum &= 0x1f; // so the rest of the code will work correctly

  if(interruptNum >= EXTERNAL_NUM_INTERRUPTS)
  {
    return;
  }

  iPinBits = (uint8_t)((mode & INT_MODE_PIN_MASK) >> 8);

  if(intInfo)
  {
    intInfo = ((intInfo + 2) & 7); // convert to actual pin number

    iPinBits |= _BV(intInfo); // set respective bit in 'iPinBits'
  }

  if(!iPinBits)
  {
    if(interruptNum == PORTR_INT0
#ifdef PORTR_INT1
       || interruptNum == PORTR_INT1
#endif // PORTR_INT1
       ) // not valid for these
    {
      return; // do nothing (for now)
    }

    iPinBits = _BV(2); // set bit for pin 2 if none specified [i.e. 'default']
  }

  iPriBits = (mode & INT_MODE_PRI_MASK)
           >> INT_MODE_PRI_SHIFT;

  if(!iPriBits) // not assigned
  {
    iPriBits = 3; // for now, just use the highest priority
  }

  mode &= INT_MODE_MODE_MASK;
  iInv = 0;

  if(mode == LOW) // normally will be this, for backward hardware compatibility
  {
    iModeBits = PORT_ISC_LEVEL_gc; // b011, high level continuously generates events
  }
  else if(mode == HIGH) // these constants correspond to the mega's bit mask on ISC00,ISC10
  {
    iModeBits = PORT_ISC_LEVEL_gc; // b011, high level continuously generates events
    iInv = 1;                      // invert input (so 'high level' becomes 'low level')

    // NOTE:  this was verified by experimentation.  The documentation is misleading, suggesting
    //        that a LEVEL interrupt triggered on HIGH, not on LOW.  But it triggers on LOW.  So
    //        if you want HIGH, you must invert it.  Not the other way around.  Yeah.
  }
  else if(mode == CHANGE)
  {
    iModeBits = PORT_ISC_BOTHEDGES_gc; // BOTHEDGES - see table 11-6
  }
  else if(mode == RISING)
  {
    iModeBits = PORT_ISC_RISING_gc; // b001, RISING
  }
  else if(mode == FALLING)
  {
    iModeBits = PORT_ISC_FALLING_gc; // b010, FALLING
  }
  else
  {
    iModeBits = PORT_ISC_BOTHEDGES_gc; // BOTH (the default - note INTPUT_DISABLED (sic) won't buffer the input, so it's useless except for analog channels)
  }

  if(iInv)
  {
    iModeBits |= _BV(PORT_INVEN_bp); // set the 'inverted' bit
  }

  oldSREG = SREG; // store the interrupt flag basically

  cli(); // disable interrupts for a bit

  intFunc[interruptNum] = userFunc;
  intPins[interruptNum] = iPinBits; // save what pins I used

  // Enable the interrupt (smaller code to use if/else and pointer)

  iNum = 0;
  if(interruptNum == PORTA_INT0
#ifdef PORTA_INT1
     || interruptNum == PORTA_INT1
#endif // PORTA_INT1
     )
  {
    port = &PORTA;
#ifdef PORTA_INT1
    if(interruptNum == PORTA_INT1)
    {
      iNum = 1;
    }
#endif // PORTA_INT1
  }
#if NUM_ANALOG_PINS > 8 /* which means we have PORT B */
  else if(interruptNum == PORTB_INT0
#ifdef PORTB_INT1
          || interruptNum == PORTB_INT1
#endif // PORTB_INT1
          )
  {
    port = &PORTB;
#ifdef PORTB_INT1
    if(interruptNum == PORTB_INT1)
    {
      iNum = 1;
    }
#endif // PORTB_INT1
  }
#endif // NUM_ANALOG_PINS > 8
  else if(interruptNum == PORTC_INT0
#ifdef PORTC_INT1
          || interruptNum == PORTC_INT1
#endif // PORTC_INT1
          )
  {
    port = &PORTC;
#ifdef PORTC_INT1
    if(interruptNum == PORTC_INT1)
    {
      iNum = 1;
    }
#endif // PORTC_INT1
  }
  else if(interruptNum == PORTD_INT0
#ifdef PORTD_INT1
          || interruptNum == PORTD_INT1
#endif // PORTD_INT1
          )
  {
    port = &PORTD;
#ifdef PORTD_INT1
    if(interruptNum == PORTD_INT1)
    {
      iNum = 1;
    }
#endif // PORTC_INT1
  }
#if NUM_DIGITAL_PINS > 18 /* which means we have PORT E */
  else if(interruptNum == PORTE_INT0
#ifdef PORTE_INT1
          || interruptNum == PORTE_INT1
#endif // PORTE_INT1
          )
  {
    port = &PORTE;
#ifdef PORTE_INT1
    if(interruptNum == PORTE_INT1)
    {
      iNum = 1;
    }
#endif // PORTE_INT1
  }
#endif // NUM_DIGITAL_PINS > 18
  else if(interruptNum == PORTR_INT0
#ifdef PORTR_INT1
          || interruptNum == PORTR_INT1
#endif // PORTR_INT1
          )
  {
    port = &PORTR;
#ifdef PORTR_INT1
    if(interruptNum == PORTR_INT1)
    {
      iNum = 1;
    }
#endif // PORTR_INT1
  }
  else
  {
    return; // do nothing
  }

  // On certain processors there's only one interrupt, so it's called 'INTMASK'
  // we test for this here

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
  if(!iNum)
  {
    // set interrupt mask for PORT A, int 0 vector
    port->INTMASK |= iPinBits; // enable int zero for these pins
    port->INTCTRL = (port->INTCTRL & ~(PORT_INTLVL_gm))
                  | (iPriBits & 3);
  }
#else // INT0MASK and INT1MASK supported
  if(!iNum)
  {
    // set interrupt mask for PORT A, int 0 vector
    port->INT0MASK |= iPinBits; // enable int zero for these pins
    port->INTCTRL = (port->INTCTRL & ~(PORT_INT0LVL_gm))
                  | (iPriBits & 3);
  }
  else // if(iNum == 1)
  {
    port->INT1MASK |= iPinBits; // enable int zero for these pins
    port->INTCTRL = (port->INTCTRL & ~(PORT_INT1LVL_gm))
                  | ((iPriBits & 3) << PORT_INT1LVL_gp);
  }
#endif // INT0MASK and INT1MASK supported

  for(iNum=0, iMask = 1; iNum < 8; iNum++, iMask <<= 1)
  {
    register8_t *pCTRL = &(port->PIN0CTRL) + iNum; // treat PIN0CTRL through PIN7CTRL as an array

    // set corresponding 'type' in the interrupt control regs for the individual bits

    if(iPinBits & iMask) // is this bit set in 'iPinBits'?
    {
      // enable the interrupt pin's mode bits and assign the 'invert' flag as needed

      *pCTRL = (*pCTRL & ~(PORT_ISC_gm | PORT_INVEN_bm))
             | iModeBits;
    }
  }

  SREG = oldSREG; // restore it, interrupts (probably) re-enabled
  // NOTE that this may throw an interrupt right away
}



void detachInterrupt(uint8_t interruptNum)
{
uint8_t iPinBits, iNum, iMask;
uint8_t oldSREG;
PORT_t *port;


  // NOTE:  this function will turn OFF the 'invert' bit if it's set for a HIGH level interrupt
  //        and digitalRead _SHOULD_ be consistent before/after this call.

  if(interruptNum >= EXTERNAL_NUM_INTERRUPTS)
  {
    return;
  }

  oldSREG = SREG; // keep track of interrupt flag state

  cli(); // clear the interrupt flag

  // grab 'pin bits' so I know what to flip around
  iPinBits = intPins[interruptNum]; // what I used when I added it

  intFunc[interruptNum] = 0;
  intPins[interruptNum] = 0; // zero both of these

  // disable the interrupt

  // Enable the interrupt (smaller code to use if/else and pointer)

  iNum = 0;
  if(interruptNum == PORTA_INT0
#ifdef PORTA_INT1
     || interruptNum == PORTA_INT1
#endif // PORTA_INT1
     )
  {
    port = &PORTA;
#ifdef PORTA_INT1
    if(interruptNum == PORTA_INT1)
    {
      iNum = 1;
    }
#endif // PORTA_INT1
  }
#if NUM_ANALOG_PINS > 8 /* which means we have PORT B */
  else if(interruptNum == PORTB_INT0
#ifdef PORTB_INT1
          || interruptNum == PORTB_INT1
#endif // PORTB_INT1
          )
  {
    port = &PORTB;
#ifdef PORTB_INT1
    if(interruptNum == PORTB_INT1)
    {
      iNum = 1;
    }
#endif // PORTB_INT1
  }
#endif // NUM_ANALOG_PINS > 8
  else if(interruptNum == PORTC_INT0
#ifdef PORTC_INT1
          || interruptNum == PORTC_INT1
#endif // PORTC_INT1
          )
  {
    port = &PORTC;
#ifdef PORTC_INT1
    if(interruptNum == PORTC_INT1)
    {
      iNum = 1;
    }
#endif // PORTC_INT1
  }
  else if(interruptNum == PORTD_INT0
#ifdef PORTD_INT1
          || interruptNum == PORTD_INT1
#endif // PORTD_INT1
          )
  {
    port = &PORTD;
#ifdef PORTD_INT1
    if(interruptNum == PORTD_INT1)
    {
      iNum = 1;
    }
#endif // PORTD_INT1
  }
#if NUM_DIGITAL_PINS > 18 /* which means we have PORT E */
  else if(interruptNum == PORTE_INT0
#ifdef PORTE_INT1
          || interruptNum == PORTE_INT1
#endif // PORTE_INT1
          )
  {
    port = &PORTE;
#ifdef PORTE_INT1
    if(interruptNum == PORTE_INT1)
    {
      iNum = 1;
    }
#endif // PORTE_INT1
  }
#endif // NUM_DIGITAL_PINS > 18
  else if(interruptNum == PORTR_INT0
#ifdef PORTR_INT1
          || interruptNum == PORTR_INT1
#endif // PORTR_INT1
          )
  {
    port = &PORTR;
#ifdef PORTR_INT1
    if(interruptNum == PORTR_INT1)
    {
      iNum = 1;
    }
#endif // PORTR_INT1
  }
  else
  {
    return; // do nothing
  }

  // On certain processors there's only one interrupt, so it's called 'INTMASK'
  // we test for this here

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
  if(!iNum)
  {
    // set interrupt mask for PORT A, int 0 vector
    port->INTMASK = 0;                  // disable interrupts - TODO, read this instead of 'iPinBits' ?
    port->INTCTRL &= ~(PORT_INTLVL_gm); // set interrupt control to 'OFF'
    port->INTFLAGS = _BV(0);            // clear the int flag

#else // INT0MASK and INT1MASK supported
  if(!iNum)
  {
    // set interrupt mask for PORT A, int 0 vector
    port->INT0MASK = 0;                  // disable interrupts - TODO, read this instead of 'iPinBits' ?
    port->INTCTRL &= ~(PORT_INT0LVL_gm); // set interrupt control to 'OFF'
    port->INTFLAGS = _BV(0);             // clear the int 0 flag
  }
  else // if(iNum == 1)
  {
#endif // INT0MASK and INT1MASK supported
    // if this matches a CTS port, I do _NOT_ want to disable interrupts
#if defined(SERIAL_0_CTS_ENABLED) && defined(SERIAL_1_CTS_ENABLED)
    if(SERIAL_0_CTS_PORT == port)
    {
#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
      if(SERIAL_1_CTS_PORT == port)
      {
        port->INTMASK = SERIAL_0_CTS_PIN | SERIAL_1_CTS_PIN;   // disable interrupts but leave BOTH enabled
      }
      else
      {
        port->INTMASK = SERIAL_0_CTS_PIN;    // disable interrupts but leave THIS one enabled
      }

      port->INTCTRL |= PORT_INTLVL_gm; // max priority when I do this
#else // INT0MASK and INT1MASK supported
      if(SERIAL_1_CTS_PORT == port)
      {
        port->INT1MASK = SERIAL_0_CTS_PIN | SERIAL_1_CTS_PIN;   // disable interrupts but leave BOTH enabled
      }
      else
      {
        port->INT1MASK = SERIAL_0_CTS_PIN;    // disable interrupts but leave THIS one enabled
      }

      port->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
#endif // INT0MASK and INT1MASK supported
    }
    else if(SERIAL_1_CTS_PORT == port)
    {
#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
      port->INTMASK = SERIAL_1_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INTLVL_gm; // max priority when I do this
#else // INT0MASK and INT1MASK supported
      port->INT1MASK = SERIAL_1_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
#endif // INT0MASK and INT1MASK supported
    }
#elif defined(SERIAL_0_CTS_ENABLED)
    if(SERIAL_0_CTS_PORT == port)
    {
#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
      port->INTMASK = SERIAL_0_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INTLVL_gm; // max priority when I do this
#else // INT0MASK and INT1MASK supported
      port->INT1MASK = SERIAL_0_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
#endif // INT0MASK and INT1MASK supported
    }
#elif defined(SERIAL_1_CTS_ENABLED)
    if(SERIAL_1_CTS_PORT == port)
    {
#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
      port->INTMASK = SERIAL_1_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INTLVL_gm; // max priority when I do this
#else // INT0MASK and INT1MASK supported
      port->INT1MASK = SERIAL_1_CTS_PIN;    // disable interrupts but leave THIS one enabled

      port->INTCTRL |= PORT_INT1LVL_gm; // max priority when I do this
#endif // INT0MASK and INT1MASK supported
    }
#endif // SERIAL_0/1_CTS_ENABLED

#if defined(SERIAL_0_CTS_ENABLED) || defined(SERIAL_1_CTS_ENABLED)
    else
#endif // SERIAL_0/1_CTS_ENABLED

    {
#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
      port->INTMASK = 0;                  // disable interrupts - TODO, read this instead of 'iPinBits' ?
      port->INTCTRL &= ~(PORT_INTLVL_gm); // set interrupt control to 'OFF'
#else // INT0MASK and INT1MASK supported
      port->INT1MASK = 0;                  // disable interrupts - TODO, read this instead of 'iPinBits' ?
      port->INTCTRL &= ~(PORT_INT1LVL_gm); // set interrupt control to 'OFF'
#endif // INT0MASK and INT1MASK supported
    }

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
    port->INTFLAGS = _BV(0);             // clear the int 0 flag
#else // INT0MASK and INT1MASK supported
    port->INTFLAGS = _BV(1);             // clear the int 1 flag
#endif // INT0MASK and INT1MASK supported
  }

  for(iNum=0, iMask = 1; iNum < 8; iNum++, iMask <<= 1)
  {
    register8_t *pCTRL = &(port->PIN0CTRL) + iNum; // treat PIN0CTRL through PIN7CTRL as an array

    // set corresponding 'type' in the interrupt control regs for the individual bits

    if(iPinBits & iMask) // is this bit set in 'iPinBits'?
    {
      *pCTRL &= ~(PORT_ISC_gm | PORT_INVEN_bm); // turn off invert flag and reset to 'BOTH' (the default)
    }
  }

  SREG = oldSREG; // restore it, interrupts (probably) re-enabled

}


#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
ISR(PORTA_INT_vect)
#else // INT0MASK and INT1MASK supported
ISR(PORTA_INT0_vect)
#endif // INT0MASK and INT1MASK supported
{
  if(intFunc[PORTA_INT0])
    intFunc[PORTA_INT0]();

#ifdef PORTC_INT0MASK // INT0MASK and INT1MASK supported
}

ISR(PORTA_INT1_vect)
{
  if(intFunc[PORTA_INT1])
    intFunc[PORTA_INT1]();
#endif // INT0MASK and INT1MASK supported

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTA)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTA))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}

#if NUM_ANALOG_PINS > 8 /* which means we have PORT B */
ISR(PORTB_INT0_vect)
{
  if(intFunc[PORTB_INT0])
    intFunc[PORTB_INT0]();
}

ISR(PORTB_INT1_vect)
{
  if(intFunc[PORTB_INT1])
    intFunc[PORTB_INT1]();

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTB)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTB))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}
#endif // NUM_ANALOG_PINS > 8

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
ISR(PORTC_INT_vect)
#else // INT0MASK and INT1MASK supported
ISR(PORTC_INT0_vect)
#endif // INT0MASK and INT1MASK supported
{
  if(intFunc[PORTC_INT0])
    intFunc[PORTC_INT0]();
#ifdef PORTC_INT0MASK // INT0MASK and INT1MASK supported
}

ISR(PORTC_INT1_vect)
{
  if(intFunc[PORTC_INT1])
    intFunc[PORTC_INT1]();
#endif // INT0MASK and INT1MASK supported

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTC)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTC))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
ISR(PORTD_INT_vect)
#else // INT0MASK and INT1MASK supported
ISR(PORTD_INT0_vect)
#endif // INT0MASK and INT1MASK supported
{
  if(intFunc[PORTD_INT0])
    intFunc[PORTD_INT0]();
#ifdef PORTC_INT0MASK // INT0MASK and INT1MASK supported
}

ISR(PORTD_INT1_vect)
{
  if(intFunc[PORTD_INT1])
    intFunc[PORTD_INT1]();
#endif // INT0MASK and INT1MASK supported

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTD)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTD))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}


#if NUM_DIGITAL_PINS > 18 /* which means we have PORT E */
ISR(PORTE_INT0_vect)
{
  if(intFunc[PORTE_INT0])
    intFunc[PORTE_INT0]();
}

ISR(PORTE_INT1_vect)
{
  if(intFunc[PORTE_INT1])
    intFunc[PORTE_INT1]();

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTE)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTE))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}
#endif // NUM_DIGITAL_PINS > 18

// TODO:  ISRs for PORTF, PORTH, PORTJ, PORTK, PORTQ

#ifndef PORTC_INT0MASK /* meaning there's only one int vector and not two */
ISR(PORTR_INT_vect)
#else // INT0MASK and INT1MASK supported
ISR(PORTR_INT0_vect)
#endif // INT0MASK and INT1MASK supported
{
  if(intFunc[PORTR_INT0])
    intFunc[PORTR_INT0]();
#ifdef PORTC_INT0MASK // INT0MASK and INT1MASK supported
}

ISR(PORTR_INT1_vect)
{
  if(intFunc[PORTR_INT1])
    intFunc[PORTR_INT1]();
#endif // INT0MASK and INT1MASK supported

#if defined(SERIAL_0_CTS_ENABLED)
  if(SERIAL_0_CTS_PORT == &(PORTR)) // this should compile as a constant expression
  {
    serial_0_cts_callback();
  }
#endif // SERIAL_0_CTS_ENABLED

#ifdef SERIAL_1_CTS_ENABLED
  if(SERIAL_1_CTS_PORT == &(PORTR))
  {
    serial_1_cts_callback();
  }
#endif // SERIAL_1_CTS_ENABLED
}



