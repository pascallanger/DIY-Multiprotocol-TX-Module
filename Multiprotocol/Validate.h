// Check selected board type
#if defined (STM32_BOARD) && defined (ORANGE_TX)
	#error You must comment the board type STM32_BOARD in _Config.h to compile ORANGE_TX
#endif
#if not defined (ORANGE_TX) && not defined (STM32_BOARD)
	//Atmega328p
	#if not defined(ARDUINO_AVR_PRO) && not defined(ARDUINO_AVR_MINI) && not defined(ARDUINO_AVR_NANO)
		#error You must select one of these boards: "Multi 4-in-1", "Arduino Pro or Pro Mini" or "Arduino Mini"
	#endif
	#if F_CPU != 16000000L || not defined(__AVR_ATmega328P__)
		#error You must select the processor type "ATmega328(5V, 16MHz)"
	#endif
#endif
#if defined (STM32_BOARD) && not defined (ORANGE_TX)
	//STM32
	#ifndef ARDUINO_GENERIC_STM32F103C
		#error You must select the board type "Generic STM32F103C series"
	#endif
#endif

//Change/Force configuration if OrangeTX
#ifdef ORANGE_TX
	#undef ENABLE_PPM			// Disable PPM for OrangeTX module
	#undef A7105_INSTALLED		// Disable A7105 for OrangeTX module
	#undef A7105_CSN_pin
	#undef CC2500_INSTALLED		// Disable CC2500 for OrangeTX module
	#undef CC25_CSN_pin
	#undef NRF24L01_INSTALLED	// Disable NRF for OrangeTX module
	#undef NRF_CSN_pin
	#define TELEMETRY			// Enable telemetry
	#define INVERT_TELEMETRY	// Enable invert telemetry
	#define DSM_TELEMETRY		// Enable DSM telemetry
#endif

//Make sure protocols are selected correctly
#ifndef A7105_INSTALLED
	#undef FLYSKY_A7105_INO
	#undef HUBSAN_A7105_INO
	#undef AFHDS2A_A7105_INO
#endif
#ifndef CYRF6936_INSTALLED
	#undef	DEVO_CYRF6936_INO
	#undef	DSM_CYRF6936_INO
	#undef	J6PRO_CYRF6936_INO
	#undef	WK2x01_CYRF6936_INO
#endif
#ifndef CC2500_INSTALLED
	#undef	FRSKYD_CC2500_INO
	#undef	FRSKYV_CC2500_INO
	#undef	FRSKYX_CC2500_INO
	#undef	SFHSS_CC2500_INO
#endif
#ifndef NRF24L01_INSTALLED
	#undef	BAYANG_NRF24L01_INO
	#undef	CG023_NRF24L01_INO
	#undef	CX10_NRF24L01_INO
	#undef	ESKY_NRF24L01_INO
	#undef	HISKY_NRF24L01_INO
	#undef	KN_NRF24L01_INO
	#undef	SLT_NRF24L01_INO
	#undef	SYMAX_NRF24L01_INO
	#undef	V2X2_NRF24L01_INO
	#undef	YD717_NRF24L01_INO
	#undef	MT99XX_NRF24L01_INO
	#undef	MJXQ_NRF24L01_INO
	#undef	SHENQI_NRF24L01_INO
	#undef	FY326_NRF24L01_INO
	#undef	FQ777_NRF24L01_INO
	#undef	ASSAN_NRF24L01_INO
	#undef	HONTAI_NRF24L01_INO
	#undef	Q303_NRF24L01_INO
	#undef	GW008_NRF24L01_INO
	#undef	DM002_NRF24L01_INO
#endif

//Make sure telemetry is selected correctly
#ifndef TELEMETRY
	#undef INVERT_TELEMETRY
	#undef AFHDS2A_FW_TELEMETRY
	#undef AFHDS2A_HUB_TELEMETRY
	#undef BAYANG_HUB_TELEMETRY
	#undef HUBSAN_HUB_TELEMETRY
	#undef HUB_TELEMETRY
	#undef SPORT_TELEMETRY
	#undef DSM_TELEMETRY
	#undef MULTI_STATUS
	#undef MULTI_TELEMETRY
#else
	#if defined MULTI_TELEMETRY && not defined INVERT_TELEMETRY
		#warning MULTI_TELEMETRY has been defined but not INVERT_TELEMETRY. They should be both enabled for OpenTX telemetry and status to work.
	#endif
	#if not defined(BAYANG_NRF24L01_INO)
		#undef BAYANG_HUB_TELEMETRY
	#endif
	#if not defined(HUBSAN_A7105_INO)
		#undef HUBSAN_HUB_TELEMETRY
	#endif
	#if not defined(AFHDS2A_A7105_INO)
		#undef 	AFHDS2A_HUB_TELEMETRY
		#undef 	AFHDS2A_FW_TELEMETRY
	#endif
	#if not defined(FRSKYD_CC2500_INO)
		#undef HUB_TELEMETRY
	#endif
	#if not defined(FRSKYX_CC2500_INO)
		#undef SPORT_TELEMETRY
	#endif
	#if not defined(DSM_CYRF6936_INO)
		#undef DSM_TELEMETRY
	#endif
	#if not defined(DSM_TELEMETRY) && not defined(SPORT_TELEMETRY) && not defined(HUB_TELEMETRY) && not defined(HUBSAN_HUB_TELEMETRY) && not defined(BAYANG_HUB_TELEMETRY) && not defined(AFHDS2A_HUB_TELEMETRY) && not defined(AFHDS2A_FW_TELEMETRY) && not defined(MULTI_TELEMETRY) && not defined(MULTI_STATUS)
		#undef TELEMETRY
		#undef INVERT_TELEMETRY
	#endif
#endif

//Make sure TX is defined correctly
#ifndef AILERON
	#error You must select a correct channel order.
#endif
#if not defined(PPM_MAX_100) || not defined(PPM_MIN_100) || not defined(PPM_MAX_125) || not defined(PPM_MIN_125)
	#error You must set correct TX end points.
#endif

#if defined(ENABLE_BIND_CH)
	#if BIND_CH<4
		#error BIND_CH must be above 4.
	#endif
	#if BIND_CH>16
		#error BIND_CH must be below or equal to 16.
	#endif
#endif

#if MIN_PPM_CHANNELS>16
	#error MIN_PPM_CHANNELS must be below or equal to 16. The default for this value is 4.
#endif
#if MIN_PPM_CHANNELS<2
	#error MIN_PPM_CHANNELS must be larger than 1. The default for this value is 4.
#endif
#if MAX_PPM_CHANNELS<MIN_PPM_CHANNELS
	#error MAX_PPM_CHANNELS must be higher than MIN_PPM_CHANNELS. The default for this value is 16.
#endif
#if MAX_PPM_CHANNELS>16
	#error MAX_PPM_CHANNELS must be below or equal to 16. The default for this value is 16.
#endif
