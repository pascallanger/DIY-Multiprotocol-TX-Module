local Log, menuLib, modelLib, DEBUG_ON, SIMULATION_ON = ... -- Get DebugON from parameters
local MAIN_MENU_LIB_VERSION = "0.56"
local MODEL     = modelLib.MODEL

local PHASE     = menuLib.PHASE
local LINE_TYPE = menuLib.LINE_TYPE

local lastGoodMenu=0    

-- Creates the menus to Render with the GUI
local function ST_LoadMenu(menuId)
    local ctx = menuLib.DSM_Context
    menuLib.clearMenuLines()

    if (menuId==0x1000) then -- MAIN MENU
        ctx.Menu = { MenuId = 0x1000, Text = "Main Menu ("..MODEL.modelName..")", PrevId = 0, NextId = 0, BackId = 0, TextId=0 }
        ctx.MenuLines[0] = { Type = LINE_TYPE.MENU, Text = "Model Setup", ValId = 0xFFF3,TextId=0 }
       
        if (SIMULATION_ON) then
            ctx.MenuLines[4] = { Type = LINE_TYPE.MENU, Text = "RX Simulator (GUI dev only)", ValId = 0xFFF1, TextId=0 } -- Menu 0xFFF2 to SIMULATOR  
        end
        ctx.MenuLines[6] = { Type = LINE_TYPE.MENU, Text = "Forward Programming RX", ValId = 0xFFF2, TextId=0 }  -- Menu 0xFFF2 to Real RX 
        ctx.SelLine = 6 

        lastGoodMenu = menuId
    else
        --print("NOT IMPLEMENTED")
        ctx.Menu = { MenuId = 0x0002, Text = "NOT IMPLEMENTED", TextId = 0, PrevId = 0, NextId = 0, BackId = lastGoodMenu }
        ctx.SelLine = menuLib.BACK_BUTTON
    end

    menuLib.PostProcessMenu()
end

local function Main_Send_Receive()
    local ctx = menuLib.DSM_Context

    if ctx.Phase == PHASE.RX_VERSION then  -- Just Init RX Version
        ctx.RX.Name = "Main Menu"
        ctx.RX.Version = MAIN_MENU_LIB_VERSION
        ctx.Phase = PHASE.MENU_TITLE
        ctx.Menu.MenuId = 0

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
        --if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end
        --if (DEBUG_ON) then Log.LOG_write("SEND SIM_updateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        ctx.Phase = PHASE.VALUE_CHANGING_WAIT

    elseif ctx.Phase == PHASE.VALUE_CHANGING_WAIT then
        local line = ctx.MenuLines[ctx.SelLine]

    elseif ctx.Phase == PHASE.VALUE_CHANGE_END then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
        --if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end
        --if (DEBUG_ON) then Log.LOG_write("SEND SIM_updateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        --if (DEBUG_ON) then Log.LOG_write("SEND SIM_validateMenuValue(ValueId=0x%X Text=\"%s\" Value=%s)\n", line.ValId, line.Text, menuLib.lineValue2String(line)) end
        ctx.Phase = PHASE.WAIT_CMD
    
    elseif ctx.Phase == PHASE.EXIT then
       ctx.Phase=PHASE.EXIT_DONE
       return 1
    end

    return 0
end

local function Main_Init()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.RX_VERSION
end

local function Main_Done()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.EXIT_DONE
end

return { init=Main_Init, run=Main_Send_Receive, done=Main_Done }