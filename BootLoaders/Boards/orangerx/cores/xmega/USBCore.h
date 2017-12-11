//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//           _   _  ____   ____    ____                      _              //
//          | | | |/ ___| | __ )  / ___| ___   _ __  ___    | |__           //
//          | | | |\___ \ |  _ \ | |    / _ \ | '__|/ _ \   | '_ \          //
//          | |_| | ___) || |_) || |___| (_) || |  |  __/ _ | | | |         //
//           \___/ |____/ |____/  \____|\___/ |_|   \___|(_)|_| |_|         //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// Copyright (c) 2010, Peter Barrett
/*
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

#ifndef __USBCORE_H__
#define __USBCORE_H__

// USB implementation MUST have USB_VID and USB_PID defined - can be done in 'boards.txt'
#if defined(USBCON) /* TODO add others here, things that require VID and PID for USB */
#ifdef USB_VID
#if USB_VID==null
#error cannot work with NULL value for VID
#endif // USB_VID==null
#else
#error must define USB_VID
#endif // USB_VID

#ifdef USB_PID
#if USB_PID==null
#error cannot work with NULL value for PID
#endif // USB_PID==null
#else
#error must define USB_PID
#endif // USB_PID
#endif // defined(USBCON)


#define FAST_USB /* necessary for 'FULL' speed operation - this is 12Mbit, not 480Mbit USB 2 'HIGH' speed - 'LOW' may not work properly */

#ifndef _PLATFORM_H_TYPES_DEFINED_ /* TEMPORARY FIX, SEE Platform.h */
#define _PLATFORM_H_TYPES_DEFINED_
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#endif // _PLATFORM_H_TYPES_DEFINED_

//  Standard requests
#define GET_STATUS         0
#define CLEAR_FEATURE      1
#define SET_FEATURE        3
#define SET_ADDRESS        5
#define GET_DESCRIPTOR     6
#define SET_DESCRIPTOR     7
#define GET_CONFIGURATION  8
#define SET_CONFIGURATION  9
#define GET_INTERFACE     10
#define SET_INTERFACE     11


// bmRequestType
#define REQUEST_HOSTTODEVICE  0x00
#define REQUEST_DEVICETOHOST  0x80 /* device to host, i.e. 'in' - when not set, it's 'out' (host to device) */
#define REQUEST_DIRECTION     0x80

#define REQUEST_STANDARD      0x00
#define REQUEST_CLASS         0x20
#define REQUEST_VENDOR        0x40
#define REQUEST_TYPE          0x60

#define REQUEST_DEVICE        0x00
#define REQUEST_INTERFACE     0x01
#define REQUEST_ENDPOINT      0x02
#define REQUEST_OTHER         0x03
#define REQUEST_RECIPIENT     0x03

#define REQUEST_DEVICETOHOST_CLASS_INTERFACE  (REQUEST_DEVICETOHOST + REQUEST_CLASS + REQUEST_INTERFACE)
#define REQUEST_HOSTTODEVICE_CLASS_INTERFACE  (REQUEST_HOSTTODEVICE + REQUEST_CLASS + REQUEST_INTERFACE)

//  Class requests

#define CDC_SET_LINE_CODING         0x20
#define CDC_GET_LINE_CODING         0x21
#define CDC_SET_CONTROL_LINE_STATE  0x22

#define MSC_RESET              0xFF
#define MSC_GET_MAX_LUN        0xFE

#define HID_GET_REPORT        0x01
#define HID_GET_IDLE          0x02
#define HID_GET_PROTOCOL      0x03
#define HID_SET_REPORT        0x09
#define HID_SET_IDLE          0x0A
#define HID_SET_PROTOCOL      0x0B

//  Descriptors

#define USB_DEVICE_DESC_SIZE                   18
#define USB_CONFIGUARTION_DESC_SIZE             9
#define USB_INTERFACE_DESC_SIZE                 9
#define USB_ENDPOINT_DESC_SIZE                  7


