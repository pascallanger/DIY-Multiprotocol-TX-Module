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

//Uncomment your TX type
#define TX_ER9X_AETR	//ER9X AETR (988<->2012µs)
//#define TX_ER9X_TAER	//ER9X TAER (988<->2012µs)
//#define TX_DEVO7		//DEVO7 EATR (1120<->1920µs)
//#define TX_SPEKTRUM	//Spektrum TAER (1100<->1900µs)
//#define TX_HISKY		//HISKY AETR (1100<->1900µs)

//Comment if a module is not installed
#define A7105_INSTALLED
#define CYRF6936_INSTALLED
#define CC2500_INSTALLED
#define NFR24L01_INSTALLED

//Comment a protocol to exclude it from compilation
#ifdef	A7105_INSTALLED
	#define	FLYSKY_A7105_INO
	#define	HUBSAN_A7105_INO
#endif
#ifdef	CYRF6936_INSTALLED
	#define	DEVO_CYRF6936_INO
	#define	DSM2_CYRF6936_INO
#endif
#ifdef	CC2500_INSTALLED
	#define	FRSKY_CC2500_INO
	#define	FRSKYX_CC2500_INO
	#define SFHSS_CC2500_INO
#endif
#ifdef	NFR24L01_INSTALLED
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
#endif

//Uncomment to enable telemetry
#define TELEMETRY

//Comment to disable a specific telemetry
#if defined(TELEMETRY)
	#if defined DSM2_CYRF6936_INO
		#define DSM_TELEMETRY	
	#endif
	#if defined FRSKYX_CC2500_INO
		#define SPORT_TELEMETRY	
	#endif
	#if defined FRSKY_CC2500_INO
		#define HUB_TELEMETRY
	#endif
#endif 

//Update this table to set which protocol and all associated settings are called for the corresponding dial number
const PPM_Parameters PPM_prot[15]=	{
//	Dial	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option
/*	1	*/	{MODE_FLYSKY,	Flysky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	2	*/	{MODE_HUBSAN,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	3	*/	{MODE_FRSKY	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0xD7	},	// D7 fine tuning
/*	4	*/	{MODE_HISKY	,	Hisky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	5	*/	{MODE_V2X2	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},
/*	6	*/	{MODE_DSM2	,	DSM2		,	0	,	P_HIGH	,	NO_AUTOBIND	,	6		},	// 6 channels @ 11ms
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
	MODE_FRSKY
		NONE
	MODE_HISKY
		Hisky
		HK310
	MODE_V2X2
		NONE
	MODE_DSM2
		DSM2
		DSMX
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
	MODE_MJXQ
		WLH08
		X600
		X800
		H26D
	MODE_SHENQI
		NONE
	MODE_FY326
		NONE
	MODE_SFHSS
		NONE

RX_Num 		value between 0 and 15

Power		P_HIGH or P_LOW

Auto Bind	AUTOBIND or NO_AUTOBIND

Option		value between 0 and 255. 0xD7 or 0x00 for Frsky fine tuning.
*/

//******************
//TX definitions with timing endpoints and channels order

// Turnigy PPM and channels
#if defined(TX_ER9X_AETR)
#define PPM_MAX		2140	//	125%
#define PPM_MIN		860		//	125%
#define PPM_MAX_100 2012	//	100%
#define PPM_MIN_100 988		//	100%
enum chan_order{
	AILERON =0,
	ELEVATOR,
	THROTTLE,
	RUDDER,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	AUX5,
	AUX6,
	AUX7,
	AUX8,
	AUX9
};
#endif

// Turnigy PPM and channels
#if defined(TX_ER9X_TAER)
#define PPM_MAX		2140	//	125%
#define PPM_MIN		860		//	125%
#define PPM_MAX_100 2012	//	100%
#define PPM_MIN_100 988		//	100%
enum chan_order{
	THROTTLE =0,
	AILERON,
	ELEVATOR,
	RUDDER,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	AUX5,
	AUX6,
	AUX7,
	AUX8,
	AUX9
};
#endif

// Devo PPM and channels
#if defined(TX_DEVO7)
#define PPM_MAX		2100	//	125%
#define PPM_MIN		900		//	125%
#define PPM_MAX_100	1920	//	100%
#define PPM_MIN_100	1120	//	100%
enum chan_order{
	ELEVATOR=0,
	AILERON,
	THROTTLE,
	RUDDER,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	AUX5,
	AUX6,
	AUX7,
	AUX8,
	AUX9
};
#endif

// SPEKTRUM PPM and channels
#if defined(TX_SPEKTRUM)
#define PPM_MAX		2000	//	125%
#define PPM_MIN		1000	//	125%
#define PPM_MAX_100	1900	//	100%
#define PPM_MIN_100	1100	//	100%
enum chan_order{
	THROTTLE=0,
	AILERON,
	ELEVATOR,
	RUDDER,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	AUX5,
	AUX6,
	AUX7,
	AUX8,
	AUX9
};
#endif

// HISKY
#if defined(TX_HISKY)
#define PPM_MAX		2000	//	125%
#define PPM_MIN		1000	//	125%
#define PPM_MAX_100	1900	//	100%
#define PPM_MIN_100	1100	//	100%
enum chan_order{
	AILERON =0,
	ELEVATOR,
	THROTTLE,
	RUDDER,
	AUX1,
	AUX2,
	AUX3,
	AUX4,
	AUX5,
	AUX6,
	AUX7,
	AUX8,
	AUX9
};
#endif

#define PPM_MIN_COMMAND 1250
#define PPM_SWITCH		1550
#define PPM_MAX_COMMAND 1750

//Uncoment the desired serial speed
#define BAUD 100000
//#define BAUD 125000
