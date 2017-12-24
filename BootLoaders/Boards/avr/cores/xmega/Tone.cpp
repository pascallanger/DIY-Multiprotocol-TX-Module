/* Tone.cpp

  A Tone Generator Library

  Written by Brett Hagman

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Version Modified By Date     Comments
------- ----------- -------- --------
0001    B Hagman    09/08/02 Initial coding
0002    B Hagman    09/08/18 Multiple pins
0003    B Hagman    09/08/18 Moved initialization from constructor to begin()
0004    B Hagman    09/09/26 Fixed problems with ATmega8
0005    B Hagman    09/11/23 Scanned prescalars for best fit on 8 bit timers
                    09/11/25 Changed pin toggle method to XOR
                    09/11/25 Fixed timer0 from being excluded
0006    D Mellis    09/12/29 Replaced objects with functions
0007    M Sproul    10/08/29 Changed #ifdefs from cpu to register
0008    S Kanemoto  12/06/22 Fixed for Leonardo by @maris_HY
*************************************************/

// COMPLETE re-write for ATXMega by Bob Frazier, S.F.T. Inc. - http://mrp3.com/

// NOTE:  this still only supports one tone output.  However, xmega can do more than one
//        due to the way the timers are.  In fact, 'E' series can probably do a LOT more
//        than one.  If you want to implement that, it's a public project, so get it working
//        reliably and submit the changes, thanks.



#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "pins_arduino.h"

#if defined(TCC4) || !defined(TCC2)

// in these cases this file isn't ready for prime time, so disable it for now

#ifdef TONE_SUPPORTED
#undef TONE_SUPPORTED
#endif // TONE_SUPPORTED

#else // !TCC4 && TCC2

#ifndef TONE_SUPPORTED
#define TONE_SUPPORTED // for now turn it off for 'E"
#endif // TONE_SUPPORTED

#endif // TCC4 || !TCC2


#ifdef TONE_SUPPORTED

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define PROGMEM_ORIG PROGMEM
#else // PROGMEM workaround

// to avoid the bogus "initialized variables" warning
#ifdef PROGMEM
#undef PROGMEM
#endif // PROGMEM re-define

#define PROGMEM __attribute__((section(".progmem.tone")))
#define PROGMEM_ORIG __attribute__((__progmem__))

#endif // check for GNUC >= or < 4.6


static PORT_t *pTonePort = NULL; // must assign at startup due to ISR
static uint8_t bToneMask = 0; // bitmask for tone pin
static unsigned long toggle_count = 0; // number of cycles to output

static void toneBegin(uint8_t _pin, uint8_t _div, uint16_t _per)
{
  pTonePort = (PORT_t *)portModeRegister(digitalPinToPort(_pin));
  bToneMask = digitalPinToBitMask(_pin);


  // Set the pinMode as OUTPUT
  pinMode(_pin, OUTPUT);

#if NUM_DIGITAL_PINS > 18 /* meaning there is a PORT E available */

  TCE0_INTCTRLA = 0;  // temporarily disable overflow interrupt
  TCE0_INTCTRLB = 0;  // disable other interrupts
  TCE0_CTRLA = _div;  // divisor for pre-scaler
  TCE0_CTRLB = TC_WGMODE_NORMAL_gc; // 'normal' mode (interrupt on 'overflow')
  TCE0_CTRLD = 0; // not an event timer, 16-bit mode (12.11.4)
  TCE0_CTRLE = 0;     // 16-bit mode
  TCE0_PER = _per;    // period (16-bit value)
  TCE0_INTCTRLA = 3;  // overflow int level 3 (enables interrupt)

#elif defined(TCC4) // E series and anything else with 'TCC4'

  TCC4_INTCTRLA = 0;  // temporarily disable overflow interrupt
  TCC4_INTCTRLB = 0;  // disable other interrupts
  TCC4_CTRLA = _div;  // divisor for pre-scaler
  TCC4_CTRLB = TC45_WGMODE_NORMAL_gc; // 'normal' mode (interrupt on 'overflow')
  TCC4_CTRLD = 0; // not an event timer, 16-bit mode (12.11.4)
  TCC4_CTRLE = 0;     // 16-bit mode
  TCC4_PER = _per;    // period (16-bit value)
  TCC4_INTCTRLA = 3;  // overflow int level 3 (enables interrupt)

#else // other stuff not yet explored by me

  TCC0_INTCTRLA = 0;  // temporarily disable overflow interrupt
  TCC0_INTCTRLB = 0;  // disable other interrupts
  TCC0_CTRLA = _div;  // divisor for pre-scaler
  TCC0_CTRLB = TC_WGMODE_NORMAL_gc; // 'normal' mode (interrupt on 'overflow')
  TCC0_CTRLD = 0; // not an event timer, 16-bit mode (12.11.4)
  TCC0_CTRLE = 0;     // 16-bit mode
  TCC0_PER = _per;    // period (16-bit value)
  TCC0_INTCTRLA = 3;  // overflow int level 3 (enables interrupt)

#endif // NUM_DIGITAL_PINS > 18

  // tone starts now, shuts off when the 'toggle_count' hits zero
}

