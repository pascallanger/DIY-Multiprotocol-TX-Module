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

//******************
// Version
//******************
#define VERSION_MAJOR		1
#define VERSION_MINOR		3
#define VERSION_REVISION	4
#define VERSION_PATCH_LEVEL	12

#define MODE_SERIAL 0

//******************
// Protocols
//******************
enum PROTOCOLS
{
	PROTO_PROTOLIST	= 0,	// NO RF
	PROTO_FLYSKY 	= 1,	// =>A7105
	PROTO_HUBSAN	= 2,	// =>A7105
	PROTO_FRSKYD	= 3,	// =>CC2500
	PROTO_HISKY		= 4,	// =>NRF24L01
	PROTO_V2X2		= 5,	// =>NRF24L01
	PROTO_DSM		= 6,	// =>CYRF6936
	PROTO_DEVO		= 7,	// =>CYRF6936
	PROTO_YD717		= 8,	// =>NRF24L01
	PROTO_KN		= 9,	// =>NRF24L01
	PROTO_SYMAX		= 10,	// =>NRF24L01
	PROTO_SLT		= 11,	// =>NRF24L01
	PROTO_CX10		= 12,	// =>NRF24L01
	PROTO_CG023		= 13,	// =>NRF24L01
	PROTO_BAYANG	= 14,	// =>NRF24L01
	PROTO_FRSKYX	= 15,	// =>CC2500
	PROTO_ESKY		= 16,	// =>NRF24L01
	PROTO_MT99XX	= 17,	// =>NRF24L01
	PROTO_MJXQ		= 18,	// =>NRF24L01
	PROTO_SHENQI	= 19,	// =>NRF24L01
	PROTO_FY326		= 20,	// =>NRF24L01
	PROTO_FUTABA	= 21,	// =>CC2500
	PROTO_J6PRO		= 22,	// =>CYRF6936
	PROTO_FQ777		= 23,	// =>NRF24L01
	PROTO_ASSAN		= 24,	// =>NRF24L01
	PROTO_FRSKYV	= 25,	// =>CC2500
	PROTO_HONTAI	= 26,	// =>NRF24L01
	PROTO_OPENLRS	= 27,	// =>OpenLRS hardware
	PROTO_AFHDS2A	= 28,	// =>A7105
	PROTO_Q2X2		= 29,	// =>NRF24L01, extension of CX-10 protocol
	PROTO_WK2x01	= 30,	// =>CYRF6936
	PROTO_Q303		= 31,	// =>NRF24L01
	PROTO_GW008		= 32,	// =>NRF24L01
	PROTO_DM002		= 33,	// =>NRF24L01
	PROTO_CABELL	= 34,	// =>NRF24L01
	PROTO_ESKY150	= 35,	// =>NRF24L01
	PROTO_H8_3D		= 36,	// =>NRF24L01
	PROTO_CORONA	= 37,	// =>CC2500
	PROTO_CFLIE     = 38,   // =>NRF24L01
	PROTO_HITEC     = 39,   // =>CC2500
	PROTO_WFLY		= 40,	// =>CYRF6936
	PROTO_BUGS		= 41,	// =>A7105
	PROTO_BUGSMINI	= 42,	// =>NRF24L01
	PROTO_TRAXXAS	= 43,	// =>CYRF6936
	PROTO_NCC1701	= 44,	// =>NRF24L01
	PROTO_E01X		= 45,	// =>CYRF6936
	PROTO_V911S		= 46,	// =>NRF24L01
	PROTO_GD00X		= 47,	// =>NRF24L01
	PROTO_V761		= 48,	// =>NRF24L01
	PROTO_KF606		= 49,	// =>NRF24L01
	PROTO_REDPINE	= 50,	// =>CC2500
	PROTO_POTENSIC	= 51,	// =>NRF24L01
	PROTO_ZSX		= 52,	// =>NRF24L01
	PROTO_HEIGHT	= 53,	// =>A7105
	PROTO_SCANNER	= 54,	// =>CC2500
	PROTO_FRSKY_RX	= 55,	// =>CC2500
	PROTO_AFHDS2A_RX= 56,	// =>A7105
	PROTO_HOTT		= 57,	// =>CC2500
	PROTO_FX		= 58,	// =>NRF24L01
	PROTO_BAYANG_RX	= 59,	// =>NRF24L01
	PROTO_PELIKAN	= 60,	// =>A7105
	PROTO_EAZYRC	= 61,	// =>NRF24L01
	PROTO_XK		= 62,	// =>NRF24L01
	PROTO_XN297DUMP	= 63,	// =>NRF24L01
	PROTO_FRSKYX2	= 64,	// =>CC2500
	PROTO_FRSKY_R9	= 65,	// =>SX1276
	PROTO_PROPEL	= 66,	// =>NRF24L01
	PROTO_FRSKYL	= 67,	// =>CC2500
	PROTO_SKYARTEC	= 68,	// =>CC2500
	PROTO_ESKY150V2	= 69,	// =>CC2500+NRF24L01
	PROTO_DSM_RX	= 70,	// =>CYRF6936
	PROTO_JJRC345	= 71,	// =>NRF24L01
	PROTO_Q90C		= 72,	// =>NRF24L01 or CC2500
	PROTO_KYOSHO	= 73,	// =>A7105
	PROTO_RLINK		= 74,	// =>CC2500
	PROTO_REALACC	= 76,	// =>NRF24L01
	PROTO_OMP		= 77,	// =>CC2500 & NRF24L01
	PROTO_MLINK		= 78,	// =>CYRF6936
	PROTO_WFLY2		= 79,	// =>A7105
	PROTO_E016HV2	= 80,	// =>CC2500 & NRF24L01
	PROTO_E010R5	= 81,	// =>CYRF6936
	PROTO_LOLI		= 82,	// =>NRF24L01
	PROTO_E129		= 83,	// =>CYRF6936
	PROTO_JOYSWAY	= 84,	// =>A7105
	PROTO_E016H		= 85,	// =>NRF24L01
	PROTO_CONFIG	= 86,	// Module config
	PROTO_IKEAANSLUTA = 87, // =>CC2500
	PROTO_WILLIFM	= 88,	// 27/35ab/40/41/72 MHz module external project
	PROTO_LOSI		= 89,	// =>CYRF6936
	PROTO_MOULDKG	= 90,	// =>NRF24L01
	PROTO_XERALL	= 91,	// =>NRF24L01
	PROTO_MT99XX2	= 92,	// =>NRF24L01, extension of MT99XX protocol
	PROTO_KYOSHO2	= 93,	// =>NRF24L01
	PROTO_SCORPIO	= 94,	// =>CYRF6936
	PROTO_BLUEFLY	= 95,	// =>CC2500 & NRF24L01
	PROTO_BUMBLEB	= 96,	// =>CC2500 & NRF24L01
	PROTO_SGF22		= 97,	// =>NRF24L01
	PROTO_KYOSHO3	= 98,	// =>CYRF6936
	PROTO_XK2		= 99,	// =>CC2500 & NRF24L01
	
