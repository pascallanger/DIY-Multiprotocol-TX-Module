# Multi-Module EEPROM

The EEPROM is used to store the Multiprotocol Modules's global ID as well as details of bound receivers for certain protocols (AFHDS2A, Bugs, Devo, Walkera).

On an Atmega328p module the EEPROM is a dedicated and persistent data store, separate from the 32KB of flash memory.  On the STM32 module there is no dedicated EEPROM, so EEPROM functionality is emulated in the last 2KB of flash memory.

This makes it relatively easy for the STM32 EEPROM data to be accidentally erased.

If the EEPROM is erased a new global ID will be generated (random for Atmega modules; the MCU UUID for STM32 modules) and models will need to be re-bound.

Backups of the EEPROM data can be made so that configuration such as module ID and bound receivers can be moved between modules.  

**Note:** Backups can only restored to a module with same MCU type - i.e. an Atmega backup cannot be restored to an STM32 module.

The remainder of this doc is separated into sections for the STM32 and Atmega328p modules.

## STM32 Module
The EEPROM data is stored in the last 2KB of flash memory. It is read using stm32flash with the module in BOOT0 mode.
#### Tools Needed
* stm32flash (Download: [Windows](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module-Boards/raw/master/source/stm32/tools/win/stm32flash.exe), [Linux 32-bit](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module-Boards/raw/master/source/stm32/tools/linux/stm32flash/stm32flash), [Linux 64-bit](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module-Boards/raw/master/source/stm32/tools/linux64/stm32flash/stm32flash), [macOS](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module-Boards/raw/master/source/stm32/tools/macosx/stm32flash/stm32flash))
* Working USB-to-Serial (FTDI) adapter

#### Preparation
Ensure that the `BOOT0` jumper is installed and the module is connected via a USB-to-Serial adapter.

### Backing up the STM32 EEPROM
The syntax of the backup command is:
`stm32flash -r [file name] -S 0x801F800:2048 [serial port]`

The `-S 0x801F800:2048` option tells stm32flash to read 2048 bytes starting at 0x801F800.

Windows example:

`stm32flash.exe -r C:\Temp\eeprom.bin -S 0x801F800:2048 COM4`

Linux and macOS example:

`stm32flash -r /tmp/eeprom.bin -S 0x801F800:2048 /dev/ttyUSB0`

Output will look similar to this:
```
stm32flash 0.4

http://stm32flash.googlecode.com/

Interface serial_w32: 57600 8E1
Version      : 0x22
Option 1     : 0x00
Option 2     : 0x00
Device ID    : 0x0410 (Medium-density)
- RAM        : 20KiB  (512b reserved by bootloader)
- Flash      : 128KiB (sector size: 4x1024)
- Option RAM : 16b
- System RAM : 2KiB
Memory read
Read address 0x08020000 (100.00%) Done.
```
### Restoring the STM32 EEPROM
The syntax of the restore command is:
`stm32flash -w [file name] -e 0 -v -S 0x801F800:2048 [serial port]`

Again, the `-S 0x801F800:2048` option tells stm32flash to write 2048 bytes starting at 0x801F800.  Additionally `-e 0` tells the tool not to erase any other blocks, which will preserve the rest of the data in the module's memory.

Windows example:

`stm32flash.exe -w C:\Temp\eeprom.bin -e 0 -v -S 0x801F800:2048 COM4`

Linux and macOS example:

`stm32flash -w /tmp/eeprom.bin -e 0 -v -S 0x801F800:2048 /dev/ttyUSB0`

Output will look similar to this:
```
stm32flash 0.4

http://stm32flash.googlecode.com/

Interface serial_w32: 57600 8E1
Version      : 0x22
Option 1     : 0x00
Option 2     : 0x00
Device ID    : 0x0410 (Medium-density)
- RAM        : 20KiB  (512b reserved by bootloader)
- Flash      : 128KiB (sector size: 4x1024)
- Option RAM : 16b
- System RAM : 2KiB
Write to memory
Wrote and verified address 0x08020000 (100.00%) Done.
```

### Erasing the STM32 EEPROM
The syntax of the erase command is:
`stm32flash -o -S 0x801F800:2048 [serial port]`

Again, the `-S 0x801F800:2048` option tells stm32flash to erase 2048 bytes starting at 0x801F800.

Windows example:

`stm32flash.exe -o -S 0x801F800:2048 COM4`

Linux and macOS example:

`stm32flash -o -S 0x801F800:2048 /dev/ttyUSB0`

Output will look similar to this:
```
stm32flash 0.4

http://stm32flash.googlecode.com/

Interface serial_w32: 57600 8E1
Version      : 0x22
Option 1     : 0x00
Option 2     : 0x00
Device ID    : 0x0410 (Medium-density)
- RAM        : 20KiB  (512b reserved by bootloader)
- Flash      : 128KiB (sector size: 4x1024)
- Option RAM : 16b
- System RAM : 2KiB
Erasing flash
```

## Atmega328p Module
The EEPROM on the Atmega328p module is a dedicated 1KB data space, separate from the main flash memory.  