// frequency (in hertz) and duration (in milliseconds).

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
{
register int8_t b1;
unsigned short per, w2;
unsigned long ulTemp;
static const uint16_t aPreScaler[] PROGMEM = {1,2,4,8,64,256,1024}; // pre-scaler

  // frequency

  // based on the frequency, set up the divider and period
  // period is 16-bits

  // NOTE:  use the smallest possible divisor

  if(!frequency)
  {
    return;
  }

  ulTemp = frequency * 16384L; // ideal counter 16384

  for(b1=sizeof(aPreScaler)/sizeof(aPreScaler[0]) - 1; b1 > 0; b1--)
  {
    w2 = pgm_read_word(&(aPreScaler[0]) + b1);
    if(((unsigned long)F_CPU / 2 / w2) >= ulTemp) // note that I flip the bit every OTHER cycle
    {
      break;
    }
  }

  if(!b1)
  {
    w2 = 1; // make sure
  }

  // b1 is the divisor bit value for CTRLA, per caches the actual divisor

  per = (F_CPU / 2 / w2) / frequency;
  if(!per)
  {
    per++;
  }

  // Calculate the toggle count
  if (duration > 0)
  {
    toggle_count = 2 * frequency * duration / 1000;
  }
  else
  {
    toggle_count = -1;
  }

  toneBegin(_pin, b1, per);
}

// XXX: this function only works properly for timer E (the only one we use
// currently).  Since I use the ISR on timer E to toggle the pin, it should
// be just fine.

