#ifndef _IFACE_XN297_H_
#define _IFACE_XN297_H_

#if defined (CC2500_INSTALLED)
	#include "iface_cc2500.h"
#endif
#if defined (NRF24L01_INSTALLED)
	#include "iface_nrf24l01.h"
#endif

#if defined (CC2500_INSTALLED) || defined (NRF24L01_INSTALLED)

//////////////////
// Configuration
#define	XN297_UNSCRAMBLED   false
#define	XN297_SCRAMBLED     true
#define	XN297_CRCDIS        false
#define	XN297_CRCEN         true
#define	XN297_1M            false
#define	XN297_250K          true
#define	XN297_NRF           false
#define	XN297_CC2500        true
#define	XN297_SCRAMBLE_SEED 0xE3B14BE0	// whitening x^7 + x^3 + 1, seed 0x71: first 28 bits

extern uint32_t xn297_lfsr;

//////////////
// Functions
static bool __attribute__((unused)) XN297_Configure(bool, bool, bool, bool force_nrf=false);
static void __attribute__((unused)) XN297_SetTXAddr(const uint8_t*, uint8_t);
static void __attribute__((unused)) XN297_SetRXAddr(const uint8_t*, uint8_t);
static void __attribute__((unused)) XN297_SetTxRxMode(enum TXRX_State);
static void __attribute__((unused)) XN297_SendPayload(uint8_t*, uint8_t);
static void __attribute__((unused)) XN297_ReSendPayload();
static void __attribute__((unused)) XN297_WritePayload(uint8_t*, uint8_t);
static void __attribute__((unused)) XN297_WriteEnhancedPayload(uint8_t*, uint8_t, uint8_t);
static bool __attribute__((unused)) XN297_IsRX();
static void __attribute__((unused)) XN297_ReceivePayload(uint8_t*, uint8_t);
static bool __attribute__((unused)) XN297_ReadPayload(uint8_t*, uint8_t);
static uint8_t __attribute__((unused)) XN297_ReadEnhancedPayload(uint8_t*, uint8_t);
static void __attribute__((unused)) XN297_HoppingCalib(uint8_t);
static void __attribute__((unused)) XN297_Hopping(uint8_t);
static void __attribute__((unused)) XN297_RFChannel(uint8_t);
static void __attribute__((unused)) XN297_SetPower();
static void __attribute__((unused)) XN297_SetFreqOffset();
static bool __attribute__((unused)) XN297_IsPacketSent();
static uint8_t __attribute__((unused)) XN297_scramble_byte();
#ifdef XN297DUMP_NRF24L01_INO
static uint16_t __attribute__((unused)) XN297_crc_xorout(bool, uint16_t);
#endif

#endif

#endif
