// Turnigy PPM and channels
#if defined(TX_ER9X)
	#define PPM_MAX_100 2012	//	100%
	#define PPM_MIN_100 988		//	100%
	#define PPM_MAX_125	2140	//	125%
	#define PPM_MIN_125	860		//	125%
#endif

// Devo PPM and channels
#if defined(TX_DEVO7)
	#define PPM_MAX_100	1920	//	100%
	#define PPM_MIN_100	1120	//	100%
	#define PPM_MAX_125	2120	//	125%
	#define PPM_MIN_125	920		//	125%
#endif

// SPEKTRUM PPM and channels
#if defined(TX_SPEKTRUM)
	#define PPM_MAX_100	1900	//	100%
	#define PPM_MIN_100	1100	//	100%
	#define PPM_MAX_125	2000	//	125%
	#define PPM_MIN_125	1000	//	125%
#endif

// HISKY
#if defined(TX_HISKY)
	#define PPM_MAX_100	1920	//	100%
	#define PPM_MIN_100	1120	//	100%
	#define PPM_MAX_125	2020	//	125%
	#define PPM_MIN_125	1020	//	125%
#endif

// Multiplex MC2020
#if defined(TX_MPX)
	#define PPM_MAX_100 1950 // 100%
	#define PPM_MIN_100 1250 // 100%
	#define PPM_MAX_125 2050 // 125%
	#define PPM_MIN_125 1150 // 125%
#endif

// Walkera PL0811-01H
#if defined(TX_WALKERA)
	#define PPM_MAX_100 1800 // 100%
	#define PPM_MIN_100 1000 // 100%
	#define PPM_MAX_125 1900 // 125%
	#define PPM_MIN_125 900  // 125%
#endif

//Serial MIN MAX values
#define SERIAL_MAX_100	2012	//	100%
#define SERIAL_MIN_100	988		//	100%
#define SERIAL_MAX_125	2140	//	125%
#define SERIAL_MIN_125	860		//	125%

//PPM values used to compare
#define PPM_MIN_COMMAND 1250
#define PPM_SWITCH		1550
#define PPM_MAX_COMMAND 1750

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

#define	AUX1	4
#define	AUX2	5
#define	AUX3	6
#define	AUX4	7
#define	AUX5	8
#define	AUX6	9
#define	AUX7	10
#define	AUX8	11
#define	AUX9	12
#define	AUX10	13
#define	AUX11	14
#define	AUX12	15
