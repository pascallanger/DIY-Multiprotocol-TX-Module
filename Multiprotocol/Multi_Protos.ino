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

const char STR_FLYSKY[]		="FlySky";
const char STR_HUBSAN[]		="Hubsan";
const char STR_FRSKYD[]		="FrSky D";
const char STR_HISKY[]		="Hisky";
const char STR_V2X2[]		="V2x2";
const char STR_DSM[]		="DSM";
const char STR_DSM_RX[]		="DSM_RX";
const char STR_DEVO[]		="Devo";
const char STR_YD717[]		="YD717";
const char STR_KN[]			="KN";
const char STR_SYMAX[]		="SymaX";
const char STR_SLT[]		="SLT";
const char STR_CX10[]		="CX10";
const char STR_CG023[]		="CG023";
const char STR_BAYANG[]		="Bayang";
const char STR_FRSKYL[]		="FrSky L";
const char STR_FRSKYX[]		="FrSky X";
const char STR_FRSKYX2[]	="FrSkyX2";
const char STR_ESKY[]		="ESky";
const char STR_MT99XX[]		="MT99XX";
const char STR_MT99XX2[]	="MT99XX2";
const char STR_MJXQ[]		="MJXq";
const char STR_SHENQI[]		="Shenqi";
const char STR_FY326[]		="FY326";
const char STR_FUTABA[]		="Futaba";
const char STR_J6PRO[]		="J6 Pro";
const char STR_JJRC345[]	="JJRC345";
const char STR_JOYSWAY[]	="JOYSWAY";
const char STR_FQ777[]		="FQ777";
const char STR_ASSAN[]		="Assan";
const char STR_FRSKYV[]		="FrSky V";
const char STR_HONTAI[]		="Hontai";
const char STR_AFHDS2A[]	="FlSky2A";
const char STR_Q2X2[]		="Q2x2";
const char STR_WK2x01[]		="Walkera";
const char STR_Q303[]		="Q303";
const char STR_Q90C[]		="Q90C";
const char STR_GW008[]		="GW008";
const char STR_DM002[]		="DM002";
const char STR_CABELL[]		="Cabell";
const char STR_ESKY150[]	="Esky150";
const char STR_ESKY150V2[]	="EskyV2";
const char STR_H8_3D[]		="H8 3D";
const char STR_CORONA[]		="Corona";
const char STR_CFLIE[]		="CFlie";
const char STR_HITEC[]		="Hitec";
const char STR_WFLY[]		="WFLY";
const char STR_WFLY2[]		="WFLY2";
const char STR_BUGS[]		="Bugs";
const char STR_BUGSMINI[]	="BugMini";
const char STR_TRAXXAS[]	="Traxxas";
const char STR_NCC1701[]	="NCC1701";
const char STR_E01X[]		="E01X";
const char STR_V911S[]		="V911S";
const char STR_GD00X[]		="GD00x";
const char STR_V761[]		="V761";
const char STR_KF606[]		="KF606";
const char STR_REDPINE[]	="Redpine";
const char STR_POTENSIC[]	="Potensi";
const char STR_ZSX[]		="ZSX";
const char STR_HEIGHT[]		="Height";
const char STR_SCANNER[]	="Scanner";
const char STR_FRSKY_RX[]	="FrSkyRX";
const char STR_AFHDS2A_RX[]	="FS2A_RX";
const char STR_HOTT[]		="HoTT";
const char STR_FX816[]		="FX816";
const char STR_BAYANG_RX[]	="BayanRX";
const char STR_PELIKAN[]	="Pelikan";
const char STR_TIGER[]		="Tiger";
const char STR_XK[]			="XK";
const char STR_XN297DUMP[]	="XN297DP";
const char STR_FRSKYR9[]	="FrSkyR9";
const char STR_PROPEL[]		="Propel";
const char STR_SKYARTEC[]	="Skyartc";
const char STR_KYOSHO[]		="Kyosho";
const char STR_RLINK[]		="RadLink";
const char STR_REALACC[]	="Realacc";
const char STR_OMP[]		="OMP";
const char STR_MLINK[]		="M-Link";
const char STR_TEST[]		="Test";
const char STR_NANORF[]		="NanoRF";
const char STR_E016HV2[]    ="E016Hv2";
const char STR_E010R5[]     ="E010r5";
const char STR_LOLI[]       ="LOLI";
const char STR_E129[]       ="E129";
const char STR_E016H[]      ="E016H";
const char STR_IKEAANSLUTA[]="Ansluta";
const char STR_CONFIG[]     ="Config";
const char STR_LOSI[]       ="Losi";
const char STR_MOULDKG[]    ="MouldKg";
const char STR_XERALL[]     ="Xerall";

