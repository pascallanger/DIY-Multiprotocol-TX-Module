# Flashing from the Transmitter

For radios running ersky9x r221e2 or newer, there is an option to flash a precompiled firmware file to the multiprotocol module using the transmitter's Maintenance Mode.

## Tools required
* A compatible transmitter running ersky9x r221e2, or newer
* A precompiled multiprotocol firmware file (.hex for Atmega328p or .bin for STM32)
* A **Flash from TX** bootloader installed on the multiprotocol module
* A means to get the firmware file onto the transmitter's SD card

Consult the [ersky9x site](http://www.er9x.com/) to see if your transmitter is compatible.

The transmitter firmware can be downloaded from the [ersky9x test firmware page](http://openrcforums.com/forum/viewtopic.php?f=7&t=4676).

## Procedure
1. Either:
   1. Connect the transmitter using a USB cable and power it on, or 
   1. Remove the SD card from the transmitter and mount it using a suitable reader
1. Copy the pre-compiled firmware file into the **\firmware** folder of the SD card (create the folder if it does not exist)
1. Power the transmitter off and remove the USB cable or put the SD card back in the transmitter
1. Enter the transmitter's Maintenance Menu by powering it on with the outer buttons of the two horizontal trims held down
1. Select **Update Multi**,
1. Choose the appropriate file type
   1. **HEX** to update an Atmega328p module
   1. **BIN** to update an STM32 module
1. Select **Update**
1. Choose the firmware file to flash, long press to select it
1. Long press again to flash the selected file to the module

When flashing has finished, long press EXIT to reboot in normal mode.

## Troubleshooting
TBD
