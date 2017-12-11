//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//     _   _  ____   ____    ____                                           //
//    | | | |/ ___| | __ )  / ___| ___   _ __  ___     ___  _ __   _ __     //
//    | | | |\___ \ |  _ \ | |    / _ \ | '__|/ _ \   / __|| '_ \ | '_ \    //
//    | |_| | ___) || |_) || |___| (_) || |  |  __/ _| (__ | |_) || |_) |   //
//     \___/ |____/ |____/  \____|\___/ |_|   \___|(_)\___|| .__/ | .__/    //
//                                                         |_|    |_|       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

/* Copyright (c) 2010, Peter Barrett
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


// **************************************************************************
// This software incorporates Arthur C. Clarke's 3rd law:
//  "Any sufficiently advanced technology is indistinguishable from magic."
//  (no chickens were harmed nor sacrificed in the completion of this work)
// **************************************************************************

// Updated for the XMegaForArduino project by Bob Frazier, S.F.T. Inc.

#define DEBUG_CODE /* debug output via 'error_print' etc. - must do this first */

#include "Platform.h"
#include "USBAPI.h"
#include "USBDesc.h"

#include "wiring_private.h"


#if defined(USBCON)

// CONDITIONALS FOR COMPILE-TIME OPTIONS

//#define DEBUG_MEM  /* debug memory and buffer manipulation */
//#define DEBUG_QUEUE /* debug queues */
//#define DEBUG_CONTROL /* control packet verbose debugging */


// 'LIKELY' and 'UNLIKELY' - 'if'/branch optimization
#define UNLIKELY(x) (__builtin_expect (!!(x), 0))
#define LIKELY(x) (__builtin_expect (!!(x), 1))


// This next block of code is to deal with gcc bug 34734 on compilers < 4.6
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define PROGMEM_ORIG PROGMEM
#else // PROGMEM workaround

// to avoid the bogus "initialized variables" warning
#ifdef PROGMEM
#undef PROGMEM
#endif // PROGMEM re-define

#define PROGMEM __attribute__((section(".progmem.usbcore")))
#define PROGMEM_ORIG __attribute__((__progmem__))

#endif // check for GNUC >= or < 4.6


// bugs in iox128a1u.h and iox64a1u.h
// the definition for USB_TRNCOMPL_vect and USB_TRNCOMPL_vect_num is wrong
// this can be corrected here.  bug reported 'upstream' for avr-libc 1.8.0 and 1.8.1
//   https://savannah.nongnu.org/bugs/index.php?44279
//
// (note this was causing me to get reboots, until I discovered the problem - very frustrating indeed)

#if defined (__AVR_ATxmega64A1U__) || defined (__AVR_ATxmega128A1U__)
#undef USB_TRNCOMPL_vect
#undef USB_TRNCOMPL_vect_num
#define USB_TRNCOMPL_vect_num  126
#define USB_TRNCOMPL_vect      _VECTOR(126)  /* Transaction complete interrupt */
#endif // __AVR_ATxmega64A1U__, __AVR_ATxmega128A1U__

// additional compatibilty bugs between older and newer versions of iox128a1u.h and iox64a1u.h
#ifdef USB_EP_BUFSIZE_gm
#define USB_EP_SIZE_64_gc USB_EP_BUFSIZE_64_gc  /* name change of definition from previous header */
#endif // USB_EP_BUFSIZE_gm

#ifndef CLK_USBEN_bm
#define CLK_USBEN_bm CLK_USBSEN_bm /* name change of definition from previous header */
#endif // CLK_USBEN_bm



// number of endpoints - to determine buffer array sizes
// see definition for _initEndpoints (below)
#ifdef CDC_ENABLED
#ifdef HID_ENABLED
#define INTERNAL_NUM_EP 5
#else // HID_ENABLED _not_ defined
#define INTERNAL_NUM_EP 4
#endif // HID_ENABLED
#elif defined(HID_ENABLED)
#define INTERNAL_NUM_EP 2
#else
#define INTERNAL_NUM_EP 1
#endif


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//    ____  _____  ____   _   _   ____  _____  _   _  ____   _____  ____    //
//   / ___||_   _||  _ \ | | | | / ___||_   _|| | | ||  _ \ | ____|/ ___|   //
//   \___ \  | |  | |_) || | | || |      | |  | | | || |_) ||  _|  \___ \   //
//    ___) | | |  |  _ < | |_| || |___   | |  | |_| ||  _ < | |___  ___) |  //
//   |____/  |_|  |_| \_\ \___/  \____|  |_|   \___/ |_| \_\|_____||____/   //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define INTERNAL_BUFFER_LENGTH 64

// TODO:  use separate chain of smaller buffers for control channel???

typedef struct __INTERNAL_BUFFER__
{
  struct __INTERNAL_BUFFER__ * volatile pNext;
  volatile uint8_t iIndex; // current pointer
  volatile uint8_t iLen;   // max pointer
  // NOTE:  if 'iLen' is zero, the buffer is being filled and 'iIndex' is the length
  //        when the buffer is released to send, iLen gets the length, iIndex is assigned
  //        to 0xff.  when sendING, iIndex is assigned to 0xfe.  On send complete, it's free'd

  uint8_t aBuf[INTERNAL_BUFFER_LENGTH];
} INTERNAL_BUFFER;



/////////////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
#ifdef DEBUG_CODE

// additional debug functions ALSO defined in 'USBAPI.h' - need 'weak' and local definitions
// that way the code still builds/links without the debug functions
extern "C"
{
  extern void error_print(const char *p1)   __attribute__((weak));
  extern void error_print_(const char *p1)  __attribute__((weak));
  extern void error_printH(unsigned long)   __attribute__((weak));
  extern void error_printH_(unsigned long)  __attribute__((weak));
  extern void error_printL(unsigned long)   __attribute__((weak));
  extern void error_printL_(unsigned long)  __attribute__((weak));
  extern void error_printP(const void * p1)  __attribute__((weak));
  extern void error_printP_(const void * p1) __attribute__((weak));

  extern void error_print(const char *p1)  { }
  extern void error_print_(const char *p1) { }
  extern void error_printH(unsigned long)  { }
  extern void error_printH_(unsigned long) { }
  extern void error_printL(unsigned long)  { }
  extern void error_printL_(unsigned long) { }
  extern void error_printP(const void * p1)  { }
  extern void error_printP_(const void * p1) { }
};


#ifndef TX_RX_LED_INIT
#define LED_SIGNAL0 (LED_BUILTIN-2) /* PQ3 */
#define LED_SIGNAL1 (LED_BUILTIN-3) /* PQ2 */
#define LED_SIGNAL2 (LED_BUILTIN-4) /* PQ1 */
#define LED_SIGNAL3 (LED_BUILTIN-5) /* PQ0 */

#define TX_RX_LED_INIT() { pinMode(LED_SIGNAL2,OUTPUT); pinMode(LED_SIGNAL3,OUTPUT); \
                           digitalWrite(LED_SIGNAL2,0); digitalWrite(LED_SIGNAL3,0); }
#define TXLED0() digitalWrite(LED_SIGNAL2,0)
#define TXLED1() digitalWrite(LED_SIGNAL2,1)
#define RXLED0() digitalWrite(LED_SIGNAL3,0)
#define RXLED1() digitalWrite(LED_SIGNAL3,1)
#endif // TX_RX_LED_INIT

#else

#define DumpHex(X,Y)
#define DumpBuffer(X)

#endif // DEBUG_CODE


// DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG
/////////////////////////////////////////////////////////////////////////////////////////////////////


#define EP_TYPE_CONTROL        0x00
#define EP_TYPE_BULK_IN        0x81
#define EP_TYPE_BULK_OUT      0x80
#define EP_TYPE_INTERRUPT_IN    0xC1
#define EP_TYPE_INTERRUPT_OUT    0xC0
#define EP_TYPE_ISOCHRONOUS_IN    0x41
#define EP_TYPE_ISOCHRONOUS_OUT    0x40



// NOTE:  auto ZLP is broken according to 128A1U errata
#define ZLP_BIT 0/*(((uint16_t)USB_EP_ZLP_bm)<<8)*/

//==================================================================
//==================================================================

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//         ____  ___   _   _  ____  _____   _     _   _  _____  ____        //
//        / ___|/ _ \ | \ | |/ ___||_   _| / \   | \ | ||_   _|/ ___|       //
//       | |   | | | ||  \| |\___ \  | |  / _ \  |  \| |  | |  \___ \       //
//       | |___| |_| || |\  | ___) | | | / ___ \ | |\  |  | |   ___) |      //
//        \____|\___/ |_| \_||____/  |_|/_/   \_\|_| \_|  |_|  |____/       //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


const u16 USB_STRING_LANGUAGE[2] PROGMEM = // not actually a 'string' but same format
{
  (3<<8) | (2+2), // high byte is '3', low byte is total length (in bytes)
  0x0409          // English
};

// TODO:  re-factor string returns into a utility function that builds the header
//        and assigns the length correctly using the array address and 'sizeof'
//        for now, the string starts with "\x03nn" where 'nn' is the total length
//        in bytes for the entire string (including the 'header' char)

#ifndef USB_PRODUCT_NAME
// if the pins_arduino.h file does NOT define a product name, define one HERE
#if USB_PID == 0x8036 && USB_VID == 0x2341
  #define USB_PRODUCT_NAME L"\x0322" "Arduino Leonardo"         /* 16 characters, total length 34 */
#elif USB_PID == 0x8037 && USB_VID == 0x2341
  #define USB_PRODUCT_NAME L"\x031c" "Arduino Micro"            /* 13 characters, total length 28 */
#elif USB_PID == 0x803C && USB_VID == 0x2341
  #define USB_PRODUCT_NAME L"\x0320" "Arduino Esplora"          /* 15 characters, total length 32 */
#elif USB_PID == 0x9208 && (USB_VID == 0x1b4f || USB_VID == 0x2341) // NOTE:  I don't know which VID is right
  #define USB_PRODUCT_NAME L"\x0316" "LilyPadUSB"               /* 10 characters, total length 22 */
#elif USB_PID == 0x0010 && USB_VID == 0x2341 // added for 'mega' clone (testing only)
  #define USB_PRODUCT_NAME L"\x0322" "Arduino Mega2560"         /* 16 characters, total length 34 */
#elif USB_VID==0x16c0 && USB_PID==0x05e1 /* CDC/ACM device using name-based device identification */
  #define USB_PRODUCT_NAME L"\x0330" "XMegaForArduino Project"  /* 23 chars, total length 48 */
#else
  #define USB_PRODUCT_NAME L"\x0330" "XMegaForArduino Project"  /* 23 characters, total length 48 */
#endif // various USB_VID/USB_PID combos
#endif // USB_PRODUCT_NAME

const wchar_t USB_STRING_PRODUCT_STR[] PROGMEM = USB_PRODUCT_NAME;
#define USB_STRING_PRODUCT ((const u16 *)&(USB_STRING_PRODUCT_STR[0]))


#ifndef USB_MANUFACTURER_NAME
// if the pins_arduino.h file does NOT define a manufacturer name, define one HERE
#if USB_VID == 0x2341
  #define USB_MANUFACTURER_NAME L"\x0318" "Arduino LLC"                       /* string length 11, total 24 */
  #warning using Arduino USB Vendor ID - do NOT ship product with this ID without permission!!!
#elif USB_VID == 0x1b4f
  #define USB_MANUFACTURER_NAME L"\x0314" "Sparkfun"                          /* string length 9, total 20 */
  #warning using SparkFun USB Vendor ID - do NOT ship product with this ID without permission!!!
#elif USB_VID == 0x1d50
  #define USB_MANUFACTURER_NAME L"\x0314" "Openmoko"                          /* string length 9, total 20 */
  /* Openmoko - see http://wiki.openmoko.org/wiki/USB_Product_IDs */
  #warning make sure you have obtained a proper product ID from Openmoko - see http://wiki.openmoko.org/wiki/USB_Product_IDs
#elif USB_VID==0x16c0 && USB_PID==0x05e1 /* CDC/ACM device using name-based device identification */
  #define USB_MANUFACTURER_NAME L"\x033a" "S.F.T. Inc. http://mrp3.com/"      /* string length 28, total length 58 */
#elif USB_VID == 0x16c0
  #define USB_MANUFACTURER_NAME L"\x0344" "Van Ooijen Technische Informatica" /* string length 34, total 70 */
  #warning Using the default vendor description for VID 16C0H - see https://raw.githubusercontent.com/arduino/ArduinoISP/master/usbdrv/USB-IDs-for-free.txt
#else
  #define USB_MANUFACTURER_NAME L"\x0312" "Unknown"                           /* string length 8, total 18 */
#endif // various USB_VID/USB_PID combos
#endif // USB_MANUFACTURER_NAME

const wchar_t USB_STRING_MANUFACTURER_STR[] PROGMEM = USB_MANUFACTURER_NAME;
#define USB_STRING_MANUFACTURER ((const u16 *)&(USB_STRING_MANUFACTURER_STR[0]))


// -------------------------------------------
// DEFAULT DEVICE DESCRIPTOR (device class 0)
// -------------------------------------------
// this instructs the USB host to use the information in the individual interface descriptors
// see http://www.usb.org/developers/defined_class/#BaseClass00h
const DeviceDescriptor USB_DeviceDescriptor PROGMEM =
  D_DEVICE(0x00,       // device class (0) - use interface descriptors
           0x00,       // device sub-class (0)
           0x00,       // device protocol (0)
           64,         // packet size (64)
           USB_VID,    // vendor ID for the USB device
           USB_PID,    // product ID for the USB device
           0x100,      // this indicates USB version 1.0
           USB_STRING_INDEX_MANUFACTURER, // string index for mfg
           USB_STRING_INDEX_PRODUCT,      // string index for product name
           USB_STRING_INDEX_SERIAL,       // string index for serial number (0 for 'none')
           1);                            // number of configurations (1)


// -----------------------------
// ALTERNATE DEVICE DESCRIPTOR
// -----------------------------
// this is derived from the latest Arduino core - note that it defins a 'miscellaneous' device
// and this descriptor is returned whenever the device is a 'composite'.  However, I think the
// logic by which this is used is just WRONG and the above descriptor is probably a better choice.
// For additional info, see http://www.usb.org/developers/defined_class/#BaseClassEFh
const DeviceDescriptor USB_DeviceDescriptorB PROGMEM =
  D_DEVICE(0xEF,       // device class (0xef, miscellaneous)
           0x02,       // device sub-class (2)
           0x01,       // device protocol (protocol 1, subclass 2 - Interface Association Descriptor)
           64,         // packet size (64)
           USB_VID,    // vendor ID for the USB device
           USB_PID,    // product ID for the USB device
           0x100,      // this indicates USB version 1.0
           USB_STRING_INDEX_MANUFACTURER, // string index for mfg
           USB_STRING_INDEX_PRODUCT,      // string index for product name
           USB_STRING_INDEX_SERIAL,       // string index for serial number (0 for 'none')
           1);                            // number of configurations (1)


//==================================================================
//==================================================================


