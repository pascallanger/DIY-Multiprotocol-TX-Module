# Credits
Code is based on the code/work by: Pascal Langer (Author of the Multi-Module) 
Rewrite/Enhancements by: Francisco Arzu

Thanks to all the people volunteered to test it.

# Introduction  (v0.53)

This script library enhances the original DSM Forward Programming tool. DSM Forward Programming is needed to setup many of the new Spektrum Receivers with Gyro AS3X/SAFE features. For the Gyro (/Safe) to correct the plane in flight, it needs to move the right surfaces therefore the RX needs to know the configuration of the plane (Wing Type, Tail Type, Mixers, Servo Assignments, Servo Reverse). That info tells the RX where the aileron(s) are (one or two), where the elevator(s) are (one or two),  V-Tail, Delta Wing, etc. 

Since EdgeTx/OpenTx doesn’t have an equivalent setup that is stored in the radio, we have to create our own version. This info is stored inside the `/MODELS/DSMDATA` directory/folder (which needs to be created by manually).

During `"Gyro Settings->initial setup"`, the RX asks the TX for model information behind the scenes.  After setup, `"Gyro Settings->System Tools-> Relearn Servo Settings"` requests the TX configuration and stores it in the RX. 

# Deployment
Make sure to manually create `/MODELS/DSMDATA`  . The script will complain at startup.

    /SCRIPTS/TOOLS/DsmFwdPrg_05_BW.lua      -- black/white text only radios
    /SCRIPTS/TOOLS/DsmFwdPrg_05_Color.lua   -- Color and touch radios
    /SCRIPTS/TOOLS/DSMLIB/      -- (ALL CAPITALS) Libraries ane extra files
    /SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lua   -- DSM Protocol Message and Menu   engine
    /SCRIPTS/TOOLS/DSMLIB/DsmFwPrgSIMLib.lua -- Simulation of AR631, FC6250HX
    /SCRIPTS/TOOLS/DSMLIB/SetupLib.lua -- Model Setup Screens
    /SCRIPTS/TOOLS/DSMLIB/img        --Images for RX orientations

Other Directories

/MODELS/DSMDATA                 --(ALL CAPITALS) Data of model config (Wing Type, Servo Assignments)
/LOGS/dsm_log.txt		       	--Readable log of the last RX/TX session, usefull for debugging problems

When upgrading from a previous version of this tool, delete your /SCRIPTS/TOOLS/DSMLIB before copying the new one (if you customized your images, inside "DSMLIB/img" do a backup first)

# Common Questions
1. `RX not accepting channels higher than Ch6 for Flight-mode o Gains:`
V0.53 improve this.. you can select any channel now. Additionally, if you already mapped the Switch to the channel, togling once the switch will select the channel on the menu field.  
<s>V0.52 and prior: The RX corrects your channel to ch5 or ch6. This means that the RX is not
detecting the upper channels from the TX. You need to exercise (move the switch) so that the RX detects it.   Put the Channel Field on edit (changing) mode, change it to Ch7 (or any other), flip the switch for Ch7 3 times, now confirm the edit. The RX now will not reject it.   All Spektrum RX are 20 channels internally, even if it only has 6 external Ch/Ports to connect servos. </s>

