# Enable the STM32 module serial debug feature

To enable serial debug on your module you must know how to buid the firmware from the source code available on this GitHub. To do so follow this page: [Compiling and programming the STM32 module](Compiling_STM32.md).

Procedure to use serial debug:
1. Edit the file [Multiprotocol.ino](../Multiprotocol/Multiprotocol.ino#L26)
1. Modify at the begining of the file the line: `//#define DEBUG_SERIAL` by removing the // leaving only: `#define DEBUG_SERIAL`
1. Save the file
1. Power on the TX
1. Open in the Arduino IDE the Serial Monitor: Tools->Serial Monitor or Ctrl+Shift+M<br> <img src="images/Serial_Monitor_1.png" />
1. Make sure the settings at the bottom of the Serial Monitor window are the same as the picture above especially the baud rate set to 115200 baud
1. Upload the firmware to the module as you usually do with the Arduino IDE and **do not disconnect the USB cable or FTDI**
1. The Serial Monitor window should show the module booting and more depending on the protocol currently loaded<br> <img src="images/Serial_Monitor_2.png" />
1. At this stage you can test whatever is needed or have been instructed to do. You can easily select text in the window to copy and paste it on the forum or in a text file.
1. **Important:** to use your module normally and before flying you must reupload the firmware with the debug line commented: `//#define DEBUG_SERIAL`
