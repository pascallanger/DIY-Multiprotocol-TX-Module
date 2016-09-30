# Transmitter Setup
Describe the transmitters this applies to.

Describe the firmware required for the transmitters. The transmitters covered here are:
1. [tx1](###)
1. [tx2](###)

Does it work in PPM and/or Serial mode?

## PPM Mode
Please refer to the [PPM Setup](PPM_Setup.md) page. 


##Serial mode
###Enabling Serial Mode
To operate in serial mode, you need one of these firmwares:
1. OpenTx supporting the DIY Multiprotocol mdule (2.18 Multi or 2.2)
1. erSky9x

Check and upload a supported firmware.  The latest available version at time of writing are:
- OpenTx 2.1.8 Multi and the hex files are available [here](http://plaisthos.de/opentx/)
- erSky9x Revision 218 and the hex files are available [here](http://www.er9x.com).  

Tutorials for uploading new firmware using the SD Card are available [here](http://www.dronetrest.com/t/how-to-upgrade-firmware-for-frsky-taranis-x9d/959) or the CompanionTx software (recommended) are available [here](http://open-txu.org/home/undergraduate-courses/fund-of-opentx/part-2-flashing-opentx/). 

**Note: in the tutorials substitute the shown firmwares with the fimware donwloaded from the links above.**

First confirm that the DIY Multiprotocol module has the required physical connections between the pins on the back of the Tx and the ATMega328 microprocessor.  This may require some soldering and depends on which version of the DIY Multiprotocol module you have.  Check out this [Enabling Your Module for Serial] page for details.

Plug in your DIY Multiprotocol module into the Taranis module bay.  If you have a rotary protocol selection switch, turn the switch to position 0 to put the unit into Serial mode.  Ensure throttle is down and all switches are in the start position and power up the Taranis.  The red LED on the DIY Multiprotocol module should be flashing with a period of about 1s indicating that it has not established a valid serial link with the Tx.  This is expected as we have not set up the Tx yet.

Create a new model (make sure channel order is AETR) and on the first Model Settings page scroll down to disable the internal RF and enable the external RF by selecting MULTI as the external RF. Your Taranis settings should look like this: {insert picture of Taranis screen showing external RF settings}

The Red LED on the DIY Multiprotocol module should briefly flash and then go off.  This confirms that the DIY Multiprotocol module has established serial communication with the Tx.  If the red LED on the module continues to flash at a period of about 1s then it signals that serial communication has not been established.  Check your settings under the model menu as described above and check that the protocol selection switch on the module is at 0 (zero).  If there is still no communication, power down and power up the Tx.  Finally check that you have correctly enabled your module for serial as described here [Enabling Your Module for Serial]
###Protocol Selection in Serial mode
To select the protocol, scroll through the available options under the Model Settings menu.  Depending on which protocol you have selected you may be required to select a sup-protocol and options.  For example, the DSM protocol has two sub-protocols DSM2 and DSMX.  Each of these sub-protocols have options that specify the number of channels and the packet frame rate.  

The following picture shows DSM – DSMX – Option 6 (6 channels and 11ms frame rate). Check out the [Available Protocols] page for detailed information and suggestions regarding the sub-protocols and options.
###Binding in Serial mode
1. Switch on the model or put the receiver into bind mode
1. On the transmitter go to the Model Settings menu and scroll down to the [Bind] menu option.


