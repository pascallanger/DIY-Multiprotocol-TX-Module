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
#define TX_ER9X			//ER9X AETR (988<->2012µs)
//#define TX_DEVO7		//DEVO7 EATR (1120<->1920µs)
//#define TX_SPEKTRUM	//Spektrum TAER (1100<->1900µs)
//#define TX_HISKY		//HISKY AETR (1100<->1900µs)

//Uncomment to enable telemetry
#define TELEMETRY
#define HUB_TELEMETRY

//Comment a protocol to exclude it from compilation
//A7105 protocols
#define	FLYSKY_A7105_INO
#define	HUBSAN_A7105_INO
//CYRF6936 protocols
#define	DEVO_CYRF6936_INO
#define	DSM2_CYRF6936_INO
//CC2500 protocols
#define	FRSKY_CC2500_INO
//#define	FRSKYX_CC2500_INO
//NFR24L01 protocols
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

//Update this table to set which protocol and all associated settings are called for the corresponding dial number
static const PPM_Parameters PPM_prot[15]=
{
//	Protocol 		Sub protocol	RX_Num	Power		Auto Bind		Option
	{MODE_FLYSKY,	Flysky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=1
	{MODE_HUBSAN,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=2
	{MODE_FRSKY	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0xD7	},	//Dial=3
	{MODE_HISKY	,	Hisky		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=4
	{MODE_V2X2	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=5
	{MODE_DSM2	,	DSM2		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=6
	{MODE_DEVO	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=7
	{MODE_YD717	,	YD717		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=8
	{MODE_KN	,	WLTOYS		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=9
	{MODE_SYMAX	,	SYMAX		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=10
	{MODE_SLT	,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=11
	{MODE_CX10	,	CX10_BLUE	,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=12
	{MODE_CG023	,	CG023		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=13
	{MODE_BAYANG,	0			,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		},	//Dial=14
	{MODE_SYMAX	,	SYMAX5C		,	0	,	P_HIGH	,	NO_AUTOBIND	,	0		}	//Dial=15
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
		NONE
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

RX_Num 		value between 0 and 15

Power		P_HIGH or P_LOW

Auto Bind	AUTOBIND or NO_AUTOBIND

Option		value between 0 and 255. 0xD7 or 0x00 for Frsky fine tuning.
*/

//******************
//TX definitions with timing endpoints and channels order

// Turnigy PPM and channels
#if defined(TX_ER9X)
#define PPM_MAX		2140
#define PPM_MIN		860
#define PPM_MAX_100 2012
#define PPM_MIN_100 988
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

// Devo PPM and channels
#if defined(TX_DEVO7)
#define PPM_MAX		2100
#define PPM_MIN		900
#define PPM_MAX_100	1920
#define PPM_MIN_100	1120
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
#define PPM_MAX		2000
#define PPM_MIN		1000
#define PPM_MAX_100	1900
#define PPM_MIN_100	1100
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
#define PPM_MAX		2000
#define PPM_MIN		1000
#define PPM_MAX_100	1900
#define PPM_MIN_100	1100
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