2. `Why Ch1 says Ch1 (TX:Ch3/Thr)?`:
 Radios with Multi-Module are usually configured to work the standard AETR convention. Spektrum uses TAER. The multi-module does the conversion when transmitting the signals. So `Spektrum Ch1 (Throttle)` really comes from the `TX Ch3`.  We show both information (+name from the TX output).  If your multi-module/radio is setup as TAER, the script will not do the re-arrangement.  

 3. `If i change the model name, the original model settings are lost.` This is correct, the model name is used to generate the file name (inside /MODEL/DSMDATA) who stores the model configuration. Currently EdgeTx and OpenTX has differt features where i could get either the Model Name or the YAML file where the EdgeTX model configuration is stored.. to keep the code compatible, the model name is used.

 4. `Reversing a channel in my TX do not reverse the AS3X/SAFE reaction.` Correct, the chanel stick direction and the Gyro direction are two separate things.

    4.1: First, you have setup your model so that the sticks and switches moves the surfaces in the right direction.
 
    4.2: Go to the script, `Model Setup` and setup your wing type, tail type, and select the channel assigment for each surface. Leave the servo settings the same as the values in the TX to start.
 
    4.3: AR63X family: Go to `Forward programming->Gyro Setting->Initial Setup` (New/factory reset), or `Forward programming->Gyro Setting->System Setup->Relearn Servo Settings` (not new). This will load your urrent Gyro servo settings into the plane's RX.
 
    4.4: Verify that the AS3X and SAFE reacts in the proper direction. You can use the Flight mode confugured as "Safe Mode: Auto-Level" to see if it moves the surfaces in the right direction.  
 
    4.5: If a surface don't move in the right direction, go to the `Model Setup->Gyro Channel Reverse` to reverse the Gyro on the channels needed, and do again the `Forward programming->Gyro Setting->System Setup->Relearn Servo Settings` to tranfer the new settings to the RX.

    4.6: Specktrum TX always passes the TX servo reverse as the Gyro Reverse, but on many OpenTX/EdgeTX radios, the Rud/Ail are usually reversed by default compared to Specktrum. So far i don't think that i can use this as a rule, that is why the `Gyro Channel Reverse` page exist. 
    


---
---

# Changes and fixes 
V0.53:
1. Improved channel selection (Flight mode, Panic Channel, Gains Channel). Now during editing a channel, you can select any channel (>Ch4). Also, of you toggle the switch/channel it will populate the screen.
2. Support for smaller screens (128x64) in B&W. The problem with this older radios is memory. In some, it does not have enouth memory to load the additional DSMLIB libraries.
3. Fix formatting problem with some TX channel names who could affect the screen.. for example, rud channel should show "Ch4/rud", but shows "Ch4ud" because /r is for right justify formatting on messages. Now the formatting is only if it appears at the end of the message.

V0.52:
1. Menus to be able to configure Plane in a similar way as Spektrum Radio (v0.52)
2. Make "Gyro Settings"->"Initial Setup" works (Tested on AR631,AR637xx with PLANE type of aircraft)
3. Properly reset and restart after initial configuration and SAFE changes.
4. Write Log of the conversation between RX/TX. To be used for debugging a problem is reported. 
5. Provide a simulation of RX to do GUI development in Companion, and understand patterns of how the data is organized.

# Tested RXs
- AR631/AR637xx
- FC6250HX (Blade 230S V2 Helicopter)
- AR636 (Blade 230S V1 Heli firmware 4.40)

Please report if you have tested it with other receivers to allow us to update the documentation. Code should work up to 10 channels for the main surfaces (Ail/Ele/etc).  All Spektrum RX are internally 20 channels, so you can use Ch7 for Flight Mode even if your RX is only 6 channels (See common Questions)

# Flight mode/Gain channels
Fixed in version 0.53. no longer a tick to select it.
<s>I ran into a case where trying to set Aux2 or Aux3 for flight mode, but the RX was correcting it to Aux1.. the RX only was allowing Gear or Aux1 (AR631/AR637).
This is because the RX doesn’t  know that we are using more than 6 channels. To make the RX aware that there are other channels, while editing the channel, you have to toggle the switch to exercise the channel (3 times), and now the RX will recognize it.</s>


# Messages Displayed in the GUI

If in a screen you get text that looks like `Unknown_XX` (ex: Unknown_D3), that message has not been setup in the script in english. If you can determine what the proper message is,  you can send us a message to be added to the library.
The `XX` represents a Hex Number (0..9,A..F)  message ID. 

If you want to fix it in your local copy, all messages are towards the end in the file `SCRIPT\TOOS\DSMLIB\DsmFwPrgLib.lua`. Messages for Headers are stored in `Text` and messages for Options are stored in `List_Text`.  Lua scripts are text files, and can be edited with Notepad or equivalent.

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

    -- Safe mode options, Inhibit + the values 
    local safeModeOptions = {0x0003,0x00B0,0x00B1}  -- inh (gap), "Self-Level/Angle Dem, Envelope
    List_Text[0x00B0] = "Self-Level/Angle Dem"
    List_Text[0x00B1] = "Envelope"