#define USB_DEVICE_DESCRIPTOR_TYPE              1
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       2
#define USB_STRING_DESCRIPTOR_TYPE              3
#define USB_INTERFACE_DESCRIPTOR_TYPE           4
#define USB_ENDPOINT_DESCRIPTOR_TYPE            5
#define USB_DEVICE_QUALIFIER_TYPE               6
#define USB_OTHER_SPEED_CONFIGURATION_TYPE      7
#define USB_INTERFACE_POWER_TYPE                8
#define USB_OTG_TYPE                            9
#define USB_DEVICE_DEBUG_TYPE                  10
#define USB_INTERFACE_ASSOCIATION_TYPE         11 /* see InterfaceAssociationDescriptor_ecn.pdf */
// NOTE:  for USB_INTERFACE_ASSOCIATION_TYPE respond with IADDescriptor

#define USB_DEVICE_CLASS_ZERO                   0x00
#define USB_DEVICE_CLASS_COMMUNICATIONS         0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE        0x03
#define USB_DEVICE_CLASS_STORAGE                0x08
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC        0xFF

#define USB_CONFIG_POWERED_MASK                 0x40
#define USB_CONFIG_BUS_POWERED                  0x80
#define USB_CONFIG_SELF_POWERED                 0xC0
#define USB_CONFIG_REMOTE_WAKEUP                0x20

// bMaxPower in Configuration Descriptor
#define USB_CONFIG_POWER_MA(mA)                 ((mA)/2)

// bEndpointAddress in Endpoint Descriptor
#define USB_ENDPOINT_DIRECTION_MASK             0x80
#define USB_ENDPOINT_OUT(addr)                  ((addr) | 0x00)
#define USB_ENDPOINT_IN(addr)                   ((addr) | 0x80)

#define USB_ENDPOINT_TYPE_MASK                  0x03
#define USB_ENDPOINT_TYPE_CONTROL               0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS           0x01
#define USB_ENDPOINT_TYPE_BULK                  0x02
#define USB_ENDPOINT_TYPE_INTERRUPT             0x03

#define TOBYTES(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)

#define CDC_V1_10                               0x0110
#define CDC_COMMUNICATION_INTERFACE_CLASS       0x02

#define CDC_CALL_MANAGEMENT                     0x01
#define CDC_ABSTRACT_CONTROL_MODEL              0x02
#define CDC_HEADER                              0x00
#define CDC_ABSTRACT_CONTROL_MANAGEMENT         0x02
#define CDC_UNION                               0x06
#define CDC_CS_INTERFACE                        0x24
#define CDC_CS_ENDPOINT                         0x25
#define CDC_DATA_INTERFACE_CLASS                0x0A

#define MSC_SUBCLASS_SCSI                       0x06
#define MSC_PROTOCOL_BULK_ONLY                  0x50

#define HID_HID_DESCRIPTOR_TYPE                 0x21
#define HID_REPORT_DESCRIPTOR_TYPE              0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE            0x23

// CONTROL LINE STATE and SERIAL STATE - BIT VALUES  (16-bits, LE)
// USED BY 'SET CONTROL LINE STATE' and 'SERIAL STATE' FOR CAM INTERRUPT 'IN'
#define CONTROL_LINE_STATE_DTR                  0x01 /* data terminal ready [ok to send on IN] */
#define CONTROL_LINE_STATE_RTS                  0x02 /* ready to send [has data ready for OUT] */
#define SERIAL_STATE_RX_CARRIER_DCD             0x01 /* receive carrier detect - on to allow receive */
#define SERIAL_STATE_TX_CARRIER_DSR             0x02 /* data set ready - inform host I have data */
#define SERIAL_STATE_BREAK_DETECT               0x04 /* 'break' detect */
#define SERIAL_STATE_RING_DETECT                0x08 /* 'ring' detect */
#define SERIAL_STATE_FRAMING_ERROR              0x10 /* framing error */
#define SERIAL_STATE_PARITY_ERROR               0x20 /* parity error */
#define SERIAL_STATE_OVERRUN                    0x40 /* overrun input (data lost) */


// A1U series needs 16-byte alignment for endpoint structure
#if defined(__AVR_ATxmega64A1U__) || defined(__AVR_ATxmega128A1U__)
#define A1U_SERIES
#endif // A1U

//  Device
typedef struct _device_descriptor_
{
  u8 len;             // 18
  u8 dtype;           // 1 USB_DEVICE_DESCRIPTOR_TYPE
  u16 usbVersion;     // 0x200
  u8  deviceClass;
  u8  deviceSubClass;
  u8  deviceProtocol;
  u8  packetSize0;    // Packet 0
  u16  idVendor;
  u16  idProduct;
  u16  deviceVersion; // 0x100
  u8  iManufacturer;
  u8  iProduct;
  u8  iSerialNumber;
  u8  bNumConfigurations;
} DeviceDescriptor;

