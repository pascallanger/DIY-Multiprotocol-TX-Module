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

local Log, menuLib, modelLib, DEBUG_ON, SIMULATION_ON = ... -- Get DebugON from parameters
local SETUP_LIB_VERSION = "0.56"

local DATA_PATH = modelLib.DATA_PATH

local PHASE = menuLib.PHASE
local LINE_TYPE = menuLib.LINE_TYPE

local MODEL     = modelLib.MODEL

local AIRCRAFT_TYPE = modelLib.AIRCRAFT_TYPE
local WING_TYPE = modelLib.WING_TYPE
local TAIL_TYPE = modelLib.TAIL_TYPE
local CH_MODE_TYPE = modelLib.CH_MODE_TYPE
local PORT = modelLib.PORT
local MEMU_VAR = modelLib.MEMU_VAR
local MENU_DATA = modelLib.MENU_DATA

local SetupLib = {}

local lastGoodMenu=0            

-------------------  Model Setup Helper functions ----------------------
local currAircraftType = -1     -- Current AircraftType selected, and to detect change
local currTailType = -1         -- Current WingType selected, and to detect change
local currWingType = -1         -- Current TailType selected, and to detect change

local menuDataChanged = false   -- Flag to notify if any data has changed


local function tailTypeCompatible(a,b)

    local function normalize(tt)
      if (tt==TAIL_TYPE.TRAILERON_A or tt==TAIL_TYPE.TRAILERON_B) then 
        return TAIL_TYPE.TRAILERON_A
    elseif (tt==TAIL_TYPE.TRAILERON_A_R2 or tt==TAIL_TYPE.TRAILERON_B_R2) then 
        return TAIL_TYPE.TRAILERON_A_R2  
      elseif (tt==TAIL_TYPE.VTAIL_A or tt==TAIL_TYPE.VTAIL_B) then
        return TAIL_TYPE.VTAIL_A
      else
        return tt
      end
    end
  
    return (normalize(a)==normalize(b))
  end


