/*
  HardwareSerial.h - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus

  Updated for 'xmega' core by bob frazier, S.F.T. Inc. - http://mrp3.com/

  In some cases, the xmega updates make assumptions about the pin assignments.
  See 'pins_arduino.h' for more detail.

*/

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>

#include "Stream.h"

struct ring_buffer;

class HardwareSerial : public Stream
{
  protected: // NEVER 'private'
    // NOTE:  xmega uses a structure pointer rather than individual pointers
    //        and the bit flags are consistent for all USART devices
    ring_buffer *_rx_buffer;
    ring_buffer *_tx_buffer;
    volatile USART_t *_usart;
    bool transmitting;
  public:
    HardwareSerial(ring_buffer *rx_buffer, ring_buffer *tx_buffer, uint16_t usart)  __attribute__ ((noinline));
    void init(ring_buffer *rx_buffer, ring_buffer *tx_buffer, uint16_t usart) __attribute__ ((noinline));
    void begin(unsigned long);
    void begin(unsigned long, uint8_t);
    void end();
    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool();
};

// Define config for Serial.begin(baud, config);
// PMODE 5:4   00=none  10=even 11=odd
// SBMODE 3    0=1 stop  1=2 stop
// CHSIZE 2:0  000=5 bit 001=6 bit  010=7 bit  011=8 bit  111=9 bit

#define SERIAL_TWO_STOP _BV(USART_SBMODE_bp)
#define SERIAL_EVEN_PARITY _BV(USART_PMODE1_bp)
#define SERIAL_ODD_PARITY (_BV(USART_PMODE1_bp) | _BV(USART_PMODE0_bp))

#define SERIAL_5N1 0x00
#define SERIAL_6N1 0x01
#define SERIAL_7N1 0x02
#define SERIAL_8N1 0x03
#define SERIAL_5N2 (SERIAL_5N1 | SERIAL_TWO_STOP)
#define SERIAL_6N2 (SERIAL_6N1 | SERIAL_TWO_STOP)
#define SERIAL_7N2 (SERIAL_7N1 | SERIAL_TWO_STOP)
#define SERIAL_8N2 (SERIAL_8N1 | SERIAL_TWO_STOP)
#define SERIAL_5E1 (SERIAL_5N1 | SERIAL_EVEN_PARITY)
#define SERIAL_6E1 (SERIAL_6N1 | SERIAL_EVEN_PARITY)
#define SERIAL_7E1 (SERIAL_7N1 | SERIAL_EVEN_PARITY)
#define SERIAL_8E1 (SERIAL_8N1 | SERIAL_EVEN_PARITY)
#define SERIAL_5E2 (SERIAL_5N2 | SERIAL_EVEN_PARITY)
#define SERIAL_6E2 (SERIAL_6N2 | SERIAL_EVEN_PARITY)
#define SERIAL_7E2 (SERIAL_7N2 | SERIAL_EVEN_PARITY)
#define SERIAL_8E2 (SERIAL_8N2 | SERIAL_EVEN_PARITY)
#define SERIAL_5O1 (SERIAL_5N1 | SERIAL_ODD_PARITY)
#define SERIAL_6O1 (SERIAL_6N1 | SERIAL_ODD_PARITY)
#define SERIAL_7O1 (SERIAL_7N1 | SERIAL_ODD_PARITY)
#define SERIAL_8O1 (SERIAL_8N1 | SERIAL_ODD_PARITY)
#define SERIAL_5O2 (SERIAL_5N2 | SERIAL_ODD_PARITY)
#define SERIAL_6O2 (SERIAL_6N2 | SERIAL_ODD_PARITY)
#define SERIAL_7O2 (SERIAL_7N2 | SERIAL_ODD_PARITY)
#define SERIAL_8O2 (SERIAL_8N2 | SERIAL_ODD_PARITY)


// this is where I must include 'pins_arduino.h' to get the 'USBCON' definition
#include "pins_arduino.h"

// DEFAULT SERIAL or 'SERIAL 1'
#if defined(USBCON)
  // NOTE:  'Serial1' will be the hardware serial and 'Serial' the USB serial
  //        whenever 'USBCON' is defined in pins_arduino.h

  #include <USBAPI.h>

  extern HardwareSerial Serial1;
#else // normal hardware serial
  extern HardwareSerial Serial;
#define Serial1 Serial /* define as 'Serial' so compatible code won't break */
#endif

extern HardwareSerial Serial2; // this is the same regardless of USBCON (there will always be at least 2)

#ifdef SERIAL_2_PORT_NAME /* note these names are off by 1 with the 'Serial_N_' definitions */
extern HardwareSerial Serial3;
#endif // SERIAL_2_PORT_NAME
#ifdef SERIAL_3_PORT_NAME
extern HardwareSerial Serial4;
#endif // SERIAL_3_PORT_NAME
#ifdef SERIAL_4_PORT_NAME
extern HardwareSerial Serial5;
#endif // SERIAL_4_PORT_NAME
#ifdef SERIAL_5_PORT_NAME
extern HardwareSerial Serial6;
#endif // SERIAL_5_PORT_NAME
#ifdef SERIAL_6_PORT_NAME
extern HardwareSerial Serial7;
#endif // SERIAL_6_PORT_NAME
#ifdef SERIAL_7_PORT_NAME
extern HardwareSerial Serial8;
#endif // SERIAL_7_PORT_NAME

// this function calls the serial event handlers.  you can override them (hence 'weak')
// the default implementation checks for data available and executes the callback if so
extern void serialEventRun(void) __attribute__((weak));

#endif // HardwareSerial_h

