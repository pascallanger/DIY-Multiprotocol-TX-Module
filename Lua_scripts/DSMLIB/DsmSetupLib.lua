---- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX                                                  #
-----#                                                                       #
---- # License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html               #
---- #                                                                       #
---- # This program is free software; you can redistribute it and/or modify  #
---- # it under the terms of the GNU General Public License version 2 as     #
---- # published by the Free Software Foundation.                            #
---- #                                                                       #
---- # This program is distributed in the hope that it will be useful        #
---- # but WITHOUT ANY WARRANTY; without even the implied warranty of        #
---- # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
---- # GNU General Public License for more details.                          #
---- #                                                                       #
---- #########################################################################

------------------------------------------------------------------------------
-- This scrip does the airplane Setup similar to how a a Spektrum radio does
-- it. You can select the plane type, the Wing type, etc.
-- This settings are needed for ForwardProgramming to send the TX aircraft 
-- configuration to the RX when in Initial Setup
-- Author: Francisco Arzu 
------------------------------------------------------------------------------

local DEBUG_ON, SIMULATION_ON = ... -- Get DebugON from parameters
local SETUP_LIB_VERSION = "0.52"

local DATA_PATH = "/MODELS/DSMDATA" -- Path to store model settings files
local dsmLib = assert(loadScript("/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lua"))(DEBUG_ON)

local PHASE = dsmLib.PHASE
local LINE_TYPE = dsmLib.LINE_TYPE
local CH_TYPE   = dsmLib.CH_TYPE
local MODEL     = dsmLib.MODEL

local AIRCRAFT_TYPE = {
    PLANE   = 0,
    HELI    = 1,
    GLIDER  = 2,
    DRONE   = 3
}
local aircraft_type_text = {[0]="Plane","Heli","Glider","Drone"}

local WING_TYPE = {
    AIL_1       = 0, --1
    AIL_2       = 1, --2
    FLAPERON    = 2, --2
    AIL_1_FLP_1 = 3, --2
    AIL_2_FLP_1 = 4, --3
    AIL_2_FLP_2 = 5, --4
    ELEVON_A    = 6, --2
    ELEVON_B    = 7  --2
}
local wing_type_text = {[0]="Normal","Dual Ail","Flapperon", "Ail + Flp","Dual Ail + Flp","Dual Ail/Flp","Elevon A","Elevon B"}

local TAIL_TYPE = {
    RUD_1        = 0,  -- 1
    RUD_1_ELEV_1 = 1,  -- 2
    RUD_1_ELEV_2 = 2,  -- 3
    RUD_2_ELEV_1 = 3,  -- 3
    RUD_2_ELEV_2 = 4,  -- 4
    VTAIL_A      = 5,  -- 2
    VTAIL_B      = 6,  -- 2
    TRAILERON_A  = 7,  -- 3
    TRAILERON_B  = 8,  -- 3
}
local tail_type_text = {[0]="Rud Only","Normal","Rud + Dual Ele","Dual Rud + Elv","Dual Rud/Ele","VTail A","VTail B","Traileron A","Traileron B"}

local CH_MODE_TYPE = {
    NORMAL      = 0,
    REVERSE     = 1,
    USE_TX      = 3
}

local PORT = {
    PORT1 = 0,
    PORT2 = 1,
    PORT3 = 2,
    PORT4 = 3,
    PORT5 = 4,
    PORT6 = 5,
    PORT7 = 6,
    PORT8 = 7,
    PORT9 = 8,
    PORT10 = 9,
}

local MEMU_VAR = {
        AIRCRAFT_TYPE = 1001,
        WING_TYPE     = 1002,
        TAIL_TYPE     = 1003,
        
        CH_BASE       = 1010,
        CH_THR        = 1010,

        CH_L_AIL      = 1011,
        CH_R_AIL      = 1012,
        CH_L_FLP      = 1013,
        CH_R_FLP      = 1014,

        CH_L_RUD      = 1015,
        CH_R_RUD      = 1016,
        CH_L_ELE      = 1017,
        CH_R_ELE      = 1018,

        PORT_BASE       = 1020,
        PORT1_MODE      = 1020,
        PORT2_MODE      = 1021,
        PORT3_MODE      = 1022,
        PORT4_MODE      = 1023,
        PORT5_MODE      = 1024,
        PORT6_MODE      = 1025,
        PORT7_MODE      = 1026,
        PORT8_MODE      = 1027,
        PORT9_MODE      = 1028,
        PORT10_MODE     = 1029,

        DATA_END        = 1040
}

local SetupLib = {}

-- MENU DATA Management
local MENU_DATA = {}            -- Store the variables used in the Menus.
local menuDataChanged = false   -- Flag to notify if any data has changed
local currAircraftType = -1     -- Current AircraftType selected, and to detect change
local currTailType = -1         -- Current WingType selected, and to detect change
local currWingType = -1         -- Current TailType selected, and to detect change

local lastGoodMenu=0            

-------------------  Model Setup Helper functions ----------------------

