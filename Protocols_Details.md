# Protocols details
Here are detailed descriptions of every supported protocols (sorted by RF modules) as well as the available options for each protocol.

 If you want to see examples of model configurations see the [Models](docs/Models.md) page.
 
 The Deviation project (on which this project was based) have a useful list of models and protocols [here](http://www.deviationtx.com/wiki/supported_models).

## Useful notes and definitions
- **Extended limits supported** - A command range of -125%..+125% will be transmitted. Otherwise the default is -100%..+100% only.
- **Channel Order** - The channel order assumed in all the documentation is AETR. You can change this in the compilation settings. The module will take whatever input channel order and will rearrange them to match the output channel order required by the selected protocol. 
- **Italic numbers** are referring to protocol/sub_protocol numbers that you should use if the radio (serial mode only) is not displaying (yet) the protocol you want to access.
- **Autobind protocol**:

1. The transmitter will automatically initiate a bind sequence on power up.  This is for models where the receiver expects to rebind every time it is powered up. In these protocols you do not need to press the bind button at power up to bind, it will be done automatically. In case a protocol is not autobind but you want to enable it, change the "Autobind" (or "Bind at powerup" on OpenTX) setting to Y for the specific model/entry.
2. Enable Bind from channel feature:
   * Bind from channel can be globally enabled/disabled in _config.h using ENABLE_BIND_CH.
   * Bind from channel can be locally enabled/disabled by setting Autobind to Y/N per model for serial or per dial switch number for ppm.
   * Bind channel can be choosen on any channel between 5 and 16 using BIND_CH in _config.h. Default is 16.
   * Bind will only happen if all these elements are happening at the same time:
      - Autobind = Y
      - Throttle = LOW (<-95%)
      - Bind channel is going from -100% to +100%

* Additional notes:
  - **It's recommended to combine the bind switch with Throttle cut or throttle at -100% to drive the bind channel. This will prevent to launch a bind while flying** and enable you to use the bind switch for something else.
  - Using channel 16 for the bind channel seems the most relevant as only one protocol so far is using 16 channels which is FrSkyX. But even on FrSkyX this feature won't have any impact since there is NO valid reason to have Autobind set to Y for such a protocol.

## Protocol selection in PPM mode
The protocol selection is based on 2 parameters:
  * selection switch: this is the rotary switch on the module numbered from 0 to 15
      - switch position 0 is to select the Serial mode for er9x/erskyTX/OpenTX radio
      - switch position 15 is to select the bank
	  - switch position 1..14 will select the protocol 1..14 in the bank *X*
  * banks are used to increase the amount of accessible protocols by the switch. There are up to 5 banks giving acces to up to 70 protocol entries (5 * 14).  To modify or verify which bank is currenlty active do the following:
      - turn on the module with the switch on position 15
      - the number of LED flash indicates the bank number (1 to 5 flash)
	  - to go to the next bank, short press the bind button, this action is confirmed by the LED staying on for 1.5 sec

Here is the full protocol selection procedure:
1. turn the selection switch to 15
2. power up the module
3. the module displays the current bank by flashing the LED x number of times, x being between 1 and up to 5
4. a short press on the bind button turns the LED on for 1 sec indicating that the system has changed the bank
5. repeat operation 3 and 4 until you have reached the bank you want
6. power off
7. change the rotary switch to the desired position (1..14)
8. power on
9. enjoy

Notes:
  * **The protocol selection must be done before the module is turned on**
  * The protocol mapping based on bank + rotary switch position can be seen/modified at the end of the file [_Config.h](/Multiprotocol/_Config.h)**

## Serial mode
Serial mode is selected by placing the rotary switch to position 0 before power on of the radio.

You've upgraded the module but the radio does not display the name of the protocol you are loking for:
 * erskyTX:
      - Place the file [Multi.txt](https://raw.githubusercontent.com/pascallanger/DIY-Multiprotocol-TX-Module/master/Multiprotocol/Multi.txt) (which is part of the MPM source files) on the root of your SD card.
      - If the entry still does not appear or is broken, [upgrade](https://openrcforums.com/forum/viewtopic.php?f=7&t=4676) to version R222d2 or newer.
 * OpenTX:
      - Upgrade to the latest version of OpenTX.
      - If still not listed, use the Custom entry along with the protocol and sub_protocol values indicated by the italic numbers under each protocol. You'll find a summary of the protocols and numbers to use in table below.
 
# Available Protocol Table of Contents (Listed Alphabetically)

Protocol Name|Protocol Number|Sub_Proto 0|Sub_Proto 1|Sub_Proto 2|Sub_Proto 3|Sub_Proto 4|Sub_Proto 5|Sub_Proto 6|Sub_Proto 7|RF Module|Emulation
---|---|---|---|---|---|---|---|---|---|---|---
[Assan](Protocols_Details.md#ASSAN---24)|24|ASSAN||||||||NRF24L01|
[Bayang](Protocols_Details.md#BAYANG---14)|14|Bayang|H8S3D|X16_AH|IRDRONE|DHD_D4||||NRF24L01|XN297
[Bayang RX](Protocols_Details.md#BAYANG-RX---59)|59|||||||||NRF24L01|XN297
[Bugs](Protocols_Details.md#BUGS---41)|41|BUGS||||||||A7105|
[BugsMini](Protocols_Details.md#BUGSMINI---42)|42|BUGSMINI|BUGS3H|||||||NRF24L01|XN297
[Cabell](Protocols_Details.md#Cabell---34)|34|Cabell_V3|C_TELEM|-|-|-|-|F_SAFE|UNBIND|NRF24L01|
CFlie|38|CFlie||||||||NRF24L01|
[CG023](Protocols_Details.md#CG023---13)|13|CG023|YD829|||||||NRF24L01|XN297
[Corona](Protocols_Details.md#CORONA---37)|37|COR_V1|COR_V2|FD_V3||||||CC2500|
[CX10](Protocols_Details.md#CX10---12)|12|GREEN|BLUE|DM007|-|J3015_1|J3015_2|MK33041||NRF24L01|XN297
[Devo](Protocols_Details.md#DEVO---7)|7|Devo|8CH|10CH|12CH|6CH|7CH|||CYRF6936|
[DM002](Protocols_Details.md#DM002---33)|33|DM002||||||||NRF24L01|XN297
[DSM](Protocols_Details.md#DSM---6)|6|DSM2-22|DSM2-11|DSMX-22|DSMX-11|AUTO||||CYRF6936|
[E01X](Protocols_Details.md#E01X---45)|45|E012|E015|E016H||||||NRF24L01|XN297/HS6200
[ESky](Protocols_Details.md#ESKY---16)|16|ESky||||||||NRF24L01|
[ESky150](Protocols_Details.md#ESKY150---35)|35|ESKY150||||||||NRF24L01|
[Flysky](Protocols_Details.md#FLYSKY---1)|1|Flysky|V9x9|V6x6|V912|CX20||||A7105|
[Flysky AFHDS2A](Protocols_Details.md#FLYSKY-AFHDS2A---28)|28|PWM_IBUS|PPM_IBUS|PWM_SBUS|PPM_SBUS|||||A7105|
[Flysky AFHDS2A RX](Protocols_Details.md#FLYSKY-AFHDS2A-RX---56)|56|||||||||A7105|
[Flyzone](Protocols_Details.md#FLYZONE---53)|53|FZ410||||||||A7105|
[FQ777](Protocols_Details.md#FQ777---23)|23|FQ777||||||||NRF24L01|SSV7241
[FrskyD](Protocols_Details.md#FRSKYD---3)|3|FrskyD||||||||CC2500|
[FrskyR9](Protocols_Details.md#FRSKYR9---65)|65|FrskyR9|R9_915|R9_868||||||SX1276|
[FrskyV](Protocols_Details.md#FRSKYV---25)|25|FrskyV||||||||CC2500|
[FrskyX](Protocols_Details.md#FRSKYX---15)|15|CH_16|CH_8|EU_16|EU_8|||||CC2500|
[FrskyX_RX](Protocols_Details.md#FRSKYX_RX---55)|55|FCC|EU_LBT|||||CC2500|
[FX816](Protocols_Details.md#FX816---58)|28|FX816|P38|||||||NRF24L01|
[FY326](Protocols_Details.md#FY326---20)|20|FY326|FY319|||||||NRF24L01|
[GD00X](Protocols_Details.md#GD00X---47)|47|GD_V1*|GD_V2*|||||||NRF24L01|
[GW008](Protocols_Details.md#GW008---32)|32|GW008||||||||NRF24L01|XN297
[H8_3D](Protocols_Details.md#H8_3D---36)|36|H8_3D|H20H|H20Mini|H30Mini|||||NRF24L01|XN297
[Hisky](Protocols_Details.md#HISKY---4)|4|Hisky|HK310|||||||NRF24L01|
[Hitec](Protocols_Details.md#HITEC---39)|39|OPT_FW|OPT_HUB|MINIMA||||||CC2500|
[Hontai](Protocols_Details.md#HONTAI---26)|26|HONTAI|JJRCX1|X5C1|FQ777_951|||||NRF24L01|XN297
[HoTT](Protocols_Details.md#HoTT---57)|57|||||||||CC2500|
[Hubsan](Protocols_Details.md#HUBSAN---2)|2|H107|H301|H501||||||A7105|
[J6Pro](Protocols_Details.md#J6Pro---22)|22|J6PRO||||||||CYRF6936|
[KF606](Protocols_Details.md#KF606---49)|49|KF606*||||||||NRF24L01|XN297
[KN](Protocols_Details.md#KN---9)|9|WLTOYS|FEILUN|||||||NRF24L01|
[MJXq](Protocols_Details.md#MJXQ---18)|18|WLH08|X600|X800|H26D|E010*|H26WH|PHOENIX*||NRF24L01|XN297
[MT99xx](Protocols_Details.md#MT99XX---17)|17|MT|H7|YZ|LS|FY805||||NRF24L01|XN297
[NCC1701](Protocols_Details.md#NCC1701---44)|44|NCC1701||||||||NRF24L01|
[OpenLRS](Protocols_Details.md#OpenLRS---27)|27|||||||||None|
[Pelikan](Protocols_Details.md#Pelikan---60)|60|||||||||A7105|
[Potensic](Protocols_Details.md#Potensic---51)|51|A20||||||||NRF24L01|XN297
[Q2X2](Protocols_Details.md#Q2X2---29)|29|Q222|Q242|Q282||||||NRF24L01|
[Q303](Protocols_Details.md#Q303---31)|31|Q303|CX35|CX10D|CX10WD|||||NRF24L01|XN297
[Redpine](Protocols_Details.md#Redpine---50)|50|FAST|SLOW|||||||NRF24L01|
[Scanner](Protocols_Details.md#Scanner---54)|54|||||||||CC2500|
[SFHSS](Protocols_Details.md#SFHSS---21)|21|SFHSS||||||||CC2500|
[Shenqi](Protocols_Details.md#Shenqi---19)|19|Shenqi||||||||NRF24L01|LT8900
[SLT](Protocols_Details.md#SLT---11)|11|SLT_V1|SLT_V2|Q100|Q200|MR100||||NRF24L01|
[SymaX](Protocols_Details.md#Symax---10)|10|SYMAX|SYMAX5C|||||||NRF24L01|
[Tiger](Protocols_Details.md#Tiger---61)|61|Tiger||||||||NRF24L01|XN297
[Traxxas](Protocols_Details.md#Traxxas---43)|43|RX6519||||||||CYRF6936|
[V2x2](Protocols_Details.md#V2X2---5)|5|V2x2|JXD506|||||||NRF24L01|
[V761](Protocols_Details.md#V761---48)|48|V761||||||||NRF24L01|XN297
[V911S](Protocols_Details.md#V911S---46)|46|V911S*||||||||NRF24L01|XN297
[WFly](Protocols_Details.md#WFLY---40)|40|WFLY||||||||CYRF6936|
[WK2x01](Protocols_Details.md#WK2X01---30)|30|WK2801|WK2401|W6_5_1|W6_6_1|W6_HEL|W6_HEL_I|||CYRF6936|
[YD717](Protocols_Details.md#YD717---8)|8|YD717|SKYWLKR|SYMAX4|XINXUN|NIHUI||||NRF24L01|
[ZSX](Protocols_Details.md#ZSX---52)|52|280||||||||NRF24L01|XN297
* "*" Sub Protocols designated by * suffix are using a XN297L@250kbps which will be emulated by default with the NRF24L01. If option (freq tune) is diffrent from 0, the CC2500 module (if installed) will be used instead. Each specific sub protocol has a more detailed explanation.

# A7105 RF Module

If USE_A7105_CH15_TUNING is enabled, the value of channel 15 is used by all A7105 protocols for tuning the frequency. This is required in rare cases where some A7105 modules and/or RXs have an inaccurate crystal oscillator.

## FLYSKY - *1*
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

Note that the RX ouput will be AETR.

### Sub_protocol Flysky - *0*

### Sub_protocol V9X9 - *1*
CH5|CH6|CH7|CH8
---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO

### Sub_protocol V6X6 - *2*
CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

### Sub_protocol V912 - *3*
CH5|CH6
---|---
BTMBTN|TOPBTN

### Sub_protocol CX20 - *4*
Model: Cheerson Cx-20

CH5|CH6|CH7
---|---|---

## FLYSKY AFHDS2A - *28*
Extended limits and failsafe supported

Telemetry enabled protocol:
 - by defaut using FrSky Hub protocol (for example er9x): RX(A1), battery voltage FS-CVT01(A2) and RX&TX RSSI
 - if using erskyTX and OpenTX: full telemetry information available

Option is used to change the servo refresh rate. A value of 0 gives 50Hz (min), 70 gives 400Hz (max). Specific refresh rate value can be calculated like this option=(refresh_rate-50)/5.

**RX_Num is used to give a number a given RX. You must use a different RX_Num per RX. A maximum of 64 AFHDS2A RXs are supported.**

OpenTX suggested RSSI alarm threshold settings (Telemetry tab): Low=15, Critical=12.

If telemetry is incomplete (missing RX RSSI for example), it means that you have to upgrade your RX firmware to version 1.6 or later. You can do it from an original Flysky TX or using a STLink like explained in [this tutorial](https://www.rcgroups.com/forums/showthread.php?2677694-How-to-upgrade-Flysky-Turnigy-iA6B-RX-to-firmware-1-6-with-a-ST-Link).

AFHDS2A_LQI_CH is a feature which is disabled by defaut in the _config.h file. When enabled, it makes LQI (Link Quality Indicator) available on one of the RX ouput channel (5-14).

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14
---|---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14

Note that the RX ouput will be AETR whatever the input channel order is.

### Sub_protocol PWM_IBUS - *0*
### Sub_protocol PPM_IBUS - *1*
### Sub_protocol PWM_SBUS - *2*
### Sub_protocol PPM_SBUS - *3*

## FLYSKY AFHDS2A RX - *56*
The Flysky AFHDS2A receiver protocol enables master/slave trainning, separate access from 2 different radios to the same model,...

Available in OpenTX 2.3.3, Trainer Mode Master/Multi

Extended limits supported

Low power: enable/disable the LNA stage on the RF component to use depending on the distance with the TX.

## FLYZONE - *53*
Models using the Flyzone FZ-410 TX: Fokker D.VII Micro EP RTF

Models using the old ARES TX (prior to Hitec RED): Tiger Moth, eRC Micro Stik

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

## HUBSAN - *2*

Telemetry enabled for battery voltage and TX RSSI

Option=vTX frequency (H107D) 5645 - 5900 MHz

### Sub_protocol H107 - *0*
Autobind protocol

Models: Hubsan H102D, H107/L/C/D and H107P/C+/D+

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

### Sub_protocol H301 - *1*
Models: Hubsan H301

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|RTH|LIGHT|STAB|VIDEO

### Sub_protocol H501 - *2*
Models: Hubsan H501S, H122D, H123D

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|----|----|----|----
A|E|T|R|RTH|LIGHT|PICTURE|VIDEO|HEADLESS|GPS_HOLD|ALT_HOLD|FLIP|FMODES

H122D: FLIP

H123D: FMODES -> -100%=Sport mode 1,0%=Sport mode 2,+100%=Acro

## BUGS - *41*
Models: MJX Bugs 3, 6 and 8

Telemetry enabled for RX & TX RSSI, Battery voltage good/bad

**RX_Num is used to give a number to a given model. You must use a different RX_Num per MJX Bugs. A maximum of 16 Bugs are supported.**

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|---
A|E|T|R|ARM|ANGLE|FLIP|PICTURE|VIDEO|LED

ANGLE: angle is +100%, acro is -100%

## Pelikan - *60*
Models: TX: CADET PRO V4, RX: RX-602 V4

Extended limits supported

**Only 1 set of frequencies for now**

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

Note that the RX ouput will be AETR.

***
# CC2500 RF Module

## CORONA - *37*
Models: Corona 2.4GHz FSS and DSSS receivers.

Extended limits supported

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

### Sub_protocol COR_V1 - *0*
Corona FSS V1 RXs

### Sub_protocol COR_V2 - *1*
Corona DSSS V2 RXs: CR8D, CR6D and CR4D

To bind V2 RXs you must follow the below procedure (original):
 - press the bind button and power on the RX
 - launch a bind from Multi -> the RX will blink 2 times
 - turn off the RX **and** TX(=Multi)
 - turn on the RX **first**
 - turn on the TX(=Multi) **second**
 - wait for the bind to complete -> the RX will flash, stop and finally fix
 - wait some time (more than 30 sec) before turning off the RX
 - turn off/on the RX and test that it can reconnect instantly, if not repeat the bind procedure

### Sub_protocol FD_V3 - *2*
FlyDream RXs like IS-4R and IS-4R0

## FRSKYV - *25*
Models: FrSky receivers V8R4, V8R7 and V8FR.
 - FrSkyV = FrSky 1 way

Extended limits supported

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.
 
CH1|CH2|CH3|CH4
---|---|---|---
CH1|CH2|CH3|CH4

## FRSKYD - *3*
Models: FrSky receivers D4R and D8R. DIY RX-F801 and RX-F802 receivers. Also known as D8.

Extended limits supported

Telemetry enabled for A0, A1, RSSI, TX_RSSI, TX_LQI and Hub. Lowest the TX_LQI value is best the quality link is, it's a good indicator of how well the module is tuned.

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

## FRSKYX - *15*
Models: FrSky receivers X4R, X6R and X8R. Also known as D16.

Extended limits and failsafe supported

Telemetry enabled for A1 (RxBatt), A2, RSSI, TX_RSSI, TX_LQI and Hub. Lowest the TX_LQI value is best the quality link is, it's a good indicator of how well the module is tuned.

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

### Sub_protocol CH_16 - *0*
FCC protocol 16 channels @18ms.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

### Sub_protocol CH_8 - *1*
FCC protocol 8 channels @9ms.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

### Sub_protocol EU_16 - *2*
EU-LBT protocol 16 channels @18ms. Note that the LBT part is not implemented, the TX transmits right away.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

### Sub_protocol EU_8 - *3*
EU-LBT protocol 8 channels @9ms. Note that the LBT part is not implemented, the TX transmits right away.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

## FRSKYX_RX - *55*
The FrSkyX receiver protocol enables master/slave trainning, separate access from 2 different radios to the same model,...

Available in OpenTX 2.3.3, Trainer Mode Master/Multi

Extended limits supported

Option for this protocol corresponds to fine frequency tuning.
If the value is equal to 0, the RX will auto tune otherwise it will use the indicated value.
This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

Low power: enable/disable the LNA stage on the RF component to use depending on the distance with the TX.

### Sub_protocol FCC - *0*
FCC protocol 8 or 16 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

### Sub_protocol EU_LBT - *1*
EU_LBT protocol 8 or 16 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

## HITEC - *39*
Models: OPTIMA, MINIMA and MICRO receivers.

Extended limits supported

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9

### Sub_protocol OPT_FW - *0*
OPTIMA RXs

Full telemetry available on OpenTX 2.3.3+, still in progress for erskyTx. Lowest the TX_LQI value is best the quality link is, it's a good indicator of how well the module is tuned.

**The TX must be close to the RX for the bind negotiation to complete successfully**

### Sub_protocol OPT_HUB - *1*
OPTIMA RXs

Basic telemetry using FrSky Hub on er9x, erskyTX, OpenTX and any radio with FrSky telemetry support with RX voltage, VOLT2 voltage, TX_RSSI and TX_LQI. Lowest the TX_LQI value is best the quality link is, it's a good indicator of how well the module is tuned.

**The TX must be close to the RX for the bind negotiation to complete successfully**

### Sub_protocol MINIMA - *2*
MINIMA, MICRO and RED receivers

## HoTT - *57*
Models: Graupner HoTT receivers (tested on GR-12L and GR-16L).

Extended limits  and failsafe supported

**RX_Num is used to give a number a given RX. You must use a different RX_Num per RX. A maximum of 64 HoTT RXs are supported.**

**Failsafe MUST be configured once with the desired channel values (hold or position) while the RX is up (wait 10+sec for the RX to learn the config) and then failsafe MUST be set to RX/Receiver otherwise the servos will jitter!!!**

The RX features configuration are done using the OpenTX script "Graupner HoTT.lua" .

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

Basic telemetry is available on OpenTX 2.3.3+ with RX voltage, Rx temperature, RX RSSI, RX LQI, TX RSSI and TX LQI. Lowest the TX_LQI value is best the quality link is, it's a good indicator of how well the module is tuned.

## SFHSS - *21*
Models: Futaba RXs and XK models.

Extended limits and failsafe supported

Option for this protocol corresponds to fine frequency tuning. This value is different for each Module and **must** be accurate otherwise the link will not be stable.
Check the [Frequency Tuning page](/docs/Frequency_Tuning.md) to determine it.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

## Scanner - *54*
2.4GHz scanner accessible using the OpenTX 2.3 Spectrum Analyser tool.

***
# CYRF6936 RF Module

If USE_CYRF6936_CH15_TUNING is enabled, the value of channel 15 is used by all CYRF6936 protocols for tuning the frequency. This is required in rare cases where some CYRF6936 modules and/or RXs have an inaccurate crystal oscillator.

## DEVO - *7*
Extended limits and failsafe supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

Note that the RX ouput will be EATR.

Bind procedure using serial:
- With the TX off, put the binding plug in and power on the RX (RX LED slow blink), then power it down and remove the binding plug. Receiver should now be in autobind mode.
- Turn on the TX, set protocol = Devo with option=0, turn off the TX (TX is now in autobind mode).
- Turn on RX (RX LED fast blink).
- Turn on TX (RX LED solid, TX LED fast blink).
- Wait for bind on the TX to complete (TX LED solid).
- Make sure to set a uniq RX_Num value for model match.
- Change option to 1 to use the global ID.
- Do not touch option/RX_Num anymore.

Bind procedure using PPM:
- With the TX off, put the binding plug in and power on the RX (RX LED slow blink), then power it down and remove the binding plug. Receiver should now be in autobind mode.
- Turn on RX (RX LED fast blink).
- Turn the dial to the model number running protocol DEVO on the module.
- Press the bind button and turn on the TX. TX is now in autobind mode.
- Release bind button after 1 second: RX LED solid, TX LED fast blink.
- Wait for bind on the TX to complete (TX LED solid).
- Press the bind button for 1 second. TX/RX is now in fixed ID mode.
- To verify that the TX is in fixed mode: power cycle the TX, the module LED should be solid ON (no blink).
- Note: Autobind/fixed ID mode is linked to the RX_Num number. Which means that you can have multiple dial numbers set to the same protocol DEVO with different RX_Num and have different bind modes at the same time. It enables PPM users to get model match under DEVO.

### Sub_protocol 8CH - *0*
### Sub_protocol 10CH - *1*
### Sub_protocol 12CH - *2*
### Sub_protocol 6CH - *3*
### Sub_protocol 7CH - *4*

## WK2X01 - *30*
Extended limits supported
Autobind protocol

Note: RX ouput will always be AETR independently of the input AETR, RETA...

### Sub_protocol WK2801 - *0*
Failsafe supported.

This roughly corresponds to the number of channels supported, but many of the newer 6-channel receivers actually support the WK2801 protocol. It is recommended to try the WK2801 protocol 1st when working with older Walkera models before attempting the WK2601 or WK2401 mode, as the WK2801 is a superior protocol. The WK2801 protocol supports up to 8 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

Bind procedure using serial:
- With the TX off, put the binding plug in and power on the RX (RX LED slow blink), then power it down and remove the binding plug. Receiver should now be in autobind mode.
- Turn on the TX, set protocol = WK2X01, sub_protocol = WK2801 with option=0, turn off the TX (TX is now in autobind mode).
- Turn on RX (RX LED fast blink).
- Turn on TX (RX LED solid, TX LED fast blink).
- Wait for bind on the TX to complete (TX LED solid).
- Make sure to set a uniq RX_Num value for model match.
- Change option to 1 to use the global ID.
- Do not touch option/RX_Num anymore.

Bind procedure using PPM:
- With the TX off, put the binding plug in and power on the RX (RX LED slow blink), then power it down and remove the binding plug. Receiver should now be in autobind mode.
- Turn on RX (RX LED fast blink).
- Turn the dial to the model number running protocol protocol WK2X01 and sub_protocol WK2801 on the module.
- Press the bind button and turn on the TX. TX is now in autobind mode.
- Release bind button after 1 second: RX LED solid, TX LED fast blink.
- Wait for bind on the TX to complete (TX LED solid).
- Press the bind button for 1 second. TX/RX is now in fixed ID mode.
- To verify that the TX is in fixed mode: power cycle the TX, the module LED should be solid ON (no blink).
- Note: Autobind/fixed ID mode is linked to the RX_Num number. Which means that you can have multiple dial numbers set to the same protocol DEVO with different RX_Num and have different bind modes at the same time. It enables PPM users to get model match under DEVO.

### Sub_protocol WK2401 - *1*
The WK2401 protocol is used to control older Walkera models.

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

### Sub_protocol W6_5_1 - *2*
WK2601 5+1: AIL, ELE, THR, RUD, GYRO (ch 7) are proportional. Gear (ch 5) is binary. Ch 6 is disabled

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
A|E|T|R|GEAR|DIS|GYRO

### Sub_protocol W6_6_1 - *3*
WK2601 6+1: AIL, ELE, THR, RUD, COL (ch 6), GYRO (ch 7) are proportional. Gear (ch 5) is binary. **This mode is highly experimental.**

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
A|E|T|R|GEAR|COL|GYRO

### Sub_protocol W6_HEL - *4* and W6HEL_I - *5*
WK2601 Heli: AIL, ELE, THR, RUD, GYRO are proportional. Gear (ch 5) is binary. COL (ch 6) is linked to Thr. If Ch6 >= 0, the receiver will apply a 3D curve to the Thr. If Ch6 < 0, the receiver will apply normal curves to the Thr. The value of Ch6 defines the ratio of COL to THR.

W6HEL_I: Invert COL servo

option= maximum range of COL servo

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
A|E|T|R|GEAR|COL|GYRO

## DSM - *6*
Extended limits supported

Telemetry enabled for TSSI and plugins

option=number of channels from 4 to 12. An invalid option value will end up with 6 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|----|CH14
---|---|---|---|---|---|---|---|---|----|----|----|----|----
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|----|TH_KILL

Notes:
 - model/type/number of channels indicated on the RX can be different from what the RX is in fact wanting to see. So don't hesitate to test different combinations until you have something working. Using Auto is the best way to find these settings.
 - RX output will match the Spektrum standard TAER independently of the input configuration AETR, RETA... unless on OpenTX 2.3.3+ you use the "Disable channel mapping" feature on the GUI.
 - RX output will match the Spektrum standard throw (1500µs +/- 400µs -> 1100..1900µs) for a 100% input. This is true for both Serial and PPM input. For PPM, make sure the end points PPM_MIN_100 and PPM_MAX_100 in _config.h are matching your TX ouput. The maximum ouput is 1000..2000µs based on an input of 125%.
    - If you want to override the above and get maximum throw either uncomment in _config.h the line #define DSM_MAX_THROW or on OpenTX 2.3.3+ use the "Enable max throw" feature on the GUI (0=No,1=Yes). In this mode to achieve standard throw use a channel weight of 84%.
 - TH_KILL is a feature which is enabled on channel 14 by default (can be disabled/changed) in the _config.h file. Some models (X-Vert, Blade 230S...) require a special position to instant stop the motor(s). If the channel 14 is above -50% the throttle is untouched but if it is between -50% and -100%, the throttle output will be forced between -100% and -150%. For example, a value of -80% applied on channel 14 will instantly kill the motors on the X-Vert.

### Sub_protocol DSM2_22 - *0*
DSM2, Resolution 1024, refresh rate 22ms
### Sub_protocol DSM2_11 - *1*
DSM2, Resolution 2048, refresh rate 11ms
### Sub_protocol DSMX_22 - *2*
DSMX, Resolution 2048, refresh rate 22ms
### Sub_protocol DSMX_11 - *3*
DSMX, Resolution 2048, refresh rate 11ms
### Sub_protocol AUTO - *4*
The "AUTO" feature enables the TX to automatically choose what are the best settings for your DSM RX and update your model protocol settings accordingly.

The current radio firmware which are able to use the "AUTO" feature are erskyTX (9XR Pro, 9Xtreme, Taranis, ...), er9x for M128(9XR)&M2561 and OpenTX (mostly Taranis).
For these firmwares, you must have a telemetry enabled TX and you have to make sure you set the Telemetry "Usr proto" to "DSMx".
Also on er9x you will need to be sure to match the polarity of the telemetry serial (normal or inverted by bitbashing), while on erskyTX you can set "Invert COM1" accordinlgy.

## J6Pro - *22*

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|----|----|----
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

## Traxxas - *43*
Receiver 6519

Extended limits supported

CH1|CH2|CH3|CH4
---|---|---|---
AUX3|AUX4|THROTTLE|STEERING

## WFLY - *40*
Receivers: WFR04S, WFR07S, WFR09S

Extended limits supported

option=number of channels from 4 to 9. An invalid option value will end up sending 9 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9

***
# NRF24L01 RF Module

## ASSAN - *24*
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

The transmitter must be close to the receiver while binding.

## BAYANG - *14*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|-|-|CH14|CH15
---|---|---|---|---|---|---|---|---|----|----|---|---|----|----
A|E|T|R|FLIP|RTH|PICTURE|VIDEO|HEADLESS|INVERTED|RATES|-|-|ANAAUX1|ANAAUX2

RATES: -100%(default)=>higher rates by enabling dynamic trims (except for Headless), 100%=>disable dynamic trims

Channels 14 and 15 (ANAAUX1 and ANAAUX2) only available with analog aux channel option, indicated below.

### Sub_protocol BAYANG - *0*
Models: Eachine H8(C) mini, BayangToys X6/X7/X9, JJRC JJ850, Floureon H101 ...

Option=0 -> normal Bayang protocol

Option=1 -> enable telemetry with [Silverxxx firmware](https://github.com/silver13/H101-acro/tree/master). Value returned to the TX using FrSkyD Hub are RX RSSI, TX RSSI, A1=uncompensated battery voltage, A2=compensated battery voltage

Option=2 -> enable analog aux channels with [NFE Silverware firmware](https://github.com/NotFastEnuf/NFE_Silverware). Two otherwise static bytes in the protocol overridden to add two 'analog' (non-binary) auxiliary channels.

Option=3 -> both Silverware telemetry and analog aux channels enabled.

### Sub_protocol H8S3D - *1*
Model: H8S 3D

Same channels assignment as above.

### Sub_protocol X16_AH - *2*
Model: X16 AH

CH12|
----|
TAKE_OFF|

### Sub_protocol IRDRONE - *3*
Model: IRDRONE

CH12|CH13
----|----
TAKE_OFF|EMG_STOP

### Sub_protocol DHD_D4 - *4*
Model: DHD D4

CH12|CH13
----|----
TAKE_OFF|EMG_STOP

## BAYANG RX - *59*
The Bayang receiver protocol enables master/slave trainning, separate access from 2 different radios to the same model,...

See the [BAYANG protocol](Protocols_Details.md#BAYANG---14) on how to activate ANAUX1 and ANAUX2 (Option/Telemetry=2).

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|---
A|E|T|R|ANAUX1|ANAUX2|FLIP|RTH|PICTURE|VIDEO

Available in OpenTX 2.3.3, Trainer Mode Master/Multi

## BUGSMINI - *42*
Models: MJX Bugs 3 Mini and 3H

Telemetry enabled for RX RSSI, Battery voltage good/warning/bad

**RX_Num is used to give a number to a given model. You must use a different RX_Num per MJX Bugs Mini. A maximum of 16 Bugs Mini are supported.**

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|---
A|E|T|R|ARM|ANGLE|FLIP|PICTURE|VIDEO|LED

ANGLE: angle is +100%, acro is -100%

### Sub_protocol BUGSMINI - *0*

### Sub_protocol BUGS3H - *1*
CH11|
---|
ALTHOLD|

## Cabell - *34*
Homegrown protocol with variable number of channels (4-16) and telemetry (RSSI, V1, V2).

It is a FHSS protocol developed by Dennis Cabell (KE8FZX) using the NRF24L01+ 2.4 GHz transceiver. 45 channels are used frequency hop from 2.403 through 2.447 GHz. The reason for using 45 channels is to keep operation within the overlap area between the 2.4 GHz ISM band (governed in the USA by FCC part 15) and the HAM portion of the band (governed in the USA by FCC part 97). This allows part 15 compliant use of the protocol, while allowing licensed amateur radio operators to operate under the less restrictive part 97 rules if desired.

Additional details about configuring and using the protocol are available at the RX project at: https://github.com/soligen2010/RC_RX_CABELL_V3_FHSS

CH1|CH2|CH3|CH4|CH5 |CH6 |CH7 |CH8 |CH9 |CH10|CH11|CH12|CH13|CH14 |CH15 |CH16
---|---|---|---|----|----|----|----|----|----|----|----|----|-----|-----|-----
 A | E | T | R |AUX1|AUX2|AUX3|AUX4|AUX5|AUX6|AUX7|AUX8|AUX9|AUX10|AUX11|AUX12

### Sub_protocol CABELL_V3 - *0*
4 to 16 channels without telemetry

### Sub_protocol CABELL_V3_TELEMETRY - *1*
4 to 16 channels with telemetry (RSSI, V1, V2). V1 & V2 can be used to return any analog voltage between 0 and 5 volts, so can be used for battery voltage or any other sensor that provides an analog voltage.

### Sub_protocol CABELL_SET_FAIL_SAFE - *6*
Stores failsafe values in the RX.  The channel values are set when the sub-protocol is changed to 6, so hold sticks in place as the sub-protocol is changed.

### Sub_protocol CABELL_UNBIND - *7*
The receiver bound to the model is un-bound.  This happens immediately when the sub-protocol is set to 7.

## CG023 - *13*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

### Sub_protocol CG023 - *0*
Models: EAchine CG023/CG031/3D X4

### Sub_protocol YD829 - *1*
Models: Attop YD-822/YD-829/YD-829C ...

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP||PICTURE|VIDEO|HEADLESS

## CX10 - *12*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|FLIP|RATE

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3

### Sub_protocol GREEN - *0*
Models: Cheerson CX-10 green pcb

Same channels assignement as above.

### Sub_protocol BLUE - *1*
Models: Cheerson CX-10 blue pcb & some newer red pcb, CX-10A, CX-10C, CX11, CX12, Floureon FX10, JJRC DHD D1

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|RATE|PICTURE|VIDEO

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3 or headless for CX-10A

### Sub_protocol DM007 - *2*

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS

### Sub_protocol JC3015_1 - *4*

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|PICTURE|VIDEO

### Sub_protocol JC3015_2 - *5*

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|LED|DFLIP

### Sub_protocol MK33041 - *6*

CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS|RTH

## DM002 - *33*
Autobind protocol

**Only 3 TX IDs available, change RX_Num value 0-1-2 to cycle through them**

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LED|CAMERA1|CAMERA2|HEADLESS|RTH|RATE_LOW

## E01X - *45*
Autobind protocol

### Sub_protocol E012 - *0*
Models: Eachine E012

This protocol has been reported to not work properly due to the emulation of the HS6200 RF component using the NRF24L01. The option value is used to adjust the timing, try every values between -127 and +127. If it works please report which value you've used.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R||FLIP||HEADLESS|RTH

### Sub_protocol E015 - *1*
Models: Eachine E015

This protocol has been reported to not work properly due to the emulation of the HS6200 RF component using the NRF24L01. The option value is used to adjust the timing, try every values between -127 and +127. If it works please report which value you've used.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|ARM|FLIP|LED|HEADLESS|RTH

### Sub_protocol E016H - *2*
Models: Eachine E016H

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|STOP|FLIP|-|HEADLESS|RTH

## ESKY - *16*

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GYRO|PITCH

## ESKY150 - *35*
ESky protocol for small models since 2014 (150, 300, 150X, ...)

### Sub_protocol 4CH - *0*

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

### Sub_protocol 7CH - *1*

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
A|E|T|R|FMODE|AUX6|AUX7

FMODE and AUX7 have 4 positions: -100%..-50%=>0, -50%..5%=>1, 5%..50%=>2, 50%..100%=>3

## FX816 - *58*
Model: FEI XIONG FX816 P38

Only 8 TX IDs available

CH1|CH2|CH3|CH4
---|---|---|---
A|-|T|-

## FY326 - *20*

### Sub_protocol FY326 - *0*
Model: FY326 Q7 Quadcopter

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RTH|HEADLESS|EXPERT|CALIBRATE

### Sub_protocol FY319 - *1*
Model: X6 FY319 Quadcopter (Needs Testing)

## FQ777 - *23*
Model: FQ777-124 (with SV7241A)

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RTH|HEADLESS|EXPERT

## GD00X - *47*
Model: GD005 C-17 Transport, GD006 DA62 and ZC-Z50

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
A||T||TRIM|LED|RATE

TRIM: either use this channel for trim only or add a mixer with aileron to increase the roll rate.

RATE: -100% high rate, +100% low rate

### Sub_protocol GD_V1 - *0*
First generation of GD models, ZC-Z50

### Sub_protocol GD_V2 - *1*
New generation of GD models

## GW008 - *32*
Model: Global Drone GW008 from Banggood

There are 3 versions of this small quad, this protocol is for the one with a XNS104 IC in the stock Tx and PAN159CY IC in the quad. The xn297 version is compatible with the CX10 protocol (green pcb). The LT8910 version is not supported yet.

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
A|E|T|R|FLIP

## H8_3D - *36*
Autobind protocol

### Sub_protocol H8_3D - *0*
Models: EAchine H8 mini 3D, JJRC H20/H22/H11D

CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---
FLIP|LIGTH|PICTURE|VIDEO|OPT1|OPT2|CAL1|CAL2|GIMBAL

JJRC H20: OPT1=Headless, OPT2=RTH

JJRC H22: OPT1=RTH, OPT2=180/360° flip mode

H8 3D: OPT1=RTH then press a direction to enter headless mode (like stock TX), OPT2=switch 180/360° flip mode

CAL1: H8 3D acc calib, H20/H20H headless calib
CAL2: H11D/H20/H20H acc calib

### Sub_protocol H20H - *1*
CH6=Motors on/off

### Sub_protocol H20 Mini - *2*
**Only 3 TX IDs available, change RX_Num value 0-1-2 to cycle through them**

### Sub_protocol H30 Mini - *3*
**Only 4 TX IDs available, change RX_Num value 0-1-2_3 to cycle through them**

## HISKY - *4*
### Sub_protocol Hisky - *0*
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|GEAR|PITCH|GYRO|CH8

GYRO: -100%=6G, +100%=3G

### Sub_protocol HK310 - *1*
Models: RX HK-3000, HK3100 and XY3000 (TX are HK-300, HK-310 and TL-3C)

Failsafe supported

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
| | |T|R|AUX

## KN - *9*
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|DR|THOLD|IDLEUP|GYRO|Ttrim|Atrim|Etrim

Dual Rate: +100%=full range, Throttle Hold: +100%=hold, Idle Up: +100%=3D, GYRO: -100%=6G, +100%=3G

### Sub_protocol WLTOYS - *0*
### Sub_protocol FEILUN - *1*
Same channels assignement as above.

## HONTAI - *26*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|CAL

### Sub_protocol HONTAI - *0*
### Sub_protocol JJRCX1 - *1*
CH6|
---|
ARM|

### Sub_protocol X5C1 clone - *2*

### Sub_protocol FQ777_951 - *3*

## KF606 - *49*
Model: KF606

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
A||T||TRIM

## MJXQ - *18*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14
---|---|---|---|---|---|---|---|---|----|----|----|----|----
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|AUTOFLIP|PAN|TILT|RATE

RATE: -100%(default)=>higher rates by enabling dynamic trims (except for Headless), 100%=>disable dynamic trims

### Sub_protocol WLH08 - *0*
### Sub_protocol X600 - *1*
Only 3 TX IDs available, change RX_Num value 0..2 to cycle through them
### Sub_protocol X800 - *2*
Only 3 TX IDs available, change RX_Num value 0..2 to cycle through them
### Sub_protocol H26D - *3*
Only 3 TX IDs available, change RX_Num value 0..2 to cycle through them
### Sub_protocol E010 - *4*
15 TX IDs available, change RX_Num value 0..14 to cycle through them

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

### Sub_protocol H26WH - *5*
CH6|
---|
ARM|

Only 1 TX ID available

### Sub_protocol PHOENIX - *6*
CH6|
---|
ARM|

## MT99XX - *17*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS

### Sub_protocol MT99 - *0*
Models: MT99xx
### Sub_protocol H7 - *1*
Models: Eachine H7, Cheerson CX023
### Sub_protocol YZ - *2*
Model: Yi Zhan i6S

Only one model can be flown at the same time since the ID is hardcoded.

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

### Sub_protocol LS - *3*
Models: LS114, 124, 215

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|INVERT|PICTURE|VIDEO|HEADLESS

### Sub_protocol FY805 - *4*
Model: FY805

Only 1 ID available

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP||||HEADLESS

## NCC1701 - *44*
Model: Air Hogs Star Trek USS Enterprise NCC-1701-A

Autobind protocol

Telemetry: RSSI is a dummy value. A1 voltage is dummy but used for crash detection. In case of a crash event A1>0V, you can assign a sound to be played on the TX in that case (siren on the original transmitter).

Only 9 IDs available, cycle through them using RX_Num.

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
A|E|T|R|Warp

## Potensic - *51*
Models: Potensic A20

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|TAKE_OFF/LANDING|EMERGENCY|MODE|HEADLESS

TAKE_OFF/LANDING: momentary switch -100% -> +100%

EMERGENCY: Stop +100%

MODE: Beginner -100%, Medium 0%, Advanced +100%

HEADLESS: Off -100%, On +100%

## Q2X2 - *29*
### Sub_protocol Q222 - *0*
Models: Q222 v1 and V686 v2

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|MODULE2|MODULE1|HEADLESS|RTH|XCAL|YCAL

### Sub_protocol Q242 - *1* and Q282 - *2*

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

Model: JXD 509 is using Q282 with CH12=Start/Stop motors

## Q303 - *31*
Autobind protocol

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

### Sub_protocol Q303 - *0*

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---
AHOLD|FLIP|PICTURE|VIDEO|HEADLESS|RTH|GIMBAL

GIMBAL needs 3 position -100%/0%/100%

### Sub_protocol CX35 - *1*
CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---
ARM|VTX|PICTURE|VIDEO||RTH|GIMBAL

ARM is 2 positions: land / take off

Each toggle of VTX will increment the channel.

Gimbal is full range.

### Sub_protocol CX10D  - *2* and Sub_protocol CX10WD - *3*
CH5|CH6
---|---
ARM|FLIP

ARM is 3 positions: -100%=land / 0%=manual / +100%=take off

## Redpine - *50*
[Link to the forum](https://www.rcgroups.com/forums/showthread.php?3236043-Redpine-Lowest-latency-RC-protocol)

### Sub_protocol FAST - *0*
### Sub_protocol SLOW - *1*

## Shenqi - *19*
Autobind protocol

Model: Shenqiwei 1/20 Mini Motorcycle

CH1|CH2|CH3|CH4
---|---|---|---
 | |T|R

Throttle +100%=full forward,0%=stop,-100%=full backward.

## SLT - *11*
Autobind protocol

### Sub_protocol V1 - *0*

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GEAR|PITCH

### Sub_protocol V2 - *1*

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

### Sub_protocol Q100 - *2*
Models: Dromida Ominus UAV

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|RATES|-|CH7|CH8|MODE|FLIP|-|-|CALIB

RATES takes any value between -50..+50%: -50%=min rates, 0%=mid rates (stock setting), +50%=max rates

CH7 and CH8 have no visible effect

MODE: -100% level, +100% acro

FLIP: sets model into flip mode for approx 5 seconds at each throw of switch (rear red LED goes out while active) -100%..+100% or +100%..-100%

CALIB: -100% normal mode, +100% gyro calibration

### Sub_protocol Q200 - *3*
Model: Dromida Ominus Quadcopter FPV, the Nine Eagles - FENG FPV and may be others

Dromida Ominus FPV channels mapping:

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|RATES|-|CH7|CH8|MODE|FLIP|VID_ON|VID_OFF|CALIB

FENG FPV: channels mapping:

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|RATES|-|CH7|CH8|FLIP|MODE|VID_ON|VID_OFF|CALIB

RATES takes any value between -50..+50%: -50%=min rates, 0%=mid rates (stock setting), +50%=max rates

CH7 and CH8 have no visible effect

MODE: -100% level, +100% acro

FLIP: sets model into flip mode for approx 5 seconds at each throw of switch (rear red LED goes out while active) -100%..+100% or +100%..-100%

CALIB: -100% normal mode, +100% gyro calibration

### Sub_protocol MR100 - *4*
Models: Vista UAV, FPV, FPV v2

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|RATES|-|CH7|CH8|MODE|FLIP|VIDEO|PICTURE

RATES takes any value between -50..+50%: -50%=min rates, 0%=mid rates (stock setting), +50%=max rates

CH7 and CH8 have no visible effect

FLIP: sets model into flip mode for approx 5 seconds at each throw of switch (rear red LED goes out while active) -100%..+100% or +100%..-100%

MODE: -100% level, +100% acro

## Symax - *10*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RATES|PICTURE|VIDEO|HEADLESS

RATES: -100%(default)=>disable dynamic trims, +100%=> higher rates by enabling dynamic trims (except for Headless)

### Sub_protocol SYMAX - *0*
Models: Syma X5C-1/X11/X11C/X12

### Sub_protocol SYMAX5C - *1*
Model: Syma X5C (original) and X2

## V2X2 - *5*
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|MAG_CAL_X|MAG_CAL_Y

### Sub_protocol V2x2 - *0*
Models: WLToys V202/252/272, JXD 385/388, JJRC H6C, Yizhan Tarantula X6 ...

PICTURE: also automatic Missile Launcher and Hoist in one direction

VIDEO: also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir

### Sub_protocol JXD506 - *1*
Model: JXD 506

CH10|CH11|CH12
---|---|---
Start/Stop|EMERGENCY|CAMERA_UP/DN

## Tiger - *61*
Autobind protocol

**Only 1 ID**

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT

## V761 - *48*
Model: Volantex V761 and may be other

Warning: Only 3 IDs, you can cycle through them using RX_Num.

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
-|E|T|R|GYRO

Gyro: -100%=Beginer mode (Gyro on, yaw and pitch rate limited), 0%=Mid Mode ( Gyro on no rate limits), +100%=Mode Expert Gyro off

## V911S - *46*
Models: WLtoys V911S, XK A110

This protocol is known to be problematic because it's using the xn297L emulation with a transmission speed of 250kbps therefore it doesn't work very well with every modules, this is an hardware issue with the accuracy of the components.

If the model does not respond well to inputs or hard to bind, you can try to switch the emulation from the default NRF24L01 RF component to the CC2500 by using an option value (freq tuning) different from 0. Option in this case is used for fine frequency tuning like any CC2500 protocols so check the [Frequency Tuning page](/docs/Frequency_Tuning.md).

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
A|E|T|R|CALIB

## YD717 - *8*
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

### Sub_protocol YD717 - *0*
### Sub_protocol SKYWLKR - *1*
### Sub_protocol SYMAX4 - *2*
### Sub_protocol XINXUN - *3*
### Sub_protocol NIHUI - *4*
Same channels assignement as above.

## ZSX - *52*
Model: JJRC ZSX-280

Autobind protocol

CH1|CH2|CH3|CH4|CH5
---|---|---|---|---
||T|R|LIGHT

# SX1276 RF Module

## FRSKYR9 - *65*
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

### Sub_protocol R9_915 - *0*
915MHz

### Sub_protocol R9_868 - *1*
868MHz

# OpenLRS module

## OpenLRS - *27*
This is a reservation for OpenLRSng which is using Multi's serial protocol for their modules: https://openlrsng.org/. On the Multi side there is no protocol affected on 27 so it's just ignored.

