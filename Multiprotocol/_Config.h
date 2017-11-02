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


/*************************/
/*** BIND FROM CHANNEL ***/
/*************************/
//Bind from channel enables you to bind when a specified channel is giong from low to high. This feature is only active
// if you specify AUTOBIND in PPM mode or set AutoBind to YES for serial mode. It also requires that the throttle channel is low.

//Comment to globaly disable the bind feature from a channel.
#define ENABLE_BIND_CH

//Set the channel number used for bind. Default is 16.
#define BIND_CH	16

//Comment to disable the wait for bind feature. This feature will not activate the selected
// protocol unless a bind is requested using bind from channel or the GUI "Bind" button.
//The goal is to prevent binding other people's model when powering up the TX, changing model or scanning through protocols.
#define WAIT_FOR_BIND


/****************/
/*** RF CHIPS ***/
/****************/
//There are 4 RF components supported. If one of them is not installed you must comment it using "//".
//If a chip is not installed all associated protocols are disabled.
//4-in-1 modules have all RF chips installed
//!!!If a RF chip is present it MUST be marked as installed!!! or weird things will happen you have been warned.
#define A7105_INSTALLED
#define CYRF6936_INSTALLED
#define CC2500_INSTALLED
#define NRF24L01_INSTALLED

//Low power is reducing the transmit power of the multi module. This setting is configurable per model in PPM (table below) or Serial mode (radio GUI).
//It can be activated when flying indoor or small models since the distance is short or if a model is causing issues when flying closed to the TX.
//By default low power is completly disabled on all rf chips to prevent mistakes, but you can enable it by uncommenting the lines below: 
//#define A7105_ENABLE_LOW_POWER
//#define CYRF6936_ENABLE_LOW_POWER
//#define CC2500_ENABLE_LOW_POWER
//#define NRF24L01_ENABLE_LOW_POWER


/*****************/
/*** GLOBAL ID ***/
/*****************/
//A global ID is used by most protocols to bind and retain the bind to models. To prevent duplicate IDs, it is automatically
// generated using a random 32 bits number the first time the eeprom is initialized.
//If you have 2 Multi modules which you want to share the same ID so you can use either to control the same RC model
// then you can force the ID to a certain known value using the lines below.
//Default is commented, you should uncoment only for test purpose or if you know exactly what you are doing!!!
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
#define	FLYSKY_A7105_INO
#define	HUBSAN_A7105_INO
#define	AFHDS2A_A7105_INO

//The protocols below need a CYRF6936 to be installed
#define	DEVO_CYRF6936_INO
#define	DSM_CYRF6936_INO
#define	J6PRO_CYRF6936_INO
#define	WK2x01_CYRF6936_INO

//The protocols below need a CC2500 to be installed
#define	FRSKYV_CC2500_INO
#define	FRSKYD_CC2500_INO
#define	FRSKYX_CC2500_INO
#define	SFHSS_CC2500_INO

//The protocols below need a NRF24L01 to be installed
#define	BAYANG_NRF24L01_INO
#define	CG023_NRF24L01_INO
#define	CX10_NRF24L01_INO		// Include Q2X2 protocol
#define	ESKY_NRF24L01_INO
#define	HISKY_NRF24L01_INO
#define	KN_NRF24L01_INO
#define	SLT_NRF24L01_INO
#define	SYMAX_NRF24L01_INO
#define	V2X2_NRF24L01_INO
#define	YD717_NRF24L01_INO
#define	MT99XX_NRF24L01_INO
#define	MJXQ_NRF24L01_INO
#define	SHENQI_NRF24L01_INO
#define	FY326_NRF24L01_INO
#define	FQ777_NRF24L01_INO
#define	ASSAN_NRF24L01_INO
#define	HONTAI_NRF24L01_INO
#define Q303_NRF24L01_INO
#define GW008_NRF24L01_INO
#define DM002_NRF24L01_INO

/**************************/
/*** TELEMETRY SETTINGS ***/
/**************************/
//In this section you can configure the telemetry.

//If you do not plan using the telemetry comment this global setting using "//" and skip to the next section.
#define TELEMETRY

//Comment to invert the polarity of the output telemetry serial signal.
//This function takes quite some flash space and processor power on an atmega.
//For OpenTX it must be uncommented.
//On a 9XR_PRO running ersky9x both commented and uncommented will work depending on the radio setting Invert COM1 under the Telemetry menu.
//On other addon/replacement boards like the 9xtreme board or the Ar9x board running ersky9x, you need to uncomment the line below.
//For er9x it depends if you have an inveter mod or not on the telemetry pin. If you don't have an inverter comment this line.
//#define INVERT_TELEMETRY