local function printChannelSummary()
    -- Summary
    print("CHANNEL INFORMATION")
    print("Aircraft:".. (aircraft_type_text[currAircraftType] or "--"))
    print("Wing Type:".. (wing_type_text[currWingType] or "--"))
    print("Tail Type:".. (tail_type_text[currTailType] or "--"))
    print("Thr:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_THR]  or 30)] or "--"))   -- use fake ch30 for non existing channels 
    print("LAil:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_AIL] or 30)] or "--"))
    print("RAil:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_AIL] or 30)] or "--"))
    print("LFlp:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_FLP] or 30)] or "--"))
    print("RFlp:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_FLP] or 30)] or "--"))
    print("LEle:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_ELE] or 30)] or "--"))
    print("REle:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_ELE] or 30)] or "--"))
    print("LRud:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_RUD] or 30)] or "--"))
    print("RRud:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_RUD] or 30)] or "--"))   
end

local function printServoReverseInfo()
    print("SERVO Normal/Reverse INFORMATION")
    for i=0,10 do
        local s="--"
        if (MENU_DATA[MEMU_VAR.PORT1_MODE+i] or 0) == 0 then s="NORMAL" else s="REVERSE" end
        print(string.format("Port%d:  %s", i+1, s))
    end
end

local function ST_PlaneWingInit(wingType) 
    print("Change Plane WingType:"..wing_type_text[wingType])

    currWingType = wingType

    MENU_DATA[MEMU_VAR.WING_TYPE] = wingType

    -- Clear all Wing Data 
    MENU_DATA[MEMU_VAR.CH_L_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_R_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_L_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_R_FLP] = nil

    MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT1
    
    -- Default Channel Assisgments for each Wing type

    if (wingType==WING_TYPE.AIL_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.FLAPERON) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_1_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT6
    elseif (wingType==WING_TYPE.AIL_2_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
    elseif (wingType==WING_TYPE.AIL_2_FLP_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT7
    elseif (wingType==WING_TYPE.ELEVON_A) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT3
    elseif (wingType==WING_TYPE.ELEVON_B) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    else -- Assume normal 
       print("ERROR: Invalid Wing Type")
    end 


    printChannelSummary()
end

local function ST_PlaneTailInit(tailType) 
    if (MENU_DATA[MEMU_VAR.WING_TYPE]==WING_TYPE.ELEVON_A) then
        tailType = TAIL_TYPE.RUD_1 -- Delta only have ruder  
    end

    print("Change Plane Tail Type:"..tail_type_text[tailType])

    currTailType = tailType

    -- Clear all data for Tail 
    MENU_DATA[MEMU_VAR.TAIL_TYPE] = tailType
    MENU_DATA[MEMU_VAR.CH_L_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_R_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_L_RUD]  = nil
    MENU_DATA[MEMU_VAR.CH_R_RUD] = nil

    -- Setup Channels for different Tail types 
    if (tailType == TAIL_TYPE.RUD_1) then
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_2) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_2_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_2_ELEV_2) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.VTAIL_A) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]  = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    elseif (tailType == TAIL_TYPE.VTAIL_B) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]   = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_R_ELE]  = PORT.PORT4
    elseif (tailType == TAIL_TYPE.TRAILERON_A) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]  = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    elseif (tailType == TAIL_TYPE.TRAILERON_B) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]   = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE]  = PORT.PORT3
    else -- Assume Normal 
        print("ERROR:invalid Tail Type")
    end

    printChannelSummary()
end

local function ST_GliderWingInit(wingType) 
    print("Change Glider WingType:"..wing_type_text[wingType])

    currWingType = wingType

    MENU_DATA[MEMU_VAR.WING_TYPE] = wingType

    -- Clear all Wing Data 
    MENU_DATA[MEMU_VAR.CH_L_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_R_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_L_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_R_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT6
    
    -- Default Channel Assisgments for each Wing type

    if (wingType==WING_TYPE.AIL_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
    elseif (wingType==WING_TYPE.AIL_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_2_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
    elseif (wingType==WING_TYPE.AIL_2_FLP_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_FLP] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT7
    elseif (wingType==WING_TYPE.ELEVON_A) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.ELEVON_B) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT1
    else -- Assume normal 
        print("ERROR: Invalid Wing Type")
    end 

    printChannelSummary()
end

local function ST_GliderTailInit(tailType) 
    if (MENU_DATA[MEMU_VAR.WING_TYPE]==WING_TYPE.ELEVON_A) then
        tailType = TAIL_TYPE.RUD_1 -- Delta only have ruder  
    end

    print("Change Glider Tail Type:"..tail_type_text[tailType])

    currTailType = tailType

    -- Clear all data for Tail 
    MENU_DATA[MEMU_VAR.TAIL_TYPE] = tailType
    MENU_DATA[MEMU_VAR.CH_L_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_R_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_L_RUD]  = nil
    MENU_DATA[MEMU_VAR.CH_R_RUD] = nil

    -- Setup Channels for different Tail types 
    if (tailType == TAIL_TYPE.RUD_1) then
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.VTAIL_A) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]  = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    elseif (tailType == TAIL_TYPE.VTAIL_B) then
        MENU_DATA[MEMU_VAR.CH_L_ELE]   = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_R_ELE]  = PORT.PORT4
    else -- Assume Normal 
        print("ERROR: Invalid Tail Type")
    end

    printChannelSummary()
end


local function ST_AircraftInit(aircraftType)
    MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE] = aircraftType
    currAircraftType = aircraftType

    print("Change Aircraft:".. aircraft_type_text[aircraftType])

    -- Setup Default Aircraft Wing/Tail 
    if (aircraftType==AIRCRAFT_TYPE.PLANE) then
        ST_PlaneWingInit(WING_TYPE.AIL_1)
        ST_PlaneTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    elseif (aircraftType==AIRCRAFT_TYPE.GLIDER) then
        ST_GliderWingInit(WING_TYPE.AIL_1)
        ST_GliderTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    else 
        ST_PlaneWingInit(WING_TYPE.AIL_1)
        ST_PlaneTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    end

   
end


-- Setup Initial Default Data for the Menus
local function ST_Default_Data()
    print("Initializing Menu DATA")
    ST_AircraftInit(AIRCRAFT_TYPE.PLANE)

    print("Initializing Servo Reverse from TX output settings")

    MENU_DATA[MEMU_VAR.PORT1_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT1].revert
    MENU_DATA[MEMU_VAR.PORT2_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT2].revert
    MENU_DATA[MEMU_VAR.PORT3_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT3].revert
    MENU_DATA[MEMU_VAR.PORT4_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT4].revert
    MENU_DATA[MEMU_VAR.PORT5_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT5].revert
    MENU_DATA[MEMU_VAR.PORT6_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT6].revert
    MENU_DATA[MEMU_VAR.PORT7_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT7].revert
    MENU_DATA[MEMU_VAR.PORT8_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT8].revert
    MENU_DATA[MEMU_VAR.PORT9_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT9].revert
    MENU_DATA[MEMU_VAR.PORT10_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT10].revert

    printServoReverseInfo()

