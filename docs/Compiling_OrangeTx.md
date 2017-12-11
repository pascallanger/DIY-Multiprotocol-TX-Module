# Compiling and Programming (OrangeRX)

Multiprotocol firmware is compiled using the Arduino IDE.  The guide below will walk you through all the steps to compile and upload your customized firmware.

**These instructions are for the OrangeRX version of the Multiprotocol module.**  If you are compling for a different module please go to the dedicated page [Atmega328p](Compiling.md) or [STM32](Compiling_STM32.md) page.

## Index
1. [Tools Required](#tools-required)
1. [Preparation](#preparation)
   1. [Install the Arduino IDE](#install-the-arduino-ide)
   1. [Download the Multiprotocol source and open the project](#download-the-multiprotocol-source-and-open-the-project)
   1. [Install the Multi 4-in-1 board](#install-the-multi-4-in-1-board)
   1. [Configure the Arduino IDE](#configure-the-arduino-ide)
1. [Configure the firmware](#configure-the-firmware)
   1. [Customize the firmware to match your hardware and your needs](#customize-the-firmware-to-match-your-hardware-and-your-needs)
   1. [Verify the firmware](#verify-the-firmware)
1. [Compiling and uploading the firmware](#compiling-and-uploading-the-firmware)
   1. [Upload the firmware](#upload-the-firmware)
      1. [Flash from TX](#flash-from-tx)
1. [Troubleshooting](#troubleshooting)

## Tools required
Flashing the bootloader to the OrangeRX module requires either a PPI-capable programmer or an Arduino Pro Mini programmed with a sketch which can program the bootloader.

Flashing the bootloader is outside the scope of this documentation.  Full instructions are available [here](http://openrcforums.com/forum/viewtopic.php?f=40&t=8753&sid=bbd74327cc518303e1c7f9e2ace04339#p114549).

## Preparation
### Install the Arduino IDE
1. Download and install the Arduino IDE. The currently supported Arduino version is 1.8.5, available for [Windows]( https://www.arduino.cc/download_handler.php?f=/arduino-1.8.5-windows.exe) and [Mac OSX](https://www.arduino.cc/download_handler.php?f=/arduino-1.8.5-macosx.zip)
1. It is recommended to upgrade Java to the [latest version](https://www.java.com/en/download/)

### Download the Multiprotocol source and open the project
1. Either
   1. Download the zip file with the Multiprotocol module source code from [here](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/archive/master.zip) and unzip and copy the source code folder **Multiprotocol** to a location of your choosing, or
   1. Clone the project using Git or Github Desktop, then
1. Double-click the **Multiprotocol.ino** file in the **Multiprotocol** folder to open the project in the Arduino IDE

### Install the Multi 4-in-1 board
1. Follow [these instructions](/BootLoaders/README.md) to install the **Multi 4-in-1 OrangeRX Board** in the Arduino IDE

### Configure the Arduino IDE
1. Under **Tools -> Board** select **'Multi 4-in-1 (OrangeRX)'**

## Configure the firmware
### Customize the firmware to match your hardware and your needs
All customization is done by editing the ```_Config.h  ``` file in the Multiprotocol Arduino project.  

The OrangeRX module has more than enough flash space for all the compatible protocols so, unlike the Atmega328p-based module, it is not necessary to disable unused protocols.

You can still disable protocols if you wish, and you may also enable or disable other optional Multiprotocol features.

### Verify the firmware
To check that the program will compile correctly and fit in the Atmega click **Sketch -> Verify/Compile**, or press **Ctrl+R**.

If there are no errors and you see output like this:
```
Sketch uses 31874 bytes (97%) of program storage space. Maximum is 32768 bytes.
Global variables use 1083 bytes (52%) of dynamic memory, leaving 965 bytes for local variables. Maximum is 2048 bytes.
```
You can proceed to the next step.

## Compiling and uploading the firmware

### Upload the firmware
You are now ready to upload the firmware to the multiprotocol module.  Uploading is done via the 'Flash-from-TX' method.

**Note:** 'Flash from TX' is only available with radios running ersky9x r221e2 or newer.

#### Flash from TX
1. In the Arduino IDE click **Sketch -> Export compiled Binary**, or press **Ctrl+Alt+S**
1. Locate the file named **multi-orx-[version].hex** in the **Multiprotocol** folder
1. Follow the instructions [here](/docs/Flash_from_Tx.md) to upload the firmware using your radio