const char STR_SUBTYPE_FLYSKY[] =     "\x04""Std\0""V9x9""V6x6""V912""CX20";
const char STR_SUBTYPE_HUBSAN[] =     "\x04""H107""H301""H501";
const char STR_SUBTYPE_FRSKYD[] =     "\x06""D8\0   ""Cloned";
const char STR_SUBTYPE_FRSKYX[] =     "\x07""D16\0   ""D16 8ch""LBT(EU)""LBT 8ch""Cloned\0""Clo 8ch";
const char STR_SUBTYPE_HISKY[] =      "\x05""Std\0 ""HK310";
const char STR_SUBTYPE_V2X2[] =       "\x06""Std\0  ""JXD506""MR101\0";
const char STR_SUBTYPE_DSM[] =        "\x04""2 1F""2 2F""X 1F""X 2F""Auto""R 1F";
const char STR_SUBTYPE_DEVO[] =       "\x04""8ch\0""10ch""12ch""6ch\0""7ch\0";
const char STR_SUBTYPE_YD717[] =      "\x07""Std\0   ""SkyWlkr""Syma X4""XINXUN\0""NIHUI\0 ";
const char STR_SUBTYPE_KN[] =         "\x06""WLtoys""FeiLun";
const char STR_SUBTYPE_SYMAX[] =      "\x03""Std""X5C";
const char STR_SUBTYPE_SLT[] =        "\x06""V1_6ch""V2_8ch""Q100\0 ""Q200\0 ""MR100\0";
const char STR_SUBTYPE_CX10[] =       "\x07""Green\0 ""Blue\0  ""DM007\0 ""-\0     ""JC3015a""JC3015b""MK33041";
const char STR_SUBTYPE_CG023[] =      "\x05""Std\0 ""YD829";
const char STR_SUBTYPE_BAYANG[] =     "\x07""Std\0   ""H8S3D\0 ""X16 AH\0""IRDrone""DHD D4\0""QX100\0 ";
const char STR_SUBTYPE_MT99[] =       "\x06""MT99\0 ""H7\0   ""YZ\0   ""LS\0   ""FY805\0""A180\0 ""Dragon""F949G\0";
const char STR_SUBTYPE_MT992[] =      "\x04""PA18";
const char STR_SUBTYPE_MJXQ[] =       "\x07""WLH08\0 ""X600\0  ""X800\0  ""H26D\0  ""E010\0  ""H26WH\0 ""Phoenix";
const char STR_SUBTYPE_FY326[] =      "\x05""Std\0 ""FY319";
const char STR_SUBTYPE_HONTAI[] =     "\x07""Std\0   ""JJRC X1""X5C1\0  ""FQ_951";
const char STR_SUBTYPE_AFHDS2A[] =    "\x08""PWM,IBUS""PPM,IBUS""PWM,SBUS""PPM,SBUS""PWM,IB16""PPM,IB16""PWM,SB16""PPM,SB16";
const char STR_SUBTYPE_Q2X2[] =       "\x04""Q222""Q242""Q282";
const char STR_SUBTYPE_WK2x01[] =     "\x06""WK2801""WK2401""W6_5_1""W6_6_1""W6_HeL""W6_HeI";
const char STR_SUBTYPE_Q303[] =       "\x06""Std\0  ""CX35\0 ""CX10D\0""CX10WD";
const char STR_SUBTYPE_CABELL[] =     "\x07""V3\0    ""V3 Telm""-\0     ""-\0     ""-\0     ""-\0     ""F-Safe\0""Unbind\0";
const char STR_SUBTYPE_H83D[] =       "\x07""Std\0   ""H20H\0  ""H20Mini""H30Mini";
const char STR_SUBTYPE_CORONA[] =     "\x05""V1\0  ""V2\0  ""FD V3";
const char STR_SUBTYPE_HITEC[] =      "\x07""Optima\0""Opt Hub""Minima\0";
const char STR_SUBTYPE_BUGS_MINI[] =  "\x06""Std\0  ""Bugs3H";
const char STR_SUBTYPE_TRAXXAS[] =    "\x04""6519";
const char STR_SUBTYPE_E01X[] =       "\x05""E012\0""E015\0";
const char STR_SUBTYPE_GD00X[] =      "\x05""GD_V1""GD_V2";
const char STR_SUBTYPE_REDPINE[] =    "\x04""Fast""Slow";
const char STR_SUBTYPE_POTENSIC[] =   "\x03""A20";
const char STR_SUBTYPE_ZSX[] =        "\x07""280JJRC";
const char STR_SUBTYPE_HEIGHT[] =     "\x03""5ch""8ch";
const char STR_SUBTYPE_FX816[] =      "\x03""P38";
const char STR_SUBTYPE_XN297DUMP[] =  "\x07""250Kbps""1Mbps\0 ""2Mbps\0 ""Auto\0  ""NRF\0   ""CC2500\0";
const char STR_SUBTYPE_ESKY150[] =    "\x03""4ch""7ch";
const char STR_SUBTYPE_ESKY150V2[] =  "\x05""150V2";
const char STR_SUBTYPE_V911S[] =      "\x05""V911S""E119\0";
const char STR_SUBTYPE_XK[] =         "\x04""X450""X420";
const char STR_SUBTYPE_FRSKYR9[] =    "\x07""915MHz\0""868MHz\0""915 8ch""868 8ch""FCC\0   ""--\0    ""FCC 8ch""-- 8ch\0";
const char STR_SUBTYPE_ESKY[] =       "\x03""Std""ET4";
const char STR_SUBTYPE_PROPEL[] =     "\x04""74-Z";
const char STR_SUBTYPE_FRSKYL[] =     "\x08""LR12\0   ""LR12 6ch";
const char STR_SUBTYPE_WFLY[] =       "\x05""WFR0x";
const char STR_SUBTYPE_WFLY2[] =      "\x05""RF20x";
const char STR_SUBTYPE_HOTT[] =       "\x07""Sync\0  ""No_Sync";
const char STR_SUBTYPE_PELIKAN[] =    "\x05""Pro\0 ""Lite\0""SCX24";
const char STR_SUBTYPE_V761[] =       "\x03""3ch""4ch";
const char STR_SUBTYPE_RLINK[] =      "\x07""Surface""Air\0   ""DumboRC";
const char STR_SUBTYPE_REALACC[] =    "\x03""R11";
const char STR_SUBTYPE_KYOSHO[] =     "\x04""FHSS""Hype";
const char STR_SUBTYPE_FUTABA[] =     "\x05""SFHSS";
const char STR_SUBTYPE_JJRC345[] =    "\x08""JJRC345\0""SkyTmblr";
const char STR_SUBTYPE_MOULKG[] =     "\x06""Analog""Digit\0";
const char STR_SUBTYPE_KF606[] =      "\x06""KF606\0""MIG320";
const char STR_SUBTYPE_E129[] =       "\x04""E129""C186";