end

-----------------------  FILE MANAGEMENT ---------------------------------------------
-- Create a fairly unique name for a model..combination of name and a hash 
-- TODO: Check with ETX why we can't get the filename used to store the model info
-- Improvement request??

local function hashName(mName)
    local c=10000;

    local prefix = string.gsub(mName,"%.","_") -- Change any "." to "_"
    prefix = string.gsub(prefix,"% ","_") -- Change any space to "_"
    prefix = string.sub(prefix,1,5) -- Take the first 5 characters

    -- Simple Hash of the Model Name adding each character 
    for i = 1, #mName do
        local ch = string.byte(mName,i,i)
        c=c+ch
    end

    return (prefix .. c) -- Return Prefix + Hash
end

-- Load Menu Data from a file 
function ST_LoadFileData() 
    local fname = hashName(MODEL.modelName)..".txt"

    print("Loading File:"..fname)

    local dataFile = io.open(DATA_PATH .. "/".. fname, "r")  -- read File 
    -- cannot read file???
    if (dataFile==nil) then return 0 end

    local line = io.read(dataFile, 5000)
    io.close(dataFile)

    if #line == 0 then return 0 end -- No data??

    -- Process the input, each line is "Var_Id : Value" format 
    -- Store it into MANU_DATA
    local i=0
    for k, v in string.gmatch(line, "(%d+):(%d+)") do
        --print(string.format("Read  MENU_DATA[%d]:[%d]",k, v))
        MENU_DATA[k+0]=v+0 -- do aritmentic to convert string to number
        i=i+1
    end

    -- Get the basic info
    currAircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
    currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
    currTailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

    print("Validation")
    print(string.format("AIRCRAFT_TYPE(%d)=%s", MEMU_VAR.AIRCRAFT_TYPE,aircraft_type_text[currAircraftType]))
    print(string.format("WING_TYPE(%d)=%s", MEMU_VAR.WING_TYPE, wing_type_text[currWingType]))
    print(string.format("TAIL_TYPE(%d)=%s", MEMU_VAR.TAIL_TYPE, tail_type_text[currTailType]))
   
    printChannelSummary()
    printServoReverseInfo()


    -- No need to save right now
    menuDataChanged = false

    -- Return 0 if no lines processed, 1 otherwise
    if (i > 0) then return 1 else return 0 end
end

-- Saves MENU_DATA to a file
function ST_SaveFileData() 
    local fname = hashName(MODEL.modelName)..".txt"

    print("Saving File:"..fname)
    local dataFile = assert(io.open(DATA_PATH .. "/" .. fname, "w"),"Please create "..DATA_PATH.." folder")  -- write File 
    
    -- Foreach MENU_DATA with a value write Var_Id:Value into file
    for i = 0, MEMU_VAR.DATA_END do
        if (MENU_DATA[i]~=nil) then
            --print(string.format("Write MENU_DATA[%s] : %s",i,MENU_DATA[i]))
            io.write(dataFile,string.format("%s:%s\n",i,MENU_DATA[i]))
        end
    end
    io.close(dataFile)
    menuDataChanged = false
end

