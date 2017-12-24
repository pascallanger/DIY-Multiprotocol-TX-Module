//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                  ____  ____    ____                                      //
//                 / ___||  _ \  / ___|    ___  _ __   _ __                 //
//                | |    | | | || |       / __|| '_ \ | '_ \                //
//                | |___ | |_| || |___  _| (__ | |_) || |_) |               //
//                 \____||____/  \____|(_)\___|| .__/ | .__/                //
//                                             |_|    |_|                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

/* Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

// Updated for the XMegaForArduino project by Bob Frazier, S.F.T. Inc.

/////////////////////////////////////////////////////////////////////////////////
// XMEGA NOTES:
//
// a) major re-factoring, including API functions
// b) K&R style is hard to read.  I won't use it.  Hard tabs are evil.  Same.
//
/////////////////////////////////////////////////////////////////////////////////



#include "Platform.h"
#include "USBAPI.h"
#include <avr/wdt.h>

#if defined(USBCON)
#ifdef CDC_ENABLED

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define PROGMEM_ORIG PROGMEM
#else // PROGMEM workaround

// to avoid the bogus "initialized variables" warning
#ifdef PROGMEM
#undef PROGMEM
#endif // PROGMEM re-define

#define PROGMEM __attribute__((section(".progmem.cdc")))
#define PROGMEM_ORIG __attribute__((__progmem__))

#endif // check for GNUC >= or < 4.6


typedef struct
{
  u32  dwDTERate;   // little-endian line rate
  u8   bCharFormat; // stop bits = one, one-and-a-half, two  (0, 1, 2 respectively)
  u8   bParityType; // none, odd, even, mark, space (0 through 4)
  u8   bDataBits;   // char bits 5, 6, 7, 8
} __attribute__((aligned(1))) LineInfo;

static volatile LineInfo _usbLineInfo; // for initialization, see CDC_Reset
static u8 _cdcLineState;
static u16 _cdcSerialState;
static uint16_t wInterval;

#define WEAK __attribute__ ((weak))

extern const DeviceDescriptor _cdcDeviceDescriptor PROGMEM;
extern const IADDescriptor _cdcIADDesc PROGMEM;
extern const CDCDescriptor _cdcInterface PROGMEM;


// CDC DEVICE DESCRIPTOR (for CDC device) - sent by CDC_SendDeviceDescriptor()

const DeviceDescriptor _cdcDeviceDescriptor PROGMEM =
  D_DEVICE(USB_DEVICE_CLASS_COMMUNICATIONS,     // device class (COMM)
           CDC_COMMUNICATION_INTERFACE_CLASS,   // device sub-class (CDC COMM)
           CDC_ABSTRACT_CONTROL_MODEL,          // device protocol (ACM)
           64,                                  // packet size (64)
           USB_VID,                             // vendor ID for the USB device
           USB_PID,                             // product ID for the USB device
           0x100,                               // device release version as BCD (1.00)
           USB_STRING_INDEX_MANUFACTURER,       // string index for mfg
           USB_STRING_INDEX_PRODUCT,            // string index for product name
           USB_STRING_INDEX_SERIAL,             // string index for serial number (0 for 'none')
           1);                                  // number of configurations (1)


// IAD descriptor - REQUIRED for composite interfaces, sent via CDC_SendIAD()

const IADDescriptor _cdcIADDesc = D_IAD(0,                                  // first interface
                                        2,                                  // count (interfaces, not endpoints)
                                        CDC_COMMUNICATION_INTERFACE_CLASS,  // interface class
                                        CDC_ABSTRACT_CONTROL_MODEL,         // interface sub-class
                                        1);                                 // protocol

// CDC interface descriptor - sent by CDC_SendInterfaceData()

const CDCDescriptor _cdcInterface = // needs to be no more than 55 bytes in length
{
  //  FIRST INTERFACE
  //  CDC communication interface (endpoint 0)
  D_INTERFACE(CDC_ACM_INTERFACE,                            // 'n'
              1,                                            // number of endpoints
              CDC_COMMUNICATION_INTERFACE_CLASS,            // interface class
              CDC_ABSTRACT_CONTROL_MODEL,                   // interface sub-class
              0),                                           // protocol

  // these headers describe the supported interfaces
  D_CDCCS(CDC_HEADER,0x10,0x01),                            // CDCCS InterfaceDescriptor Header (1.10 bcd) - version 1.10?
//  D_CDCCS(CDC_CALL_MANAGEMENT,1,1),                         // Device handles call management (seems to be optional)
  D_CDCCS4(CDC_ABSTRACT_CONTROL_MANAGEMENT,6),              // SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
  D_CDCCS(CDC_UNION,CDC_ACM_INTERFACE,CDC_DATA_INTERFACE),  // Communication interface is master, data interface is slave 0 (?)

  D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_ACM),            // IN endpoint for CDC_ENDPOINT_ACM
             USB_ENDPOINT_TYPE_INTERRUPT,                   // INTERRUPT type
             0x10,                                          // max packet size 16
             0x40),                                         // interval 64 frames i.e. 64 msec (see USB spec table 9-13)

  //  SECOND INTERFACE
  //  CDC data interface (endpoints 1, 2)
  D_INTERFACE(CDC_DATA_INTERFACE,                           // 'n'
              2,                                            // number of endpoints
              CDC_DATA_INTERFACE_CLASS,                     // interface class
              0,                                            // interface sub-class
              0),                                           // protocol

  D_ENDPOINT(USB_ENDPOINT_OUT(CDC_ENDPOINT_OUT),            // OUT endpoint, index 'CDC_ENDPOINT_OUT'
             USB_ENDPOINT_TYPE_BULK,                        // BULK data transfers
             0x40,                                          // max packet size 64
             1),                                            // interval 1 (was 0)

  D_ENDPOINT(USB_ENDPOINT_IN (CDC_ENDPOINT_IN),             // IN endpoint, index 'CDC_ENDPOINT_IN'
             USB_ENDPOINT_TYPE_BULK,                        // BULK data transfers
             0x40,                                          // max packet size 64
             0)                                             // interval 0 (apparently not needed)
};

void WEAK CDC_Reset(void)
{
  _usbLineInfo.dwDTERate = 115200;

  _usbLineInfo.bCharFormat
     = _usbLineInfo.bParityType
     = _usbLineInfo.bDataBits
     = 0; // says I'm not initialized, basically

  _cdcLineState = 0;
  _cdcSerialState = 0;

  wInterval = 0;
}

bool WEAK CDC_SendIAD(void)
{
  return USB_SendControl(TRANSFER_PGM, &_cdcIADDesc, sizeof(_cdcIADDesc))
         != 0;
}

int WEAK CDC_GetNumInterfaces(void)
{
  return 2; // always 2
}

int WEAK CDC_GetInterfaceDataLength(void)
{
  return sizeof(_cdcInterface);
}

int WEAK CDC_SendInterfaceData(void)
{
  return USB_SendControl(TRANSFER_PGM, &_cdcInterface, sizeof(_cdcInterface));
}

bool WEAK CDC_SendDeviceDescriptor(void)
{
  return 0 != USB_SendControl(TRANSFER_PGM, &_cdcDeviceDescriptor, sizeof(_cdcDeviceDescriptor));
}

bool WEAK CDC_Setup(Setup& setup)
{
  u8 r = setup.bRequest;
  u8 requestType = setup.bmRequestType;

  if(REQUEST_DEVICETOHOST_CLASS_INTERFACE == requestType)
  {
    if (CDC_GET_LINE_CODING == r)
    {
      error_printP(F("Get Line Coding"));

#if 1
      USB_SendControl(0,(void*)&_usbLineInfo, sizeof(_usbLineInfo)/*7*/);
