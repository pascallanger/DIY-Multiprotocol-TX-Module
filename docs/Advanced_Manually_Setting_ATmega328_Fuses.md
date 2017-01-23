#Manually Uploading HEX files and setting ATmega328 Fuses

There are many different options to upload a .hex firmware file to the MULTI-Module and to set the correct fuses.  This document outlines an approach that uses a USBASP programmer and which is equally compatible with OSX, Windows and Linux operating systems.  It does this by using the uploading capabilities bundled in the Arduino IDE package.  There are five steps to the process:  

1. Finding the location of the AVRdude uploader and the uploading command
2. Modifying the uploading command to create three commands:
   - Verify the connection and current fuse settings
   - Upload your .hex file
   - Set your fuses
2. Verify 
1. Uploading your firmware
1. Setting your fuses



## Fuse settings
To understand fuses refer to the ATmega328P datasheet (See Section 28.3 Fuse Bits)
Here are some fuse settings for common configurations:

Board|Low Fuse|High Fuse|Extended Fuse
-----|--------|---------|-------------
Arduino Pro Mini with Arduino bootloader|0xFF|0xD2|0xFD
DIY 2.3d PCB without bootloader |0xFF|0xD3|0xFD
DIY 2.3d PCB with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD
Banggood 4-in-1 module without bootloader |0xFF|0xD3|0xFD
Banggood 4-in-1 module with [custom mikeb bootloader](Advanced_ATmega_Serial_Uploader.md) |0xFF|0xD6|0xFD


## Step 1. Determining the location of the avrdude program
The AVRdude software is commonly used to upload firmware and set fuses on the ATMega microprocessor. 

You can install avrdude on your computer, but it is already contained in the Arduino IDE bundle and we suggest that you use the Arduino-bundled version.  

1. Unplug any programmer that may be connected to the computer
1. In the Arduino IDE Tools -> Board and select the Arduino Pro Mini board
1. Then click on Sketch -> Upload Using Programmer
1. After a series of compiling messages you will see an error that a programmer is not found.  Scroll up and find the programming command that caused the errors (usually the last white line before the red errors) and copy it into TextEdit or Notepad. 
1. This is the command we will be modifying to verify, program and set fuses.

It should look something like this:

**Mac:**

> /Applications/Arduino.app/Contents/Java/hardware/tools /avr/bin/avrdude  -C/Applications/Arduino.app/Contents/ Java/hardware/tools/avr/etc/avrdude.conf -patmega328p -cusbasp -Pusb -Uflash:w:{this part will be unique to your system} /Multiprotocol.ino.hex:i 

**PC:** 

> C:\Program Files (x86)\Arduino\Contents\Java\hardware\tools\ avr\bin\avrdude -CC:\Program Files (x86)\Arduino\Contents\Java\ hardware\tools\avr\etc\avrdude.conf -patmega328p -cusbasp -Pusb -Uflash:w:{this part will be unique on your system}\ Multiprotocol.ino.hex:i 


## Step 2. Modifying the command

### Verify commnad
Select all the text up to the ```-Uflash ``` command, copy it and paste it into a new line and add a “-v” (without the "") at the end of the line.  
 
 This is your “verify” command and it should look something like this:

> /Applications/Arduino.app/Contents/Java/hardware/tools/avr/bin/avrdude -C/Applications/Arduino.app/Contents/Java/hardware/ tools/avr/etc/avrdude.conf -patmega328p -cusbasp -Pusb -v

### Program command
Select all the text up to and including the ```-Uflash:w: ```, copy it and paste it into a new line.  Add the full name of the .hex firmware file you wish to upload, including the ".hex" suffix, and at the end add a ":i". There should be no spaces before or after the file name.  

This is your “program” command and it should look something like this:

> /Applications/Arduino.app/Contents/Java/hardware/tools /avr/bin/avrdude  -C/Applications/Arduino.app/Contents/ Java/hardware/tools/avr/etc/avrdude.conf -patmega328p -cusbasp -Pusb -Uflash:w:**Multiprotocol.ino.hex**:i 

We will be using the command line to program the module.

## STEP 3: Verify

Check that your USBASP programmer is set to 3.3.V (make 100% certain or you will blow all your RF modules). Plug your USBASP into your computer and connect it to the ISP pins on your MULTI-Module. 

Open a terminal or command window and change to the directory where your .hex file is located

Copy and past the "verify" command from above into the terminal and press enter.  You should see an output that looks something like this.  Confirm that a connection has been made with the MCU. It may be important to note and record the fuse settings and compare them with the table above.

Once you have established that the programmer is connecting correctly move on to the programming step.

## STEP 4: Program

Copy and past the "program" command from above into the terminal and press enter.  You should see an output shows that the correct board has been identified and progress uploading the firmware.

If this completes successfully you have flashed the new firmware onto the board.

## STEP 5: Set Fuses

1. It is always good practice to check on the connection with the board before you program fuses.  This is the one step that could "brick" your MCU.   Paste the "verify" command from above into a command line terminal.  It should retun with messages that indicate an ATmega328p was successfully found and it should return the current fuse settings.
1. To program the Low Fuse to 0xFF (for example) copy the “verify” command and paste it into the shell add the following text to the end of the line ```-U lfuse:w:0xFF:m ``` .  Press Enter. **Note: If you want a different fuse setting, change the 0xFF with the hexadecimal value of the low fuse setting.**  
1. To program the Extended Fuse  to 0xFD (for example) copy the “verify” command and paste it into the shell add the following text to the end of the line ```-U efuse:w:0xFD:m ``` .  Press Enter.  **Note: If you want a different fuse setting, change the 0xFD with the hexadecimal value of the extended fuse setting.** 
1. There are two options for the High fuse.  
 - If you selected the 4-in-1 Board in the Arduino IDE then you compiled firmware to not include the unecessary Arduino bootloader.  To program the High Fuse  to 0xD3 (for example) copy the “verify” command and paste it into the shell and add the following text to the end of the line ```-U hfuse:w:0xD3:m ``` .  Press Enter. **Note: If you want a different fuse setting, change the 0xD3 with the hexadecimal value of the extended fuse setting.** 
 - If you selected the Arduino Pro-Mini (or any other Arduino board) in the Arduino IDE then you compiled firmware to include the Arduino bootloader.  To program the High Fuse  to 0xD2 (for example) copy the “verify” command and paste it into the shell and add the following text to the end of the line ```-U hfuse:w:0xD2:m ``` .  Press Enter. **Note: If you want a different fuse setting, replace the 0xD2 with the hexadecimal value of the extended fuse setting.** 
