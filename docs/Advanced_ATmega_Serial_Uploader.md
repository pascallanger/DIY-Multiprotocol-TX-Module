# ATmega Serial Uploader

Mike Blandford adapted the optiboot bootloader for the 4-in-1 module to allow flashing of the module using a standard Arduino USB to serial adapter or FTDI adapter.  No need to open the module case. Once set up is very easy to use: 

1. plug the serial wires into the module connector, 
2. To activate the bootloader, set the rotary switch to 0 
3. hold the bind button down for 0.5s while connecting the USB end of the serial cable into the computer
4. Press upload on the Arduino IDE or issue an AVRdude command from the terminal.

It uses a baudrate of 57600, so is the same as a Pro Mini.

The Serial / FTDI connections  on the Tx module are as follows:
- Top Pin: Programmer Tx
- 2nd Pin: 
- 3rd Pin: Programmer V+
- 4th Pin: Programmer Gnd
- 5th Pin: Programmer Rx

The bootloader starts up, waits half a second, then checks the rotary switch and the bind button. If they aren't as described above, then the normal application runs.

While the bootloader is running, if it detects a communication problem, it configures the watchdog to reset in 16mS, then waits forever. 16mS later the board should reset, and then restart the bootloader, dropping back to the application half a second later.

This bootloader is for reading and writing the flash only, the EEPROM is not supported, neither is reading/writing the fuses, but it only uses 512 bytes of flash.

## Install the bootloader
To get the bootloader onto the ATmega you need to connect an flashing tool (like USBasp) to the 6-pin ISP connector on the board.
Simply flash the .hex file to get the bootloader on the chip, and change the high fuse at the same time.

The bootloader only uses 512 bytes of flash and is avaialble for download [here](http://www.rcgroups.com/forums/showatt.php?attachmentid=9291360&d=1472324155).  The orginal rcgroups post is [here](http://www.rcgroups.com/forums/showpost.php?p=35584619&postcount=4867). 

The HIGH fuse needs to be set to 0xD6.  (See the section  on [Manually Setting the ATmega328 Fuses](Advanced_Manually_Setting_ATmega328_Fuses.md).)