	PROTO_NANORF	= 126,	// =>NRF24L01
	PROTO_TEST		= 127,	// =>CC2500
};

enum Flysky
{
	Flysky	= 0,
	V9X9	= 1,
	V6X6	= 2,
	V912	= 3,
	CX20	= 4,
};
enum Height
{
	FZ410	= 0,
};
enum Hubsan
{
	H107	= 0,
	H301	= 1,
	H501	= 2,
};
enum AFHDS2A
{
	PWM_IBUS = 0,
	PPM_IBUS = 1,
	PWM_SBUS = 2,
	PPM_SBUS = 3,
	PWM_IB16 = 4,
	PPM_IB16 = 5,
	PWM_SB16 = 6,
	PPM_SB16 = 7,
};
enum Hisky
{
	Hisky	= 0,
	HK310	= 1,
};
enum DSM
{
	DSM2_1F		= 0,
	DSM2_2F		= 1,
	DSMX_1F		= 2,
	DSMX_2F		= 3,
	DSM_AUTO	= 4,
	DSMR		= 5,
	DSM2_SFC	= 6,
};
enum DSM_RX
{
	DSM_RX		= 0,
	DSM_CLONE	= 1,
	DSM_ERASE	= 2,
};
enum YD717
{       			
	YD717	= 0,
	SKYWLKR	= 1,
	SYMAX4	= 2,
	XINXUN	= 3,
	NIHUI	= 4,
};
enum KN
{
	WLTOYS	= 0,
	FEILUN	= 1,
};
enum SYMAX
{
	SYMAX	= 0,
	SYMAX5C	= 1,
};
enum SLT
{
	SLT_V1		= 0,
	SLT_V2		= 1,
	Q100		= 2,
	Q200		= 3,
	MR100		= 4,
	SLT_V1_4	= 5,
};
enum CX10
{
	CX10_GREEN	= 0,
	CX10_BLUE	= 1,	// also compatible with CX10-A, CX12
	DM007		= 2,
	JC3015_1	= 4,
	JC3015_2	= 5,
	MK33041		= 6,
};
enum Q2X2
{
	Q222		= 0,
	Q242		= 1,
	Q282		= 2,
	F_Q222		= 8,
	F_Q242		= 9,
	F_Q282		= 10,
};
enum CG023
{
    CG023	= 0,
    YD829	= 1,
};
enum BAYANG
{
    BAYANG	= 0,
    H8S3D	= 1,
    X16_AH  = 2,
	IRDRONE = 3,
	DHD_D4	= 4,
	QX100   = 5,
};
enum MT99XX
{
	MT99	= 0,
	H7		= 1,
	YZ		= 2,
	LS		= 3,
	FY805	= 4,
	A180	= 5,
	DRAGON	= 6,
	F949G	= 7,
};
enum MT99XX2
{
	PA18	= 0,  
	SU35	= 1,
};
enum MJXQ
{
	WLH08	= 0,
	X600	= 1,
	X800	= 2,
	H26D	= 3,
	E010	= 4,
	H26WH	= 5,
	PHOENIX = 6,
};
enum FRSKYD
{
	FRSKYD	= 0,
	DCLONE	= 1,
};
enum FRSKYX
{
	CH_16		= 0,
	CH_8		= 1,
	EU_16		= 2,
	EU_8		= 3,
	XCLONE_16	= 4,
	XCLONE_8	= 5,
};
enum HONTAI
{
	HONTAI	= 0,
	JJRCX1	= 1,
	X5C1	= 2,
	FQ777_951 =3,
};
enum V2X2
{
	V2X2	= 0,
	JXD506	= 1,
	V2X2_MR101 = 2,
};
enum FY326
{
	FY326	= 0,
	FY319	= 1,
};
enum WK2x01
{
	WK2801	= 0,
	WK2401	= 1,
	W6_5_1	= 2,
	W6_6_1	= 3,
	W6_HEL	= 4,
	W6_HEL_I= 5,
};
enum Q303
{
	Q303	= 0,
	CX35	= 1,
	CX10D	= 2,
	CX10WD	= 3,
};
enum CABELL
{
	CABELL_V3			= 0,
	CABELL_V3_TELEMETRY	= 1,
	CABELL_SET_FAIL_SAFE= 6,
	CABELL_UNBIND		= 7,
};
enum H8_3D
{
	H8_3D	= 0,
	H20H	= 1,
	H20MINI	= 2,
	H30MINI	= 3,
};
enum CORONA
{
	COR_V1	= 0,
	COR_V2	= 1,
	FD_V3	= 2,
};
enum HITEC
{
	OPT_FW	= 0,
	OPT_HUB	= 1,
	MINIMA	= 2,
};
enum E01X
{
	E012	= 0,
	E015	= 1,
	E016H	= 2,
};
enum GD00X
{
	GD_V1	= 0,
	GD_V2	= 1,
};
enum BUGSMINI
{
	BUGSMINI= 0,
	BUGS3H	= 1,
};
enum REDPINE
{
	RED_FAST= 0,
	RED_SLOW= 1,
};
enum TRAXXAS
{
	TRAXXAS_TQ2	= 0,
	TRAXXAS_TQ1	= 1,
};
enum ESKY150
{
	ESKY150_4CH	= 0,
	ESKY150_7CH	= 1,
};
enum V911S
{
	V911S_STD	= 0,
	V911S_E119	= 1,
};
enum XK
{
	X450	= 0,
	X420	= 1,
	XK_CARS	= 2,
};
enum XN297DUMP
{
	XN297DUMP_250K	= 0,
	XN297DUMP_1M	= 1,
	XN297DUMP_2M	= 2,
	XN297DUMP_AUTO	= 3,
	XN297DUMP_NRF	= 4,
	XN297DUMP_CC2500	= 5,
};
enum FRSKY_R9
{
	R9_915		= 0,
	R9_868		= 1,
	R9_915_8CH	= 2,
	R9_868_8CH	= 3,
	R9_FCC		= 4,
	R9_EU		= 5,
	R9_FCC_8CH	= 6,
	R9_EU_8CH	= 7,
};
enum ESKY
{
	ESKY_STD	= 0,
	ESKY_ET4	= 1,
};
enum FRSKY_RX
{
	FRSKY_RX	= 0,
	FRSKY_CLONE	= 1,
	FRSKY_ERASE	= 2,
	FRSKY_CPPM  = 3,
};
enum FRSKYL
{
	LR12		= 0,
	LR12_6CH	= 1,
};
enum HOTT
{
	HOTT_SYNC	= 0,
	HOTT_NO_SYNC= 1,
};
enum PELIKAN
{
	PELIKAN_PRO	= 0,
	PELIKAN_LITE= 1,
	PELIKAN_SCX24=2,
};
enum V761
{
	V761_3CH	= 0,
	V761_4CH	= 1,
	V761_TOPRC	= 2,
};
enum HEIGHT
{
	HEIGHT_5CH	= 0,
	HEIGHT_8CH	= 1,
};
enum KYOSHO
{
	KYOSHO_FHSS	= 0,
	KYOSHO_HYPE	= 1,
};
enum JJRC345
{
	JJRC345		= 0,
	SKYTMBLR	= 1,
};
enum RLINK
{
	RLINK_SURFACE	= 0,
	RLINK_AIR		= 1,
	RLINK_DUMBORC	= 2,
	RLINK_RC4G		= 3,
};
enum MOULDKG
{
	MOULDKG_ANALOG	= 0,
	MOULDKG_DIGIT	= 1,
};
enum KF606
{
	KF606_KF606		= 0,
	KF606_MIG320	= 1,
	KF606_ZCZ50		= 2,
};
enum E129
{
	E129_E129		= 0,
	E129_C186		= 1,
};
enum FX
{
	FX816			= 0,
	FX620			= 1,
    FX9630          = 2,
	FX_Q560			= 3,
};
enum SGF22
{
	SGF22_F22		= 0,
	SGF22_F22S		= 1,
	SGF22_J20 		= 2,
};

