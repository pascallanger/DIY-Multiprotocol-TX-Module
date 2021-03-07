# CCPM Multi module hardware modification

By default Multi uses the telemetry line to send the received channels using a RX protocol (FrSky, DSM, AFHDS2A, Bayang) to the radio.
But this does not work on FrSky radios since the telemetry lines of the internal and external modules are shared (hardware limitation).
On a STM32 module and with a simple hardware modification, you can go around this limitation using CPPM to send the trainer information to the radio.

For the hardware modification you need:
1. 1K resistor
1. wire
1. shrink tube to isolate the resistor
1. soldering iron and solder

The goal is to add the 1K resistor between the STM32 USART1.TX pin (Boot0 programming TX pin) and the radio bay pin 2.

Here is the modification on a Banggood 4-in-1 or an iRangeX IRX4 / IRX4+:

    ![Image](/docs/images/CPPM_BG_IRX4_Mark.jpg)

Here is the modification on a Jumper 4-in-1:
    ![Image](/docs/images/CPPM_JP4IN1_Mark.jpg)