#define NO_SUBTYPE		nullptr

#ifdef SEND_CPPM
	const char STR_SUB_FRSKY_RX[] =   "\x07""Multi\0 ""CloneTX""EraseTX""CPPM\0  ";
	#define FRCPPM   4
	const char STR_CPPM[] =           "\x05""Multi""CPPM\0";
	#define NBR_CPPM 2
#else
	const char STR_SUB_FRSKY_RX[] =   "\x07""Multi\0 ""CloneTX""EraseTX";
	#define FRCPPM   3
	#define STR_CPPM NO_SUBTYPE
	#define NBR_CPPM 0
#endif

enum
{
	OPTION_NONE,
	OPTION_OPTION,
	OPTION_RFTUNE,
	OPTION_VIDFREQ,
	OPTION_FIXEDID,
	OPTION_TELEM,
	OPTION_SRVFREQ,
	OPTION_MAXTHR,
	OPTION_RFCHAN,
	OPTION_RFPOWER,
	OPTION_WBUS,
};

const mm_protocol_definition multi_protocols[] = {
// Protocol number, Protocol String, Sub_protocol strings, Number of sub_protocols, Option type, Failsafe, ChMap, RF switch, Init, Callback
	#if defined(MULTI_CONFIG_INO)
		{PROTO_CONFIG,     STR_CONFIG,    NO_SUBTYPE,            0, OPTION_NONE,    0, 0, 0,         CONFIG_init,     CONFIG_callback     },
	#endif
	#if defined(ASSAN_NRF24L01_INO)
		{PROTO_ASSAN,      STR_ASSAN,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    ASSAN_init,      ASSAN_callback      },
	#endif
	#if defined(BAYANG_NRF24L01_INO)
		{PROTO_BAYANG,     STR_BAYANG,    STR_SUBTYPE_BAYANG,    6, OPTION_TELEM,   0, 0, SW_NRF,    BAYANG_init,     BAYANG_callback     },
	#endif
	#if defined(BAYANG_RX_NRF24L01_INO)
		{PROTO_BAYANG_RX,  STR_BAYANG_RX, STR_CPPM,       NBR_CPPM, OPTION_NONE,    0, 0, SW_NRF,    BAYANG_RX_init,  BAYANG_RX_callback  },	
	#endif
	#if defined(BUGS_A7105_INO)
		{PROTO_BUGS,       STR_BUGS,      NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_A7105,  BUGS_init,       BUGS_callback       },
	#endif
	#if defined(BUGSMINI_NRF24L01_INO)
		{PROTO_BUGSMINI,   STR_BUGSMINI,  STR_SUBTYPE_BUGS_MINI, 2, OPTION_NONE,    0, 0, SW_NRF,    BUGSMINI_init,   BUGSMINI_callback   },
	#endif
	#if defined(CABELL_NRF24L01_INO)                           
		{PROTO_CABELL,     STR_CABELL,    STR_SUBTYPE_CABELL,    8, OPTION_OPTION,  0, 0, SW_NRF,    CABELL_init,     CABELL_callback     },
	#endif
	#if defined(CFLIE_NRF24L01_INO)
		{PROTO_CFLIE,      STR_CFLIE,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    CFLIE_init,      CFLIE_callback      }, // review protocol
	#endif
	#if defined(CG023_NRF24L01_INO)
		{PROTO_CG023,      STR_CG023,     STR_SUBTYPE_CG023,     2, OPTION_NONE,    0, 0, SW_NRF,    CG023_init,      CG023_callback      },
	#endif
	#if defined(CORONA_CC2500_INO)
		{PROTO_CORONA,     STR_CORONA,    STR_SUBTYPE_CORONA,    3, OPTION_RFTUNE,  0, 0, SW_CC2500, CORONA_init,     CORONA_callback     },
	#endif
	#if defined(CX10_NRF24L01_INO)
		{PROTO_CX10,       STR_CX10,      STR_SUBTYPE_CX10,      7, OPTION_NONE,    0, 0, SW_NRF,    CX10_init,       CX10_callback       },
	#endif
	#if defined(DEVO_CYRF6936_INO)
		{PROTO_DEVO,       STR_DEVO,      STR_SUBTYPE_DEVO,      5, OPTION_FIXEDID, 1, 1, SW_CYRF,   DEVO_init,       DEVO_callback       },
	#endif
	#if defined(DM002_NRF24L01_INO)
		{PROTO_DM002,      STR_DM002,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    DM002_init,      DM002_callback      },
	#endif
	#if defined(DSM_CYRF6936_INO)
		{PROTO_DSM,        STR_DSM,       STR_SUBTYPE_DSM,       6, OPTION_MAXTHR,  0, 1, SW_CYRF,   DSM_init,        DSM_callback        },
	#endif
	#if defined(DSM_RX_CYRF6936_INO)
		{PROTO_DSM_RX,     STR_DSM_RX,    STR_CPPM,       NBR_CPPM, OPTION_NONE,    0, 1, SW_CYRF,   DSM_RX_init,     DSM_RX_callback     },
	#endif
	#if defined(E010R5_CYRF6936_INO)
		{PROTO_E010R5,     STR_E010R5,    NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_CYRF,   E010R5_init,     E010R5_callback     },
	#endif
	#if defined(E016H_NRF24L01_INO)
		{PROTO_E016H,      STR_E016H,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    E016H_init,      E016H_callback      },
	#endif
	#if defined(E016HV2_CC2500_INO)
		{PROTO_E016HV2,    STR_E016HV2,   NO_SUBTYPE,            0, OPTION_RFTUNE,  0, 0, SW_CC2500, E016HV2_init,    E016HV2_callback    },
	#endif
	#if defined(E01X_CYRF6936_INO)
		{PROTO_E01X,       STR_E01X,      STR_SUBTYPE_E01X,      2, OPTION_NONE,    0, 0, SW_CYRF,   E01X_init,       E01X_callback       },
	#endif
	#if defined(E129_CYRF6936_INO)
		{PROTO_E129,       STR_E129,      STR_SUBTYPE_E129,      2, OPTION_NONE,    0, 0, SW_CYRF,   E129_init,       E129_callback       },
	#endif
	#if defined(ESKY_NRF24L01_INO)
		{PROTO_ESKY,       STR_ESKY,      STR_SUBTYPE_ESKY,      2, OPTION_NONE,    0, 1, SW_NRF,    ESKY_init,       ESKY_callback       },
	#endif
	#if defined(ESKY150_NRF24L01_INO)
		{PROTO_ESKY150,    STR_ESKY150,   STR_SUBTYPE_ESKY150,   2, OPTION_NONE,    0, 0, SW_NRF,    ESKY150_init,    ESKY150_callback    },
	#endif
	#if defined(ESKY150V2_CC2500_INO)
		{PROTO_ESKY150V2,  STR_ESKY150V2, STR_SUBTYPE_ESKY150V2, 1, OPTION_RFTUNE,  0, 1, SW_CC2500, ESKY150V2_init,  ESKY150V2_callback  },
	#endif
	#if defined(FLYSKY_A7105_INO)
		{PROTO_FLYSKY,     STR_FLYSKY,    STR_SUBTYPE_FLYSKY,    5, OPTION_NONE,    0, 1, SW_A7105,  FLYSKY_init,     FLYSKY_callback     },
	#endif
	#if defined(AFHDS2A_A7105_INO)
		{PROTO_AFHDS2A,    STR_AFHDS2A,   STR_SUBTYPE_AFHDS2A,   8, OPTION_SRVFREQ, 1, 1, SW_A7105,  AFHDS2A_init,    AFHDS2A_callback    },
	#endif
	#if defined(AFHDS2A_RX_A7105_INO)
		{PROTO_AFHDS2A_RX, STR_AFHDS2A_RX,STR_CPPM,       NBR_CPPM, OPTION_NONE,    0, 0, SW_A7105,  AFHDS2A_RX_init, AFHDS2A_RX_callback },
	#endif
	#if defined(FQ777_NRF24L01_INO)
		{PROTO_FQ777,      STR_FQ777,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    FQ777_init,      FQ777_callback      },
	#endif
//OpenTX 2.3.x issue: DO NOT CHANGE ORDER below
	#if defined(FRSKY_RX_CC2500_INO)
		{PROTO_FRSKY_RX,   STR_FRSKY_RX,  STR_SUB_FRSKY_RX, FRCPPM, OPTION_RFTUNE,  0, 0, SW_CC2500, FRSKY_RX_init,   FRSKY_RX_callback   },
	#endif
	#if defined(FRSKYD_CC2500_INO)
		{PROTO_FRSKYD,     STR_FRSKYD,    STR_SUBTYPE_FRSKYD,    2, OPTION_RFTUNE,  0, 0, SW_CC2500, FRSKYD_init,     FRSKYD_callback     },
	#endif
	#if defined(FRSKYV_CC2500_INO)
		{PROTO_FRSKYV,      STR_FRSKYV,   NO_SUBTYPE,            0, OPTION_RFTUNE,  0, 0, SW_CC2500, FRSKYV_init,     FRSKYV_callback     },
	#endif
	#if defined(FRSKYX_CC2500_INO)
		{PROTO_FRSKYX,     STR_FRSKYX,    STR_SUBTYPE_FRSKYX,    6, OPTION_RFTUNE,  1, 0, SW_CC2500, FRSKYX_init,     FRSKYX_callback     },
		{PROTO_FRSKYX2,    STR_FRSKYX2,   STR_SUBTYPE_FRSKYX,    6, OPTION_RFTUNE,  1, 0, SW_CC2500, FRSKYX_init,     FRSKYX_callback     },
	#endif
//OpenTX 2.3.x issue: DO NOT CHANGE ORDER above
	#if defined(FRSKYL_CC2500_INO)
		{PROTO_FRSKYL,     STR_FRSKYL,    STR_SUBTYPE_FRSKYL,    2, OPTION_RFTUNE,  0, 0, SW_CC2500, FRSKYL_init,     FRSKYL_callback     },
	#endif
	#if defined(FRSKYR9_SX1276_INO)
		#if MULTI_5IN1_INTERNAL == T18
		{PROTO_FRSKY_R9,   STR_FRSKYR9,   STR_SUBTYPE_FRSKYR9,   8, OPTION_NONE,    1, 0, 0,         FRSKYR9_init,    FRSKYR9_callback    },
		#else	// DIY & T-Lite
		{PROTO_FRSKY_R9,   STR_FRSKYR9,   STR_SUBTYPE_FRSKYR9,   8, OPTION_RFPOWER, 1, 0, 0,         FRSKYR9_init,    FRSKYR9_callback    },
		#endif
	#endif
	#if defined(FUTABA_CC2500_INO)
		{PROTO_FUTABA,     STR_FUTABA,    STR_SUBTYPE_FUTABA,    1, OPTION_RFTUNE,  1, 1, SW_CC2500, SFHSS_init,      SFHSS_callback      },
	#endif
	#if defined(FX816_NRF24L01_INO)
		{PROTO_FX816,      STR_FX816,     STR_SUBTYPE_FX816,     1, OPTION_NONE,    0, 0, SW_NRF,    FX816_init,      FX816_callback      },
	#endif
	#if defined(FY326_NRF24L01_INO)
		{PROTO_FY326,      STR_FY326,     STR_SUBTYPE_FY326,     2, OPTION_NONE,    0, 0, SW_NRF,    FY326_init,      FY326_callback      },
	#endif
	#if defined(GD00X_CCNRF_INO)
		{PROTO_GD00X,      STR_GD00X,     STR_SUBTYPE_GD00X,     2, OPTION_RFTUNE,  0, 0, SW_NRF,    GD00X_init,      GD00X_callback      },
	#endif
	#if defined(GW008_NRF24L01_INO)
		{PROTO_GW008,      STR_GW008,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    GW008_init,      GW008_callback      },
	#endif
	#if defined(H8_3D_NRF24L01_INO)
		{PROTO_H8_3D,      STR_H8_3D,     STR_SUBTYPE_H83D,      4, OPTION_NONE,    0, 0, SW_NRF,    H8_3D_init,      H8_3D_callback      },
	#endif
	#if defined(HEIGHT_A7105_INO)
		{PROTO_HEIGHT,     STR_HEIGHT,    STR_SUBTYPE_HEIGHT,    2, OPTION_NONE,    0, 0, SW_A7105,  HEIGHT_init,     HEIGHT_callback     },
	#endif
	#if defined(HISKY_NRF24L01_INO)
		{PROTO_HISKY,      STR_HISKY,     STR_SUBTYPE_HISKY,     2, OPTION_NONE,    1, 1, SW_NRF,    HISKY_init,      HISKY_callback      },
	#endif
	#if defined(HITEC_CC2500_INO)
		{PROTO_HITEC,      STR_HITEC,     STR_SUBTYPE_HITEC,     3, OPTION_RFTUNE,  0, 0, SW_CC2500, HITEC_init,      HITEC_callback      },
	#endif
	#if defined(HONTAI_NRF24L01_INO)
		{PROTO_HONTAI,     STR_HONTAI,    STR_SUBTYPE_HONTAI,    4, OPTION_NONE,    0, 0, SW_NRF,    HONTAI_init,     HONTAI_callback     },
	#endif
	#if defined(HOTT_CC2500_INO)
		{PROTO_HOTT,       STR_HOTT,      STR_SUBTYPE_HOTT,      2, OPTION_RFTUNE,  1, 0, SW_CC2500, HOTT_init,       HOTT_callback       },
	#endif
	#if defined(HUBSAN_A7105_INO)
		{PROTO_HUBSAN,     STR_HUBSAN,    STR_SUBTYPE_HUBSAN,    3, OPTION_VIDFREQ, 0, 0, SW_A7105,  HUBSAN_init,     HUBSAN_callback     },
	#endif
	#if defined(IKEAANSLUTA_CC2500_INO)
		{PROTO_IKEAANSLUTA,STR_IKEAANSLUTA,NO_SUBTYPE,           0, OPTION_OPTION,  0, 0, SW_CC2500, IKEAANSLUTA_init,IKEAANSLUTA_callback },
	#endif
	#if defined(J6PRO_CYRF6936_INO)
		{PROTO_J6PRO,      STR_J6PRO,     NO_SUBTYPE,            0, OPTION_NONE,    0, 1, SW_CYRF,   J6PRO_init,      J6PRO_callback      },
	#endif
	#if defined(JJRC345_NRF24L01_INO)
		{PROTO_JJRC345,    STR_JJRC345,   STR_SUBTYPE_JJRC345,   2, OPTION_NONE,    0, 0, SW_NRF,    JJRC345_init,    JJRC345_callback    },
	#endif
	#if defined(JOYSWAY_A7105_INO)
		{PROTO_JOYSWAY,    STR_JOYSWAY,   NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_A7105,  JOYSWAY_init,    JOYSWAY_callback    },
	#endif
	#if defined(KF606_CCNRF_INO)
		{PROTO_KF606,      STR_KF606,     STR_SUBTYPE_KF606,     2, OPTION_RFTUNE,  0, 0, SW_NRF,    KF606_init,      KF606_callback      },
	#endif
	#if defined(KN_NRF24L01_INO)
		{PROTO_KN,         STR_KN,        STR_SUBTYPE_KN,        2, OPTION_NONE,    0, 0, SW_NRF,    KN_init,         KN_callback         },
	#endif
	#if defined(KYOSHO_A7105_INO)
		{PROTO_KYOSHO,     STR_KYOSHO,    STR_SUBTYPE_KYOSHO,    2, OPTION_NONE,    0, 1, SW_A7105,  KYOSHO_init,     KYOSHO_callback     },
	#endif
	#if defined(LOLI_NRF24L01_INO)
		{PROTO_LOLI,       STR_LOLI,      NO_SUBTYPE,            0, OPTION_NONE,    1, 0, SW_NRF,    LOLI_init,       LOLI_callback       },
	#endif
	#if defined(LOSI_CYRF6936_INO)
		{PROTO_LOSI,       STR_LOSI,      NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_CYRF,   LOSI_init,       LOSI_callback       },
	#endif
	#if defined(MJXQ_CCNRF_INO)
		{PROTO_MJXQ,       STR_MJXQ,      STR_SUBTYPE_MJXQ,      7, OPTION_NONE,    0, 0, SW_NRF,    MJXQ_init,       MJXQ_callback       },
	#endif
	#if defined(MLINK_CYRF6936_INO)
		{PROTO_MLINK,      STR_MLINK,     NO_SUBTYPE,            0, OPTION_NONE,    1, 0, SW_CYRF,   MLINK_init,      MLINK_callback      },
	#endif
	#if defined(MOULDKG_NRF24L01_INO)
		{PROTO_MOULDKG,    STR_MOULDKG,   STR_SUBTYPE_MOULKG,    2, OPTION_OPTION,  0, 0, SW_NRF,    MOULDKG_init,    MOULDKG_callback    },
	#endif
	#if defined(MT99XX_CCNRF_INO)
		{PROTO_MT99XX,     STR_MT99XX,    STR_SUBTYPE_MT99,      8, OPTION_NONE,    0, 0, SW_NRF,    MT99XX_init,     MT99XX_callback     },
	#endif
	#if defined(MT99XX_CCNRF_INO)
		{PROTO_MT99XX2,    STR_MT99XX2,   STR_SUBTYPE_MT992,     1, OPTION_NONE,    0, 0, SW_NRF,    MT99XX_init,     MT99XX_callback     },
	#endif
	#if defined(NCC1701_NRF24L01_INO)
		{PROTO_NCC1701,    STR_NCC1701,   NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    NCC_init,        NCC_callback        },
	#endif
	#if defined(OMP_CCNRF_INO)
		{PROTO_OMP,        STR_OMP,       NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    OMP_init,        OMP_callback        },
	#endif
	#if defined(PELIKAN_A7105_INO)
		{PROTO_PELIKAN,    STR_PELIKAN,   STR_SUBTYPE_PELIKAN,   3, OPTION_NONE,    0, 1, SW_A7105,  PELIKAN_init,    PELIKAN_callback    },
	#endif
	#if defined(POTENSIC_NRF24L01_INO)
		{PROTO_POTENSIC,   STR_POTENSIC,  STR_SUBTYPE_POTENSIC,  1, OPTION_NONE,    0, 0, SW_NRF,    POTENSIC_init,   POTENSIC_callback   },
	#endif
	#if defined(PROPEL_NRF24L01_INO)
		{PROTO_PROPEL,     STR_PROPEL,    STR_SUBTYPE_PROPEL,    1, OPTION_NONE,    0, 0, SW_NRF,    PROPEL_init,     PROPEL_callback     },
	#endif
	#if defined(CX10_NRF24L01_INO)
		{PROTO_Q2X2,       STR_Q2X2,      STR_SUBTYPE_Q2X2,      3, OPTION_NONE,    0, 0, SW_NRF,    CX10_init,       CX10_callback       },
	#endif
	#if defined(Q303_CCNRF_INO)
		{PROTO_Q303,       STR_Q303,      STR_SUBTYPE_Q303,      4, OPTION_NONE,    0, 0, SW_NRF,    Q303_init,       Q303_callback       },
	#endif
	#if defined(Q90C_CCNRF_INO)
		{PROTO_Q90C,       STR_Q90C,      NO_SUBTYPE,            0, OPTION_RFTUNE,  0, 0, SW_NRF,    Q90C_init,       Q90C_callback       },
	#endif
	#if defined(RLINK_CC2500_INO)
		{PROTO_RLINK,      STR_RLINK,     STR_SUBTYPE_RLINK,     3, OPTION_RFTUNE,  0, 0, SW_CC2500, RLINK_init,      RLINK_callback      },
	#endif
	#if defined(REALACC_NRF24L01_INO)
		{PROTO_REALACC,    STR_REALACC,   STR_SUBTYPE_REALACC,   1, OPTION_NONE,    0, 0, SW_NRF,    REALACC_init,    REALACC_callback    },
	#endif
	#if defined(REDPINE_CC2500_INO)
		{PROTO_REDPINE,    STR_REDPINE,   STR_SUBTYPE_REDPINE,   2, OPTION_RFTUNE,  0, 0, SW_CC2500, REDPINE_init,    REDPINE_callback    },
	#endif
	#if defined(SCANNER_CC2500_INO)
		{PROTO_SCANNER,    STR_SCANNER,   NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_CC2500, SCANNER_init,    SCANNER_callback    },
	#endif
	#if defined(SHENQI_NRF24L01_INO)
		{PROTO_SHENQI,     STR_SHENQI,    NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    SHENQI_init,     SHENQI_callback     },
	#endif
	#if defined(SKYARTEC_CC2500_INO)
		{PROTO_SKYARTEC,   STR_SKYARTEC,  NO_SUBTYPE,            0, OPTION_RFTUNE,  0, 1, SW_CC2500, SKYARTEC_init,   SKYARTEC_callback   },
	#endif
	#if defined(SLT_CCNRF_INO)
		{PROTO_SLT,        STR_SLT,       STR_SUBTYPE_SLT,       5, OPTION_RFTUNE,  0, 1, SW_NRF,    SLT_init,        SLT_callback        },
	#endif
	#if defined(SYMAX_NRF24L01_INO)
		{PROTO_SYMAX,      STR_SYMAX,     STR_SUBTYPE_SYMAX,     2, OPTION_NONE,    0, 0, SW_NRF,    SYMAX_init,      SYMAX_callback      },
	#endif
	#if defined(TIGER_NRF24L01_INO)
		{PROTO_TIGER,      STR_TIGER,     NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    TIGER_init,      TIGER_callback      },	
	#endif
	#if defined(TRAXXAS_CYRF6936_INO)
		{PROTO_TRAXXAS,    STR_TRAXXAS,   STR_SUBTYPE_TRAXXAS,   1, OPTION_NONE,    0, 0, SW_CYRF,   TRAXXAS_init,    TRAXXAS_callback    },
	#endif
	#if defined(V2X2_NRF24L01_INO)
		{PROTO_V2X2,       STR_V2X2,      STR_SUBTYPE_V2X2,      3, OPTION_NONE,    0, 0, SW_NRF,    V2X2_init,       V2X2_callback       },
	#endif
	#if defined(V761_NRF24L01_INO)
		{PROTO_V761,       STR_V761,      STR_SUBTYPE_V761,      2, OPTION_NONE,    0, 0, SW_NRF,    V761_init,       V761_callback       },
	#endif
	#if defined(V911S_CCNRF_INO)
		{PROTO_V911S,      STR_V911S,     STR_SUBTYPE_V911S,     2, OPTION_RFTUNE,  0, 0, SW_NRF,    V911S_init,      V911S_callback      },
	#endif
	#if defined(WK2x01_CYRF6936_INO)
		{PROTO_WK2x01,     STR_WK2x01,    STR_SUBTYPE_WK2x01,    6, OPTION_NONE,    1, 1, SW_CYRF,   WK_init,         WK_callback         },
	#endif
	#if defined(WFLY_CYRF6936_INO)
		{PROTO_WFLY,       STR_WFLY,      STR_SUBTYPE_WFLY,      1, OPTION_NONE,    1, 0, SW_CYRF,   WFLY_init,       WFLY_callback       },
	#endif
	#if defined(WFLY2_A7105_INO)
		{PROTO_WFLY2,      STR_WFLY2,     STR_SUBTYPE_WFLY2,     1, OPTION_OPTION,  1, 0, SW_A7105,  WFLY2_init,      WFLY2_callback      },
//		{PROTO_WFLY2,      STR_WFLY2,     STR_SUBTYPE_WFLY2,     1, OPTION_WBUS,    1, 0, SW_A7105,  WFLY2_init,      WFLY2_callback      },// crash OpenTX...
	#endif
	#if defined(XERALL_NRF24L01_INO)
		{PROTO_XERALL,     STR_XERALL,    NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    XERALL_init,     XERALL_callback     },	
	#endif
	#if defined(XK_CCNRF_INO)
		{PROTO_XK,         STR_XK,        STR_SUBTYPE_XK,        2, OPTION_RFTUNE,  0, 0, SW_NRF,    XK_init,         XK_callback         },	
	#endif
	#if defined(XN297DUMP_NRF24L01_INO)
		{PROTO_XN297DUMP,  STR_XN297DUMP, STR_SUBTYPE_XN297DUMP, 6, OPTION_RFCHAN,  0, 0, SW_NRF,    XN297Dump_init,  XN297Dump_callback  },
	#endif
	#if defined(YD717_NRF24L01_INO)
		{PROTO_YD717,      STR_YD717,     STR_SUBTYPE_YD717,     5, OPTION_NONE,    0, 0, SW_NRF,    YD717_init,      YD717_callback      },
	#endif
	#if defined(ZSX_NRF24L01_INO)
		{PROTO_ZSX,        STR_ZSX,       STR_SUBTYPE_ZSX,       1, OPTION_NONE,    0, 0, SW_NRF,    ZSX_init,        ZSX_callback        },
	#endif
	#if defined(TEST_CC2500_INO)
		{PROTO_TEST,       STR_TEST,      NO_SUBTYPE,            0, OPTION_RFTUNE,  0, 0, SW_NRF,    TEST_init,       TEST_callback       },
	#endif
	#if defined(NANORF_NRF24L01_INO)
		{PROTO_NANORF,     STR_NANORF,    NO_SUBTYPE,            0, OPTION_NONE,    0, 0, SW_NRF,    NANORF_init,     NANORF_callback     },
	#endif
		{0xFF,             nullptr,       nullptr,               0, 0,              0, 0, 0,         nullptr,         nullptr             }
};