//  Config
typedef struct _config_descriptor_
{
  u8  len;            // 9
  u8  dtype;          // 2
  u16 clen;           // total length
  u8  numInterfaces;
  u8  config;
  u8  iconfig;
  u8  attributes;
  u8  maxPower;
} ConfigDescriptor;

//  String

//  Interface
typedef struct _interface_descriptor_
{
  u8 len;                // 9
  u8 dtype;              // 4
  u8 number;
  u8 alternate;
  u8 numEndpoints;
  u8 interfaceClass;
  u8 interfaceSubClass;
  u8 protocol;
  u8 iInterface;
} InterfaceDescriptor;

//  Endpoint
typedef struct _endpoint_descriptor_
{
  u8 len;    // 7
  u8 dtype;  // 5
  u8 addr;
  u8 attr;
  u16 packetSize;
  u8 interval;
} EndpointDescriptor;

// Interface Association Descriptor
// Used to bind 2 interfaces together in CDC compostite device
typedef struct _iad_descriptor_
{
  u8 len;              // 8
  u8 dtype;            // 11
  u8 firstInterface;
  u8 interfaceCount;
  u8 functionClass;
  u8 funtionSubClass;
  u8 functionProtocol;
  u8 iInterface;
} IADDescriptor;

//  CDC CS interface descriptor
typedef struct _cdccs_interface_descriptor_
{
  u8 len;    // 5
  u8 dtype;  // 0x24
  u8 subtype;
  u8 d0;
  u8 d1;
} CDCCSInterfaceDescriptor;

typedef struct _cdccs_interface_descriptor4_
{
  u8 len;    // 4
  u8 dtype;  // 0x24
  u8 subtype;
  u8 d0;
} CDCCSInterfaceDescriptor4;

typedef struct  _cm_functional_descriptor_
{
  u8  len;             // 5
  u8   dtype;          // 0x24
  u8   subtype;        // 1
  u8   bmCapabilities;
  u8   bDataInterface;
} CMFunctionalDescriptor;

typedef struct  _acm_functional_descriptor_
{
  u8  len;
  u8   dtype;    // 0x24
  u8   subtype;  // 1
  u8   bmCapabilities;
} ACMFunctionalDescriptor;

typedef struct _cdc_descriptor_
{
//  // IAD
//  IADDescriptor             iad;  // for complex endpoints (apparently not critical)

  //  Control
  InterfaceDescriptor       cif;  //
  CDCCSInterfaceDescriptor  header;
//  CMFunctionalDescriptor    callManagement;      // Call Management
  ACMFunctionalDescriptor   controlManagement;    // ACM
  CDCCSInterfaceDescriptor  functionalDescriptor;  // CDC_UNION
  EndpointDescriptor        cifin;

  //  Data
  InterfaceDescriptor       dif;
  EndpointDescriptor        in;
  EndpointDescriptor        out;
} CDCDescriptor;

typedef struct _msc_descriptor_
{
  InterfaceDescriptor       msc;
  EndpointDescriptor        in;
  EndpointDescriptor        out;
} MSCDescriptor;

typedef struct _hid_desc_descriptor_
{
  u8 len;        // 9
  u8 dtype;      // 0x21
  u8 addr;
  u8  versionL;  // 0x101
  u8  versionH;  // 0x101
  u8  country;
  u8  desctype;  // 0x22 report
  u8  descLenL;
  u8  descLenH;
} HIDDescDescriptor;

typedef struct _hid_descriptor_
{
  InterfaceDescriptor       hid;
  HIDDescDescriptor         desc;
  EndpointDescriptor        in;
} HIDDescriptor;


// XMEGA STRUCTURES

#define MAXEP 5 /*15*/ /* 4 bit value, 0-15 - see 20.14.1 'CTRLA register' in AU manual */
// I assigned this to '5' because there are only 5 endpoints being used by THIS code.
// The 'mega' code could only have a total of 6 and 0 was always the 'control' endpoint
// and setting a 'smaller-than-maximum' value for XMEGA burns up less RAM.
// If this poses a problem I _could_ malloc the structures, but to be effective, it would
// have to require a reboot to change it, or you'd get major RAM fragmentation
//
// TODO:  put MAXEP into pins_arduino.h and override if too small (or not defined already)