#endif // 0

      return true;
    }
  }
  else if(REQUEST_HOSTTODEVICE_CLASS_INTERFACE == requestType)
  {
    if(CDC_SET_LINE_CODING == r)
    {
      error_printP_(F("CDC_SET_LINE_CODING"));

      // setup packet is followed by data?
      memcpy((void *)&_usbLineInfo, (char *)&(setup) + sizeof(Setup), sizeof(_usbLineInfo));

      error_printP_(F("  rate:"));
      error_printL_(_usbLineInfo.dwDTERate);
      error_printP_(F("  fmt:"));
      error_printL_(_usbLineInfo.bCharFormat);
      error_printP_(F("  par:"));
      error_printL_(_usbLineInfo.bParityType);
      error_printP_(F("  bit:"));
      error_printL(_usbLineInfo.bDataBits);

      USB_SendControl(0, NULL, 0); // send a ZLP

      _cdcLineState = CONTROL_LINE_STATE_DTR; // for now... assume "this"

      // now set up the ACM interrupt info in '_cdcSerialState' and send it back

      _cdcSerialState = SERIAL_STATE_TX_CARRIER_DSR; // to tell host "I have data" (always)

      return true;
    }
    else if(CDC_SET_CONTROL_LINE_STATE == r)
    {
      error_printP_(F("Set Control Line State: "));
      error_printL(setup.wValueL);

      _cdcLineState = setup.wValueL;

      // NOTE:  this next part is for the 'caterina' CDC bootloader, arduino/bootloaders/caterina/Caterina.c
      //        it has some "special" code in it, like using 0x0800 in RAM as an address for a 'key' (7777H)
      //        to indicate it was soft-booted.  XMEGA has better ways of handling this, like a CPU flag that
      //        indicates "I was soft-booted" as one example, and a 'WDT' timeout flag on top of that.

      // auto-reset into the bootloader is triggered when the port, already
      // open at 1200 bps, is closed.  this is the signal to start the watchdog
      // with a relatively long period so it can finish housekeeping tasks
      // like servicing endpoints before the sketch ends

      if (1200 == _usbLineInfo.dwDTERate)
      {
        // We check DTR state to determine if host port is open (bit 0 of _cdcLineState).
        if ((_cdcLineState & 0x01) == 0)
        {
// This section of code is support for the 'caterina' bootloader, which allows USB flashing (apparently)
//
//          *(uint16_t *)0x0800 = 0x7777; note that on XMEGA this is a VERY bad thing
//          wdt_enable(WDTO_120MS);
//
//          on the atmega, address 800H is the start of the final 256-byte page in RAM space for 2k RAM
//
//          atmega328(p) RAM goes from 0x100 through 0x8ff - see datasheet for atmega 328 [etc.] section 8.3
//          32U4 RAM goes through 0xaff - see datasheet for U4 processors, section 5.2
//          8/16/32U2 RAM goes through 4FFH so this won't even work - see datasheet for U2 processors, section 7.2
//          basically it's a 'hack' and needs to be re-evaluated

          // TODO:  would it be safe to enable interrupts, NOT return from this function,
          //        and simply wait until the appropriate time has elapsed?  Or, as is
          //        handled in the section below, this 'wait period' is canceled

          // TODO:  if I use a function that's part of the USB driver to trigger a soft boot, I can detect
          //        that a soft boot has taken place using the bits in the 'RESET' status register.  If all
          //        I have to do is detect this, it's not a problem, and I won't need "magic memory locations"

//          TODO:  timeout-based reboot
        }
        else
        {
          // Most OSs do some intermediate steps when configuring ports and DTR can
          // twiggle more than once before stabilizing.
          // To avoid spurious resets we set the watchdog to 250ms and eventually
          // cancel if DTR goes back high.

// This section of code is support for the 'caterina' bootloader, which allows USB flashing (apparently)
//
//          TODO:  reset whatever boot timeout I did
//          wdt_disable();
//          wdt_reset();
//          *(uint16_t *)0x0800 = 0x0; note that on XMEGA this is a VERY bad thing
        }
      }

      USB_SendControl(0, NULL, 0); // send a ZLP

      return true;
    }
  }

  // unrecognized request - report it

  error_printP_(F("CDC request: type="));
  error_printL_(requestType);
  error_printP_(F(" request="));
  error_printL(r);  
  return false;
}

