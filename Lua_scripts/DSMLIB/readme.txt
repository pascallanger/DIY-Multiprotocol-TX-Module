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
- Created a black/white version with only Key/Roller Inputs (OTX)
- Created a nicer GUI for EdgeTX touchscreen color Radios

Known Problems:
1. When trying to Factory Reset an RX, even that navigation to menus seems OK, it did not reset. 
	Maybe another message needs to be sent to RX when reaching that page
2. When initially setting a new RX, there is a point where a menu navigates to MenuID=0x0001, this seems like a 
	special Save/Restart type of menu.. but it does not reset the RX. maybe another meesage needs to be send
3. Some Menu List line types (LINE_TYPE.LIST_MENU1), the range (min/max) seems to be incorrect, but cannot see in the data how to fix it
	Some of the valid values are not even sequential, very spread apart. There has to be a list of valid options somewhere (in RX or config for each field).
4. The RX return unknow lines when requesting the Lines for a menu. Realy don't understand what they are for.
	in some menus, seems to stay stuck in the same return line or no response to the request, making the RX reset/close the connection.
	Did a fix to stop requesting the same menu line if the response is the same. This gives an empty menu, but does not reset the connection.


Version 0.2
Original Version from Pascal Langer
 

