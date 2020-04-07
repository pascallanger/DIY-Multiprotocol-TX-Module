#ifndef _IFACE_NRF250K_H_

#define _IFACE_NRF250K_H_

#if defined (CC2500_INSTALLED)
	#include "iface_cc2500.h"
#endif
#if defined (NRF24L01_INSTALLED)
	#include "iface_nrf24l01.h"
#endif

//XN297L
static void __attribute__((unused)) XN297L_Init();
static void __attribute__((unused)) XN297L_SetTXAddr(const uint8_t*, uint8_t);
static void __attribute__((unused)) XN297L_WritePayload(uint8_t*, uint8_t);
static void __attribute__((unused)) XN297L_WriteEnhancedPayload(uint8_t*, uint8_t, uint8_t);
static void __attribute__((unused)) XN297L_HoppingCalib(__attribute__((unused)) uint8_t);
static void __attribute__((unused)) XN297L_Hopping(uint8_t);
static void __attribute__((unused)) XN297L_RFChannel(uint8_t);
static void __attribute__((unused)) XN297L_SetPower();
static void __attribute__((unused)) XN297L_SetFreqOffset();

//NRF250K
#define NRF250K_Init() XN297L_Init()
#define NRF250K_HoppingCalib(X) XN297L_HoppingCalib(X)
#define NRF250K_Hopping(X) XN297L_Hopping(X)
#define NRF250K_RFChannel(X) XN297L_RFChannel(X)
#define NRF250K_SetPower() XN297L_SetPower()
#define NRF250K_SetFreqOffset() XN297L_SetFreqOffset()
static void __attribute__((unused)) NRF250K_SetTXAddr(uint8_t*, uint8_t);
static void __attribute__((unused)) NRF250K_WritePayload(uint8_t*, uint8_t);
static boolean __attribute__((unused)) NRF250K_IsPacketSent();

#endif