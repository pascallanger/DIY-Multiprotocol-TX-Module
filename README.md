# Overview

Multiprotocol is a 2.4GHz transmitter which enables any TX to control lot of different models available on the market.

The source code is partly based on the [Deviation TX project](http://www.deviationtx.com), thanks to all the developers for their great job on protocols.

## Quicklinks
* [Download latest releases of the firmware](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/releases)
* [Forum on rcroups](http://www.rcgroups.com/forums/showthread.php?t=2165676)
* [Available Protocols list](docs/Protocol_Details.md)
* [The old documentation](docs/README-old.md)
* [Documentation to-do list](docs/Documentation_To_Do_List.md)

## Outline of the documentation
1. Introduction (this page)
1. [Available protocols](docs/Protocol_Details.md)
1. [Compatible Transmitters](docs/Transmitters.md)
1. [Module Hardware options](docs/Hardware.md)
1. [Compiling and programming the module (ATmega328)](docs/Compiling.md) and [Compiling STM32](Compiling_STM32.md).
1. Transmitter Setup 
   - [Taranis](docs/Tx-Taranis.md)
   - [FlySky TH9X, Turnigy 9X/R](docs/Tx-FlyskyTH9X.md)
1. [How to for popular models](docs/Models.md)
1. [Troubleshooting](docs/Troubleshooting.md)
2. [Advanced Topics (not for the fainthearted!)](docs/Advanced_Topics.md)

## Introduction
A functioning DIY Multiprotocol module consists of (see image below):
![Screenshot](docs/images/DIY_Multiprotocol_Module_Overview.png)

1.  A host RC transmitter

1. DIY Multiprotocol module that connects to a host transmitter.  This module is typically comprised of

  * A microprocessor (currently ATMega328P) that interfaces with the Tx, controls the module functions and forwards the RC commands to the RF hardware

  * One or more (but at least one) RF modules that provide the capability to communicate with RC receivers.  To communicate with the receiver the RF module in the Tx must match with the RF module type in the receiver.  The four most common 2.4GHz RF chips on the market are supported TI CC2500, Nordic NRF24L01, Cypress CYRF6936, and the Amiccom A7105

  * DIY Multiprotocol firmware loaded on to the microprocessor.  At a high level, this firmware performs a few different functions: 
     * It interfaces with signals from the host Tx and decodes these for transmission to the model, it manages the activation of the correct hardware RF module for each protocol
     * It implements the unique communication protocols for each receiver/model and manages the all-important binding process with a receiver/model
     * In the case of some protocols (for example DSMX and FrSky) it receives and decodes the telemetry information and makes this available to the receiver.
1. The physical 2.4GHz antenna (or in some cases multiple antennas) for the modules



In constructing a functioning module there are important choices to be made and tradeoffs to be aware of.  The most important are:

##**Choice 1:** Which module hardware option 

There are currently four generic paths to construct your own DIY Multiprotocol module.  These are outlined in detail on the [hardware] page.  Here they are in order of increasing difficulty:
  * You can purchase a ready-made DIY Multiprotocol module from Banggood
  * You can purchase a DIY Multiprotocol PCB from [OSHPark] and then buy and solder on your own RF modules 
  * You can use a Orange Rx transmitter module available from Hobyking
  * You can build the module from scratch using an Arduino Pro Mini and a perf board base.
The last option is where it all started and how the pioneers in this project made their boards.  However, due to the growing interest in “one module to rule them all” you now have options to purchase a ready-made board (with old firmware that you will need to upgrade).  

For more information on these options see the [hardware](docs/Hardware.md) page

##**Choice 2:** Which RF modules to include

This depends on your specific needs.  However, recent the availability of the 4-in-1 RF modules from Banggood for less than $35 makes it easy to “have it all”.  Most manufacturers of RC systems (Spektrum, FrSky, FlySky) and toys (Syma, Hubsan, Horizon Hobby, etc.) use one of these four RF chips to manage the RF link between the transmitter and the reciever/model.  Here is an incomplete list of the RF modules and some of the most popular toys that use them.  For the complete list see the [Protocol Details](docs/Protocol_Details.md) page.

Manufacturer|RF Chip|Example Protocols
:-----------|-------|:-------
Cyprus Semiconductor| CYRF6936|DSM/DSMX
||Walkera Devo
||J6Pro
Texas Instruments|CC2500|FrSky
||Futaba SFHSS
Amiccom|A7105|FlySky
||Turnigy (most)
||Hubsan
Nordic Semiconductor|NRF24L01|HiSky
||Syma
||ASSAN
||and most other Chinese models

For example, if you have no interest in binding your Tx to an model with and FrSky or Futaba SFHSS receiver you do not need to include the CC2500 RF module in your system.

##**Choice 3:** Which protocols to upload the module 

Of course there always a catch. There is a 32K memory limit on the ATmega328 processor. Due to the amazing work done by devs on this project, the memory required by all the possible protocols exceeds the limit. This means that you will need to make a choice of which protocols you will compile into your firmware.  Fortunately, the process of selecting and compiling is not too difficult and it is fully documented on the [Compiling and Programming](docs/Compiling.md) page.
Also, the lead dev Pascal Langer (rcgroups:hpnuts) makes this process even easier for many users by making compiled binaries available for three popular combinations of RF modules.  These are always “fresh” (based on the latest stable firmware) and available on the [Releases](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/releases) page.

##**Choice 4:** Choosing the type of interface with your Tx (PPM or Serial)

The DIY Mulitprotocol module supports industry standard PPM interface that works with all transmitters with either a module bay, and/or a trainer port.  Even the older 72MHz FM radios support this standard.

If you are the owner of a transmitter that supports the er9X/erSky9X or OpenTX firmwares (Frsky Taranis running erSky9x or OpenTx, or any of the FlySky/Turnigy family of Txs running ER9X, ERSky9x or OpenTx) you have the additional option to use a serial protocol to communicate between your Tx and the DIY Mulitprotocol module. (Owners of Walkera Devo transmitters should look at the [Deviation Tx](http://www.deviationtx.com) project for how to achieve the same end goal). This serial protocol does not require any hardware modifications, but will require updating the firmware on your radio. For those willing to do this, there are some nice advantages:
  * The model and protocol selection and binding is done from the Model Settings menu on the Tx
  * For telemetry capable receivers, the telemetry integration is done seamlessly with the Tx firmware.
See the [Setting up your Tx](docs/TransmitterSetup.md) page for more details.

#How to get started?
1. Browse the [Protocols] page to see which protocols you would like on your module
1. Go to the [Hardware Options] page to decide what Tx module you need and which RF modules you plan to integrate
1. Once you have your module, you should go to [Compiling and Programming](docs/Compiling.md) page to download, compile and program the DIY Multiprotocol module
1. Finally, you should visit the [Setting up your Tx](docs/TransmitterSetup.md) page to configure the last few settings before you can fly to your heart’s content!!!!!

# Troubleshooting
Visit the [Troubleshooting](docs/Troubleshooting.md) page.  Please bear in mind that the DIY Multiprotocol module is a complex system of hardware and software and it make take some patience to get it up and running.  Also remember that the developers of the system are actual users of the system.  This means that at any moment in time the system is working perfectly for them.  A corollary to this is that if you are struggling there are likely two scenarios.  First, that the problem is with your hardware or with your configuration, second, and much more unlikely but not impossible scenario, is that you are struggling with a new undiscovered bug.  (The author of this documentation speaks from experience ;-)   Please check the RC Groups forum and search for keywords relating to your problem before posting a reply.  When you do post a reply please so humbly and respectfully – you will find many helpful people there.  In your reply please include as much relevant information as possible and attach compilation output and _Config.h files as text attachments to keep the forum clean.
# A final word
A very big thanks to all the people who have shared their time so graciously to create this great project.  If you come across them on RC Groups, please be kind and show appreciation.  In no particular order:
* Pascal Langer (rcgroups: hpnuts)
* Mike Blandford (rcgroups: Mike Blandford)
* (rcgroups: midelic)
* victhz – from Deviation-tx
* Enter the other Deviation people here (###)

Your help would be greatly appreciated.  If protocol reverse-engineering and dev is not your thing then any help with testing and contributing to the documentation would be amazing.  Given the number of different Tx/module hardware/RF module/protocol/model combinations the process of testing and documenting is a major bottleneck for the developers.  Anything you can do to help will free them up to do even greater things. Check out this quick guide {How to help with documentation} for how you can submit changes to the documentation. 
