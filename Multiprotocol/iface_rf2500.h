#ifndef _IFACE_RF2500_H_
#define _IFACE_RF2500_H_

#include "iface_cyrf6936.h"

//RF2500
static void __attribute__((unused)) RF2500_Init(uint8_t, bool);
static void __attribute__((unused)) RF2500_SetTXAddr(const uint8_t*);
static void __attribute__((unused)) RF2500_BuildPayload(uint8_t*);
static void __attribute__((unused)) RF2500_SendPayload();
static void __attribute__((unused)) RF2500_RFChannel(uint8_t);
static void __attribute__((unused)) RF2500_SetPower();

#endif
