This script library is a rewrite of the original DSM forward programming Lua 
Script.  The goal is to make it easier to understand, mantain, and to  
separate the GUI from the DSM Forward  programming engine/logic
in this way, GUIs can evolve independent. OpenTX Gui, EdgeTx GUI, etc.

Code is based on the code/work by: Pascal Langer (Author of the Multi-Module) 
Rewrite/Enhancements By: Francisco Arzu 



Deployment:
/SCRIPTS/TOOLS/DsmFwdPrg_05_BaW.lua		-- OpenTX/EdgeTx version, no touch (black/white radios)
/SCRIPTS/TOOLS/DsmFwdPrg_05_Color.lua		-- EdgeTX/OpenTx Color version, +touch
/SCRIPTS/TOOLS/DSMLIB/				-- Libraries (ALL CAPITALS) to hide them from the Tools menu
/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lub		-- DSM Protocol Message and Menu engine
/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgSIMLib.lub	-- Simulation of AR631 for GUI Development in Companion

/LOGS/dsm_log.txt				-- Readable log of the last RX session, usefull for debuging new RX


Version 0.5
- Make the code more readable and understadable
- Separate the DSM Forwards Programing logic from the GUI
- Log the comunnication with the RX on a /LOGS/dsm_log.txt to allow to debug it easier 
	and see the exchange of data between the RX/TX
- Created a black/white Text only version with only Key/Roller Inputs
- Created a nicer GUI for EdgeTX touchscreen color Radios
- RX simulation for GUI development:  turn on SIMULATION_ON=true in the beginning of the lua file


Some settings that can change (top of Lua file):
	SIMULATION_ON = false   -- FALSE: use real communication to DSM RX (DEFAULT), TRUE: use a simulated version of RX 
	DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm_log.txt)
	DEBUG_ON_LCD = false   -- Interactive Information on LCD of Menu data from RX 
	USE_SPECKTRUM_COLORS = true -- true: Use spectrum colors, false: use theme colors (default on OpenTX, OpenTX handle colors different) 


Known Problems:
1. Some Menu List line types (LINE_TYPE.LIST_MENU1 or "L_m1" in logs), the range (min/max) seems to be incorrect, but cannot see in the data how to fix it
	Some of the valid values are not even sequential, very spread apart. There has to be a list of valid options somewhere (in RX or config for each field).
2. The RX return unknow lines when requesting the Lines for a menu. Realy don't understand what they are for.
	in some menus, seems to stay stuck in the same return line or no response to the request, making the RX reset/close the connection.
	Was able to hack it for AR631 "First Time Setup" and "First Time SAFE Setup", and "Servo Realm" (don't know if it works here)


Version 0.2
Original Version from Pascal Langer
 

