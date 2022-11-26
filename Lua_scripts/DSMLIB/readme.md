# Credits
Code is based on the code/work by: Pascal Langer (Author of the Multi-Module) 

Rewrite/Enhancements By: Francisco Arzu 

# Introduction

This script library is a rewrite of the original DSM forward programming Lua 
Script.  The goal was to make it easier to understand, mantain, and to  
separate the GUI from the DSM Forward programming engine/logic.
In this way, GUIs can evolve independent. Color/Touch Gui, Text only GUI, etc.

Most of the changes has been cometic, but fixed a few operational ones
1. Make "Gyro Settings"->"Initial Setup" works (Tested on AR631,AR637xx)
2. Properly reset and restart after initial configuration and SAFE changes.
3. Write Log of the conversation between RX/TX. To be use for debugging when some reports a problem.
4. Provide a simulation of RX to do GUI development in Companion, and undestand patterns of how the data is organized.

# Tested RXs
- AR631/AR637xx     Coded a hack to be able to make `Initial Setup` to work
- FC6250HX (Helicopter)

Most RX will run without problems, it could be that some others receivers will need to apply the same hack as the AR631 for some specific menus to work.
Since is RX and Menu specific, we cannot create a general hack.

Please report of you have test it with other receivers to update the documentation.   

If you get `"Unable to Load menu lines"` when trying to navidate to a menu, could be that the code needs to be specially adapter to mandle that menu in a different way than others. We did that HACK for AR631/AR637 for the `Initial Setup` and `Initial Safe Setup` menus.  Before starting the script again, the problem shouls show at the log. Usually not been able to process some `Unknown_0x5` lines who has not been fully reversed engineered (you can share the logs with us to try to understand what is going on. 


# Flight mode/Gain channels

I ran into a case where trying to set Aux2 or Aux3 for flight mode, but the RX was correcting it to Aux1.. the RX only was allowing Gear or Aux1 (AR631/AR637).
This is because the RX don't know that we are using more than 6 channels. To make the RX aware that there are other channels, while edditing the channel, you have to toggle the switch to excersist the channel, and now the RX will recognize it.


# Deployment

    /SCRIPTS/TOOLS/DsmFwdPrg_05_BW.lua      -- black/white text only radios
    /SCRIPTS/TOOLS/DsmFwdPrg_05_Color.lua   -- Color+touch radios
    /SCRIPTS/TOOLS/DSMLIB/      -- (ALL CAPITALS) Libraries ane extra files
    /SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lua   -- DSM Protocol Message and Menu engine
    SCRIPTS/TOOLS/DSMLIB/DsmFwPrgSIMLib.lua -- Simulation of AR631, FC6250HX
    SCRIPTS/TOOLS/DSMLIB/img        --Images for RX orientations
    LOGS/dsm_log.txt				--Readable log of the last RX/TX session, usefull for debuging new RX

# Messages Displayed in the GUI

If in a screen you get text that looks like `Unknown_XX` (ex: Unknown_D3), that message has not been setup in the script in english. If you can get what is the proper message, you can send us a message to be added to the library.
The `XX` represents a Hex Number (0..9,A..F)  message ID. 

If you want to fix it in your local copy, all messages are towards the end in the file `SCRIPT\TOOS\DSMLIB\DsmFwPrgLib.lua`. Messages for Haders are stored in `Text` and messages for Options are stored in `List_Text`.  Lua scripts are text files, and can be editted with Notepad or equivalent.

Portion of DsmFwPrgLib.lua:

    Text[0x0097] = "Factory Reset"
    Text[0x0098] = "Factory Reset" -- FC6250HX: Title
    Text[0x0099] = "Advanced Setup"
    Text[0x009A] = "Capture Failsafe Positions"
    Text[0x009C] = "Custom Failsafe"

    Text[0x009F] = "Save & Reset RX"  -- TODO: Find the Proper Spektrum Value ??

    Text[0x00A5] = "First Time Setup"
    Text[0x00AA] = "Capture Gyro Gains"
    Text[0x00AD] = "Gain Channel Select"

    -- Safe mode options, Ihnibit + thi values 
    local safeModeOptions = {0x0003,0x00B0,0x00B1}  -- inh (gap), "Self-Level/Angle Dem, Envelope
    List_Text[0x00B0] = "Self-Level/Angle Dem"
    List_Text[0x00B1] = "Envelope"

For example, if you get `Unknown_9D` in the GUI and your now that it should say **NEW Text**, you can edit the lua script to look like this:

    Text[0x009A] = "Capture Failsafe Positions"
    Text[0x009C] = "Custom Failsafe"

    Text[0x009D] = "NEW Text" -- NEW Text added for AR98xx

    Text[0x009F] = "Save & Reset RX"  -- TODO: Find the proper Spektrum text


# LOG File

The log file of the last use of the script is located at `/LOGS/dsm_log.txt`. **It is overriden on every start to avoid filling up the SD card**. So if you want to keep it, copy or rename it before starting the script again. (can be renamed in the TX by browsing the SD card)

The log is human readable. The first number is the number of seconds since the start, and then what is the current state of the Library, and what is been sent and received. The info in the log can be easilly used to create a new simulation for that RX in the future.

Example Log:

    5.340 WAIT_CMD: DSM_GotoMenu(0x1010,LastSelectedLine=0)
    5.350 MENU_TITLE: SEND DSM_getMenu(MenuId=0x1010 LastSelectedLine=0)
    5.440 MENU_TITLE: RESPONSE Menu: M[Id=0x1010 P=0x0 N=0x0 B=0x1000 Text="Gyro settings"[0xF9]]
    5.490 MENU_LINES: SEND DSM_getFirstMenuLine(MenuId=0x1010)
    5.590 MENU_LINES: RESPONSE MenuLine: L[#0 T=M VId=0x1011 Text="AS3X Settings"[0x1DD]   MId=0x1010 ]
    5.640 MENU_LINES: SEND DSM_getNextLine(MenuId=0x1010,LastLine=0)
    5.740 MENU_LINES: RESPONSE MenuLine: L[#1 T=M VId=0x1019 Text="SAFE Settings"[0x1E2]   MId=0x1010 ]
    5.790 MENU_LINES: SEND DSM_getNextLine(MenuId=0x1010,LastLine=1)
    5.850 MENU_LINES: RESPONSE MenuLine: L[#2 T=M VId=0x1021 Text="F-Mode Setup"[0x87]   MId=0x1010 ]
    5.910 MENU_LINES: SEND DSM_getNextLine(MenuId=0x1010,LastLine=2)
    5.970 MENU_LINES: RESPONSE MenuLine: L[#3 T=M VId=0x1022 Text="System Setup"[0x86]   MId=0x1010 ]
    6.020 MENU_LINES: SEND DSM_getNextLine(MenuId=0x1010,LastLine=3

Exmple of the Unknown_0x05 Lines correctly processed (receiving lines 0..5):

    0.130 MENU_TITLE: SEND DSM_getMainMenu()
    0.230 MENU_TITLE: RESPONSE Menu: M[Id=0x1000 P=0x0 N=0x0 B=0x0 Text="Main Menu"[0x4B]]
    0.280 MENU_LINES: SEND DSM_getFirstMenuLine(MenuId=0x1000)
    0.400 MENU_LINES: RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.460 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=0)
    0.550 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=1  DATA=RX: 09 05 01 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.600 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=1)
    0.700 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=2  DATA=RX: 09 05 02 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.760 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=2)

Example when it will not be able to load the menu (gets stuck in line 0).
After nor advancing on the lines, it will show in the UI as `"Unable to load menu lines"`

    0.280 MENU_LINES: SEND DSM_getFirstMenuLine(MenuId=0x1022)
    0.400 MENU_LINES: RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.460 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=0)
    0.550 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.600 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=0)
    0.700 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 02 00 00 00 00 07 00 00 00 00 00 00 00 00
    0.760 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=0)


