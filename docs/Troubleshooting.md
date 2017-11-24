# Troubleshooting

## LED status

### Green LED

- **_Off_**: no power to the module
- **_On_**: module is powered up 

### Red LED (bind LED)

- **_Off_**: program not running or a protocol selected with the associated module not installed
- **_Flash(on=0.05s,off=1s)_**: invalid protocol selected (excluded from compilation or invalid protocol number)
- **_Inverted Flash(on=1s,off=0.1s)_**: module is waiting for a bind event (Bind from channel or Bind in radio GUI) to launch the protocol in bind mode
- **_Fast blink(on=0.1s,off=0.1s)_**: bind in progress
- **_Slow blink(on=0.5s,off=0.5s)_**: serial has been selected but no valid signal is being seen on the RX pin.
- **_Slower blink(on=1s,off=1s)_**: PPM has been selected but no valid signal is being seen on the PPM pin.
- **_On_**: Module is in normal operation mode (transmitting control signals).

## Protocol selection

### Input Mode - PPM

- The protocol/mode selection must be done before the power is applied to the module
- Often the signal is not sent to the module until the transmitter has performed safety checks (like switch and throttle position settings)
- Check that at least one of the protocol selection pins is connected to GND.
- Some radios have an open collector output (Futaba, Graupner...), in this case add a 4.7K resistor between PPM and BATT.

### Input Mode - Serial

- Make sure you have done the serial mods as indicated in the [hardware page for your board](Hardware.md).
- Protocol selection dial must be in the 0 position or leave all 4 selection pins unconnected.
- Often the signal is not sent to the module until the transmitter has performed safety checks (like switch and throttle position settings)

## Bind

Make sure to follow this procedure: press the bind button, apply power and then release after the red LED starts flashing. The LED should be blinking fast indicating a bind status and then fixed on when the bind period is over. It's normal that the LED turns off when you press the bind button, this behavior is not controlled by the Atmega328.
For serial, the preffered method is to bind via the GUI protocol page.

If your module is always/sometime binding at power up without pressing the button:
 - Arduino Pro Mini with an external status LED: to work around this issue connect a 10K resistor between D13 and 3.3V.
 - 4in1 module V1 (check 4in1 pictures): to solve this issue, replacing the BIND led resistor (on the board back) of 1.2K by a 4.7K.

FrSky & SFHSS bind issues are ususally due to Option=fine frequency tuning not set properly. This value is different for each Module. To determine this value:
 - find a value where the RX accepts to bind. A good start is to use one of these values -40, 0 and 40.
 - find the values min/max where the RX loses connection. In serial mode you can change the value and see the effect live.
 - set the value to half way between min and max.

## Report issues

You can report your problem using the [GitHub issue](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/issues) system or go to the [Main thread on RCGROUPS](http://www.rcgroups.com/forums/showthread.php?t=2165676) to ask your question.
Please provide the following information:
- Multiprotocol code version
- TX type
- Using PPM or Serial, if using er9x or ersky9x the version in use
- Different led status (multimodule and model)
- Explanation of the behavior and reproduction steps
