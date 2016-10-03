# Compatible Transmitters

Any Tx that provides a PPM output (like a trainer port, or a RF module bay) is compatible with the DIY Multiprotocol module.  In practice, most of the documentation on this site is focused on building modules that slip into your transmitter’s module bay. 
 
There are two different options for the interface between the Mulitprotocol Module and the transmitter: PPM and Serial. The considerations are different for each.
- **PPM** is more generic, easy to implement and will work with most transmitters. 
- **Serial** requires custom firmware on the transmitter but brings added functionality including telemetry and protocol selection through the Tx interface

##PPM
The DIY Mulitprotocol module supports industry standard PPM interface that works with all transmitters with either a module bay, and/or a trainer port.  Even the older 72MHz FM radios support this standard.

When using the standard PPM Tx output, the protocol selection is achieved through a 16 position rotary switch on the module. This enables 15 protocol/sub-protocol/options combinations to be selected.  Binding is achieved by pressing a bind button on the back of the module (see picture below) 

<img src="images/4-in-1_Module_PPM_Controls.jpg" width="150" height="180" /> 

Since the module supports literally hundreds of protocol/sub-protocol/options combinations, you must select which of these will map to the 15 positions on the switch.  Refer to the [Compiling and Programming](Compiling.md) page for information on how to do his.

Telemetry is available as a serial output on the TX pin of the Atmega328p using the FrSky hub format for Hubsan, FrSkyD, FrSkyX and DSM format for DSM2/X.  The serial parameters depends on the protocol:

Protocol|Serial Parameters
--------|-----------------
Hubsan|9600bps 8n1
FrSkyD|9600bps 8n1
FrSkyX|57,600bps 8n1
DSM2/X|125,000bps 8n1


You can connect it to your TX if it is telemetry enabled or use a bluetooth adapter to send it to a tablet/phone. See [Advanced Topics - Bluetooth Telemetry](Advanced_Bluetooth_Telemetry.md)

For transmitter setup using the PPM protocol go to the [PPM Setup page](PPM_Setup.md)

##Serial
Transmitters that run er9X, erSky9X or OpenTx firmwares - like the FrSky Taranis and FlySky TH9X/Turnigy 9X/R family of transmitters - have the option of using a fast, two-way serial, communication protocol between the Tx and the DIY Multiprotocol module.  Using this serial communication protocol has some significant advantages:

1. selecting the specific radio protocol (e.g. DSM) and the sub protocol (e.g. DSMX) directly in the menu system of the Tx (see the picture below) 
1. binding through the menu on the Tx 
1. range checking through the menu on the Tx 
1. enabling two-way telemetry for telemetry capable receivers and protocols. 

<img src="images/OpenTx_Multi_Menu.jpg" width="470" height="180" /> <img src="images/er9X_Multi_Menu.jpg" width="250" height="180" /> 


This serial protocol does not require any hardware modifications, but **will** require updating the firmware on your radio. 

To enable serial telemetry **may** require modifications to your Tx. See the table below.

Transmitters and firmware combinations that support the Serial protocol are:



Transmitter|Firmware Options|Telemetry Enabled
:----------|:---------------|:----------------
[FrSky Taranis/Plus/9XE](Tx-Taranis.md)| erSky9x, OpenTx 2.1.8 Multi|Yes - native
[Turnigy 9X/9xR](Tx-FlyskyTH9X.md)|er9x|[Mod required](#Telemetry_Mod), No DSM telem
[Turnigy 9XR-Pro](Tx-erSky9X.md)|erSky9x|Yes - native
[FrSky TH9x](Tx-FlyskyTH9X.md)|er9x|[Mod required](#Telemetry_Mod), No DSM telem
[SKY board](Tx-erSky9X.md)|erSky9x|Yes - native
[AR9X board](Tx-erSky9X.md)|erSky9x|Yes - native
[9Xtreme board](Tx-erSky9X.md)|erSky9x|Yes - native
[AR9X UNI board](Tx-erSky9X.md)|erSky9x|Yes - native

Click on your transmitter above to view specific setup instructions.

<a name="Telemetry_Mod"></a>   
##Optional Telemetry mod for 9X/r TH9X transmitters
The telemetry mod for these transmitters has evolved.  The original and popular "FrSky Telemetry Mod" requires 2 pins on the transmitter module board to be modified (RX on pin 5 and TX on pin 2).  All the recent MPTM hardware options supports serial transmission on pin 1 (the same pin as the PPM signal) so, in this case, only the mod on pin 5 is required. 

A good tutorial to follow is Oscar Liang's [here](http://blog.oscarliang.net/turnigy-9x-advance-mod/) but when you get to wiring up the Tx Module bay pins, you only need to perform the steps relevant for Pin 5.

You can see Midelic's original instructions [here](http://www.rcgroups.com/forums/showpost.php?p=28359305&postcount=2)  


##Other Notes:  
- er9X and erSky9X firmware already supports Multiprotocol Module as a standard feature.  At time of writing it looks like that the next major release of OpenTx - OpenTx 2.2 - will have DIY Mulitprotocol support as a standard feature.  

- Owners of Walkera Devo transmitters should look at the [Deviation-Tx](http://www.deviationtx.com) project for how to achieve the same end goal with your transmitters. 

- To enable telemetry on a Turnigy 9X or 9XR you need to modify your TX following one of the Frsky mod like this [one](http://blog.oscarliang.net/turnigy-9x-advance-mod/).

- DSM telemetry is not available on er9x due to a lack of flash space.

- Enabling telemetry on a 9XR PRO and may be other TXs does not require any hardware modifications. The additional required serial pin is already available on the TX back module pins.

- Once the TX is telemetry enabled, it just needs to be configured on the model (see er9x/ersky9x documentation).

