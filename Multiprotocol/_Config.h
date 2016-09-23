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



/** Multiprotocol module configuration file ***/

/*******************/
/*** TX SETTINGS ***/
/*******************/
//Modify the channel order based on your TX: AETR, TAER, RETA...
//For example a JR/Spektrum radio is TAER. Default is AETR.
#define AETR

//Comment if a module is not installed
#define A7105_INSTALLED
#define CYRF6936_INSTALLED
#define CC2500_INSTALLED
#define NRF24L01_INSTALLED

//The protocols below need an A7105 to be installed
#define	FLYSKY_A7105_INO
#define	HUBSAN_A7105_INO

//The protocols below need a CYRF6936 to be installed
#define	DEVO_CYRF6936_INO
#define	DSM_CYRF6936_INO
#define J6PRO_CYRF6936_INO

//The protocols below need a CC2500 to be installed
#define	FRSKYD_CC2500_INO
#define	FRSKYV_CC2500_INO
#define	FRSKYX_CC2500_INO
#define SFHSS_CC2500_INO

//The protocols below need a NRF24L01 to be installed
#define	BAYANG_NRF24L01_INO
#define	CG023_NRF24L01_INO
#define	CX10_NRF24L01_INO
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


/**************************/
/*** TELEMETRY SETTINGS ***/
/**************************/
//In this section you can configure the telemetry.

//Uncomment to enable telemetry
#define TELEMETRY

//Uncomment to invert the polarity of the telemetry serial signal.
//For ER9X and ERSKY9X it must be commented. For OpenTX it must be uncommented.
//#define INVERT_TELEMETRY	
//Comment to disable a specific telemetry
#define DSM_TELEMETRY	
#define SPORT_TELEMETRY	
#define HUB_TELEMETRY




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
#define TX_ER9X			//ER9X/ERSKY9X/OpenTX	( 988<->2012Âµs)
//#define TX_DEVO7		//DEVO					(1120<->1920Âµs)
//#define TX_SPEKTRUM	//Spektrum				(1100<->1900Âµs)
//#define TX_HISKY		//HISKY					(1100<->1900Âµs)
//#define TX_MPX		//Multiplex MC2020		(1250<->1950Âµs)
//#define TX_CUSTOM		//Custom

// The lines below are used to set the end points in microseconds (µs) if you have selected TX_CUSTOM.
// A few things to considered:
//  - If you put too big values compared to your TX you won't be able to reach the extremes which is bad for throttle as an example
//  - If you put too low values you won't be able to use your full stick range, it will be maxed out before reaching the end
//  - Centered stick value is usually 1500. It should match the middle between MIN and MAX, ie Center=(MAX-MIN)/2+MIN. If your TX is not centered you can adjust the value MIN or MAX.
//  - 100% is the value when the model is by default, 125% is the value when you extend the servo travel which is only used by some protocols
#if defined(TX_CUSTOM)
#define PPM_MAX_100	1900	//	100%
#define PPM_MIN_100	1100	//	100%
#define PPM_MAX_125	2000	//	125%
#define PPM_MIN_125	1000	//	125%
#endif

/*
struct PPM_Parameters
{
	uint8_t protocol : 5;
	uint8_t sub_proto : 3;
	uint8_t rx_num : 4;
	uint8_t power : 1;
	uint8_t autobind : 1;
	uint8_t option;
};
*/
//Update this table to set which protocol and all associated settings are called for the corresponding dial number
const PPM_Parameters PPM_prot[15]=	{
	//	Dial	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option
	/*	1	*/	{MODE_FLYSKY,	Flysky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	2	*/	{MODE_HUBSAN,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	3	*/	{MODE_FRSKYD,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	40		},	// option=fine freq tuning
	/*	4	*/	{MODE_HISKY	,	Hisky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	5	*/	{MODE_V2X2	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	6	*/	{MODE_DSM	,	DSM2_22		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6		},	// option=number of channels
	/*	7	*/	{MODE_DEVO	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	8	*/	{MODE_YD717	,	YD717		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	9	*/	{MODE_KN	,	WLTOYS		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	10	*/	{MODE_SYMAX	,	SYMAX		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	11	*/	{MODE_SLT	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	12	*/	{MODE_CX10	,	CX10_BLUE	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	13	*/	{MODE_CG023	,	CG023		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	14	*/	{MODE_BAYANG,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
	/*	15	*/	{MODE_SYMAX	,	SYMAX5C		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		}
};
/* Available protocols and associated sub protocols:
	MODE_FLYSKY
		Flysky
		V9X9
		V6X6
		V912
	MODE_HUBSAN
		NONE
	MODE_FRSKYD
		NONE
	MODE_HISKY
		Hisky
		HK310
	MODE_V2X2
		NONE
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
		Q282
		JC3015_1
		JC3015_2
		MK33041
		Q242
	MODE_CG023
		CG023
		YD829
		H8_3D
	MODE_BAYANG
		NONE
	MODE_FRSKYX
		CH_16
		CH_8
	MODE_ESKY
		NONE
	MODE_MT99XX
		MT99
		H7
		YZ
		LS
	MODE_MJXQ
		WLH08
		X600
		X800
		H26D
		E010
	MODE_SHENQI
		NONE
	MODE_FY326
		NONE
	MODE_SFHSS
		NONE
	MODE_J6PRO
		NONE
	MODE_FQ777
		NONE
	MODE_ASSAN
		NONE	
	MODE_FRSKYV
		NONE
	MODE_HONTAI
		FORMAT_HONTAI
		FORMAT_JJRCX1
		FORMAT_X5C1


RX_Num 		value between 0 and 15

Power		P_HIGH or P_LOW

Auto Bind	AUTOBIND or NO_AUTOBIND

Option		value between -127 and 127. 
*/
