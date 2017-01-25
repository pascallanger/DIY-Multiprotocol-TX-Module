#Manually Uploading HEX files and setting Fuses on ATmega328

First, the .hex files provided are only for tests purpose as the recommended method is to use [Compiling and Programming](Compiling.md).

There are many different options to upload a .hex firmware file to the MULTI-Module and to set the correct fuses.  This document outlines an approach that uses a USBASP programmer and which is equally compatible with OSX, Windows and Linux operating systems.

1. Follow this section: [Install the Arduino IDE](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/docs/Compiling.md#install-the-arduino-ide-and-the-multiprotocol-project-firmware)
1. Make sure to write down the location of your installation since you need to know where avrdude is installed to configure the AVR8 Burn-O-Mat. For example on a default windows installation, avrdude.exe is located in "C:\Program Files (x86)\Arduino\hardware\tools\avr\bin" where "C:\Program Files (x86)\Arduino" is the installation path.
1. Follow this section: [Material you need to upload the firmware](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/docs/Compiling.md#material-you-need-to-upload-the-firmware)
1. Follow this section: [Connect the programmer](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/blob/master/docs/Compiling.md#connect-the-programmer)
1. Install [AVR8 Burn-O-Mat](http://avr8-burn-o-mat.brischalle.de/avr8_burn_o_mat_avrdude_gui_en.php) which is available for all platforms. Installation instructions are on the software page (Don't forget to install [Java](http://java.sun.com/javase/downloads) as explained).
1. Launch AVR8 Burn-O-Mat.
1. You should now have a window which looks like this: 
  <img src="images/AVR8BurnOMat-main.png" />
1. click on **Settings->AVRDUDE** and fill in the details about avrdude location using the installation path written previously as well as selecting USBASP for the programmer: 
  <img src="images/AVR8BurnOMat-settings.png" />
1. 


## Fuse settings
Here are some fuse settings for common configurations:

Board|Low Fuse|High Fuse|Extended Fuse
-----|--------|---------|-------------
Banggood 4-in-1 module |0xFF|0xD3|0xFD
Arduino Pro Mini |0xFF|0xD3|0xFD
DIY 2.3d PCB |0xFF|0xD3|0xFD
DIY 2.3d PCB with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD
Banggood 4-in-1 module with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD

If you don't know the 1st line is the one you want.