#define NONE 		0
#define P_HIGH		1
#define P_LOW		0
#define AUTOBIND	1
#define NO_AUTOBIND	0

//PPM protocols
struct PPM_Parameters
{
	uint8_t protocol;
	uint8_t sub_proto	: 3;
	uint8_t rx_num		: 6;
	uint8_t power		: 1;
	uint8_t autobind	: 1;
	int8_t option;
	uint32_t chan_order;
};

//Callback
typedef uint16_t (*uint16_function_t) (void);	//pointer to a function with no parameters which return an uint16_t integer
typedef void     (*void_function_t  ) (void);	//pointer to a function with no parameters which returns nothing

//Protocols definition
struct __attribute__((__packed__)) mm_protocol_definition {
	uint8_t protocol;
	const char *ProtoString;
	const char *SubProtoString;
	uint8_t nbrSubProto        : 4;
	uint8_t optionType         : 4;
	uint8_t failSafe           : 1;
	uint8_t chMap              : 1;
	uint8_t rfSwitch           : 2;
	void_function_t		Init;
	uint16_function_t	CallBack;
};
extern const mm_protocol_definition multi_protocols[];

enum RF_SWITCH
{
	SW_A7105	= 0,	//antenna RF1
	SW_CC2500	= 1,	//antenna RF2
	SW_NRF		= 2,	//antenna RF3
	SW_CYRF		= 3,	//antenna RF4
};

// Telemetry
enum MultiPacketTypes
{
	MULTI_TELEMETRY_STATUS			= 1,
	MULTI_TELEMETRY_SPORT			= 2,
	MULTI_TELEMETRY_HUB				= 3,
	MULTI_TELEMETRY_DSM				= 4,
	MULTI_TELEMETRY_DSMBIND			= 5,
	MULTI_TELEMETRY_AFHDS2A			= 6,
	MULTI_TELEMETRY_REUSE_1			= 7,
	MULTI_TELEMETRY_SYNC			= 8,
	MULTI_TELEMETRY_REUSE_2			= 9,
	MULTI_TELEMETRY_HITEC			= 10,
	MULTI_TELEMETRY_SCANNER			= 11,
	MULTI_TELEMETRY_AFHDS2A_AC		= 12,
	MULTI_TELEMETRY_RX_CHANNELS		= 13,
	MULTI_TELEMETRY_HOTT			= 14,
	MULTI_TELEMETRY_MLINK			= 15,
	MULTI_TELEMETRY_CONFIG			= 16,
	MULTI_TELEMETRY_PROTO			= 17,
};

// Macros
#define NOP() __asm__ __volatile__("nop")

