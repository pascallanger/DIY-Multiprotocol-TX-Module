/*
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
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

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/

#include "wiring_private.h"

#include <avr/sleep.h>      // Include for sleep mode (see 'wait_for_interrupt()')


// The xmega architecture differs significantly from the mega in a number
// of ways that render the existing code unworkable.  Therefore a complete
// re-write was done to ensure 100% compatibility at the code level.



// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
// (NOTE:  for 'E' series, it's 32 clock sycles)
//#ifdef TCC4 /* using timer 4,5 rather than 0,2 - 'E' series */
//#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(32 * 256))
//#else // TCC4
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
//#endif // TCC4

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// NOTE:  xmega runs at 32 mhz typically.  However, it uses THE PERIPHERAL CLOCK
//        for the timer.  Normally this is the same as the CPU clock unless you use
//        some crazy clock pre-scaler.
//        See sections 6.9 and 6.10 in D manual for system clock setup

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz and 32 Mhz for xmega - this doesn't lose precision.)

#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

// timer zero overflow - affects pins 5 and 6 for PWM on Arduino and compatibles
// what I want to do is simulate what the Arduino already does, using TCD2
// since it will share the same pre-scaler AND clock for all of the port D PWM out

// the timer prescaler MUST tick every 64 clock cycles for this to work, just like the mega TIMER 0
// NOTE:  at 32Mhz MICROSECONDS_PER_TIMER0_OVERFLOW will be 512 - consider a divider of 128 instead
//        unless you want 2khz for the PWM (which is actually a good idea, for servos etc.)


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//            _____  _                          ___  ____   ____            //
//           |_   _|(_) _ __ ___    ___  _ __  |_ _|/ ___| |  _ \           //
//             | |  | || '_ ` _ \  / _ \| '__|  | | \___ \ | |_) |          //
//             | |  | || | | | | ||  __/| |     | |  ___) ||  _ <           //
//             |_|  |_||_| |_| |_| \___||_|    |___||____/ |_| \_\          //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef TCC4 // 'E' series or later that has TCC4 and TCD5
ISR(TCD5_OVF_vect)
#elif !defined(TCD2_LUNF_vect)
ISR(TCD0_OVF_vect) // for A series
#else // USING TCD2
ISR(TCD2_LUNF_vect)
#endif // TCD2, TCD5
{
  // for this to work the limit must be 255 (8-bit mode)

#ifdef TCC4 // 'E' series or later that has TCC4 and TCD5
  TCD5_INTFLAGS = 1; // clears the flag so I don't 'spin' (this behavior changed from previous timers)
#endif // 'E' series

  // copy these to local variables so they can be stored in registers
  // (volatile variables must be read from memory on every access)
  unsigned long m = timer0_millis;
  unsigned char f = timer0_fract;

#if MILLIS_INC > 0
  m += MILLIS_INC;
#endif // MILLIS_INC
  f += FRACT_INC;
  if (f >= FRACT_MAX)
  {
    f -= FRACT_MAX;
    m += 1;
  }

  timer0_fract = f;
  timer0_millis = m;
  timer0_overflow_count++;
}

unsigned long millis()
{
  unsigned long m;
  uint8_t oldSREG = SREG;

  // disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis)
  cli();
  m = timer0_millis;
  SREG = oldSREG;

  return m;
}

unsigned long micros()
{
  unsigned long m;
  uint8_t t, oldSREG;

  oldSREG = SREG;
  cli(); // for consistency, don't let this part get interrupted

  m = timer0_overflow_count; // for xmega it's really an underflow except 'E' series
#ifdef TCC4
  t = 255 - (TCD5_CNT & 0xff);
#elif !defined(TCD2)
  t = 255 - (TCD0_CNT & 0xff);
#else // TCC4
  t = 255 - TCD2_LCNT; // 'low' count, it's what we interrupt on (and it always counts DOWN)
                       // must subtract count value from 255 for this to work correctly
#endif // TCC4

  // check the interrupt flag to see if I just got an underflow

#ifdef TCC4
  if((TCD5_INTFLAGS & _BV(0)) && (t < 255)) // which means I overflowed but didn't call the ISR yet
#elif !defined(TCD2)
  if((TCD0_INTFLAGS & _BV(0)) && (t < 255)) // which means I underflowed but didn't call the ISR yet
#else // TCC4
  if((TCD2_INTFLAGS & _BV(0)) && (t < 255)) // which means I underflowed but didn't call the ISR yet
#endif // TCC4
  {
    m++; // increment ISR count for more accurate microseconds
  }

  SREG = oldSREG;

  return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond()); // TODO:  make the '64' a #define ?
}

void delay(unsigned long ms)
{
  uint16_t start = (uint16_t)micros();

  while (ms > 0) /* BF - fixed K&R style to Allman for readability/consistency */
  {
    if (((uint16_t)micros() - start) >= 1000)
    {
      ms--;
      start += 1000;
    }
  }
}

