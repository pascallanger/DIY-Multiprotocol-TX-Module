
#4-in-1 Banggood module
Currently the form factor of this module is designed for the JR-style module bay. Many of the popular RC transmitters use the JR-style module bay: FrSky Taranis, FlySky Th9x, Turnigy 9X/R/Pro
##What you need
1. A fully assembled module + case available from Banggood.com [here](http://www.banggood.com/CC2500-NRF24L01-A7105-CYRF693-4-In-1-RF-Module-With-Case-For-Futaba-JR-Frsky-Transmitter-p-1116892.html)

<img src="images/4-in-1_Module_Case_BG.jpeg" width="221" height="200" /> 

or

1. The ready-made module available from Banggood.com [here](http://www.banggood.com/2_4G-CC2500-A7105-Flysky-Frsky-Devo-DSM2-Multiprotocol-TX-Module-With-Antenna-p-1048377.html)  

<img src="images/4-in-1_Module_BG.jpeg" width="200" height="200" />
 
1. A module case that fits your module like the one [here](https://www.xtremepowersystems.net/proddetail.php?prod=XPS-J1CASE)  
 <img src="https://www.xtremepowersystems.net/prodimages/j1case.jpg" width="200" height="180" />  
  or you can 3D print your own from a selection on Thingiverse ([Example 1](http://www.thingiverse.com/thing:1852868) [Example 2](http://www.thingiverse.com/thing:1661833)).  
 [<img src="http://thingiverse-production-new.s3.amazonaws.com/renders/55/1c/cb/0a/e4/5d2c2b06be7f3f6f8f0ab4638dd7c6fc_preview_featured.jpg" width="250" height="200" /> ](http://www.thingiverse.com/thing:1852868)

For 9XR/9XR Pro, a new 3D printed module is available which makes use of the built in antenna in the handle. This means nothing is getting out of the radio back! You can find all details of this module case on [thingiverse](http://www.thingiverse.com/thing:2050717).

<img src="images/9XR_module.jpg" width="113" height="200" /> <img src="images/9XR_module_connector.jpg" width="274" height="200" /> 

##Different working modes

###PPM interface
If you are only planning on using the PPM interface with transmitter you are ready to program the module as described in Compiling and Programming the module.

###Enabling Serial and Telemetry interface
If you have a transmitter that can support serial communication with the board then you need to wire up the board appropriately. There are two versions of the module and the steps are slightly different.

Check which module you have and based on the pictures below.  If you purchased the module after June 2016 then it is likely that you have a V2 type module.  

#### **Version 2 (V2) type modules** 

Solder two bridges over the pads shown in the pictures below. 
 
V2a

<img src="images/V2a_Serial_Enable.jpeg" width="300" height="340" /> 
<img src="images/V2a_zoom_Serial_Enable.jpeg" width="450" height="340" /> 

V2b

<img src="images/V2b_Serial_Enable.jpeg" width="220" height="340" /> 

V2c

<img src="images/V2c_Serial_Enable.jpeg" width="220" height="340" /> 

You are now ready to go over to [Compiling and Programming](Compiling.md).

#### **Version 1 (V1) module** 

Solder bridges and resistors as illustrated in the picture below.
<img src="images/V1_Serial_Enable.jpeg" width="500" height="340" /> 

You are now ready to go over to [Compiling and Programming](Compiling.md).