# Validation of data by the RX

When you change a value in the GUI, the RX validates that the value is valid.
For example, I ran into a case where trying to set Aux2 or Aux3 for flight mode, but the RX was correcting it to Aux1.. the RX only was allowing Gear or Aux1 (AR631/AR637).

If you go to the logs, you can see that the RX was correcting the value:

    20.520 VALUE_CHANGE_END: SEND DSM_updateMenuValue(ValueId=0x1000,val=7) Extra: Text="FM Channel" Value=7|"Aux2"
    20.570 VALUE_CHANGE_END: SEND DSM_validateMenuValue(ValueId=0x1000) Extra: Text="FM Channel" Value=7|"Aux2"
    20.680 VALUE_CHANGE_END: RESPONSE MenuValue: UPDATED: L[#0 T=L_m1 VId=0x1000 Text="FM Channel"[0x78] Val=6|"Aux1" NL=(0->32,0,S=53) [53->85,53] MId=0x7CA6 ]




---

# Version 0.5

- Make the code more readable and understadable
- Separate the DSM Forwards Programing logic from the GUI
- Log the comunnication with the RX on a /LOGS/dsm_log.txt to allow to debug it easier 
	and see the exchange of data between the RX/TX
- Created a black/white Text only version with only Key/Roller Inputs
- Created a nicer GUI for EdgeTX touchscreen color Radios
- RX simulation for GUI development:  turn on `SIMULATION_ON=true` in the beginning of the lua file
- Test it on AR631, AR637xx, FC6250HX (Helicopter)


### Some settings that can change (top of Lua file):
    SIMULATION_ON = false   -- FALSE: use real communication to DSM RX (DEFAULT), TRUE: use a simulated version of RX 
	DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm_log.txt)
	DEBUG_ON_LCD = false   -- Interactive Information on LCD of Menu data from RX 
	USE_SPECKTRUM_COLORS = true -- true: Use spectrum colors, false: use theme colors (default on OpenTX, OpenTX handle colors different) 


### Known Problems:
1. **Incorrect List Value Options:** Some Menu List line (`LINE_TYPE.LIST_MENU1` or `L_m1` in logs), the range (min/max) of valid values seems to be incorrect, but cannot see in the data how to fix it.
Some of the valid values are not even sequential, very spread apart. There has to be a list of valid options somewhere. Currently fixed some by overriding the valid values in the script code (config for each field).

2. **Unable to Load menu lines**: The RX return unknow lines when requesting menu lines. **Realy don't understand what they are for**. Some menus
seems to stay stuck in the same return line or no response to the request, making the RX reset/close the connection and terminate.
Was able to hack it for AR631/AR637 `"First Time Setup"`, `"First Time SAFE Setup"`, and `"Servo Realm"`.  Maybe this hack will work in other RX, so let us know if you get this problem.



# Version 0.2
Original Version from Pascal Langer
 

