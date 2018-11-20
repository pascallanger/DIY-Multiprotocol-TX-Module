// Check selected board type
#if defined (STM32_BOARD) && defined (ORANGE_TX)
	#error You must comment the board type STM32_BOARD in _Config.h to compile ORANGE_TX
#endif
#if not defined (ORANGE_TX) && not defined (STM32_BOARD)
	//Atmega328p
	#if not defined(ARDUINO_AVR_PRO) && not defined(ARDUINO_MULTI_NO_BOOT) && not defined(ARDUINO_MULTI_FLASH_FROM_TX) && not defined(ARDUINO_AVR_MINI) && not defined(ARDUINO_AVR_NANO)
		#error You must select one of these boards: "Multi 4-in-1", "Arduino Pro or Pro Mini" or "Arduino Mini"
	#endif
	#if F_CPU != 16000000L || not defined(__AVR_ATmega328P__)
		#error You must select the processor type "ATmega328(5V, 16MHz)"
	#endif
#endif
#if defined (STM32_BOARD) && not defined (ORANGE_TX)
	//STM32
	#if not defined(ARDUINO_GENERIC_STM32F103C) && not defined(ARDUINO_MULTI_STM32_FLASH_FROM_TX) && not defined(ARDUINO_MULTI_STM32_NO_BOOT) && not defined(ARDUINO_MULTI_STM32_WITH_BOOT)
		#error You must select one of these boards: "Multi 4-in-1 (STM32F103CB)" or "Generic STM32F103C series"
	#endif
#endif

// Check for minimum version of multi-module boards
#define MIN_AVR_BOARD 103
#define MIN_ORX_BOARD 103
#define MIN_STM32_BOARD 104
//AVR
#if (defined(ARDUINO_MULTI_NO_BOOT) && ARDUINO_MULTI_NO_BOOT < MIN_AVR_BOARD) || (defined(ARDUINO_MULTI_FLASH_FROM_TX) && ARDUINO_MULTI_FLASH_FROM_TX < MIN_AVR_BOARD)
	#error You need to update your Multi 4-in-1 board definition.  Open Boards Manager and update to the latest version of the Multi 4-in-1 AVR Boards.
#endif
//OrangeRX
#if (defined(ARDUINO_MULTI_ORANGERX) && ARDUINO_MULTI_ORANGERX < MIN_ORX_BOARD)
	#error You need to update your Multi 4-in-1 board definition.  Open Boards Manager and update to the latest version of the Multi 4-in-1 AVR Boards.
#endif
//STM32
#if (defined(ARDUINO_MULTI_STM32_NO_BOOT) && ARDUINO_MULTI_STM32_NO_BOOT < MIN_STM32_BOARD) || (defined(ARDUINO_MULTI_STM32_FLASH_FROM_TX) && ARDUINO_MULTI_STM32_FLASH_FROM_TX < MIN_STM32_BOARD) || (defined(ARDUINO_MULTI_STM32_WITH_BOOT) && ARDUINO_MULTI_STM32_WITH_BOOT < MIN_STM32_BOARD)
	#error You need to update your Multi 4-in-1 board definition.  Open Boards Manager and update to the latest version of the Multi 4-in-1 STM32 Board.
#endif

// Error if CHECK_FOR_BOOTLOADER is not enabled but a FLASH_FROM_TX board is selected
#if (defined(ARDUINO_MULTI_FLASH_FROM_TX) || defined(ARDUINO_MULTI_STM32_FLASH_FROM_TX)) &! defined(CHECK_FOR_BOOTLOADER)
	#if defined(STM32_BOARD)
		#error "You have selected the 'Flash from TX' upload method but not enabled CHECK_FOR_BOOTLOADER."
	#else
		#error "You have selected the 'Flash from TX' bootloader but not enabled CHECK_FOR_BOOTLOADER."
	#endif
#endif

// Error if CHECK_FOR_BOOTLOADER is enabled but the 'Flash from TX' bootloader
#if defined(ARDUINO_MULTI_NO_BOOT) && defined(CHECK_FOR_BOOTLOADER)
	#error "You have enabled CHECK_FOR_BOOTLOADER but not selected the 'Flash from TX' bootloader."
#endif

//Check number of banks
#if NBR_BANKS < 1 || NBR_BANKS > 5
	#error "You need to select a number of banks between 1 and 5."
