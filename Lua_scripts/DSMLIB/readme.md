# Credits
Code is based on the code/work by: Pascal Langer (Author of the Multi-Module) 
Rewrite/Enhancements by: Francisco Arzu

Thanks to all the people volunteered to test it.

# NOTE for FC6250HX FC+RX version
For the full size FC6250HX, Only use V0.55 or newer.

DO NOT use previous versions to do the Swashplate -> RX Orientation. The problem was that it did not have the orientation messages.. and you are choosing blind. The calibration will never stop until you place the RX in the right orientation, even after restarting the RX (if flashing red, is not in the right orientation.. if flashshing white is in the right orientation).  If you run into this problem, and lights are blinking red, rotate the FC on the longer axis until you get white blinking.. keep it stable, will blink white faster andlet calibration finishes.. after that is back to normal.

# Introduction  (v0.55)

This script library enhances the original DSM Forward Programming tool. DSM Forward Programming is needed to setup many of the new Spektrum Receivers with Gyro AS3X/SAFE features. For the Gyro (/Safe) to correct the plane in flight, it needs to move the right surfaces therefore the RX needs to know the configuration of the plane (Wing Type, Tail Type, Mixers, Servo Assignments, Servo Reverse). That info tells the RX where the aileron(s) are (one or two), where the elevator(s) are (one or two),  V-Tail, Delta Wing, etc. 

Since EdgeTx/OpenTx doesn’t have an equivalent setup that is stored in the radio, we have to create our own version. This info is stored inside the `/MODELS/DSMDATA` directory/folder (which needs to be created by manually).

During `"Gyro Settings->initial setup"`, the RX asks the TX for model information behind the scenes.  After setup, `"Gyro Settings->System Tools-> Relearn Servo Settings"` requests the TX servo configuration and stores it in the RX. 

# Deployment

When upgrading from a previous version of this tool, delete your /SCRIPTS/TOOLS/DSMLIB before copying the new one (if you customized your images, inside "DSMLIB/img" do a backup first)

Uncompress the Zip file (ZIP version) into your local computer.
In another window, open your TX SDCard.

1. The zip file has the same structure as your SDCard. If you want to copy all the content of the zip file into your SDCard top level folder, it will create all the directories and files in the right place.
2. Make sure to check that  `/MODELS/DSMDATA` is there. The script will complain at startup if it does not exist. Here the script saves the Spektrun settings for each of your models.

Your TX SDCard should looks like this:

    /SCRIPTS/TOOLS/     -- you only need one of the 3 to save some space in your TOOLS screen
        DSM FwdPrg_05_BW.lua      -- black/white text only 
        DSM FwdPrg_05_Color.lua   -- Color and touch radios
        DSM FwdPrg_05_MIN.lua     -- `NEW!` Minimalistic version for radios with LOW memory (cannot setup new planes)
        
    /SCRIPTS/TOOLS/DSMLIB/        -- (ALL CAPITALS) Libraries ane extra files
            DsmFwPrgLib.lua       -- DSM Protocol Message and Menu   engine
            DsmFwPrgSIMLib.lua    -- Simulation of AR631, FC6250HX  (For GUI development)
            SetupLib.lua          -- Model Setup Screens
            msg_fwdp_en.txt       -- `NEW!` Messages for forward programing externalized. To support other langs (english)
	    ... a few other files

    /SCRIPTS/TOOLS/DSMLIB/img                -- Images for RX orientations

Other Directories

    /MODELS/DSMDATA                 --(ALL CAPITALS) Data of model config (Wing Type, Servo Assignments)
    /LOGS/dsm_log.txt		    --Readable log of the last RX/TX session, usefull for debugging problems



# Common Questions
1. `RX not accepting channels higher than Ch6 for Flight-mode o Gains:`
- V0.55 and newer:  Problem solved.. Should allow you to select up to 12ch with the switch technique or with the scroller.

- V0.53/0.54:  The RX is listening to channel changes for this options. Configure the Switch to the channel, togling once the switch will select the channel on the menu field.  

2. `Only able to switch to Fligh-mode 2 and 3, but not 1:`
Check that the module "Enable max throw" is OFF in you Multi-Module settings (where you do BIND), otherwise the TX signals will be out of range.
The multi-module is already adjusting the TX/FrSky servo range internally to match Spektrum.

3. `Why Ch1 says Ch1 (TX:Ch3/Thr)?`:
 Radios with Multi-Module are usually configured to work the standard AETR convention. Spektrum uses TAER. The multi-module does the conversion when transmitting the signals. So `Spektrum Ch1 (Throttle)` really comes from the `TX Ch3`.  We show both information (+name from the TX output).  If your multi-module/radio is setup as TAER, the script will not do the re-arrangement.  

4. `If i change the model name, the original model settings are lost.` This is correct, the model name is used to generate the file name (inside /MODEL/DSMDATA) who stores the model configuration. Currently EdgeTx and OpenTX has differt features where i could get either the Model Name or the YAML file where the EdgeTX model configuration is stored.. to keep the code compatible, the model name is used.