void disableTimer(uint8_t _timer)
{
// parameter is ignored

#if NUM_DIGITAL_PINS > 18 /* meaning there is a PORT E available */

  // disable under/overflow and comparison interrupts FIRST
  TCE0_INTCTRLA = 0;   // no underflow interrupts
  TCE0_INTCTRLB = 0;   // no comparison interrupts

  pTonePort = NULL; // make sure

  // re-assign TCE0 defaults.  see 'wiring.c'

#if NUM_DIGITAL_PINS > 22 /* meaning PORTE has 8 pins */

  TCE2_CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1
  TCE2_CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  TCE2_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCE2_CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
  TCE2_CTRLF = 0;   // not resetting or anything (13.9.7)

  TCE2_LPER = 255; // count 255 to 0 (total period = 256)
  TCE2_HPER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  This, however, would generate a '1' output.

  TCE2_LCMPA = 255;
  TCE2_LCMPB = 255;
  TCE2_LCMPC = 255;
  TCE2_LCMPD = 255;

  TCE2_HCMPA = 255;
  TCE2_HCMPB = 255;
  TCE2_HCMPC = 255;
  TCE2_HCMPD = 255;

  TCE2_INTCTRLA = 0;   // no underflow interrupts
  TCE2_INTCTRLB = 0;   // no comparison interrupts

#else // 16-bit timer on TCE0

  TCE0_CTRLA = 5; // b0101 - divide by 64 - D manual 12.11.1
  TCE0_CTRLB = TC_WGMODE_SS_gc; // single-slope PWM.  NOTE:  this counts UP, whereas the other timers count DOWN
               // other bits (high nybble) are OFF - they enable output on the 4 port E pins
//  TCE0_CTRLC = 0; // when timer not running, sets compare (12.11.3)
  TCE0_CTRLD = 0; // not an event timer, 16-bit mode (12.11.4)
  TCE0_CTRLE = 1; // normal 8-bit timer (set to 0 for 16-bit mode) (12.11.5)

  // make sure the timer E 'period' register is correctly set at 255 (i.e. 0-255 or 256 clock cycles).
  TCE0_PER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // timer 0 can be configured to count UP or DOWN, but for single-slope PWM it is
  // always 'UP'.  A value of '255' should generate a '1' output for each PWM.

  TCE0_CCA = 255;
  TCE0_CCB = 255;
  TCE0_CCC = 255;
  TCE0_CCD = 255;

#endif // 8/16 bit timer on E

#elif defined(TCC4) // E series and anything else with 'TCC4'

  // disable under/overflow and comparison interrupts FIRST
  TCC4_INTCTRLA = 0;   // no underflow interrupts
  TCC4_INTCTRLB = 0;   // no comparison interrupts

  pTonePort = NULL; // make sure

  // re-assign TCC0 defaults.  see 'wiring.c'

  TCC4_CTRLA = 5; // b0101 - divide by 64 - E manual 13.13.1
  TCC4_CTRLB = TC45_BYTEM_BYTEMODE_gc | TC45_WGMODE_SINGLESLOPE_gc; // byte mode, single slope
//  TCC5_CTRLC = 0; // when timer not running, sets compare (13.9.3)
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

#else // other stuff not yet explored by me

  // disable under/overflow and comparison interrupts FIRST
  TCC0_INTCTRLA = 0;   // no underflow interrupts
  TCC0_INTCTRLB = 0;   // no comparison interrupts

  pTonePort = NULL; // make sure

  // re-assign TCC0 defaults.  see 'wiring.c'

  TCC2_CTRLA = 5; // b0101 - divide by 64 - D manual 13.9.1
  TCC2_CTRLB = 0; // compare outputs disabled on all 8 bits (13.9.2)
//  TCC2_CTRLC = 0; // when timer not running, sets compare (13.9.3)
  TCC2_CTRLE = 0x2; // b10 - 'split' mode - D manual 13.9.4
  TCC2_CTRLF = 0;   // not resetting or anything (13.9.7)

  TCC2_LPER = 255; // count 255 to 0 (total period = 256)
  TCC2_HPER = 255;

  // pre-assign comparison registers to 'zero' (for PWM out) which is actually 255
  // 'timer 2' counts DOWN.  This, however, would generate a '1' output.

  TCC2_LCMPA = 255;
  TCC2_LCMPB = 255;
  TCC2_LCMPC = 255;
  TCC2_LCMPD = 255;

  TCC2_HCMPA = 255;
  TCC2_HCMPB = 255;
  TCC2_HCMPC = 255;
  TCC2_HCMPD = 255;

  TCC2_INTCTRLA = 0;   // no underflow interrupts
  TCC2_INTCTRLB = 0;   // no comparison interrupts

#endif // NUM_DIGITAL_PINS > 18
}

void noTone(uint8_t _pin)
{
  disableTimer(0);

  digitalWrite(_pin, 0);
}

#if NUM_DIGITAL_PINS > 18 /* meaning PORTE exists */
ISR(TCE0_OVF_vect) // the 'overflow' vector on timer E0
#elif defined(TCC4) // E series and anything else with 'TCC4'
ISR(TCC4_OVF_vect) // the 'overflow' vector on timer C4
#else // everything else
ISR(TCC0_OVF_vect) // the 'overflow' vector on timer C0
#endif // PORTE exist check
{
  if(!toggle_count || !pTonePort || !bToneMask
#if 1 /* this section in for bullet-proofing, consider removing */
     || (pTonePort != &PORTA &&
#if NUM_ANALOG_PINS > 8
         pTonePort != &PORTB &&
#endif // NUM_ANALOG_PINS > 8
         pTonePort != &PORTC && pTonePort != &PORTD &&
#if NUM_DIGITAL_PINS > 18
         pTonePort != &PORTE &&
#endif // PORTE exist check
         pTonePort != &PORTR)
#endif // 1
    )
  {
    // disable the timer (also disables the interrupt)
    disableTimer(0);
    return;
  }

  // each time I get an overflow, toggle the tone pin

  pTonePort->OUTTGL = bToneMask; // toggle that bit
  toggle_count--;
}



