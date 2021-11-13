#ifndef _IFACE_HS6200_H_
#define _IFACE_HS6200_H_

#include "iface_cyrf6936.h"

//HS6200
static void __attribute__((unused)) HS6200_Init(bool);
static void __attribute__((unused)) HS6200_SetTXAddr(const uint8_t*, uint8_t);
static void __attribute__((unused)) HS6200_SendPayload(uint8_t*, uint8_t);
#define HS6200_SetPower() CYRF_GFSK1M_SetPower()
#define HS6200_RFChannel(X) CYRF_ConfigRFChannel(X)

#endif
