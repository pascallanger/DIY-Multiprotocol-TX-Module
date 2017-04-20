# MULTI-Module Protocol Details
**You'll find below a detailed description of every supported protocols sorted by RF modules.**

Legend:
- Extended limits supported: -125%..+125% can be used and will be transmitted. Otherwise the default is -100%..+100% only.
- Autobind protocol: you do not need to press the bind button at power up to bind, this is done automatically.

The AETR mentionned here for all protocols depends on the TX settings compilation option set in _Config.h.

***
# A7105 RF Module

## FLYSKY
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

Note that the RX ouput will be AETR.

### Sub_protocol V9X9
CH5|CH6|CH7|CH8
---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO

### Sub_protocol V6X6
CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

### Sub_protocol V912
CH5|CH6
---|---
BTMBTN|TOPBTN

## HUBSAN
Models: Hubsan H102D, H107/L/C/D and Hubsan H107P/C+/D+

Autobind protocol

Telemetry enabled for battery voltage and TX RSSI

Option=vTX frequency (H107D) 5645 - 5900 MHz

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

# CC2500 RF Module

## FRSKYV = FrSky 1 way
Models: FrSky receivers V8R4, V8R7 and V8FR.

Extended limits supported

Option=fine frequency tuning. This value is different for each board. To determine the option value, find the two limits where the RX loses connection then set the option value to half way between them. If you have a 4in1 V2 board the value is around 40.

CH1|CH2|CH3|CH4
---|---|---|---
CH1|CH2|CH3|CH4

## FRSKYD
Models: FrSky receivers D4R and D8R. DIY RX-F801 and RX-F802 receivers.

Extended limits supported

Telemetry enabled for A0, A1, RSSI, TSSI and Hub

Option=fine frequency tuning. This value is different for each board. To determine the option value, find the two limits where the RX loses connection then set the option value to half way between them. If you have a 4in1 V2 board the value is around 40.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

## FRSKYX
Models: FrSky receivers X4R, X6R and X8R.

Extended limits supported

Telemetry enabled for A1 (RxBatt), A2, RSSI, TSSI and Hub

Option=fine frequency tuning. This value is different for each board. To determine the option value, find the two limits where the RX loses connection then set the option value to half way between them. If you have a 4in1 V2 board the value is around 40.

### Sub_protocol CH_16
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

### Sub_protocol CH_8
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

## SFHSS
Models: Futaba RXs and XK models.

Option=fine frequency tuning. This value is different for each board. To determine the option value, find the two limits where the RX loses connection then set the option value to half way between them. If you have a 4in1 V2 board the value is around 40.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

# CYRF6936 RF Module

## DEVO
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

Note that the RX ouput will be EATR.

Bind procedure using serial:
- With the TX off, put the binding plug in and power on the RX (RX LED slow blink), then power it down and remove the binding plug. Receiver should now be in autobind mode.
- Turn on the TX, set protocol = Devo with option=0, turn off the TX (TX is now in autobind mode).
- Turn on RX (RX LED fast blink).
- Turn on TX (RX LED solid, TX LED fast blink).
- Wait for bind on the TX to complete (TX LED solid).
- Make sure to set the RX_Num value for model match.
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
- Note: Autobind/fixed ID mode is linked to the dial number. Which means that you can have multiple dial numbers set to the same protocol DEVO with different RX_Num and have different bind modes at the same time. It enables PPM users to get model match under DEVO.

## DSM
Extended limits supported

Telemetry enabled for TSSI and plugins

option=number of channels from 4 to 12. An invalid option value will end up with 6 channels.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|----|----|----
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

Notes:
 - model/type/number of channels indicated on the RX can be different from what the RX is in fact wanting to see. So don't hesitate to test different combinations until you have something working. Using Auto is the best way to find these settings.
 - RX ouput will always be TAER independently of the input AETR, RETA...

### Sub_protocol DSM2_22
DSM2, Resolution 1024, refresh rate 22ms

### Sub_protocol DSM2_11
DSM2, Resolution 2048, refresh rate 11ms

### Sub_protocol DSMX_22
DSMX, Resolution 2048, refresh rate 22ms

### Sub_protocol DSMX_11
DSMX, Resolution 2048, refresh rate 11ms

### Sub_protocol AUTO
The "AUTO" feature enables the TX to automatically choose what are the best settings for your DSM RX and update your model protocol settings accordingly.

The current radio firmware which are able to use the "AUTO" feature are ersky9x (9XR Pro, 9Xtreme, Taranis, ...) and er9x for M128 (9XR) and M2561.
For these firmwares, you must have a telemetry enabled TX and you have to make sure you set the Telemetry "Usr proto" to "DSMx".
Also on er9x you will need to be sure to match the polarity of the telemetry serial (normal or inverted by bitbashing), while on ersky9x you can set "Invert COM1" accordinlgy.