// TODO:  make this dynamically generated instead, using supported devices
//        this may require 'malloc' for aSendQ and aRecvQ, however...
const u8 _initEndpoints[INTERNAL_NUM_EP] PROGMEM =
{
  EP_TYPE_CONTROL,         // EP_TYPE_CONTROL   control endpoint [always endpoint 0]

#ifdef CDC_ENABLED
  EP_TYPE_INTERRUPT_IN,    // CDC_ENDPOINT_ACM  write, interrupt (endpoint 1) max 8 bytes
  EP_TYPE_BULK_OUT, // EP_TYPE_ISOCHRONOUS_OUT, // EP_TYPE_BULK_OUT,        // CDC_ENDPOINT_OUT  bulk read (endpoint 2) max 64 bytes
  EP_TYPE_BULK_IN, //EP_TYPE_ISOCHRONOUS_IN,  // EP_TYPE_BULK_IN,         // CDC_ENDPOINT_IN   bulk write (endpoint 3) max 64 bytes

#ifdef HID_ENABLED
#error THIS kind of composite device NOT supported at this time
#endif

#endif

#ifdef HID_ENABLED
  EP_TYPE_INTERRUPT_IN     // HID_ENDPOINT_INT  write, interrupt (endpoint 1)
#endif
};


// XMegaForArduino still uses older defs for these (TODO - update to something better)
#define EP_SINGLE_64 0x32  // EP0 and single-buffer int-driven endpoints
#define EP_DOUBLE_64 0x36  // endpoints that support double-buffer out




//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//               ____  _      ___   ____     _     _      ____              //
//              / ___|| |    / _ \ | __ )   / \   | |    / ___|             //
//             | |  _ | |   | | | ||  _ \  / _ \  | |    \___ \             //
//             | |_| || |___| |_| || |_) |/ ___ \ | |___  ___) |            //
//              \____||_____|\___/ |____//_/   \_\|_____||____/             //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// A linked list of pre-allocated buffers, twice the # of endpoints plus 4 .
// Most endpoints ONLY read OR write, and control EP does both. This lets me have
// 2 read or 2 write buffers per endpoint, perfect for 'ping pong' buffering
// (even if I implement that manually), or one each for bi-directional comms.
// The additional 4 buffers allows me to construct ~256 byte data packets using
// 4 64-byte buffers, and not have to wait for anything being sent.  For buffers
// with data larger than 256 bytes, I guess you'll have to get a bigger CPU anyway...

INTERNAL_BUFFER *pFree = NULL;

#if INTERNAL_NUM_EP <= 2
INTERNAL_BUFFER aBufList[8]; // the minimum number is 8 buffers, determined by experimentation
#else // INTERNAL_NUM_EP > 2
INTERNAL_BUFFER aBufList[INTERNAL_NUM_EP * 2 + 4];  // twice the # of endpoints plus 4
#endif // INTERNAL_NUM_EP <= 2

#define NUM_INTERNAL_BUFFERS (sizeof(aBufList)/sizeof(aBufList[0])) /* universally accurate */


INTERNAL_BUFFER *aSendQ[INTERNAL_NUM_EP]; // send and receive queue head pointers, one per in/out per endpoint
INTERNAL_BUFFER *aRecvQ[INTERNAL_NUM_EP];

#define INTERNAL_BUFFER_FILLING(X) ((X)->iLen == 0 && (X)->iIndex < 0xfe) /* buffer filling (or empty) */
#define INTERNAL_BUFFER_SEND_READY(X) ((X)->iIndex == 0xff)               /* ready to send */
#define INTERNAL_BUFFER_MARK_SEND_READY(X) {(X)->iLen = (X)->iIndex; (X)->iIndex = 0xff;}
#define INTERNAL_BUFFER_SENDING(X) ((X)->iIndex == 0xfe)                  /* is 'sending' */
#define INTERNAL_BUFFER_MARK_SENDING(X) {(X)->iIndex = 0xfe;}             /* mark 'sending' */
#define INTERNAL_BUFFER_RECV_EMPTY(X) (!(X)->iIndex && !(X)->iLen)        /* empty receive buffer */
#define INTERNAL_BUFFER_RECV_READY(X) ((X)->iLen > 0)                     /* received data ready */



/** Pulse generation counters to keep track of the number of milliseconds remaining for each pulse type */
#ifdef TX_RX_LED_INIT /* only when these are defined */
#define TX_RX_LED_PULSE_MS 100
volatile u8 TxLEDPulse; /**< Milliseconds remaining for data Tx LED pulse */
volatile u8 RxLEDPulse; /**< Milliseconds remaining for data Rx LED pulse */
#endif // TX_RX_LED_INIT

static XMegaEPDataStruct epData; // the data pointer for the hardware's memory registers

//// for debugging only - remove later
//static uint8_t led_toggle;

static uint8_t bUSBAddress = 0; // when not zero, and a packet has been sent on EP 0, set the address to THIS
static uint8_t _usbConfiguration = 0; // assigned when I get a 'SET CONFIGURATION' control packet

static uint16_t wProcessingFlag = 0;  // 'processing' flag
// the endpoint's bit in 'wProcessingFlag' will be set to '1' whenever an incoming
// packet is being processed.  This prevents re-entrant processing of OTHER incoming
// packets while this is going on, so that incoming packets are serialized.
static uint16_t wMultipacketOutFlag = 0; // 'multipacket out I/O' flag
// the endpoint's bit in 'wMultipacketFlag' will be set whenever the 'IN' endpoint is
// being used to receive data (normally it's just 'OUT' that receives).  This can happen
// with bulk, interrupt, or control endpoints.  yeah, it changes things quite a bit.
static uint16_t wEndpointToggle = 0; // controls use of 'toggle' bit when sending a packet




/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//   _                        _   ____               _          _                              //
//  | |     ___    ___  __ _ | | |  _ \  _ __  ___  | |_  ___  | |_  _   _  _ __    ___  ___   //
//  | |    / _ \  / __|/ _` || | | |_) || '__|/ _ \ | __|/ _ \ | __|| | | || '_ \  / _ \/ __|  //
//  | |___| (_) || (__| (_| || | |  __/ | |  | (_) || |_| (_) || |_ | |_| || |_) ||  __/\__ \  //
//  |_____|\___/  \___|\__,_||_| |_|    |_|   \___/  \__|\___/  \__| \__, || .__/  \___||___/  //
//                                                                   |___/ |_|                 //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////


static void init_buffers_and_endpoints(void);
static void internal_do_control_request(INTERNAL_BUFFER *pBuf, bool bIsSetup);
static void consolidate_packets(INTERNAL_BUFFER *pHead);

static void InitEP(u8 index, u8 type, u8 size);

static bool SendDescriptor(Setup& rSetup);

static uint16_t buffer_data_pointer(INTERNAL_BUFFER *pBuf);
static INTERNAL_BUFFER * inverse_buffer_data_pointer(uint16_t dataptr);

static void internal_flush(int index);
static bool internal_send0(int index);
static bool internal_send(int index, const void *pData, uint16_t cbData, uint8_t bSendNow);
static int internal_receive(int index, void *pData, uint16_t nMax); // only for OUT endpoints
static void configure_buffers(void);

static bool ClassInterfaceRequest(Setup& rSetup);


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//              ____   _   _  _____  _____  _____  ____   ____              //
//             | __ ) | | | ||  ___||  ___|| ____||  _ \ / ___|             //
//             |  _ \ | | | || |_   | |_   |  _|  | |_) |\___ \             //
//             | |_) || |_| ||  _|  |  _|  | |___ |  _ <  ___) |            //
//             |____/  \___/ |_|    |_|    |_____||_| \_\|____/             //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

