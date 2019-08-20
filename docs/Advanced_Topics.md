# Advanced Topics
Warning: the topics on this page are not for the fainthearted.  It is strongly recommended that you have some experience in getting up and runnning with your module before you dive in there.  On the other hand what is described on this page are some very useful options that could greatly increase the value and the enjoyment of your Multiprotocol module.

# Enable STM32 module serial debug 
This document describes how to enable serial debug for STM32 MULTI-modules.  This can be useful in case of issues with a protocol or to reverse a protocol based on the XN297L RF component.  See the [MULTI-Module Serial Debug](Advanced_Debug.md) page for more details.

# XN297L dump feature 
This document describes how to dump packets sent from a TX using a XN297L RF compatible component over the air on a STM32 MULTI-modules.  This can be useful to get details on a protocol or even fully reverse a protocol as used in many remote controls lately.  See the [MULTI-Module XN297L Dump](Advanced_XN297Ldump.md) page for more details.

# EEPROM Backup and Restore
This document describes how to back up and restore the EEPROM for both Atmega328p and STM32 MULTI-modules.  This can be useful if cloning a module, or to preserve settings.  See the [MULTI-Module EEPROM](EEPROM.md) page for more details.

# Telemetry in PPM mode
It is possible to access the telemetry stream coming from the receiver through the MULTI-module. This document describes a simple bluetooth module to stream telemetry information to a mobile device like an Android smartphone or tablet.  The method may be generalized to feed telemetry to the transmitter if the transmitter has the capabilities to process the information.  This is very useful with modules used in the PPM mode with transmitters that do not support telemetry.  See the [Advanced Bluetooth Telemetry](Advanced_Bluetooth_Telemetry.md) page for more details.  

# Manually setting fuses on ATmega328
This document describes a relatively simple process to set the fuses on ATmega328.   See the [Advanced Manually Setting ATmega328 Fuses](Advanced_Manually_Setting_ATmega328_Fuses.md) page for more details.  