//***************
//***  Flags  ***
//***************
#define RX_FLAG_on			protocol_flags |= _BV(0)
#define RX_FLAG_off			protocol_flags &= ~_BV(0)
#define IS_RX_FLAG_on		( ( protocol_flags & _BV(0) ) !=0 )
//
#define CHANGE_PROTOCOL_FLAG_on		protocol_flags |= _BV(1)
#define CHANGE_PROTOCOL_FLAG_off	protocol_flags &= ~_BV(1)
#define IS_CHANGE_PROTOCOL_FLAG_on	( ( protocol_flags & _BV(1) ) !=0 )
//
#define POWER_FLAG_on		protocol_flags |= _BV(2)
#define POWER_FLAG_off		protocol_flags &= ~_BV(2)
#define IS_POWER_FLAG_on	( ( protocol_flags & _BV(2) ) !=0 )
//
#define RANGE_FLAG_on		protocol_flags |= _BV(3)
#define RANGE_FLAG_off		protocol_flags &= ~_BV(3)
#define IS_RANGE_FLAG_on	( ( protocol_flags & _BV(3) ) !=0 )
//
#define AUTOBIND_FLAG_on	protocol_flags |= _BV(4)
#define AUTOBIND_FLAG_off	protocol_flags &= ~_BV(4)
#define IS_AUTOBIND_FLAG_on	( ( protocol_flags & _BV(4) ) !=0 )
//
#define BIND_BUTTON_FLAG_on		protocol_flags |= _BV(5)
#define BIND_BUTTON_FLAG_off	protocol_flags &= ~_BV(5)
#define IS_BIND_BUTTON_FLAG_on	( ( protocol_flags & _BV(5) ) !=0 )
//PPM RX OK
#define PPM_FLAG_off		protocol_flags &= ~_BV(6)
#define PPM_FLAG_on			protocol_flags |= _BV(6)
#define IS_PPM_FLAG_on		( ( protocol_flags & _BV(6) ) !=0 )
//Bind flag
#define BIND_IN_PROGRESS	protocol_flags &= ~_BV(7)
#define BIND_DONE			protocol_flags |= _BV(7)
#define IS_BIND_DONE		( ( protocol_flags & _BV(7) ) !=0 )
#define IS_BIND_IN_PROGRESS	( ( protocol_flags & _BV(7) ) ==0 )
//
#define FAILSAFE_VALUES_off	protocol_flags2 &= ~_BV(0)
#define FAILSAFE_VALUES_on		protocol_flags2 |= _BV(0)
#define IS_FAILSAFE_VALUES_on	( ( protocol_flags2 & _BV(0) ) !=0 )
//
#define RX_DONOTUPDATE_off	protocol_flags2 &= ~_BV(1)
#define RX_DONOTUPDATE_on	protocol_flags2 |= _BV(1)
#define IS_RX_DONOTUPDATE_on	( ( protocol_flags2 & _BV(1) ) !=0 )
//
#define RX_MISSED_BUFF_off	protocol_flags2 &= ~_BV(2)
#define RX_MISSED_BUFF_on	protocol_flags2 |= _BV(2)
#define IS_RX_MISSED_BUFF_on	( ( protocol_flags2 & _BV(2) ) !=0 )
//TX Pause
#define TX_MAIN_PAUSE_off	protocol_flags2 &= ~_BV(3)
#define TX_MAIN_PAUSE_on		protocol_flags2 |= _BV(3)
#define IS_TX_MAIN_PAUSE_on	( ( protocol_flags2 & _BV(3) ) !=0 )
#define TX_RX_PAUSE_off		protocol_flags2 &= ~_BV(4)
#define TX_RX_PAUSE_on		protocol_flags2 |= _BV(4)
#define IS_TX_RX_PAUSE_on	( ( protocol_flags2 & _BV(4) ) !=0 )
#define IS_TX_PAUSE_on		( ( protocol_flags2 & (_BV(4)|_BV(3)) ) !=0 )
#define IS_TX_PAUSE_off		( ( protocol_flags2 & (_BV(4)|_BV(3)) ) ==0 )
//Signal OK
#define INPUT_SIGNAL_off	protocol_flags2 &= ~_BV(5)
#define INPUT_SIGNAL_on		protocol_flags2 |= _BV(5)
#define IS_INPUT_SIGNAL_on	( ( protocol_flags2 & _BV(5) ) !=0 )
#define IS_INPUT_SIGNAL_off	( ( protocol_flags2 & _BV(5) ) ==0 )
//Bind from channel
#define BIND_CH_PREV_off	protocol_flags2 &= ~_BV(6)
#define BIND_CH_PREV_on		protocol_flags2 |= _BV(6)
#define IS_BIND_CH_PREV_on	( ( protocol_flags2 & _BV(6) ) !=0 )
#define IS_BIND_CH_PREV_off	( ( protocol_flags2 & _BV(6) ) ==0 )
//Wait for bind
#define WAIT_BIND_off		protocol_flags2 &= ~_BV(7)
#define WAIT_BIND_on		protocol_flags2 |= _BV(7)
#define IS_WAIT_BIND_on		( ( protocol_flags2 & _BV(7) ) !=0 )
#define IS_WAIT_BIND_off	( ( protocol_flags2 & _BV(7) ) ==0 )
//Incoming telemetry data buffer
#define DATA_BUFFER_LOW_off		protocol_flags3 &= ~_BV(0)
#define DATA_BUFFER_LOW_on		protocol_flags3 |= _BV(0)
#define IS_DATA_BUFFER_LOW_on	( ( protocol_flags3 & _BV(0) ) !=0 )
#define IS_DATA_BUFFER_LOW_off	( ( protocol_flags3 & _BV(0) ) ==0 )
#define SEND_MULTI_STATUS_off		protocol_flags3 &= ~_BV(1)
#define SEND_MULTI_STATUS_on		protocol_flags3 |= _BV(1)
#define IS_SEND_MULTI_STATUS_on		( ( protocol_flags3 & _BV(1) ) !=0 )
#define IS_SEND_MULTI_STATUS_off	( ( protocol_flags3 & _BV(1) ) ==0 )
#define DISABLE_CH_MAP_off		protocol_flags3 &= ~_BV(2)
#define DISABLE_CH_MAP_on		protocol_flags3 |= _BV(2)
#define IS_DISABLE_CH_MAP_on	( ( protocol_flags3 & _BV(2) ) !=0 )
#define IS_DISABLE_CH_MAP_off	( ( protocol_flags3 & _BV(2) ) ==0 )
#define DISABLE_TELEM_off		protocol_flags3 &= ~_BV(3)
#define DISABLE_TELEM_on		protocol_flags3 |= _BV(3)
#define IS_DISABLE_TELEM_on		( ( protocol_flags3 & _BV(3) ) !=0 )
#define IS_DISABLE_TELEM_off	( ( protocol_flags3 & _BV(3) ) ==0 )
//Valid/invalid sub_proto
#define SUB_PROTO_VALID			protocol_flags3 &= ~_BV(6)
#define SUB_PROTO_INVALID		protocol_flags3 |= _BV(6)
#define IS_SUB_PROTO_INVALID	( ( protocol_flags3 & _BV(6) ) !=0 )
#define IS_SUB_PROTO_VALID		( ( protocol_flags3 & _BV(6) ) ==0 )
//LBT power
#define LBT_POWER_off		protocol_flags3 &= ~_BV(7)
#define LBT_POWER_on		protocol_flags3 |= _BV(7)
#define IS_LBT_POWER_on		( ( protocol_flags3 & _BV(7) ) !=0 )
#define IS_LBT_POWER_off	( ( protocol_flags3 & _BV(7) ) ==0 )


// Failsafe
#define FAILSAFE_CHANNEL_HOLD		2047
#define	FAILSAFE_CHANNEL_NOPULSES	0

//********************
//** Debug messages **
//********************
#if defined(STM32_BOARD) && (defined (DEBUG_SERIAL) || defined (ARDUINO_MULTI_DEBUG))
	uint16_t debug_time=0;
	char debug_buf[64];
	#define debug(msg, ...)  { sprintf(debug_buf, msg, ##__VA_ARGS__); Serial.write(debug_buf);}
	#define debugln(msg, ...)  { sprintf(debug_buf, msg "\r\n", ##__VA_ARGS__); Serial.write(debug_buf);}
	#define debug_time(msg)  { uint16_t debug_time_TCNT1=TCNT1; debug_time=debug_time_TCNT1-debug_time; debug(msg "%u", debug_time>>1); debug_time=debug_time_TCNT1; }
	#define debugln_time(msg)  { uint16_t debug_time_TCNT1=TCNT1; debug_time=debug_time_TCNT1-debug_time; debug(msg "%u\r\n", debug_time>>1); debug_time=debug_time_TCNT1; }
#else
	#define debug(...) { }
	#define debugln(...) { }
	#define debugln_time(...) { }
	#undef DEBUG_SERIAL