#if 0 // OLD CODE for reference only

#if defined(__AVR_ATmega8__) || defined(__AVR_ATmega128__)
#define TCCR2A TCCR2
#define TCCR2B TCCR2
#define COM2A1 COM21
#define COM2A0 COM20
#define OCR2A OCR2
#define TIMSK2 TIMSK
#define OCIE2A OCIE2
#define TIMER2_COMPA_vect TIMER2_COMP_vect
#define TIMSK1 TIMSK
#endif

// timerx_toggle_count:
//  > 0 - duration specified
//  = 0 - stopped
//  < 0 - infinitely (until stop() method called, or new play() called)

#if !defined(__AVR_ATmega8__)
volatile long timer0_toggle_count;
volatile uint8_t *timer0_pin_port;
volatile uint8_t timer0_pin_mask;
#endif

volatile long timer1_toggle_count;
volatile uint8_t *timer1_pin_port;
volatile uint8_t timer1_pin_mask;
volatile long timer2_toggle_count;
volatile uint8_t *timer2_pin_port;
volatile uint8_t timer2_pin_mask;

#if defined(TIMSK3)
volatile long timer3_toggle_count;
volatile uint8_t *timer3_pin_port;
volatile uint8_t timer3_pin_mask;
#endif

#if defined(TIMSK4)
volatile long timer4_toggle_count;
volatile uint8_t *timer4_pin_port;
volatile uint8_t timer4_pin_mask;
#endif

#if defined(TIMSK5)
volatile long timer5_toggle_count;
volatile uint8_t *timer5_pin_port;
volatile uint8_t timer5_pin_mask;
#endif


#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#define AVAILABLE_TONE_PINS 1
#define USE_TIMER2

const uint8_t PROGMEM tone_pin_to_timer_PGM[] = { 2 /*, 3, 4, 5, 1, 0 */ };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255 /*, 255, 255, 255, 255, 255 */ };

#elif defined(__AVR_ATmega8__)

#define AVAILABLE_TONE_PINS 1
#define USE_TIMER2

const uint8_t PROGMEM tone_pin_to_timer_PGM[] = { 2 /*, 1 */ };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255 /*, 255 */ };

#elif defined(__AVR_ATmega32U4__)

#define AVAILABLE_TONE_PINS 1
#define USE_TIMER3

const uint8_t PROGMEM tone_pin_to_timer_PGM[] = { 3 /*, 1 */ };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255 /*, 255 */ };

#else

#define AVAILABLE_TONE_PINS 1
#define USE_TIMER2

// Leave timer 0 to last.
const uint8_t PROGMEM tone_pin_to_timer_PGM[] = { 2 /*, 1, 0 */ };
static uint8_t tone_pins[AVAILABLE_TONE_PINS] = { 255 /*, 255, 255 */ };

#endif

// NOTE:  K&R coding style edited away.  Allman style rules - BF

