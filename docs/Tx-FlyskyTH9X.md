# Flysky TH9X family of transmitters
This page is relevant to the following transmitters:
* FlySky TH9X
* Turnigy 9X, Turnigy 9XR
* EURGLE


## Features
The MULTI-Module can be used in the Flysky family of transmitters in either PPM mode or in Serial mode.  To operate in Serial mode, a version of er9X supporting the MULTI-Module must be installed on the Tx. 

## PPM Mode
Please refer to the [PPM Setup](PPM_Setup.md) page. 


## Serial Mode
Serial mode is only supported by the er9X firmware.  Loading this firmware is beyond the scope of this document but it is well covered in tutorial and video tutorials online. The firmware you should use with the MULTI-Module is available on Mike's [er9X Test Version Page](http://openrcforums.com/forum/viewtopic.php?f=5&t=4598). Use the following firmwares:  
 - **er9x.hex** : for a standard unmodified transmitter  
 - **er9x_frsky.hex** : if you have done the FrSky telemetry mod  
 
er9X is very well documented, see [here](http://openrcforums.com/forum/viewtopic.php?f=5&t=6473#p90349)

### Enabling Serial Mode
1. Confirm that the MULTI-Module has the required physical connections between the pins on the back of the Tx and the ATMega328 microprocessor.  This may require some soldering and depends on which version of the MULTI-Module you have.  Check out your module’s hardware page under the section **Enabling your MULTI-Module for Serial** for details. Click here for the [hardware](Hardware.md) pages.
1. Plug in your DIY Multiprotocol module into the transmitter module bay.  If you have a rotary protocol selection switch, turn the switch to position 0 to put the unit into Serial mode.  
1. Ensure throttle is down and all switches are in the start position and power up the Tx.  The red LED on the DIY Multiprotocol module should be flashing with a period of about 1 second indicating that it has not established a valid serial link with the Tx.  This is expected as we have not set up the Tx yet.
1. Create a new model and confirm that the channel order is set to AETR (**This is really important - this is for all protocols - even for DSM as the MULTI-module firware will change the transmitted channel order according to the protocol.**  
1. In the Model Settings menu scroll down to change the RF settings to MULTI {mikeb - can you write this line }
1. The red LED on the MULTI-Module should briefly flash and then go off.  This confirms that the MULTI-Module has established serial communication with the Tx.  If the red LED on the module continues to flash at a period of about 1 seconds then it signals that serial communication has not been established.  Check your settings under the model menu as described above and check that the protocol selection switch on the module is at 0 (enable Serial mode).  If there is still no communication, power down and power up the Tx.  Finally check that you have correctly enabled your module for serial as described on the hardware page for your module under the heading "Enabling your module for Serial". Click here to access the [hardware](Hardware.md) and then click on the picture of your module.

### Protocol Selection in Serial mode
To select the protocol:
 1. In the Model Setting menu, scroll through the available options under the MULTI option {mikeb to confirm}.   
 1. Depending on which protocol you have selected you may be required to select a sup-protocol and options.  For example, the FrSky protocol has three sub-protocols FrSky_V, FrSky_D and FrSky_X.  In some cases the sub-protocols have options that could specify the number of channels, packet frame rate or fine frequency tuning. Check out the [Protocol Details](../Protocols_Details.md) page for detailed information and suggestions regarding the sub-protocols and options. The picture below shows the settings in the erSky Model Setup menu for FrSkyX subprotocol with {mike to insert} options:
 {mikeb to send simple picture}

### Binding in Serial mode
1. Switch on the model or put the receiver into bind mode 
1. On the transmitter go to the Model Settings menu and scroll down to the [Bind] menu option and press Enter. 
1. Press Enter again to exit Bind mode 

For many consumer models consider checking the Autobind option.  This will initiate the bind sequence as soon as the module is powered up by the transmitter.

If you are struggling to get a bind please see the [Getting the bind timing right page](Bind_Timing.md)
