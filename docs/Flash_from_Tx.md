# Flashing from the Transmitter

For radios running ersky9x and OpenTX, there is an option to flash a precompiled firmware file to the multiprotocol module using the transmitter's Bootloader mode.

## Tools required
* A compatible transmitter running an ersky9x bootloader v2.9 or newer. This is true for both OpenTX and ersky9x.
* A precompiled multiprotocol firmware file (.hex for Atmega328p or .bin for STM32)
* A **Flash from TX** bootloader installed on an Atmega328p or STM32 multiprotocol module
* A means to get the firmware file onto the transmitter's SD card

## Radio bootloader and apps

### How to check the bootloader version
1. Push both horizontals trims inwards (close to each others) while powering on the radio
1. The screen title should indicate `Boot Loader V2.9Ready` or newer
1. Launch the `FlashMulti_xxx.app` app from the `Run App` menu
1. The App version at the bottom right of the screen should be `28.Aug.18` or newer
1. If everything is correct you are ready to upgrade the Multimodule firmware

### Upgrade the bootloader and install app(s)
1. Download the latest zip file of the [ersky9x firmware](https://openrcforums.com/forum/viewtopic.php?f=7&t=4676)
1. Extract the .bin file corresponding to your radio in your SD card `\FIRMWARE` directory
1. Download the latest [Flash Multiprotocol Module app](http://www.er9x.com/Ersky9xapps.html) for your radio
1. Copy the .app file in a folder called `APPS` at the root of the SD card (if the directory does not exist create it)
1. For ersky9x
   1. Power on the radio in maintenance mode while pushing both horizontals trims outwards (away from each others)
   1. Select Upgrade Bootloader
   1. Select the ersky9x firmware matching your radio
   1. Long press it and select `Flash bootloader`
1. For OpenTX
   1. Boot the radio normaly
   1. Go in the RADIO SETUP menu page 2 called SD-HC CARD
   1. Open the FIRMWARE directory
   1. Select the ersky9x firmware matching your radio
   1. Long press it and select `Flash bootloader`
1. Check by rebooting the radio in bootloader mode that everything is [ok](###-How-to-check-the-bootloader-version)

## Multimodule upgrade procedure
1. Either:
   1. Connect the transmitter using a USB cable and power it on, or 
   1. Remove the SD card from the transmitter and mount it using a suitable reader
1. Copy the pre-compiled firmware file into the `\FIRMWARE` folder of the SD card (create the folder if it does not exist)
1. Power the transmitter off and remove the USB cable or put the SD card back in the transmitter
1. Push both horizontals trims inwards (close to each others) while powering on the radio
1. The screen title should indicate `Boot Loader V2.9Ready` or newer
1. Launch the `FlashMulti_xxx.app` app from the `Run App` menu
1. Choose the appropriate file type
   1. `HEX` to update an Atmega328p module
   1. `BIN` to update an STM32 module
1. Select `Update`
1. Choose the firmware file to flash, long press to select it
1. Long press again to flash the selected file to the module
1. When flashing has finished, long press EXIT to reboot in normal mode
1. If the flashing procedure fails try to redo with the process `Invert Com Port` enabled