-- This Creates the Servo Settings that will be used to pass to 
-- Forward programming
local function CreateDSMPortChannelInfo()
    local function ApplyWingMixA(b2)
        -- ELEVON
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return 0x20 end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x50 end; -- 0x23
       
       -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x00 end; -- 0x83
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x70 end; -- 0xA3
    end

    local function ApplyWingMixB(b2)
        -- ELEVON 
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return 0x00 end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x70 end; -- 0x23

        -- Difference with B 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x20 end; -- 0x83
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x50 end; -- 0xA3
   end

    local function ApplyTailMixA(b2)
        -- VTAIL
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE) then return 0x00 end; -- 0x06
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x70 end; -- 0x26

        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x20 end; -- 0x86
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x50 end; -- 0xA6

        --TRAILERON
         -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return 0x00 end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x70 end; -- 0x23

        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x10 end; -- 0x83
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x60 end; -- 0xA3

    end

    local function ApplyTailMixB(b2)
        -- VTAIL 
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE) then return 0x00 end; -- 0x06
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x70 end; -- 0x26

        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x40 end; -- 0x86
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x30 end; -- 0xA6

        --TAILERON
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return 0x10 end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.REVERSE) then return 0x60 end; -- 0x23

        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return 0x00 end; -- 0x83
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE+CH_TYPE.REVERSE) then return 0x70 end; -- 0xA3
    end



    local DSM_ChannelInfo = {} 

    for i=0, 9 do
        DSM_ChannelInfo[i] = {[0]= 0x00, CH_TYPE.NONE} -- Initialize with no special function 
    end

    local wingType = MENU_DATA[MEMU_VAR.WING_TYPE]
    local tailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

    local thrCh  =  MENU_DATA[MEMU_VAR.CH_THR]
    local lAilCh =  MENU_DATA[MEMU_VAR.CH_L_AIL]
    local rAilCh =  MENU_DATA[MEMU_VAR.CH_R_AIL]
    local lflapCh = MENU_DATA[MEMU_VAR.CH_L_FLP]
    local rflapCh = MENU_DATA[MEMU_VAR.CH_R_FLP]

    local lElevCh = MENU_DATA[MEMU_VAR.CH_L_ELE]
    local rElevCh = MENU_DATA[MEMU_VAR.CH_R_ELE]

    local lRudCh = MENU_DATA[MEMU_VAR.CH_L_RUD]
    local rRudCh = MENU_DATA[MEMU_VAR.CH_R_RUD]

    -- Channels in menu vars are Zero base, Channel info is 1 based 
    
    -- THR 
    if (thrCh~=nil) then DSM_ChannelInfo[thrCh][1]= CH_TYPE.THR end

    -- AIL (Left and Right)
    if (lAilCh~=nil) then DSM_ChannelInfo[lAilCh][1] = CH_TYPE.AIL  end
    if (rAilCh~=nil) then DSM_ChannelInfo[rAilCh][1] = CH_TYPE.AIL+CH_TYPE.SLAVE end
    -- ELE (Left and Right)
    if (lElevCh~=nil) then DSM_ChannelInfo[lElevCh][1] = CH_TYPE.ELE end
    if (rElevCh~=nil) then DSM_ChannelInfo[rElevCh][1] = CH_TYPE.ELE+CH_TYPE.SLAVE end
    -- RUD (Left and Right)
    if (lRudCh~=nil) then DSM_ChannelInfo[lRudCh][1] = CH_TYPE.RUD end
    if (rRudCh~=nil) then DSM_ChannelInfo[rRudCh][1] = CH_TYPE.RUD+CH_TYPE.SLAVE end

    -- VTAIL: RUD + ELE
    if (tailType==TAIL_TYPE.VTAIL_A) then 
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE
    elseif (tailType==TAIL_TYPE.VTAIL_B) then
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE
    end

    -- TRAILERRON: 2-ELE + AIL
    if (tailType==TAIL_TYPE.TRAILERON_A) then 
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
    elseif (tailType==TAIL_TYPE.TRAILERON_B) then
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
    end

    ---- ELEVON :  AIL + ELE 
    if (wingType==WING_TYPE.ELEVON_A) then 
        DSM_ChannelInfo[lAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
        DSM_ChannelInfo[rAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
    elseif (wingType==WING_TYPE.ELEVON_B) then
        DSM_ChannelInfo[lAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
        DSM_ChannelInfo[rAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
    end

    -- Apply Gyro Reverse as needed for each channel as long as it is used 
    for i=0, 9 do
        if (MENU_DATA[MEMU_VAR.PORT_BASE+i]==CH_MODE_TYPE.REVERSE and DSM_ChannelInfo[i][1]>0) then
            DSM_ChannelInfo[i][0]=DSM_ChannelInfo[i][1]+0x70 -- ALL REVERSE is 0x70 for normal
            DSM_ChannelInfo[i][1]=DSM_ChannelInfo[i][1]+CH_TYPE.REVERSE
        end
    end

    -- VTAIL: RUD + ELE
    if (tailType==TAIL_TYPE.VTAIL_A) then 
        DSM_ChannelInfo[lElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[rElevCh][1])
    elseif (tailType==TAIL_TYPE.VTAIL_B) then
        DSM_ChannelInfo[lElevCh][0] = ApplyTailMixB(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][0] = ApplyTailMixB(DSM_ChannelInfo[rElevCh][1])
    end

    -- TRAILERRON: ELE + AIL
    if (tailType==TAIL_TYPE.TRAILERON_A) then 
        DSM_ChannelInfo[lElevCh][1] = ApplyTailMixA(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][1] = ApplyTailMixA(DSM_ChannelInfo[rElevCh][1])
    elseif (tailType==TAIL_TYPE.TRAILERON_B) then
        DSM_ChannelInfo[lElevCh][1] = ApplyTailMixB(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][1] = ApplyTailMixB(DSM_ChannelInfo[rElevCh][1])
    end

     ---- ELEVON :  AIL + ELE 
     if (wingType==WING_TYPE.ELEVON_A) then 
        DSM_ChannelInfo[lAilCh][0] = ApplyWingMixA(DSM_ChannelInfo[lAilCh][1])
        DSM_ChannelInfo[rAilCh][0] = ApplyWingMixA(DSM_ChannelInfo[rAilCh][1])
    elseif (wingType==WING_TYPE.ELEVON_B) then
        DSM_ChannelInfo[lAilCh][0] = ApplyWingMixB(DSM_ChannelInfo[lAilCh][1])
        DSM_ChannelInfo[rAilCh][0] = ApplyWingMixB(DSM_ChannelInfo[rAilCh][1])
    end

    -- Show how it looks
    for i=0, 9 do
        local b1,b2 =  DSM_ChannelInfo[i][0], DSM_ChannelInfo[i][1]
        print(string.format("%s (%02X %02X)  %s", MODEL.PORT_TEXT[i],
             b1, b2, dsmLib.channelType2String(b1,b2)))
    end

    return DSM_ChannelInfo, string.format("Aircraft(%s) Wing(%s) Tail(%s)",aircraft_type_text[currAircraftType],wing_type_text[wingType],tail_type_text[tailType])
end

---- Memu Processing Helpers to Mimic the DSM RX menu behaviour

local function ST_StartConnection()
    return 0
end

local function ST_ReleaseConnection()
end

-- Clear each line of the menu 
local function clearMenuLines() 
    local ctx = dsmLib.DSM_Context
    for i = 0, dsmLib.MAX_MENU_LINES do -- clear menu
        ctx.MenuLines[i] = { MenuId = 0, lineNum = 0, Type = 0, Text = "", TextId = 0, ValId = 0, Min=0, Max=0, Def=0, TextStart=0, Val=nil }
    end
end

-- Post processing needed for each menu 
local function PostProcessMenu()
    local ctx = dsmLib.DSM_Context

    if (ctx.Menu.Text==nil) then
        ctx.Menu.Text = dsmLib.Get_Text(ctx.Menu.TextId)
        dsmLib.MenuPostProcessing (ctx.Menu)
    end

    if (DEBUG_ON) then dsmLib.LOG_write("SIM RESPONSE Menu: %s\n", dsmLib.menu2String(ctx.Menu)) end

    for i = 0, dsmLib.MAX_MENU_LINES do -- clear menu
        local line = ctx.MenuLines[i]
        if (line.Type~=0) then
            line.MenuId = ctx.Menu.MenuId
            line.lineNum = i
            dsmLib.MenuLinePostProcessing(line) -- Do the same post processing as if they come from the RX
            if (DEBUG_ON) then dsmLib.LOG_write("SIM RESPONSE MenuLine: %s\n", dsmLib.menuLine2String(line))  end
        end

    end
end

-- Creates the menus to Render with the GUI
local function ST_LoadMenu(menuId)
    local ctx = dsmLib.DSM_Context

    local function formatTXRevert(port)
        return ((MODEL.modelOutputChannel[port].revert==0 and "  (Tx:Normal)") or "  (Tx:Reverse)")
    end

    clearMenuLines()

    if (menuId==0x1000) then -- MAIN MENU
        ctx.Menu = { MenuId = 0x1000, Text = "Main Menu ("..MODEL.modelName..")", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, Text = "Model Setup", ValId = 0x1001,TextId=0 }
       
        if (menuDataChanged) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, Text="Save Changes", TextId = 0, ValId = 0x1005 }
            ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text="Discart Changes", TextId = 0, ValId = 0x1006 }
            ctx.SelLine = 4 
        else
            if (SIMULATION_ON) then
                ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, Text = "RX Simulator (GUI dev only)", ValId = 0xFFF1, TextId=0 } -- Menu 0xFFF2 to SIMULATOR  
            end
            ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text = "Forward Programming RX", ValId = 0xFFF2, TextId=0 }  -- Menu 0xFFF2 to Real RX 
            ctx.SelLine = 6 
        end
        lastGoodMenu = menuId
    elseif (menuId==0x1001) then -- MODEL SETUP
        ctx.Menu = { MenuId = 0x1001, Text = "Model Setup  ("..MODEL.modelName..")", PrevId = 0, NextId = 0, BackId = 0x1000, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, Text = "Aircraft Type Setup", ValId = 0x1010,TextId=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, Text = "Wing & Tail Channels ", ValId = 0x1020, TextId=0 } 
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text = "Gyro Channel Reverse", ValId = 0x1030, TextId=0 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text = "WARNING: Changing of Aircraft or Wing will", ValId = 0x1001, TextId=0 } 
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text = "delete previous Channel/Port assigments.", ValId = 0x1001, TextId=0 } 

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1005) then
        printChannelSummary()
        ST_SaveFileData()


        local msg1 = "Data saved to: " 
        local msg2 = DATA_PATH..hashName(MODEL.modelName)..".txt" 

        ctx.Menu = { MenuId = 0x1005, Text = "Config Saved", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, Text=msg1, TextId = 0, ValId = 0x1005 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text=msg2, TextId = 0, ValId = 0x1005 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Complete", TextId = 0, ValId = 0x1000 }
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1006) then
        ST_LoadFileData()
        local msg1 = "Data restored from: " 
        local msg2 = DATA_PATH..hashName(MODEL.modelName)..".txt" 
        
        ctx.Menu = { MenuId = 0x1006, Text = "Discart Changes", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, Text=msg1, TextId = 0, ValId = 0x1006 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text=msg2, TextId = 0, ValId = 0x1006 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Complete", TextId = 0, ValId = 0x1000 }
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1010) then
        ctx.Menu = { MenuId = 0x1010, Text = "Aircraft Type", PrevId = 0x1001, NextId = 0x1011, BackId = 0, TextId=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Aircraft Type", TextId = 0, ValId = MEMU_VAR.AIRCRAFT_TYPE, Min=50, Max=53, Def=50, Val=MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE] }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1011) then
        ctx.Menu = { MenuId = 0x1011, Text = "Model Type:"..aircraft_type_text[currAircraftType], PrevId = 0x1010, NextId = 0x1020, BackId = 0, TextId=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Wing Type", TextId = 0, ValId = MEMU_VAR.WING_TYPE, Min=100, Max=107, Def=100, Val=MENU_DATA[MEMU_VAR.WING_TYPE] }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Tail Type", TextId = 0, ValId = MEMU_VAR.TAIL_TYPE, Min=200, Max=208, Def=200, Val=MENU_DATA[MEMU_VAR.TAIL_TYPE] }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1020) then
        ------ WING  SETUP -------
        local thr = MENU_DATA[MEMU_VAR.CH_THR]
        local leftAil = MENU_DATA[MEMU_VAR.CH_L_AIL]
        local rightAil = MENU_DATA[MEMU_VAR.CH_R_AIL]
        local leftFlap = MENU_DATA[MEMU_VAR.CH_L_FLP]
        local rightFlap = MENU_DATA[MEMU_VAR.CH_R_FLP]

        local thrText     = "Thr"
        local leftAilText = "Left Aileron"
        local rightAilText = "Right Aileron"
        local leftFlapText = "Left Flap"
        local rightFlapText = "Right Flap"

        if (rightAil==nil) then leftAilText = "Aileron" end
        if (rightFlap==nil) then leftFlapText = "Flap" end

        local title = aircraft_type_text[currAircraftType].."   Wing:"..wing_type_text[currWingType]

        ctx.Menu = { MenuId = 0x1020, Text = title, PrevId = 0x1011, NextId = 0x1021, BackId = 0, TextId=0 }

        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=thrText, TextId = 0, ValId = MEMU_VAR.CH_THR, Min=0, Max=9, Def=0, Val= thr }

        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftAilText, TextId = 0, ValId = MEMU_VAR.CH_L_AIL, Min=0, Max=9, Def=0, Val= leftAil }

        if (rightAil~=nil) then
            ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightAilText, TextId = 0, ValId = MEMU_VAR.CH_R_AIL, Min=0, Max=9, Def=0, Val= rightAil }
        end

        if (leftFlap~=nil) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftFlapText, TextId = 0, ValId = MEMU_VAR.CH_L_FLP, Min=0, Max=9, Def=0, Val= leftFlap }
        end
        if (rightFlap~=nil) then
            ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightFlapText, TextId = 0, ValId = MEMU_VAR.CH_L_FLP, Min=0, Max=9, Def=0, Val= leftFlap }
        end

        ctx.SelLine = 1
        lastGoodMenu = menuId

    elseif (menuId==0x1021) then
        ------ TAIL SETUP -------
        local leftRud = MENU_DATA[MEMU_VAR.CH_L_RUD] 
        local rightRud = MENU_DATA[MEMU_VAR.CH_R_RUD] 
        local leftEle = MENU_DATA[MEMU_VAR.CH_L_ELE]
        local rightEle = MENU_DATA[MEMU_VAR.CH_R_ELE]

        local leftRudText = "Left Rudder"
        local rightRudText = "Right Rudder"

        local leftElvText = "Left Elevator"
        local rightElvText = "Right Elevator"

        if (rightRud==nil) then leftRudText = "Rudder"  end
        if (rightEle==nil) then leftElvText = "Elevator" end

        local title = aircraft_type_text[currAircraftType].."  Tail:"..tail_type_text[currTailType]

        ctx.Menu = { MenuId = 0x1021, Text = title, PrevId = 0x1020, NextId = 0x1001, BackId = 0, TextId=0 }
        if (leftRud~=nil) then
            ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftRudText, TextId = 0, ValId = MEMU_VAR.CH_L_RUD, Min=0, Max=9, Def=0, Val= leftRud}
        end

        if (rightRud~=nil) then
            ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightRudText, TextId = 0, ValId = MEMU_VAR.CH_R_RUD, Min=0, Max=9, Def=0, Val=rightRud }
        end

        if (leftEle~=nil) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftElvText, TextId = 0, ValId = MEMU_VAR.CH_L_ELE, Min=0, Max=9, Def=0, Val=leftEle }
        end

        if (rightEle~=nil) then
            ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightElvText, TextId = 0, ValId = MEMU_VAR.CH_R_ELE, Min=0, Max=9, Def=0, Val=rightEle }
        end

        ctx.SelLine = 1
        lastGoodMenu = menuId
   
    elseif (menuId==0x1030) then
        printChannelSummary()
        
        ctx.Menu = { MenuId = 0x1030, Text = "Gyro Channel Reverse (Port 1-5)", PrevId = 0, NextId = 0x1031, BackId = 0x1001, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT1], TextId = 0, ValId = MEMU_VAR.PORT1_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT1_MODE], Format = formatTXRevert(PORT.PORT1) }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT2], TextId = 0, ValId = MEMU_VAR.PORT2_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT2_MODE], Format = formatTXRevert(PORT.PORT2) }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT3], TextId = 0, ValId = MEMU_VAR.PORT3_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT3_MODE], Format = formatTXRevert(PORT.PORT3) }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT4], TextId = 0, ValId = MEMU_VAR.PORT4_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT4_MODE], Format = formatTXRevert(PORT.PORT4) }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT5], TextId = 0, ValId = MEMU_VAR.PORT5_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT5_MODE], Format = formatTXRevert(PORT.PORT5) }

        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="      Usually Rud/Ail needs to be the oposite of the TX", TextId = 0, ValId = 0x1030 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1031) then
        printChannelSummary()
        ctx.Menu = { MenuId = 0x1031, Text = "Gyro Channel Reverse (Port 6-10)", PrevId = 0x1030, NextId = 0, BackId = 0x1001, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT6], TextId = 0, ValId = MEMU_VAR.PORT6_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT6_MODE], Format = formatTXRevert(PORT.PORT6) }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT7], TextId = 0, ValId = MEMU_VAR.PORT7_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT7_MODE], Format = formatTXRevert(PORT.PORT7) }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT8], TextId = 0, ValId = MEMU_VAR.PORT8_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT8_MODE], Format = formatTXRevert(PORT.PORT8) }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT9], TextId = 0, ValId = MEMU_VAR.PORT9_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT9_MODE], Format = formatTXRevert(PORT.PORT9) }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=MODEL.PORT_TEXT[PORT.PORT10], TextId = 0, ValId = MEMU_VAR.PORT10_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT10_MODE], Format = formatTXRevert(PORT.PORT10) }
     
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="      Usually Rud/Ail needs to be the oposite of the TX", TextId = 0, ValId = 0x1031 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    else
        print("NOT IMPLEMENTED")
        ctx.Menu = { MenuId = 0x0002, Text = "NOT IMPLEMENTED", TextId = 0, PrevId = 0, NextId = 0, BackId = lastGoodMenu }
        ctx.SelLine = dsmLib.BACK_BUTTON
    end

    PostProcessMenu()