#endif

//********************
//*** Blink timing ***
//********************
#define BLINK_BIND_TIME				100
#define BLINK_SERIAL_TIME			500
#define BLINK_PPM_TIME				1000
#define BLINK_BAD_PROTO_TIME_HIGH	50
#define BLINK_BAD_PROTO_TIME_LOW	1000
#define BLINK_WAIT_BIND_TIME_HIGH	1000
#define BLINK_WAIT_BIND_TIME_LOW	100
#define BLINK_BANK_TIME_HIGH		50
#define BLINK_BANK_TIME_LOW			500
#define BLINK_BANK_REPEAT			1500

//*******************
//***  AUX flags  ***
//*******************
#define GET_FLAG(ch, mask) ( ch ? mask : 0)
#define CH5_SW	(Channel_AUX & _BV(0))
#define CH6_SW	(Channel_AUX & _BV(1))
#define CH7_SW	(Channel_AUX & _BV(2))
#define CH8_SW	(Channel_AUX & _BV(3))
#define CH9_SW	(Channel_AUX & _BV(4))
#define CH10_SW	(Channel_AUX & _BV(5))
#define CH11_SW	(Channel_AUX & _BV(6))
#define CH12_SW	(Channel_AUX & _BV(7))
#define CH13_SW	(Channel_data[CH13]>CHANNEL_SWITCH)
#define CH14_SW	(Channel_data[CH14]>CHANNEL_SWITCH)
#define CH15_SW	(Channel_data[CH15]>CHANNEL_SWITCH)
#define CH16_SW	(Channel_data[CH16]>CHANNEL_SWITCH)

//************************
//***  Power settings  ***
//************************
enum {
	TXPOWER_100uW,
	TXPOWER_300uW,
	TXPOWER_1mW,
	TXPOWER_3mW,
	TXPOWER_10mW,
	TXPOWER_30mW,
	TXPOWER_100mW,
	TXPOWER_150mW
};

// A7105 power
// The numbers do not take into account any outside amplifier
enum A7105_POWER
{
	A7105_POWER_0 = 0x00<<3 | 0x00,	// -23dBm == PAC=0 TBG=0
	A7105_POWER_1 = 0x00<<3 | 0x01,	// -20dBm == PAC=0 TBG=1
	A7105_POWER_2 = 0x00<<3 | 0x02,	// -16dBm == PAC=0 TBG=2
	A7105_POWER_3 = 0x00<<3 | 0x04,	// -11dBm == PAC=0 TBG=4
	A7105_POWER_4 = 0x01<<3 | 0x05,	//  -6dBm == PAC=1 TBG=5
	A7105_POWER_5 = 0x02<<3 | 0x07,	//   0dBm == PAC=2 TBG=7
	A7105_POWER_6 = 0x03<<3 | 0x07,	//  +1dBm == PAC=3 TBG=7
	A7105_POWER_7 = 0x03<<3 | 0x07	//  +1dBm == PAC=3 TBG=7
};
#define A7105_HIGH_POWER	A7105_POWER_7
#define	A7105_LOW_POWER		A7105_POWER_3
#define	A7105_RANGE_POWER	A7105_POWER_0
#define	A7105_BIND_POWER	A7105_POWER_0

// NRF Power
// The numbers do not take into account any outside amplifier
enum NRF_POWER
{
	NRF_POWER_0 = 0x00,	// -18dBm
	NRF_POWER_1 = 0x01,	// -12dBm
	NRF_POWER_2 = 0x02,	//  -6dBm
	NRF_POWER_3 = 0x03	//   0dBm
};
#define NRF_HIGH_POWER		NRF_POWER_3
#define	NRF_LOW_POWER		NRF_POWER_1
#define	NRF_RANGE_POWER		NRF_POWER_0
#define	NRF_BIND_POWER		NRF_POWER_0

// CC2500 power output from the chip itself
// The numbers do not take into account any outside amplifier
enum CC2500_POWER
{
	CC2500_POWER_0  = 0x00,	// -55dbm or less
	CC2500_POWER_1  = 0x50,	// -30dbm
	CC2500_POWER_2  = 0x44, // -28dbm
	CC2500_POWER_3  = 0xC0, // -26dbm
	CC2500_POWER_4  = 0x84, // -24dbm
	CC2500_POWER_5  = 0x81, // -22dbm
	CC2500_POWER_6  = 0x46, // -20dbm
	CC2500_POWER_7  = 0x93, // -18dbm
	CC2500_POWER_8  = 0x55, // -16dbm
	CC2500_POWER_9  = 0x8D, // -14dbm
	CC2500_POWER_10 = 0xC6,	// -12dbm
	CC2500_POWER_11 = 0x97,	// -10dbm
	CC2500_POWER_12 = 0x6E,	//  -8dbm
	CC2500_POWER_13 = 0x7F,	//  -6dbm
	CC2500_POWER_14 = 0xA9,	//  -4dbm
	CC2500_POWER_15 = 0xBB,	//  -2dbm
	CC2500_POWER_16 = 0xFE,	//   0dbm
	CC2500_POWER_17 = 0xFF	//  +1dbm
};
#define CC2500_HIGH_POWER	CC2500_POWER_17
#define CC2500_LBT_POWER	CC2500_POWER_14
#define CC2500_LOW_POWER	CC2500_POWER_13
#define CC2500_RANGE_POWER	CC2500_POWER_1
#define CC2500_BIND_POWER	CC2500_POWER_1

// CYRF power
// The numbers do not take into account any outside amplifier
enum CYRF_POWER
{
	CYRF_POWER_0 = 0x00,	// -35dbm
	CYRF_POWER_1 = 0x01,	// -30dbm
	CYRF_POWER_2 = 0x02,	// -24dbm
	CYRF_POWER_3 = 0x03,	// -18dbm
	CYRF_POWER_4 = 0x04,	// -13dbm
	CYRF_POWER_5 = 0x05,	//  -5dbm
	CYRF_POWER_6 = 0x06,	//   0dbm
	CYRF_POWER_7 = 0x07		//  +4dbm
};
#define CYRF_HIGH_POWER		CYRF_POWER_7
#define	CYRF_LOW_POWER		CYRF_POWER_3
#define	CYRF_RANGE_POWER	CYRF_POWER_1	// 1/30 of the full power distance
#define	CYRF_BIND_POWER		CYRF_POWER_0

// SX1276
#define JP_T18		1
#define JP_TLite	2

enum TXRX_State {
	TXRX_OFF,
	TX_EN,
	RX_EN
};

// Packet ack status values
enum {
	PKT_PENDING = 0,
	PKT_ACKED,
	PKT_TIMEOUT
};