static int8_t toneBegin(uint8_t _pin)
{
  int8_t _timer = -1;

  // if we're already using the pin, the timer should be configured.
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++)
  {
    if (tone_pins[i] == _pin)
    {
      return pgm_read_byte(tone_pin_to_timer_PGM + i);
    }
  }

  // search for an unused timer.
  for (int i = 0; i < AVAILABLE_TONE_PINS; i++)
  {
    if (tone_pins[i] == 255)
    {
      tone_pins[i] = _pin;
      _timer = pgm_read_byte(tone_pin_to_timer_PGM + i);
      break;
    }
  }

  if (_timer != -1)
  {
    // Set timer specific stuff
    // All timers in CTC mode
    // 8 bit timers will require changing prescalar values,
    // whereas 16 bit timers are set to either ck/1 or ck/64 prescalar
    switch (_timer)
    {
      #if defined(TCCR0A) && defined(TCCR0B)
      case 0:
        // 8 bit timer
        TCCR0A = 0;
        TCCR0B = 0;
        bitWrite(TCCR0A, WGM01, 1);
        bitWrite(TCCR0B, CS00, 1);
        timer0_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer0_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif

      #if defined(TCCR1A) && defined(TCCR1B) && defined(WGM12)
      case 1:
        // 16 bit timer
        TCCR1A = 0;
        TCCR1B = 0;
        bitWrite(TCCR1B, WGM12, 1);
        bitWrite(TCCR1B, CS10, 1);
        timer1_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer1_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif

      #if defined(TCCR2A) && defined(TCCR2B)
      case 2:
        // 8 bit timer
        TCCR2A = 0;
        TCCR2B = 0;
        bitWrite(TCCR2A, WGM21, 1);
        bitWrite(TCCR2B, CS20, 1);
        timer2_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer2_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif

      #if defined(TCCR3A) && defined(TCCR3B) &&  defined(TIMSK3)
      case 3:
        // 16 bit timer
        TCCR3A = 0;
        TCCR3B = 0;
        bitWrite(TCCR3B, WGM32, 1);
        bitWrite(TCCR3B, CS30, 1);
        timer3_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer3_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif

      #if defined(TCCR4A) && defined(TCCR4B) &&  defined(TIMSK4)
      case 4:
        // 16 bit timer
        TCCR4A = 0;
        TCCR4B = 0;
        #if defined(WGM42)
          bitWrite(TCCR4B, WGM42, 1);
        #elif defined(CS43)
          #warning this may not be correct
          // atmega32u4
          bitWrite(TCCR4B, CS43, 1);
        #endif
        bitWrite(TCCR4B, CS40, 1);
        timer4_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer4_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif

      #if defined(TCCR5A) && defined(TCCR5B) &&  defined(TIMSK5)
      case 5:
        // 16 bit timer
        TCCR5A = 0;
        TCCR5B = 0;
        bitWrite(TCCR5B, WGM52, 1);
        bitWrite(TCCR5B, CS50, 1);
        timer5_pin_port = portOutputRegister(digitalPinToPort(_pin));
        timer5_pin_mask = digitalPinToBitMask(_pin);
        break;
      #endif
    }
  }

  return _timer;
}