end

-- ST_SendReceive
-- Main state machine for the Setup menu

local function ST_SendReceive()
    local ctx = dsmLib.DSM_Context
    --if (DEBUG_ON) then dsmLib.LOG_write("%3.3f %s: ", dsmLib.getElapsedTime(), dsmLib.phase2String(ctx.Phase)) end

    if ctx.Phase == PHASE.RX_VERSION then -- request RX version
        ctx.RX.Name = "MODEL SETUP"
        ctx.RX.Version = SETUP_LIB_VERSION
        ctx.Phase = PHASE.MENU_TITLE

        ctx.Refresh_Display = true

        
    elseif ctx.Phase == PHASE.WAIT_CMD then 
        
    elseif ctx.Phase == PHASE.MENU_TITLE then -- request menu title
        if ctx.Menu.MenuId == 0 then  -- First time loading a menu ?
            ST_LoadMenu(0x01000)
        else
            ST_LoadMenu(ctx.Menu.MenuId)
        end
        ctx.Phase = PHASE.WAIT_CMD
        ctx.Refresh_Display = true

    elseif ctx.Phase == PHASE.VALUE_CHANGING then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
       
        if (MENU_DATA[line.ValId] ~= line.Val ) then
            MENU_DATA[line.ValId] = line.Val 
            print(string.format("MENU_DATA[%d/%s]=%d",line.ValId,line.Text, line.Val))
            menuDataChanged=true
        end

        ctx.Phase = PHASE.VALUE_CHANGING_WAIT

    elseif ctx.Phase == PHASE.VALUE_CHANGING_WAIT then
        local line = ctx.MenuLines[ctx.SelLine]

    elseif ctx.Phase == PHASE.VALUE_CHANGE_END then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
        
        -- Update the menu data from the line
        if (MENU_DATA[line.ValId] ~= line.Val ) then
            MENU_DATA[line.ValId] = line.Val 
            print(string.format("MENU_DATA[%d/%s]=%d",line.ValId,line.Text, line.Val))
            menuDataChanged=true
        end

        -- Did the aircraft type change?
        if (currAircraftType ~= MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]) then
            ST_AircraftInit(MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE])
        end

        -- Did the Wing type change?
        if (currWingType ~= MENU_DATA[MEMU_VAR.WING_TYPE]) then
            if (currAircraftType==AIRCRAFT_TYPE.GLIDER) then
                ST_GliderWingInit(MENU_DATA[MEMU_VAR.WING_TYPE])
            else
                ST_PlaneWingInit(MENU_DATA[MEMU_VAR.WING_TYPE])
            end

            -- DELTA has only RUDER 
            if ((currWingType==WING_TYPE.ELEVON_A or currWingType==WING_TYPE.ELEVON_B) and TAIL_TYPE~=TAIL_TYPE.RUD_1) then
                currTailType = TAIL_TYPE.RUD_1
            end
        end

        --- Did the tail changed?
        if (currTailType ~= MENU_DATA[MEMU_VAR.TAIL_TYPE]) then
            if (currAircraftType==AIRCRAFT_TYPE.GLIDER) then
                ST_GliderTailInit(MENU_DATA[MEMU_VAR.TAIL_TYPE])
            else
                ST_PlaneTailInit(MENU_DATA[MEMU_VAR.TAIL_TYPE])
            end
        end

        ctx.Phase = PHASE.WAIT_CMD
    elseif ctx.Phase == PHASE.EXIT then
       ctx.Phase=PHASE.EXIT_DONE
    end
