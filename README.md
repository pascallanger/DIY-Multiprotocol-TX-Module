# DIY-Multiprotocol-TX-Module
Multiprotocol is a TX module which enables any TX to control lot of different models available on the market.

[Main Thread on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676)

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952733-114-thumb-P4100002.JPG?d=1433910155) ![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952734-189-thumb-P4100003.JPG?d=1433910159)

##Compatible TX

###Using standard PPM output (trainer port)
The multiprotocol TX module can be used on any TX with a trainer port.

The protocol selection is done via a dip switch or a rotary dip switch for access to up to 15 different protocols. The selection must be done before the power is applied.

To bind a model press the bind button, apply power and then release.

###Using a serial output
The multiprotocol TX module can be used on a Turnigy 9X, 9XR, 9XR Pro, Taranis, ... running er9x or ersky9X. (A version for OpenTX is being looked at)

This enables full integration using the radio GUI to setup models with all the available protocols options.

![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8520065-194-thumb-IMG_20151217_002215%20%28Medium%29.jpg?d=1450308588)

Options are:
- Protocol and type: many main protocols have variants
- RX Num: number your different RXs and make sure only one model will react to the commands
- Power: High or low, enables to lower the power setting of your TX (indoor for example). 
- Option: -127..+127 allowing to set specific protocol options. Like for Hubsan to set the video frequency.
- Bind: bind a RX/model
- Autobind: Yes or No. At the model selection (or power applied to the TX) a bind sequence will be initiated
- Range: test range by setting the transmission power to the lowest value

##Hardware

###RF modules
Up to 4 RF modules can be installed:
- [A7105](http://www.banggood.com/XL7105-D03-A7105-Modification-Module-Support-Deviation-Galee-Flysky-p-922603.html)    for Flysky, Hubsan
- [CC2500](http://www.banggood.com/CC2500-PA-LNA-Romote-Wireless-Module-CC2500-SI4432-NRF24L01-p-922595.html)   for Frsky
- [CYRF6936](http://www.ehirobo.com/walkera-wk-devo-s-mod-devo-8-or-12-to-devo-8s-or-12s-upgrade-module.html) for DSM2, DSMX, DEVO, Walkera
- [NRF24L01](http://www.banggood.com/2_4G-NRF24L01-PA-LNA-Wireless-Module-1632mm-Without-Antenna-p-922601.html) for Hisky, V2x2, CX-10, SYMAX and plenty other protocols

RF modules can be installed for protocols need only. Example: if you only need the Hubsan protocol then install only a A7105 on your board.

You also need some [antennas](http://www.banggood.com/2_4GHz-3dBi-RP-SMA-Connector-Booster-Wireless-Antenna-Modem-Router-p-979407.html) and [cables](http://www.banggood.com/10cm-PCI-UFL-IPX-to-RPSMA-Female-Jack-Pigtail-Cable-p-924933.html).

###Microcontroller
The main program is running on a ATMEGA328 running @16MHz and 3.3V.
An [Arduino pro mini](http://www.banggood.com/Wholesale-New-Ver-Pro-Mini-ATMEGA328-328p-5V-16MHz-Arduino-Compatible-Nano-Size-p-68534.html) can be used to build your Multimodule:

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t8214655-87-thumb-uploadfromtaptalk1405598143749.jpg?d=1441459923)

or build your own board using SMD components and an associated PCB:

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/t7952726-108-thumb-image-62c29cf2.jpg?d=1433909893)

###Schematic
![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/a8443844-119-multiprotocol_diagram_rotary_serial_2.jpg)

###Radio integration
You can 3D print your box (detalis [here](http://www.rcgroups.com/forums/showpost.php?p=33294140&postcount=2034)):

![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8462144-54-thumb-Multi_case_9XR.jpg?d=1448575289)
![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8462145-106-thumb-Multi_case_v1.jpg?d=1448575293)
