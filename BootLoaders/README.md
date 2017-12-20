# Arduino IDE board definitions for Multi 4-in-1
Board definitions are available for the Atmega328p, STM32, and OrangeRX modules.  The board definitions make it easier to compile and install the multiprotocol module firmware.

**Note:** The Orange RX module is now included in the **Multi 4-in-1 AVR Boards** package, it no longer has a dedicated package.  If you have the dedicated OrangeRX package installed you should remove it and install the most recent AVR package (v1.0.2 or newer).

## Installing
The board definitions are installed using the Arduino IDE Boards Manager.

1. Open the Arduino IDE

2. Go to **File -> Preferences**, or press Ctrl+Comma

3. Locate the **Aditional Boards Manager URLs** field and paste in this URL: `https://raw.githubusercontent.com/pascallanger/DIY-Multiprotocol-TX-Module/master/BootLoaders/package_multi_4in1_board_index.json`

**Note:** Multiple URLs are comma-separated.

<p align="center">
  <img src="/docs/images/ide-prefs.jpg">
</p>

4. Click **OK** to save the change

5. Click **Tools -> Board -> Boards Manager**

6. Type **multi** into the search box to see the Multi 4-in-1 boards
<p align="center">
  <img src="/docs/images/multi-boards.jpg">
</p>

7. Click on the board you require and click the **Install** button.  Repeat for both boards, if required.  If you are installing the STM32 board for the first time, and you have not installed any STM32 or SAMD boards before (such as Arduino Due or Zero) the dependency toolchain will also be downloaded and installed.

7. Click **Close** to close the Boards Manager

## Verify the boards are installed
1. Click **Tools -> Board**
2. Scroll down the list to the **Multi 4-in-1** board headings verify that the boards you installed are available:
    ![Image](/docs/images/boards-menu.jpg)
    
    * **Multi 4-in-1 (STM32F103CB)** for the STM32 module
    * **Multi 4-in-1 (Atmega328p, 3.3V, 16MHz)** for the Atmega module
    * **Multi 4-in-1 (OrangeRX)** for the OrangeRX module

## Compiling and Uploading
Refer to the hardware-specific pages for information on compiling the firmware and uploading it to the multiprotocol module:

* [Compiling for Atmega](/docs/Compiling.md)
* [Compiling for STM32](/docs/Compiling_STM32.md)
* [Compiling for OrangeRX](/docs/Compiling_OrangeTx.md)
