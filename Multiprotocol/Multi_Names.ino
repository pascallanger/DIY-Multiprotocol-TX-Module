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

#if defined(MULTI_NAMES)

const char STR_FLYSKY[]		="FlySky";
const char STR_HUBSAN[]		="Hubsan";
const char STR_FRSKYD[]		="FrSky D";
const char STR_HISKY[]		="Hisky";
const char STR_V2X2[]		="V2x2";
const char STR_DSM[]		="DSM";
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
const char STR_MJXQ[]		="MJXq";
const char STR_SHENQI[]		="Shenqi";
const char STR_FY326[]		="FY326";
const char STR_SFHSS[]		="SFHSS";
const char STR_J6PRO[]		="J6 Pro";
const char STR_FQ777[]		="FQ777";
const char STR_ASSAN[]		="Assan";
const char STR_FRSKYV[]		="FrSky V";
const char STR_HONTAI[]		="Hontai";
const char STR_AFHDS2A[]	="FSky 2A";
const char STR_Q2X2[]		="Q2x2";
const char STR_WK2x01[]		="Walkera";
const char STR_Q303[]		="Q303";
const char STR_GW008[]		="GW008";
const char STR_DM002[]		="DM002";
const char STR_CABELL[]		="Cabell";
const char STR_ESKY150[]	="Esky150";
const char STR_ESKY150V2[]	="EskyV2";
const char STR_H8_3D[]		="H8 3D";
const char STR_CORONA[]		="Corona";
const char STR_CFLIE[]		="CFlie";
const char STR_HITEC[]		="Hitec";
const char STR_WFLY[]		="WFly";
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
const char STR_FLYZONE[]	="FlyZone";
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
const char STR_PROPEL[]		="PROPEL";
const char STR_SKYARTEC[]	="Skyartc";