// baudrate defines for serial
#define SPEED_100K	0
#define SPEED_9600	1
#define SPEED_57600	2
#define SPEED_125K	3

/** EEPROM Layout */
#define EEPROM_CID_INIT_OFFSET	0		// 1 byte flag that Cyrf ID is initialized
#define EEPROM_CID_OFFSET		1		// 6 bytes Cyrf ID
#define EEPROM_ID_OFFSET		10		// Module ID (4 bytes)
#define EEPROM_BANK_OFFSET		15		// Current bank number (1 byte)
#define EEPROM_ID_VALID_OFFSET	20		// 1 byte flag that ID is valid
#define MODELMODE_EEPROM_OFFSET	30		// Autobind mode, 1 byte per model, end is 30+16=46
#define AFHDS2A_EEPROM_OFFSET	50		// RX ID, 4 bytes per model id, end is 50+64=114
#define BUGS_EEPROM_OFFSET		114		// RX ID, 2 bytes per model id, end is 114+32=146
#define BUGSMINI_EEPROM_OFFSET	146		// RX ID, 2 bytes per model id, end is 146+32=178
#define FRSKY_RX_EEPROM_OFFSET	178		// (1) format + (3) TX ID + (1) freq_tune + (47) channels, 52 bytes, end is 178+52=230
#define AFHDS2A_RX_EEPROM_OFFSET 230	// (4) TX ID + (16) channels, 20 bytes, end is 230+20=250
#define AFHDS2A_EEPROM_OFFSET2	250		// RX ID, 4 bytes per model id, end is 250+192=442
#define HOTT_EEPROM_OFFSET		442		// RX ID, 5 bytes per model id, end is 320+442=762
#define BAYANG_RX_EEPROM_OFFSET	762		// (5) TX ID + (4) channels, 9 bytes, end is 771 
#define FRSKYD_CLONE_EEPROM_OFFSET	771	// (1) format + (3) TX ID + (47) channels, 51 bytes, end is 822
#define FRSKYX_CLONE_EEPROM_OFFSET	822	// (1) format + (3) TX ID + (47) channels, 51 bytes, end is 873
#define FRSKYX2_CLONE_EEPROM_OFFSET	873	// (1) format + (3) TX ID, 4 bytes, end is 877
#define DSM_RX_EEPROM_OFFSET	877		// (4) TX ID + format, 5 bytes, end is 882
#define MOULDKG_EEPROM_OFFSET	882		// RX ID, 3 bytes per model, end is 882+64*3=1074
#define DSM_CLONE_EEPROM_OFFSET	1074	// (4) TX ID, (1) Initialized, end is 1079
#define TRAXXAS_EEPROM_OFFSET	1079	// RX ID and SOP index, 3 bytes per model id, end is 1079+192=1271
#define XK2_EEPROM_OFFSET		1271	// RX ID checksum, 1 byte per model, end is 1271+64=1335
//#define CONFIG_EEPROM_OFFSET 	1335	// Current configuration of the multimodule

/* STM32 Flash Size */
#ifndef DISABLE_FLASH_SIZE_CHECK
	#ifdef MCU_STM32F103C8
		#define MCU_EXPECTED_FLASH_SIZE 64	// STM32F103C8 has 64KB of flash space
	#else
		#define MCU_EXPECTED_FLASH_SIZE 128	// STM32F103CB has 128KB of flash space
	#endif
#endif