For example, if you get `Unknown_9D` in the GUI and your now that it should say **NEW Text**, you can edit the lua script to look like this:

    Text[0x009A] = "Capture Failsafe Positions"
    Text[0x009C] = "Custom Failsafe"

    Text[0x009D] = "NEW Text" -- NEW Text added for AR98xx

    Text[0x009F] = "Save & Reset RX"  -- TODO: Find the proper Spektrum text


# LOG File

The log file of the last use of the script is located at `/LOGS/dsm_log.txt`. **It is overridden on every start to avoid filling up the SD card**. So if you want to keep it, copy or rename it before starting the script again. (it can be renamed in the TX by browsing the SD card)

The log is human readable. The first number is the number of seconds since the start, and then what is the current state of the Library, and what has been sent and received. The info in the log can be easily used to create a new simulation for that RX in the future.

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

Example of the Unknown_0x05 Lines correctly processed (receiving lines 0..5):

    0.130 MENU_TITLE: SEND DSM_getMainMenu()
    0.230 MENU_TITLE: RESPONSE Menu: M[Id=0x1000 P=0x0 N=0x0 B=0x0 Text="Main Menu"[0x4B]]
    0.280 MENU_LINES: SEND DSM_getFirstMenuLine(MenuId=0x1000)
    0.400 MENU_LINES: RESPONSE MenuUknownLine_0x05: LineNum=0  DATA=RX: 09 05 00 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.460 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=0)
    0.550 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=1  DATA=RX: 09 05 01 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.600 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=1)
    0.700 MENU_UNKNOWN_LINES: RESPONSE MenuUknownLine_0x05: LineNum=2  DATA=RX: 09 05 02 01 00 00 00 07 00 00 00 00 00 00 00 00
    0.760 MENU_UNKNOWN_LINES: CALL DSM_getNextUknownLine_0x05(LastLine=2)


# Validation of data by the RX

<s>When you change a value in the GUI, the RX validates that the value is valid.
For example, I ran into a case where trying to set Aux2 or Aux3 for flight mode, but the RX was correcting it back to Aux1.. the RX only was allowing Gear or Aux1 (AR631/AR637).. in this case, toggle the Switch while editing it on the screen.

If you go to the logs, you can see that the RX was correcting the value:

    20.520 VALUE_CHANGE_END: SEND DSM_updateMenuValue(ValueId=0x1000,val=7) Extra: Text="FM Channel" Value=7|"Aux2"
    20.570 VALUE_CHANGE_END: SEND DSM_validateMenuValue(ValueId=0x1000) Extra: Text="FM Channel" Value=7|"Aux2"
    20.680 VALUE_CHANGE_END: RESPONSE MenuValue: UPDATED: L[#0 T=L_m1 VId=0x1000 Text="FM Channel"[0x78] Val=6|"Aux1" NL=(0->32,0,S=53) [53->85,53] MId=0x7CA6 ]
</s>

---
# Version 0.53
- Improve Channel selection in menus
- Support smaller screens 128x64 in the black/white mode.

# Version 0.52
- Fix Reversing of Servos
- Properly detect Multimodule Ch settings AETR 
---

# Version 0.51  (volunteer testing version, not for production)
- New Screens to Configure Model (Wing Type/Tail Tail, etc)
- Finally got understanding that the previous unknown 0x05 lines are to send Model/Servo data to RX.
- Fix use of AR636B (Firmware version 4.40.0 for Blade 230 heli, is the only one with Forward Programming)
- Aircraft types:  Tested With Plane type only.. Glider and other in progress

### Known Problems:
- 4-Servo Wing type (Dual Ail/Tail) in planes give conflicting servo assignments by defaults.. Solution choose your own Ch.
- Glider, Heli, Drone: Still in development. In glider, only a few wing type works.. needs to restrict menu options for the only valid one.


# Version 0.5

- Make the code more readable and understandable
- Separate the DSM Forwards Programming logic from the GUI
- Log the communication with the RX on a /LOGS/dsm_log.txt to allow to debug it easier 
	and see the exchange of data between the RX/TX
- Created a black/white Text only version with only Key/Roller Inputs
- Created a nicer GUI for EdgeTX touch screen color Radios
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

2. Glider/Heli/Drone wing types not ready.

For Helicopter, use airplane normal wing and normal tail


# Version 0.2
Original Version from Pascal Langer
 