By default the EEPROM would be erased every time the module is flashed, but we configure the `EESAVE` bit so that the EEPROM is not erased during flashes.  This is one reason why it is crucial to set the 'fuses' on a new module using the **Burn Bootloader** command in the Arduino IDE, as described in the [documentation](Compiling.md#burn-bootloader-and-set-fuses).

The module's EEPROM can be read, written, and erased using the avrdude tool.

#### Tools needed
* A USBasp device, or another Arduino programmed to function as a USBasp
* avrdude - installed as part of the Arduino IDE installation, or [downloaded separately](http://savannah.nongnu.org/projects/avrdude)

With a default Arduino IDE installation, the path to avrdude will be:
* Windows - `C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe`
* Linux - `[Arduino IDE path]/hardware/tools/avr/bin/avrdude`
* macOS - TBD

You will also need to know the path to the avrdude configuration file.  Default locations are:
* Windows - `C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf`
* Linux - `[Arduino IDE path]/hardware/tools/avr/etc/avrdude.conf`
* macOS - TBD

#### Preparation
Connect the module using the USBasp.

### Backing up the Atmega328p EEPROM
The syntax of the backup command is:
`avrdude -C [config file] -c usbasp -p atmega328p -U eeprom:r:[filename]:i`

Windows example:

`"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -C "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -c usbasp -p atmega328p -U eeprom:r:C:\Temp\eeprom.hex:i`

Linux and macOS example:

`~/Downloads/arduino-1.8.5/hardware/tools/avr/bin/avrdude -C ~/Downloads/arduino-1.8.5/hardware/tools/avr/etc/avrdude.conf -c usbasp -p atmega328p -U eeprom:r:/tmp/eeprom.hex:i`

Output will look similar to this:
```
avrdude.exe: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.10s

avrdude.exe: Device signature = 0x1e950f (probably m328p)
avrdude.exe: reading eeprom memory:

Reading | ################################################## | 100% 7.51s

avrdude.exe: writing output file "C:\Temp\eeprom.hex"

avrdude.exe: safemode: Fuses OK (E:FD, H:D6, L:FF)

avrdude.exe done.  Thank you.
```
### Restoring the Atmega328p EEPROM
The syntax of the restore command is:
`avrdude -C [config file] -c usbasp -p atmega328p -U eeprom:w:[filename]:i`

Windows example:

`"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -C "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -c usbasp -p atmega328p -U eeprom:w:C:\Temp\eeprom.hex:i`

Linux and macOS example:

`~/Downloads/arduino-1.8.5/hardware/tools/avr/bin/avrdude -C ~/Downloads/arduino-1.8.5/hardware/tools/avr/etc/avrdude.conf -c usbasp -p atmega328p -U eeprom:w:/tmp/eeprom.hex:i`

Output will look similar to this:
```
avrdude.exe: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.04s

avrdude.exe: Device signature = 0x1e950f (probably m328p)
avrdude.exe: reading input file "C:\Temp\eeprom.hex"
avrdude.exe: writing eeprom (1024 bytes):

Writing | ################################################## | 100% 17.17s

avrdude.exe: 1024 bytes of eeprom written
avrdude.exe: verifying eeprom memory against C:\Temp\eeprom.hex:
avrdude.exe: load data eeprom data from input file C:\Temp\eeprom.hex:
avrdude.exe: input file C:\Temp\eeprom.hex contains 1024 bytes
avrdude.exe: reading on-chip eeprom data:

Reading | ################################################## | 100% 6.51s

avrdude.exe: verifying ...
avrdude.exe: 1024 bytes of eeprom verified

avrdude.exe: safemode: Fuses OK (E:FD, H:D6, L:FF)

avrdude.exe done.  Thank you.
```

### Erasing the Atmega328p EEPROM
It's not possible to simply erase the EEPROM so instead we write a file which overwrites all of the content with `0xFF`.  Download the 'erase.hex' file [here](https://raw.githubusercontent.com/pascallanger/DIY-Multiprotocol-TX-Module/master/docs/erase.hex).

The syntax of the 'erase' command is the same as the restore command.

Windows example:

`"C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe" -C "C:\Program Files (x86)\Arduino\hardware\tools\avr\etc\avrdude.conf" -c usbasp -p atmega328p -U eeprom:w:C:\Temp\erase.hex:i`

Linux and macOS example:

`~/Downloads/arduino-1.8.5/hardware/tools/avr/bin/avrdude -C ~/Downloads/arduino-1.8.5/hardware/tools/avr/etc/avrdude.conf -c usbasp -p atmega328p -U eeprom:r:/tmp/erase.hex:i`

Output will look similar to this:
```
avrdude.exe: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.04s

avrdude.exe: Device signature = 0x1e950f (probably m328p)
avrdude.exe: reading input file "C:\Temp\erase.hex"
avrdude.exe: writing eeprom (1024 bytes):

Writing | ################################################## | 100% 17.17s

avrdude.exe: 1024 bytes of eeprom written
avrdude.exe: verifying eeprom memory against C:\Temp\erase.hex:
avrdude.exe: load data eeprom data from input file C:\Temp\erase.hex:
avrdude.exe: input file C:\Temp\erase.hex contains 1024 bytes
avrdude.exe: reading on-chip eeprom data:

Reading | ################################################## | 100% 6.51s

avrdude.exe: verifying ...
avrdude.exe: 1024 bytes of eeprom verified

avrdude.exe: safemode: Fuses OK (E:FD, H:D6, L:FF)

avrdude.exe done.  Thank you.
```