// 'frame received' callback - notification that a 'Start Of Frame' took place

void CDC_FrameReceived(void)
{
bool bSend = false;

  // NOTE:  if I haven't configured the baud/bits yet, or the DTR bit is cleared,
  //        do NOT send anything nor muck with the flags.  Wait until the device
  //        is actually RUNNING, first.

  if(!_usbLineInfo.bDataBits || !(_cdcLineState & CONTROL_LINE_STATE_DTR))
  {
    return; // don't do anything if I haven't properly set up the data bits yet
  }

  if(USB_Available(CDC_RX) >= 64) // allow ~64 buffered bytes
  {
    if(_cdcSerialState & SERIAL_STATE_RX_CARRIER_DCD) // was on?
    {
      _cdcSerialState &= ~SERIAL_STATE_RX_CARRIER_DCD;

      bSend = true;
    }
  }
  else
  {
    if(!(_cdcSerialState & SERIAL_STATE_RX_CARRIER_DCD)) // was off?
    {
      _cdcSerialState |= SERIAL_STATE_RX_CARRIER_DCD;

      bSend = true;
    }
  }

//  if(USB_SendQLength(CDC_TX) > 0) // anything to send??
//  {
//    if(!(_cdcSerialState & SERIAL_STATE_TX_CARRIER_DSR))
//    {
//      _cdcSerialState |= SERIAL_STATE_TX_CARRIER_DSR; // to tell host "I have data"
//
//      bSend = true;
//    }
//  }
//  else
//  {
//    if(_cdcSerialState & SERIAL_STATE_TX_CARRIER_DSR)
//    {
//      _cdcSerialState &= ~SERIAL_STATE_TX_CARRIER_DSR; // to tell host "I have data"
//
//      bSend = true;
//    }
//  }

  if((bSend || wInterval >= 64) // will send every 64 'bus cycles' or when there's a change
     && !USB_SendQLength(CDC_ACM))
  {
    CDC_SendACM();

    wInterval = 0;
  }
  else if(wInterval < 64)
  {
    wInterval++;
  }
}