## J6Pro

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|----|----|----
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

# NRF24L01 RF Module

## ASSAN
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10

The transmitter must be close to the receiver while binding.

## BAYANG
Models: EAchine H8(C) mini, BayangToys X6/X7/X9, JJRC JJ850, Floureon H101 ...

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|----
A|E|T|R|FLIP|RTH|PICTURE|VIDEO|HEADLESS|INVERTED

## CG023
Models: EAchine CG023/CG031/3D X4

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

### Sub_protocol YD829
Models: Attop YD-822/YD-829/YD-829C ...

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP||PICTURE|VIDEO|HEADLESS

### Sub_protocol H8_3D
Models: EAchine H8 mini 3D, JJRC H20/H22

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP|LIGTH|OPT1|OPT2|CAL

JJRC H20: OPT1=Headless, OPT2=RTH

JJRC H22: OPT1=RTH, OPT2=180/360° flip mode

H8 3D: OPT1=RTH then press a direction to enter headless mode (like stock TX), OPT2=switch 180/360° flip mode

CAL: calibrate accelerometers

## CX10
Extended limits supported

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|FLIP|RATE

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3

### Sub_protocol GREEN
Models: Cheerson CX-10 green pcb

Same channels assignement as above.

### Sub_protocol BLUE
Models: Cheerson CX-10 blue pcb & some newer red pcb, CX-10A, CX-10C, CX11, CX12, Floureon FX10, JJRC DHD D1

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|RATE|PICTURE|VIDEO

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3 or headless for CX-10A

### Sub_protocol DM007

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS

### Sub_protocol Q282 and Q242

CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---
FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

Model: JXD 509 is using Q282 with CH12=Start/Stop motors

### Sub_protocol JC3015_1

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|PICTURE|VIDEO

### Sub_protocol JC3015_2

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|LED|DFLIP

### Sub_protocol MK33041

CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS|RTH

## ESKY

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GYRO|PITCH

## FY326

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RTH|HEADLESS|EXPERT|CALIBRATE

## FQ777
Model: FQ777-124

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RTH|HEADLESS|EXPERT

## HISKY
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|GEAR|PITCH|GYRO|CH8

GYRO: -100%=6G, +100%=3G

### HK310
Models: RX HK-3000, HK3100 and XY3000 (TX are HK-300, HK-310 and TL-3C)

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
|||T|R|AUX|T_FSAFE|R_FSAFE|AUX_FSAFE

## KN
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|DR|THOLD|IDLEUP|GYRO|Ttrim|Atrim|Etrim

Dual Rate: +100%=full range, Throttle Hold: +100%=hold, Idle Up: +100%=3D, GYRO: -100%=6G, +100%=3G

### Sub_protocol WLTOYS

### Sub_protocol FEILUN
Same channels assignement as above.

## HONTAI
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|CAL

### Sub_protocol HONTAI

### Sub_protocol JJRCX1
CH6|
---|
ARM|

### Sub_protocol X5C1 clone

## MJXQ
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|AUTOFLIP|PAN|TILT

### Sub_protocol WLH08

### Sub_protocol X600
Only 3 TX IDs available, change RX_Num value 0..2 to cycle through them

### Sub_protocol X800
Only 3 TX IDs available, change RX_Num value 0..2 to cycle through them

### Sub_protocol H26D

### Sub_protocol E010

## MT99XX
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS

### Sub_protocol MT
Models: MT99xx

### Sub_protocol H7
Models: Eachine H7, Cheerson CX023

### Sub_protocol YZ
Model: Yi Zhan i6S
Only one model can be flown at the same time since the ID is hardcoded.

### Sub_protocol LS
Models: LS114, 124, 215

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|INVERT|PICTURE|VIDEO|HEADLESS

## Shenqi
Autobind protocol

Model: Shenqiwei 1/20 Mini Motorcycle

CH1|CH2|CH3|CH4
---|---|---|---
 | |T|R

Throttle +100%=full forward,0%=stop,-100%=full backward.

## SLT
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GEAR|PITCH

## Symax
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP||PICTURE|VIDEO|HEADLESS

### Sub_protocol SYMAX
Models: Syma X5C-1/X11/X11C/X12

### Sub_protocol SYMAX5C
Model: Syma X5C (original) and X2

## V2X2
Models: WLToys V202/252/272, JXD 385/388, JJRC H6C, Yizhan Tarantula X6 ...

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|MAG_CAL_X|MAG_CAL_Y

PICTURE: also automatic Missile Launcher and Hoist in one direction

VIDEO: also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir

## YD717
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

### Sub_protocol YD717

### Sub_protocol SKYWLKR

### Sub_protocol SYMAX4

### Sub_protocol XINXUN

### Sub_protocol NIHUI
Same channels assignement as above.

