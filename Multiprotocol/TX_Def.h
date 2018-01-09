// Turnigy PPM and channels
#if defined(TX_ER9X)
	#define PPM_MAX_100 2012	//	100%
	#define PPM_MIN_100 988		//	100%
#endif

// Devo PPM and channels
#if defined(TX_DEVO7)
	#define PPM_MAX_100	1920	//	100%
	#define PPM_MIN_100	1120	//	100%
#endif

// SPEKTRUM PPM and channels
#if defined(TX_SPEKTRUM)
	#define PPM_MAX_100	1900	//	100%
	#define PPM_MIN_100	1100	//	100%
#endif

// HISKY
#if defined(TX_HISKY)
	#define PPM_MAX_100	1920	//	100%
	#define PPM_MIN_100	1120	//	100%
#endif

// Multiplex MC2020
#if defined(TX_MPX)
	#define PPM_MAX_100 1950 // 100%
	#define PPM_MIN_100 1250 // 100%
#endif

// Walkera PL0811-01H
#if defined(TX_WALKERA)
	#define PPM_MAX_100 1800 // 100%
	#define PPM_MIN_100 1000 // 100%
#endif

//Channel MIN MAX values
#define CHANNEL_MAX_100	1844	//	100%
#define CHANNEL_MIN_100	204		//	100%
#define CHANNEL_MAX_125	2047	//	125%
#define CHANNEL_MIN_125	0		//	125%

#define CHANNEL_MIN_COMMAND 784		// 1350us
#define CHANNEL_SWITCH		1104	// 1550us
#define CHANNEL_MAX_COMMAND 1424	// 1750us

//Channel definitions
#ifdef AETR
	#define	AILERON  0
	#define	ELEVATOR 1
	#define	THROTTLE 2
	#define	RUDDER   3
#endif
#ifdef AERT
	#define	AILERON  0
	#define	ELEVATOR 1
	#define	THROTTLE 3
	#define	RUDDER   2
#endif
#ifdef ARET
	#define	AILERON  0
	#define	ELEVATOR 2
	#define	THROTTLE 3
	#define	RUDDER   1
#endif
#ifdef ARTE
	#define	AILERON  0
	#define	ELEVATOR 3
	#define	THROTTLE 2
	#define	RUDDER   1
#endif
#ifdef ATRE
	#define	AILERON  0
	#define	ELEVATOR 3
	#define	THROTTLE 1
	#define	RUDDER   2
#endif
#ifdef ATER
	#define	AILERON  0
	#define	ELEVATOR 2
	#define	THROTTLE 1
	#define	RUDDER   3
#endif

#ifdef EATR
	#define	AILERON  1
	#define	ELEVATOR 0
	#define	THROTTLE 2
	#define	RUDDER   3
#endif
#ifdef EART
	#define	AILERON  1
	#define	ELEVATOR 0
	#define	THROTTLE 3
	#define	RUDDER   2
#endif
#ifdef ERAT
	#define	AILERON  2
	#define	ELEVATOR 0
	#define	THROTTLE 3
	#define	RUDDER   1
#endif
#ifdef ERTA
	#define	AILERON  3
	#define	ELEVATOR 0
	#define	THROTTLE 2
	#define	RUDDER   1
#endif
#ifdef ETRA
	#define	AILERON  3
	#define	ELEVATOR 0
	#define	THROTTLE 1
	#define	RUDDER   2
#endif
#ifdef ETAR
	#define	AILERON  2
	#define	ELEVATOR 0
	#define	THROTTLE 1
	#define	RUDDER   3
#endif

#ifdef TEAR
	#define	AILERON  2
	#define	ELEVATOR 1
	#define	THROTTLE 0
	#define	RUDDER   3
#endif
#ifdef TERA
	#define	AILERON  3
	#define	ELEVATOR 1
	#define	THROTTLE 0
	#define	RUDDER   2
#endif
#ifdef TREA
	#define	AILERON  3
	#define	ELEVATOR 2
	#define	THROTTLE 0
	#define	RUDDER   1
#endif
#ifdef TRAE
	#define	AILERON  2
	#define	ELEVATOR 3
	#define	THROTTLE 0
	#define	RUDDER   1
#endif
#ifdef TARE
	#define	AILERON  1
	#define	ELEVATOR 3
	#define	THROTTLE 0
	#define	RUDDER   2
#endif
#ifdef TAER
	#define	AILERON  1
	#define	ELEVATOR 2
	#define	THROTTLE 0
	#define	RUDDER   3
#endif

#ifdef RETA
	#define	AILERON  3
	#define	ELEVATOR 1
	#define	THROTTLE 2
	#define	RUDDER   0
#endif
#ifdef REAT
	#define	AILERON  2
	#define	ELEVATOR 1
	#define	THROTTLE 3
	#define	RUDDER   0
#endif
#ifdef RAET
	#define	AILERON  1
	#define	ELEVATOR 2
	#define	THROTTLE 3
	#define	RUDDER   0
#endif
#ifdef RATE
	#define	AILERON  1
	#define	ELEVATOR 3
	#define	THROTTLE 2
	#define	RUDDER   0
#endif
#ifdef RTAE
	#define	AILERON  2
	#define	ELEVATOR 3
	#define	THROTTLE 1
	#define	RUDDER   0
#endif
#ifdef RTEA
	#define	AILERON  3
	#define	ELEVATOR 2
	#define	THROTTLE 1
	#define	RUDDER   0
#endif

#define	CH1		0
#define	CH2		1
#define	CH3		2
#define	CH4		3
#define	CH5		4
#define	CH6		5
#define	CH7		6
#define	CH8		7
#define	CH9		8
#define	CH10	9
#define	CH11	10
#define	CH12	11
#define	CH13	12
#define	CH14	13
#define	CH15	14
#define	CH16	15
