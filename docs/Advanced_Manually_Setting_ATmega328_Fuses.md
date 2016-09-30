#Manually Setting ATmega328 Fuses

To understand fuses refer to the ATmega328P datasheet (See Section 28.3 Fuse Bits)
## Fuse settings
Here are some fuse settings for common configurations:

Board|Low Fuse|High Fuse|Extended Fuse
-----|--------|---------|-------------
Arduino Pro Mini with Arduino bootloader|0xFF|0xD2|0xFD
DIY 3.2d PCB without bootloader |0xFF|0xD3|0xFD
DIY 3.2d PCB with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD
Banggood 4-in-1 module without bootloader |0xFF|0xD3|0xFD
Banggood 4-in-1 module with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD


##Determining the location of the avrdude program
The Arduino IDE is used to upload firmware and set fuses on the ATMega microprocessor. 

You can install avrdude on your computer, but it is already contained in the Arduino IDE bundle and we suggest that you use the Arduino-bundled version.  

1. Unplug any programmer that may be connected to the computer
1. In the Arduino IDE click on Sketch -> Upload Using Programmer
1. After a series of compiling messages you will see an error that a programmer is not found.  Scroll up and find the programming command that caused the errors (usually the last white line before the red errors) and copy it into TextEdit or Notepad. 
1. This is your programming command. You can use it to manually upload the latest .hex file compiled by the Arduino IDE "Verify" button, and it should look something like this:

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


We will be using the command line to program the module.

1. It is good practice to check on the connection with the board before you program fuses.  Paste the "verify" command from above into a command line terminal.  It should retun with messages that indicate an ATmega328p was successfully found and it should return the current fuse settings.
1. To program the Low Fuse copy the “verify” command and paste it into the shell add the following text to the end of the line ```-U lfuse:w:0xFF:m ``` .  Press Enter. ** Note: If you want a different fuse setting, change the 0xFF with the hexadecimal value of the low fuse setting. ** 
1. To program the Extended Fuse copy the “verify” command and paste it into the shell add the following text to the end of the line ```-U efuse:w:0xFD:m ``` .  Press Enter.  ** Note: If you want a different fuse setting, change the 0xFD with the hexadecimal value of the extended fuse setting. ** 
1. There are two options for the High fuse.  
 - If you selected the 4-in-1 Board in the Arduino IDE then you compiled firmware to not include the unecessary Arduino bootloader.  To program the High Fuse copy the “verify” command and paste it into the shell and add the following text to the end of the line ```-U hfuse:w:0xD3:m ``` .  Press Enter. ** Note: If you want a different fuse setting, change the 0xD3 with the hexadecimal value of the extended fuse setting. ** 
 - If you selected the Arduino Pro-Mini (or any other Arduino board) in the Arduino IDE then you compiled firmware to include the Arduino bootloader.  To program the High Fuse copy the “verify” command and paste it into the shell and add the following text to the end of the line ```-U hfuse:w:0xD2:m ``` .  Press Enter. ** Note: If you want a different fuse setting, replace the 0xD2 with the hexadecimal value of the extended fuse setting. ** 