-- Creates the menus to Render with the GUI
local function ST_LoadMenu(menuId)
    local ctx = menuLib.DSM_Context

    local function portUse(p)
        local out = "" 
        if p==MENU_DATA[MEMU_VAR.CH_THR] then out = "Thr"
        elseif p == MENU_DATA[MEMU_VAR.CH_L_AIL] then 
            out=(MENU_DATA[MEMU_VAR.CH_R_AIL] and "Ail_L") or "Ail"
        elseif p == MENU_DATA[MEMU_VAR.CH_R_AIL] then out="Ail_R"
        elseif p == MENU_DATA[MEMU_VAR.CH_L_ELE] then 
            out=(MENU_DATA[MEMU_VAR.CH_R_ELE] and "Ele_L") or "Ele"
        elseif p == MENU_DATA[MEMU_VAR.CH_R_ELE] then out="Ele_R"
        elseif p == MENU_DATA[MEMU_VAR.CH_L_RUD] then 
            out=(MENU_DATA[MEMU_VAR.CH_R_RUD] and "Rud_L") or "Rud"
        elseif p == MENU_DATA[MEMU_VAR.CH_R_RUD] then out="Rud_R"
        elseif p == MENU_DATA[MEMU_VAR.CH_L_FLP] then 
            out=(MENU_DATA[MEMU_VAR.CH_R_FLP] and "Flp_L") or "Flp"
        elseif p == MENU_DATA[MEMU_VAR.CH_R_FLP] then out="Flp_R"
        end
        return out
    end

    local function formatTXRevert(port)
        local out = " " .. modelLib.channelType2String(MODEL.DSM_ChannelInfo[port][0], MODEL.DSM_ChannelInfo[port][1]);
        return out
    end

    local function Header(p)
        return MODEL.PORT_TEXT[p].." "..portUse(p)
    end

    menuLib.clearMenuLines()

    
    if (menuId==0x1000) then -- MAIN MENU
        ctx.Menu = { MenuId = 0x1000, Text = "Save-Exit ("..MODEL.modelName..")", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
       
        if (true) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, Text="Save Changes", TextId = 0, ValId = 0x1005 }
            ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text="Discard Changes", TextId = 0, ValId = 0x1006 }
            ctx.SelLine = 4 
        end
        lastGoodMenu = menuId
    elseif (menuId==0x1001) then -- MODEL SETUP
        local backId = 0xFFF9 -- No changes, just exit
        local title  =  "Model Setup  ("..MODEL.modelName..")"
        if (menuDataChanged) then
            backId = 0x1000  -- Go to Save menu
            title = title.." *"
        end
        ctx.Menu = { MenuId = 0x1001, Text = title, PrevId = 0, NextId = 0, BackId = backId, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, Text = "Aircraft Type Setup", ValId = 0x1010,TextId=0 }
        ctx.MenuLines[1] = { Type = LINE_TYPE.MENU, Text = "Wing & Tail Channels ", ValId = 0x1020, TextId=0 } 
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text = "Gyro Channel Reverse", ValId = 0x1030, TextId=0 } 
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text = "WARNING: Changing of Aircraft or Wing will", ValId = 0x1001, TextId=0 } 
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text = "delete previous Channel/Port assigments.", ValId = 0x1001, TextId=0 } 

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1005) then
        modelLib.printChannelSummary()
        modelLib.ST_SaveFileData()
        menuDataChanged = false


        local msg1 = "Data saved to: " 
        local msg2 = "  "..DATA_PATH.."/"..modelLib.hashName(MODEL.modelName)..".txt" 

        ctx.Menu = { MenuId = 0x1005, Text = "Config Saved", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, Text=msg1, TextId = 0, ValId = 0x1005 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text=msg2, TextId = 0, ValId = 0x1005 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Complete", TextId = 0, ValId = 0xFFF9 }
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1006) then
        modelLib.ST_LoadFileData()
        menuDataChanged = false
        currAircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
        currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
        currTailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

        local msg1 = "Data restored from: " 
        local msg2 = "  "..DATA_PATH.."/"..modelLib.hashName(MODEL.modelName)..".txt" 
        
        ctx.Menu = { MenuId = 0x1006, Text = "Discart Changes", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[2] = { Type = LINE_TYPE.MENU, Text=msg1, TextId = 0, ValId = 0x1006 }
        ctx.MenuLines[3] = { Type = LINE_TYPE.MENU, Text=msg2, TextId = 0, ValId = 0x1006 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Complete", TextId = 0, ValId = 0xFFF9 }
        ctx.SelLine = 6
        lastGoodMenu = menuId
    elseif (menuId==0x1010) then
        modelLib.printChannelSummary()
        ctx.Menu = { MenuId = 0x1010, Text = "Aircraft Type", PrevId = 0, NextId = 0x1011, BackId = 0x1001, TextId=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Aircraft Type", TextId = 0, ValId = MEMU_VAR.AIRCRAFT_TYPE, Min=50, Max=53, Def=50, Val=MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE] }
        ctx.SelLine = 5
        lastGoodMenu = menuId
    elseif (menuId==0x1011) then
        ctx.Menu = { MenuId = 0x1011, Text = "Model Type:"..modelLib.aircraft_type_text[currAircraftType], PrevId = 0, NextId = 0x1020, BackId = 0x1010, TextId=0 }
        ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Wing Type", TextId = 0, ValId = MEMU_VAR.WING_TYPE, Min=100, Max=107, Def=100, Val=MENU_DATA[MEMU_VAR.WING_TYPE] }
        ctx.MenuLines[6] = { Type = LINE_TYPE.LIST_MENU_NC, Text="Tail Type", TextId = 0, ValId = MEMU_VAR.TAIL_TYPE, Min=200, Max=210, Def=200, Val=MENU_DATA[MEMU_VAR.TAIL_TYPE] }
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

        local title = modelLib.aircraft_type_text[currAircraftType].."   Wing:"..modelLib.wing_type_text[currWingType]

        ctx.Menu = { MenuId = 0x1020, Text = title, PrevId = 0, NextId = 0x1021, BackId = 0x1011, TextId=0 }

        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=thrText, TextId = 0, ValId = MEMU_VAR.CH_THR, Min=0, Max=10, Def=0, Val= thr }

        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftAilText, TextId = 0, ValId = MEMU_VAR.CH_L_AIL, Min=0, Max=9, Def=0, Val= leftAil }

        if (rightAil~=nil) then
            ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightAilText, TextId = 0, ValId = MEMU_VAR.CH_R_AIL, Min=0, Max=9, Def=0, Val= rightAil }
        end

        if (leftFlap~=nil) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=leftFlapText, TextId = 0, ValId = MEMU_VAR.CH_L_FLP, Min=0, Max=9, Def=0, Val= leftFlap }
        end
        if (rightFlap~=nil) then
            ctx.MenuLines[5] = { Type = LINE_TYPE.LIST_MENU_NC, Text=rightFlapText, TextId = 0, ValId = MEMU_VAR.CH_R_FLP, Min=0, Max=9, Def=0, Val= rightFlap }
        end

        ctx.SelLine = 0
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

        local title = modelLib.aircraft_type_text[currAircraftType].."  Tail:"..modelLib.tail_type_text[currTailType]

        ctx.Menu = { MenuId = 0x1021, Text = title, PrevId = 0, NextId = 0x1001, BackId = 0x1020, TextId=0 }
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
        modelLib.CreateDSMPortChannelInfo()
        modelLib.printChannelSummary()
        
        ctx.Menu = { MenuId = 0x1030, Text = "Gyro Channel Reverse (Port 1-5)", PrevId = 0, NextId = 0x1031, BackId = 0x1001, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT1), TextId = 0, ValId = MEMU_VAR.PORT1_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT1_MODE], Format = formatTXRevert(PORT.PORT1) }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT2), TextId = 0, ValId = MEMU_VAR.PORT2_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT2_MODE], Format = formatTXRevert(PORT.PORT2) }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT3), TextId = 0, ValId = MEMU_VAR.PORT3_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT3_MODE], Format = formatTXRevert(PORT.PORT3) }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT4), TextId = 0, ValId = MEMU_VAR.PORT4_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT4_MODE], Format = formatTXRevert(PORT.PORT4) }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT5), TextId = 0, ValId = MEMU_VAR.PORT5_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT5_MODE], Format = formatTXRevert(PORT.PORT5) }

        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text="Only Thr/Ail/Rud/Ele. This affects AS3X/SAFE reaction dir./b", TextId = 0, ValId = 0x1030 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Any changes, use RX 'Relearn Servo Settings'/b", TextId = 0, ValId = 0x1030 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    elseif (menuId==0x1031) then
        modelLib.CreateDSMPortChannelInfo()
        modelLib.printChannelSummary()
        ctx.Menu = { MenuId = 0x1031, Text = "Gyro Channel Reverse (Port 6-10)", PrevId = 0x1030, NextId = 0, BackId = 0x1001, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT6), TextId = 0, ValId = MEMU_VAR.PORT6_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT6_MODE], Format = formatTXRevert(PORT.PORT6) }
        ctx.MenuLines[1] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT7), TextId = 0, ValId = MEMU_VAR.PORT7_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT7_MODE], Format = formatTXRevert(PORT.PORT7) }
        ctx.MenuLines[2] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT8), TextId = 0, ValId = MEMU_VAR.PORT8_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT8_MODE], Format = formatTXRevert(PORT.PORT8) }
        ctx.MenuLines[3] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT9), TextId = 0, ValId = MEMU_VAR.PORT9_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT9_MODE], Format = formatTXRevert(PORT.PORT9) }
        ctx.MenuLines[4] = { Type = LINE_TYPE.LIST_MENU_NC, Text=Header(PORT.PORT10), TextId = 0, ValId = MEMU_VAR.PORT10_MODE, Min=300, Max=301, Def=300, Val=MENU_DATA[MEMU_VAR.PORT10_MODE], Format = formatTXRevert(PORT.PORT10) }
     
        ctx.MenuLines[5] = { Type = LINE_TYPE.MENU, Text="Only Thr/Ail/Rud/Ele. This affects AS3X/SAFE reaction dir./b", TextId = 0, ValId = 0x1031 }
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text="Any changes, use RX 'Relearn Servo Settings'/b", TextId = 0, ValId = 0x1031 }

        ctx.SelLine = 0
        lastGoodMenu = menuId
    else
        print("NOT IMPLEMENTED")
        ctx.Menu = { MenuId = 0x0002, Text = "NOT IMPLEMENTED", TextId = 0, PrevId = 0, NextId = 0, BackId = lastGoodMenu }
        ctx.SelLine = menuLib.BACK_BUTTON
    end

    menuLib.PostProcessMenu()
