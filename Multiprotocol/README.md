# DIY-Multiprotocol-TX-Module

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952733-114-thumb-P4100002.JPG?d=1433910155) ![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952734-189-thumb-P4100003.JPG?d=1433910159)

Fork du projet https://github.com/pascallanger/DIY-Multiprotocol-TX-Module

Afin d'ajouter :
- Un rebind hardware en PPM
- La radio TARANIS (TAERB, B = rebind ;-) ) et redéclaration des radios



Programme des évolutions :
- Ajout du futur protocole INAV


#Schematic
![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/a8443844-119-multiprotocol_diagram_rotary_serial_2.jpg)

Notes:
- Attention: All modules are 3.3V only, never power them with 5V.


#Protocoles ajoutés mais non testés (Issue de Deviation)
##Other
###OPENLRS
Empty protocol

##CYRF6936 RF Module
###J6PRO
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

###WK2x01
Autobind

####Sub_protocol WK2401
CH1|CH2|CH3|CH4
---|---|---|---
CH1|CH2|CH3|CH4


####Sub_protocol WK2601
Option:	

		0 = 5+1
		2 = 6+1
		..1 = Hélicoptère (. = autres options pour ce mode)
		.01 = Hélicoptère normal
		.11 = Hélicoptère avec pit inversé
		0.1 = Pitch curve -100
		1.1 = Pitch curve 100

CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|???|CONF|Gyro & Rudder mix

CONF:	Option 1 = Rate Throtle

		Option 2 = Pitch
		

####Sub_protocol WK2801
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8


##A7105 RF Module
###Flysky AFHDS2A
Telemetry enabled for battery voltage and TX RSSI
Option= 0-PWM_IBUS 1-PPM_IBUS 2-PWM_SBUS 3-PPM_SBUS

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|CH15
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---
T|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12|CH13|CH14|Failsave T

###HUBSAN
Models: Hubsan H102D, H107/L/C/D and Hubsan H107P/C+/D+

Autobind protocol

Telemetry enabled for battery voltage and TX RSSI

Option=vTX frequency (H107D) 5645 - 5900 MHz

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|LIGHT|PICTURE|VIDEO|HEADLESS
####Sub_protocol H301
CH5|CH6|CH7|CH8
---|---|---|---
LED|STAB|RTH|VIDEO

####Sub_protocol H501
CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
FLIP|LIGHT|RTH|VIDEO|HEADLESS|GPS_HOLD|ALT_HOLD|SNAPSHOT

###Joysway
CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

##CC2500 RF Module
###SKYARTEC
CH1|CH2|CH3|CH4|CH5|CH6|CH7
---|---|---|---|---|---|---
 ? | ? | ? | ? | ? | ? | ? 

##NRF24L01 RF Module
###BLUEFLY
Autobind

CH1|CH2|CH3|CH4|CH5|CH6
---|---|---|---|---|---
A|E|T|R|GEAR|PITCH

###CFLIE
Modele: CrazyFlie Nano quad

Autobind

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

###ESKY150

Autobind

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

###FBL100
Autobind

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
 ? | ? | ? | ? | ? | ? | ? | ? 

####Sub_protocol HP100
Same channels assignement as above.

###Fy326
Autobind

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9
---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP|HEADLESS|RTH|Calibrate|Expert

####Sub_protocol FY319
Same channels assignement as above.

###H377
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

###HM830
Modele: HM Hobby HM830 RC Paper Airplane

Autobind

CH1|CH2|CH3|CH4|CH5
---|---|---|---
A|Turbo|T|Trim|Bouton

###HONTAI
Autobind protocol

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|---|---|---|---|----|----
A|E|T|R|FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|CAL

####Sub_protocol HONTAI
####Sub_protocol JJRCX1
CH6|
---|
ARM|

####Sub_protocol X5C1
X5C1 clone
CH5|CH6|CH7|CH8|CH9|CH10|CH11
---|---|---|---|---|----|----
FLIP|LED|PICTURE|VIDEO|HEADLESS|RTH|CAL
####Sub_protocol FQ777
Format FQ777-951C

CH6|CH7|CH8|CH9
---|---|---|---
FLIP|SNAPSHOT|VIDEO|HEADLESS

###INAV
En cours de passage

###NE260
Modele: Nine Eagles SoloPro

Autobind

CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

###UDI
Modele: Known UDI 2.4GHz protocol variants, all using BK2421
* UDI U819 coaxial 3ch helicoper
* UDI U816/817/818 quadcopters
  - "V1" with orange LED on TX, U816 RX labeled '' , U817/U818 RX labeled 'UD-U817B'
  - "V2" with red LEDs on TX, U816 RX labeled '', U817/U818 RX labeled 'UD-U817OG'
  - "V3" with green LEDs on TX. Did not get my hands on yet.
* U830 mini quadcopter with tilt steering ("Protocol 2014")
* U839 nano quadcopter ("Protocol 2014")

Autobind

CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10
---|---|---|---|---|---|---|---|---|---
A|E|T|R|FLIP 360|FLIP|VIDEO|LED|MODE 2

####Sub_protocol U816_V1 (orange)
####Sub_protocol U816_V2 (red)
####Sub_protocol U839_2014
Same channels assignement as above.


###D'autres à venir