end

------------------------------------------------------------------------------------------------------------
-- TEXT Management

local List_Text = {}
local Text = {}
local List_Text_Img = {}
local List_Values = {}

-- Get the text for this menus 
local function ST_Get_Text(index)
    local out = Text[index]   -- Find in regular header first
    if out== nil then
        out = List_Text[index]  -- Try list values, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "Unknown_" .. string.format("%X", index)
    end
    return out
end

-- Get the List text for this menus 
local function ST_Get_List_Text(index)
    local out = List_Text[index]   -- Try to find the message in List_Text
    if out == nil then
        out = Text[index]  -- Try list headers, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "UnknownLT_" .. string.format("%X", index)
    end
    return out
end

-- Get the List_Text Images
local function ST_Get_List_Text_Img(index) 
    local out = List_Text_Img[index]
    return out
end

local function ST_Get_List_Values(index)
    local out = List_Values[index]
    return out
end

-- Inital List and Image Text for this menus
local function ST_Init_Text(rxId) 
    dsmLib.Init_Text(rxId)

    -- Channel Names use the Port Text Retrived from OTX/ETX
    for i = 0, 9 do List_Text[i] = MODEL.PORT_TEXT[i]  end

    -- Aircraft Type
    List_Text[50+AIRCRAFT_TYPE.PLANE]  = "Airplane";    --List_Text_Img[50+AIRCRAFT_TYPE.PLANE]   = "at_plane.png|Airplane" 
    List_Text[50+AIRCRAFT_TYPE.GLIDER] = "Glider (Partial work)";      --List_Text_Img[50+AIRCRAFT_TYPE.GLIDER]  = "at_glider.png|Glider" 
    List_Text[50+AIRCRAFT_TYPE.HELI]   = "Helicopter (Not done)";  --List_Text_Img[50+AIRCRAFT_TYPE.HELI]    = "at_heli.png|Helicopter" 
    List_Text[50+AIRCRAFT_TYPE.DRONE]  = "Drone (not done)";       --List_Text_Img[50+AIRCRAFT_TYPE.DRONE]   = "at_drone.png|Drone" 

    -- Wing Types
    List_Text[100+WING_TYPE.AIL_1] = "Single Ail";  List_Text_Img[100+WING_TYPE.AIL_1]  = "wt_1ail.png|Single Aileron" 
    List_Text[100+WING_TYPE.AIL_2] = "Dual Ail";  List_Text_Img[100+WING_TYPE.AIL_2]  = "wt_2ail.png|Dual Aileron" 
    List_Text[100+WING_TYPE.FLAPERON] = "Flaperon";  List_Text_Img[100+WING_TYPE.FLAPERON]  = "wt_flaperon.png|Flaperon" 
    List_Text[100+WING_TYPE.AIL_1_FLP_1] = "Ail + Flap";  List_Text_Img[100+WING_TYPE.AIL_1_FLP_1]  = "wt_1ail_1flp.png|Aileron + Flap" 
    List_Text[100+WING_TYPE.AIL_2_FLP_1] = "Dual Ail + Flap";  List_Text_Img[100+WING_TYPE.AIL_2_FLP_1]  = "wt_2ail_1flp.png|Dual Aileron + Flap" 
    List_Text[100+WING_TYPE.AIL_2_FLP_2] = "Dual Ail + Dual Flap";  List_Text_Img[100+WING_TYPE.AIL_2_FLP_2]  = "wt_2ail_2flp.png|Dual Aileron + Dual Flap" 
    List_Text[100+WING_TYPE.ELEVON_A] = "Delta/Elevon A";  List_Text_Img[100+WING_TYPE.ELEVON_A]  = "wt_elevon.png|Delta/Elevon A" 
    List_Text[100+WING_TYPE.ELEVON_B] = "Delta/Elevon B";  List_Text_Img[100+WING_TYPE.ELEVON_B]  = "wt_elevon.png|Delta/Elevon B" 

    -- Tail Types
    List_Text[200+TAIL_TYPE.RUD_1] = "Rudder Only";  List_Text_Img[200+TAIL_TYPE.RUD_1]  = "tt_1rud.png|Rudder Only" 
    List_Text[200+TAIL_TYPE.RUD_1_ELEV_1] = "Rud + Ele";  List_Text_Img[200+TAIL_TYPE.RUD_1_ELEV_1]  = "tt_1rud_1ele.png|Tail Normal" 
    List_Text[200+TAIL_TYPE.RUD_1_ELEV_2] = "Rud + Dual Ele";  List_Text_Img[200+TAIL_TYPE.RUD_1_ELEV_2]  = "tt_1rud_2ele.png|Rud + Dual Elev" 
    List_Text[200+TAIL_TYPE.RUD_2_ELEV_1] = "Dual Rud + Ele";  List_Text_Img[200+TAIL_TYPE.RUD_2_ELEV_1]  = "tt_2rud_1ele.png|Dual Rud + Elev" 
    List_Text[200+TAIL_TYPE.RUD_2_ELEV_2] = "Dual Rud + Dual Ele";  List_Text_Img[200+TAIL_TYPE.RUD_2_ELEV_2]  = "tt_2rud_2ele.png|Dual Rud + Dual Elev" 
    List_Text[200+TAIL_TYPE.VTAIL_A] = "V-Tail A";  List_Text_Img[200+TAIL_TYPE.VTAIL_A]  = "tt_vtail.png|V-Tail A" 
    List_Text[200+TAIL_TYPE.VTAIL_B] = "V-Tail B";  List_Text_Img[200+TAIL_TYPE.VTAIL_B]  = "tt_vtail.png|V-Tail B" 
    List_Text[200+TAIL_TYPE.TRAILERON_A] = "Traileron A";  List_Text_Img[200+TAIL_TYPE.TRAILERON_A]  = "tt_traileron.png|Traileron A" 
    List_Text[200+TAIL_TYPE.TRAILERON_B] = "Traileron B";  List_Text_Img[200+TAIL_TYPE.TRAILERON_B]  = "tt_traileron.png|Traileron B" 

    -- Servo Reverse
    List_Text[300+CH_MODE_TYPE.NORMAL]  = "Normal "
    List_Text[300+CH_MODE_TYPE.REVERSE] = "Reverse"