end

-- ST_SendReceive
-- Main state machine for the Setup menu

local function ST_SendReceive()
    local ctx = menuLib.DSM_Context
    --if (DEBUG_ON>1) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end

    if ctx.Phase == PHASE.RX_VERSION then -- request RX version
        ctx.RX.Name = "MODEL SETUP"
        ctx.RX.Version = SETUP_LIB_VERSION
        ctx.Phase = PHASE.MENU_TITLE
        ctx.Menu.MenuId = 0x01001

        ctx.Refresh_Display = true

        
    elseif ctx.Phase == PHASE.WAIT_CMD then 
        
    elseif ctx.Phase == PHASE.MENU_TITLE then -- request menu title
        ST_LoadMenu(ctx.Menu.MenuId)
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
            currAircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
            modelLib.ST_AircraftInit(currAircraftType)
            currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
            currTailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]
        end

        -- Did the Wing type change?
        if (currWingType ~= MENU_DATA[MEMU_VAR.WING_TYPE]) then
            if (currAircraftType==AIRCRAFT_TYPE.GLIDER) then
                currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
                modelLib.ST_GliderWingInit(currWingType)
            else
                currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
                modelLib.ST_PlaneWingInit(currWingType)
                
            end

            -- DELTA has only RUDER 
            if ((currWingType==WING_TYPE.ELEVON_A or currWingType==WING_TYPE.ELEVON_B) and TAIL_TYPE~=TAIL_TYPE.RUD_1) then
                MENU_DATA[MEMU_VAR.TAIL_TYPE] = TAIL_TYPE.RUD_1
            end
        end

        --- Did the tail changed?
        local ntt = MENU_DATA[MEMU_VAR.TAIL_TYPE]
        if (currTailType ~= ntt) then
            if (currAircraftType==AIRCRAFT_TYPE.GLIDER) then
                currTailType = ntt
                modelLib.ST_GliderTailInit(currTailType)
            else
                if (not tailTypeCompatible(currTailType,ntt)) then
                    modelLib.ST_PlaneTailInit(ntt)
                end
                currTailType = ntt
            end
        end

        ctx.Phase = PHASE.WAIT_CMD
    elseif ctx.Phase == PHASE.EXIT then
       ctx.Phase=PHASE.EXIT_DONE
    end
