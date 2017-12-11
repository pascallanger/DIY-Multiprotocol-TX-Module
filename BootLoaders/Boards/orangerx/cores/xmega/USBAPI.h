//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//              _   _  ____   ____     _            _     _                 //
//             | | | |/ ___| | __ )   / \    _ __  (_)   | |__              //
//             | | | |\___ \ |  _ \  / _ \  | '_ \ | |   | '_ \             //
//             | |_| | ___) || |_) |/ ___ \ | |_) || | _ | | | |            //
//              \___/ |____/ |____//_/   \_\| .__/ |_|(_)|_| |_|            //
//                                          |_|                             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef __USBAPI__
#define __USBAPI__

#if defined(USBCON)

#include "USBCore.h" /* make sure since I use its definitions here */

#ifdef DEBUG_CODE
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
extern void error_print(const char *p1); // TEMPORARY
extern void error_print_(const char *p1); // TEMPORARY
extern void error_printH(unsigned long); // TEMPORARY
extern void error_printH_(unsigned long); // TEMPORARY
extern void error_printL(unsigned long); // TEMPORARY
extern void error_printL_(unsigned long); // TEMPORARY
extern void error_printP(const void * /*PROGMEM*/ p1);  // TEMPORARY
extern void error_printP_(const void * /*PROGMEM*/ p1);  // TEMPORARY
extern void DumpHex(void *pBuf, uint8_t nBytes);

#ifdef __cplusplus
}
#endif // __cplusplus

#else // DEBUG_CODE

#define error_print(p1)
#define error_print_(p1)
#define error_printH(X)
#define error_printH_(X)
#define error_printL(X)
#define error_printL_(X)
#define error_printP(p1)
#define error_printP_(p1)
#define DumpHex(X,Y)

#endif // DEBUG_CODE


//================================================================================
//================================================================================
//  USB

class USBDevice_
{
public:
    USBDevice_();
    bool configured();

    void attach();
    void detach();  // Serial port goes down too...
    void poll();

protected:
  static XMegaEPDataStruct *GetEPData(); // necessary
};

extern USBDevice_ USBDevice;

//================================================================================
//================================================================================
//  Serial over CDC (Serial1 is the physical port)

class Serial_ : public Stream
{
protected:
    int peek_buffer;
public:
    Serial_() { peek_buffer = -1; };
    void begin(unsigned long);
    void begin(unsigned long, uint8_t);
    void end(void);

    virtual int available(void);
    virtual int peek(void);
    virtual int read(void);
    virtual void flush(void);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t*, size_t);

    using Print::write; // pull in write(str) etc. from Print

    operator bool();
};
extern Serial_ Serial; // NOTE: HardwareSerial.h defines the 1st port as 'Serial1' whenever USBCON defined

//================================================================================
//================================================================================
//  Mouse

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_ALL (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)

class Mouse_
{
protected:
    uint8_t _buttons;
    void buttons(uint8_t b);
public:
    Mouse_(void);
    void begin(void);
    void end(void);
    void click(uint8_t b = MOUSE_LEFT);
    void move(signed char x, signed char y, signed char wheel = 0);
    void press(uint8_t b = MOUSE_LEFT);     // press LEFT by default
    void release(uint8_t b = MOUSE_LEFT);   // release LEFT by default
    bool isPressed(uint8_t b = MOUSE_LEFT); // check LEFT by default
};
extern Mouse_ Mouse;

//================================================================================
//================================================================================
//  Keyboard

#define KEY_LEFT_CTRL       0x80
#define KEY_LEFT_SHIFT      0x81
#define KEY_LEFT_ALT        0x82
#define KEY_LEFT_GUI        0x83
#define KEY_RIGHT_CTRL      0x84
#define KEY_RIGHT_SHIFT     0x85
#define KEY_RIGHT_ALT       0x86
#define KEY_RIGHT_GUI       0x87

#define KEY_UP_ARROW        0xDA
#define KEY_DOWN_ARROW      0xD9
#define KEY_LEFT_ARROW      0xD8
#define KEY_RIGHT_ARROW     0xD7
#define KEY_BACKSPACE       0xB2
#define KEY_TAB             0xB3
#define KEY_RETURN          0xB0
#define KEY_ESC             0xB1
#define KEY_INSERT          0xD1
#define KEY_DELETE          0xD4
#define KEY_PAGE_UP         0xD3
#define KEY_PAGE_DOWN       0xD6
#define KEY_HOME            0xD2
#define KEY_END             0xD5
#define KEY_CAPS_LOCK       0xC1
#define KEY_F1              0xC2
#define KEY_F2              0xC3
#define KEY_F3              0xC4
#define KEY_F4              0xC5
#define KEY_F5              0xC6
#define KEY_F6              0xC7
#define KEY_F7              0xC8
#define KEY_F8              0xC9
#define KEY_F9              0xCA
#define KEY_F10             0xCB
#define KEY_F11             0xCC
#define KEY_F12             0xCD

