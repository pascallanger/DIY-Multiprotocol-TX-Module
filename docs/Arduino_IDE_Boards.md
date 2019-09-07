# Arduino IDE board definitions for Multi 4-in-1
Board definitions are available for the Atmega328p, STM32, and OrangeRX modules.  The board definitions make it easier to compile and install the multiprotocol module firmware.

**Note:** The Orange RX module is now included in the **Multi 4-in-1 AVR Boards** package, it no longer has a dedicated package.  If you have the dedicated OrangeRX package installed you should remove it and install the most recent AVR package (v1.0.2 or newer).

## Installing
The board definitions are installed using the Arduino IDE Boards Manager.

1. Open the Arduino IDE

2. Go to **File -> Preferences**, or press Ctrl+Comma

3. Locate the **Aditional Boards Manager URLs** field and paste in this URL: `https://raw.githubusercontent.com/pascallanger/DIY-Multiprotocol-TX-Module-Boards/master/package_multi_4in1_board_index.json`

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

## Install device drivers

### Windows 7 or newer:
1. If you haven't already done so, clone or download and unpack the Multiprocol source
1. Open the folder where you unzipped or cloned the Multiprotocol project
1. Browse to **\BootLoaders\Boards\Windows**
1. Run **install-drivers.bat**
1. Follow the prompts to install the two drivers

### Windows XP or older
1. Download and install the legacy Windows XP drivers from [here](https://github.com/rogerclarkmelbourne/Arduino_STM32/tree/master/drivers/win/win_xp_legacy)

**NOTE:** If you have installed the drivers and your module is not detected as a Maple device it most likely does not have a USB bootloader installed. Ready-made modules from Banggood **do not** come with a USB bootloader installed.  You will need to follow the procedure to upload using a USB-to-serial adapter one time before you can upload firmware using the USB port.

### Jumper JP4IN1 drivers
The driver for the Jumper JP4IN1 module, the Silicon Labs CP210x driver, can be downloaded from here: https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

### Other USB-to-serial device drivers
Other drivers may be needed if you are using an external USB-to-serial adapter. Consult the documentation for your adapter.

Windows 10 includes drivers for many common serial devices, including many USB-to-serial adapters, so check Device Manager to see if your device is recognised.

### Mac OS X
Uploading via USB requires the [libusb library](https://libusb.info/) to be installed.  The easiest way to install the library is using the [Homebrew package manager for macOS](https://brew.sh/) by executing the two lines given below in a Terminal.

Install Homebrew:

    `/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

Once Homebrew is installed, use it to install libusb:

    `brew install libusb`

### Linux
Permissions must be configured to allow access to serial devices.
1. If you haven't already done so, clone or download and unpack the Multiprocol source
1. Open a Terminal and change to the directory where you have cloned or unzipped the Multiprotocol source
1. Run the following commands:

    ```
    sudo cp -v BootLoaders/Boards/Linux/45-maple.rules /etc/udev/rules.d/45-maple.rules
    sudo chown root:root /etc/udev/rules.d/45-maple.rules
    sudo chmod 644 /etc/udev/rules.d/45-maple.rules
    sudo udevadm control --reload-rules
    sudo usermod -a -G plugdev $USER
    sudo usermod -a -G dialout $USER
    ```

## Compiling and Uploading
Refer to the hardware-specific pages for information on compiling the firmware and uploading it to the multiprotocol module:

* [Compiling for Atmega](/docs/Compiling.md)
* [Compiling for STM32](/docs/Compiling_STM32.md)
* [Compiling for OrangeRX](/docs/Compiling_OrangeTx.md)