end

-- Initial Setup
local function ST_Init()
    -- Initialize dsmLib background
    dsmLib.Init()
    -- Initialize text (use RX_ID 0)
    ST_Init_Text(0)

    -- Setup default Data, and load a file if exist
    ST_Default_Data()
    if (ST_LoadFileData()==0) then -- Did not load a file
        ST_SaveFileData() -- Save Defaults
    end
end


------------------------------------------------------------------------------------------------------------
-- Lib EXPORTS

-- Export Constants
SetupLib.PHASE       = dsmLib.PHASE
SetupLib.LINE_TYPE   = dsmLib.LINE_TYPE
SetupLib.RX          = dsmLib.RX 
SetupLib.DISP_ATTR   = dsmLib.DISP_ATTR

SetupLib.BACK_BUTTON = dsmLib.BACK_BUTTON
SetupLib.NEXT_BUTTON = dsmLib.NEXT_BUTTON
SetupLib.PREV_BUTTON = dsmLib.PREV_BUTTON
SetupLib.MAX_MENU_LINES = dsmLib.MAX_MENU_LINES

-- Export Shared Context Variables
SetupLib.DSM_Context = dsmLib.DSM_Context

-- Export Functions
SetupLib.LOG_write = dsmLib.LOG_write
SetupLib.LOG_close = dsmLib.LOG_close
SetupLib.getElapsedTime = dsmLib.getElapsedTime

