/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */

/**********************************************/
/** Multiprotocol module configuration file ***/
/**********************************************/

/********************/
/*** LOCAL CONFIG ***/
/********************/
//If you know parameters you want for sure to be enabled or disabled which survives in future, you can use a file named "_MyConfig.h".
//An example is given within the file named "_MyConfig.h.example" which needs to be renamed if you want to use it.
//To enable this config file remove the // from the line below.
//#define USE_MY_CONFIG


/*************************/
/*** BOOTLOADER USE     ***/
/*************************/
//Allow flashing multimodule directly with TX(erky9x or opentx maintenance mode)
//Instructions:https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/docs/Flash_from_Tx.md
//To disable this feature add "//" at the begining of the next line.  Requires a compatible bootloader or upload method to be selected when you use the Multi 4-in-1 Boards Manager definitions.
#define CHECK_FOR_BOOTLOADER


/*******************/
/*** TX SETTINGS ***/
/*******************/
//Modify the channel order based on your TX: AETR, TAER, RETA...
//Examples: Flysky & DEVO is AETR, JR/Spektrum radio is TAER, Multiplex is AERT...
//Default is AETR.
#define AETR

//Uncomment to reverse the direction of the specified channel for all protocols
//#define REVERSE_AILERON
//#define REVERSE_ELEVATOR
//#define REVERSE_THROTTLE
//#define REVERSE_RUDDER


/*****************/
/*** AUTO BIND ***/  // Also referred as "Bind on powerup"
/*****************/
//Bind from channel enables you to bind when a specified channel is going from low to high. This feature is only active
// if you specify AUTOBIND in PPM mode or set AutoBind to YES for serial mode.
//Comment to globaly disable the bind feature from a channel.
#define ENABLE_BIND_CH
//Set the channel number used for bind. Default is 16.
#define BIND_CH	16

//Comment to disable the wait for bind feature. If Autobind is enabled in the model config, this feature will not activate
// the selected protocol unless a bind is requested using bind from channel or the GUI "Bind" button.
//The goal is to prevent binding other people's model when powering up the TX, changing model or scanning through protocols.
#define WAIT_FOR_BIND


/****************/
/*** RF CHIPS ***/
/****************/
//There are 4 RF components supported. If one of them is not installed you must comment it using "//".
//If a chip is not installed all associated protocols are automatically disabled.
//4-in-1 modules have all RF chips installed
//!!!If a RF chip is present it MUST be marked as installed!!! or weird things will happen you have been warned.
#define A7105_INSTALLED
#define CYRF6936_INSTALLED
#define CC2500_INSTALLED
#define NRF24L01_INSTALLED
//#define SX1276_INSTALLED		// only supported on STM32 modules

/** OrangeRX TX **/
//If you compile for the OrangeRX TX module you need to select the correct board type.
//By default the compilation is done for the GREEN board, to switch to a BLUE board uncomment the line below by removing the "//"
//#define ORANGE_TX_BLUE

/** CC2500 Fine Frequency Tuning **/
//For optimal performance the CC2500 RF module used by the CORONA, FrSkyD, FrSkyV, FrSkyX, Hitec, HoTT, SFHSS and Redpine protocols needs to be tuned for each protocol.
//Initial tuning should be done via the radio menu with a genuine CORONA/FrSky/Hitec/HoTT/Futaba/Redpine receiver.  
//Once a good tuning value is found it can be set here and will override the radio's 'option' setting for all existing and new models which use that protocol.
//For more information: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/tree/master/docs/Frequency_Tuning.md
//Uncomment the lines below (remove the "//") and set an appropriate value (replace the "0") to enable. Valid range is -127 to +127.
//#define FORCE_CORONA_TUNING	0
//#define FORCE_FRSKYD_TUNING	0
//#define FORCE_FRSKYL_TUNING	0
//#define FORCE_FRSKYV_TUNING	0
//#define FORCE_FRSKYX_TUNING	0
//#define FORCE_SFHSS_TUNING	0
//#define FORCE_SKYARTEC_TUNING	0
//#define FORCE_HITEC_TUNING	0
//#define FORCE_HOTT_TUNING		0
//#define FORCE_REDPINE_TUNING	0