#ifdef MULTI_TELEMETRY
uint16_t PROTOLIST_callback()
{
	if(option != prev_option)
	{//Only send once
		/* Type 0x11 Protocol list export via telemetry. Used by the protocol PROTO_PROTOLIST=0, the list entry is given by the Option field.
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
		prev_option = option;

		if(option >= (sizeof(multi_protocols)/sizeof(mm_protocol_definition)) - 1)
		{//option is above the end of the list
			//Header
			multi_send_header(MULTI_TELEMETRY_PROTO, 1);
			if(option == 0xFF)
				Serial_write((sizeof(multi_protocols)/sizeof(mm_protocol_definition)) - 1);	//Nbr proto
			else
				Serial_write(0xFF);															//Error
		}
		else
		{//valid option value
			uint8_t proto_len = strlen(multi_protocols[option].ProtoString) + 1;
			uint8_t nbr_sub = multi_protocols[option].nbrSubProto;
			uint8_t sub_len = 0;
			if(nbr_sub)
				sub_len = multi_protocols[option].SubProtoString[0];
			
			//Header
			multi_send_header(MULTI_TELEMETRY_PROTO, 1 + proto_len + 1 + 1 + (nbr_sub?1:0) + (nbr_sub * sub_len));
			//Protocol number
			Serial_write(multi_protocols[option].protocol);
			//Protocol name
			for(uint8_t i=0;i<proto_len;i++)
				Serial_write(multi_protocols[option].ProtoString[i]);
			//Flags
			uint8_t flags=0;
			#ifdef FAILSAFE_ENABLE
				if(multi_protocols[option].failSafe)
					flags |= 0x01;		//Failsafe supported
			#endif
			if(multi_protocols[option].chMap)
				flags |= 0x02;			//Disable_ch_mapping supported
			Serial_write( flags | (multi_protocols[option].optionType<<4));	// flags && option type
			//Number of sub protocols
			Serial_write(nbr_sub);
			
			if(nbr_sub !=0 )
			{//Sub protocols length and texts
				for(uint8_t i=0;i<=nbr_sub*sub_len;i++)
					Serial_write(multi_protocols[option].SubProtoString[i]);
			}
		}
	}
	return 1000;
}
#endif