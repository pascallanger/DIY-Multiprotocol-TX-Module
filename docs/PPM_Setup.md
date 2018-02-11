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

The protocol selection is based on 2 parameters:
  * selection switch: this is the rotary switch on the module numbered from 0 to 15
      - switch position 0 is to select the Serial mode for er9x/ersky9x/OpenTX radio
      - switch position 15 is to select the bank
	  - switch position 1..14 will select the protocol 1..14 in the bank *X*
  * banks are used to increase the amount of accessible protocols by the switch. There are up to 5 banks giving acces to up to 70 protocol entries (5 * 14).  To modify or verify which bank is currenlty active do the following:
      - turn on the module with the switch on position 15
      - the number of LED flash indicates the bank number (1 to 5 flash)
	  - to go to the next bank, short press the bind button, this action is confirmed by the LED staying on for 1.5 sec

Here is the full protocol selection procedure:
  * turn the selection switch to 15
  * power up the module
  * the module displays the current bank by flashing the LED x number of times, x being between 1 and up to 5
  * a short press on the bind button turns the LED on for 1 sec indicating that the system has changed the bank
  * repeat operation 3 and 4 until you have reached the bank you want
  * power off
  * change the selection switch to the desired position (1..14)
  * power on
  * enjoy

Notes:
  * **The protocol selection must be done before the module is turned on**
  * The protocol mapping based on bank + selection switch position can be seen/modified at the end of the file [_Config.h](/Multiprotocol/_Config.h)**

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
