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
	#define PPM_MAX_125	2100	//	125%
	#define PPM_MIN_125	900		//	125%
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
	#define PPM_MAX_125	2000	//	125%
	#define PPM_MIN_125	1000	//	125%
	#define PPM_MAX_100	1900	//	100%
	#define PPM_MIN_100	1100	//	100%
	#define PPM_MAX_125	2000	//	125%
	#define PPM_MIN_125	1000	//	125%
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
enum {
	AILERON =0,
	ELEVATOR,
	THROTTLE,
	RUDDER,
};
#endif
#ifdef AERT
enum {
	AILERON =0,
	ELEVATOR,
	RUDDER,
	THROTTLE,
};
#endif
#ifdef ARET
enum {
	AILERON =0,
	RUDDER,
	ELEVATOR,
	THROTTLE,
};
#endif
#ifdef ARTE
enum {
	AILERON =0,
	RUDDER,
	THROTTLE,
	ELEVATOR,
};
#endif
#ifdef ATRE
enum {
	AILERON =0,
	THROTTLE,
	RUDDER,
	ELEVATOR,
};
#endif
#ifdef ATER
enum {
	AILERON =0,
	THROTTLE,
	ELEVATOR,
	RUDDER,
};
#endif

#ifdef EATR
enum {
	ELEVATOR =0,
	AILERON,
	THROTTLE,
	RUDDER,
};
#endif
#ifdef EART
enum {
	ELEVATOR =0,
	AILERON,
	RUDDER,
	THROTTLE,
};
#endif
#ifdef ERAT
enum {
	ELEVATOR =0,
	RUDDER,
	AILERON,
	THROTTLE,
};
#endif
#ifdef ERTA
enum {
	ELEVATOR =0,
	RUDDER,
	THROTTLE,
	AILERON,
};
#endif
#ifdef ETRA
enum {
	ELEVATOR =0,
	THROTTLE,
	RUDDER,
	AILERON,
};
#endif
#ifdef ETAR
enum {
	ELEVATOR =0,
	THROTTLE,
	AILERON,
	RUDDER,
};
#endif

#ifdef TEAR
enum {
	THROTTLE =0,
	ELEVATOR,
	AILERON,
	RUDDER,
};
#endif
#ifdef TERA
enum {
	THROTTLE =0,
	ELEVATOR,
	RUDDER,
	AILERON,
};
#endif
#ifdef TREA
enum {
	THROTTLE =0,
	RUDDER,
	ELEVATOR,
	AILERON,
};
#endif
#ifdef TRAE
enum {
	THROTTLE =0,
	RUDDER,
	AILERON,
	ELEVATOR,
};
#endif
#ifdef TARE
enum {
	THROTTLE =0,
	AILERON,
	RUDDER,
	ELEVATOR,
};
#endif
#ifdef TAER
enum {
	THROTTLE =0,
	AILERON,
	ELEVATOR,
	RUDDER,
};
#endif

#ifdef RETA
enum {
	RUDDER =0,
	ELEVATOR,
	THROTTLE,
	AILERON,
};
#endif
#ifdef REAT
enum {
	RUDDER =0,
	ELEVATOR,
	AILERON,
	THROTTLE,
};
#endif
#ifdef RAET
enum {
	RUDDER =0,
	AILERON,
	ELEVATOR,
	THROTTLE,
};
#endif
#ifdef RATE
enum {
	RUDDER =0,
	AILERON,
	THROTTLE,
	ELEVATOR,
};
#endif
#ifdef RTAE
enum {
	RUDDER =0,
	THROTTLE,
	AILERON,
	ELEVATOR,
};
#endif
#ifdef RTEA
enum {
	RUDDER =0,
	THROTTLE,
	ELEVATOR,
	AILERON,
};
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
#define	AUX13	16