SetupLib.Get_Text = ST_Get_Text
SetupLib.Get_List_Text = ST_Get_List_Text
SetupLib.Get_List_Text_Img = ST_Get_List_Text_Img

SetupLib.phase2String = dsmLib.phase2String
SetupLib.menu2String = dsmLib.menu2String
SetupLib.menuLine2String = dsmLib.menuLine2String

SetupLib.isSelectableLine = dsmLib.isSelectableLine
SetupLib.isEditableLine = dsmLib.isEditableLine
SetupLib.isListLine = dsmLib.isListLine
SetupLib.isPercentValueLine = dsmLib.isPercentValueLine
SetupLib.isNumberValueLine = dsmLib.isNumberValueLine
SetupLib.isDisplayAttr = dsmLib.isDisplayAttr
SetupLib.isFlightModeLine = dsmLib.isFlightModeLine
SetupLib.GetFlightModeValue = dsmLib.GetFlightModeValue

SetupLib.StartConnection = ST_StartConnection   -- Override Function 
SetupLib.ReleaseConnection = ST_ReleaseConnection  -- Override Function
SetupLib.ChangePhase = dsmLib.ChangePhase
SetupLib.Value_Add = dsmLib.Value_Add
SetupLib.Value_Default = dsmLib.Value_Default
SetupLib.Value_Write_Validate = dsmLib.Value_Write_Validate
SetupLib.GotoMenu = dsmLib.GotoMenu
SetupLib.MoveSelectionLine = dsmLib.MoveSelectionLine
SetupLib.Send_Receive = ST_SendReceive -- Override Function 
SetupLib.Init = ST_Init
SetupLib.Init_Text = ST_Init_Text

SetupLib.CreateDSMPortChannelInfo = CreateDSMPortChannelInfo

return SetupLib