// frequency (in hertz) and duration (in milliseconds).

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration)
{
  uint8_t prescalarbits = 0b001;
  long toggle_count = 0;
  uint32_t ocr = 0;
  int8_t _timer;

  _timer = toneBegin(_pin);

  if (_timer >= 0)
  {
    // Set the pinMode as OUTPUT
    pinMode(_pin, OUTPUT);

    // if we are using an 8 bit timer, scan through prescalars to find the best fit
    if (_timer == 0 || _timer == 2)
    {
      ocr = F_CPU / frequency / 2 - 1;
      prescalarbits = 0b001;  // ck/1: same for both timers
      if (ocr > 255)
      {
        ocr = F_CPU / frequency / 2 / 8 - 1;
        prescalarbits = 0b010;  // ck/8: same for both timers

        if (_timer == 2 && ocr > 255)
        {
          ocr = F_CPU / frequency / 2 / 32 - 1;
          prescalarbits = 0b011;
        }

        if (ocr > 255)
        {
          ocr = F_CPU / frequency / 2 / 64 - 1;
          prescalarbits = _timer == 0 ? 0b011 : 0b100;

          if (_timer == 2 && ocr > 255)
          {
            ocr = F_CPU / frequency / 2 / 128 - 1;
            prescalarbits = 0b101;
          }

          if (ocr > 255)
          {
            ocr = F_CPU / frequency / 2 / 256 - 1;
            prescalarbits = _timer == 0 ? 0b100 : 0b110;
            if (ocr > 255)
            {
              // can't do any better than /1024
              ocr = F_CPU / frequency / 2 / 1024 - 1;
              prescalarbits = _timer == 0 ? 0b101 : 0b111;
            }
          }
        }
      }

#if defined(TCCR0B)
      if (_timer == 0)
      {
        TCCR0B = prescalarbits;
      }
      else
#endif
#if defined(TCCR2B)
      {
        TCCR2B = prescalarbits;
      }
#else
      {
        // dummy place holder to make the above ifdefs work
      }
#endif
    }
    else
    {
      // two choices for the 16 bit timers: ck/1 or ck/64
      ocr = F_CPU / frequency / 2 - 1;

      prescalarbits = 0b001;
      if (ocr > 0xffff)
      {
        ocr = F_CPU / frequency / 2 / 64 - 1;
        prescalarbits = 0b011;
      }

      if (_timer == 1)
      {
#if defined(TCCR1B)
        TCCR1B = (TCCR1B & 0b11111000) | prescalarbits;
#endif
      }
#if defined(TCCR3B)
      else if (_timer == 3)
        TCCR3B = (TCCR3B & 0b11111000) | prescalarbits;
#endif
#if defined(TCCR4B)
      else if (_timer == 4)
        TCCR4B = (TCCR4B & 0b11111000) | prescalarbits;
#endif
#if defined(TCCR5B)
      else if (_timer == 5)
        TCCR5B = (TCCR5B & 0b11111000) | prescalarbits;
#endif

    }


    // Calculate the toggle count
    if (duration > 0)
    {
      toggle_count = 2 * frequency * duration / 1000;
    }
    else
    {
      toggle_count = -1;
    }

    // Set the OCR for the given timer,
    // set the toggle count,
    // then turn on the interrupts
    switch (_timer)
    {

#if defined(OCR0A) && defined(TIMSK0) && defined(OCIE0A)
      case 0:
        OCR0A = ocr;
        timer0_toggle_count = toggle_count;
        bitWrite(TIMSK0, OCIE0A, 1);
        break;
#endif

      case 1:
#if defined(OCR1A) && defined(TIMSK1) && defined(OCIE1A)
        OCR1A = ocr;
        timer1_toggle_count = toggle_count;
        bitWrite(TIMSK1, OCIE1A, 1);
#elif defined(OCR1A) && defined(TIMSK) && defined(OCIE1A)
        // this combination is for at least the ATmega32
        OCR1A = ocr;
        timer1_toggle_count = toggle_count;
        bitWrite(TIMSK, OCIE1A, 1);
#endif
        break;

#if defined(OCR2A) && defined(TIMSK2) && defined(OCIE2A)
      case 2:
        OCR2A = ocr;
        timer2_toggle_count = toggle_count;
        bitWrite(TIMSK2, OCIE2A, 1);
        break;
#endif

#if defined(TIMSK3)
      case 3:
        OCR3A = ocr;
        timer3_toggle_count = toggle_count;
        bitWrite(TIMSK3, OCIE3A, 1);
        break;
#endif

#if defined(TIMSK4)
      case 4:
        OCR4A = ocr;
        timer4_toggle_count = toggle_count;
        bitWrite(TIMSK4, OCIE4A, 1);
        break;
#endif

#if defined(OCR5A) && defined(TIMSK5) && defined(OCIE5A)
      case 5:
        OCR5A = ocr;
        timer5_toggle_count = toggle_count;
        bitWrite(TIMSK5, OCIE5A, 1);
        break;
#endif

    }
  }
}


// XXX: this function only works properly for timer 2 (the only one we use
// currently).  for the others, it should end the tone, but won't restore
// proper PWM functionality for the timer.
void disableTimer(uint8_t _timer)
{
  switch (_timer)
  {
    case 0:
      #if defined(TIMSK0)
        TIMSK0 = 0;
      #elif defined(TIMSK)
        TIMSK = 0; // atmega32
      #endif
      break;

#if defined(TIMSK1) && defined(OCIE1A)
    case 1:
      bitWrite(TIMSK1, OCIE1A, 0);
      break;
#endif

    case 2:
      #if defined(TIMSK2) && defined(OCIE2A)
        bitWrite(TIMSK2, OCIE2A, 0); // disable interrupt
      #endif
      #if defined(TCCR2A) && defined(WGM20)
        TCCR2A = (1 << WGM20);
      #endif
      #if defined(TCCR2B) && defined(CS22)
        TCCR2B = (TCCR2B & 0b11111000) | (1 << CS22);
      #endif
      #if defined(OCR2A)
        OCR2A = 0;
      #endif
      break;

#if defined(TIMSK3)
    case 3:
      TIMSK3 = 0;
      break;
#endif

#if defined(TIMSK4)
    case 4:
      TIMSK4 = 0;
      break;
#endif

#if defined(TIMSK5)
    case 5:
      TIMSK5 = 0;
      break;
#endif
  }
}