/** A7105 Fine Frequency Tuning **/
//This is required in rare cases where some A7105 modules and/or RXs have an inaccurate crystal oscillator.
//If using Serial mode only (for now), you can use CH15 to find the right tuning value. -100%=-300, 0%=default 0, +100%=+300.
//Uncomment the line below (remove the "//") to enable this feature.
//#define USE_A7105_CH15_TUNING

//Once a good tuning value is found it can be set here and will override the frequency tuning for a specific protocol.
//Uncomment the lines below (remove the "//") and set an appropriate value (replace the "0") to enable. Valid range is -300 to +300 and default is 0.
//#define FORCE_AFHDS2A_TUNING	0
//#define FORCE_BUGS_TUNING		0
//#define FORCE_FLYSKY_TUNING	0
//#define FORCE_FLYZONE_TUNING	0
//#define FORCE_PELIKAN_TUNING	0
//#define FORCE_HUBSAN_TUNING	0

/** CYRF6936 Fine Frequency Tuning **/
//This is required in rare cases where some CYRF6936 modules and/or RXs have an inaccurate crystal oscillator.
//If using Serial mode only (for now), you can use CH15 to find the right tuning value. -100%=-300, 0%=default 0, +100%=+300.
//Uncomment the line below (remove the "//") to enable this feature.
//#define USE_CYRF6936_CH15_TUNING

/** Low Power **/
//Low power is reducing the transmit power of the multi module. This setting is configurable per model in PPM (table below) or Serial mode (radio GUI).
//It can be activated when flying indoor or small models since the distance is short or if a model is causing issues when flying closed to the TX.
//By default low power selection is enabled on all rf chips, but you can disable it by commenting (add //) the lines below if you don't want to risk
//flying a model with low power.
#define A7105_ENABLE_LOW_POWER
#define CYRF6936_ENABLE_LOW_POWER
#define CC2500_ENABLE_LOW_POWER
#define NRF24L01_ENABLE_LOW_POWER


/*****************/
/*** GLOBAL ID ***/
/*****************/
//A global ID is used by most protocols to bind and retain the bind to models. To prevent duplicate IDs, it is automatically
// generated using a random 32 bits number the first time the eeprom is initialized.
//If you have 2 Multi modules which you want to share the same ID so you can use either to control the same RC model
// then you can force the ID to a certain known value using the lines below.
//Default is commented, you should uncoment only for test purpose or if you know exactly what you are doing!!!
//The 8 numbers below can be anything between 0...9 and A..F
//#define FORCE_GLOBAL_ID	0x12345678

//Protocols using the CYRF6936 (DSM, Devo, Walkera...) are using the CYRF ID instead which should prevent duplicated IDs.
//If you have 2 Multi modules which you want to share the same ID so you can use either to control the same RC model
// then you can force the ID to a certain known value using the lines below.
//Default is commented, you should uncoment only for test purpose or if you know exactly what you are doing!!!
//#define FORCE_CYRF_ID	"\x12\x34\x56\x78\x9A\xBC"


/****************************/
/*** PROTOCOLS TO INCLUDE ***/
/****************************/
//In this section select the protocols you want to be accessible when using the module.
//All the protocols will not fit in the Atmega328p module so you need to pick and choose.
//Comment the protocols you are not using with "//" to save Flash space.

//The protocols below need an A7105 to be installed
#define	AFHDS2A_A7105_INO
#define AFHDS2A_RX_A7105_INO
#define	BUGS_A7105_INO
#define	FLYSKY_A7105_INO
#define	FLYZONE_A7105_INO
#define	HUBSAN_A7105_INO
#define PELIKAN_A7105_INO

//The protocols below need a CYRF6936 to be installed
#define	DEVO_CYRF6936_INO
#define	DSM_CYRF6936_INO
#define	J6PRO_CYRF6936_INO
#define	TRAXXAS_CYRF6936_INO
#define	WFLY_CYRF6936_INO
#define	WK2x01_CYRF6936_INO