end

------------------------------------------------------------------------------------------------------------

-- Inital List and Image Text for this menus
local function ST_Init_Text(rxId) 
    menuLib.clearAllText()

    local List_Values = menuLib.List_Values
    local List_Text = menuLib.List_Text
    local Text = menuLib.Text
    local List_Text_Img = menuLib.List_Text_Img

    -- Channel Names use the Port Text Retrived from OTX/ETX
    for i = 0, 9 do List_Text[i] = MODEL.PORT_TEXT[i]  end
    List_Text[10]="--"

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
    List_Text[200+TAIL_TYPE.TRAILERON_A] = "Taileron A";  List_Text_Img[200+TAIL_TYPE.TRAILERON_A]  = "tt_taileron.png|Taileron A" 
    List_Text[200+TAIL_TYPE.TRAILERON_B] = "Taileron B";  List_Text_Img[200+TAIL_TYPE.TRAILERON_B]  = "tt_taileron.png|Taileron B" 
    List_Text[200+TAIL_TYPE.TRAILERON_A_R2] = "Taileron A + 2x Rud";  List_Text_Img[200+TAIL_TYPE.TRAILERON_A_R2]  = "tt_taileron2.png|Taileron A + Dual Rud" 
    List_Text[200+TAIL_TYPE.TRAILERON_B_R2] = "Taileron B + 2x Rud";  List_Text_Img[200+TAIL_TYPE.TRAILERON_B_R2]  = "tt_taileron2.png|Taileron B + Dual Rud" 


    -- Servo Reverse
    if (LCD_W > 128) then
        List_Text[300+CH_MODE_TYPE.NORMAL]  = "Normal "
        List_Text[300+CH_MODE_TYPE.REVERSE] = "Reverse"
        else
        List_Text[300+CH_MODE_TYPE.NORMAL]  = "Nor"
        List_Text[300+CH_MODE_TYPE.REVERSE] = "Rev"
    end
end

-- Initial Setup
local function ST_Init()
    -- Initialize text (use RX_ID 0)
    ST_Init_Text(0)

    -- Setup default Data, and load a file if exist
    --modelLib.ST_Default_Data()
    if (modelLib.ST_LoadFileData()==0) then -- Did not load a file
        modelLib.ST_Default_Data()
        modelLib.ST_SaveFileData() -- Save Defaults
    end
    menuDataChanged = false
    currAircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
    currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
    currTailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.RX_VERSION
end

local function ST_Done()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.EXIT_DONE
end


return { init=ST_Init, run=ST_SendReceive, done=ST_Done }