//Comment if you don't want to send Multi status telemetry frames (Protocol available, Bind in progress, version...)
//Use with er9x/erksy9x, for OpenTX MULTI_TELEMETRY below is preferred instead
#define MULTI_STATUS

//Uncomment to send Multi status and allow OpenTX to autodetect the telemetry format
//Supported by OpenTX version 2.2 RC9 and newer. NOT supported by er9x/ersky9x use MULTI_STATUS instead.
//#define MULTI_TELEMETRY

//Comment a line to disable a specific protocol telemetry
#define DSM_TELEMETRY				// Forward received telemetry packet directly to TX to be decoded
#define SPORT_TELEMETRY				// Use FrSkyX SPORT format to send telemetry to TX
#define AFHDS2A_FW_TELEMETRY		// Forward received telemetry packet directly to TX to be decoded
#define HUB_TELEMETRY				// Use FrSkyD Hub format to send telemetry to TX
#define AFHDS2A_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define BAYANG_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX
#define HUBSAN_HUB_TELEMETRY		// Use FrSkyD Hub format to send telemetry to TX


/****************************/
/*** SERIAL MODE SETTINGS ***/
/****************************/
//In this section you can configure the serial mode.
//The serial mode enables full editing of all the parameters in the GUI of the radio.
//This is available natively for ER9X and ERSKY9X. It is available for OpenTX on Taranis with a special version.

//If you do not plan to use the Serial mode comment this line using "//" to save Flash space
#define ENABLE_SERIAL


/*************************/
/*** PPM MODE SETTINGS ***/
/*************************/
//In this section you can configure all details about PPM.
//If you do not plan to use the PPM mode comment this line using "//" to save Flash space, you don't need to configure anything below in this case
#define ENABLE_PPM

/*** TX END POINTS ***/
//It is important for the module to know the endpoints of your radio.
//Below are some standard transmitters already preconfigured.
//Uncomment only the one which matches your transmitter.
#define TX_ER9X			//ER9X/ERSKY9X/OpenTX	( 988<->2012µs)
//#define TX_DEVO7		//DEVO					(1120<->1920µs)
//#define TX_SPEKTRUM	//Spektrum				(1100<->1900µs)
//#define TX_HISKY		//HISKY					(1120<->1920µs)
//#define TX_MPX		//Multiplex MC2020		(1250<->1950µs)
//#define TX_WALKERA	//Walkera PL0811-01H	(1000<->1800µs)
//#define TX_CUSTOM		//Custom

// The lines below are used to set the end points in microseconds (µs) if you have selected TX_CUSTOM.
// A few things to consider:
//  - If you put too big values compared to your TX you won't be able to reach the extremes which is bad for throttle as an example
//  - If you put too low values you won't be able to use your full stick range, it will be maxed out before reaching the ends
//  - Centered stick value is usually 1500. It should match the middle between MIN and MAX, ie Center=(MAX-MIN)/2+MIN. If your TX is not centered you can adjust the value MIN or MAX.
//  - 100% is the value when the model is by default, 125% is the value when you extend the servo travel which is only used by some protocols
#if defined(TX_CUSTOM)
	#define PPM_MAX_100	1900	//	100%
	#define PPM_MIN_100	1100	//	100%
	#define PPM_MAX_125	2000	//	125%
	#define PPM_MIN_125	1000	//	125%
	
	#define PPM_MAP		1		// MAP PPM to SERIAL
	#define PPM_CHG		5		// channel for switch ELEVATOR / RUDDER
	#define PPM_CHG_A	ELEVATOR		// channel for switch ELEVATOR / RUDDER
	#define PPM_CHG_B	AILERON		// channel for switch ELEVATOR / RUDDER
#endif

// The line below is used to set the minimum number of channels which the module should receive to consider a PPM frame valid.
// The default value is 4 to receive at least AETR for flying models but you could also connect the PPM from a car radio which has only 3 channels by changing this number to 3.
#define MIN_PPM_CHANNELS 4
// The line below is used to set the maximum number of channels which the module should work with. Any channels received above this number are discarded.
// The default value is 16 to receive all possible channels but you might want to filter some "bad" channels from the PPM frame like the ones above 6 on the Walkera PL0811.
#define MAX_PPM_CHANNELS 16