//The protocols below need a CC2500 to be installed
#define	CORONA_CC2500_INO
#define	ESKY150V2_CC2500_INO	//Need both CC2500 and NRF
#define	FRSKYL_CC2500_INO
#define	FRSKYD_CC2500_INO
#define	FRSKYV_CC2500_INO
#define	FRSKYX_CC2500_INO
#define	FRSKY_RX_CC2500_INO
#define	HITEC_CC2500_INO
#define	HOTT_CC2500_INO
#define	SCANNER_CC2500_INO
#define	SFHSS_CC2500_INO
#define	SKYARTEC_CC2500_INO
#define	REDPINE_CC2500_INO

//The protocols below need a NRF24L01 to be installed
#define	ASSAN_NRF24L01_INO
#define	BAYANG_NRF24L01_INO
#define	BAYANG_RX_NRF24L01_INO
#define	BUGSMINI_NRF24L01_INO
#define	CABELL_NRF24L01_INO
#define	CFLIE_NRF24L01_INO
#define	CG023_NRF24L01_INO
#define	CX10_NRF24L01_INO		 //Include Q2X2 protocol
#define	DM002_NRF24L01_INO
#define	E01X_NRF24L01_INO
#define	ESKY_NRF24L01_INO
#define	ESKY150_NRF24L01_INO
#define	FQ777_NRF24L01_INO
#define	FX816_NRF24L01_INO
#define	FY326_NRF24L01_INO
#define	GD00X_NRF24L01_INO
#define	GW008_NRF24L01_INO
#define	HISKY_NRF24L01_INO
#define	HONTAI_NRF24L01_INO
#define	H8_3D_NRF24L01_INO
#define	KF606_NRF24L01_INO
#define	KN_NRF24L01_INO
#define	MJXQ_NRF24L01_INO
#define	MT99XX_NRF24L01_INO
#define	NCC1701_NRF24L01_INO
#define	POTENSIC_NRF24L01_INO
#define	PROPEL_NRF24L01_INO
#define	Q303_NRF24L01_INO
#define	SHENQI_NRF24L01_INO
#define	SLT_NRF24L01_INO
#define	SYMAX_NRF24L01_INO
#define	TIGER_NRF24L01_INO
#define	V2X2_NRF24L01_INO
#define	V761_NRF24L01_INO
#define	V911S_NRF24L01_INO
#define	XK_NRF24L01_INO
#define	YD717_NRF24L01_INO
#define	ZSX_NRF24L01_INO

//The protocols below need a SX1276 to be installed
//#define FRSKYR9_SX1276_INO

/***************************/
/*** PROTOCOLS SETTINGS  ***/
/***************************/

//DSM specific settings
//---------------------
//The DSM protocol is using by default the Spektrum throw of 1100..1900us @100% and 1000..2000us @125%.
// For more throw, 1024..1976us @100% and 904..2096us @125%, remove the "//" on the line below. Be aware that too much throw can damage some UMX servos. To achieve standard throw in this mode use a channel weight of 84%.
//#define DSM_MAX_THROW
//Some models (X-Vert, Blade 230S...) require a special value to instant stop the motor(s).
// You can disable this feature by adding "//" on the line below. You have to specify which channel (14 by default) will be used to kill the throttle channel.
// If the channel 14 is above -50% the throttle is untouched but if it is between -50% and -100%, the throttle output will be forced between -100% and -150%.
// For example, a value of -80% applied on channel 14 will instantly kill the motors on the X-Vert.
#define DSM_THROTTLE_KILL_CH 14 

//AFHDS2A specific settings
//-------------------------
//When enabled (remove the "//"), the below setting makes LQI (Link Quality Indicator) available on one of the RX ouput channel (5-14).
//#define AFHDS2A_LQI_CH 14


/**************************/
/*** FAILSAFE SETTINGS  ***/
/**************************/
//The following protocols are supporting failsafe: FrSkyX, Devo, WK2x01, SFHSS, HISKY/HK310 and AFHDS2A
//In Serial mode failsafe is configured on the radio itself.
//In PPM mode and only after the module is up and fully operational, press the bind button for at least 5sec to send the current stick positions as failsafe to the RX.
//If you want to disable failsafe globally comment the line below using "//".
#define FAILSAFE_ENABLE

