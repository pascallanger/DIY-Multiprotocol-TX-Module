#ATmega Serial Uploader

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

##Install the bootloader
To get the bootloader onto the ATmega you need to connect an flashing tool (like USBasp) to the 6-pin ISP connector on the board.
Simply flash the .hex file to get the bootloader on the chip, and change the high fuse at the same time.

The bootloader only uses 512 bytes of flash and is avaialble for download [here](http://www.rcgroups.com/forums/showatt.php?attachmentid=9291360&d=1472324155).  The orginal rcgroups post is [here](http://www.rcgroups.com/forums/showpost.php?p=35584619&postcount=4867). 

The HIGH fuse needs to be set to 0xD6.  (See the section below on Setting the Fuses with AVRdude.)

## Setting fuses with AVRdude
###Determining the location of the avrdude program
The Arduino IDE is used to upload firmware and set fuses on the ATMega microprocessor. 

You can install avrdude on your computer, but it is already contained in the Arduino IDE bundle and we suggest that you use the Arduino-bundled version.  
1. Unplug any programmer that may be connected to the computer
1. In the Arduino IDE click on Sketch -> Upload Using Programmer
1. After a series of compiling messages you will see an error that a programmer is not found.  Scroll up and find the programming command that caused the errors (usually the last white line before the red errors) and copy it into TextEdit or Notepad. 
1. This is your programming command and it should look something like this:

**Mac:**


> ```
> /Applications/Arduino.app/Contents/Java/hardware/tools /avr/bin/avrdude  -C/Applications/Arduino.app/Contents/ Java/hardware/tools/avr/etc/avrdude.conf -patmega328p -cusbasp -Pusb -Uflash:w:{this part will be unique to your system} /Multiprotocol.ino.hex:i 
> ```

**PC:** 


> ```
> C:\Program Files (x86)\Arduino\Contents\Java\hardware\tools\ avr\bin\avrdude -CC:\Program Files (x86)\Arduino\Contents\Java\ hardware\tools\avr\etc\avrdude.conf -patmega328p -cusbasp -Pusb -Uflash:w:{this part will be unique on your system}\ Multiprotocol.ino.hex:i 
> ```


Select all the text up to the ```-Uflash ``` command, copy it and paste it into a new line and add a “-v” (without the "") at the end of the line.  
 
 This is your “verify” command and it should look something like this:

> ```
> /Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude -C/Applications/Arduino.app/Contents/Java/hardware/ tools/avr/etc/avrdude.conf -patmega328p -cusbasp -Pusb -v
> ```


We will be using these two commands to program the module.

1. Verify that the connection is working by pasting the Verify line into a terminal.  You should see output that includes the fuse settings. 
2. 1. To program the High Fuse copy the “verify” command and paste it into the shell add the following text to the end of the line ```-U hfuse:w:0xD6:m ``` .  Press Enter.