const char STR_SUBTYPE_FLYSKY[] =     "\x04""Std\0""V9x9""V6x6""V912""CX20";
const char STR_SUBTYPE_HUBSAN[] =     "\x04""H107""H301""H501";
const char STR_SUBTYPE_FRSKYD[] =     "\x06""D8\0   ""Cloned";
const char STR_SUBTYPE_FRSKYX[] =     "\x07""D16\0   ""D16 8ch""LBT(EU)""LBT 8ch""Cloned\0";
const char STR_SUBTYPE_HISKY[] =      "\x05""Std\0 ""HK310";
const char STR_SUBTYPE_V2X2[] =       "\x06""Std\0  ""JXD506";
const char STR_SUBTYPE_DSM[] =        "\x06""2 22ms""2 11ms""X 22ms""X 11ms";
const char STR_SUBTYPE_DEVO[] =       "\x04""8ch\0""10ch""12ch""6ch\0""7ch\0";
const char STR_SUBTYPE_YD717[] =      "\x07""Std\0   ""SkyWlkr""Syma X4""XINXUN\0""NIHUI\0 ";
const char STR_SUBTYPE_KN[] =         "\x06""WLtoys""FeiLun";
const char STR_SUBTYPE_SYMAX[] =      "\x03""Std""X5C";
const char STR_SUBTYPE_SLT[] =        "\x06""V1_6ch""V2_8ch""Q100\0 ""Q200\0 ""MR100\0";
const char STR_SUBTYPE_CX10[] =       "\x07""Green\0 ""Blue\0  ""DM007\0 ""-\0     ""JC3015a""JC3015b""MK33041";
const char STR_SUBTYPE_CG023[] =      "\x05""Std\0 ""YD829";
const char STR_SUBTYPE_BAYANG[] =     "\x07""Std\0   ""H8S3D\0 ""X16 AH\0""IRDrone""DHD D4";
const char STR_SUBTYPE_MT99[] =       "\x06""MT99\0 ""H7\0   ""YZ\0   ""LS\0   ""FY805";
const char STR_SUBTYPE_MJXQ[] =       "\x07""WLH08\0 ""X600\0  ""X800\0  ""H26D\0  ""E010\0  ""H26WH\0 ""Phoenix";
const char STR_SUBTYPE_FY326[] =      "\x05""Std\0 ""FY319";
const char STR_SUBTYPE_HONTAI[] =     "\x07""Std\0   ""JJRC X1""X5C1\0  ""FQ_951";
const char STR_SUBTYPE_AFHDS2A[] =    "\x08""PWM,IBUS""PPM,IBUS""PWM,SBUS""PPM,SBUS";
const char STR_SUBTYPE_Q2X2[] =       "\x04""Q222""Q242""Q282";
const char STR_SUBTYPE_WK2x01[] =     "\x06""WK2801""WK2401""W6_5_1""W6_6_1""W6_HeL""W6_HeI";
const char STR_SUBTYPE_Q303[] =       "\x06""Std\0  ""CX35\0 ""CX10D\0""CX10WD";
const char STR_SUBTYPE_CABELL[] =     "\x07""V3\0    ""V3 Telm""-\0     ""-\0     ""-\0     ""-\0     ""F-Safe\0""Unbind\0";
const char STR_SUBTYPE_H83D[] =       "\x07""Std\0   ""H20H\0  ""H20Mini""H30Mini";
const char STR_SUBTYPE_CORONA[] =     "\x05""V1\0  ""V2\0  ""FD V3";
const char STR_SUBTYPE_HITEC[] =      "\x07""Optima\0""Opt Hub""Minima\0";
const char STR_SUBTYPE_BUGS_MINI[] =  "\x06""Std\0  ""Bugs3H";
const char STR_SUBTYPE_TRAXXAS[] =    "\x04""6519";
const char STR_SUBTYPE_E01X[] =       "\x05""E012\0""E015\0""E016H";
const char STR_SUBTYPE_GD00X[] =      "\x05""GD_V1""GD_V2";
const char STR_SUBTYPE_REDPINE[] =    "\x04""Fast""Slow";
const char STR_SUBTYPE_POTENSIC[] =   "\x03""A20";
const char STR_SUBTYPE_ZSX[] =        "\x07""280JJRC";
const char STR_SUBTYPE_FLYZONE[] =    "\x05""FZ410";
const char STR_SUBTYPE_FX816[] =      "\x03""P38";
const char STR_SUBTYPE_XN297DUMP[] =  "\x07""250Kbps""1Mbps\0 ""2Mbps\0 ""Auto\0  ";
const char STR_SUBTYPE_ESKY150[] =    "\x03""4CH""7CH";
const char STR_SUBTYPE_ESKY150V2[] =  "\x05""150V2";
const char STR_SUBTYPE_V911S[] =      "\x05""V911S""E119\0";
const char STR_SUBTYPE_XK[] =         "\x04""X450""X420";
const char STR_SUBTYPE_FRSKYR9[] =    "\x07""915MHz\0""868MHz\0""915 8ch""868 8ch";
const char STR_SUBTYPE_ESKY[] =       "\x03""Std""ET4";
const char STR_SUBTYPE_PROPEL[] =     "\x04""74-Z";
const char STR_SUBTYPE_FRSKY_RX[] =   "\x07""RX\0    ""CloneTX";
const char STR_SUBTYPE_FRSKYL[] =     "\x08""LR12\0   ""LR12 6ch";
const char STR_SUBTYPE_WFLY[] =       "\x06""WFR0xS";

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
	OPTION_RFCHAN
};

#define NO_SUBTYPE		nullptr