5. `Reversing a channel in my TX do not reverse the AS3X/SAFE reaction.` Correct, the chanel stick direction and the Gyro direction are two separate things.

    5.1: First, you have setup your model so that the sticks and switches moves the surfaces in the right direction.
 
    5.2: Go to the script, `Model Setup` and setup your wing type, tail type, and select the channel assigment for each surface. Leave the servo settings the same as the values in the TX to start.
 
    5.3: Go to `Forward programming->Gyro Setting->Initial Setup` (New/factory reset), or `Forward programming->Gyro Setting->System Setup->Relearn Servo Settings` (not new). This will load your current Gyro servo settings into the plane's RX. This moves the current servo TX settings to the RX, so it is now in a known state.
 
    5.4: Verify that the AS3X and SAFE reacts in the proper direction. You can use the Flight mode configured as "Safe Mode: Auto-Level" to see if it moves the surfaces in the right direction.  
 
    5.5: If a surface don't move in the right direction, go to the `Model Setup->Gyro Channel Reverse` to reverse the Gyro on the channels needed, and do again the `Forward programming->Gyro Setting->System Setup->Relearn Servo Settings` to tranfer the new settings to the RX.

    5.6: Specktrum TX always passes the TX servo reverse as the Gyro Reverse, but on many OpenTX/EdgeTX radios, the Rud/Ail are usually reversed by default compared to Specktrum. So far i don't think that i can use this as a rule, that is why the `Gyro Channel Reverse` page exist. 
    


---
---

# Changes and fixes 
V0.55:
1. Finally found where the TX reports to the RX how many channels is transmiting. The TX now reports itself as a 12ch radio instead of 6h. (DSM Multi-Module limit).  This fixes a few things:
    
    
    a. Many places where you have to select channels > CH6 for Flight-Mode, Gains, Panic now works properly with the scroller. The radio is still validating that you are not selecting an invalid channel. For example, if you have an additional AIL on CH6, it will not allow you to use CH6 for FM or Gains.. it just move to the next valid one.

    b. When setting up AIL/ELE on channels greater than CH6, on previous versions SAFE/AS3X was not moving them.. now they work up correctly.  Set them up in the first in CH1-CH10.  Why CH10?? Thats what fits on the reverse screen, otherwise, have to add more screens.

    c. Some individual Gain channels was not allowing to setup on CH greater than CH6. Now is fixed.

2. User Interface:
    a. `RTN` Key now works as `Back` when the screen has a `Back`. Makes it easy for navigation.. Presing `RTN` on the main screen exists the tool.
    b. Much faster refresh of the menus. Optimize the process of send/recive menu data from the RX.

3. The TX now comunicates the SubTrim positions to the RX during `Relearn Servo Setting`. This changes the center of movement to one side or another. Really not much difference with small amounts of subtrim, previous versions where asuming subtrim of 0. When you have an extreame subtrim to one side, it was not moving simetrically.

4. Support for FC6250HX (the one with separate RX).. Setup Swashplate type, RX orientation works properly.. This are menu options that the smaller version that comes in the
Blade 230S did not have.


V0.54:
1. Fix a problem in the Attitude Trim page (`Gyro Settings->System Setup->SAFE/Panic Setup->Attitude Trim`). It was not saving the values after exiting the menu. This is to change what SAFE considers "Level" flying.
2. Wings 2-Ail 2-Flaps had a bug on the 2nd flap.
3. New Minimalistic script (`DsmFwdPrg_05_MIN.lua`): For radios with very low memory (FrSky QX7, RM Zorro, others). It can only change existing settings, but does not have the Plane Setup menus to setup a completly new plane.  In some radios, the very first time it runs (compile + run), it might give you a `not enouth memory` error.. try to run it again.
4. External menu message file (DSMLIB/msg_fwdp_en.txt and MIN_msg_fwdp_en.txt).  Intial work to do localization and different languages.

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


# Tested Hardware
- AR631/AR637xx
- FC6250HX (Blade 230S V2 Helicopter; FC+RX in one, mini version)
- FC6250HX (Separate RX.. use only V55 or newer of this tool)
- AR636 (Blade 230S V1 Heli firmware 4.40)

- Radiomaster TX16S  (All versions)

Please report if you have tested it with other receivers to allow us to update the documentation. Code should work up to 10 channels for the main surfaces (Ail/Ele/etc).  All Spektrum RX are internally 20 channels, so you can use Ch7 for Flight Mode even if your RX is only 6 channels (See common Questions)


# Messages Displayed in the GUI

If in a screen you get text that looks like `Unknown_XX` (ex: Unknown_D3), that message has not been setup in the script in english. If you can determine what the proper message is,  you can send us a message to be added to the library.
The `XX` represents a Hex Number (0..9,A..F)  message ID. 


### Version 0.53 and older: 
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

### Version 0.54 and newer:
The menu messages are stored in DSMLIB/msg_fwdp_en.txt (For english). Just add the message there. MIN_msg_fwdp_en.txt has shorter messages overrides for screens who are smaller (for minimalistic 128x64 version).   The reference to the message file is at the file `/DSMLIB/DsmFwPrgLib.lua` if you want to change to use another language.

    T |0x0097|Factory Reset
    LT|0x00B0|Self-Level/Angle Dem
    LT|0x00B1|Envelope


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

# Validation of data by the RX

The RX validates the data. if you change to an invalid channel or do a invalid number range, the RX will change it at the end of editing the field.

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
    SIMULATION_ON = false   -- FALSE: hide similation menu (DEFAULT), TRUE: show RX simulation menu 
	DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm_log.txt)
	USE_SPECKTRUM_COLORS = true -- true: Use spectrum colors, false: use theme colors (default on OpenTX, OpenTX handle colors different) 


### Known Problems:
1. **Incorrect List Value Options:** Some Menu List line (`LINE_TYPE.LIST_MENU1` or `L_m1` in logs), the range (min/max) of valid values seems to be incorrect, but the RX corrects the values.
in the MINimalistic version, the RX is doing all the range validation, and will show invalid options temporarilly. In an Spektrum radio, it happens so fast, that you don't notice it, but in LUA scripts who are slower, you can see it in the screen. 
In the COLOR version, The code has hardcoded the valid ranges to avoid this problem.

2. Glider/Heli/Drone wing types not ready.

For Helicopter, use airplane normal wing and normal tail


# Version 0.2
Original Version from Pascal Langer
 