#endif

//Check failsafe throttle value
#ifdef FAILSAFE_ENABLE
	#if ( FAILSAFE_THROTTLE_LOW < -125 ) || ( FAILSAFE_THROTTLE_LOW > 125 )
		#error "The failsafe value for throttle is outside of the range -125..125."
	#endif
#endif

// Check forced tuning values are valid
#ifdef FORCE_FRSKYD_TUNING
	#if ( FORCE_FRSKYD_TUNING < -127 ) || ( FORCE_FRSKYD_TUNING > 127 )
		#error "The FrSkyD forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_FRSKYV_TUNING
	#if ( FORCE_FRSKYV_TUNING < -127 ) || ( FORCE_FRSKYV_TUNING > 127 )
		#error "The FrSkyV forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_FRSKYX_TUNING
	#if ( FORCE_FRSKYX_TUNING < -127 ) || ( FORCE_FRSKYX_TUNING > 127 )
		#error "The FrSkyX forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_SFHSS_TUNING
	#if ( FORCE_SFHSS_TUNING < -127 ) || ( FORCE_SFHSS_TUNING > 127 )
		#error "The SFHSS forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_CORONA_TUNING
	#if ( FORCE_CORONA_TUNING < -127 ) || ( FORCE_CORONA_TUNING > 127 )
		#error "The CORONA forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_HITEC_TUNING
	#if ( FORCE_HITEC_TUNING < -127 ) || ( FORCE_HITEC_TUNING > 127 )
		#error "The HITEC forced frequency tuning value is outside of the range -127..127."
	#endif
#endif
#ifdef FORCE_FLYSKY_TUNING
	#if ( FORCE_FLYSKY_TUNING < -300 ) || ( FORCE_FLYSKY_TUNING > 300 )
		#error "The Flysky forced frequency tuning value is outside of the range -300..300."
	#endif
#endif
#ifdef FORCE_HUBSAN_TUNING
	#if ( FORCE_HUBSAN_TUNING < -300 ) || ( FORCE_HUBSAN_TUNING > 300 )
		#error "The Hubsan forced frequency tuning value is outside of the range -300..300."
	#endif
#endif
#ifdef FORCE_AFHDS2A_TUNING
	#if ( FORCE_AFHDS2A_TUNING < -300 ) || ( FORCE_AFHDS2A_TUNING > 300 )
		#error "The AFHDS2A forced frequency tuning value is outside of the range -300..300."
	#endif
#endif
#ifndef USE_A7105_CH15_TUNING
	#ifndef FORCE_FLYSKY_TUNING
		#define FORCE_FLYSKY_TUNING 0
	#endif
	#ifndef FORCE_HUBSAN_TUNING
		#define FORCE_HUBSAN_TUNING 0
	#endif
	#ifndef FORCE_AFHDS2A_TUNING
		#define FORCE_AFHDS2A_TUNING 0
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
	#undef BUGS_A7105_INO
#endif
#ifndef CYRF6936_INSTALLED
	#undef	DEVO_CYRF6936_INO
	#undef	DSM_CYRF6936_INO
	#undef	J6PRO_CYRF6936_INO
	#undef	WFLY_CYRF6936_INO
	#undef	WK2x01_CYRF6936_INO
	#undef TRAXXAS_CYRF6936_INO
#endif
#ifndef CC2500_INSTALLED
	#undef	FRSKYD_CC2500_INO
	#undef	FRSKYV_CC2500_INO
	#undef	FRSKYX_CC2500_INO
	#undef	SFHSS_CC2500_INO
	#undef	CORONA_CC2500_INO
	#undef	HITEC_CC2500_INO
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
	#undef	GD00X_NRF24L01_INO
	#undef	DM002_NRF24L01_INO
	#undef	CABELL_NRF24L01_INO
	#undef	ESKY150_NRF24L01_INO
	#undef	H8_3D_NRF24L01_INO
	#undef	CFLIE_NRF24L01_INO
	#undef	BUGSMINI_NRF24L01_INO
	#undef	NCC1701_NRF24L01_INO
	#undef	E01X_NRF24L01_INO
	#undef	V911S_NRF24L01_INO
#endif