/**************************/
/*** TELEMETRY SETTINGS ***/
/**************************/
//In this section you can configure the telemetry.

//If you do not plan using the telemetry comment this global setting using "//" and skip to the next section.
#define TELEMETRY

//Comment to invert the polarity of the output telemetry serial signal.
//This function takes quite some flash space and processor power on an atmega.
//For a Taranis/T16 with an external module it must be uncommented. For a T16 internal module it must be commented.
//A 9XR_PRO running erskyTX will work with both commented and uncommented depending on the radio setting Invert COM1 under the Telemetry menu.
//On other addon/replacement boards like the 9xtreme board or the Ar9x board running erskyTX, you need to uncomment the line below.
//For er9x it depends if you have an inveter mod or not on the telemetry pin. If you don't have an inverter comment this line.
#define INVERT_TELEMETRY
//For STM32 and OrangeRX modules, comment to prevent the TX from forcing the serial telemetry polarity normal/invert.
#define INVERT_TELEMETRY_TX

//Uncomment if you want to send Multi status telemetry frames (Protocol available, Bind in progress, version...)
//Use with er9x/erskyTX, for OpenTX you must select MULTI_TELEMETRY below
//#define MULTI_STATUS

//Sends Multi status and allow OpenTX to autodetect the telemetry format. Comment to disable.
//Supported by OpenTX version 2.2 RC9 and newer. NOT supported by er9x/erskyTX use MULTI_STATUS instead.
#define MULTI_TELEMETRY
//Work in progress: Sync OpenTX frames with the current protocol timing. This feature is only available on the STM32 module. Uncomment to enable.
//#define MULTI_SYNC

//Comment a line to disable a specific protocol telemetry
#define DSM_TELEMETRY				// Forward received telemetry packet directly to TX to be decoded by er9x, erskyTX and OpenTX
#define SPORT_TELEMETRY				// Use FrSkyX format to send/receive telemetry
#define AFHDS2A_FW_TELEMETRY		// Forward received telemetry packet directly to TX to be decoded by erskyTX and OpenTX
#define AFHDS2A_HUB_TELEMETRY		// Use FrSkyD Hub format to send basic telemetry to TX like er9x
#define HUB_TELEMETRY				// Use FrSkyD Hub format to send telemetry to TX
#define BAYANG_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define BUGS_HUB_TELEMETRY			// Use FrSkyD Hub format to send telemetry to TX
#define DEVO_HUB_TELEMETRY			// Use FrSkyD Hub format to send telemetry to TX
#define HUBSAN_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define NCC1701_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define CABELL_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define HITEC_HUB_TELEMETRY			// Use FrSkyD Hub format to send basic telemetry to the radios which can decode it like er9x, erskyTX and OpenTX
#define HITEC_FW_TELEMETRY			// Forward received telemetry packets to be decoded by erskyTX and OpenTX
#define SCANNER_TELEMETRY			// Forward spectrum scanner data to TX
#define FRSKY_RX_TELEMETRY			// Forward channels data to TX
#define AFHDS2A_RX_TELEMETRY		// Forward channels data to TX
#define HOTT_FW_TELEMETRY			// Forward received telemetry packets to be decoded by erskyTX and OpenTX
#define BAYANG_RX_TELEMETRY			// Forward channels data to TX

/****************************/
/*** SERIAL MODE SETTINGS ***/
/****************************/
//In this section you can configure the serial mode.
//The serial mode enables full editing of all the parameters in the GUI of the radio. It is enabled by placing the rotary switch on position 0.
//This is available natively for ER9X, ERSKY9X and OpenTX.

//If you do not plan to use the Serial mode comment this line using "//" to save Flash space
#define ENABLE_SERIAL


/*************************/
/*** PPM MODE SETTINGS ***/
/*************************/
//In this section you can configure all details about PPM.
//If you do not plan to use the PPM mode comment this line using "//" to save Flash space, you don't need to configure anything below in this case
#define ENABLE_PPM