//The table below indicates which protocol to run when a specific position on the dial has been selected.
//All fields and values are explained below. Everything is configurable from here like in the Serial mode.
//Example: You can associate multiple times the same protocol to different dial positions to take advantage of the model match (RX_Num)
const PPM_Parameters PPM_prot[15]=	{
//	Dial	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option
/*	1	*/	{MODE_FLYSKY,	Flysky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	2	*/	{MODE_HUBSAN,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	3	*/	{MODE_FRSKYD,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	40		},	// option=fine freq tuning
/*	4	*/	{MODE_HISKY	,	Hisky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	5	*/	{MODE_V2X2	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	6	*/	{MODE_DSM	,	DSMX_11		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6		},	// option=number of channels
/*	7	*/	{MODE_DSM	,	DSM2_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6		},
/*	8	*/	{MODE_YD717	,	YD717		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	9	*/	{MODE_KN	,	WLTOYS		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	10	*/	{MODE_SYMAX	,	SYMAX		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	11	*/	{MODE_SLT	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	12	*/	{MODE_CX10	,	CX10_BLUE	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	13	*/	{MODE_CG023	,	CG023		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	14	*/	{MODE_BAYANG,	BAYANG		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	15	*/	{MODE_SYMAX	,	SYMAX5C		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		}
};
/* Available protocols and associated sub protocols to pick and choose from
	MODE_FLYSKY
		Flysky
		V9X9
		V6X6
		V912
		CX20
	MODE_HUBSAN
		NONE
	MODE_FRSKYV
		NONE
	MODE_FRSKYD
		NONE
	MODE_FRSKYX
		CH_16
		CH_8
		EU_16
		EU_8
	MODE_HISKY
		Hisky
		HK310
	MODE_V2X2
		V2X2
		JXD506
	MODE_DSM
		DSM2_22
		DSM2_11
		DSMX_22
		DSMX_11
	MODE_DEVO
		NONE
	MODE_YD717
		YD717
		SKYWLKR
		SYMAX4
		XINXUN
		NIHUI
	MODE_KN
		WLTOYS
		FEILUN
	MODE_SYMAX
		SYMAX
		SYMAX5C
	MODE_SLT
		NONE
	MODE_CX10
		CX10_GREEN
		CX10_BLUE
		DM007
		JC3015_1
		JC3015_2
		MK33041
	MODE_Q2X2
		Q222
		Q242
		Q282
	MODE_SLT
		SLT
		VISTA
	MODE_CG023
		CG023
		YD829
		H8_3D
	MODE_BAYANG
		BAYANG
		H8S3D
	MODE_ESKY
		NONE
	MODE_MT99XX
		MT99
		H7
		YZ
		LS
		FY805
	MODE_MJXQ
		WLH08
		X600
		X800
		H26D
		E010
		H26WH
	MODE_SHENQI
		NONE
	MODE_FY326
		FY326
		FY319
	MODE_SFHSS
		NONE
	MODE_J6PRO
		NONE
	MODE_FQ777
		NONE
	MODE_ASSAN
		NONE
	MODE_HONTAI
		FORMAT_HONTAI
		FORMAT_JJRCX1
		FORMAT_X5C1
		FORMAT_FQ777_951
	MODE_AFHDS2A
		PWM_IBUS
		PPM_IBUS
		PWM_SBUS
		PPM_SBUS
	MODE_WK2X01
		WK2801
		WK2401
		W6_5_1
		W6_6_1
		W6_HEL
		W6_HEL_I
	MODE_Q303
		Q303
		CX35
		CX10D
		CX10WD
	MODE_GW008
		NONE
	MODE_DM002
		NONE
*/

// RX_Num is used for model match. Using RX_Num	values different for each receiver will prevent starting a model with the false config loaded...
// RX_Num value is between 0 and 15.

// Power P_HIGH or P_LOW: High or low power setting for the transmission.
// For indoor P_LOW is more than enough.

// Auto Bind	AUTOBIND or NO_AUTOBIND
// For protocols which does not require binding at each power up (like Flysky, FrSky...), you might still want a bind to be initiated each time you power up the TX.
// As an example, it's usefull for the WLTOYS F929/F939/F949/F959 (all using the Flysky protocol) which requires a bind at each power up.
// It also enables the Bind from channel feature, allowing to execute a bind by toggling a designated channel.

// Option: the value is between -128 and +127.
// The option value is only valid for some protocols, read this page for more information: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/Protocols_Details.md