//Make sure telemetry is selected correctly
#ifndef TELEMETRY
	#undef INVERT_TELEMETRY
	#undef AFHDS2A_FW_TELEMETRY
	#undef AFHDS2A_HUB_TELEMETRY
	#undef HITEC_FW_TELEMETRY
	#undef HITEC_HUB_TELEMETRY
	#undef BAYANG_HUB_TELEMETRY
	#undef CABELL_HUB_TELEMETRY
	#undef HUBSAN_HUB_TELEMETRY
	#undef BUGS_HUB_TELEMETRY
	#undef NCC1701_HUB_TELEMETRY
	#undef HUB_TELEMETRY
	#undef SPORT_TELEMETRY
	#undef SPORT_POLLING
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
	#if not defined(NCC1701_NRF24L01_INO)
		#undef NCC1701_HUB_TELEMETRY
	#endif
	#if not ( defined(BUGS_A7105_INO) || defined(BUGSMINI_NRF24L01_INO) )
		#undef BUGS_HUB_TELEMETRY
	#endif
	#if not defined(CABELL_NRF24L01_INO)
		#undef CABELL_HUB_TELEMETRY
	#endif
	#if not defined(HUBSAN_A7105_INO)
		#undef HUBSAN_HUB_TELEMETRY
	#endif
	#if not defined(AFHDS2A_A7105_INO)
		#undef 	AFHDS2A_HUB_TELEMETRY
		#undef 	AFHDS2A_FW_TELEMETRY
	#endif
	#if not defined(HITEC_CC2500_INO)
		#undef 	HITEC_HUB_TELEMETRY
		#undef 	HITEC_FW_TELEMETRY
	#endif
	#if not defined(FRSKYD_CC2500_INO)
		#undef HUB_TELEMETRY
	#endif
	#if not defined(FRSKYX_CC2500_INO)
		#undef SPORT_TELEMETRY
		#undef SPORT_POLLING
	#endif
	#if not defined (SPORT_TELEMETRY) || not defined (STM32_BOARD)
		#undef SPORT_POLLING
	#endif
	#if defined SPORT_POLLING && not defined INVERT_TELEMETRY
		#error SPORT_POLLING has been defined but not INVERT_TELEMETRY. They should be both enabled to work.
	#endif
	#if not defined(DSM_CYRF6936_INO)
		#undef DSM_TELEMETRY
	#endif
	#if not defined(DSM_TELEMETRY) && not defined(SPORT_TELEMETRY) && not defined(HUB_TELEMETRY) && not defined(HUBSAN_HUB_TELEMETRY) && not defined(BUGS_HUB_TELEMETRY) && not defined(NCC1701_HUB_TELEMETRY) && not defined(BAYANG_HUB_TELEMETRY) && not defined(CABELL_HUB_TELEMETRY) && not defined(AFHDS2A_HUB_TELEMETRY) && not defined(AFHDS2A_FW_TELEMETRY) && not defined(MULTI_TELEMETRY) && not defined(MULTI_STATUS) && not defined(HITEC_HUB_TELEMETRY) && not defined(HITEC_FW_TELEMETRY)
		#undef TELEMETRY
		#undef INVERT_TELEMETRY
		#undef SPORT_POLLING
	#endif
#endif

//Make sure TX is defined correctly
#ifndef AILERON
	#error You must select a correct channel order.
#endif
#if not defined(PPM_MAX_100) || not defined(PPM_MIN_100)
	#error You must set correct PPM end points for your TX.
#endif

#if defined(ENABLE_BIND_CH)
	#if BIND_CH<4
		#error BIND_CH must be above 4.
	#endif
	#if BIND_CH>16
		#error BIND_CH must be below or equal to 16.
	#endif
#endif

#if defined(DSM_THROTTLE_KILL_CH)
	#if DSM_THROTTLE_KILL_CH<4
		#error DSM_THROTTLE_KILL_CH must be above 4.
	#endif
	#if DSM_THROTTLE_KILL_CH>16
		#error DSM_THROTTLE_KILL_CH must be below or equal to 16.
	#endif
#endif

#if defined(AFHDS2A_LQI_CH)
	#if AFHDS2A_LQI_CH<4
		#error AFHDS2A_LQI_CH must be above 4.
	#endif
	#if AFHDS2A_LQI_CH>14
		#error AFHDS2A_LQI_CH must be below or equal to 14.
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