/** TX END POINTS **/
//It is important for the module to know the endpoints of your radio.
//Below are some standard transmitters already preconfigured.
//Uncomment only the one which matches your transmitter.
#define TX_ER9X			//ER9X/ERSKY9X/OpenTX	( 988<->2012 microseconds)
//#define TX_DEVO7		//DEVO					(1120<->1920 microseconds)
//#define TX_SPEKTRUM	//Spektrum				(1100<->1900 microseconds)
//#define TX_HISKY		//HISKY					(1120<->1920 microseconds)
//#define TX_MPX		//Multiplex MC2020		(1250<->1950 microseconds)
//#define TX_WALKERA	//Walkera PL0811-01H	(1000<->1800 microseconds)
//#define TX_CUSTOM		//Custom

// The lines below are used to set the end points in microseconds if you have selected TX_CUSTOM.
// A few things to consider:
//  - If you put too big values compared to your TX you won't be able to reach the extremes which is bad for throttle as an example
//  - If you put too low values you won't be able to use your full stick range, it will be maxed out before reaching the ends
//  - Centered stick value is usually 1500. It should match the middle between MIN and MAX, ie Center=(MAX+MIN)/2. If your TX is not centered you can adjust the value MIN or MAX.
//  - 100% is referred as the value when the TX is set to default with no trims
#if defined(TX_CUSTOM)
	#define PPM_MAX_100	1900	//	100%
	#define PPM_MIN_100	1100	//	100%
#endif

/** Number of PPM Channels **/
// The line below is used to set the minimum number of channels which the module should receive to consider a PPM frame valid.
// The default value is 4 to receive at least AETR for flying models but you could also connect the PPM from a car radio which has only 3 channels by changing this number to 3.
#define MIN_PPM_CHANNELS 4
// The line below is used to set the maximum number of channels which the module should work with. Any channels received above this number are discarded.
// The default value is 16 to receive all possible channels but you might want to filter some "bad" channels from the PPM frame like the ones above 6 on the Walkera PL0811.
#define MAX_PPM_CHANNELS 16

/** Telemetry **/
//Send simple FrSkyX telemetry using the FrSkyD telemetry format
#define TELEMETRY_FRSKYX_TO_FRSKYD

/** Rotary Switch Protocol Selector Settings **/
//The table below indicates which protocol to run when a specific position on the rotary switch has been selected.
//All fields and values are explained below. Everything is configurable from here like in the Serial mode.
//Tip: You can associate multiple times the same protocol to different rotary switch positions to take advantage of the model match based on RX_Num

//A system of banks enable the access to more protocols than positions on the rotary switch. Banks can be selected by placing the rotary switch on position 15, power up the module and
// short press the bind button multiple times until you reach the desired one. The bank number currently selected is indicated by the number of LED flash.
// Full procedure is located here: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/Protocols_Details.md#protocol-selection-in-ppm-mode

//The parameter below indicates the number of desired banks between 1 and 5. Default is 1.
#define NBR_BANKS 1