// XMEGA-specific code
void wait_for_interrupt(void)
{
  cli(); // disable interrupts

  set_sleep_mode(SLEEP_MODE_IDLE); // everything on but CPU and NVRAM

  sleep_enable();

  sei(); // re-enable interrupts

  sleep_cpu(); // go to sleep

  sleep_disable(); // first thing to do out of sleep
}

void low_power_delay(unsigned long ms)
{
  uint16_t start = (uint16_t)micros();

  while (ms > 0)
  {
    wait_for_interrupt(); // up to 1msec perhaps?

    while (((uint16_t)micros() - start) >= 1000)
    {
      ms--;
      start += 1000;
    }
  }
}


/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. */
// NOTE:  for XMEGA, you can have a 32mhz clock
void delayMicroseconds(unsigned int us)
{
  // NOTE:  for 32mhz clock, max time is 65536 / 8 or about 8k microsecs

  // calling avrlib's delay_us() function with low values (e.g. 1 or
  // 2 microseconds) gives delays longer than desired.
  //delay_us(us);
#if F_CPU >= 32000000L /* the xmega typically has this */

  // for a one-microsecond delay, simply wait 12 cycles and return. The overhead
  // of the function call yields a delay of exactly a one microsecond.
  __asm__ __volatile__ (
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    "nop"); //just waiting 12 cycle
  if (--us == 0)
    return;

  // the following loop takes a 1/8 of a microsecond (4 cycles)
  // per iteration, so execute it five times for each microsecond of
  // delay requested.
  us = (us<<3);// * 8

  // account for the time taken in the preceeding commands.
  us -= 2; // 2 clock cycles

#elif F_CPU >= 20000000L
  // for the 20 MHz clock on rare Arduino boards

  // for a one-microsecond delay, simply wait 2 cycle and return. The overhead
  // of the function call yields a delay of exactly a one microsecond.
  __asm__ __volatile__ (
    "nop" "\n\t"
    "nop"); //just waiting 2 cycle
  if (--us == 0)
    return;

  // the following loop takes a 1/5 of a microsecond (4 cycles)
  // per iteration, so execute it five times for each microsecond of
  // delay requested.
  us = (us<<2) + us; // x5 us

  // account for the time taken in the preceeding commands.
  us -= 2;

#elif F_CPU >= 16000000L
  // for the 16 MHz clock on most Arduino boards

  // for a one-microsecond delay, simply return.  the overhead
  // of the function call yields a delay of approximately 1 1/8 us.
  if (--us == 0)
    return;

  // the following loop takes a quarter of a microsecond (4 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2;

  // account for the time taken in the preceeding commands.
  us -= 2;
#else
  // for the 8 MHz internal clock on the ATmega168

  // for a one- or two-microsecond delay, simply return.  the overhead of
  // the function calls takes more than two microseconds.  can't just
  // subtract two, since us is unsigned; we'd overflow.
  if (--us == 0)
    return;
  if (--us == 0)
    return;

  // the following loop takes half of a microsecond (4 cycles)
  // per iteration, so execute it twice for each microsecond of
  // delay requested.
  us <<= 1;

  // partially compensate for the time taken by the preceeding commands.
  // we can't subtract any more than this or we'd overflow w/ small delays.
  us--;
#endif

  // busy wait
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                           //
//   ____               _                      ____  _               _      ____         _                   //
//  / ___|  _   _  ___ | |_  ___  _ __ ___    / ___|| |  ___    ___ | | __ / ___|   ___ | |_  _   _  _ __    //
//  \___ \ | | | |/ __|| __|/ _ \| '_ ` _ \  | |    | | / _ \  / __|| |/ / \___ \  / _ \| __|| | | || '_ \   //
//   ___) || |_| |\__ \| |_|  __/| | | | | | | |___ | || (_) || (__ |   <   ___) ||  __/| |_ | |_| || |_) |  //
//  |____/  \__, ||___/ \__|\___||_| |_| |_|  \____||_| \___/  \___||_|\_\ |____/  \___| \__| \__,_|| .__/   //
//          |___/                                                                                   |_|      //
//                                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this function is separate since it provides a specific functionality
// and aids in readability by separating it from the main 'init()' code
// regardless of the extra bytes needed to make the function call
void clock_setup(void)
{
unsigned short sCtr;
register unsigned char c1;

  // TODO:  get rid of magic bit numbers, and use bit value constants from iox64d4.h etc. (ongoing)

  // TODO:  consider clock setup using PLL and 2Mhz multiplied by 16, to free up the 32Mhz to be
  //        used by the USB at 48Mhz (sync on SOF).  this would be an alternate config for USB devices.

  // --------------------------------------------------------------------------------------------
  //                                         CLOCK SETUP
  //
  // enable BOTH the 32Mhz and 32.768KHz internal clocks [ignore what else might be set for now]
  // --------------------------------------------------------------------------------------------

  OSC_CTRL |= OSC_RC32KEN_bm | OSC_RC32MEN_bm; // CLK_SCLKSEL_RC32M_gc | CLK_SCLKSEL_RC32K_gc;

  if(!(CLK_LOCK & CLK_LOCK_bm)) // clock lock bit NOT set, so I can muck with the clock
  {
    if((CLK_CTRL & CLK_SCLKSEL_gm) != CLK_SCLKSEL_RC32M_gc) // it's not already 32 Mhz
    {
      // wait until 32mhz clock is 'stable'

      for(sCtr=32767; sCtr > 0; sCtr--) // TODO:  remove counter?
      {
        // spin on oscillator status bit for 32Mhz oscillator

        if(OSC_STATUS & OSC_RC32MRDY_bm/*CLK_SCLKSEL_RC32M_gc*/) // 32Mhz oscillator is 'ready' (6.10.2)
        {
          break;
        }
      }

      // for now, I can allow the clock to NOT be changed if it's
      // not ready.  This prevents infinite loop inside startup code

      if(!(OSC_STATUS & OSC_RC32MRDY_bm/*CLK_SCLKSEL_RC32M_gc*/)) // is my oscillator 'ready' ?
      {
        return; // exit - don't change anything
      }

      // switch to 32Mhz clock using internal source

      CCP = CCP_IOREG_gc; // 0xd8 - see D manual, sect 3.14.1 (protected I/O)
      CLK_CTRL = CLK_SCLKSEL_RC32M_gc; // set the clock to 32Mhz (6.9.1)
    }

    if(CLK_PSCTRL != 0)
    {
      CCP = CCP_IOREG_gc; // 0xd8 - see D manual, sect 3.14.1 (protected I/O)
      CLK_PSCTRL = CLK_PSADIV_1_gc | CLK_PSBCDIV_1_1_gc/*0*/; // set the clock divider(s) to 1:1 (6.9.2)
    }

    // now that I've changed the clock, disable 2Mhz, PLL, and external clocks
    // 32.768KHz should remain active, but I need to make sure it's stable
    OSC_CTRL &= // ~(_BV(4) | _BV(3) | _BV(0)); // sect 6.10.1 - disable PLL, external, 2Mhz clocks
      ~(OSC_PLLEN_bm | OSC_XOSCEN_bm
        | OSC_RC2MEN_bm    /* disable the 2Mhz oscillator - startup code *DOES* do this, boot code does NOT */
#ifdef OSC_RC8MCAL // only present in 'E' series - for now shut it off
        | OSC_RC8MEN_bm    /* disable the 8M oscillator (when present) */
#endif // OSC_RC8MCAL
        );

    // wait until 32.768KHz clock is 'stable'.  this one goes for a while
    // in case it doesn't stabilize in a reasonable time.  I figure about
    // 64*255 clock cycles should be enough, ya think?  Timeout if it's not
    // actually ready, I don't want infinite loops.  TODO:  re-consider?
    for(sCtr=65535; sCtr > 0; sCtr--)
    {
      for(c1=255; c1 > 0; c1--)
      {
        if(OSC_STATUS & OSC_RC32KRDY_bm/*CLK_SCLKSEL_RC32K_gc*/) // 32.768KHz oscillator is 'ready' (6.10.2)
        {
          sCtr = 1; // this will bail out of the outer loop
          break;
        }
      }
    }

    // enable DFLL auto-calibration of the 32Mhz internal oscillator
    // (it uses the reasonably precise 32.768KHz clock to do it)

    OSC_DFLLCTRL = 0; // sect 6.10.7 - select 32.768KHz osc for everything, basically
    DFLLRC32M_CTRL = 1; // set the bit to enable DFLL calibration - section 6.11.1
  }

  // I'll be using the 1.024khz clock (from the 32.768KHz clock) for the real-time counter
  // this will give me a reasonable "about 1 millisecond" accuracy on the RTC

  // NOTE:  I may not have checked for this if I skipped the previous section,
  //        so now I check again, just in case, to make sure the 32.768KHz osc is stable
  for(sCtr=65535; sCtr > 0; sCtr--)
  {
    for(c1=255; c1 > 0; c1--)
    {
      if(OSC_STATUS & OSC_RC32KRDY_bm/*CLK_SCLKSEL_RC32K_gc*/) // 32.768KHz oscillator is 'ready' (6.10.2)
      {
        sCtr = 1; // this will bail out of the outer loop
        break;
      }
    }
  }

  if(!(OSC_STATUS & OSC_RC32KRDY_bm/*CLK_SCLKSEL_RC32K_gc*/)) // is my oscillator 'ready' ?
  {
    return; // exit - don't change anything else.  Better to fail than to hang
  }


  // RUN-TIME clock - use internal 1.024 khz source.  cal'd 32khz needed for this (but it's running)
  // The RTC can be used to wake up the CPU.  It uses VERY little current.

  CLK_RTCCTRL = CLK_RTCSRC_RCOSC_gc; // section 6.9.4
}


// this was derived from a message board post.  The function is public to make it easy to
// use the 'Production Signature Row'.  There is a unique identifier for the CPU as well as
// calibration data for the ADC available, and also USB settings (for USB-capable devices)
// See sect. 4.14 "Production Signature Row" in 'D' manual.  
uint8_t readCalibrationData(uint16_t iIndex)
{
  uint8_t rVal;

  /* Load the NVM Command register to read the calibration row. */
  NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc; // see the section on NVM operations and lpm instruction

//  rVal = pgm_read_byte_near(iIndex); // effectively the same thing as the inline assembler
  __asm__ ("lpm %0, Z\n" : "=r" (rVal) : "z" (iIndex)); // do it THIS way instead

  /* Clean up NVM Command register. */
  NVM_CMD = NVM_CMD_NO_OPERATION_gc;

  return(rVal);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                           //
//   _____  _                          ___         _  _    _         _  _             _    _                 //
//  |_   _|(_) _ __ ___    ___  _ __  |_ _| _ __  (_)| |_ (_)  __ _ | |(_) ____ __ _ | |_ (_)  ___   _ __    //
//    | |  | || '_ ` _ \  / _ \| '__|  | | | '_ \ | || __|| | / _` || || ||_  // _` || __|| | / _ \ | '_ \   //
//    | |  | || | | | | ||  __/| |     | | | | | || || |_ | || (_| || || | / /| (_| || |_ | || (_) || | | |  //
//    |_|  |_||_| |_| |_| \___||_|    |___||_| |_||_| \__||_| \__,_||_||_|/___|\__,_| \__||_| \___/ |_| |_|  //
//                                                                                                           //
//                                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef TCC2
static void Timer2Init(TC2_t *port)
{
  port->CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1
  port->CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  port->CTRLC = 0; // when timer not running, sets compare (13.9.3)
  port->CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
  port->CTRLF = 0;   // not resetting or anything (13.9.7)

  port->LPER = 255; // count 255 to 0 (total period = 256)
  port->HPER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  This, however, would generate a '1' output.

  port->LCMPA = 255;
  port->LCMPB = 255;
  port->LCMPC = 255;
  port->LCMPD = 255;

  port->HCMPA = 255;
  port->HCMPB = 255;
  port->HCMPC = 255;
  port->HCMPD = 255;

  // disable underflow and comparison interrupts
  port->INTCTRLA = 0;   // no underflow interrupts
  port->INTCTRLB = 0;   // no comparison interrupts
}
#elif defined(TCC0) // use TC0_t
static void Timer2Init(TC0_t *port)
{
  // TCC2
  // first the clock selection
  port->CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1
  port->CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  TCC2_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  port->CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
#ifdef TCC0_CTRLFCLR // NOTE:  is this correct?
  port->CTRLFCLR = 0xff; // this does NOT map to anything for TC_t, but I want zeros in CTRLF so 'just in case'
  port->CTRLFSET = 0; // this maps to 'CTRLF' for TC2_t, and this assignment should work on its own
#else // TCC0_CTRLFCLR
  // NOTE: this code will probably NOT be compiled due to the way TC0_t maps to TC2_t
  port->CTRLF = 0;   // not resetting or anything (13.9.7)
#endif // TCC0_CTRLFCLR

  port->PER = 255;
//  TCC2_LPER = 255; // count 255 to 0 (total period = 256)
//  TCC2_HPER = 255; // should this be zero?

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  This, however, would generate a '1' output.

//  ((uint8_t *)&(port->CCA))[0] = 255; // low bytes
//  ((uint8_t *)&(port->CCB))[0] = 255;
//  ((uint8_t *)&(port->CCC))[0] = 255;
//  ((uint8_t *)&(port->CCD))[0] = 255;

//  ((uint8_t *)&(port->CCA))[1] = 255; // high bytes
//  ((uint8_t *)&(port->CCB))[1] = 255;
//  ((uint8_t *)&(port->CCC))[1] = 255;
//  ((uint8_t *)&(port->CCD))[1] = 255;

  // NOTE:  according to the docs, 16-bit registers MUST be accessed
  //        low byte first, then high byte, before the actual value
  //        is transferred to the register.  THIS code will.
  //        see A1U manual sect. 3.11 (and others as well)

  port->CCA = 0xffff;
  port->CCB = 0xffff;
  port->CCC = 0xffff;
  port->CCD = 0xffff;

  // disable underflow and comparison interrupts
  port->INTCTRLA = 0;   // no underflow interrupts
  port->INTCTRLB = 0;   // no comparison interrupts
}
#endif // TCC2


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//        ____               _                     ___         _  _         //
//       / ___|  _   _  ___ | |_  ___  _ __ ___   |_ _| _ __  (_)| |_       //
//       \___ \ | | | |/ __|| __|/ _ \| '_ ` _ \   | | | '_ \ | || __|      //
//        ___) || |_| |\__ \| |_|  __/| | | | | |  | | | | | || || |_       //
//       |____/  \__, ||___/ \__|\___||_| |_| |_| |___||_| |_||_| \__|      //
//               |___/                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// NOTE:  calibration data for ADC must be loaded BEFORE it's initialized
// ADCA.CALL = readCalibrationData(&PRODSIGNATURES_ADCACAL0);
// ADCA.CALH = readCalibrationData(&PRODSIGNATURES_ADCACAL1);

void init()
{


  cli(); // do this before _ANYTHING_

  // ----------------------------------------------------------
  // first thing first - the system clock _MUST_ run at 32Mhz
  // ----------------------------------------------------------

  clock_setup();


  // The watchdog timer MUST be off (the bootloader should do this too)
  // this next section of code will disable it.

  CCP = CCP_IOREG_gc; // 0xd8 - see D manual, sect 3.14.1 (protected I/O)
  WDT_CTRL = 1; // sets watchdog timer "enable" bit to zero - bit 0 must be set to change bit 1 - section 9.7.1
  CCP = CCP_IOREG_gc; // 0xd8 - see D manual, sect 3.14.1 (protected I/O)
  WDT_WINCTRL = 1; // sets watchdog 'window' timer "enable" bit to zero - bit 0 must be set to change bit 1 - section 9.7.2

  NVM_INTCTRL = 0; // disable interrupts in the NVM subsystem

#ifdef WEXC_OUTOVDIS
  WEXC_OUTOVDIS = 0; // in essence, it should allow waveform output on all pins (default value)
                     // assigning this to FFH prevents PWM output on PORTC - does not appear to affect PORTD

  WEXC_CTRL = 0;     // hopefully disabling everything
  WEXC_SWAP = 0;     // no bit swapping
  WEXC_PGO = 0;      // disable PGV output on all bits

#endif // WEXC_OUTOVDIS

#ifdef HIRESC_CTRLA
  HIRESC_CTRLA = 0; // disable hi-res timer extension
#endif // HIRESC_CTRLA


  // --------------------------------
  //   INITIAL TIMER CONFIGURATION
  // --------------------------------

  // For the E series, set up timers TCC4, TCC5, and TCD5 in 'normal' mode with a pre-scale of 64.
  // TODO:  consider CC ISR for PWM and manual bit-flip, if it's even possible.
  //
  // For everything else, set up timers TCC2 and TCD2 and TCE0.  Use pre-scale of 64.
  // If other timers exist (like 'A' series) initialize them as well.
  //
  // For a 32Mhz clock they will run at 2Khz with the appropriate pre-scale + divide.
  // For PWM out, use the comparison result to drive the appropriate pins.

  // If you don't need PWM, or want 'other than 2khz', you can re-configure the other timers,
  // but leave TCD2 (or TCD5) alone because it's needed for the system clock (via TCD2_LUNF_vect, etc.)


#ifdef TCC4 /* this is my trigger for 'E' series */

  // TCD5 first (the system timer)
  TCD5_INTCTRLA = 0;   // no underflow interrupts
  TCD5_INTCTRLB = 0;   // no comparison interrupts

  TCD5_CTRLA = 5; // b0101 - divide by 64 - E manual 13.13.1
//  TCD5_CTRLB = TC45_BYTEM_BYTEMODE_gc; // byte mode, normal mode
  TCD5_CTRLB = TC45_BYTEM_BYTEMODE_gc | TC45_WGMODE_SINGLESLOPE_gc; // byte mode, single slope
//  TCD5_CTRLB = TC45_BYTEM_BYTEMODE_gc | TC45_WGMODE_DSBOTH_gc; // byte mode, dual slope, ovf on bottom AND top
////  TCD5_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCD5_CTRLD = 0; // events off
  TCD5_CTRLE = 0; // no output on L pins
  TCD5_CTRLF = 0; // no output on H pins

  TCD5_PER = 255; // 255 for period limit

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.

  TCD5_CCA = 65535;
  TCD5_CCB = 65535;

  TCD5_CTRLGCLR = 0xfe;
  TCD5_CTRLGSET = 1; // count DOWN

  // enable the underflow interrupt on A, disable on B, disable comparison interrupts
  TCD5_INTCTRLA = 0x3; // enable LOW underflow interrupt, pri level 3 (see 13.9.5 in D manual)

// TODO:  this is not well documented - does it even work for TIMER D5 ??
#ifdef TCD5_PIN_SHIFT /* shifting PWM output pins, normally 4,5,6,7 */
  PORTD_REMAP = (PORTD_REMAP & PORT_USART0_bm) | TCD5_PIN_SHIFT;
#else  // PORTD_REMAP
  PORTD_REMAP &= PORT_USART0_bm; // all other pins are zero except maybe USART0 remap
#endif // PORTD_REMAP


// if 'HIRES' enabled, shut it off
#ifdef HIRESC_ENABLE
  HIRESC_ENABLE = HIRES_HREN_NONE_gc;
#endif // HIRESC_ENABLE

  PORTC_REMAP &= PORT_USART0_bm; // all other pins are zero except maybe USART0 remap

  // TCC4
  // first the clock selection
  TCC4_CTRLA = 5; // b0101 - divide by 64 - E manual 13.13.1
  TCC4_CTRLB = TC45_BYTEM_BYTEMODE_gc | TC45_WGMODE_SINGLESLOPE_gc; // byte mode, single slope
//  TCC4_CTRLB = TC45_BYTEM_BYTEMODE_gc | TC45_WGMODE_DSBOTH_gc; // byte mode, dual slope, ovf on bottom AND top
////  TCC4_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCC4_CTRLD = 0; // events off
  TCC4_CTRLE = 0; // no output on L pins
  TCC4_CTRLF = 0; // no output on H pins

  TCC4_PER = 255; // 255 for period limit

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.

  TCC4_CCA = 65535;
  TCC4_CCB = 65535;
  TCC4_CCC = 65535;
  TCC4_CCD = 65535;

  TCC4_CTRLGCLR = 0xfe;
  TCC4_CTRLGSET = 1; // count DOWN

  // disable underflow and comparison interrupts
  TCC4_INTCTRLA = 0;   // no underflow interrupts
  TCC4_INTCTRLB = 0;   // no comparison interrupts


  // also set up TCC5

#ifdef TCC5

  TCC5_INTCTRLA = 0;   // no underflow interrupts
  TCC5_INTCTRLB = 0;   // no comparison interrupts

  TCC5_CTRLA = 5; // b0101 - divide by 64 - E manual 13.13.1
  TCC5_CTRLB = TC45_WGMODE_NORMAL_gc; // 'normal' mode, 16-bit mode
////  TCC5_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCC5_CTRLD = 0; // events off
  TCC5_CTRLE = 0; // no output on L pins
  TCC5_CTRLF = 0; // no output on H pins

  TCC5_PER = 255; // 255 for period limit

  TCC5_CCA = 0;
  TCC5_CCB = 0;

  TCC5_CTRLGCLR = 0xff;
  TCC5_CTRLGSET = 0; // count UP

  // disable underflow and comparison interrupts
  TCC5_INTCTRLA = 0;   // no underflow interrupts
  TCC5_INTCTRLB = 0;   // no comparison interrupts

#endif // TCC5


#else // everything else uses TCD2 for system timer

#ifndef TCC2 /* A1 series doesn't define this properly, so use TCC0 and TCD0, etc. */

  // TCD2
  // first the clock selection
  TCD0_CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1 (should be the same for 'A' and others)
  TCD0_CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  TCD0_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCD0_CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
#ifdef TCD0_CTRLFCLR
  TCD0_CTRLFCLR = 0xff;
#else // TCD0_CTRLFCLR
  TCD0_CTRLF = 0;   // not resetting or anything (13.9.7)
#endif // TCD0_CTRLFCLR

  TCD0_PER = 255;
//  TCD2_LPER = 255; // count 255 to 0 (total period = 256)
//  TCD2_HPER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  use FFFFH in the compare registers.

  // NOTE:  according to the docs, 16-bit registers MUST be accessed
  //        low byte first, then high byte, before the actual value
  //        is transferred to the register.  THIS code will do that.
  //        see A1U manual sect. 3.11 (and others as well)

  TCD0_CCA = 0xffff;
  TCD0_CCB = 0xffff;
  TCD0_CCC = 0xffff;
  TCD0_CCD = 0xffff;

  // enable the underflow interrupt on A, disable on B, disable comparison interrupts
  TCD0_INTCTRLA = 0x3; // enable LOW underflow interrupt, pri level 3 (see 13.9.5 in D manual)
  TCD0_INTCTRLB = 0;   // no comparison or underflow interrupts on anything else

  Timer2Init(&TCC0);

#else // TCC2

  // TCD2
  // first the clock selection
  TCD2_CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1
  TCD2_CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  TCD2_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCD2_CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
  TCD2_CTRLF = 0;   // not resetting or anything (13.9.7)

  TCD2_LPER = 255; // count 255 to 0 (total period = 256)
  TCD2_HPER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  Timer 2 regs are 8-bit.

  TCD2_LCMPA = 255;
  TCD2_LCMPB = 255;
  TCD2_LCMPC = 255;
  TCD2_LCMPD = 255;

  TCD2_HCMPA = 255;
  TCD2_HCMPB = 255;
  TCD2_HCMPC = 255;
  TCD2_HCMPD = 255;

  // enable the underflow interrupt on A, disable on B, disable comparison interrupts
  TCD2_INTCTRLA = 0x3; // enable LOW underflow interrupt, pri level 3 (see 13.9.5 in D manual)
  TCD2_INTCTRLB = 0;   // no comparison or underflow interrupts on anything else

  Timer2Init(&TCC2);

#endif // TCC2

#endif // TCD5 or TCD2


#if NUM_DIGITAL_PINS > 22 /* meaning PORTE is available and has 8 pins */

#if !defined(TCE2) && defined(TCE0)

  Timer2Init(&TCE0);

#elif defined(TCE2) // TCE2 defined, use that

  Timer2Init(&TCE2);

#endif // TCE2, TCE0


#if NUM_DIGITAL_PINS > 30 /* meaning PORTF exists */

#if !defined(TCF2) && defined(TCF0)

  Timer2Init(&TCF0);

#elif defined(TCF2) // TCF2 defined, use that

  Timer2Init(&TCF2);

#endif // TCF2, TCF0


// TODO:  other timers on other ports when more than 38 pins available?


#endif // NUM_DIGITAL_PINS > 30


#elif NUM_DIGITAL_PINS > 18 /* meaning there is a PORT E available with only 4 pins */

  // now set up TCE0 as an 8-bit timer so it's compatible with Arduino's PWM
  // first the clock selection
  TCE0_CTRLA = 5; // b0101 - divide by 64 - D manual 12.11.1
  TCE0_CTRLB = TC_WGMODE_SS_gc; // single-slope PWM.  NOTE:  this counts UP, whereas the other timers count DOWN
               // other bits (high nybble) are OFF - they enable output on the 4 port E pins
//  TCE0_CTRLC = 0; // when timer not running, sets compare (12.11.3)
  TCE0_CTRLD = 0; // not an event timer, 16-bit mode (12.11.4)
  TCE0_CTRLE = 1; // normal 8-bit timer (set to 0 for 16-bit mode) (12.11.5)

  // disable under/overflow and comparison interrupts
  TCE0_INTCTRLA = 0;   // no underflow interrupts
  TCE0_INTCTRLB = 0;   // no comparison interrupts

  // make sure the timer E 'period' register is correctly set at 255 (i.e. 0-255 or 256 clock cycles).
  TCE0_PER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // timer 0 can be configured to count UP or DOWN, but for single-slope PWM it is
  // always 'UP'.  A value of '255' should generate a '1' output for each PWM.

  TCE0_CCA = 255;
  TCE0_CCB = 255;
  TCE0_CCC = 255;
  TCE0_CCD = 255;

#endif // NUM_DIGITAL_PINS > 18, 22



  // in case the bootloader enabled serial or TWI, disable it
  // and make sure the associated port input pins are inputs
  //
  // NOTE:  Port R pins 0 and 1 will be outputs, but all others should be inputs
  //        PR0 and PR1 are designated LED output pins for this design.  PR1 is
  //        the blinking LED pin used by the bootloader.  These will NOT be re-assigned
  //        at this time, but left 'as-is'.

  // -----------------------------------------
  // DISABLE TWI (specifically TWI interrupts)
  // -----------------------------------------

#ifdef TWIC_CTRL
  TWIC_MASTER_CTRLA = 0;
  TWIC_SLAVE_CTRLA = 0;
#endif // TWIC_CTRL
#ifdef TWID_CTRL
  TWID_MASTER_CTRLA = 0;
  TWID_SLAVE_CTRLA = 0;
#endif // TWID
//#if NUM_DIGITAL_PINS > 18 /* meaning there is a PORT E available */
#ifdef TWIE_CTRL
  TWIE_MASTER_CTRLA = 0;
  TWIE_SLAVE_CTRLA = 0;
#endif // TWIE_CTRL
#ifdef TWIF_CTRL
  TWIF_MASTER_CTRLA = 0;
  TWIF_SLAVE_CTRLA = 0;
#endif // TWIF

  // --------------------
  // DISABLE SERIAL PORTS
  // --------------------

  USARTD0_CTRLA = 0; // disables interrupts
  USARTD0_CTRLB = 0; // disables TX and RX pin override
  USARTC0_CTRLA = 0; // do the same thing
  USARTC0_CTRLB = 0; // for both port C and D

#ifdef USARTC0_CTRLD
  USARTC0_CTRLD = 0;  // E5 has this register, must assign to zero
#endif // USARTC0_CTRLD  
#ifdef USARTD0_CTRLD
  USARTD0_CTRLD = 0;  // E5 has this register, must assign to zero
#endif // USARTC0_CTRLD  

  // other serial ports found on A series
#ifdef USARTDD1_CTRLA
  USARTD1_CTRLA = 0; // disables interrupts
  USARTD1_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA
#ifdef USARTDC1_CTRLA
  USARTC1_CTRLA = 0; // disables interrupts
  USARTC1_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA
#ifdef USARTDE0_CTRLA
  USARTE0_CTRLA = 0; // disables interrupts
  USARTE0_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA
#ifdef USARTDE1_CTRLA
  USARTE1_CTRLA = 0; // disables interrupts
  USARTE1_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA
#ifdef USARTDF0_CTRLA
  USARTF0_CTRLA = 0; // disables interrupts
  USARTF0_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA
#ifdef USARTDF1_CTRLA
  USARTF1_CTRLA = 0; // disables interrupts
  USARTF1_CTRLB = 0; // disables interrupts
#endif // USARTD1_CTRLA



  //--------------------------------------------------------------
  // all pins on all ports are inputs except for the LEDs on PR0,1
  //--------------------------------------------------------------

  PORTC_DIR = 0; // all 'port C' pins are now inputs
  PORTD_DIR = 0; // all 'port D' pins are now inputs
#ifdef PORTE_DIR
  PORTE_DIR = 0; // all 'port E' pins are now inputs
#endif // PORTE_DIR

#ifdef PORTF_DIR
  PORTF_DIR = 0;
#endif // PORTF_DIR

#ifdef PORTF_DIR
  PORTF_DIR = 0;
#endif // PORTF_DIR

#ifdef PORTG_DIR
  PORTG_DIR = 0;
#endif // PORTF_DIR

#ifdef PORTH_DIR
  PORTH_DIR = 0;
#endif // PORTF_DIR

// TODO:  external ram support for A series?

#ifdef PORTJ_DIR
  PORTJ_DIR = 0;
#endif // PORTF_DIR

#ifdef PORTK_DIR
  PORTK_DIR = 0;
#endif // PORTF_DIR

#ifdef PORTQ_DIR
  PORTQ_DIR = 0;
#endif // PORTF_DIR

// port R - outputs on pin 1 if LED_BUILTIN defined as 'PR1'

  PORTR_OUT = 0; // turn them off
#ifdef LED_BUILTIN
#if LED_BUILTIN == PR1
  PORTR_DIR = 2; // define as output
#else
  PORTR_DIR = 0;
#endif // LED_BUILTIN == PR1
#else
  PORTR_DIR = 0;
#endif // LED_BUILTIN not defined

  // Added code to pre-set input pins also - note PIN0CTRL through PIN7CTRL are like an array
  // also, 'PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc' evaluates to '0' and is the normal default
  memset((void *)&(PORTC.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
  memset((void *)&(PORTD.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);

#ifdef PORTE 
#if NUM_DIGITAL_PINS > 22 /* meaning there is a PORT E available and it has 8 pins */
  memset((void *)&(PORTE.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#else // NUM_DIGITAL_PINS <= 22
  memset((void *)&(PORTE.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 4);
#endif // NUM_DIGITAL_PINS > 22
#endif // PORTE defined

#ifdef PORTF_DIR
  memset((void *)&(PORTF.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#endif // PORTF_DIR

#ifdef PORTG_DIR
  memset((void *)&(PORTG.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#endif // PORTG_DIR

#ifdef PORTH_DIR
  memset((void *)&(PORTH.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#endif // PORTH_DIR

// TODO:  external ram support for A series?

#ifdef PORTJ_DIR
  memset((void *)&(PORTJ.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#endif // PORTJ_DIR

#ifdef PORTK_DIR
  memset((void *)&(PORTK.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 8);
#endif // PORTK_DIR

#ifdef PORTQ_DIR /* PORTQ only has 4 pins */
  memset((void *)&(PORTQ.PIN0CTRL), PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc, 4); // always 4?
#endif // PORTQ_DIR

  // PORT R (which is typically an output on both pins)
  PORTR.PIN0CTRL = PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc;
  PORTR.PIN1CTRL = PORT_ISC_BOTHEDGES_gc | PORT_OPC_TOTEM_gc;


  // ---------------------------------------------------
  // ANALOG INPUT PINS - 'INPUT_DISABLED' (recommended)
  // ---------------------------------------------------

  PORTA_DIR = 0; // direction bits - set all of them as input
#if defined(PORTB_DIR)// NUM_ANALOG_PINS > 8 /* meaning there is a PORT B */
  PORTB_DIR = 0;
#endif // NUM_ANALOG_PINS > 8

  // all analog pins set up for 'INPUT_DISABLED' which is recommended for analog read
  memset((void *)&(PORTA.PIN0CTRL), PORT_ISC_INPUT_DISABLE_gc | PORT_OPC_TOTEM_gc, 8);
#ifdef PORTB
#if NUM_ANALOG_INPUTS > 12
  memset((void *)&(PORTB.PIN0CTRL), PORT_ISC_INPUT_DISABLE_gc | PORT_OPC_TOTEM_gc, 8);
#elif NUM_ANALOG_INPUTS > 8
  memset((void *)&(PORTB.PIN0CTRL), PORT_ISC_INPUT_DISABLE_gc | PORT_OPC_TOTEM_gc, 4);
#endif // NUM_ANALOG_INPUTS > 8, 12
#endif // PORTB


  // TODO:  handling PORTC as analog input for 'E' series?

  // --------------------
  // INTERRUPT CONTROLLER
  // --------------------

  // FINALLY, set up the interrupt controller for priority-based interrupts
  // _AND_ enable them.  Important.  See 10.8.3 in D manual.
  //
  // This also makes sure the IVT is at the bottom of NVRAM, not the boot section
  // It's very important to make sure the IVT is pointed at 00:0000 and not someplace else
  // *BEFORE* I enable interrupts.

  *((volatile uint8_t *)&(CCP)) = CCP_IOREG_gc; // 0xd8 - see D manual, sect 3.14.1 (protected I/O)
  *((volatile uint8_t *)&(PMIC_CTRL)) = PMIC_RREN_bm | PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;


  adc_setup(); // set up the ADC (function exported from wiring_analog.c)

  // this needs to be called before setup() or some functions won't work there
  // but it's safe to enable interrupts so I shall simply do it!

  sei();
}