static void init_buffers_and_endpoints(void) __attribute__((noinline));
static INTERNAL_BUFFER *end_of_chain(INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static void configure_buffers(void) __attribute__((noinline));
static INTERNAL_BUFFER * next_buffer(void) __attribute__((noinline));
static void free_buffer(INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static void free_queue(INTERNAL_BUFFER **pQ) __attribute__((noinline));
static uint8_t not_in_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static void add_to_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static void remove_from_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static void remove_from_queue_and_free(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static uint16_t buffer_data_pointer(INTERNAL_BUFFER *pBuf) __attribute__((noinline));
static INTERNAL_BUFFER * inverse_buffer_data_pointer(uint16_t dataptr) __attribute__((noinline));

static void init_buffers_and_endpoints(void)
{
int index;
uint8_t oldSREG;

#ifdef DEBUG_MEM
  error_printP(F("init_buffers_and_endpoints"));
#endif // DEBUG_MEM

  oldSREG = SREG;
  cli(); // to avoid spurious interrupts (just in case)

  memset(&epData, 0, sizeof(epData));
  for(index=0; index <= MAXEP; index++)
  {
    epData.endpoint[index].out.status = USB_EP_BUSNACK0_bm; // disables I/O
    epData.endpoint[index].in.status = USB_EP_BUSNACK0_bm;  // necessary for 128A1U rev 'K' etc. (see errata)

    epData.endpoint[index].out.ctrl = USB_EP_TYPE_DISABLE_gc; // to disable it (endpoint 'type 0' disables)
    epData.endpoint[index].in.ctrl = USB_EP_TYPE_DISABLE_gc; // initially (disable)
  }

  configure_buffers(); // TODO:  move code here instead of function call?

  // set up endpoint 0 (always), now that I'm done doing that other stuff

  InitEP(0,EP_TYPE_CONTROL,EP_SINGLE_64);  // init ep0 and zero out the others

  wProcessingFlag = 0;
  wMultipacketOutFlag = 0;
  wEndpointToggle = 0;

  bUSBAddress = 0;

#ifdef TX_RX_LED_INIT
  TxLEDPulse = RxLEDPulse = 0;
  TXLED0();
  RXLED0();
#endif // TX_RX_LED_INIT

#ifdef CDC_ENABLED
  CDC_Reset();
#endif // CDC_ENABLED

#ifdef HID_ENABLED
  HID_Reset();
#endif // HID_ENABLED

  SREG = oldSREG;
}


static INTERNAL_BUFFER *end_of_chain(INTERNAL_BUFFER *pBuf)
{
uint8_t oldSREG;
register INTERNAL_BUFFER *pRval;


  if(!pBuf)
  {
    return NULL;
  }

  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pRval = pBuf;

  if(pRval < &(aBufList[0]) || pRval >= &(aBufList[NUM_INTERNAL_BUFFERS]))
  {
    error_printP_(F("end_of_chain:  pBuf out of bounds "));
    error_printH((unsigned long)pBuf);

    SREG = oldSREG;
    return NULL;
  }

  while(pRval->pNext)
  {
    pRval = pRval->pNext;

    if(pRval < &(aBufList[0]) || pRval >= &(aBufList[NUM_INTERNAL_BUFFERS]))
    {
      error_printP_(F("end_of_chain:  pRval out of bounds "));
      error_printH((unsigned long)pRval);

      SREG = oldSREG;
      return NULL;
    }
  }

  SREG = oldSREG;

  if(pRval >= &(aBufList[0]) && pRval < &(aBufList[NUM_INTERNAL_BUFFERS]))
  {
    return pRval;
  }

  error_printP(F("WARN:  end_of_chain - corrupt buffer chain, returns NULL"));

  return NULL;
}


// simple pre-allocated buffer management
static void configure_buffers(void)
{
register uint8_t i1;

#ifdef DEBUG_MEM
  error_printP(F("configure_buffers"));
#endif // DEBUG_MEM

  memset(aBufList, 0, sizeof(aBufList));

  for(i1=1; i1 < NUM_INTERNAL_BUFFERS; i1++)
  {
    aBufList[i1 - 1].pNext = &(aBufList[i1]);
  }

  // tail gets NULL for 'pNext'
  aBufList[NUM_INTERNAL_BUFFERS - 1].pNext = NULL;

  // head of free list is first entry
  pFree = &(aBufList[0]);

  // send and receive queues
  for(i1=0; i1 < INTERNAL_NUM_EP; i1++)
  {
    aSendQ[i1] = NULL;
    aRecvQ[i1] = NULL;
  }
}


// this function peels a pointer from the 'free' list and returns it.
// caller must call 'free_buffer' with returned pointer when done with it
// and *NOT* re-use it after calling 'free_buffer'.
static INTERNAL_BUFFER * next_buffer(void)
{
INTERNAL_BUFFER *pRval;
uint8_t oldSREG;


#ifdef DEBUG_MEM
  error_printP_(F("next_buffer "));
#endif // DEBUG_MEM

  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pRval = pFree;

  if(pRval) // non-NULL return
  {
    pFree = pRval->pNext;
    pRval->pNext = NULL;
  }

  if(!pRval)
  {
#ifdef DEBUG_MEM
    error_printP_(F("!!pRval NULL!!"));
#else // DEBUG_MEM
    error_printP(F("next_buffer !!pRval NULL!!"));
#endif // DEBUG_MEM
  }
  else if(pRval < &(aBufList[0]) || pRval >= &(aBufList[NUM_INTERNAL_BUFFERS]))
  {
#ifdef DEBUG_MEM
    error_printP_(F("!!pRval out of bounds!!"));
    error_printH_((uint16_t)pRval);
#else // DEBUG_MEM
    error_printP_(F("next_buffer !!pRval out of bounds!!"));
    error_printH((uint16_t)pRval);
#endif // DEBUG_MEM

    pRval = NULL; // prevent propagating errors
  }
  else
  {
#ifdef DEBUG_MEM
    error_printH_((uint16_t)pRval);
#endif // DEBUG_MEM
  }

#ifdef DEBUG_MEM // extra debug

  uint8_t nBuf;
  INTERNAL_BUFFER *pR = pFree;

  for(nBuf=0; pR; nBuf++, pR = pR->pNext)
    ;

  error_printP_(F(" ("));
  error_printL_(nBuf);
  error_printP(F(" free)"));

#endif // DEBUG_MEM

  SREG = oldSREG;

  return pRval; // now it belongs to the caller
}


static void free_buffer(INTERNAL_BUFFER *pBuf)
{
#ifdef DEBUG_MEM
  error_printP_(F("free_buffer "));
#endif // DEBUG_MEM

  // simple sanity test, must be in range ('address valid' could be tested as well)
  if(pBuf >= &(aBufList[0]) && pBuf < &(aBufList[NUM_INTERNAL_BUFFERS]))
  {
    uint8_t oldSREG = SREG; // save int flag
    cli(); // locking resource

    pBuf->pNext = pFree;
    pFree = pBuf;

    SREG = oldSREG;

#ifdef DEBUG_MEM
    error_printH_((uint16_t)pBuf);
#endif // DEBUG_MEM
  }
  else
  {
#ifdef DEBUG_MEM
    error_printP_(F("- address "));
    error_printH_((uint16_t)pBuf);
    error_printP_(F(" not valid"));
#endif // DEBUG_MEM
  }

#ifdef DEBUG_MEM // extra debug

  uint8_t nBuf;
  INTERNAL_BUFFER *pR = pFree;

  for(nBuf=0; pR; nBuf++, pR = pR->pNext)
    ;

  error_printP_(F(" ("));
  error_printL_(nBuf);
  error_printP(F(" free)"));

#endif // DEBUG_MEM

}

// Free an entire queue, assigning NULL to '*pQ' when done
static void free_queue(INTERNAL_BUFFER **pQ)
{
INTERNAL_BUFFER *pE;
uint8_t oldSREG;

  if(!pQ)
  {
    return;
  }

#ifdef DEBUG_MEM // extra debug
  error_printP(F("free_queue "));

  if(pQ >= &(aSendQ[0]) && pQ < &(aSendQ[sizeof(aSendQ)/sizeof(aSendQ[0])]))
  {
    error_printP_(F("aSendQ["));
    error_printL_(pQ - &(aSendQ[0]));
    error_printP(F("]"));
  }
  else if(pQ >= &(aRecvQ[0]) && pQ < &(aRecvQ[sizeof(aRecvQ)/sizeof(aRecvQ[0])]))
  {
    error_printP_(F("aRecvQ["));
    error_printL_(pQ - &(aRecvQ[0]));
    error_printP(F("]"));
  }
  else
  {
    error_printP(F("Q???Q"));
  }
#endif // DEBUG_MEM

  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pE = *pQ;  // NOTE:  if NULL, function will do nothing

  *pQ = NULL; // detach entire queue at once

  while(pE)
  {
    register INTERNAL_BUFFER *pE2 = pE->pNext;

    pE->pNext = NULL;

    free_buffer(pE);

    pE = pE2; // advances to next entry (NULL when done)
  }

  SREG = oldSREG;
}

static uint8_t not_in_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf)
{
INTERNAL_BUFFER *pE;
uint8_t oldSREG;

  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pE = *pQ;

  while(pE)
  {
    if(pE == pBuf)
    {
      SREG = oldSREG;
      return 0;
    }

    pE = pE->pNext;
  }

  SREG = oldSREG;

  return 1; // buffer not in queue
}


static void add_to_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf)
{
INTERNAL_BUFFER *pE;
uint8_t oldSREG;
#ifdef DEBUG_MEM // extra debug
uint8_t nQ;
#endif // DEBUG_MEM // extra debug


  if(!pQ || !pBuf)
  {
    return;
  }

#ifdef DEBUG_MEM // extra debug
  error_printP_(F("add_to_queue "));

  if(pQ >= &(aSendQ[0]) && pQ < &(aSendQ[sizeof(aSendQ)/sizeof(aSendQ[0])]))
  {
    error_printP_(F("aSendQ["));
    error_printL_(pQ - &(aSendQ[0]));
    error_printP_(F("]"));
  }
  else if(pQ >= &(aRecvQ[0]) && pQ < &(aRecvQ[sizeof(aRecvQ)/sizeof(aRecvQ[0])]))
  {
    error_printP_(F("aRecvQ["));
    error_printL_(pQ - &(aRecvQ[0]));
    error_printP_(F("]"));
  }
  else
  {
    error_printP_(F("Q???Q"));
  }

  error_printP_(F(" length="));
#endif // DEBUG_MEM


  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pE = *pQ;

  pBuf->pNext = NULL; // make sure

  if(!pE)
  {
    *pQ = pBuf;
  }
  else
  {
    // walk to the end
    while(pE->pNext)
    {
      pE = pE->pNext;
    }

    // attach
    pE->pNext = pBuf;
  }

#ifdef DEBUG_MEM // extra debug
  // count them (for debugging)
  pE = *pQ;
  nQ = 0;

  while(pE) // just count them
  {
    nQ++;
    pE = pE->pNext; // walk the chain
  }

  error_printL(nQ);
#endif // DEBUG_MEM

  SREG = oldSREG;
}


// this utility removes the buffer, but does not free it
static void remove_from_queue(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf)
{
INTERNAL_BUFFER *pE;
uint8_t oldSREG;
#ifdef DEBUG_MEM // extra debug
uint8_t nQ;
#endif // DEBUG_MEM // extra debug


  if(!pQ || !pBuf)
  {
    return;
  }

#ifdef DEBUG_MEM // extra debug
  error_printP_(F("remove_from_queue "));

  if(pQ >= &(aSendQ[0]) && pQ < &(aSendQ[sizeof(aSendQ)/sizeof(aSendQ[0])]))
  {
    error_printP_(F("aSendQ["));
    error_printL_(pQ - &(aSendQ[0]));
    error_printP_(F("]"));
  }
  else if(pQ >= &(aRecvQ[0]) && pQ < &(aRecvQ[sizeof(aRecvQ)/sizeof(aRecvQ[0])]))
  {
    error_printP_(F("aRecvQ["));
    error_printL_(pQ - &(aRecvQ[0]));
    error_printP_(F("]"));
  }
  else
  {
    error_printP_(F("Q???Q"));
  }
#endif // DEBUG_MEM


  oldSREG = SREG; // save int flag
  cli(); // locking resource

  pE = *pQ;

  if(pE) // if not NULL [allow for problems, delete buffer anyway]
  {
    if(LIKELY(pBuf == pE))
    {
#ifdef DEBUG_MEM // extra debug
      error_printP_(F(" (head) "));
#endif // DEBUG_MEM

      *pQ = pBuf->pNext; // remove the head of the queue (typical)
    }
    else
    {
      while(pE && pE->pNext != pBuf)
      {
        pE = pE->pNext; // walk the chain
      }

      if(pE && pE->pNext == pBuf) // found
      {
#ifdef DEBUG_MEM // extra debug
        error_printP_(F(" (mid) "));
#endif // DEBUG_MEM

        pE->pNext = pBuf->pNext; // remove 'pBuf' from the chain
      }
      else
      {
#ifdef DEBUG_MEM // extra debug
        error_printP_(F("  NOT removing "));
        error_printH_((uint16_t)pBuf);
        error_printP_(F(" --> "));
        error_printH_((uint16_t)pBuf->pNext);
#endif // DEBUG_MEM
      }
    }

  }

  pBuf->pNext = NULL; // always

#ifdef DEBUG_MEM // extra debug
  // count them (for debugging)
  pE = *pQ;
  nQ = 0;

  while(pE) // just count them
  {
    nQ++;
    pE = pE->pNext; // walk the chain
  }

  error_printP_(F(" length="));
  error_printL(nQ);
#endif // DEBUG_MEM

  SREG = oldSREG;
}


static void remove_from_queue_and_free(INTERNAL_BUFFER **pQ, INTERNAL_BUFFER *pBuf)
{
  if(pBuf)
  {
    remove_from_queue(pQ, pBuf);

    free_buffer(pBuf);  // and NOW 'pBuf' is back in the 'free' pool
  }
}


static uint16_t buffer_data_pointer(INTERNAL_BUFFER *pBuf)
{
  if(!pBuf)
  {
    return 0;
  }

  return (uint16_t)&(pBuf->aBuf[0]); // assign 'dataptr' element in USB Endpoint descriptor (sect 20.15 in 'AU' manual)
}


static INTERNAL_BUFFER * inverse_buffer_data_pointer(uint16_t dataptr)
{
  if(!dataptr)
  {
    return NULL;
  }

  dataptr -= (uint16_t)&(((INTERNAL_BUFFER *)0)->aBuf[0]);

  return (INTERNAL_BUFFER *)dataptr;
}





#ifdef DEBUG_CODE

static char hex_digit(unsigned char b1)
{
  if(b1 < 10)
  {
    return b1 + '0';
  }
  else if(b1 < 16)
  {
    return (b1 - 10) + 'A';
  }

  return '?';
}

void DumpHex(void *pBuf, uint8_t nBytes)
{
char tbuf[60];
uint8_t *pD;
char *pOut, nCol, c1;
short iCtr;

  error_printP_(F("Dump of data  nBytes="));
  error_printL(nBytes);
  error_printP(F("   --------------------------------------"));

  pOut = tbuf;
  *pOut = 0; // must do this

  for(iCtr=0, nCol=0, pD = (uint8_t *)pBuf; iCtr < (int)nBytes; iCtr++, pD++)
  {
    if(!nCol)
    {
      pOut = tbuf;

      if(iCtr > 0)
      {
        error_print(tbuf);
        tbuf[0] = 0; // always do this
      }

      *(pOut++) = hex_digit((iCtr >> 4) & 0xf);
      *(pOut++) = hex_digit(iCtr & 0xf);
      *(pOut++) = ':';
    }
    else
    {
      *(pOut++) = ' ';
    }

    c1 = *pD;

    *(pOut++) = hex_digit((c1 >> 4) & 0xf);
    *(pOut++) = hex_digit(c1 & 0xf);
    *pOut = 0; // always do this

    if(nCol < 15)
    {
      nCol++;
    }
    else
    {
      nCol = 0; // starts new line
    }
  }

  if(tbuf[0])
  {
    error_print(tbuf);
  }
}

void DumpBuffer(INTERNAL_BUFFER *pBuf)
{
char tbuf[60];
uint8_t *pD;
char *pOut, nCol, c1;
short iCtr;

  error_printP_(F("Dump of buffer "));
  error_printH_((uint16_t)pBuf);
  error_printP_(F("  iIndex="));
  error_printL_(pBuf->iIndex);
  error_printP_(F("  iLen="));
  error_printL_(pBuf->iLen);
  error_printP_(F("  pNext="));
  error_printH((uint16_t)pBuf->pNext);
  error_printP(F("   --------------------------------------"));

  pOut = tbuf;
  *pOut = 0;

  for(iCtr=0, nCol=0, pD = pBuf->aBuf; pD < &(pBuf->aBuf[sizeof(pBuf->aBuf)]) && iCtr < pBuf->iLen; iCtr++, pD++)
  {
    if(!nCol)
    {
      pOut = tbuf;

      if(iCtr > 0)
      {
        error_print(tbuf);
        tbuf[0] = 0; // always do this
      }

      *(pOut++) = hex_digit((iCtr >> 4) & 0xf);
      *(pOut++) = hex_digit(iCtr & 0xf);
      *(pOut++) = ':';
    }
    else
    {
      *(pOut++) = ' ';
    }

    c1 = *pD;

    *(pOut++) = hex_digit((c1 >> 4) & 0xf);
    *(pOut++) = hex_digit(c1 & 0xf);
    *pOut = 0; // always do this

    if(nCol < 15)
    {
      nCol++;
    }
    else
    {
      nCol = 0; // starts new line
    }
  }

  if(tbuf[0])
  {
    error_print(tbuf);
  }


}

#endif // DEBUG_CODE



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//       ___                              __  __                    _       //
//      / _ \  _   _   ___  _   _   ___  |  \/  |  __ _  _ __ ___  | |_     //
//     | | | || | | | / _ \| | | | / _ \ | |\/| | / _` || '_ ` _ \ | __|    //
//     | |_| || |_| ||  __/| |_| ||  __/ | |  | || (_| || | | | | || |_     //
//      \__\_\ \__,_| \___| \__,_| \___| |_|  |_| \__, ||_| |_| |_| \__|    //
//                                                |___/                     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


static void set_up_EP_for_receive(int index)
{
INTERNAL_BUFFER *pBuf;

  // assumed interrupts are OFF, setting up single endpoint to receive data
  // with a fresh buffer added to the queue.  existing buffers in recv queue are left 'as-is'.


  // queue up the next one if there is one
  pBuf = aRecvQ[index];

  while(pBuf)
  {
    if(INTERNAL_BUFFER_RECV_EMPTY(pBuf)) // find empty buffer already in queue
    {
      break;
    }

    pBuf = pBuf->pNext;
  }

  if(!pBuf) // receive queue has ONLY non-empty buffers (or none at all) - allocate a new one
  {
    pBuf = next_buffer();

    if(pBuf)
    {
      pBuf->iIndex = pBuf->iLen = 0; // make sure buffer is 'empty'
      add_to_queue(&(aRecvQ[index]), pBuf); // this adds the new buffer to the chain, leaving the others 'as-is'
    }
  }

  epData.endpoint[index].out.status |= USB_EP_BUSNACK0_bm     // make sure these are on, first 
                                     | USB_EP_UNF_bm | USB_EP_STALL_bm;    // writing 1 is supposed to clear them
  epData.endpoint[index].out.status &= ~(USB_EP_UNF_bm | USB_EP_STALL_bm); // but I'll do THIS, too

  epData.endpoint[index].out.cnt = 0;
  epData.endpoint[index].out.auxdata = 0;

  if(pBuf)
  {
    epData.endpoint[index].out.dataptr = buffer_data_pointer(pBuf); //(uint16_t)pBuf;

    // I'm going to turn all of the 'bad bits' off...
    epData.endpoint[index].out.status |= USB_EP_STALL_bm; // this turns off the bit by writing a '1'

    epData.endpoint[index].out.status &= USB_EP_TOGGLE_bm; // this allows receiving data (the old toggle bit remains)
  }
  else
  {
    epData.endpoint[index].out.dataptr = 0; // no buffer (stalled)
  }

  epData.endpoint[index].out.ctrl &= ~_BV(2); // in case it was on, turn off the stall
}


static void DispatchIncomingPacket(uint8_t index, INTERNAL_BUFFER *pE, bool bIsSetup)
{
uint16_t wProcessingMask = 1 << index;


  wProcessingFlag |= wProcessingMask; // indicate I'm processing, and avoid recursion

  if(index == 0 /* && bOldStatus & USB_EP_SETUP_bm */)
  {
    // consolidate all of the packets pointed to by 'pRecvQ[0]'

    consolidate_packets(aRecvQ[0]);
    pE = aRecvQ[0]; // grab packet from new head of queue (should be ALL of them actually) [for now assume only 1, later check]

    remove_from_queue(&(aRecvQ[index]), pE); // removes it from the queue (does not delete)
    // NOTE:  *ONLY* the control endpoint does this...

    if(pE->iLen)
    {
      internal_do_control_request(pE, bIsSetup); // always for endpoint 0
    }

    free_buffer(pE); // free up buffer [I am done with it]

    internal_flush(0); // flush the CONTROL endpoint's output buffers, now that I've done whatever operation this is
  }
  else
  {
    // TODO:  handle BULK and INTERRUPT endpoints correctly

    if(pE->iLen)
    {
      // NOTE:  this needs to remove the buffer from the queue to avoid
      //        any memory leaks.  If not, it could fill up...

      if(// internal_get_endpoint_type(index) != EP_TYPE_CONTROL &&
         aRecvQ[index] != pE)
      {
        consolidate_packets(aRecvQ[index]);
      }
      else
      {
        pE->iIndex = 0;  // always, to indicate where I am in the buffer [at the beginning]
      }

      // NOTE:  buffer remains in queue or is deleted by callback
    }
    else // zero-length packet
    {
      error_printP_(F("Received zero-length packet EP="));
      error_printL(index);

      // regardless, this packet must now be disposed of

      remove_from_queue(&(aRecvQ[index]), pE); // removes it from the queue (does not delete)
      free_buffer(pE); // remove buffer (ZLP ignored for now)
    }
  }

  wProcessingFlag &= ~wProcessingMask; // indicate I'm no longer processing, and allow callbacks again
}

static void check_recv_queue(void)
{
int index;
INTERNAL_BUFFER *pE;//, *pEtmp;
uint8_t oldSREG;


  oldSREG = SREG;
  cli(); // this must be done with interrupts blocked

  for(index=MAXEP; index >= 0; index--)
  {
    uint16_t wProcessingMask = 1 << index;

    // skip disabled endpoints
    if((epData.endpoint[index].out.ctrl & USB_EP_TYPE_gm) == USB_EP_TYPE_DISABLE_gc)
    {
      continue; // endpoint disabled (skip it)
    }

    // skip endpoints that are currently processing something
    if(wProcessingFlag & wProcessingMask)
    {
      continue; // skip something that's already processing, in case of recursion
    }

#ifdef ENABLE_PINGPPONG
    // a special section for certain *kinds* of endpoints
    // If I must receive data from the 'IN' endpoint as well as the 'OUT', such as
    // for a BULK or INTERRUPT endpoint, I check for that here.
    // NOTE:  a CONTROL endpoint cannot do 'ping pong' mode.  See AU manual section 20.6

    if(wMultipacketOutFlag & wProcessingMask) // I'm currently using 'IN' to receive for this endpoint
    {
      pE = inverse_buffer_data_pointer(epData.endpoint[index].in.dataptr);

      if(!pE) // an internal error - clear the bit and process the packet in the queue
      {
        wMultipacketOutFlag &= ~wProcessingMask; // I'm no longer using 'IN' to receive for this endpoint

        if(index || aSendQ[0]) // NOT control, or send queue not empty
        {
          continue;  // receive is stalled for the moment so skip this next part
        }
      }
      else if((epData.endpoint[index].in.status & USB_EP_TRNCOMPL0_bm) // note:  'SETUP' is TRNCOMPL1, as needed
              || (epData.endpoint[index].in.status & USB_EP_SETUP_bm)) // TODO:  is this right??
      {
        register uint8_t bOldStatus = epData.endpoint[index].in.status;

        // OK so what I did was to attach THIS packet to the receive queue, THEN assign its pointer
        // to the IN endpoint, THEN wait for data to show up.  Now that data has shown up, I need to
        // consolidate it, and then process it "as it would have been".

        epData.endpoint[index].in.status = USB_EP_BUSNACK0_bm | // mark it 'do not send/receive'
                                           (bOldStatus & ~(USB_EP_SETUP_bm | USB_EP_TRNCOMPL0_bm)); // turn these 2 bits off

        epData.endpoint[0].in.dataptr = 0; // zero out data pointer so I can send again
        epData.endpoint[0].in.cnt = 0;

        wMultipacketOutFlag &= ~wProcessingMask; // I'm no longer using 'IN' to receive for this endpoint

        DispatchIncomingPacket(index, pE, 0); // not a SETUP because not index zero

        if(index || !aSendQ[0]) // remember, control endpoints don't receive until done sending
        {
#ifdef DEBUG_QUEUE
          error_printP(F("check_recv_queue -set up for receive (b)"));
#endif // DEBUG_QUEUE

          set_up_EP_for_receive(index); // set up to receive
        }

        continue;
      }
      else
      {
        continue; // waiting for receive to complete
      }
    }
#endif // ENABLE_PINGPONG

    pE = inverse_buffer_data_pointer(epData.endpoint[index].out.dataptr);

    // check for an endpoint that has 'BUSNACK0' set
    if((!pE || (epData.endpoint[index].out.status & USB_EP_BUSNACK0_bm)) && // NOT receiving at the moment
       !aRecvQ[index]) // no new buffers to attach
    {
      // NOTE:  control endpoints need to re-establish themselves to listen for more packets,
      //        whenever the send queue is empty, so flow through in THOSE cases

      if(!index && aSendQ[0]) // control, and send queue not empty
      {
        continue;  // receive is stalled for the moment so skip this next part
      }

      if(index && _usbConfiguration == 0)
      {
        continue; // when unconfigured, don't automatically add a new buffer
      }

      // TODO:  a callback to determine whether I should NOT add a buffer automatically??

      // flow through and it should work
    }

    // check for errors before anything else
    if(epData.endpoint[index].out.status & USB_EP_STALL_bm)
    {
#ifdef DEBUG_QUEUE
      // first, handle "sent" transaction
      error_printP_(F("check_recv_queue "));
      error_printL_(index);
      error_printP(F(" - USB_EP_STALL"));
#endif // DEBUG_QUEUE

      // for now - turn off any 'stall' bit in the control reg and status reg

      epData.endpoint[index].out.status |= USB_EP_STALL_bm;

      if((epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
      {
        // clear the stall bits in the control reg in case they were on
        epData.endpoint[index].out.ctrl &= ~_BV(2); // note that this is NOT USB_EP_STALL_bm but the actual 'stall' command bit
      }

      epData.endpoint[index].out.status &= ~USB_EP_STALL_bm; // just turn it off [for now]

      continue;
    }

    if(pE) // && !(epData.endpoint[index].out.status & USB_EP_BUSNACK0_bm)) // AM receiving at the moment
    {
      // check for received data.  If received, set up length and other info, and
      // make sure it's at the end of the receive queue.  THEN set up another buffer
      // for it automatically, except for endpoint 0.  'buffer combining' might be good
      // as well, if possible.

      if(epData.endpoint[index].out.status & USB_EP_UNF_bm) // TODO:  allow me to read 'long packets' when this happens
      {
#ifdef DEBUG_QUEUE
        // first, handle "sent" transaction
        error_printP_(F("check_recv_queue "));
        error_printL_(index);
        error_printP(F(" - USB_EP_UNF  toggle="));
        error_printL(epData.endpoint[index].out.status & USB_EP_TOGGLE_bm);

        // TODO:  should I flip the toggle bit in this case???  Is it due to a DATA1/0 with toggle clear/set?
#endif // DEBUG_QUEUE

        epData.endpoint[index].out.status |= USB_EP_UNF_bm;
        epData.endpoint[index].out.status &= ~USB_EP_UNF_bm; // just turn it off [for now]
        if(index)
        {
          epData.endpoint[index].out.status ^= USB_EP_TOGGLE_bm; // does this work?
        }

        // TODO:  see if this ever happens

//        continue; // let this cycle around a bit more [I probably didn't read anything]
      }

      if((epData.endpoint[index].out.status & USB_EP_TRNCOMPL0_bm) // note:  'SETUP' is TRNCOMPL1, as needed
         || (/*!index &&*/ (epData.endpoint[index].out.status & USB_EP_SETUP_bm))) // TRNCOMPL1 for index != 0
      {
        uint8_t bOldStatus = epData.endpoint[index].out.status;

        register Setup *pSetup = (Setup *)&(pE->aBuf[0]);

//        epData.endpoint[index].out.status = USB_EP_BUSNACK0_bm                                       // mark 'do not receive'
//                                          | (bOldStatus & ~(USB_EP_SETUP_bm | USB_EP_TRNCOMPL0_bm)); // turn these 2 bits off

        epData.endpoint[index].out.status |= USB_EP_BUSNACK0_bm;                                       // mark 'do not receive'

//        if(index &&
//           (epData.endpoint[index].out.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
//        {
//          epData.endpoint[index].out.ctrl |= _BV(2); // set the 'stall' flag for bulk in, basically
//        }

//        if(bOldStatus & (USB_EP_UNF_bm | USB_EP_STALL_bm))
//        {
//          epData.endpoint[index].out.status |= (USB_EP_UNF_bm | USB_EP_STALL_bm); // to clear them (?)
//          epData.endpoint[index].out.status &= ~(USB_EP_UNF_bm | USB_EP_STALL_bm); // to clear them
//
//          bOldStatus &= ~(USB_EP_UNF_bm | USB_EP_STALL_bm); // to clear them
//        }

        if(!index ||
           (epData.endpoint[index].out.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
        {
          // IF the out status has the toggle bit set, indicate it in 'wEndpointToggle'
          // so that I correctly assign the TOGGLE bit when I reply

          // NOTE that the toggle bit SHOULD flip correctly with each complete transaction

          if(!(epData.endpoint[index].out.status & USB_EP_TOGGLE_bm)) // was 'toggle' ON or OFF for the last packet?
          {
            wEndpointToggle |= wProcessingMask; // do NOT use the 'toggle' bit when I get next packet or send a reply
          }
          else
          {
            wEndpointToggle &= ~wProcessingMask; // indicate that I need to use the 'toggle' bit when I get/send
          }
        }

#ifdef TX_RX_LED_INIT
        RxLEDPulse = TX_RX_LED_PULSE_MS;
        RXLED1(); // LED pm - macro must be defined in variants 'pins_arduino.h'
#endif // TX_RX_LED_INIT

#ifdef DEBUG_QUEUE
        error_printP_(F("check_recv_queue "));
        error_printL_(index);

        if(bOldStatus & USB_EP_TRNCOMPL0_bm)
        {
          error_printP_(F(" USB_EP_TRNCOMPL0_bm "));
        }
        else
        {
          error_printP_(F(" USB_EP_SETUP_bm "));
        }

        error_printL_(index);
        error_printP_(F(": status="));
        error_printH_(bOldStatus);
        error_printP_(F("H length="));
        error_printL_(epData.endpoint[index].out.cnt);
        error_printP_(F(" address="));
        error_printH(epData.endpoint[index].out.dataptr);
#endif // DEBUG_QUEUE

        // ASSERT( epData.endpoint[index].out.dataptr == buffer_data_pointer(aRecvQ[index]) );

            //(INTERNAL_BUFFER *)(void *)(epData.endpoint[index].out.dataptr);  <-- wrong

        // TODO:  verify pE is valid AND/OR a part of the chain

        pE->iIndex = 0;
        pE->iLen = epData.endpoint[index].out.cnt;

        // 'HOST TO DEVICE' CONTROL PACKET WITH A DATA PAYLOAD

        if(!index && // to handle control packets that have data payloads...
           !(epData.endpoint[0].out.status & USB_EP_TRNCOMPL0_bm) &&
           epData.endpoint[0].out.cnt >= sizeof(Setup) &&
           (pSetup->bmRequestType & REQUEST_DIRECTION) == REQUEST_HOSTTODEVICE &&
           pSetup->wLength > 0) // SETUP has a data payload!
        {
          error_printP_(F("CTRL packet, payload="));
          error_printL_(pSetup->wLength);
          error_printP_(F(" toggle="));
          error_printL(epData.endpoint[0].out.status & USB_EP_TOGGLE_bm);

          pE = next_buffer(); // get a new buffer

          if(pE)
          {
            add_to_queue(&aRecvQ[0], pE);

            epData.endpoint[0].out.auxdata = 0;
            epData.endpoint[0].out.dataptr = buffer_data_pointer(pE); // new pointer
            epData.endpoint[0].out.cnt = 0; // immediate receive

            // with a packet payload, I need to receive a DATA 1 packet to complete the transaction
            epData.endpoint[0].out.status = USB_EP_TOGGLE_bm;             // allows me to read data (toggle bit set)

            continue;
          }

          // note if pE is NULL, then I proceed forward with truncated packet anyway.
        }


        // THIS IS WHERE I DISPATCH THE INCOMING PACKET

        DispatchIncomingPacket(index, pE, bOldStatus & USB_EP_SETUP_bm ? 1 : 0);

        // check the send queue, see if there's anything waiting.  chances are there ARE things waiting.
        // if something is waiting, make sure it's sent before allowing more receive data

        if(!index && aSendQ[0]) // something is waiting to send on the control endpoint
        {
#ifdef DEBUG_QUEUE
          error_printP(F("check_recv_queue - waiting for send queue empty"));
#endif // DEBUG_QUEUE

          epData.endpoint[0].out.dataptr = 0; // no data will be received until I have empty send queue
          epData.endpoint[0].out.cnt = 0;
        }
        else
        {
          set_up_EP_for_receive(index); // set up to receive (without checking/freeing buffers)
          // NOTE:  if there is no buffer, this will leave the endpoint in 'BUSNACK0' mode
        }
      }
    }
    else if(index || !aSendQ[0]) // remember, control endpoints don't receive until done sending
    {
#ifdef DEBUG_QUEUE
      error_printP_(F("check_recv_queue - set up EP "));
      error_printL_(index);
      error_printP(F(" for receive"));
#endif // DEBUG_QUEUE

      set_up_EP_for_receive(index); // set up to receive
    }
  }

  SREG = oldSREG;
}

static void check_send_queue(void)
{
int index;
INTERNAL_BUFFER *pX;
uint8_t oldSREG;


  oldSREG = SREG;
  cli(); // this must be done with interrupts blocked

  for(index=0; index <= MAXEP; index++)
  {
    uint16_t wProcessingMask = 1 << index;

    // skip those not enabled
    if((epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) == USB_EP_TYPE_DISABLE_gc)
    {
      continue;
    }

    if(wMultipacketOutFlag & wProcessingMask) // I'm currently using 'IN' to receive for this endpoint
    {
      continue; // skip it - it should be left alone
    }

    pX = inverse_buffer_data_pointer(epData.endpoint[index].in.dataptr);
    uint8_t status = epData.endpoint[index].in.status;

    // check for errors before anything else

    if((status & USB_EP_STALL_bm) || // stall
       (status & USB_EP_UNF_bm))     // underflow
    {
#ifdef DEBUG_QUEUE
      // first, handle "sent" transaction
      error_printP_(F("check_send_queue "));
      error_printL_(index);
      error_printP_(F(" - "));

      if(status & USB_EP_STALL_bm) // stall
        error_printP_(F(" USB_EP_STALL"));

      if(status & USB_EP_UNF_bm)   // underflow
        error_printP_(F(" USB_EP_UNF"));

      error_printP(F("!!!"));
#endif // DEBUG_QUEUE

      epData.endpoint[index].in.status |= (USB_EP_STALL_bm | USB_EP_UNF_bm); // do I set them like this?

      if((status & USB_EP_STALL_bm) &&
         (epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
      {
        error_printP_(F("check_send_queue "));
        error_printL_(index);
        error_printP(F(" STALL"));

        epData.endpoint[index].in.ctrl |= _BV(2);  // to clear the bits
        epData.endpoint[index].in.ctrl &= ~_BV(2); // note that this is NOT USB_EP_STALL_bm but the actual 'stall' command bit
      }

      // TODO:  should a stall cause TOGGLE to flip?  for now, NO

      epData.endpoint[index].in.status &= ~(USB_EP_STALL_bm | USB_EP_UNF_bm);  // for now do this
    }

    if(pX &&
       !(epData.endpoint[index].in.status & USB_EP_TRNCOMPL0_bm))
    {
      //  TODO:  check for stale buffer???
      if(epData.endpoint[index].in.status & USB_EP_BUSNACK0_bm)
      {
        if(index)
        {
          error_printP_(F("NACK ep="));
          error_printL(index);
        }

        epData.endpoint[index].in.status &= ~USB_EP_BUSNACK0_bm; // clear this bit so I'll send
      }

      continue; // nothing else needed, I hope
    }

    if(pX &&
       (epData.endpoint[index].in.status & USB_EP_TRNCOMPL0_bm)) // note:  'SETUP' is TRNCOMPL1, if needed
    {
      uint8_t bOldStatus = epData.endpoint[index].in.status;

      if(index && 
         (epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
      {
        // IF the out status has the toggle bit set, indicate it in 'wEndpointToggle'
        // so that I correctly assign the TOGGLE bit when I send the next packet

        // NOTE that the toggle bit SHOULD flip correctly with each complete transaction

        if(!(epData.endpoint[index].in.status & USB_EP_TOGGLE_bm)) // was 'toggle' ON or OFF for the last packet?
        {
          wEndpointToggle |= wProcessingMask; // do NOT use the 'toggle' bit when I get next packet or send a reply
        }
        else
        {
          wEndpointToggle &= ~wProcessingMask; // indicate that I need to use the 'toggle' bit when I get/send
        }
      }

      epData.endpoint[index].in.status = USB_EP_BUSNACK0_bm                                       // mark 'do not send'
                                       | (bOldStatus & ~(USB_EP_TRNCOMPL0_bm | USB_EP_SETUP_bm)); // clear these 2 bits

#ifdef DEBUG_QUEUE
      error_printP_(F("check_send_queue "));
      error_printL_(index);
      error_printP_(F(" - USB_EP_TRNCOMPL0 "));
      error_printL_(index);
      error_printP_(F(": status="));
      error_printH_(bOldStatus);
      error_printP_(F(" length="));
      error_printL_(epData.endpoint[index].in.cnt);
      error_printP_(F(" address="));
      error_printH(epData.endpoint[index].in.dataptr);
#endif // DEBUG_QUEUE

      // if I need to assign the USB address, do it HERE, after packet send completes
      // the process of changing the address after getting the command to do so requires
      // that I send a zero-length packet.  I'm assuming (here) that 'any packet will do'
      // and just handling it after send completes on whatever packet is sent following
      // the request.  If this fails I'll get another 'bus reset' and it will start again
      // and so it's no big deal if it doesn't work...

      if(!index && bUSBAddress) // control endpoint needs to change the USB address?
      {
#ifdef DEBUG_QUEUE
        error_printP_(F("USB address changed to "));
        error_printL(bUSBAddress);
#endif // DEBUG_QUEUE

        USB_ADDR = bUSBAddress;
        bUSBAddress = 0; // so I don't try to change it again and again
      }

      // once a buffer has been SENT, I can remove it from the send queue as 'completed'.

      if(pX->iLen >= 64 && !pX->pNext) // last buffer and 'full size', need to send 0-length packet
      {
        epData.endpoint[index].in.cnt = 0;
        epData.endpoint[index].in.dataptr = buffer_data_pointer(pX); // do anyway, should already be 'this'

        if((!index || (epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
           && !(wEndpointToggle & wProcessingMask)) // toggle is ON for this endpoint
        {
          // DATA1 packet

#ifdef DEBUG_QUEUE
          if(index)
          {
            error_printP_(F("Endpoint "));
            error_printL_(index);
            error_printP(F(" using TOGGLE"));
          }
#endif // DEBUG_QUEUE

          epData.endpoint[index].in.status = USB_EP_TOGGLE_bm; // set JUST the 'toggle' bit to send
          wEndpointToggle |= wProcessingMask; // turn it off next time (it alternates for bulk xfer, yeah)
        }
        else
        {
          // DATA0 packet - toggle is always OFF for this endpoint

          epData.endpoint[index].in.status = 0; // send without toggle bit (ISOCHRONOUS always does this)

          if(!index || (epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
          {
#ifdef DEBUG_QUEUE
            if(index)
            {
              error_printP_(F("Endpoint "));
              error_printL_(index);
              error_printP(F(" using !TOGGLE"));
            }
#endif // DEBUG_QUEUE

            wEndpointToggle &= ~wProcessingMask; // turn it on next time (it alternates for bulk xfer, yeah)
          }
        }

        // NOTE:  leave pX not NULL; next section will skip, and we're ready to go
      }
      else
      {
        remove_from_queue_and_free(&(aSendQ[index]), pX);
        pX = NULL; // also a flag to add new buffer in next section
      }
    }

    if(!pX) // not in process of sending
    {
      pX = aSendQ[index]; // new buffer (TODO:  walk entire chain looking for 'ready to send' packets?)

      if(pX && INTERNAL_BUFFER_SEND_READY(pX)) // only send if ready to send
      {
#ifdef DEBUG_QUEUE
        error_printP_(F("check_send_queue "));
        error_printL_(index);
        error_printP_(F("  NEXT buffer: "));
        error_printH_((unsigned long)pX);
        error_printP_(F(" address="));
        error_printH_(buffer_data_pointer(pX));
        error_printP_(F(" length="));
        error_printL(pX->iLen);
#endif // DEBUG_QUEUE

        epData.endpoint[index].in.cnt = pX->iLen;
        epData.endpoint[index].in.dataptr = buffer_data_pointer(pX);

        if(!index || (epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc)
        {
          if(!(wEndpointToggle & wProcessingMask))
          {
            if(index)
            {
#ifdef DEBUG_QUEUE
              error_printP_(F("Endpoint "));
              error_printL_(index);
              error_printP(F(" using TOGGLE"));
#endif // DEBUG_QUEUE
            }

            epData.endpoint[index].in.status = USB_EP_TOGGLE_bm; // send with toggle bit
            wEndpointToggle |= wProcessingMask; // turn it off next time (it alternates for bulk xfer, yeah)
          }
          else
          {
            if(index)
            {
#ifdef DEBUG_QUEUE
              error_printP_(F("Endpoint "));
              error_printL_(index);
              error_printP(F(" using !TOGGLE"));
#endif // DEBUG_QUEUE
            }

            epData.endpoint[index].in.status = 0; // send without toggle bit
            wEndpointToggle &= ~wProcessingMask; // turn it on next time (it alternates for bulk xfer, yeah)
          }
        }
        else
        {
          epData.endpoint[index].in.status = 0; // send without toggle bit (ISOCHRONOUS always does this)
        }
      }
      else
      {
        // OK I'm officially "not sending" now, and so I must reset the DATA thingy

        epData.endpoint[index].in.status |= USB_EP_BUSNACK0_bm; // mark 'do not send' (make sure) but leave TOGGLE alone

        epData.endpoint[index].in.cnt = 0;
        epData.endpoint[index].in.dataptr = 0;
      }
    }
  }

  SREG = oldSREG;
}

static void internal_flush(int index) // send all pending data in send queue
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;


  oldSREG = SREG;
  cli(); // disable interrupts

  pB = aSendQ[index];

  while(pB)
  {
    if(!INTERNAL_BUFFER_SEND_READY(pB))
    {
      INTERNAL_BUFFER_MARK_SEND_READY(pB);
    }

    pB = pB->pNext;
  }

  SREG = oldSREG;
}

// queue up "zero length" send data
static bool internal_send0(int index)
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;


#ifdef DEBUG_QUEUE
  error_printP_(F("internal_send0 "));
  error_printL(index);
#endif // DEBUG_QUEUE

#ifdef TX_RX_LED_INIT
  TxLEDPulse = TX_RX_LED_PULSE_MS;
  TXLED1(); // LED pm - macro must be defined in variants 'pins_arduino.h'
#endif // TX_RX_LED_INIT


  oldSREG = SREG;
  cli(); // disable interrupts

  pB = end_of_chain(aSendQ[index]);

  if(pB && !INTERNAL_BUFFER_SEND_READY(pB)) // see if I need to mark the previous buffer as 'ready to send'
  {
    INTERNAL_BUFFER_MARK_SEND_READY(pB); // mark 'ready to send' so that it goes out
  }

  pB = next_buffer(); // always

  if(pB)
  {
    pB->iIndex = pB->iLen = 0; // zero-length

    INTERNAL_BUFFER_MARK_SEND_READY(pB); // mark 'ready to send' so that it goes out
    add_to_queue(&(aSendQ[index]), pB);

    check_send_queue();
  }

  SREG = oldSREG;

  return pB != NULL; // indicate success/fail
}

// queue up send data
static bool internal_send(int index, const void *pData, uint16_t cbData, uint8_t bSendNow)
{
INTERNAL_BUFFER *pB;
const uint8_t *pD = (const uint8_t *)pData;
uint8_t oldSREG;


  if(!cbData)
  {
    if(bSendNow)
    {
#ifdef DEBUG_QUEUE
      error_printP_(F("internal_send "));
      error_printL_(index);
      error_printP(F(" - calling internal_send0"));
#endif // DEBUG_QUEUE

      return internal_send0(index); // send a ZLP
    }
    else
    {
      error_printP_(F("internal_send - zero bytes, no effect"));
    }

    return true; // indicate 'ok' anyway
  }

  if(!pData)
  {
    error_printP_(F("internal_send "));
    error_printL_(index);
    error_printP(F(" - NULL pointer"));

    return false;
  }

#ifdef DEBUG_QUEUE
  error_printP_(F("internal_send  USB addr="));
  error_printL_(USB_ADDR);
  error_printP_(F(" EP="));
  error_printL_(index);
  error_printP_(F(" len="));
  error_printL_(cbData);
  error_printP(F(" bytes"));
#endif // DEBUG_QUEUE

#ifdef TX_RX_LED_INIT
  TxLEDPulse = TX_RX_LED_PULSE_MS;
  TXLED1(); // LED pm - macro must be defined in variants 'pins_arduino.h'
#endif // TX_RX_LED_INIT


  oldSREG = SREG;
  cli(); // disable interrupts

  pB = end_of_chain(aSendQ[index]);

  if(!pB || INTERNAL_BUFFER_SEND_READY(pB)) // see if I need to allocate a new buffer
  {
    pB = next_buffer();
    pB->iIndex = pB->iLen = 0; // make sure
  }

  while(pB)
  {
    register uint8_t cb, cbSize;

    if(pB->iIndex <  sizeof(pB->aBuf))
    {
      cbSize = sizeof(pB->aBuf) - pB->iIndex;
    }
    else
    {
      pB->iIndex = sizeof(pB->aBuf); // fix it, in case of corruption
      cbSize = 0;
    }

    if(cbData > cbSize)
    {
      cb = cbSize;
    }
    else
    {
      cb = cbData;
    }

    if(cb) // just in case, should never be zero here
    {
      memcpy(&(pB->aBuf[pB->iIndex]), pD, cb);
    }

    pB->iIndex += cb;      // new position within the buffer
    pB->iLen = pB->iIndex; // assign these to the same value ('send ready' will use it)

    pD += cb;
    cbData -= cb;

    if(bSendNow || pB->iLen >= sizeof(pB->aBuf)) // note that 'aBufList' size MUST match max packet size for EP
    {
      INTERNAL_BUFFER_MARK_SEND_READY(pB); // mark 'ready to send' so that it goes out
    }

    if(not_in_queue(&(aSendQ[index]), pB))
    {
      add_to_queue(&(aSendQ[index]), pB);
    }

    if(!cbData) // this ends the loop.
    {
      if(bSendNow)
      {
        check_send_queue(); // to force a write
      }

      SREG = oldSREG;
      return true; // done!
    }

    pB = next_buffer();
  }

  // if I get here, pB is NULL

  SREG = oldSREG;

//#ifdef DEBUG_QUEUE
  error_printP_(F("internal_send "));
  error_printL_(index);
  error_printP(F(" - NULL buffer"));
//#endif // DEBUG_QUEUE

  return false; // if I get here, something failed
}

// byte-level receive
static int internal_receive(int index, void *pData, uint16_t nMax)
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;
int iRval = 0;
uint8_t *pD = (uint8_t *)pData;


#ifdef DEBUG_QUEUE
  error_printP_(F("internal_receive "));
  error_printL(index);
#endif // DEBUG_QUEUE

  check_recv_queue();

  // this manipulates buffers, so stop ints temporarily

  oldSREG = SREG;
  cli(); // disable interrupts

  pB = aRecvQ[index]; // first buffer

  while(pB && nMax && pB->iLen > 0)//INTERNAL_BUFFER_RECV_READY(pB))
  {
    uint8_t cb = pB->iLen - pB->iIndex;

    if(cb > 0)
    {
      if(cb > nMax)
      {
        cb = nMax;
      }

      if(pData)
      {
        memcpy(pD, &(pB->aBuf[pB->iIndex]), cb);
      }

      nMax -= cb;
      pD += cb;
      pB->iIndex += cb;

      iRval += cb;
    }

    if(pB->iIndex >= pB->iLen)
    {
      // get next packet
      aRecvQ[index] = pB->pNext; // it's always the head of the queue
      pB->pNext = NULL;
      free_buffer(pB);

      if(!index) // for control, don't exceed packet boundary
      {
        break;
      }

      pB = aRecvQ[index];
    }
  }

  SREG = oldSREG;
  return iRval;
}


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//           _____             _                _         _                 //
//          | ____| _ __    __| | _ __    ___  (_) _ __  | |_  ___          //
//          |  _|  | '_ \  / _` || '_ \  / _ \ | || '_ \ | __|/ __|         //
//          | |___ | | | || (_| || |_) || (_) || || | | || |_ \__ \         //
//          |_____||_| |_| \__,_|| .__/  \___/ |_||_| |_| \__||___/         //
//                               |_|                                        //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_CODE
static uint16_t endpoint_data_pointer(void) __attribute__((noinline));
#endif // DEBUG_CODE
static void InitializeSingleEndpoint(XMegaEndpointChannel *pEP) __attribute__((noinline));

uint16_t endpoint_data_pointer(void)
{
  return (uint16_t)(uint8_t *)&(epData.endpoint[0]); // set 'EP Data' pointer to THIS address
}


static void InitializeSingleEndpoint(XMegaEndpointChannel *pEP)
{
  // if the endpoint is in the middle of something, this will 'cancel' it
  // NOTE: 'BUSNACK0' is needed by 'in' on 128A1U rev K or earlier [a bug, apparently] - leave it on
  pEP->out.status |= USB_EP_BUSNACK0_bm;
  pEP->in.status |= USB_EP_BUSNACK0_bm;

  pEP->out.ctrl = USB_EP_TYPE_DISABLE_gc; // to disable it (endpoint 'type 0' disables)
  pEP->in.ctrl = USB_EP_TYPE_DISABLE_gc;  // initially (disable)

  pEP->in.cnt = 0;
  pEP->in.dataptr = 0;
  pEP->in.auxdata = 0;

  pEP->out.cnt = 0;
  pEP->out.dataptr = 0;
  pEP->out.auxdata = 0;

  pEP->out.status = USB_EP_BUSNACK0_bm; // make sure
  pEP->in.status = USB_EP_BUSNACK0_bm;
}


void InitEP(u8 index, u8 type, u8 size)
{
uint8_t oldSREG;
int i1;


  if(index > MAXEP || index >= INTERNAL_NUM_EP)  // MAXEP or INTERNAL_NUM_EP will be the highest index (MAXEP is inclusive)
  {
    return;
  }

  // IMPORTANT:  the definition of 'in' and 'out' are from the perspective of the USB HOST
  //             Since I'm on 'the other end', 'in' writes data, 'out' receives it

  oldSREG = SREG;
  cli(); // disable interrupts

  // clear the appropriate bit in all of the global state vars
  uint16_t wProcessingMask = 1 << index;

  wProcessingFlag     &= ~wProcessingMask;
  wMultipacketOutFlag &= ~wProcessingMask;
  wEndpointToggle     &= ~wProcessingMask;


  // NOTE:  this code is based on my research into the documentation (inadequate) and the ATMel Studio
  //        sample project (somewhat difficult to follow) after spending a couple of weeks or so frustratingly
  //        attempting to use ONLY the information found in the 'AU' manual, which SHOULD be enough (but was not).
  //        In particular the behavior CAUSED by the 'NACK0' flag, and the requirement to set it on 'in' endpoints
  //        upon initialization for 128A1U rev K or earlier was NOT obvious, nor even mentioned as far as I know.

  XMegaEndpointChannel *pEP = &(epData.endpoint[index]);

  error_printP_(F("USB InitEP "));
  error_printL(index);

  InitializeSingleEndpoint(pEP);

  free_queue(&(aSendQ[index]));
  free_queue(&(aRecvQ[index]));

  if(index == 0 && type == EP_TYPE_CONTROL) // control (these can receive SETUP requests)
  {
    // aBuf1 is output, aBuf2 is input

    aRecvQ[0] = next_buffer();  // allocate buffer

    epData.endpoint[0].out.dataptr = buffer_data_pointer(aRecvQ[index]);

    // NOTE:  size will be sent as 'EP_SINGLE_64'

    epData.endpoint[0].out.ctrl = USB_EP_TYPE_CONTROL_gc // NOTE:  interrupt enabled
                                | USB_EP_SIZE_64_gc;//(size == EP_SINGLE_64 ? USB_EP_SIZE_64_gc : 0);        /* data size */

    // NOTE: 'BUSNACK0' is needed by 'in' on 128A1U rev K or earlier [a bug, apparently, see errata]
    epData.endpoint[0].out.status = 0; // make sure they're ready to go (this allows receive data)

    // cancel 'in' queue
    epData.endpoint[0].in.status = USB_EP_BUSNACK0_bm; // leave 'BUSNACK0' bit ON (stalls sending data)
    epData.endpoint[0].in.dataptr = 0;
    epData.endpoint[0].in.cnt = 0;

    epData.endpoint[0].in.ctrl = USB_EP_TYPE_CONTROL_gc // NOTE:  interrupt enabled
                               | USB_EP_SIZE_64_gc;     /* data size */

    // zero out the rest of the endopints, leaving ONLY the control
    for(i1=1; i1 <= MAXEP; i1++)
    {
      pEP = &(epData.endpoint[i1]);

      InitializeSingleEndpoint(pEP);

      if(i1 < INTERNAL_NUM_EP)
      {
        free_queue(&(aSendQ[i1]));
        free_queue(&(aRecvQ[i1]));
      }
    }

    // after initializing all endpoints, zero out all of the global state flags for all endpoints
    wProcessingFlag = 0;
    wMultipacketOutFlag = 0;
    wEndpointToggle = 0;
  }
  else if(type == EP_TYPE_INTERRUPT_IN || type == EP_TYPE_BULK_IN
          || type == EP_TYPE_ISOCHRONOUS_IN) /* these types have *ME* write data and send to 'in' for host */
  {
    // 'in' is actually the WRITE/SEND function

    pEP->in.status = USB_EP_BUSNACK0_bm; // leave 'BUSNACK0' bit ON (stalls sending data)
    pEP->in.dataptr = 0;
    pEP->in.auxdata = 0;

    pEP->in.cnt = 0; // no data (so I won't send) - note 'ZLP_BIT' is broken (don't bother)

    pEP->in.ctrl = (type == EP_TYPE_ISOCHRONOUS_IN ? USB_EP_TYPE_ISOCHRONOUS_gc : USB_EP_TYPE_BULK_gc)
//                 | (type == EP_TYPE_BULK_IN ? USB_EP_INTDSBL_bm : 0)       /* disable interrupt */
                 | (size == EP_DOUBLE_64 ? USB_EP_SIZE_64_gc :             // TODO:  set 'double buffer' flag?
                    size == EP_SINGLE_64 ? USB_EP_SIZE_64_gc : 0);         /* data size */

    if(type != EP_TYPE_ISOCHRONOUS_IN)
    {
      error_printP(F("  setting TOGGLE bit to '1'"));
      wEndpointToggle |= wProcessingMask; // (1 << index); // set bit so next time I don't use TOGGLE
    }
  }
  else if(type == EP_TYPE_INTERRUPT_OUT || type == EP_TYPE_BULK_OUT /* these send *ME* data */
          || type == EP_TYPE_ISOCHRONOUS_OUT)
  {
    // 'out' is actually the RECEIVE function

    pEP->out.status = USB_EP_BUSNACK0_bm; // disable receive at first

    aRecvQ[index] = next_buffer();  // allocate buffer
    pEP->out.dataptr = buffer_data_pointer(aRecvQ[index]);

    pEP->out.auxdata = 0;

    pEP->out.cnt = 0; // no data (so I can receive)

    pEP->out.ctrl = (type == EP_TYPE_ISOCHRONOUS_OUT ? USB_EP_TYPE_ISOCHRONOUS_gc : USB_EP_TYPE_BULK_gc)
//                  | (type == EP_TYPE_BULK_OUT ? USB_EP_INTDSBL_bm : 0)      /* disable interrupt */
                  | (size == EP_DOUBLE_64 ? USB_EP_SIZE_64_gc :             // TODO:  set 'double buffer' flag?
                     size == EP_SINGLE_64 ? USB_EP_SIZE_64_gc : 0);         /* data size */

    pEP->out.status = 0; // this allows receive data

    if(type != EP_TYPE_ISOCHRONOUS_OUT)
    {
      error_printP(F("  setting TOGGLE bit to '1'"));
      wEndpointToggle |= wProcessingMask; // (1 << index); // set bit so next time I don't use TOGGLE
    }
  }
  // TODO:  'INOUT' types?
  else
  {
    // endpoint 'disabled' now
  }

  SREG = oldSREG; // restore interrupts (etc.)

  // TODO:  anything else?
}

// this function consolidates queued packets - useful for receiving bulk data
void consolidate_packets(INTERNAL_BUFFER *pHead)
{
INTERNAL_BUFFER *pX, *pX2;

  // assume I can consolidate everything into the least number of packets

  if(!pHead)
  {
    return;
  }

  while(pHead->pNext)
  {
    while(pHead->pNext && pHead->iLen >= sizeof(pHead->aBuf))
    {
      pHead = pHead->pNext; // find next non-full buffer
    }

    pX = pHead->pNext;

    if(!pX) // first 'incomplete' buffer found - anything else?
    {
      return; // I am done
    }

    error_printP_(F("Consolidating "));
    error_printL(pX->iLen);
    error_printP_(F(" left="));
    error_printL(sizeof(pHead->aBuf) - pHead->iLen);

    do
    {
      uint8_t nLeft = sizeof(pHead->aBuf) - pHead->iLen;

      if(nLeft >= pX->iLen)
      {
        memcpy(&(pHead->aBuf[pHead->iLen]), pX->aBuf, pX->iLen);
        pHead->iLen += pX->iLen;

        pX2 = pX;
        pX = pX->pNext;

        remove_from_queue(&pHead, pX2);
        free_buffer(pX2);

        if(!pX)
        {
          return; // I am done
        }
      }
      else
      {
        memcpy(&(pHead->aBuf[pHead->iLen]), pX->aBuf, nLeft);
        pHead->iLen += nLeft;

        pX->iLen -= nLeft;
        memmove(&(pX->aBuf[0]), &(pX->aBuf[nLeft]), pX->iLen);
      }

    } while(pHead->iLen < sizeof(pHead->aBuf));

    error_printP_(F("new head length "));
    error_printL(pHead->iLen);

    // get the next head, search for unconsolidated bufs (again)
    // slightly less efficient, but simpler algorithm

    pHead = pHead->pNext;
  }
}

uint8_t USB_GetEPType(uint8_t nEP)
{
uint8_t nRval = 0xff;

  if(nEP < INTERNAL_NUM_EP)
  {
    nRval = pgm_read_byte(&(_initEndpoints[nEP]));
  }

  return nRval;
}

void internal_do_control_request(INTERNAL_BUFFER *pBuf, bool bIsSetup)
{
INTERNAL_BUFFER *pX;
uint8_t requestType;


  if(pBuf && (pBuf->iLen - pBuf->iIndex) >= 8) // control packets only send one of these
  {
    Setup *pSetup = (Setup *)&(pBuf->aBuf[pBuf->iIndex]); // must point DIRECTLY to packet buffer
    uint8_t ok = 0;

    pBuf->iIndex += sizeof(Setup); // add the # of bytes I have read thus far

#ifdef DEBUG_CONTROL
    error_printP_(F("USB setup/control  request="));
    error_printL_(pSetup->bRequest);
    error_printP_(F(" type="));
    error_printL_(pSetup->bmRequestType);
    error_printP_(F(" value="));
    error_printL_(pSetup->wValueH);
    error_printP_(F(":"));
    error_printL_(pSetup->wValueL);
    error_printP_(F(" index="));
    error_printL_(pSetup->wIndex);
    error_printP_(F(" length="));
    error_printL(pSetup->wLength);
#endif // DEBUG_CONTROL

    requestType = pSetup->bmRequestType;

    if((requestType & REQUEST_DIRECTION) == REQUEST_DEVICETOHOST) // i.e. an 'in'
    {
#ifdef DEBUG_CONTROL
      error_printP(F("dev to host request"));
#endif // DEBUG_CONTROL

      // TODO:  will there be multiple requests in the buffer?
    }
    else // HOST TO DEVICE - an 'out' - will be sending ZLP on success
    {
#ifdef DEBUG_CONTROL
      error_printP(F("host to dev")); // - eat remainder of packet"));
#endif // DEBUG_CONTROL

//      pBuf->iIndex = pBuf->iLen; // this eats the remainder of the packet
    }

    if(REQUEST_STANDARD == (requestType & REQUEST_TYPE))
    {
      //  Standard Requests
      uint8_t r;

#ifdef DEBUG_CONTROL
      error_printP_(F("standard request "));
      error_printL(pSetup->bRequest);
#endif // DEBUG_CONTROL

      r = pSetup->bRequest;

      if (GET_STATUS == r) // 0
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: get status"));
#endif // DEBUG_CONTROL

        // send a 2-byte packet with 2 zero bytes in it
        pX = next_buffer();

        if(pX)
        {
          pX->iLen = pX->iIndex = 2;
          pX->aBuf[0] = 0;
          pX->aBuf[1] = 0;

          INTERNAL_BUFFER_MARK_SEND_READY(pX);

          add_to_queue(&aSendQ[0], pX);
          check_send_queue(); // transmit available packets
        }
      }
      else if (CLEAR_FEATURE == r) // 1
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: clear feature"));
#endif // DEBUG_CONTROL

        ok = true; // temporary
      }
      else if (SET_FEATURE == r) // 3
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: set feature"));
#endif // DEBUG_CONTROL

        ok = true; // temporary
      }
      else if (SET_ADDRESS == r) // 5
      {
#ifdef DEBUG_CONTROL
        error_printP_(F("CONTROL: set address ="));
        error_printL(pSetup->wValueL & 0x7f);
#endif // DEBUG_CONTROL

        bUSBAddress = pSetup->wValueL & 0x7f; // this will asynchronously set the address

        ok = true;
      }
      else if (GET_DESCRIPTOR == r) // 6
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: get descriptor"));
#endif // DEBUG_CONTROL

        ok = SendDescriptor(*pSetup);  // TODO POOBAH FIX THIS
      }
      else if (SET_DESCRIPTOR == r) // 7
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: set descriptor"));
#endif // DEBUG_CONTROL

        ok = false;
      }
      else if (GET_CONFIGURATION == r) // 8
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: get config"));
#endif // DEBUG_CONTROL

        // send a 1-byte packet with a 1 byte in it
        pX = next_buffer();

        if(pX)
        {
          pX->iLen = pX->iIndex = 1;
          pX->aBuf[0] = 1; // always config #1???  TODO:  see if I should return _usbConfiguration

          INTERNAL_BUFFER_MARK_SEND_READY(pX);

          add_to_queue(&aSendQ[0], pX);

          internal_flush(0);
          check_send_queue(); // transmit available packets
        }

        ok = true;
      }
      else if (SET_CONFIGURATION == r) // 9
      {
#ifdef DEBUG_CONTROL
        error_printP(F("CONTROL: set config"));
#endif // DEBUG_CONTROL

        if(REQUEST_DEVICE == (requestType & REQUEST_RECIPIENT))
        {
#ifdef DEBUG_CONTROL
          error_printP(F("CONTROL: request device"));
#endif // DEBUG_CONTROL

          // SEE SECTION 20.3 in 'AU' manual for sequence of operation

#ifdef DEBUG_CONTROL
          error_printP(F("USB InitEndpoints")); // was 'InitEndpoints()'
#endif // DEBUG_CONTROL

          // init the first one as a control input
          for (uint8_t i = 1; i < sizeof(_initEndpoints); i++)
          {
            InitEP(i, pgm_read_byte(_initEndpoints+i), EP_SINGLE_64);
                   // EP_DOUBLE_64); // NOTE:  EP_DOUBLE_64 allocates a 'double bank' of 64 bytes, with 64 byte max length
          }

          _usbConfiguration = pSetup->wValueL;

          ok = true;
        }
        else
        {
#ifdef DEBUG_CONTROL
          error_printP(F("CONTROL: other 'set config' request ="));
          error_printL(requestType);
#endif // DEBUG_CONTROL

          ok = false;
        }
      }
      else if (GET_INTERFACE == r) // 10
      {
#ifdef DEBUG_CONTROL
        error_printP(F("get interface request - temporarily returns NOTHING"));
#endif // DEBUG_CONTROL

        ok = true; // TEMPORARY
      }
      else if (SET_INTERFACE == r) // 11
      {
#ifdef DEBUG_CONTROL
        error_printP(F("set interface request - temporarily does NOTHING"));
#endif // DEBUG_CONTROL

        ok = true; // TEMPORARY
      }
      else
      {
        error_printP_(F("CONTROL: unknown request "));
        error_printL_(pSetup->bRequest);
        error_printP_(F(", index="));
        error_printL(pSetup->wIndex);

        ok = false; // returns an error, sends ZLP for things that expect data
      }
    }
    else
    {
#ifdef DEBUG_CONTROL
      error_printP_(F("INTERFACE REQUEST "));

      if((requestType & REQUEST_TYPE) == REQUEST_CLASS)
      {
        error_printP_(F("(CLASS)"));
      }
      else if((requestType & REQUEST_TYPE) == REQUEST_VENDOR)
      {
        error_printP_(F("(VENDOR)"));
      }
      else // probably impossible
      {
        error_printP_(F("type="));
        error_printH_(requestType & REQUEST_TYPE);
      }

      error_printP_(F(" request="));
      error_printL_(pSetup->bRequest);
      error_printP_(F(" index="));
      error_printL_(pSetup->wIndex);
      error_printP_(F(" len="));
      error_printL_(pSetup->wLength);
#endif // DEBUG_CONTROL

      uint8_t idx = pSetup->wIndex;

#ifdef CDC_ENABLED
      if(CDC_ACM_INTERFACE == idx)
      {
#ifdef DEBUG_CONTROL
        error_printP(F(" (CDC)"));
#endif // DEBUG_CONTROL

        ok = CDC_Setup(*pSetup); // sends 7 control bytes
      }
#ifdef HID_ENABLED
      else
#endif // HID_ENABLED
#endif // CDC_ENABLED
#ifdef HID_ENABLED
      if(HID_INTERFACE == idx)
      {
#ifdef DEBUG_CONTROL
        error_printP(F(" (HID)"));
#endif // DEBUG_CONTROL

        ok = HID_Setup(*pSetup);
      }
#endif // HID_ENABLED
#if defined(CDC_ENABLED) || defined(HID_ENABLED)
      else
#endif // defined(CDC_ENABLED) || defined(HID_ENABLED)
      {
#ifdef DEBUG_CONTROL
        error_printP_(F(" (unknown dev index "));
        error_printL_(idx);
        error_printP(F(")"));
#endif // DEBUG_CONTROL

        ok = ClassInterfaceRequest(*pSetup);
      }

      internal_flush(0);
      check_send_queue(); // send whatever's ready to go
    }

    if(ok || (requestType & REQUEST_DIRECTION) != REQUEST_DEVICETOHOST)
    {
      if(!ok || (requestType & REQUEST_DIRECTION) == REQUEST_HOSTTODEVICE) // i.e. an 'out' or unresponsive 'in'
      {
#ifdef DEBUG_CONTROL
        if(!ok) // device to host request expects return value so give it one
        {
          error_printP(F("sending ZLP for DEVICE TO HOST"));
        }
        else
        {
          error_printP(F("sending ZLP for HOST TO DEVICE"));
        }
#endif // DEBUG_CONTROL

        internal_send0(0); // send a zero-length packet [this acknowledges what I got]
      }
      else
      {
        // if I get 'ok' it should have already sent a packet for an 'in' (device to host)
        // NOTE:  everything else will send a regular packet with data in it
      }

      internal_flush(0); // this is actually an asynchronous flush operation - mark 'send it' basically
      check_send_queue();
    }
  }
}



////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//   _   _  _         _             _                    _      _     ____  ___   //
//  | | | |(_)  __ _ | |__         | |  ___ __   __ ___ | |    / \   |  _ \|_ _|  //
//  | |_| || | / _` || '_ \  _____ | | / _ \\ \ / // _ \| |   / _ \  | |_) || |   //
//  |  _  || || (_| || | | ||_____|| ||  __/ \ V /|  __/| |  / ___ \ |  __/ | |   //
//  |_| |_||_| \__, ||_| |_|       |_| \___|  \_/  \___||_| /_/   \_\|_|   |___|  //
//             |___/                                                              //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

// "high level" API - manipulates buffers (async only) - no blocking
int USB_SendControlP(uint8_t flags, const void * PROGMEM_ORIG d, int len)
{
  if(!len) // TODO:  ZLP?
  {
    if(internal_send0(0))
    {
      return 0x8000;
    }
    else
    {
      return 0; // error
    }
  }

  uint8_t *p1 = (uint8_t *)malloc(len);

  if(!p1)
  {
    return 0;
  }

  memcpy_P(p1, d, len);

  len = internal_send(0, p1, len, 1); // auto-flush packet

  free(p1);

  return len; // return the length that I send or 0 on error
}

int USB_SendControl(uint8_t flags, const void* d, int len)
{
  if(flags & TRANSFER_PGM) // PROGMEM memory
  {
    return USB_SendControlP(flags, d, len);
  }

  // TODO:  TRANSFER_RELEASE, TRANSFER_ZERO <-- what do these do?

  if(!len) // TODO:  ZLP?
  {
    if(internal_send0(0)) // ZLP
    {
      return 0x8000;
    }
    else
    {
      return 0; // error
    }
  }

  return internal_send(0, d, len, 1); // auto-flush packet
}


uint16_t USB_Available(uint8_t ep)
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;
uint16_t iRval = 0;

  // this manipulates buffers, so stop ints temporarily

  oldSREG = SREG;
  cli(); // disable interrupts

  pB = aRecvQ[ep];

  while(pB)
  {
    if(INTERNAL_BUFFER_RECV_READY(pB))
    {
      if(pB->iIndex < pB->iLen)
      {
        iRval += pB->iLen - pB->iIndex;
      }
    }

    pB = pB->pNext;
  }

  SREG = oldSREG;

  return iRval;
}


bool USB_IsStalled(uint8_t index)
{
bool bRval = false;
uint8_t oldSREG;


  oldSREG = SREG;
  cli(); // disable interrupts

  // checking this requires knowledge of the endpoint type

  if((epData.endpoint[index].out.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_DISABLE_gc)
  {
    if(epData.endpoint[index].out.status & (USB_EP_UNF_bm | USB_EP_STALL_bm))
    {
      bRval = true;
      goto the_end;
    }

    if((epData.endpoint[index].out.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_ISOCHRONOUS_gc
       && (epData.endpoint[index].out.ctrl & _BV(2))) // stall bit ON in control reg
    {
      bRval = true;
      goto the_end;
    }
  }

  if((epData.endpoint[index].in.ctrl & USB_EP_TYPE_gm) != USB_EP_TYPE_DISABLE_gc)
  {
    if(epData.endpoint[index].in.status & (USB_EP_UNF_bm | USB_EP_STALL_bm))
    {
      bRval = true;
      goto the_end;
    }
  }

the_end:

  SREG = oldSREG;

  return bRval;
}


bool USB_IsSendQFull(uint8_t ep)
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;
uint8_t nBuf = 0;

  // this manipulates buffers, so stop ints temporarily

  oldSREG = SREG;
  cli(); // disable interrupts

  pB = aSendQ[ep];

  while(pB)
  {
    if(INTERNAL_BUFFER_SEND_READY(pB) ||
       INTERNAL_BUFFER_SENDING(pB))
    {
      nBuf++;
      if(nBuf > 2)
      {
        SREG = oldSREG;

        return true;
      }
    }

    pB = pB->pNext;
  }

  SREG = oldSREG;

  return false;
}


uint16_t USB_SendQLength(uint8_t ep)
{
INTERNAL_BUFFER *pB;
uint8_t oldSREG;
uint16_t iRval = 0;

  // this manipulates buffers, so stop ints temporarily

  oldSREG = SREG;
  cli(); // disable interrupts

  pB = aSendQ[ep];

  while(pB)
  {
    if(INTERNAL_BUFFER_SEND_READY(pB) ||
       INTERNAL_BUFFER_SENDING(pB))
    {
      iRval += pB->iLen;
    }

    pB = pB->pNext;
  }

  SREG = oldSREG;

  return iRval;
}


void USB_Flush(uint8_t ep) // sends all pending data
{
  internal_flush(ep);
}


int USB_Send(uint8_t ep, const void* data, int len, uint8_t bSendNow)
{
uint8_t index = ep & 0xf;
uint16_t wProcessingMask = 1 << index;


  if(index && !aSendQ[index]) // nothing in there at the moment?
  {
    if(ep & TRANSFER_TOGGLE_ON)
    {
      error_printP_(F("USB_Send - TRANSFER_TOGGLE_ON - ep="));
      error_printL(index);

      wEndpointToggle &= ~wProcessingMask; // toggle bit ON
    }
    else if(ep & TRANSFER_TOGGLE_OFF)
    {
      error_printP_(F("USB_Send - TRANSFER_TOGGLE_OFF - ep="));
      error_printL(index);

      wEndpointToggle |= wProcessingMask; // toggle bit OFF
    }
  }

  return internal_send(index, data, len, bSendNow) ? len : 0;
}


int USB_Recv(uint8_t ep, void* data, int len)
{
int iRval;

  // THIS ONLY WORKS FOR OUT ENDPOINTS - if it's not an 'OUT', then there will
  // be no buffered read data available.  Does NOT work for control endpoints.

  if(!ep)
  {
    return -1; // can't use for the control endpoint
  }

  iRval = internal_receive(ep, data, len);

  return iRval < 0 ? -1 : iRval;

//  return iRval < 0 ? -1 : iRval == len ? 0 : 1;  // TODO:  is this right?
}


int USB_Recv(uint8_t ep)
{
static uint8_t a1;
int iRval;

  // THIS ONLY WORKS FOR OUT ENDPOINTS - if it's not an 'OUT', then there will
  // be no buffered read data available.  Does NOT work for control endpoints.

  if(!ep)
  {
    return -1; // can't use for the control endpoint
  }

  iRval = internal_receive(ep, &a1, 1);

  if(iRval < 0)
  {
    return -1;
  }
  else if(iRval == 1) // there was data and I read it (1 byte)
  {
    return a1;
  }
  else
  {
    return 0; // for now [no data] [TODO:  block?]
  }
}


// an API for debugging help (mostly)
uint16_t GetFrameNumber(void)
{
  return epData.framenum; // last frame number read in by the device
}



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//         ___         _                                   _                //
//        |_ _| _ __  | |_  ___  _ __  _ __  _   _  _ __  | |_  ___         //
//         | | | '_ \ | __|/ _ \| '__|| '__|| | | || '_ \ | __|/ __|        //
//         | | | | | || |_|  __/| |   | |   | |_| || |_) || |_ \__ \        //
//        |___||_| |_| \__|\___||_|   |_|    \__,_|| .__/  \__||___/        //
//                                                 |_|                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//  General interrupt (for the xmega, it's 'BUSEVENT' to handle SOF, etc.)
ISR(USB_BUSEVENT_vect) // USB_GEN_vect)
{
uint8_t udint;
static bool bSOF = false; // use this to detect 'start of frame'


  udint = *((volatile uint8_t *)&(USB_INTFLAGSACLR));// UDINT;

  // on startup of the hardware
  if((udint & (USB_SUSPENDIF_bm | USB_RESUMEIF_bm)) == (USB_SUSPENDIF_bm | USB_RESUMEIF_bm))
  {
    // NOTE:  I have observed that when you power up the USB for the first time, you get an interrupt
    //        in which BOTH the RESUME and SUSPEND bits are set.  This may *not* be a documented behavior
    //        but it happens, and I'd like to do something maybe... ?
#ifdef DEBUG_CODE
    error_printP(F("USB powerup"));
#endif // DEBUG_CODE

    bSOF = false;
  }
  else if(udint & USB_SUSPENDIF_bm)
  {
    // NOTE:  I get several suspend/resume combos after pulling the USB cable out
  }
  else if(udint & USB_RESUMEIF_bm)
  {
    // NOTE:  I get several suspend/resume combos after pulling the USB cable out

    if(!bSOF) // did I get an SOF recently?
    {
      // NOTE:  if this causes spurious misfires on 'disconnected', then use a counter of
      //        the times I get 'resume IF' and set it to zero when I get an SOF.  If the
      //        resume count happens enough times, call THAT "the disconnect".

      if(_usbConfiguration) // if I didn't, and I'm configured, the device was UN-PLUGGED!
      {
#ifdef DEBUG_CODE
        error_printP(F("USB Disconnected"));
#endif // DEBUG_CODE

        // TODO:  'On disconnect' handler??

        goto do_reset_interface;
      }
    }
    else
    {
      bSOF = false;
    }
  }

  //  End of Reset - happens when you first plug in, etc.
  //                 (typically a reset will do this several times in a row)
  if(udint & USB_RSTIF_bm)
  {
#ifdef DEBUG_CODE
    if(USB_ADDR != 0 || _usbConfiguration)
    {
      error_printP_(F("USB RESET: ADDR="));
      error_printL(USB_ADDR);
    }
#endif // DEBUG_CODE

do_reset_interface:

    USB_ADDR = 0; // IMMEDIATELY set USB address to 0 on reset.  not sure if this is necessary however

    bUSBAddress = 0; // so I don't accidentally set the USB address after a zero-length packet is sent

    _usbConfiguration = 0; // make sure (un-configured)

    // TODO:  see if endpoint 0 needs to be re-done or not, perhaps just leaving it
    //        'as-is' may be MORE stable than re-doing it every! single! time!

#ifdef CDC_ENABLED
    CDC_Reset();
#endif // CDC_ENABLED

#ifdef HID_ENABLED
    HID_Reset();
#endif // HID_ENABLED

    InitEP(0,EP_TYPE_CONTROL,EP_SINGLE_64);  // clear queues, init endpoints

    // clear any 'stall' event this might cause (it can happen)
    *((volatile uint8_t *)&(USB_INTFLAGSACLR)) = USB_STALLIF_bm; // clears the bit

    goto the_end;
  }

  if(udint & USB_UNFIF_bm)
  {
    check_send_queue();  // getting an early start on this
  }

  if(udint & USB_OVFIF_bm)
  {
//    error_printP(F("OVF detected")); - note, seems to happen kinda often

    check_recv_queue();  // getting an early start on this
  }

  //  Start of Frame - happens every millisecond so we use it for TX and RX LED one-shot timing, too
  if(udint & USB_SOFIF_bm)
  {
    // every time I get a 'start of frame' I will assign 'true' to 'bSOF'.  If I
    // get a suspend/resume with no 'start of frame' in between, then I know that
    // the USB is no longer connected, and I can shut it down.

    bSOF = true;    

#ifdef TX_RX_LED_INIT
    // check whether the one-shot period has elapsed.  if so, turn off the LED
    if(TxLEDPulse && !(--TxLEDPulse))
    {
      TXLED0(); // LED off - macro must be defined in variants 'pins_arduino.h'
    }
    if (RxLEDPulse && !(--RxLEDPulse))
    {
      RXLED0(); // LED off - macro must be defined in variants 'pins_arduino.h'
    }
#endif // TX_RX_LED_INIT

//    // if anything needs to be sent or received, make it happen NOW
//
//    check_send_queue(); // check SEND queue first
//    check_recv_queue(); // check this too, just in case, probably nothing

    if(_usbConfiguration) // am I 'configured' ?
    {
#ifdef CDC_ENABLED
      CDC_FrameReceived(); // callback for housekeeping things
#endif // CDC_ENABLED
    }
  }

the_end:
  // if any other flags were set during the ISR, I should get another interrupt

  *((volatile uint8_t *)&(USB_INTFLAGSACLR)) = udint; // clear whatever flags I processed this time

  return; // warning avoidance
}

//  Transaction Complete
ISR(USB_TRNCOMPL_vect)
{
uint8_t udint;


  udint = *((volatile uint8_t *)&(USB_INTFLAGSBCLR));

  // since I never know exactly why I was interrupted, I'll check all of the queues
  // to see if anything was sent/received, then process it.

  check_recv_queue(); // NOTE:  this dispatches received packets for me
  check_send_queue();

  *((volatile uint8_t *)&(USB_INTFLAGSBCLR)) = udint; //  now it's safe to clear interrupt flags
    // NOTE:  only the 2 lower bits of INTFLAGSB are actually used

  return; // warning avoidance
}


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//   __  __  _      _         _                    _   _   _  _    _  _        //
//  |  \/  |(_)  __| |       | |  ___ __   __ ___ | | | | | || |_ (_)| | ___   //
//  | |\/| || | / _` | _____ | | / _ \\ \ / // _ \| | | | | || __|| || |/ __|  //
//  | |  | || || (_| ||_____|| ||  __/ \ V /|  __/| | | |_| || |_ | || |\__ \  //
//  |_|  |_||_| \__,_|       |_| \___|  \_/  \___||_|  \___/  \__||_||_||___/  //
//                                                                             //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

// mostly OLD CODE that needs to be adapted, or eliminated

//==================================================================
//==================================================================

//  Handle CLASS_INTERFACE requests
static bool ClassInterfaceRequest(Setup& rSetup)
{
  u8 i = rSetup.wIndex;

#ifdef CDC_ENABLED
  if (CDC_ACM_INTERFACE == i)
  {
    error_printP(F("ClassInterfaceRequest - return CDC_Setup()"));
    return CDC_Setup(rSetup);
  }
#endif

#ifdef HID_ENABLED
  if (HID_INTERFACE == i)
  {
    error_printP(F("ClassInterfaceRequest - return HID_Setup()"));
    return HID_Setup(rSetup);
  }
#endif

  error_printP(F("ClassInterfaceRequest - return FALSE"));

  return false;
}


//  Construct a dynamic configuration descriptor
//  TODO:  This really needs dynamic endpoint allocation etc

static bool SendConfiguration(int maxlen)
{
int interfaces, total;

  error_printP_(F("USB SendConfiguration - maxlen="));
  error_printL(maxlen);

  //  Count and measure interfaces
  // to do this properly, I'll allocate a packet, build it, and then
  // add it to the send queue.  MUCH better!

  interfaces = 0;
  total = 0;

#ifdef CDC_ENABLED
  interfaces += CDC_GetNumInterfaces();
  total += CDC_GetInterfaceDataLength();
#endif // CDC_ENABLED

#ifdef HID_ENABLED
  interfaces += HID_GetNumInterfaces();
  total += HID_GetInterfaceDataLength();
#endif // HID_ENABLED

  // NOTE:  'total' says how long it's supposed to be.  the next request will get ALL of it
  //        when 'maxlen' is only big enough for ConfigDescriptor.  SO, I _must_ indicate
  //        the TRUE size of the response, but only send what I can.

  ConfigDescriptor config = D_CONFIG(total + sizeof(ConfigDescriptor),interfaces);

  // this default config assignment consists of the following information:
  //
  // clen              total + sizeof(ConfigDescriptor)       total length of all info
  // numInterfaces     interfaces                             total number of interfaces
  // config            1                                      config number to assign
  // iconfig           0                                      string description for config
  // attributes        USB_CONFIG_BUS_POWERED                 attributes of config
  // maxPower          USB_CONFIG_POWER_MA(500)               max bus power

  // if I want a string index for my description I must assign it manually
  config.iconfig = USB_STRING_INDEX_CONFIG;

  if(maxlen <= (int)sizeof(ConfigDescriptor))
  {
    USB_SendControl(0, &config, sizeof(ConfigDescriptor)); // just 'config'
  }
  else
  {
    internal_send(0, &config, sizeof(ConfigDescriptor), 0); // 9 bytes into the output buffer

    // follow this with the REST of the descriptor
#ifdef CDC_ENABLED
    CDC_SendInterfaceData(); // TODO:  allow me to send without marking 'send now'
#endif // CDC_ENABLED
#ifdef HID_ENABLED
    HID_SendInterfaceData();
#endif // HID_ENABLED
  }

  return true;
}


// NOTE this function does *NOT* do what it says on the tin...

static bool SendDescriptor(Setup& rSetup)
{
u8 t;
bool bRval;

  error_printP(F("USB SendDescriptor"));

//#ifdef LED_SIGNAL1
//  digitalWrite(LED_SIGNAL1,digitalRead(LED_SIGNAL1) == LOW ? HIGH : LOW);
//#endif // LED_SIGNAL1

  t = rSetup.wValueH;
  if (USB_CONFIGURATION_DESCRIPTOR_TYPE == t) // 2
  {
    error_printP_(F("Send Configuration [descriptor type] - length="));
    error_printL(rSetup.wLength);

    bRval = SendConfiguration(rSetup.wLength);

    return bRval;
  }

#ifdef HID_ENABLED
  else if (HID_REPORT_DESCRIPTOR_TYPE == t) // 0x22
  {
    error_printP(F("HID Get Descriptor [HID REPORT descriptor type]"));

    return HID_GetDescriptor(rSetup.wLength);
  }
#endif

  u8 desc_length = 0;
  const u8* desc_addr = 0;
  if (USB_DEVICE_DESCRIPTOR_TYPE == t) // 1
  {
    if (rSetup.wLength == 8) // this actually happens - it expects 8 bytes - this differentiates IAD vs Descriptor
    {
#ifdef CDC_ENABLED
      return CDC_SendIAD(); // send the 8-byte IAD thingy
#else
      // TODO:  something??

      return false; // it will fail anyway
#endif // CDC_ENABLED
    }
    else if (rSetup.wLength == 9) // experiment
    {
#ifdef CDC_ENABLED
#ifdef HID_ENABLED // both HID and CDC
      desc_addr = (const u8 *)&USB_DeviceDescriptorB; // this one requires 'association type'
#else // !HID_ENABLED
      return CDC_SendDeviceDescriptor(); // send the 9-byte device descriptor
#endif // HID_ENABLED
#endif // CDC_ENABLED
    }
    else
    {
      error_printP_(F("Device Descriptor Type - length was "));
      error_printL(rSetup.wLength);
    }

    desc_addr = (const u8*)&USB_DeviceDescriptor; // always
  }
  else if (USB_DEVICE_QUALIFIER_TYPE == t) // 6
  {
//    error_printP(F("Device Qualifier Type - a USB 2 request"));
    // according to available documentation, it's a USB 2.0 request that wants to know how
    // the device will behave in high speed mode (vs full speed mode).  Since I don't support
    // high speed mode, I'll 'nack' it with a zero length packet.

    // https://msdn.microsoft.com/en-us/library/windows/hardware/ff539288%28v=vs.85%29.aspx

    internal_send0(0); // ZLP which keeps host from waiting
    return true; // for now indicate "it worked"
  }
  else if (USB_DEVICE_DEBUG_TYPE == t) // 10
  {
    error_printP(F("Device DEBUG Type - a USB 2 request?"));

    internal_send0(0); // ZLP which keeps host from waiting
    return true; // for now indicate "it worked"
  }
  else if (USB_INTERFACE_ASSOCIATION_TYPE == t) // 11
  {
#ifdef CDC_ENABLED
    return CDC_SendIAD(); // send the 8-byte IAD thingy in response
#else // !CDC_ENABLED
    internal_send0(0); // ZLP which keeps host from waiting
    return true; // for now indicate "it worked"
#endif // CDC_ENABLED
  }
  else if (USB_STRING_DESCRIPTOR_TYPE == t) // 3
  {
    if(rSetup.wValueL == USB_STRING_INDEX_LANGUAGE)
    {
      desc_addr = (const u8*) USB_STRING_LANGUAGE; //&(USB_STRING_LANGUAGE[0]);
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_PRODUCT)
    {
      desc_addr = (const u8 *) USB_STRING_PRODUCT; // &(USB_STRING_PRODUCT[0]);
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_MANUFACTURER)
    {
      desc_addr = (const u8 *) USB_STRING_MANUFACTURER; // &(USB_STRING_MANUFACTURER[0]);
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_DESCRIPTION)
    {
      static const wchar_t szStr[] PROGMEM = L"\x0346" L"XMegaForArduino USB implementation"; // len=34

      desc_addr = (const u8 *)szStr;
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_VERSION)
    {
      static const wchar_t szStr[] PROGMEM = L"\x0310" L"1.00.00"; // len=7

      desc_addr = (const u8 *)szStr;
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_URL)
    {
      static const wchar_t szStr[] PROGMEM = L"\x0356" L"http://github.com/XMegaForArduino/arduino/"; // len=42

      desc_addr = (const u8 *)szStr;
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_CONFIG)
    {
      static const wchar_t szStr[] PROGMEM = L"\x034c" L"Default XMegaForArduino Configuration"; // len=37

      desc_addr = (const u8 *)szStr;
    }
    else if(rSetup.wValueL == USB_STRING_INDEX_SERIAL)
    {
      wchar_t tbuf[22]; // 7 hex digits with ':' between, plus lead byte + 1 extra
      short i1, i2;

      // build the serial # string (20 wide chars)
      // use ascii/unicode basic values of 0-255 only for simplicity

      for(i1=0, i2=1; i1 < 7; i1++)
      {
        char c1;
        uint8_t b1;

        // use the CPU's unique signature information to generate a serial number
        if(i1 < 6)
        {
          b1 = readCalibrationData(8 + i1); // 8 is 'LOTNUM0'; there are 6 of them
        }
        else
        {
          b1 = readCalibrationData(0x10); // WAFNUM is offset 10H
        }

        if(i1)
        {
          tbuf[i2++] = ':';
        }

        c1 = (b1 >> 4) & 0xf;
        if(c1 < 10)
        {
          c1 += '0';
        }
        else
        {
          c1 += 'A' - 10;
        }

        tbuf[i2++] = c1;

        c1 = b1 & 0xf;
        if(c1 < 10)
        {
          c1 += '0';
        }
        else
        {
          c1 += 'A' - 10;
        }

        tbuf[i2++] = c1;
      }

      // now the lead-in byte, 300H + length (in bytes)
      i2 *= sizeof(wchar_t); // the actual length now in i2

      tbuf[0] = 0x300 + i2;

      USB_SendControl(0, (const u8 *)tbuf, i2);
      return true;
    }

    // TODO:  others?
    else
    {
      error_printP_(F("SendDescriptor - bad setup index "));
      error_printL(rSetup.wValueL);

      return false;
    }
  }
  else
  {
    error_printP_(F("*** Unknown Descriptor Type - "));
    error_printL(t);

    return false;
  }

  if(desc_addr == 0)
  {
    error_printP(F("SendDescriptor - zero pointer"));

    return false;
  }
  else if(desc_length == 0)
  {
    desc_length = pgm_read_byte(desc_addr); // first byte of the descriptor, always
  }

  USB_SendControlP(TRANSFER_PGM, desc_addr, desc_length);

  return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                            //
//   _   _  ____   ____   ____                _                          _                    //
//  | | | |/ ___| | __ ) |  _ \   ___ __   __(_)  ___  ___          ___ | |  __ _  ___  ___   //
//  | | | |\___ \ |  _ \ | | | | / _ \\ \ / /| | / __|/ _ \        / __|| | / _` |/ __|/ __|  //
//  | |_| | ___) || |_) || |_| ||  __/ \ V / | || (__|  __/       | (__ | || (_| |\__ \\__ \  //
//   \___/ |____/ |____/ |____/  \___|  \_/  |_| \___|\___|_____   \___||_| \__,_||___/|___/  //
//                                                        |_____|                             //
//                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////

USBDevice_ USBDevice;

USBDevice_::USBDevice_()
{
}

void USBDevice_::attach()
{
//  error_printP(F("USB - attach"));

  uint8_t oldSREG = SREG; // save int flag
  cli(); // no interrupt handling until I'm done setting this up

  USB_INTCTRLA = 0;
  USB_INTCTRLB = 0;

  USB_INTFLAGSACLR = 0xff; // clear all int flags
  USB_INTFLAGSBCLR = 0x3;  // clear all int flags

  _usbConfiguration = 0;

  // enable the USB clock using the 32mhz RC oscillator
  // assume either slow (6mhz) or fast (48mhz)
  // and of course the pre-scaler must be assigned accordingly
  // Also, assume that the oscillator is *SET UP* *PROPERLY* already, and
  // that all I have to do is configure the PLL to run at 48Mhz

  // setting up the PLL - source is usually RC32M 'divided by 4' then multiplied by 6 for 48Mhz

  USB_CTRLA = 0; // shut down USB
  USB_CTRLB = 0; // detach D- and D+

  CLK_USBCTRL = 0; // shut off USB clock - section 7.9.5 in AU manual

  OSC_CTRL &= ~(OSC_PLLEN_bm); // disable PLL osc - section 7.9.1 in AU manual

#ifdef USE_RC2M // am I using the 2Mhz oscillator for the USB instead of 32Mhz?

  OSC_CTRL |= OSC_RC2MEN_bm;     // enable 2M osc

  while(!(OSC_STATUS & OSC_RC2MRDY_bm)) // wait for 2M RC osc to be 'ready'
  {
    // TODO:  timeout?  I must wait until the osc is stable
  }

  // now config PLL and USB clock stuff

  // 2Mhz as the source, multiplicatino factor of 24 = 48Mhz
  OSC_PLLCTRL = OSC_PLLSRC_RC2M_gc | 24; // 24 times the 2Mhz frequency

  // TODO:  set up the calibration PLL for 2Mhz ?

#else // !USE_RC2M - I am using the 32Mhz oscillator with the PLL - this is normal

  // 32Mhz (divided by 4, so it's 8Mhz) as the source
  // multiplication factor of 6 - result = 48Mhz
  OSC_PLLCTRL = OSC_PLLSRC_RC32M_gc | 6; // 6 times the 8Mhz frequency - section 7.10.6 in AU manual

#endif // USE_RC2M

  OSC_CTRL |= OSC_PLLEN_bm; // re-enable PLL - section 7.9.5 in AU manual

  while(!(OSC_STATUS & OSC_PLLRDY_bm)) // wait for PLL to be 'ready'
  {
    // TODO:  timeout?  I need to wait until it's stable
  }


#ifdef FAST_USB /* note this is 12Mbit operation, 'FULL' speed, not 'HIGH' speed 480Mbit */
  CLK_USBCTRL = CLK_USBSRC_PLL_gc; // use PLL (divide by 1, no division) - section 7.9.5 in AU manual

#else // SLOW
  CLK_USBCTRL = CLK_USBSRC_PLL_gc  // use PLL - section 7.9.5 in AU manual
              | CLK_USBPSDIV_8_gc; // divide by 8 for 6mhz operation (12Mhz?  see 7.3.6 which says 12Mhz or 48Mhz)

#endif // FAST_USB or SLOW

  CLK_USBCTRL |= CLK_USBEN_bm;     // enable bit - section 7.9.5 in AU manual


  // assign CAL register from product signatures (4.17.17,18)
  USB_CAL0 = readCalibrationData((uint8_t)(uint16_t)&PRODSIGNATURES_USBCAL0); // docs say 'CALL'
  USB_CAL1 = readCalibrationData((uint8_t)(uint16_t)&PRODSIGNATURES_USBCAL1); // docs say 'CALH'

  // set the max # of endpoints, speed, and 'store frame number' flags
  USB_CTRLA = MAXEP // max # of endpoints minus 1 - section 20.14.1 in AU manual
#ifdef FAST_USB
            | USB_SPEED_bm       // FAST USB - all ahead 'FULL' - aka 'FULL' speed ahead! - ok bad PUNishment
#endif // FAST_USB
            | USB_STFRNUM_bm     // store the frame number (mostly for debugging)
   // TODO:  FIFO ?
            ;

  init_buffers_and_endpoints(); // initialize everything in RAM registers, basically

  USB_EPPTR = endpoint_data_pointer(); // assign the data pointer to the RAM registers - section 20.14.7,8
  // NOTE:  the xmega USB implementation puts most of the 'register' info for USB into RAM

  USB_ADDR = 0; // set USB address to 0 (before doing the first 'SETUP' request)

  // LAST of all, enable interrupts
  USB_INTCTRLB = USB_TRNIE_bm | USB_SETUPIE_bm;    // enable 'transaction complete' and 'setup' interrupts
  USB_INTCTRLA  = USB_SOFIE_bm                     // enable the start of frame interrupt
                | USB_BUSEVIE_bm                   // 'bus event' interrupt - suspend, resume, reset
                                                   // for the RESET event, RESETIF will be set (20.14.11 in AU manual)
                | USB_INTLVL1_bm;                  // int level 2 (deliberately lower than serial port, SPI, I2C)
//                | USB_INTLVL0_bm | USB_INTLVL1_bm; // int level 3 (for if there are performance problems with int level 2)


  // NOW enable the USB
  USB_CTRLA |= USB_ENABLE_bm; // and, we're UP and RUNNING!  section 20.14.1 in AU manual

  SREG = oldSREG; // restore int flags

  // attach the wiring for D- and D+ AFTER everything else.

  USB_CTRLB = USB_ATTACH_bm; // attach D- and D+ (also enables pullup resistors based on speed)

  // on A1U and A4U devices, this is PD6 (D-) and PD7 (D+) [YMMV on the other processors]
  // this is partly why it's good to use PORTC for the primary SPI, etc. since PORTD
  // gets used for 'other things' (like USB).  Default serial port on PORTD is still ok.


#ifdef TX_RX_LED_INIT
  TX_RX_LED_INIT(); // macro must be defined in variants 'pins_arduino.h' for the USB activity lights
#endif // TX_RX_LED_INIT

  error_printP(F("USB Attach (done)"));

  // NOTE:  at THIS point the ISR will manage everything else.  The USB will receive
  //        messages to set up the connection, etc. and the ISR will drive it.
}

void USBDevice_::detach()
{
  error_printP(F("USB - detach"));

  uint8_t oldSREG = SREG; // save int flag
  cli(); // no interrupt handling until I'm done setting this up

  USB_INTCTRLA = 0; // disabling interrupts
  USB_INTCTRLB = 0;
  USB_CTRLB = 0; // detach D- and D+
  USB_CTRLA = 0; // shut down USB
  CLK_USBCTRL = 0; // shut off USB clock

  init_buffers_and_endpoints(); // re-initialize these

  _usbConfiguration = 0;

  SREG = oldSREG; // restore int flags

  error_printP(F("USB Detach (done)"));
}

// added this for access to USB device structures
XMegaEPDataStruct *USBDevice_::GetEPData()
{
  return &epData;
}

//  Check for interrupts
//  TODO: VBUS detection
bool USBDevice_::configured()
{
  return _usbConfiguration;
}

void USBDevice_::poll()
{
}

#endif /* if defined(USBCON) */