const PPM_Parameters PPM_prot[14*NBR_BANKS]=	{
#if NBR_BANKS > 0
//******************************       BANK 1       ******************************
//	Switch	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option	Chan Order
/*	1	*/	{PROTO_FLYSKY,	Flysky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	2	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 0
/*	3	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	1	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 1
/*	4	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	2	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 2
/*	5	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	3	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 3
/*	6	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	2	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 4
/*	7	*/	{PROTO_AFHDS2A,	PWM_IBUS	,	3	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },	// RX number 5
/*	8	*/	{PROTO_SFHSS,	H107		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	9	*/	{PROTO_FRSKYV,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	40	,	0x00000000 },	// option=fine freq tuning
/*	10	*/	{PROTO_FRSKYD,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	40	,	0x00000000 },	// option=fine freq tuning
/*	11	*/	{PROTO_FRSKYX,	CH_16		,	0	,	P_HIGH	,	NO_AUTOBIND	,	40	,	0x00000000 },	// option=fine freq tuning
/*	12	*/	{PROTO_FRSKYX,	EU_16		,	0	,	P_HIGH	,	NO_AUTOBIND	,	40	,	0x00000000 },	// option=fine freq tuning
/*	13	*/	{PROTO_DEVO	,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	14	*/	{PROTO_WK2x01,	WK2801		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
#endif
#if NBR_BANKS > 1
//******************************       BANK 2       ******************************
//	Switch	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option	Chan Order
/*	1	*/	{PROTO_DSM	,	DSM2_11		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6	,	0x00000000 },	// option=number of channels
/*	2	*/	{PROTO_DSM	,	DSM2_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6	,	0x00000000 },	// option=number of channels
/*	3	*/	{PROTO_DSM	,	DSMX_11		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6	,	0x00000000 },	// option=number of channels
/*	4	*/	{PROTO_DSM	,	DSMX_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6	,	0x00000000 },	// option=number of channels
/*	5	*/	{PROTO_DSM	,	DSM2_11		,	0	,	P_HIGH	,	NO_AUTOBIND	,	8	,	0x00000000 },	// option=number of channels
/*	6	*/	{PROTO_DSM	,	DSM2_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	8	,	0x00000000 },	// option=number of channels
/*	7	*/	{PROTO_DSM	,	DSMX_11		,	0	,	P_HIGH	,	NO_AUTOBIND	,	8	,	0x00000000 },	// option=number of channels
/*	8	*/	{PROTO_DSM	,	DSMX_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	8	,	0x00000000 },	// option=number of channels
/*	9	*/	{PROTO_SLT	,	SLT_V1		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6	,	0x00000000 },
/*	10	*/	{PROTO_HUBSAN,	H107		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	11	*/	{PROTO_HUBSAN,	H301		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	12	*/	{PROTO_HUBSAN,	H501		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	13	*/	{PROTO_HISKY,	Hisky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	14	*/	{PROTO_V2X2	,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
#endif
#if NBR_BANKS > 2
//******************************       BANK 3       ******************************
//	Switch	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option	Chan Order
/*	1	*/	{PROTO_ESKY	,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	2	*/	{PROTO_ESKY150,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	3	*/	{PROTO_ASSAN,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	4	*/	{PROTO_CORONA,	COR_V2		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	5	*/	{PROTO_SYMAX,	SYMAX		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	6	*/	{PROTO_KN	,	WLTOYS		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	7	*/	{PROTO_BAYANG,	BAYANG		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	8	*/	{PROTO_BAYANG,	H8S3D		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	9	*/	{PROTO_BAYANG,	X16_AH		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	10	*/	{PROTO_BAYANG,	IRDRONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	11	*/	{PROTO_H8_3D,	H8_3D		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	12	*/	{PROTO_H8_3D,	H20H		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	13	*/	{PROTO_H8_3D,	H20MINI		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	14	*/	{PROTO_H8_3D,	H30MINI		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
#endif
#if NBR_BANKS > 3
//******************************       BANK 4       ******************************
//	Switch	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option	Chan Order
/*	1	*/	{PROTO_MJXQ	,	WLH08		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	2	*/	{PROTO_MJXQ	,	X600		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	3	*/	{PROTO_MJXQ	,	X800		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	4	*/	{PROTO_MJXQ	,	H26D		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	5	*/	{PROTO_MJXQ	,	E010		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	6	*/	{PROTO_MJXQ	,	H26WH		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	7	*/	{PROTO_HONTAI,	HONTAI		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	8	*/	{PROTO_HONTAI,	JJRCX1		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	9	*/	{PROTO_HONTAI,	X5C1		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	10	*/	{PROTO_HONTAI,	FQ777_951	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	11	*/	{PROTO_Q303	,	Q303		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	12	*/	{PROTO_Q303	,	CX35		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	13	*/	{PROTO_Q303	,	CX10D		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	14	*/	{PROTO_Q303	,	CX10WD		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
#endif
#if NBR_BANKS > 4
//******************************       BANK 5       ******************************
//	Switch	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option	Chan Order
/*	1	*/	{PROTO_CX10	,	CX10_GREEN	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	2	*/	{PROTO_CX10	,	CX10_BLUE	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	3	*/	{PROTO_CX10	,	DM007		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	4	*/	{PROTO_CX10	,	JC3015_1	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	5	*/	{PROTO_CX10	,	JC3015_2	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	6	*/	{PROTO_CX10	,	MK33041		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	7	*/	{PROTO_Q2X2	,	Q222		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	8	*/	{PROTO_Q2X2	,	Q242		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	9	*/	{PROTO_Q2X2	,	Q282		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	10	*/	{PROTO_CG023,	CG023		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	11	*/	{PROTO_CG023,	YD829		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	12	*/	{PROTO_FQ777,	NONE		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	13	*/	{PROTO_YD717,	YD717		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
/*	14	*/	{PROTO_MT99XX,	MT99		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0	,	0x00000000 },
#endif
};
// RX_Num is used for TX & RX match. Using different RX_Num values for each receiver will prevent starting a model with the false config loaded...
// RX_Num value is between 0 and 15.

// Power P_HIGH or P_LOW: High or low power setting for the transmission.
// For indoor P_LOW is more than enough.

// Auto Bind	AUTOBIND or NO_AUTOBIND
// For protocols which does not require binding at each power up (like Flysky, FrSky...), you might still want a bind to be initiated each time you power up the TX.
// As an example, it's usefull for the WLTOYS F929/F939/F949/F959 (all using the Flysky protocol) which requires a bind at each power up.
// It also enables the Bind from channel feature, allowing to execute a bind by toggling a designated channel.

// Option: the value is between -128 and +127.
// The option value is only valid for some protocols, read this page for more information: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/Protocols_Details.md

// Chan order: if the value is different from 0, this setting will remap the first 8 channels in any given order before giving them to the protocol.
// It does not disable the automatic channel remapping of the protocol itself but changes the input of it.
// Even if your TX is sending less than 8 channels you have to respect the format like if it was.
// Examples:
//  - 0x12345678 will give to the protocol the channels in the order 1,2,3,4,5,6,7,8 which is equivalent to 0x00000000.
//  - 0x42315678 will give to the protocol the channels in the order 4,2,3,1,5,6,7,8 swapping channel 1 and 4.
//  - 0x40010000 will give to the protocol the channels in the order 4,2,3,1,5,6,7,8 swapping channel 1 and 4. Note: 0 means leave the channel where it is.
//  - 0x0000ABCD will give to the protocol the channels in the order 1,2,3,4,10,11,12,13 which potentially enables acces to channels not available on your TX. Note A=10,B=11,C=12,D=13,E=14,F=15.

/* Available protocols and associated sub protocols to pick and choose from (Listed in alphabetical order)
	PROTO_AFHDS2A
		PWM_IBUS
		PPM_IBUS
		PWM_SBUS
		PPM_SBUS
	PROTO_AFHDS2A_RX
		NONE
	PROTO_ASSAN
		NONE
	PROTO_BAYANG
		BAYANG
		H8S3D
		X16_AH
		IRDRONE
		DHD_D4
	PROTO_BAYANG_RX
		NONE
	PROTO_BUGS
		NONE
	PROTO_BUGSMINI
		BUGSMINI
		BUGS3H
	PROTO_CABELL
		CABELL_V3
		CABELL_V3_TELEMETRY
		CABELL_SET_FAIL_SAFE
		CABELL_UNBIND
	PROTO_CFLIE
		NONE
	PROTO_CG023
		CG023
		YD829
	PROTO_CORONA
		COR_V1
		COR_V2
		FD_V3
	PROTO_CX10
		CX10_GREEN
		CX10_BLUE
		DM007
		JC3015_1
		JC3015_2
		MK33041
	PROTO_DEVO
		NONE
	PROTO_DM002
		NONE
	PROTO_DSM
		DSM2_22
		DSM2_11
		DSMX_22
		DSMX_11
	PROTO_E01X
		E012
		E015
		E016H
	PROTO_ESKY
		ESKY_STD
		ESKY_ET4
	PROTO_ESKY150
		ESKY150_4CH
		ESKY150_7CH
	PROTO_ESKY150V2
		NONE
	PROTO_FLYSKY
		Flysky
		V9X9
		V6X6
		V912
		CX20
	PROTO_FLYZONE
		FZ410
	PROTO_FQ777
		NONE
	PROTO_FRSKY_RX
		FRSKY_RX
		FRSKY_CLONE
	PROTO_FRSKYD
		FRSKYD
		DCLONE
	PROTO_FRSKYL
		LR12
		LR12_6CH
	PROTO_FRSKYR9
		R9_915
		R9_868
		R9_915_8CH
		R9_868_8CH
	PROTO_FRSKYV
		NONE
	PROTO_FRSKYX
		CH_16
		CH_8
		EU_16
		EU_8
		XCLONE
	PROTO_FRSKYX2
		CH_16
		CH_8
		EU_16
		EU_8
		XCLONE
	PROTO_FRSKY_RX
		FRSKY_RX
		FRSKY_CLONE
	PROTO_FX816
		NONE
	PROTO_FY326
		FY326
		FY319
	PROTO_GD00X
		GD_V1
		GD_V2
	PROTO_GW008
		NONE
	PROTO_H8_3D
		H8_3D
		H20H
		H20MINI
		H30MINI
	PROTO_HISKY
		Hisky
		HK310
	PROTO_HITEC
		OPT_FW
		OPT_HUB
		MINIMA
	PROTO_HONTAI
		HONTAI
		JJRCX1
		X5C1
		FQ777_951
	PROTO_HOTT
		NONE
	PROTO_HUBSAN
		H107
		H301
		H501
	PROTO_J6PRO
		NONE
	PROTO_KF606
		NONE
	PROTO_KN
		WLTOYS
		FEILUN
	PROTO_MJXQ
		WLH08
		X600
		X800
		H26D
		E010
		H26WH
		PHOENIX
	PROTO_MT99XX
		MT99
		H7
		YZ
		LS
		FY805
	PROTO_NCC1701
		NONE
	PROTO_PELIKAN
		NONE
	PROTO_POTENSIC
		NONE
	PROTO_PROPEL
		NONE
	PROTO_Q2X2
		Q222
		Q242
		Q282
	PROTO_Q303
		Q303
		CX35
		CX10D
		CX10WD
	PROTO_REDPINE
		RED_FAST
		RED_SLOW
	PROTO_SCANNER
		NONE
	PROTO_SFHSS
		NONE
	PROTO_SHENQI
		NONE
	PROTO_SKYARTEC
		NONE
	PROTO_SLT
		SLT_V1
		SLT_V2
		Q100
		Q200
		MR100
	PROTO_SYMAX
		SYMAX
		SYMAX5C
	PROTO_TIGER
		NONE
	PROTO_TRAXXAS
		RX6519
	PROTO_V2X2
		V2X2
		JXD506
	PROTO_V761
		NONE
	PROTO_V911S
		V911S_STD
		V911S_E119
	PROTO_WFLY
		NONE
	PROTO_WK2x01
		WK2801
		WK2401
		W6_5_1
		W6_6_1
		W6_HEL
		W6_HEL_I
	PROTO_XK
		X450
		X420
	PROTO_YD717
		YD717
		SKYWLKR
		SYMAX4
		XINXUN
		NIHUI
	PROTO_ZSX
		NONE
*/

/**********************************/
/*** DIRECT INPUTS SETTINGS ***/
/**********************************/
//In this section you can configure the direct inputs.
//It enables switches wired directly to the board
//Direct inputs works only in ppm mode and only for stm_32 boards
//Uncomment following lines to enable derect inputs or define your own configuration in _MyConfig.h
/*
#define ENABLE_DIRECT_INPUTS
		
#define DI1_PIN				PC13	
#define IS_DI1_on			(digitalRead(DI1_PIN)==LOW)

#define DI2_PIN				PC14	
#define IS_DI2_on			(digitalRead(DI2_PIN)==LOW)

#define DI3_PIN				PC15	
#define IS_DI3_on			(digitalRead(DI3_PIN)==LOW)

//Define up to 4 direct input channels
//CHANNEL1 - 2pos switch
#define DI_CH1_read			IS_DI1_on ? PPM_MAX_100*2 : PPM_MIN_100*2
//CHANNEL2 - 3pos switch
#define DI_CH2_read			IS_DI2_on ? PPM_MAX_100*2 : (IS_DI2_on ? PPM_MAX_100 + PPM_MIN_100 : PPM_MIN_100*2)
*/
