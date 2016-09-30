#Documentation ToDos
1. Documentation on all the FlySky boards: (MikeB?)
   - SKY board	erSky9x
   - AR9X board	erSky9x
   - 9Xtreme board	erSky9x
   - AR9X UNI board
1. Add to the troubleshooting page
1. Add how to do custom protocol setup on er9x and OpenTx
1. Document the OrangeRx Transmitter module (Mikeb?)
1. enabling Serial on the DIY PCB page
1. lots of pictures mentioned between the {} markers
2. Add how to wire the antenna switcher in the "solder your own board and use 4-in-1 Rf module (add pictures of the wires from the ATmega pins to PE1 and PE2)
1. Someone to add the Build the board from scratch if it is still relevant 

#Proposal for renaming options to make things simpler

Multiprotocol Transmitter Module or Multiprotocol Module is not only becoming quite a mouthful but we use module on two different levels. (for example the CC2500 module and the DIY Multiprotocol Module). It is nevertheless very descriptive. Without throwing the proverbial "baby out with the bathwater" I suggest that we set expectations that **MPTM** can be interchangeably used for Multiprotocol Module. If we use the term interchangeably we can see if it begins to stick.  I do not suggest that we change any of the github or rcgroups pages.

In addition to this it would be very useful if we could bucket the different MPTM options according to how they are built.  For example:

- **Ready-made:** Banggood 4-in-1 module : Readymade Mulitprotocol Module or Readymade Multiprotocol Transmitter Module or Readymade MPTM or Banggood MPTM
- **DIY:** Modules made from PCBs : DIY Multiprotocol Module, DIY ATmega Multiprotocol Module, DIY STM32 Multiprotocol Module or DIY STM32 MPTM
- **Scratchbuild:** Modules made from scratch:  
   - Option 1: Perfboard Multiprotocol Module or Perfboard MPTM
   - Option 2: Scratchbuild Multiprotocol Module or Scratchbuild MPTM


1.  Move to atmega specific and add ftdi to stm32 AVR ISP programmer like the popular USBASP programming dongle that is 3.3V safe - available from many sellers on ebay. There are reports that some of the cheap programmers are not safe to use with 3.3V units (like this unit). Look for USBAsp programmers with the “LC Technologies” label. {Pascal to confirm these reports are true} Also, you will need a 10-pin to 6-pin connector to connect the USBASP to the board.