void CDC_SendACM(void)
{
  USB_Send(CDC_ACM, &_cdcSerialState, sizeof(_cdcSerialState), 1);
}

void Serial_::begin(unsigned long baud_count)
{
  peek_buffer = -1;
}

void Serial_::begin(unsigned long baud_count, byte config)
{
  peek_buffer = -1;
}

void Serial_::end(void)
{
}

int Serial_::available(void)
{
  if (peek_buffer >= 0)
  {
    return 1 + USB_Available(CDC_RX);
  }

  return USB_Available(CDC_RX);
}

int Serial_::peek(void)
{
  if (peek_buffer < 0)
  {
    if(USBDevice.configured())
    {
      peek_buffer = USB_Recv(CDC_RX);
    }
  }

  return peek_buffer;
}

int Serial_::read(void)
{
  if (peek_buffer >= 0)
  {
    int c = peek_buffer;
    peek_buffer = -1;
    return c;
  }

  if(USBDevice.configured())
  {
    return USB_Recv(CDC_RX);
  }

  return -1;
}

void Serial_::flush(void)
{
  if(USBDevice.configured())
  {
    USB_Flush(CDC_TX);
  }
}

size_t Serial_::write(uint8_t c)
{
  return write(&c, 1);
}

size_t Serial_::write(const uint8_t *buffer, size_t size)
{
  /* only try to send bytes if the high-level CDC connection itself
   is open (not just the pipe) - the OS should set _cdcLineState when the port
   is opened and clear _cdcLineState when the port is closed.
   bytes sent before the user opens the connection or after
   the connection is closed are lost - just like with a UART. */

  // NOTE:  if my outgoing buffer is too full, stop sending

  // TODO - ZE - check behavior on different OSes and test what happens if an
  // open connection isn't broken cleanly (cable is yanked out, host dies
  // or locks up, or host virtual serial port hangs)

  if(USBDevice.configured() && // make sure I'm running
//     !USB_IsStalled(CDC_TX) && // make sure I'm not stalled
     !USB_IsSendQFull(CDC_TX)) // make sure I'm not flooding the queue
  {  
    if(_cdcLineState & CONTROL_LINE_STATE_DTR) // make sure DTR is set
    {
      if(size > 128)
      {
        size = 128; // adjust size DOWN to limit output buffer size
      }

      int r = USB_Send(CDC_TX, buffer, size, 1);

      // TODO:  check for partial sends and retry??

      if(r > 0)
      {
        CDC_FrameReceived(); // inform the host of my data send/receive state

        return r;
      }
    }
  }

  // TODO:  block?

  setWriteError();
  return 0;
}

// This operator is a convenient way for a sketch to check whether the
// port has actually been configured and opened by the host (as opposed
// to just being connected to the host).  It can be used, for example, in
// setup() before printing to ensure that an application on the host is
// actually ready to receive and display the data.

Serial_::operator bool()
{
  bool result = false;
  if(USBDevice.configured()
     && (_cdcLineState & CONTROL_LINE_STATE_DTR)
     && !USB_IsSendQFull(CDC_TX)
//     && !USB_IsStalled(CDC_TX)
     )
  {
    result = true;
  }

// We add a short delay before returning to fix a bug observed by Federico
// where the port is configured (_cdcLineState != 0) but not quite opened.
//  delay(10);

  if(!result)
  {
    if(!USBDevice.configured())
    {
      error_printP(F("USB device not configured"));
    }
    else if(!(_cdcLineState & CONTROL_LINE_STATE_DTR))
    {
      error_printP(F("DTR is off"));
    }
    else if(USB_IsSendQFull(CDC_TX))
    {
      error_printP(F("Send Queue FULL"));
    }
//    else if(USB_IsStalled(CDC_TX))
//    {
//      error_printP(F("USB is stalled"));
//    }
  }

  return result;
}

Serial_ Serial;

#endif
#endif /* if defined(USBCON) */