//  Low level key report: up to 6 keys and shift, ctrl etc at once
typedef struct
{
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} KeyReport;

class Keyboard_ : public Print
{
protected:
    KeyReport _keyReport;
    void sendReport(KeyReport* keys);
public:
    Keyboard_(void);
    void begin(void);
    void end(void);
    virtual size_t write(uint8_t k);
    virtual size_t press(uint8_t k);
    virtual size_t release(uint8_t k);
    virtual void releaseAll(void);
};
extern Keyboard_ Keyboard;

//================================================================================
//================================================================================
//  Low level API

typedef struct
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint8_t wValueL;
    uint8_t wValueH;
    uint16_t wIndex;
    uint16_t wLength;
} Setup;

//================================================================================
//================================================================================
//  HID 'Driver'

int   HID_GetNumInterfaces(void);
int   HID_GetInterfaceDataLength(void);
int   HID_SendInterfaceData(void);
bool  HID_SendDeviceDescriptor(void);

int   HID_GetDescriptor(int i); // handles the 'GET DESCRIPTOR' control packet
bool  HID_Setup(Setup& setup);  // handles a 'SETUP' control packet

void  HID_SendReport(uint8_t id, const void* data, int len);

void  HID_Reset(void);          // called whenever I get a bus reset

//================================================================================
//================================================================================
//  CDC 'Driver'

bool  CDC_SendIAD(void);
int   CDC_GetNumInterfaces(void);
int   CDC_GetInterfaceDataLength(void);
int   CDC_SendInterfaceData(void);
bool  CDC_SendDeviceDescriptor(void);

int   CDC_GetDescriptor(int i); // handles the 'GET DESCRIPTOR' control packet
bool  CDC_Setup(Setup& setup);  // handles a 'SETUP' control packet
void  CDC_FrameReceived(void);  // call when frame is received and EP is configured
void  CDC_SendACM(void);        // call when you need to send a packet on the interrupt EP

void  CDC_Reset(void);          // called whenever I get a bus reset


//================================================================================
//================================================================================

#define TRANSFER_PGM        0x80
#define TRANSFER_RELEASE    0x40
#define TRANSFER_TOGGLE_ON  0x20 /* assign this to pre-set the 'toggle' bit on - only works when send queue is empty */
#define TRANSFER_TOGGLE_OFF 0x10 /* assign this to pre-set the 'toggle' bit off - only works when send queue is empty */

// NOTE:  USB_SendControl returns # of bytes sent, or 0x8000 if a ZLP is sent
//        it will return 0 on error, such as the inability to allocate a buffer
//        control packets send 64 bytes at a time, so the total size is limited
//        by the number of available buffers.

int USB_SendControl(uint8_t flags, const void* d, int len);
#ifdef PROGMEM
int USB_SendControlP(uint8_t flags, const void * PROGMEM d, int len);
// called internally if you use TRANSFER_PGM flag; you can also call this directly
#endif // PROGMEM

uint16_t USB_Available(uint8_t ep);        // returns # of bytes in the buffer on an OUT or CONTROL endpoint
uint16_t USB_SendQLength(uint8_t ep);      // returns # of buffers in the send queue for an IN or CONTROL endpoint
bool USB_IsSendQFull(uint8_t ep);          // this returns TRUE if there are too many outgoing buffers already (IN, CONTROL)
bool USB_IsStalled(uint8_t ep);            // this tells me I'm 'stalled' (BULK IN, INTERRUPT, CONTROL)
int USB_Send(uint8_t ep, const void* data, // send endpoint data.  bSendNow marks it "to send"
             int len, uint8_t bSendNow);
int USB_Recv(uint8_t ep, void* data,       // 'receive' data from endpoint receive queue.  returns < 0 on error, or # of bytes
             int len);
int USB_Recv(uint8_t ep);                  // 'receive' one byte of data from endpoint receive queue
void USB_Flush(uint8_t ep);                // 'sends' all pending data by marking the buffers "to send"

uint16_t GetFrameNumber(void);             // a debug API to obtain the latest USB frame number
uint8_t USB_GetEPType(uint8_t nEP);        // another debug API to return endpoint type by index

#endif

#endif /* if defined(USBCON) */

