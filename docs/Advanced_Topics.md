# Advanced Topics {This page is currently a proof of concept}
Warning: the topics on this page are not for the fainthearted.  It is strongly recommended that you have some experience in getting up and runnning with your module before you dive in there.  On the other hand what is described on this page are some very useful options that could greatly increase the value and the enjoyment of your Multiprotocol module.

# Serial uploader that works through the transmitter pins
This document describes how you can set up your ATmega-based Mulitprotocol module to allow you to update the firmware by connecting a USB to TTL serial (like a FTDI) adapter to the module's transmitter interface pins. It is great if you exclusively use the Serial interface with your transmitter because the Bind button is used as "bootloader" button.  It requires a small custom bootloader to be uploaded and a simple interface cable to be soldered up.  See the [Advanced ATmega Serial Uploader](Advanced_ATmega_Serial_Uploader.md) page for more details.  
Created and supported by: Mike Blandford 

RCGroups page: http://www.rcgroups.com/forums/showpost.php?p=35584619&postcount=4867

# Telemetry in PPM mode
It is possible to access the telemetry stream coming from the receiver through the MULTI-module. This document describes a simple bluetooth module to stream telemetry information to a mobile device like an Android smartphone or tablet.  The method may be generalized to feed telemetry to the transmitter if the transmitter has the capabilities to process the information.  This is very useful with modules used in the PPM mode with transmitters that do not support telemetry.  See the [Advanced Bluetooth Telemetry](Advanced_Bluetooth_Telemetry.md) page for more details.  
Created and supported by: Midelic 

RCGroups page: None


# Manually setting fuses on ATmega328
This document describes a relatively simple process to set the fuses on ATmega328 using the flexibility of the command line.  It does not require installation of AVRdude because it uses the AVRdude that is bundled with the Arduino IDE.   See the [Advanced Manually Setting ATmega328 Fuses](Advanced_Manually_Setting_ATmega328_Fuses.md) page for more details.  

Created and supported by: hpnuts

## Flashing Multi_STM32 module.

#### Flashing without Tx power

This is another method of Flshing Multi_STM32 which is riskier.This method is for skilled users who understand the task.

The key difference of this method is that the 3.3V FTDI cable must also provide power to the 5V circuitry during the flashing process.  To do this, a jumper must be enabled connecting the 3.3V VCC to the 5V line.  The risk is to forget 3.3V jumper in, after flashing  and when TX restarted.

**If the module is powered through the transmitter and this jumper(3.3V) is enabled, then it will feed 5V throughout the 3.3V circuit and this will fry your RF modules.  Do not plug the module into the transmitter before removing this jumper!**  

**YOU HAVE BEEN WARNED!!!.**

1. Remove the module from the transmitter bay
1. Set BOOT0 jumper Skip this step if you made your own cable.
1. Set the 3.3V jumper. 
1. Connect your 3.3V FTDI cable (USB - TTL serial) to  Multiprotocol serial port (RX,TX,GND,5V).  Connect the pins as follows:   
  - Module RX pin to FTDI TX pin
  - Module TX pin to FTDI Rx pin
  - Module GND to FTDI GND 
  - Module 5V to FTDI 3.3V FTDI power supply.
1. In arduino IDE under the **Tools** -> **Board:** check that you have selected the **Generic STM32F103C series** board 
1. Under **Tools** -> **Upload Method:** select **Serial**. 
1. Click "Upload" and the sketch will be uploaded normally.
1. Once the firmware has uploaded:   
   - Remove the 3.3V jumper!!!! 
   - Remove the BOOT0 jumper
   - Check that you removed the 3.3V jumper
1. Insert the module into the transmitter bay


RCGroups page: No rcgroups page
