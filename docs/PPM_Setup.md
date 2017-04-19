# PPM Setup

The Multiprotocol Module is compatible with any transmitter that is able to generate a PPM (Pulse Postion Modulation) output.  This includes all transmitters with a module bay or a trainer port.  It supports up to 16 channels from a PPM frame in the normal or inverted format (sometimes called positive or negative format in some transmitters).
If you want the best performance you can set the number of channels and framerate corresponding to the number of channels of the specific receiver/model.

## PPM Connections
If you do not have a module bay, there are only three wires you need to connect to get PPM to work.  (The pins are numbered from top to bottom) 
- PPM on pin 1
- vbat on pin 3
- ground on pin 4  

Note: vbat should be between 6V and 13V when using the 4-in-1 and 2.3 PCB boards. If you built a module from scratch it depends on the voltage regulator you chose.


## Enabling PPM mode in your transmitter

1. Enable the default Tx mode to be AETR. If you do not want to change the default channel order on your Tx you must remember to change the channel order for each new model using the module to AETR under the Model Mixer menu. (**This is really important - this is for all protocols - even for DSM as the MULTI-module firware will change the transmitted channel order according to the protocol.**)  
1. The default PPM settings is 8 channels with a frame period of 22.5 ms (sometimes called the frame rate).  If you want to optimize performance you should change the channels to the actual number of channels required by your model.  The corresponding frame period should be set to (number of channels + 1) * 2.5 ms.  For example:
    - A 4 channel model the frame period is (4 + 1)*2.5 = 12.5 ms.
    - A 6 channel model the frame period is (6 + 1)*2.5 = 17.5ms. 
    
## Protocol selection in PPM mode

To select the protocol simply switch off the transmitter and rotate the protocol selection switch on the module to the desired position. 

**Note that the dial selection must be done before the module receives power - this is not necessarily the same time that the transmitter is powered up.  The transmitter often only provides power to the module once it has passed switch checks and throttle position checks.**

The default mapping of protocols to switch positions can be viewed on the Protocol Details page found [here](Protocol_Details.md#DefaultMapping)

The mapping of protocols to protocol selection switch positions can be changed in configuration settings as described on the [Compiling and Programming page](Compiling.md).

## Binding in PPM mode

In PPM mode follow the standard transmitter - receiver binding process: 
 1. Switch off the transmitter
 1. Switch on the receiving device in bind mode (if it is not already autobind). Check the documentation for your device.
 1. Press and hold the bind button on the back of the module as you power up the transmitter. Hold the button down until the transmitter powers up the module. The red LED on the module should be flashing at about 5Hz - indicating bind mode.
 1. Watch the receiver for the completion of the bind process
 1. This is a model supporting autobind (binds every time it powers up) then you should be ready to go
 1. For traditional RC receivers with a bind memory - power down the receiver and the Tx and then power up the Tx and the Rx to confirm bind.

If you are having trouble binding to a consumer quad check the section below on [Getting your Bind Timing right](Bind_Timing.md). For more details on setting up specific receivers or models, check out the [Protocol Details page](Protocol_Details.md).

## Telemetry in PPM mode

Telemetry is available as a serial stream on the TX pin of the Atmega328p in the FrSky HUB format. The serial parameters are based on the protocol selected by the protocol selection dial. 

Protocol|Serial Parameters
---|---
Hubsan|9600bps 8n1
FrSkyD|9600bps 8n1
FrSkyX|57,600bps 8n1
DSM2/X|125,000bps 8n1

The serial stream is also available on pin 5 of the Module connector (pins numbered from top to bottom) on the [4-in-1 module](Module_BG_4-in-1.md) and the [V2.3d modules](Module_Build_yourself_PCB.md#atmega-board-v23d) provided the Tx jumper has been soldered.  See the linked module documentation for what this means. 

You can connect it to your TX if it is telemetry enabled or use a bluetooth adapter (HC05/HC06) along with an app on your phone/tablet [(app example)](https://play.google.com/store/apps/details?id=biz.onomato.frskydash&hl=fr) to display telemetry information and setup alerts.