const mm_protocol_definition multi_protocols[] = {
// Protocol number, Protocol String, Number of sub_protocols, Sub_protocol strings, Option type
	#if defined(ASSAN_NRF24L01_INO)
		{PROTO_ASSAN,      STR_ASSAN,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(BAYANG_NRF24L01_INO)
		{PROTO_BAYANG,     STR_BAYANG,    5, STR_SUBTYPE_BAYANG,    OPTION_TELEM   },
	#endif
	#if defined(BAYANG_RX_NRF24L01_INO)
		{PROTO_BAYANG_RX,  STR_BAYANG_RX, 0, NO_SUBTYPE,            OPTION_NONE    },	
	#endif
	#if defined(BUGS_A7105_INO)
		{PROTO_BUGS,       STR_BUGS,      0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(BUGSMINI_NRF24L01_INO)
		{PROTO_BUGSMINI,   STR_BUGSMINI,  2, STR_SUBTYPE_BUGS_MINI, OPTION_NONE    },
	#endif
	#if defined(CABELL_NRF24L01_INO)
		{PROTO_CABELL,     STR_CABELL,    8, STR_SUBTYPE_CABELL,    OPTION_OPTION  },
	#endif
	#if defined(CFLIE_NRF24L01_INO)
		{PROTO_CFLIE,      STR_CFLIE,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(E01X_NRF24L01_INO)
		{PROTO_E01X,       STR_E01X,      3, STR_SUBTYPE_E01X,      OPTION_OPTION  },
	#endif
	#if defined(CG023_NRF24L01_INO)
		{PROTO_CG023,      STR_CG023,     2, STR_SUBTYPE_CG023,     OPTION_NONE    },
	#endif
	#if defined(CORONA_CC2500_INO)
		{PROTO_CORONA,     STR_CORONA,    3, STR_SUBTYPE_CORONA,    OPTION_RFTUNE  },
	#endif
	#if defined(CX10_NRF24L01_INO)
		{PROTO_CX10,       STR_CX10,      7, STR_SUBTYPE_CX10,      OPTION_NONE    },
	#endif
	#if defined(DEVO_CYRF6936_INO)
		{PROTO_DEVO,       STR_DEVO,      5, STR_SUBTYPE_DEVO,      OPTION_FIXEDID },
	#endif
	#if defined(DM002_NRF24L01_INO)
		{PROTO_DM002,      STR_DM002,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(DSM_CYRF6936_INO)
		{PROTO_DSM,        STR_DSM,       4, STR_SUBTYPE_DSM,       OPTION_MAXTHR  },
	#endif
	#if defined(ESKY_NRF24L01_INO)
		{PROTO_ESKY,       STR_ESKY,      2, STR_SUBTYPE_ESKY,      OPTION_NONE    },
	#endif
	#if defined(ESKY150_NRF24L01_INO)
		{PROTO_ESKY150,    STR_ESKY150,   2, STR_SUBTYPE_ESKY150,   OPTION_NONE    },
	#endif
	#if defined(ESKY150V2_CC2500_INO)
		{PROTO_ESKY150V2,  STR_ESKY150V2, 1, STR_SUBTYPE_ESKY150V2, OPTION_RFTUNE  },
	#endif
	#if defined(FLYSKY_A7105_INO)
		{PROTO_FLYSKY,     STR_FLYSKY,    5, STR_SUBTYPE_FLYSKY,    OPTION_NONE    },
	#endif
	#if defined(AFHDS2A_A7105_INO)
		{PROTO_AFHDS2A,    STR_AFHDS2A,   4, STR_SUBTYPE_AFHDS2A,   OPTION_SRVFREQ },
	#endif
	#if defined(AFHDS2A_RX_A7105_INO)
		{PROTO_AFHDS2A_RX, STR_AFHDS2A_RX,0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(FLYZONE_A7105_INO)
		{PROTO_FLYZONE,    STR_FLYZONE,   1, STR_SUBTYPE_FLYZONE,   OPTION_NONE    },
	#endif
	#if defined(FQ777_NRF24L01_INO)
		{PROTO_FQ777,      STR_FQ777,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
//OpenTX 2.3.x issue: DO NOT CHANGE ORDER below
	#if defined(FRSKY_RX_CC2500_INO)
		{PROTO_FRSKY_RX,   STR_FRSKY_RX,  2, STR_SUBTYPE_FRSKY_RX,  OPTION_RFTUNE  },
	#endif
	#if defined(FRSKYD_CC2500_INO)
		{PROTO_FRSKYD,     STR_FRSKYD,    2, STR_SUBTYPE_FRSKYD,    OPTION_RFTUNE  },
	#endif
	#if defined(FRSKYV_CC2500_INO)
		{PROTO_FRSKYV,      STR_FRSKYV,   0, NO_SUBTYPE,            OPTION_RFTUNE  },
	#endif
	#if defined(FRSKYX_CC2500_INO)
		{PROTO_FRSKYX,     STR_FRSKYX,    5, STR_SUBTYPE_FRSKYX,    OPTION_RFTUNE  },
		{PROTO_FRSKYX2,    STR_FRSKYX2,   5, STR_SUBTYPE_FRSKYX,    OPTION_RFTUNE  },
	#endif
//OpenTX 2.3.x issue: DO NOT CHANGE ORDER above
	#if defined(FRSKYL_CC2500_INO)
		{PROTO_FRSKYL,     STR_FRSKYL,    2, STR_SUBTYPE_FRSKYL,    OPTION_RFTUNE  },
	#endif
	#if defined(FRSKYR9_SX1276_INO)
		{PROTO_FRSKY_R9,   STR_FRSKYR9,   4, STR_SUBTYPE_FRSKYR9,   OPTION_NONE    },
	#endif
	#if defined(FX816_NRF24L01_INO)
		{PROTO_FX816,      STR_FX816,     1, STR_SUBTYPE_FX816,     OPTION_NONE    },
	#endif
	#if defined(FY326_NRF24L01_INO)
		{PROTO_FY326,      STR_FY326,     2, STR_SUBTYPE_FY326,     OPTION_NONE    },
	#endif
	#if defined(GD00X_NRF24L01_INO)
		{PROTO_GD00X,      STR_GD00X,     2, STR_SUBTYPE_GD00X,     OPTION_RFTUNE  },
	#endif
	#if defined(GW008_NRF24L01_INO)
		{PROTO_GW008,      STR_GW008,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(H8_3D_NRF24L01_INO)
		{PROTO_H8_3D,      STR_H8_3D,     4, STR_SUBTYPE_H83D,      OPTION_NONE    },
	#endif
	#if defined(HISKY_NRF24L01_INO)
		{PROTO_HISKY,      STR_HISKY,     2, STR_SUBTYPE_HISKY,     OPTION_NONE    },
	#endif
	#if defined(HITEC_CC2500_INO)
		{PROTO_HITEC,      STR_HITEC,     3, STR_SUBTYPE_HITEC,     OPTION_RFTUNE  },
	#endif
	#if defined(HONTAI_NRF24L01_INO)
		{PROTO_HONTAI,     STR_HONTAI,    4, STR_SUBTYPE_HONTAI,    OPTION_NONE    },
	#endif
	#if defined(HOTT_CC2500_INO)
		{PROTO_HOTT,       STR_HOTT,      0, NO_SUBTYPE,            OPTION_RFTUNE  },
	#endif
	#if defined(HUBSAN_A7105_INO)
		{PROTO_HUBSAN,     STR_HUBSAN,    3, STR_SUBTYPE_HUBSAN,    OPTION_VIDFREQ },
	#endif
	#if defined(J6PRO_CYRF6936_INO)
		{PROTO_J6PRO,      STR_J6PRO,     0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(KF606_NRF24L01_INO)
		{PROTO_KF606,      STR_KF606,     0, NO_SUBTYPE,            OPTION_RFTUNE  },
	#endif
	#if defined(KN_NRF24L01_INO)
		{PROTO_KN,         STR_KN,        2, STR_SUBTYPE_KN,        OPTION_NONE    },
	#endif
	#if defined(MJXQ_NRF24L01_INO)
		{PROTO_MJXQ,       STR_MJXQ,      7, STR_SUBTYPE_MJXQ,      OPTION_RFTUNE  },
	#endif
	#if defined(MT99XX_NRF24L01_INO)
		{PROTO_MT99XX,     STR_MT99XX,    5, STR_SUBTYPE_MT99,      OPTION_NONE    },
	#endif
	#if defined(NCC1701_NRF24L01_INO)
		{PROTO_NCC1701,    STR_NCC1701,   0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(PELIKAN_A7105_INO)
		{PROTO_PELIKAN,    STR_PELIKAN  , 0, NO_SUBTYPE,            OPTION_NONE    },	
	#endif
	#if defined(POTENSIC_NRF24L01_INO)
		{PROTO_POTENSIC,   STR_POTENSIC,  1, STR_SUBTYPE_POTENSIC,  OPTION_NONE    },
	#endif
	#if defined(PROPEL_NRF24L01_INO)
		{PROTO_PROPEL,     STR_PROPEL,    4, STR_SUBTYPE_PROPEL,    OPTION_NONE    },
	#endif
	#if defined(CX10_NRF24L01_INO)
		{PROTO_Q2X2,       STR_Q2X2,      3, STR_SUBTYPE_Q2X2,      OPTION_NONE    },
	#endif
	#if defined(Q303_NRF24L01_INO)
		{PROTO_Q303,       STR_Q303,      4, STR_SUBTYPE_Q303,      OPTION_NONE    },
	#endif
	#if defined(REDPINE_CC2500_INO)
		{PROTO_REDPINE,    STR_REDPINE,   2, STR_SUBTYPE_REDPINE,   OPTION_RFTUNE  },
	#endif
	#if defined(SCANNER_CC2500_INO)
	//	{PROTO_SCANNER,    STR_SCANNER,   0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(SFHSS_CC2500_INO)
		{PROTO_SFHSS,      STR_SFHSS,     0, NO_SUBTYPE,            OPTION_RFTUNE  },
	#endif
	#if defined(SHENQI_NRF24L01_INO)
		{PROTO_SHENQI,     STR_SHENQI,    0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(SKYARTEC_CC2500_INO)
		{PROTO_SKYARTEC,   STR_SKYARTEC,  0, NO_SUBTYPE,            OPTION_RFTUNE  },
	#endif
	#if defined(SLT_NRF24L01_INO)
		{PROTO_SLT,        STR_SLT,       5, STR_SUBTYPE_SLT,       OPTION_RFTUNE  },
	#endif
	#if defined(SYMAX_NRF24L01_INO)
		{PROTO_SYMAX,      STR_SYMAX,     2, STR_SUBTYPE_SYMAX,     OPTION_NONE    },
	#endif
	#if defined(TIGER_NRF24L01_INO)
		{PROTO_TIGER,      STR_TIGER    , 0, NO_SUBTYPE,            OPTION_NONE    },	
	#endif
	#if defined(TRAXXAS_CYRF6936_INO)
		{PROTO_TRAXXAS,    STR_TRAXXAS,   1, STR_SUBTYPE_TRAXXAS,   OPTION_NONE    },
	#endif
	#if defined(V2X2_NRF24L01_INO)
		{PROTO_V2X2,       STR_V2X2,      2, STR_SUBTYPE_V2X2,      OPTION_NONE    },
	#endif
	#if defined(V761_NRF24L01_INO)
		{PROTO_V761,       STR_V761,      0, NO_SUBTYPE,            OPTION_NONE    },
	#endif
	#if defined(V911S_NRF24L01_INO)
		{PROTO_V911S,      STR_V911S,     2, STR_SUBTYPE_V911S,     OPTION_RFTUNE  },
	#endif
	#if defined(WFLY_CYRF6936_INO)
		{PROTO_WFLY,       STR_WFLY,      1, STR_SUBTYPE_WFLY,      OPTION_NONE    },
	#endif
	#if defined(WK2x01_CYRF6936_INO)
		{PROTO_WK2x01,     STR_WK2x01,    6, STR_SUBTYPE_WK2x01,    OPTION_NONE    },
	#endif
	#if defined(XK_NRF24L01_INO)
		{PROTO_XK,         STR_XK       , 2, STR_SUBTYPE_XK,        OPTION_RFTUNE  },	
	#endif
	#if defined(XN297DUMP_NRF24L01_INO)
		{PROTO_XN297DUMP,  STR_XN297DUMP, 4, STR_SUBTYPE_XN297DUMP, OPTION_RFCHAN  },
	#endif
	#if defined(YD717_NRF24L01_INO)
		{PROTO_YD717,      STR_YD717,     5, STR_SUBTYPE_YD717,     OPTION_NONE    },
	#endif
	#if defined(ZSX_NRF24L01_INO)
		{PROTO_ZSX,        STR_ZSX,       1, STR_SUBTYPE_ZSX,       OPTION_NONE    },
	#endif
		{0x00,             nullptr,       0, nullptr,               0 }
};

#endif