//****************************************
//*** MULTI protocol serial definition ***
//****************************************
/*
***************************
16 channels serial protocol
***************************
Serial: 100000 Baud 8e2      _ xxxx xxxx p --
  Total of 26 bytes for protocol V1, variable length 27..36 for protocol V2
  Stream[0]   = header
				0x55	sub_protocol values are 0..31	Stream contains channels
				0x54	sub_protocol values are 32..63	Stream contains channels
				0x57	sub_protocol values are 0..31	Stream contains failsafe
				0x56	sub_protocol values are 32..63	Stream contains failsafe
				Note: V2 adds the 2 top bits to extend the number of protocols to 256 in Stream[26]
  Stream[1]   = sub_protocol|BindBit|RangeCheckBit|AutoBindBit;
   sub_protocol is 0..31 (bits 0..4)
				Reserved	0
				Flysky		1
				Hubsan		2
				FrskyD		3
				Hisky		4
				V2x2		5
				DSM			6
				Devo		7
				YD717		8
				KN			9
				SymaX		10
				SLT			11
				CX10		12
				CG023		13
				Bayang		14
				FrskyX		15
				ESky		16
				MT99XX		17
				MJXQ		18
				SHENQI		19
				FY326		20
				Futaba		21
				J6PRO		22
				FQ777		23
				ASSAN		24
				FrskyV		25
				HONTAI		26
				OpenLRS		27
				AFHDS2A		28
				Q2X2		29
				WK2x01		30
				Q303		31
				GW008		32
				DM002		33
				CABELL		34
				ESKY150		35
				H8_3D		36
				CORONA		37
				CFlie		38
				Hitec		39
				WFLY		40
				BUGS		41
				BUGSMINI	42
				TRAXXAS		43
				NCC1701		44
				E01X		45
				V911S		46
				GD00X		47
				V761		48
				KF606		49
				REDPINE		50
				POTENSIC	51
				ZSX			52
				HEIGHT		53
				SCANNER		54
				FRSKY_RX	55
				AFHDS2A_RX	56
				HOTT		57
				FX		58
				BAYANG_RX	59
				PELIKAN		60
				XK			62
				XN297DUMP	63
				FRSKYX2		64
				FRSKY_R9	65
				PROPEL		66
				FRSKYL		67
				SKYARTEC	68
				ESKY150V2	69
				DSM_RX		70
				JJRC345		71
				Q90C		72
				KYOSHO		73
				RLINK		74
				REALACC		76
				OMP			77
				MLINK		78
				WFLY2		79
				E016HV2		80
				E010R5		81
				LOLI		82
				E129		83
				JOYSWAY		84
				E016H		85
				XERALL		91
   BindBit=>		0x80	1=Bind/0=No
   AutoBindBit=>	0x40	1=Yes /0=No
   RangeCheck=>		0x20	1=Yes /0=No
  Stream[2]   = RxNum | Power | Type;
   RxNum value is 0..15 (bits 0..3)
   Type is 0..7 <<4     (bit 4..6)
		sub_protocol==Flysky
			Flysky		0
			V9x9		1
			V6x6		2
			V912		3
			CX20		4
		sub_protocol==Hubsan
			H107		0
			H301		1
			H501		2
		sub_protocol==Hisky
			Hisky		0
			HK310		1
		sub_protocol==DSM
			DSM2_1F 	0
			DSM2_2F 	1
			DSMX_1F 	2
			DSMX_2F 	3
			DSM_AUTO	4
		sub_protocol==DSM_RX
			DSM_RX		0
			DSM_CLONE	1
			DSM_ERASE	2
		sub_protocol==YD717
			YD717		0
			SKYWLKR		1
			SYMAX4		2
			XINXUN		3
			NIHUI		4
		sub_protocol==KN
			WLTOYS		0
			FEILUN		1
		sub_protocol==SYMAX
			SYMAX		0
			SYMAX5C		1
		sub_protocol==CX10
			CX10_GREEN	0
			CX10_BLUE	1	// also compatible with CX10-A, CX12
			DM007		2
			---			3
			JC3015_1	4
			JC3015_2	5
			MK33041		6
		sub_protocol==Q2X2
			Q222		0
			Q242		1
			Q282		2
		sub_protocol==CG023
			CG023		0
			YD829		1
		sub_protocol==BAYANG
			BAYANG		0
			H8S3D		1
			X16_AH		2
			IRDRONE		3
			DHD_D4		4
		sub_protocol==MT99XX
			MT99		0
			H7			1
			YZ			2
			LS			3
			FY805		4
		sub_protocol==MJXQ
			WLH08		0
			X600		1
			X800		2
			H26D		3
			E010		4
			H26WH		5
			PHOENIX		6
		sub_protocol==FRSKYD
			FRSKYD		0
			DCLONE		1
		sub_protocol==FRSKYX
			CH_16		0
			CH_8		1
			EU_16		2
			EU_8		3
			XCLONE		4
		sub_protocol==FRSKYX2
			CH_16		0
			CH_8		1
			EU_16		2
			EU_8		3
			XCLONE		4
		sub_protocol==HONTAI
			HONTAI	0
			JJRCX1	1
			X5C1		2
			FQ777_951 3
		sub_protocol==AFHDS2A
			PWM_IBUS	0
			PPM_IBUS	1
			PWM_SBUS	2
			PPM_SBUS	3
			PWM_IB16	4
			PPM_IB16	5
		sub_protocol==V2X2
			V2X2		0
			JXD506		1
			V2X2_MR101 2
		sub_protocol==FY326
			FY326		0
			FY319		1
		sub_protocol==WK2x01
			WK2801		0
			WK2401		1
			W6_5_1		2
			W6_6_1		3
			W6_HEL		4
			W6_HEL_I	5
		sub_protocol==Q303
			Q303		0
			CX35		1
			CX10D		2
			CX10WD		3
		sub_protocol==CABELL
			CABELL_V3				0
			CABELL_V3_TELEMETRY		1
			CABELL_SET_FAIL_SAFE	6
			CABELL_UNBIND			7
		sub_protocol==H8_3D
			H8_3D		0
			H20H		1
			H20MINI		2
			H30MINI		3
		sub_protocol==CORONA
			COR_V1		0
			COR_V2		1
			FD_V3		2
		sub_protocol==HITEC
			OPT_FW		0
			OPT_HUB		1
			MINIMA		2
		sub_protocol==SLT
			SLT_V1		0
			SLT_V2		1
			Q100		2
			Q200		3
			MR100		4
		sub_protocol==E01X
			E012		0
			E015		1
		sub_protocol==GD00X
			GD_V1		0
			GD_V2		1
		sub_protocol==REDPINE
			RED_FAST	0
			RED_SLOW	1
		sub_protocol==TRAXXAS
			TQ			0
		sub_protocol==ESKY150
			ESKY150_4CH	0
			ESKY150_7CH	1
		sub_protocol==V911S
			V911S_STD	0
			V911S_E119	1
		sub_protocol==XK
			X450		0
			X420		1
		sub_protocol==FRSKY_R9
			R9_915		0
			R9_868		1
			R9_915_8CH	2
			R9_868_8CH	3
			R9_FCC		4
			R9_EU		5
			R9_FCC_8CH	6
			R9_EU_8CH	7
		sub_protocol==ESKY
			ESKY_STD	0
			ESKY_ET4	1
		sub_protocol==FRSKY_RX
			FRSKY_RX	0
			FRSKY_CLONE	1
		sub_protocol==FRSKYL
			LR12		0
			LR12_6CH	1
		sub_protocol==HOTT
			HOTT_SYNC		0
			HOTT_NO_SYNC	1
		sub_protocol==PELIKAN
			PELIKAN_PRO		0
			PELIKAN_LITE	1
			PELIKAN_SCX24	2
		sub_protocol==V761
			V761_3CH	0
			V761_4CH	1
		sub_protocol==HEIGHT
			HEIGHT_5CH	0
			HEIGHT_8CH	1
		sub_protocol==JJRC345
			JJRC345		0
			SKYTMBLR	1
		sub_protocol==RLINK
			RLINK_SURFACE	0
			RLINK_AIR		1
			RLINK_DUMBORC	2

   Power value => 0x80	0=High/1=Low
  Stream[3]   = option_protocol;
   option_protocol value is -128..127
  Stream[4] to [25] = Channels or failsafe depending on Steam[0]
   16 Channels on 11 bits (0..2047)
	0		-125%
    204		-100%
	1024	   0%
	1843	+100%
	2047	+125%
   Values are concatenated to fit in 22 bytes like in SBUS protocol.
   Failsafe values have exactly the same range/values than normal channels except the extremes where
      0=no pulse, 2047=hold. If failsafe is not set or RX then failsafe packets should not be sent.
  Stream[26]   = sub_protocol bits 6 & 7|RxNum bits 4 & 5|Telemetry_Invert 3|Future_Use 2|Disable_Telemetry 1|Disable_CH_Mapping 0
   sub_protocol is 0..255 (bits 0..5 + bits 6..7)
   RxNum value is 0..63 (bits 0..3 + bits 4..5)
   Telemetry_Invert		=> 0x08	0=normal, 1=invert
   Future_Use			=> 0x04	0=      , 1=
   Disable_Telemetry	=> 0x02	0=enable, 1=disable
   Disable_CH_Mapping	=> 0x01	0=enable, 1=disable
  Stream[27.. 35] = between 0 and 9 bytes for additional protocol data
    Protocol specific use:
      FrSkyX and FrSkyX2: Stream[27] during bind Telem on=0x00,off=0x01 | CH1-8=0x00,CH9-16=0x02
      FrSkyX and FrSkyX2: Stream[27..34] during normal operation unstuffed SPort data to be sent
	  HoTT: Stream[27] 1 byte for telemetry type
	  DSM: Stream[27..33] Forward Programming
*/
/*
  Multiprotocol telemetry/command definition for OpenTX and erskyTX
  Based on #define MULTI_TELEMETRY enables OpenTX and erskyTX to get the multimodule status and select the correct telemetry type automatically.

  Serial: 100000 Baud 8e2 (same as input)

  TLV Protocol (type, length, value), allows a TX to ignore unknown messages

  Format: header (4 byte) + data (variable)
   [0] = 'M' (0x4d)
   [1] = 'P' (0x50)

   The first byte is deliberatly chosen to be different from other telemetry protocols
   (e.g. 0xAA for DSM/Multi, 0xAA for FlySky and 0x7e for Frsky) to allow a TX to detect
   the telemetry format of older versions

   [2] Type (see below)
   [3] Length (excluding the 4 header bytes)

   [4-xx] data

  Commands from TX to multi cannot be longer than 22 bytes (RXLen -4byte header)

  Type = 0x01 Multimodule Status:
   [4] Flags
   0x01 = Input signal detected
   0x02 = Serial mode enabled
   0x04 = Protocol is valid
   0x08 = Module is in binding mode
   0x10 = Module waits a bind event to load the protocol
   0x20 = Current protocol supports failsafe
   0x40 = Current protocol supports disable channel mapping
   0x80 = Data buffer is almost full
   [5] major
   [6] minor
   [7] revision
   [8] patchlevel
     version of multi code, should be displayed as major.minor.revision.patchlevel
   [9] channel order: CH4|CH3|CH2|CH1 with CHx value A=0,E=1,T=2,R=3
   [10] Next valid protocol number, can be used to skip invalid protocols
   [11] Prev valid protocol number, can be used to skip invalid protocols
   [12..18] Protocol name [7], not null terminated if prototcol len == 7
   [19>>4] Option text to be displayed: 
			OPTION_NONE		0	Hidden field
			OPTION_OPTION	1	"Option:"		value=-128..0(default)..127
			OPTION_RFTUNE	2	"RF freq tune:"	value=-128..0(default)..127
			OPTION_VIDFREQ	3	"Video freq:"	value=-128..0(default)..127
			OPTION_FIXEDID	4	"ID type:"		value="Auto":0(default), "Fixed":1
			OPTION_TELEM	5	"Telem:"		value="Off":0(default), "On":1, "Off+Aux":2, "On+Aux":3
			OPTION_SRVFREQ	6	"Servo freq(Hz):"	value="50":0(default).."400":70 => display=50+5*option with option=0..70
			OPTION_MAXTHR	7	"Max throw:"	value="Disabled":0, "Enabled":1
			OPTION_RFCHAN	8	"Select RF chan:"	value=-128..0(default)..127
			OPTION_RFPOWER	9	"RF power:"		"1.6mW":0(default),"2.0mW":1,"2.5mW":2,"3.2mW":3,"4.0mW":4,"5.0mW":5,"6.3mW":6,"7.9mW":7,"10mW\0":8,"13mW\0":9,"16mW\0":10,"20mW\0":11,"25mW\0":12,"32mW\0":13,"40mW\0":14,"50mW\0":15
			OPTION_WBUS		10	"Output:"		"WBUS":0(default),"PPM":1
   [19&0x0F] Number of sub protocols
   [20..27] Sub protocol name [8], not null terminated if sub prototcol len == 8
   If the current protocol is invalid [12..27] are all 0x00.
   
   more information can be added by specifing a longer length of the type, the TX will just ignore these bytes

  Type 0x02 Frksy S.port telemetry
  Type 0x03 Frsky Hub telemetry

	*No* usual frsky byte stuffing and without start/stop byte (0x7e)

  Type 0x04 Spektrum telemetry data
   data[0] TX RSSI
   data[1-15] telemetry data

  Type 0x05 DSM bind data
	data[0-16] DSM bind data

    technically DSM bind data is only 10 bytes but multi sends 16
    like with telemtery, check length field)

  Type 0x06 Flysky AFHDS2 telemetry data type 0xAA
   length: 29
   data[0] = RSSI value
   data[1-28] telemetry data

  Type 0x08 Input synchronisation
    Informs the TX about desired rate and current delay
    length: 4
    data[0-1]     Desired refresh rate in ??s
    data[2-3]     Time (??s) between last serial servo input received and servo input needed (lateness), TX should adjust its
                  sending time to minimise this value.
	data[4]		  Interval of this message in ms
	data[5]		  Input delay target in 10??s
   Note that there are protocols (AFHDS2A) that have a refresh rate that is smaller than the maximum achievable
   refresh rate via the serial protocol, in this case, the TX should double the rate and also subract this
   refresh rate from the input lag if the input lag is more than the desired refresh rate.
   The remote should try to get to zero of  (inputdelay+target*10).

  Type 0x0A Hitec telemetry data
   length: 8
   data[0] = TX RSSI value
   data[1] = TX LQI value
   data[2] = frame number
   data[3-7] telemetry data
   Full description at the bottom of Hitec_cc2500.ino

  Type 0x0B Spectrum Scanner telemetry data
   length: 6
   data[0] = start channel (2400 + x*0.333 Mhz)
   data[1-5] power levels

  Type 0x0C Flysky AFHDS2 telemetry data type 0xAC
   length: 29
   data[0] = RSSI value
   data[1-28] telemetry data

  Type 0x0D RX channels forwarding
   length: variable
   data[0] = received packets per second
   data[1] = rssi
   data[2] = start channel
   data[3] = number of channels to follow
   data[4-]= packed channels data, 11 bit per channel

  Type 0x0E HoTT telemetry
   length: 15
   data[0] = TX_RSSI
   data[1] = TX_LQI
   data[2] = type
   data[3] = page
   data[4-14] = data

  Type 0x0F M-Link telemetry
   length: 10
   data[0] = TX_RSSI
   data[1] = TX_LQI
   data[2] = telem_type
   data[3-9] = data

  Type 0x10 Config telemetry
   length: 22
   data[0..21] = Config data
   
  Type 0x11 Protocol list export via telemetry. Used by the protocol PROTO_PROTOLIST=0, the list entry is given by the Option field.
   length: variable
   data[0]     = protocol number, 0xFF is an invalid list entry (Option value too large), Option == 0xFF -> number of protocols in the list
   data[1..n]  = protocol name null terminated
   data[n+1]   = flags
                 flags>>4 Option text number to be displayed (check multi status for description)
                 flags&0x01 failsafe supported
                 flags&0x02 Channel Map Disabled supported
   data[n+2]   = number of sub protocols
   data[n+3]   = sub protocols text length, only sent if nbr_sub != 0
   data[n+4..] = sub protocol names, only sent if nbr_sub != 0
   
*/
