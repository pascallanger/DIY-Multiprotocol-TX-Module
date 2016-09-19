#Documentation ToDos
1. Documentation on all the FlySky boards: (MikeB?)
   - SKY board	erSky9x
   - AR9X board	erSky9x
   - 9Xtreme board	erSky9x
   - AR9X UNI board
1. Add to the troubleshooting page
1. Document the OrangeRx Transmitter module (Mikeb?)
1. enabling Serial on the DIY PCB page
1. lots of pictures mentioned between the {} markers
2. Add how to wire the antenna switcher in the "solder your own board and use 4-in-1 Rf module (add pictures of the wires from the ATmega pins to PE1 and PE2)
3. Must this be added to the (Compiling for Taranis)Also, if you are using a Taranis, then you need to invert the telemetry as shown:
Code:
//Uncomment to invert the polarity of the telemetry serial signal.
//For ER9X and ERSKY9X it must be commented. For OpenTX it must be uncommented.
#define INVERT_TELEMETRY1
1. ~~restructure the transmitter setup documentation~~ 
   - ~~PPM setup all on one page~~ 
   - ~~Serial setup by Transmitter~~ 

1. PPM Telemetry: added different serial speeds based on protocol for none inverted telemetry:
FrSkyD (Incl Hubsan): 9600bps 8n1
FrSkyX: 57600bps 8n1
DSM: 125000bps 8n1
I've done this if people wants to connect something (arduino, bluetooth,...) behind the module to display telemetry when used in PPM mode. It's using the default speed of the original transmitter.
This is not something supported by er9x or ersky9x which are meant to be used in serial mode. 
1. Someone to add the Build the board from scratch if it is still relevant 