typedef union _xmega_fifo_entry_
{
  struct
  {
    uint8_t h, l;
  };
  uint16_t heW; // high endian word
} XMegaFIFOEntry;

typedef struct _xmega_fifo_
{
  XMegaFIFOEntry in;
  XMegaFIFOEntry out;
} XMegaFIFO;

// see AU manual, pg 231 section 20.4
typedef struct _xmega_endpoint_descriptor_
{
  volatile uint8_t status;
  volatile uint8_t ctrl;
  volatile uint16_t /*HLByteWord*/ cnt; // so, cnt.w is the 16-bit value, cnt.l and cnt.h the 8-bit low/high
                           // it is the 'data count' value.  it may be zero.
                           // only bits 1:0 of cnt.h are valid.  cnt.w should be 'and'd with 0x3ff
                           // the high bit of cnt.w and cnt.h is the 'AZLP' (auto zero length packet) bit
                           // see AU manual section 20.15.4
  volatile uint16_t /*HLByteWord*/ dataptr; // pointer to data buffer.  max packet length assigned to CTRL [1:0] or [2:0]
                               // see table 20-5 in AU manual for max packet length.  may need 2 buffers "that long"
  volatile uint16_t /*HLByteWord*/ auxdata; // used for multi-packet transfers
} XMegaEndpointDescriptor; // NOTE:  2 per channel (one 'out', one 'in') pointed by EPPTR

typedef struct _xmega_endpoint_channel_
{
  XMegaEndpointDescriptor out;
  XMegaEndpointDescriptor in;
} XMegaEndpointChannel
#ifdef A1U_SERIES
 __attribute (( aligned (16) ));
#else // not an A1U series
 __attribute__ (( aligned (2) /*, packed*/));
#endif // A1U or not

// also section 20.4 in AU manual
typedef struct _XMegaEPDataStruct_
{
#ifdef A1U_SERIES
#if ((MAXEP+1)%4) != 0 // A1U needs 16-byte boundary, not merely word-aligned
  uint8_t padding[sizeof(XMegaFIFO) * (4 - ((MAXEP+1)%4))]; // this should 16-byte align 'endpoint' as required
#endif // needs padding
#endif // XMEGA A1U series
  XMegaFIFO               fifo[MAXEP + 1];
  XMegaEndpointChannel    endpoint[MAXEP + 1];  // point EPPTR to THIS  (must be word boundary)
  volatile uint16_t       framenum;             // 1 word frame number
} XMegaEPDataStruct /*__attribute__ ((packed))*/;   // note:  point EPPTR to &endpoint[0], word alignment needed

#ifdef DEBUG_CODE
uint16_t endpoint_data_pointer(void);
#define EP_DATA_STRUCT() ((XMegaEPDataStruct *)(endpoint_data_pointer() - (uint16_t)&(((XMegaEPDataStruct *)0)->endpoint[0])))
#endif // DEBUG_CODE

// this is the USB Device Descriptor
#define D_DEVICE(_class,_subClass,_proto,_packetSize0,_vid,_pid,_version,_im,_ip,_is,_configs) \
  { 18, 1, 0x200, _class,_subClass,_proto,_packetSize0,_vid,_pid,_version,_im,_ip,_is,_configs }

#define D_CONFIG(_totalLength,_interfaces) \
  { 9, 2, _totalLength,_interfaces, 1, 0, USB_CONFIG_BUS_POWERED, USB_CONFIG_POWER_MA(500) }

#define D_INTERFACE(_n,_numEndpoints,_class,_subClass,_protocol) \
  { 9, 4, _n, 0, _numEndpoints, _class,_subClass, _protocol, 0 }

#define D_ENDPOINT(_addr,_attr,_packetSize, _interval) \
  { 7, 5, _addr,_attr,_packetSize, _interval }

#define D_IAD(_firstInterface, _count, _class, _subClass, _protocol) \
  { 8, 11, _firstInterface, _count, _class, _subClass, _protocol, 0 }

#define D_HIDREPORT(_descriptorLength) \
  { 9, 0x21, 0x1, 0x1, 0, 1, 0x22, _descriptorLength, 0 }

#define D_CDCCS(_subtype,_d0,_d1)  { 5, 0x24, _subtype, _d0, _d1 }
#define D_CDCCS4(_subtype,_d0)    { 4, 0x24, _subtype, _d0 }


#endif

