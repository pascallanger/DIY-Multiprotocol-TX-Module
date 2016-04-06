#Protocols details
**You'll find below a detailed description of every supported protocols sorted by RF modules.**

Legend:
- Extended limits supported: -125%..+125% can be used and will be transmitted. Otherwise the default is -100%..+100% only.
- Autobind protocol: you do not need to press the bind button at power up to bind, this is done automatically.

***
#A7105 RF Module

##FLYSKY
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

###Sub_protocol V9X9
CH5|CH6|CH7|CH8
---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO

###Sub_protocol V6X6
CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---
FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

###Sub_protocol V912
CH5|CH6
---|---
BTMBTN|TOPBTN

##HUBSAN
Models: Hubsan H102D, H107/L/C/D and Hubsan H107P/C+/D+

Autobind protocol

Telemetry enabled for battery voltage and TX RSSI

Option=vTX frequency (H107D) 5645 - 5900 MHz

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

***
#CC2500 RF Module

##FRSKY
Extended limits supported

Telemetry enabled for A0, A1, RSSI, TSSI and Hub

Option=fine frequency tuning, usually 0 or -41 based on the manufacturer boards

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

##FRSKYX
Models: FrSky receivers X4R, X6R and X8R.

Extended limits supported

Telemetry enabled for A1 (RxBatt), A2, RSSI, TSSI and Hub

Option=fine frequency tuning, usually 0 or -41 based on the manufacturer boards

###Sub_protocol CH_16
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16
---|---|---|---|---|---|---|---|---|----|----|----|----|----|----|----
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15|CH16

###Sub_protocol CH_8
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

##SFHSS
Models: Futaba RXs and XK models.

Option=fine frequency tuning, usually 0 or -41 based on the manufacturer boards

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

***
#CYRF6936 RF Module

##DEVO
Extended limits supported

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8

##DSM2
Extended limits supported

Telemetry enabled for TSSI and plugins

option=number of channels and frame rate:
 - 0 : 4 channels @22ms
 - 1 : 5 channels @22ms
 - 2 : 6 channels @22ms
 - 3 : 7 channels @22ms

 - 4 : 4 channels @11ms
 - 5 : 5 channels @11ms
 - 6 : 6 channels @11ms
 - 7 : 7 channels @11ms

 - 8 : 8 channels @22ms
 - 9 : 9 channels @22ms
 - 10 : 10 channels @22ms
 - 11 : 11 channels @22ms
 - 12 : 12 channels @22ms

6 is usually giving the best results with most of the RX.

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|----|----|----
A|E|T|R|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

###Sub_protocol DSMX
Same as above

***
#NRF24L01 RF Module

##BAYANG
Models: EAchine H8(C) mini, BayangToys X6/X7/X9, JJRC JJ850, Floureon H101 ...

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|----
A|E|T|R|FLIP|RTH|PICTURE|VIDEO|HEADLESS|INVERTED

##CG023
Models: EAchine CG023/CG031/3D X4

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

###Sub_protocol YD829
Models: Attop YD-822/YD-829/YD-829C ...

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP||PICTURE|VIDEO|HEADLESS

###Sub_protocol H8_3D
Models: EAchine H8 mini 3D, JJRC H20/H22

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP|LIGTH|OPT1|OPT2|CAL

JJRC H20: OPT1=Headless, OPT2=RTH

JJRC H22: OPT1=RTH, OPT2=180/360° flip mode

H8 3D: OPT1=RTH + headless, OPT2=180/360° flip mode

CAL: calibrate accelerometers

##CX10
Extended limits supported

Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|FLIP|RATE

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3

###Sub_protocol GREEN
Models: Cheerson CX-10 green pcb

Same channels assignement as above.

###Sub_protocol BLUE
Models: Cheerson CX-10 blue pcb & some newer red pcb, CX-10A, CX-10C, CX11, CX12, Floureon FX10, JJRC DHD D1

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|RATE|PICTURE|VIDEO

Rate: -100%=rate 1, 0%=rate 2, +100%=rate 3 or headless for CX-10A

###Sub_protocol DM007

CH5|CH6|CH7|CH8|CH9
---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS

###Sub_protocol Q282 and Q242

CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---
FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|XCAL|YCAL

Model: JXD 509 is using Q282 with CH12=Start/Stop motors

###Sub_protocol JC3015_1

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|PICTURE|VIDEO

###Sub_protocol JC3015_2

CH5|CH6|CH7|CH8
---|---|---|---
FLIP|MODE|LED|DFLIP

###Sub_protocol MK33041

CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---
FLIP|MODE|PICTURE|VIDEO|HEADLESS|RTH

##ESKY

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GYRO|PITCH

##FY326

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|RTH|HEADLESS|EXPERT|CALIBRATE

##HISKY
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|GEAR|PITCH|GYRO|CH8

GYRO: -100%=6G, +100%=3G

###HK310
Models: RX HK-3000, HK3100 and XY3000 (TX are HK-300, HK-310 and TL-3C)

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
|||T|R|AUX|T_FSAFE|R_FSAFE|AUX_FSAFE

##KN
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|DR|THOLD|IDLEUP|GYRO|Ttrim|Atrim|Etrim

Dual Rate: +100%=full range, Throttle Hold: +100%=hold, Idle Up: +100%=3D, GYRO: -100%=6G, +100%=3G

###Sub_protocol WLTOYS
###Sub_protocol FEILUN
Same channels assignement as above.

##MJXQ
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13
---|---|---|---|---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|AUTOFLIP|PAN|TILT

###Sub_protocol WLH08
###Sub_protocol X600
###Sub_protocol X800
###Sub_protocol H26D

##MT99XX
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS

###Sub_protocol MT
Models: MT99xx
###Sub_protocol H7
Models: Eachine H7, Cheerson CX023
###Sub_protocol YZ
Model: Yi Zhan i6S
Only one model can be flown at the same time since the ID is hardcoded.

##Shenqi
Autobind protocol

Model: Shenqiwei 1/20 Mini Motorcycle

CH1|CH2|CH3|CH4
---|---|---|---
 | |T|R

Throttle +100%=full forward,0%=stop,-100%=full backward.

##SLT
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GEAR|PITCH

##Symax
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP||PICTURE|VIDEO|HEADLESS

###Sub_protocol SYMAX
Models: Syma X5C-1/X11/X11C/X12

###Sub_protocol SYMAX5C
Model: Syma X5C (original) and X2

##V2X2
Models: WLToys V202/252/272, JXD 385/388, JJRC H6C, Yizhan Tarantula X6 ...

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS|MAG_CAL_X|MAG_CAL_Y

PICTURE: also automatic Missile Launcher and Hoist in one direction

VIDEO: also Sprayer, Bubbler, Missile Launcher(1), and Hoist in the other dir

##YD717
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS

###Sub_protocol YD717
###Sub_protocol SKYWLKR
###Sub_protocol SYMAX4
###Sub_protocol XINXUN
###Sub_protocol NIHUI
Same channels assignement as above.
