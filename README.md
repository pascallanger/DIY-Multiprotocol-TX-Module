# DIY-Multiprotocol-TX-Module

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952733-114-thumb-P4100002.JPG?d=1433910155) ![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952734-189-thumb-P4100003.JPG?d=1433910159)

Fork du projet https://github.com/pascallanger/DIY-Multiprotocol-TX-Module

Afin d'ajouter :
- Une sélection du protocole via les manches de la radio
- Un rebind hardware en PPM
- La radio TARANIS (TAERB, B = rebind ;-) ) et redéclaration des radios
- Un script "LUA" afin de faciliter la position des manches



Programme des évolutions :
- Ajout du de la télémetrie TARANIS à l'aide du projet https://github.com/shadow974/TxAdapter

	(Attention, il faut rajouter un transistor afin d'inverser et amplifier le signal)


#Schematic
![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/a8443844-119-multiprotocol_diagram_rotary_serial_2.jpg)

Notes:
- Attention: All modules are 3.3V only, never power them with 5V.
- For serial, the dial switch is not needed and the bind button optionnal
- Ajout d'un switch + transistor sur le TX
![Alt text](telemetryFRSKY.jpg)


#Protocoles ajoutés mais non testés (Issue de Deviation)
##CYRF6936 RF Module
###J6PRO
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12
---|---|---|---|---|---|---|---|---|---|---|---
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8|CH9|CH10|CH11|CH12

###WK2x01
En cours ...

##A7105 RF Module
###Joysway
CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

##NRF24L01 RF Module
###CFLIE
Modele: CrazyFlie Nano quad
CH1|CH2|CH3|CH4
---|---|---|---
A|E|T|R

###Fy326
Autobind
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|FLIP|HEADLESS|RTH|Calibrate

###H377
CH1|CH2|CH3|CH4|CH5|CH6|CH7|CH8
---|---|---|---|---|---|---|---
A|E|T|R|CH5|CH6|CH7|CH8

###HM830
Modele: HM Hobby HM830 RC Paper Airplane
CH1|CH2|CH3|CH4|CH5
---|---|---|---
A|Turbo|T|Trim|Bouton ???

