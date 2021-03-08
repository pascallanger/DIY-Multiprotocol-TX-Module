# CCPM Multi module hardware modification

By default Multi uses the telemetry line to send the received channels using a RX protocol (FrSky, DSM, AFHDS2A, Bayang) to the radio.
But this does not work on FrSky radios since the telemetry lines of the internal and external modules are shared.
On a STM32 module and with a simple hardware modification, you can go around this hardware limitation by using CPPM to send the trainer information to the radio.

There are 2 ways to connect the module CPPM trainer signal available on the STM32 USART1.TX pin (BOOT0 programming TX pin) to the radio:
1. Use the trainer jack input which is supported by all radios.
1. Use the heart beat module bay pin 2, currently supported by erskyTX but not yet by OpenTX. It also seems from reports that not every FrSky radios (X10/X12?) supports this... **After this mod the module should be flashed with the radio off**

For the hardware modification you need:
1. 1K resistor
1. Wire
1. Shrink tube to isolate the resistor
1. For an external trainer, a 3.5mm mono jack plug
1. Soldering iron and solder

## Trainer jack

### Here is the modification on a Banggood 4-in-1 or an iRangeX IRX4 / IRX4+:

![Image](/docs/images/CPPM_BG_IRX4_Jack.jpg)

### Here is the modification on a Jumper 4-in-1:
   
![Image](/docs/images/CPPM_JP4IN1_Jack.jpg)

## Heart beat within the module bay

### Here is the modification on a Banggood 4-in-1 or an iRangeX IRX4 / IRX4+:

![Image](/docs/images/CPPM_BG_IRX4_Mark.jpg)
![Image](/docs/images/CPPM_BG_IRX4_Soldered.jpg)

### Here is the modification on a Jumper 4-in-1:
   
![Image](/docs/images/CPPM_JP4IN1_Mark.jpg)
![Image](/docs/images/CPPM_JP4IN1_Soldered.jpg)
