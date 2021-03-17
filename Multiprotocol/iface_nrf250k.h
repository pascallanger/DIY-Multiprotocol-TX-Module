#ifndef _IFACE_NRF250K_H_
#define _IFACE_NRF250K_H_

#ifdef CC2500_INSTALLED
	#include "iface_cc2500.h"
#endif
#ifdef NRF24L01_INSTALLED
	#include "iface_nrf24l01.h"
#endif
#include "iface_xn297.h"

#if defined (CC2500_INSTALLED) || defined (NRF24L01_INSTALLED)

//////////////
// Functions
#define NRF250K_Init()			XN297_Configure(XN297_CRCEN, XN297_SCRAMBLED, XN297_250K)
#define NRF250K_HoppingCalib(X)	XN297_HoppingCalib(X)
#define NRF250K_Hopping(X)		XN297_Hopping(X)
#define NRF250K_RFChannel(X)	XN297_RFChannel(X)
#define NRF250K_SetPower()		XN297_SetPower()
#define NRF250K_SetFreqOffset()	XN297_SetFreqOffset()
#define NRF250K_IsPacketSent()  XN297_IsPacketSent()
static void __attribute__((unused)) NRF250K_SetTXAddr(uint8_t*, uint8_t);
static void __attribute__((unused)) NRF250K_WritePayload(uint8_t*, uint8_t);

#endif

#endif
