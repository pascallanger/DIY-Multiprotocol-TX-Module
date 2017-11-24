# erSky9X family of transmitters
This page is relevant to the following transmitters:  
 - Taranis running erSky9X (for the Taranis running OpenTx see [here](Tx-Taranis.md))
 - Turnigy 9XR Pro (for Turnigy 9X see [here](Tx-FlyskyTH9X.md))
 - A variety of replacement mother boards for FlySky transmitters:  
    - SKY board
    - AR9X board
    - 9Xtreme board
    - AR9X UNI board


## Features
The MULTI-Module can be used in this family of transmitters in either PPM mode or in Serial mode.  To operate in Serial mode, a version of erSky9X supporting the Multiprotocol Module must be installed on the Tx. 

## PPM Mode
Please refer to the [PPM Setup](PPM_Setup.md) page. 


## Serial Mode
Serial mode is only supported by the erSky9X firmware.  Loading this firmware is beyond the scope of this document but it is well covered in tutorial and video tutorials online. The firmware you should use with the MULTI-Module is available on Mike's [erSky9X Test Version Page](http://openrcforums.com/forum/viewtopic.php?f=7&t=4676).  Analyze the names of the different firmware options carefully - they contain all the information requried. (For example for the Taranis X9D Plus use the x9dp.bin firmware, etc.)
 
erSky9X is well documented, the slightly outdated erSky9X documentation is [here](http://openrcforums.com/forum/viewtopic.php?f=5&t=6473#p90349).  You may find the new Er9x manual very helpful in understanding Ersky9x as the two firmwares are very closely related. It provides more detailed explanations and numerous relevant programming examples and is available [here](http://openrcforums.com/forum/viewtopic.php?f=5&t=6473#p90349).

### Enabling Serial Mode
1. Confirm that the MULTI-Module has the required physical connections between the pins on the back of the Tx and the ATMega328 microprocessor.  This may require some soldering and depends on which version of the MULTI-Module you have.  Check out your module’s hardware page under the section **Enabling your module for Serial** for details. Click on the image that corresponds to your MULTI-Module on the [hardware](Hardware.md) page. 
1. Plug in your MULTI-Module into the transmitter module bay.  If you have a rotary protocol selection switch, turn the switch to position 0 to put the unit into Serial mode.  
1. Ensure throttle is down and all switches are in the start position and power up the Tx.  The red LED on the MULTI-Module should be flashing with a period of about 1 second indicating that it has not established a valid serial link with the Tx.  This is expected as we have not set up the Tx yet.
1.  Create a new model and check that the channel order is AETR ( **This is really important - this is for all protocols - even for DSM as the MULTI-module firware will change the transmitted channel order according to the protocol.**)  
1. In the Model Setup menu scroll down to the **Protocol** submenu and change the RF settings to MULTI.  This should reveal a set of additional options (like Protocol and Options) 
1. The red LED on the MULTI-Module should briefly flash and then remain solid.  This confirms that the MULTI-Module has established serial communication with the Tx.  If the red LED on the module continues to flash at a period of about 1 seconds then it signals that serial communication has not been established.  Check your settings under the model menu as described above and check that the protocol selection switch on the module is at 0 (enable Serial mode).  If there is still no communication, power down and power up the Tx.  Finally check that you have correctly enabled your module for serial as described on the hardware page for your module under the heading "Enabling your module for Serial". Click here to access the [hardware](Hardware.md) and then click on the picture of your module.

### Protocol Selection in Serial mode
To select the protocol:
 1. In the Model Setting menu, scroll through the available options under the MULTI protocol   
 1. Depending on which protocol you have selected you may be required to select a sup-protocol and options.  For example, there are three protocols that correspond to FrSky recievers: FrSky_V, FrSky_D and FrSky_X.  In some cases the sub-protocols have options that could specify the number of channels, packet frame rate or fine frequency tuning. Check out the [Protocol Details](../Protocols_Details.md) page for detailed information and suggestions regarding the sub-protocols and options. The picture below shows the settings in the erSky Model Setup menu for FrSkyX subprotocol with {mike to insert} options:
 {mikeb to send simple picture}

### Binding in Serial mode
1. Switch on the model or put the receiver into bind mode 
1. On the transmitter go to the Model Settings menu and scroll down to the [Bind] menu option and press Enter. 
1. Press Enter again to exit Bind mode 

For many consumer models consider checking the Autobind option.  This will initiate the bind sequence as soon as the module is powered up by the transmitter.

If you are struggling to get a bind please see the [Getting the bind timing right page](Bind_Timing.md)

