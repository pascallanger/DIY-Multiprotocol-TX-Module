#Advanced Topics {This page is currently a proof of concept}
Warning: the topics on this page are not for the fainthearted.  It is strongly recommended that you have some experience in getting up and runnning with your module before you dive in there.  On the other hand what is described on this page are some very useful options that could greatly increase the value and the enjoyment of your Multiprotocol module.
#Serial uploader that works through the transmitter pins
This document describes how you can set up your ATmega-based Mulitprotocol module to allow you to update the firmware by connecting a USB to TTL serial (like a FTDI) adapter to the module's transmitter interface pins. It is great if you exclusively use the Serial interface with your transmitter because the Bind button is used as "bootloader" button.  It requires a small custom bootloader to be uploaded and a simple interface cable to be soldered up.  See the [Advanced ATmega Serial Uploader](Advanced_ATmega_Serial_Uploader.md) page for more details.  
Created and supported by: Mike Blandford 

RCGroups page: http://www.rcgroups.com/forums/showpost.php?p=35584619&postcount=4867

#Bluetooth telemetry board for telemetry in PPM mode
This document describes a simple bluetooth module to stream telemetry information to a mobile device like an Android smartphone or tablet.  This is very useful with modules used in the PPM mode with transmitters that do not support telemetry.  See the [Advanced Bluetooth Telemetry](Advanced_Bluetooth_Telemetry.md) page for more details.  
Created and supported by: Midelic 

RCGroups page: None


#Manually setting fuses on ATmega328
This document describes a relatively simple process to set the fuses on ATmega328 using the flexibility of the command line.  It does not require installation of AVRdude because it uses the AVRdude that is bundled with the Arduino IDE.   See the [Advanced Manually Setting ATmega328 Fuses](Advanced_Manually_Setting_ATmega328_Fuses.md) page for more details.  
Created and supported by: hpnuts 

RCGroups page: No rcgroups page
