# Arduino IDE board definition for Multi 4-in-1 transmitter module
At this point the only supported board/module is the Atmega328p-based module, the STM32-based module is not yet supported.

## Installing
The board definition is installed using the Arduino IDE Boards Manager.

1. Open the Arduino IDE
2. Go to File->Preferences (or Ctrl+Comma)
3. Locate the 'Aditional Boards Manager URLs' field and paste in this URL: https://raw.githubusercontent.com/pascallanger/DIY-Multiprotocol-TX-Module/tree/master/BootLoaders/package_multi_4in1_board_index.json
4. Click OK to save the change
5. Click Tools->Board [Board Name]->Boards Manager
6. Scroll to the bottom of the list of boards and click on 'Multi 4-in-1 Boards' then click the Install button
7. Click Close to close the Boards Manager

## Selecting the board
1. Click Tools->Board [Board Name]
2. Scroll down the list to 'Multi 4-in-1 Boards' and select 'Multi 4-in-1 (Atmega328p, 3.3V, 16MHz)'

## Choosing the bootloader
There are two bootloader options.  The default 'No bootloader' won't install a bootloader, allowing the maximum space for protocols.  The 'Flash from TX' option installs a small Optiboot bootloader which is compatible with Ersky9x and OpenTX option to flash firmware from the transmitter's maintenance menu.

1. Click Tools->Bootloader [Bootloader Option]
2. Select the desired bootloader

**Recommended:** Click Tools->Burn Bootloader to set the fuses, even if the 'No bootloader' option was selected.

## Compiling / uploading firmware with the 'Flash from TX' bootloader
1. Follow the normal compiling method.
2. Copy the compiled .hex file (usually Multiprotocol.ino.hex) in the "/firmware" directory of the SD card.
3. Start Ersky9x in "Maintenance Mode" by holding the horizontal trims APART while powering on the transmitter.
4. Select "Update Multi", then change "File Type" to "HEX", then select "Update", choose the firmware to flash, long pressto select it and long press to flash it.
5. When finished, long press EXIT to reboot in normal mode.