// XXX: this function only works properly for timer 2 (the only one we use
// currently).  for the others, it should end the tone, but won't restore
// proper PWM functionality for the timer.
void disableTimer(uint8_t _timer)

void noTone(uint8_t _pin)
{
  int8_t _timer = -1;

  for (int i = 0; i < AVAILABLE_TONE_PINS; i++)
  {
    if (tone_pins[i] == _pin)
    {
      _timer = pgm_read_byte(tone_pin_to_timer_PGM + i);
      tone_pins[i] = 255;
    }
  }

  disableTimer(_timer);

  digitalWrite(_pin, 0);
}

#ifdef USE_TIMER0
ISR(TIMER0_COMPA_vect)
{
  if (timer0_toggle_count != 0)
  {
    // toggle the pin
    *timer0_pin_port ^= timer0_pin_mask;

    if (timer0_toggle_count > 0)
      timer0_toggle_count--;
  }
  else
  {
    disableTimer(0);
    *timer0_pin_port &= ~(timer0_pin_mask);  // keep pin low after stop
  }
}
#endif


#ifdef USE_TIMER1
ISR(TIMER1_COMPA_vect)
{
  if (timer1_toggle_count != 0)
  {
    // toggle the pin
    *timer1_pin_port ^= timer1_pin_mask;

    if (timer1_toggle_count > 0)
      timer1_toggle_count--;
  }
  else
  {
    disableTimer(1);
    *timer1_pin_port &= ~(timer1_pin_mask);  // keep pin low after stop
  }
}
#endif


#ifdef USE_TIMER2
ISR(TIMER2_COMPA_vect)
{

  if (timer2_toggle_count != 0)
  {
    // toggle the pin
    *timer2_pin_port ^= timer2_pin_mask;

    if (timer2_toggle_count > 0)
      timer2_toggle_count--;
  }
  else
  {
    // need to call noTone() so that the tone_pins[] entry is reset, so the
    // timer gets initialized next time we call tone().
    // XXX: this assumes timer 2 is always the first one used.
    noTone(tone_pins[0]);
//    disableTimer(2);
//    *timer2_pin_port &= ~(timer2_pin_mask);  // keep pin low after stop
  }
}
#endif


#ifdef USE_TIMER3
ISR(TIMER3_COMPA_vect)
{
  if (timer3_toggle_count != 0)
  {
    // toggle the pin
    *timer3_pin_port ^= timer3_pin_mask;

    if (timer3_toggle_count > 0)
      timer3_toggle_count--;
  }
  else
  {
    disableTimer(3);
    *timer3_pin_port &= ~(timer3_pin_mask);  // keep pin low after stop
  }
}
#endif


#ifdef USE_TIMER4
ISR(TIMER4_COMPA_vect)
{
  if (timer4_toggle_count != 0)
  {
    // toggle the pin
    *timer4_pin_port ^= timer4_pin_mask;

    if (timer4_toggle_count > 0)
      timer4_toggle_count--;
  }
  else
  {
    disableTimer(4);
    *timer4_pin_port &= ~(timer4_pin_mask);  // keep pin low after stop
  }
}
#endif


#ifdef USE_TIMER5
ISR(TIMER5_COMPA_vect)
{
  if (timer5_toggle_count != 0)
  {
    // toggle the pin
    *timer5_pin_port ^= timer5_pin_mask;

    if (timer5_toggle_count > 0)
      timer5_toggle_count--;
  }
  else
  {
    disableTimer(5);
    *timer5_pin_port &= ~(timer5_pin_mask);  // keep pin low after stop
  }
}
#endif

#endif // 0 [OLD CODE]

#endif // TONE_SUPPORTED

