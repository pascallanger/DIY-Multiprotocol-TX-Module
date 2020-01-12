#ifndef _IFACE_XN297L_H_

#define _IFACE_XN297L_H_

#if defined (CC2500_INSTALLED)
	#include "iface_cc2500.h"
#endif
#if defined (NRF24L01_INSTALLED)
	#include "iface_nrf24l01.h"
#endif

static void __attribute__((unused)) XN297L_Init();
static void __attribute__((unused)) XN297L_SetTXAddr(const uint8_t*, uint8_t);
static void __attribute__((unused)) XN297L_WritePayload(uint8_t*, uint8_t);
static void __attribute__((unused)) XN297L_WriteEnhancedPayload(uint8_t*, uint8_t, uint8_t);
static void __attribute__((unused)) XN297L_HoppingCalib(__attribute__((unused)) uint8_t);
static void __attribute__((unused)) XN297L_Hopping(uint8_t);
static void __attribute__((unused)) XN297L_RFChannel(uint8_t);
static void __attribute__((unused)) XN297L_SetPower();
static void __attribute__((unused)) XN297L_SetFreqOffset();

#endif