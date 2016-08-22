# DIY-Multiprotocol-TX-Module
Multiprotocol is a 2.4GHz transmitter which enables any TX to control lot of different models available on the market.

The source code is partly based on the Deviation TX project, thanks to all the developpers for their great job on protocols.

[Forum link on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676) for additional information or requesting a new protocol integration.

**To download the latest compiled version (.bin file), click on  [Release](https://github.com/midelic/DIY-Multiprotocol-TX-Module/releases) on the top menu.**

##Contents

[Compatible TX](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/README.md#compatible-tx)

[Protocols](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/README.md#protocols)

[Hardware](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/README.md#hardware)

[Compilation and programmation](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/README.md#compilation-and-programmation)

[Troubleshooting](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/README.md#troubleshooting)

##Compatible TX

###Using standard PPM output (trainer port)
The multiprotocol TX module can be used on any TX with a trainer port.

Channels order is AETR by default but can be changed in the _Config.h.

The protocol selection is done via a dip switch, rotary dip switch or scsi ID selector.

![Screenshot](http://media.digikey.com/photos/CTS%20Photos/206-4,%20206-4ST_sml.jpg)
![Screenshot](http://media.digikey.com/photos/Grayhill%20Photos/94HBB16T_sml.jpg)
![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8637216-7-thumb-SCSI%20ID%20selector.jpg?d=1453737244)

You can access to up to 15 different protocols and associated settings.
 
Settings per selection are located in _Config.h:
 - Protocol and type: many main protocols have variants
 - RX Num: number your different RXs and make sure only one model will react to the commands
 - Power: High or low, enables to lower the power setting of your TX (indoor for example). 
 - Option: -127..+127 allowing to set specific protocol options. Like for Hubsan to set the video frequency.
 - Autobind: Yes or No. At the model selection (or power applied to the TX) a bind sequence will be initiated
 
###Using a serial output
The multiprotocol TX module takes full advantage of being used on a Turnigy 9X, 9XR, 9XR Pro, Taranis, 9Xtreme, AR9X, ... running [er9x](http://openrcforums.com/forum/viewtopic.php?f=5&t=4598) or [ersky9X](http://openrcforums.com/forum/viewtopic.php?f=7&t=4676). An OpenTX version for Taranis is available [here](http://plaisthos.de/opentx/).

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

Notes:
 - Using this solution does not need any modification of the TX since it uses the TX module slot PPM pin for serial transfer.
 - There are 2 versions of serial protocol either 8 or 16 channels. 16 channels is the latest and only available version going forward. Make sure to use the right version based on your version of er9x/ersky9x.
 - Channels order is AETR by default but can be changed in _Config.h.

###Telemetry

There are 4 protocols supporting telemetry: Hubsan, DSM, FrSky and FrSkyX.

Hubsan displays the battery voltage and TX RSSI.

DSM displays TX RSSI and full telemetry.

FrSky displays full telemetry (A0, A1, RX RSSI, TX RSSI and Hub).

FrSkyX displays full telemetry (A1, A2, RX RSSI, TX RSSI and Hub).

### If used in PPM mode

Telemetry is available as a serial 9600 8 n 1 output on the TX pin of the Atmega328p using the FRSky hub format for Hubsan, FrSky, FrSkyX and DSM format for DSM2/X.

You can connect it to your TX if it is telemetry enabled or use a bluetooth adapter (HC05/HC06) along with an app on your phone/tablet ([app example](https://play.google.com/store/apps/details?id=biz.onomato.frskydash&hl=fr)) to display telemetry information and setup alerts.

### If used in Serial mode
Telemetry is built in for er9x and ersky9x TXs.

To enable telemetry on a Turnigy 9X or 9XR you need to modify your TX following one of the Frsky mod like this [one](http://blog.oscarliang.net/turnigy-9x-advance-mod/).

Note: DSM telemetry is not available on er9x due to a lack of flash space.

Enabling telemetry on a 9XR PRO and may be other TXs does not require any hardware modifications. The additional required serial pin is already available on the TX back module pins.

Once the TX is telemetry enabled, it just needs to be configured on the model (see er9x/ersky9x documentation).

##Protocols

###TX ID
The multiprotocol TX module is using a 32bits ID generated randomly at first power up. This global ID is used by nearly all protocols.
There are little chances to get a duplicated ID.

For DSM2/X and Devo the CYRF6936 unique manufacturer ID is used.

It's possible to generate a new ID using bind button on the Hubsan protocol during power up.

###Bind
To bind a model in PPM Mode press the physical bind button, apply power and then release.

In Serial Mode you have 2 options:
- use the GUI, access the model protocol page and long press on Bind. This operation can be done at anytime.
- press the physical bind button, apply power and then release. It will request a bind of the first loaded model protocol.

Notes:
- the physical bind button is only effective at power up. Pressing the button later has no effects.
- a bind in progress is indicated by the LED fast blinking. Make sure to bind during this period.

###Protocol selection

####Using the dial for PPM input
PPM is only allowing access to a subset of existing protocols.
The protocols, subprotocols and all other settings can be personalized by modifying the **_Config.h** file. 

The default association dial position / protocol in every release is listed below.

Dial|Protocol|Sub_protocol|RX Num|Power|Auto Bind|Option|RF Module
----|--------|------------|------|-----|---------|------|---------
0|Select serial||||||
1|FLYSKY|Flysky|0|High|No|0|A7105
2|HUBSAN|-|0|High|No|0|A7105
3|FRSKY|-|0|High|No|-41|CC2500
4|HISKY|Hisky|0|High|No|0|NRF24L01
5|V2X2|-|0|High|No|0|NRF24L01
6|DSM2|DSM2|0|High|No|6|CYRF6936
7|DEVO|-|0|High|No|0|CYRF6936
8|YD717|YD717|0|High|No|0|NRF24L01
9|KN|WLTOYS|0|High|No|0|NRF24L01
10|SYMAX|SYMAX|0|High|No|0|NRF24L01
11|SLT|-|0|High|No|0|NRF24L01
12|CX10|BLUE|0|High|No|0|NRF24L01
13|CG023|CG023|0|High|No|0|NRF24L01
14|BAYANG|-|0|High|No|0|NRF24L01
15|SYMAX|SYMAX5C|0|High|No|0|NRF24L01

Note:
- The dial selection must be done before the power is applied.

####Using serial input with er9x/ersky9x
Serial is allowing access to all existing protocols & sub_protocols listed below.

#####A7105 RF module
Protocol|Sub_protocol
--------|------------
Flysky|
 |Flysky
 |V9x9
 |V6x6
 |V912
Hubsan|

#####CC2500 RF module
Protocol|Sub_protocol
--------|------------
FrSky|
FrSkyX|
 |CH_16
 |CH_8
SFHSS|

#####CYRF6936 RF module
Protocol|Sub_protocol
--------|------------
DSM2|
 |DSM2
 |DSMX
Devo|

#####NRF24L01 RF module
Protocol|Sub_protocol
--------|------------
Hisky|
 |Hisky
 |HK310
V2x2|
YD717|
 |YD717
 |SKYWLKR
 |SYMAX4
 |XINXUN
 |NIHUI
KN|
 |WLTOYS
 |FEILUN
SymaX|
 |SYMAX
 |SYMAX5C
SLT|
CX10|
 |GREEN
 |BLUE
 |DM007
 |Q282
 |JC3015_1
 |JC3015_2
 |MK33041
 |Q242
CG023|
 |CG023
 |YD829
 |H8_3D
Bayang|
FrskyX||CC2500
ESky|
MT99XX|
 |MT
 |H7
 |YZ
MJXQ|
 |WLH08
 |X600
 |X800
 |H26D
Shenqi|
FY326|

Note:
- The dial should be set to 0 for serial. Which means all protocol selection pins should be left unconnected.

###Protocols details
**Check the [Protocols_Details.md](./Protocols_Details.md) file for a detailed description of every protocols with channels assignements.**

##Hardware

You also need some [antennas](http://www.banggood.com/2_4GHz-3dBi-RP-SMA-Connector-Booster-Wireless-Antenna-Modem-Router-p-979407.html) and [cables](http://www.banggood.com/10cm-PCI-UFL-IPX-to-RPSMA-Female-Jack-Pigtail-Cable-p-924933.html).

###Board
The main program is running on an STM32F103CBT6 chip running @8MHz X-tal and 3.3V power supply.

####Build your own board using [SMD components]

![Screenshot](https://644db4de3505c40a0444-327723bce298e3ff5813fb42baeefbaa.ssl.cf1.rackcdn.com/f2d598eacd8e9eebc3818adf47777199.png)
![Screenshot](https://644db4de3505c40a0444-327723bce298e3ff5813fb42baeefbaa.ssl.cf1.rackcdn.com/2026cfd1d0187a770570052590168df1.png)

[OSH Park link](https://oshpark.com/shared_projects/toBXcpNK)  if you want to order.

####Buy a 4 in 1 tuner to be used with new STM32 multimodule board

This module(4 in 1 tuner) can be purchased [here](http://www.banggood.com/DIY-2_4G-CC2500-NRF24L01-A7105-CYRF6936-Multi-RF-4-IN-1-Wireless-Module-p-1046308.html?AID=12202217&PID=3836173&SID=ir5dm8sw730004o402ecu&source=affiliate&utm_source=Banggood_CJ&utm_medium=commission_junction&utm_campaign=OpenPilot&utm_content=sandy). All the 4 RF modules are already implemented A7105, NRF24L01, CC2500 and CYRF6936. The board is also equiped with an antenna switcher which means only one antenna for all.

To update the firmware of this module you have to solder a 5 pin header(bottom ) and use an USB-serial like FTDI 

Serial mode  is already enabled by default so no need any resistor soldering.

###Schematic and Parts list

The Part list is [here](https://github.com/midelic/DIY-Multiprotocol-TX-Module/blob/multi-STM32/Multiprotocol_STM32.txt).

![Screenshot](http://static.rcgroups.net/forums/attachments/4/0/8/5/8/3/a9220887-113-multi-STM32.png)

Notes:
- Attention: All modules are 3.3V only, never power them with 5V.
- For serial, the dial switch is not needed and the bind button optionnal

###Radio integration
You can 3D print your box (details [here](http://www.rcgroups.com/forums/showpost.php?p=33294140&postcount=2034)):

![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8462144-54-thumb-Multi_case_9XR.jpg?d=1448575289)
![Screenshot](http://static.rcgroups.net/forums/attachments/1/1/5/4/3/7/t8462145-106-thumb-Multi_case_v1.jpg?d=1448575293)

##Compilation and programmation

###Toolchain
Multiprotocol source can be compiled using the Arduino IDE.

The currently supported Arduino version is [1.6.5](https://www.arduino.cc/download_handler.php?f=/arduino-1.6.5-windows.exe).
You need to download and install aditionally [STM32 core](https://github.com/rogerclarkmelbourne/Arduino_STM32/archive/master.zip) ,Arduino_STM32 folder in ...../Arduino/hardware/ folder (note. if the hardware folder doesn't exist you will need to create it).

In order to compile suscesfully you need also to modify a maple library file.
In ....\hardware\Arduino_STM32\STM32F1\cores\maple\libmaple\usart_f1.c
Comment the 2 function as shown below.This is required to have acces to low level of USART intrerupt.

//void __irq_usart2(void) {
 usart_irq(&usart2_rb, USART2_BASE);
}

//void __irq_usart3(void) {
usart_irq(&usart3_rb, USART3_BASE);	
}	


Then run the IDE, and on the Tools menu, select the Boards manager, and install the Arduino DUE(ARM-Cortex M3) from the list of available boards. You must do this step, it installs the arm-none-eabi-g++ toolchain!

Download the [zip file](https://github.com/midelic/DIY-Multiprotocol-TX-Module/archive/multi-STM32.zip) of this repository, unzip it in a folder, navigate to the Multiprotocol directory and then click on Multiprotocol.ino. The Arduino environment will appear and the Multiprotocol project will be loaded.Then you can use "Upload" button which will compile and upload to the module: Skecth->Upload (Ctrl+U).

Warning:
The dial must be set to 0 before flashing!

Notes:
- Make sure to select in arduino IDE the "Generic STM32F103C series" before compiling.
- Before uploading select from Tools, Upload Method: "Serial".
- Compilation of the code posted here works. So if it doesn't for you this is a problem with your setup, please double check everything before asking.

###Upload the code using FTDI (USB serial to TTL)

Use an external FTDI adapter like [this one](http://www.banggood.com/FT232RL-FTDI-USB-To-TTL-Serial-Converter-Adapter-Module-For-Arduino-p-917226.html).
or other USB serial-TTL dongle.The device can work with both  5V and 3.3V. When in 3.3V selection there are some precautions to be taken ,you can read below.

###Important:
Flashing:
###Method 1:

Don't flash new firmware with the TX switched on !!!!!

If using 3.3V USB-TTL device it is required to use a jumper on "3.3V jumper" on top of the board.After flashing it is imperative to be removed.

Before uploading new firmware use a jumper on BOOT0.After flashing new firmware remove also the BOOT0 jumper,or move it to oposite pin.

###Warning!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

After flashing new firmware it is mandatory to remove the 3.3V jumper if installed.If not when starting the module and micro you may fry them as 5V is supplied directly on the RF module and micro.Extreme care ,do not flash with 5V setting on USB-SERIAL and 3.3V jumper attached.If in doubt measure the serial  device output with a multimeter.

###Method 2.

Uploading new firmware with TX switched on.
For this method DON'T TOUCH THE 3.3V JUMPER(NO JUMPER)!!!
This method is less prone to mistakes.The module is inside the TX bay.Move/set the jumper on BOOT0.
Connect the USB-serial device TX ,RX,and GND lines to coresponding RX,TX GND on the multimodule.
Start TX ,plug the USB -serial device on computer USB port,open arduino IDE upload the sketch normally.
At the end of uploading remove the BOOT0 jumper.Start normally.


##Troubleshooting

###LED status
- off: program not running or a protocol selected with the associated module not installed.
- flash(on=0.1s,off=1s): invalid protocol selected (excluded from compilation or invalid protocol number)
- slow blink(on=0.5s,off=0.5s): serial has been selected but no valid signal has been seen on the RX pin.
- fast blink(on=0.1s,off=0.1s): bind in progress.
- on: normal operation.

###Protocol selection
####Input Mode - PPM

- The protocol/mode selection must be done before the power is applied.
- Connect 1 to 4 of the selection protocol pins to GND.

####Input Mode - Serial
Put dial switch on "zero" activates serial mode on multi.If you want to use permanetly serial  mode there is no need of the dial switch installation.

###Bind
Make sure to follow this procedure: press the bind button, apply power and then release it after 1sec. The LED should be blinking fast indicating a bind status and then fixed on when the bind period is over. It's normal that the LED turns off when you press the bind button, this behavior is not controlled by the Atmega328.
For serial, the preffered method is to bind via the GUI protocol page.

It migth happen that your module is always binding at power up. If this is the case, there is a big chance that you are using an Arduino Pro Mini with an external status LED. To work around this issue connect a 10K resistor between D13 and 3.3V.

###Report issues
You can report your problem using the [GitHub issue](https://github.com/midelic/DIY-Multiprotocol-TX-Module/issues) system or go to the [Main thread on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676) to ask your question.
Please provide the following information:
- Multiprotocol code version
- TX type
- Using PPM or Serial, if using er9x or ersky9x the version in use
- Different led status (multimodule and model)
- Explanation of the behavior and reproduction steps
