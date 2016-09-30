# Troubleshooting

##LED status
###Green LED
- Off: no power to the module
- On: module is powered up 

###Red LED (bind LED)
- Off: program not running or a protocol selected with the associated module not installed
- Flash(on=0.1s,off=1s): invalid protocol selected (excluded from compilation or invalid protocol number)
- Fast blink(on=0.1s,off=0.1s): bind in progress
- Slow blink(on=0.5s,off=0.5s): serial has been selected but no valid signal has been seen on the RX pin.
- On: Module is in normal operation mode (transmitting control signals).

##Protocol selection
###Input Mode - PPM
- The protocol/mode selection must be done before the power is applied
- Check the Green LED to see when power is applied.  Often power is not applied to the module until the transmitter has performed safety checks (like switch and throttle position settings)
- Check that at least one of the protocal selection to GND.

###Input Mode - Serial
- Make sure you have done the mods to the v2.3c PCB by adding the 2.2k and 470 ohm resistors as indicated in the [hardware page for your board] (Hardware.md).
- Protocol selection dial must be in the 0 position or leave all 4 selection pins unconnected.

##Bind
Make sure to follow this procedure: press the bind button, apply power and then release after the red LED starts flashing. The LED should be blinking fast indicating a bind status and then fixed on when the bind period is over. It's normal that the LED turns off when you press the bind button, this behavior is not controlled by the Atmega328.
For serial, the preffered method is to bind via the GUI protocol page.

If your module is always/sometime binding at power up without pressing the button:
 - Arduino Pro Mini with an external status LED: to work around this issue connect a 10K resistor between D13 and 3.3V.
 - 4in1 module V1 (check 4in1 pictures): to solve this issue, replacing the BIND led resistor (on the board back) of 1.2K by a 4.7K.

##Report issues
You can report your problem using the [GitHub issue](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/issues) system or go to the [Main thread on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676) to ask your question.
Please provide the following information:
- Multiprotocol code version
- TX type
- Using PPM or Serial, if using er9x or ersky9x the version in use
- Different led status (multimodule and model)
- Explanation of the behavior and reproduction steps
