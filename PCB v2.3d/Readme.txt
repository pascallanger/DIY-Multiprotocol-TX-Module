These are KiCad files and you are free to do what you will with them. KiCad is a good, free, and fairly
easy to learn.  Build your own BOM and gerber files.

This is a variant of the Multipro V2.3c circuit design.  It is basicly the same as the 2.3c board as far
as component placement goes.  What's changed is the added resistors for the serial protocol and also
the addition of solder jumpers on the bottom of the board for the various options to connect the TX, RX, and PPM
lines through them. See below for more detail.

The schematic has been updated to reflect the added components and jumper pads as well as cleaned
up a little.  As it sits now, the .net file loads without any complaints and DRC checks pass.

The jumpers, and how they are used:
	

There are four solder type jumpers on the bottom side of the board near the lower left corner when the
bottom of the board is facing towards you. The silkscreen shows which jumper is which. These four jumpers 
enable the board to be configured in several ways as explaned below.

	(J-1)	Use (PPM V/V) if the incoming PPM signal is at a higher voltage level, leave open if ~~5V.

	(J-2)	Use (Jumper 2) to connect the incomming PPM signal to the RX pin on the processor

	(J-3)	Short (TELEM) only if you have done a telemetry mod to your radio, leave open if not needed. When
		connected, pin 2 of the two pin header (P3) is also connected.

	(J-4) Use (MOD) only to connect the transmitter pin 2 to pin 1 of the two pin header (P3).

The direction this project is going, it is most likely J-2 will be the only one needing to be shorted for
the serial method of sending model protocols.


These files are submitted without any guarentee of accureacy or suitability for any intended use.  I am strictly
an amature with time on his hands. Although I have done all I know to make it correct, things outside of my
knowledge base are beyond my control.  Do not use untested equipment around persons not familiar with the hazards
of remote controlled vehicals.