# Compliling and Programming (STM32)

**If you are Compling for the Arduino ATmega328p version of the Multiprotocol Module please go to the dedicated [Compiling and Programming ATmega328](Compiling.md) page.**

Multiprotocol source can be compiled using the Arduino IDE using STM32 Core (Maple) and Arduino ARM-Cortex-M3 libraries.  

###Install the Arduino IDE and the Multiprotocol project
1. Download the Arduino IDE. The currently supported Arduino version is 1.6.11 available for [Windows]( https://www.arduino.cc/download_handler.php?f=/arduino-1.6.12-windows.exe) and [Mac OSX](http://arduino.cc/download_handler.php?f=/arduino-1.6.12-macosx.zip)
1. Download the [STM32 Core](https://github.com/rogerclarkmelbourne/Arduino_STM32/archive/master.zip) and copy the Arduino_STM32 folder to:
  - OSX: ```Arduino.app/Contents/Java/hardware```  (you can open Arduino.app by Ctl Clicking on Arduino.app and selecting "Show Package Contents") 
  - Windows: ```C:\Program Files (x86)\Arduino\hardware``` 
1. Download the zip file with the Multiprotocol module source code from [here](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module)
1. Unzip and copy the source code folder ```Multiprotocol``` to a folder of your choosing
1. Click on the ```Multiprotocol.ino file``` in the ```Multiprotocol``` folder and the Arduino environment should appear and the Multiprotocol project will be loaded.

###Prepare the Arduino IDE:

1. In order to compile successfully you need also to modify a maple library file. In ```....\hardware\Arduino_STM32\STM32F1\cores\maple\libmaple\usart_f1.c``` comment out the 2 functions as shown below. This is required to have low-level access to the USART interrupt. 

 > ```//void __irq_usart2(void) { usart_irq(&usart2_rb, USART2_BASE); } ``` 
 
 > ```//void __irq_usart3(void) { usart_irq(&usart3_rb, USART3_BASE); } ```  
1. Run the IDE, and on the **Tools** menu, select **Board** and then **Boards manager**. Click on the Arduino DUE (32 Bits ARM-Cortex M3) from the list of available boards. You must do this step, it installs the arm-none-eabi-g++ toolchain!
1. Close and reopen the Arduino IDE and load the Multiprotocol project.
1. In arduino IDE under the **Tools** -> **Board:** select the **Generic STM32F103C series** board
1. Click on the **Verify** button to test compile the before you make any changes.  If there are errors check the process above and be sure to have the right version of the Arduino IDE.


## Common process for OSX and Windows

###Customize the firmware to your hardware and your needs
On all modules with STM32F103 microcontroller, the program flash memory on the microcontroller is large enough to accommodate all the protocols.  You do not have to make choices on which protocols to upload.  Also, it is likely that you used the Banggood 4-in-1 RF module and you will therefore have access to all the RF modules.  However, you can follow these instructions to select only a subset protocols. 

If you plan to use the PPM mode then you should follow the instructions to customize the protocol selection switch to protocol mapping.  

Before customizing your firmware it would be good to review the protocol on the [Protocol Details](../Protocols_Details.md) page and to identify the protocols you would like to support on your module.  

At the same time make a note of RF modules required by your protocols.  For example, if you do not wish to use the FlySky or the Husan protocols then you do not need to compile support the the A7105 RF Module into your firmware.  Similarly, if you have no need to bind with ASSAN RC receivers then you do not need to compile the ASSAN protocol into your firmware. 

If you plan to use the PPM communication interface with your transmitter, then you need to perform protocol selection with the 16 position switch on your module.  This will limit the available protocols you can usefully access in PPM mode on your module to 15 (this limitation does not apply to Serial mode).  You should make a list of your 15 chosen protocols, sub protocols and options like this:

Switch Position|Protocol|Sub-Protocol|Option|Notes
---------------|--------|------------|------|-----
1.|DSM|DSM2|2|6 channels @ 22ms
2.|DSM|DSMX|6|6 channels @ 11ms
....|...|...|...|...
....|...|...|...|...
15.|FRSKYX|CH_16| |FrSky X receiver 16 chan


With the above information (required RF modules, selected protocols and 16 pos switch mapping) you are ready to customize your firmware.  

All customization is done by editing the ```_Config.h  ``` file in the Multiprotocol Arduino project.  

In the Arduino IDE and click on the down arrow on the far right of the tab bar to show a list of project files (see the red circle on the screenshot below).  Scroll down and select the _Config.h file.
<img src="images/Arduino.png" width="600" height="400" />

It is unlikely that you would need to do this, but you can comment out any of the RF modules that you do not need by typing ```// ``` at the begining of the line that reads : 
```#define <RF Module name>_INSTALLED ``` .  The following line shows the CC2500 module removed 

> ```#define A7105_INSTALLED ```

> ```#define CYRF6936_INSTALLED ```

> **```//#define CC2500_INSTALLED ```**

> ```#define NFR24L01_INSTALLED ```

Again it is unlikely that you would want to do this, but you can scroll down to the available protocols and comment out all the protocols you will not require.  The following example shows the DEVO protocol commented out.

> **```#ifdef	CYRF6936_INSTALLED ```

> **``` //	#define	DEVO_CYRF6936_INO ```**

> ``` 	#define	DSM_CYRF6936_INO ```

> ```	#define J6PRO_CYRF6936_INO ```

> ``` #endif ```**

Look for the line containing ```#define INVERT_TELEMETRY``` and make sure that it is uncommented: 
> ```#define INVERT_TELEMETRY ``` 

 Scroll down to the bottom of the file and list your switch mapping to your desired **protocol/sub-protocol/options**.  You typically only need to change the three relevant columns.  On models that require a rebind on every start-up (like Syma quads) you can change the **```NO_AUTOBIND ```** to **```AUTOBIND ```**.

Finally, if you have not already done so, specify the correct board for the compiler.  Under **Tools** -> **Board:** select the  **Generic STM32F103C series** board.   You can now compile the firmware by clicking on the check mark (Tooltip: Verify) on the menu bar.  If everything goes according to plan you should see something like the following line in the lower pane of the window:

> Sketch uses 32,464 bytes (99%) of program storage space. Maximum is 32,768 bytes.
> Global variables use 1,219 bytes (59%) of dynamic memory, leaving 829 bytes for local variables. Maximum is 2,048 bytes.

If you get an error carefully read the error to see the approximate line number where the error occured and correct it. 

###Preparing for STM32 microcontroller for firmware flashing

There are three options for flashing the firmware.  The first (and strongly recommended) is flashing it while it is plugged into and powered by the transmitter.  The second is flashing it out of the transmitter (the power is supplied by the 3.3V FTDI cable).  The second option is very risky because if the 3.3V bridge jumper is not removed after flashing it will fry your RF module - **you have been warned**.  The third is preparing the board for flashing with a USB cable. 

The third method is definitely the easiest in the long-term, but it does require setting up the bootloader on the STM32 MCU.

####Option 1: Flashing with Tx power(highly recommended)

1. Put the module in the Tx 
1. Place a jumper over the BOOT0 pins 
1. Connect your 3.3V/5V FTDI cable (USB - TTL serial) to  Multiprotocol serial port.  Connect only RX, TX and GND.  **Do not connect the 5V or 3.3V between the FTDI cable and the module - the power will be supplied by the transmitter**.  Connect the pins as follows:   
  - Module RX pin to FTDI TX pin
  - Module TX pin to FTDI Rx pin
  - Module GND to FTDI GND   
1. In arduino IDE under the **Tools** -> **Board:** check that you have selected the **Generic STM32F103C series** board 
1. Under **Tools** -> **Upload Method:** select **Serial** 
1. Click "Upload" and the sketch will be uploaded normally.   This is valid for  all arduino versions. 
1. Once the firmware has uploaded, remove the BOOT0 jumper. 

If you have the module inside a box and to be inserted in TX bay, you may build a flashing cable like in the picture below.
You can attach and solder a 5 pin header female and top outside the box.**ALways insert first the USB serial device in USB port , and TX start after.**

[<img src="images/Multi_STM32_ flashing.jpg" />]

See below my module for reference

[<img src="images/Multi_STM32 module.JPG"  width="600" height="400" />]


####Option 2: Flashing without Tx power

The key difference of this method is that the 3.3V FTDI cable must also provide power to the 5V circuitry during the flashing process.  To do this, a jumper must be enabled connecting the 3.3V VCC to the 5V line.  

**If the module is powered through the transmitter and this jumper is enabled, then it will feed 5V throughout the 3.3V circuit and this will fry your RF modules.  Do not plug the module into the transmitter before removing this jumper!**  

1. Remove the module from the transmitter bay
1. Set BOOT0 jumper 
1. Set the 3.3V jumper. 
1. Connect your 3.3V FTDI cable (USB - TTL serial) to  Multiprotocol serial port (RX,TX,GND,5V).  Connect the pins as follows:   
  - Module RX pin to FTDI TX pin
  - Module TX pin to FTDI Rx pin
  - Module GND to FTDI GND  
  - Module V to FTDI 3.3V
1. In arduino IDE under the **Tools** -> **Board:** check that you have selected the **Generic STM32F103C series** board 
1. Under **Tools** -> **Upload Method:** select **Serial**. 
1. Click "Upload" and the sketch will be uploaded normally.
1. Once the firmware has uploaded:   
   - Remove the 3.3V jumper!!!! 
   - Remove the BOOT0 jumper
   - Check that you removed the 3.3V jumper
1. Insert the module into the transmitter bay

####Option 3: Flashing with USB cable.

This method use USB connector on the STM32 V1.0 board or on the maple clone board.  

1. Install first maple USB driver by running the batch file found in Arduino STM32 package folder "..\hardware\Arduino_STM32\drivers\win\install_drivers.bat"  
1. Download the free STM32 flash loader demonstrator from [ST.com](http://www.st.com/en/development-tools/flasher-stm32.html) and using a USB-TTL device (like FTDI cable) flash the STM32duino bootloader available from Roger Clark's great STM32 site [here](https://github.com/rogerclarkmelbourne/STM32duino-bootloader/tree/master/STM32F1/binaries)  
1. In Arduino IDE under "Upload method" select STM32duino-bootloader.
1. After that select the correct serial port and and upload sketches normally in Arduino using USB port

###Flashing binary file:  
If you want to flash a pre-compiled binary file (like the Release .bin files) you need specialized software and the FTDI cable.  

1. Set BOOT0 jumper  
1. Connect your 3.3V FTDI cable (USB - TTL serial) to  Multiprotocol serial port (RX,TX,GND,5V)  
1. The other steps regarding power supply the same as previous recommandation regarding jumpers  
For uploading binaries(.bin files) there is a specialized software you need to install on your computer.  

#### Windows:
Download the ST Flash Loader Demonstrator from here: http://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/flasher-stm32.html

Run the ST Flash Loader program. There are many tutorials on the web on how to use this program.For example

[here](http://www.scienceprog.com/flashing-programs-to-stm32-embedded-bootloader)

#### OSX:
To be checked.

###Report issues for the STM32 board
You can report your problem using the [GitHub issue](https://github.com/midelic/DIY-Multiprotocol-TX-Module/issues) system or go to the [Main thread on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676) to ask your question.
Please provide the following information:

- Multiprotocol code version
- STM32 version
- TX type
- Using PPM or Serial, if using er9x or ersky9x the version in use
- Different led status (multimodule and model)
- Explanation of the behavior and reproduction steps
