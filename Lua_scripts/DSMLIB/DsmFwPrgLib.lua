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
-- This script library is a rewrite of the original DSM forward programming Lua 
-- Script.  The goal is to make it easier to understand, mantain, and to  
-- separate the GUI from the DSM Forward  programming engine/logic
-- in this way, GUIs can evolve independent. OpenTX Gui, EdgeTx GUI, Small Radios, etc.

-- Code is based on the code/work by: Pascal Langer (Author of the Multi-Module)  
-- Rewrite/Enhancements By: Francisco Arzu 
-- 
------------------------------------------------------------------------------
    --###############################################################################
    -- Multi buffer for DSM description
    -- Multi_Buffer[0..2]=="DSM" -> Lua script is running
    -- Multi_Buffer[3]==0x70+len -> TX to RX data ready to be sent
    -- Multi_Buffer[4..9]=6 bytes of TX to RX data
    -- Multi_Buffer[10..25]=16 bytes of RX to TX data
    --
    -- To start operation:
    --   Write 0x00 at address 3
    --   Write 0x00 at address 10
    --   Write "DSM" at address 0..2
    --###############################################################################


local DEBUG_ON = ... -- Get Debug_ON from parameters.  -- 0=NO DEBUG, 1=HIGH LEVEL 2=MORE DETAILS 
local LIB_VERSION = "0.5"
local FORCED_HACK = false


local Lib = { Init_Text = function (rxId) end }

--RX IDs--
local RX = {
    AR636B = 0x0001,
    SPM4651T = 0x0014,
    AR637T = 0x0015,
    AR637TA = 0x0016,
    FC6250HX = 0x0018,
    AR630   = 0x0019,
    AR8360T = 0x001A,
    AR631 = 0x001E
    -- AR10360T
}

local PHASE = {
    RX_VERSION = 0,
    WAIT_CMD = 1,
    MENU_TITLE = 2,
    MENU_UNKNOWN_LINES = 3,
    MENU_LINES = 4, MENU_VALUES = 5,
    VALUE_CHANGING = 6, VALUE_CHANGING_WAIT = 7, VALUE_CHANGE_END = 8,
    EXIT = 9, EXIT_DONE = 10
}

local LINE_TYPE = {
    MENU = 0x1C,
    LIST_MENU = 0x0C,  -- List:  TODO: Investigate why the Min/Max on some lines comes with a wide range (0..244) when non-contiguos values. example Valid (3,176,177)
    LIST_MENU_NC = 0x6C,  -- List:  No Incremental Change     
    LIST_MENU_TOG = 0x4C,  -- List:   Seems like a bolean/Toggle menu, just 2 values 0->1  (off/on, ihn/Act)

    VALUE_NUM_I8_NC = 0x60,  --  8 bit number, no incremental change
    VALUE_PERCENT = 0xC0, -- 8 bit number, Signed, percent
    VALUE_DEGREES  = 0xE0, -- 8 bit number, Signed, Degress
    VALUE_NUM_I8 = 0x40, -- 8 bit number
    VALUE_NUM_I16 = 0x41, -- 16 Bit number
    VALUE_NUM_SI16 = 0xC1, -- 16 bit number, Signed  
    LT_EMPTY = 0x00
}

local DISP_ATTR = {
    BOLD = 0x01,  RIGHT=0x02, CENTER=0x04, PERCENT = 0x10, DEGREES=0x20, FORCED_MENU = 0x40 
}

local DSM_Context = {
    Phase = PHASE.RX_VERSION,
    Menu = { MenuId = 0, Text = "", TextId = 0, PrevId = 0, NextId = 0, BackId = 0 },
    MenuLines = {},
    RX = { Id=0, Name = "", Version = "" },
    Refresh_Display = true,

    SelLine = 0,        -- Current Selected Line
    EditLine = nil,     -- Current Editing Line 
    CurLine = -1,       -- Current Line Requested/Parsed via h message protocol 
    isReset = false     -- false when starting from scracts, true when starting from Reset
}

local MAX_MENU_LINES = 6
local BACK_BUTTON    = -1                   -- Tread it as a display line #-1
local NEXT_BUTTON    = MAX_MENU_LINES + 1   -- Tread it as a display line #7
local PREV_BUTTON    = MAX_MENU_LINES + 2   -- Tread it as a display line #7

local SEND_TIMEOUT = 2000 / 10 --  Home many 10ms intervals to wait on sending data to tx to keep connection open   (2s)
local InactivityTime = 0      -- Next time to do heartbeat after inactivity 
local StartTime = 0           -- Start time since the start of the script

local Waiting_RX = 0 -- 1 if Waiting for an RX response, 0 if transmiting
local Value_Change_Step = 0  -- 2 Steps to update. 0=Send update value, 1=Send Verificatin request

-- Text Arrays for Display Text and Debuging 
local PhaseText = {}
local LineTypeText = {}
local RxName = {}

local Text = {}             -- Text for Menu and Menu Lines   (Headers only)
local List_Text = {}        -- Messages for List Options (values only)
local List_Text_Img = {}    -- If the Text has Attached Images
local List_Values = {}      -- Additiona restrictions on List Values when non contiguos  (L_M1 lines has this problem) 

local LOG_FILE = "/LOGS/dsm_log.txt"
local logFile  = nil

function DSM_Context.isEditing() return DSM_Context.EditLine~=nil end

------------------------------------------------------------------------------------------------------------
local logCount=0
local function LOG_open()  
    logFile = io.open(LOG_FILE, "w")  -- Truncate Log File 
end

local function LOG_write(...)
    if (logFile==nil) then LOG_open() end
    local str = string.format(...)
    io.write(logFile, str)

    print(str)

    if (logCount > 10) then  -- Close an re-open the file
        io.close(logFile)
        logFile = io.open(LOG_FILE, "a")
        logCount =0
    end
end

local function LOG_close()
    if (logFile~=nil) then io.close(logFile) end
end

------------------------------------------------------------------------------------------------------------
-- Get Elapsed Time since we started running the Script. Return a float in format: Seconds.Milliseconds
local function getElapsedTime()
    local t = getTime()
    if (StartTime == 0) then StartTime = t end

    return ((t - StartTime) * 10) / 1000
end

-------------  Line Type helper functions ------------------------------------------------------------------

-- Check if the text are Flight modes, who will be treated different for Display
local function isFlightModeLine(line)
    return (line.TextId >= 0x8000 and line.TextId <= 0x8003)
end

local function isSelectableLine(line)   -- is the display line Selectable??
    -- values who are not selectable
    if (line.Type == 0) then return false end -- Empty Line
    if (line.Type == LINE_TYPE.MENU and line.ValId == line.MenuId and bit32.band(line.TextAttr, DISP_ATTR.FORCED_MENU)==0) then return false end -- Menu that navigates to Itself?
    if (line.Min==0 and line.Max==0 and line.Def==0) then return false end -- Values with no Range are only for display 
    if (line.Type == LINE_TYPE.VALUE_NUM_I8_NC and isFlightModeLine(line)) then return false end -- Flight mode is not Selectable
    return true
end

local function isEditableLine(line) -- is the display line editable??
    -- values who are not editable
    if (line.Type == 0 or line.Type == LINE_TYPE.MENU) then return false end -- Menus are not editable
    if (line.Min==0 and line.Max==0 and line.Def==0) then return false end -- Values with no Range are only for display 
    if (line.Type == LINE_TYPE.VALUE_NUM_I8_NC and isFlightModeLine(line)) then return false end -- Flight mode is not Editable 
    -- any other is Editable
    return true
end

local function isListLine(line)   -- is it a List of options??
    if (line.Type == LINE_TYPE.LIST_MENU_NC or line.Type == LINE_TYPE.LIST_MENU or line.Type == LINE_TYPE.LIST_MENU_TOG) then return true end
    return false
end

local function isPercentValueLineByMinMax(line)
    return
    (line.Min == 0 and  line.Max == 100) or ( line.Min == -100 and  line.Max == 100) or 
    ( line.Min == 0 and  line.Max == 150) or ( line.Min == -150 and  line.Max == 150)
end

local function isPercentValueLine(line)   -- is it a Percent value??
    if (line.Type == LINE_TYPE.VALUE_PERCENT)  then return true end
    return false
end

local function isNumberValueLine(line)     -- is it a number ??
    if (isListLine(line) or line.Type == LINE_TYPE.MENU or line.Type == 0) then return false
    else return true end
end

local function isIncrementalValueUpdate(line)
    if (line.Type == LINE_TYPE.LIST_MENU_NC or line.Type == LINE_TYPE.VALUE_NUM_I8_NC or line.Type == LINE_TYPE.VALUE_DEGREES) then return false end
    return true
end

------------------------------------------------------------------------------------------------------------
local function Get_Text(index)
    local out = Text[index]   -- Find in regular header first
    if out== nil then
        out = List_Text[index]  -- Try list values, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "Unknown_" .. string.format("%X", index)
    end
    return out
end

local function Get_List_Text(index)
    local out = List_Text[index]   -- Try to find the message in List_Text
    if out == nil then
        out = Text[index]  -- Try list headers, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "UnknownLT_" .. string.format("%X", index)
    end
    return out
end

local function Get_List_Text_Img(index) 
    local out = List_Text_Img[index]
    return out
end

local function Get_List_Values(index)
    local out = List_Values[index]
    return out
end

------------------------------------------------------------------------------------------------------------
local function Get_RxName(index)
    local out = RxName[index]
    return out or ("Unknown_" .. string.format("%X", index))
end

----------- Debugging 2-String functions -------------------------------------------------------------------

local function phase2String(index)
    local out = PhaseText[index]
    return out or ("Phase_" .. string.format("%X", index))
end

local function lineType2String(index)
    local out = LineTypeText[index]
    return out or ("LT_" .. string.format("%X", index or 0xFF))
end

local function lineValue2String(l)
    if (DEBUG_ON == 0) then
        return ""
    end
    if (l ~= nil and l.Val ~= nil) then
        local value = l.Val
        if isListLine(l) then
            value = value .. "|\"" .. Get_List_Text(l.Val + l.TextStart) .. "\""
        else
            value = value..(l.Format or "")
        end
        return value
    end
    return "nil"
end

local function menu2String(m)
    local txt = "Menu[]"
    if (m ~= nil) then
        txt = string.format("M[Id=0x%X P=0x%X N=0x%X B=0x%X Text=\"%s\"[0x%X]]",
            m.MenuId, m.PrevId, m.NextId, m.BackId, m.Text, m.TextId)
    end
    return txt
end

local function menuLine2String(l)
    local txt = "Line[]"
    if (l ~= nil) then
        local value = ""
        local range = ""
        if l.Type~=LINE_TYPE.MENU then
            value = "Val="..lineValue2String(l)
            if isListLine(l) then
               range = string.format("NL=(%s->%s,%s,S=%s) ",l.Min, l.Max, l.Def, l.TextStart ) 
               range = range .. (l.MinMaxOrig or "")
            else
                range = string.format("[%s->%s,%s]",l.Min, l.Max, l.Def) 
            end    
        end

        txt = string.format("L[#%s T=%s VId=0x%X Text=\"%s\"[0x%X] %s %s MId=0x%X ]",
            l.lineNum, lineType2String(l.Type), l.ValId,
            l.Text, l.TextId,
            value,
            range,
            l.MenuId
        )
    end
    return txt
end

------------------------------------------------------------------------------------------------------------

local function multiBuffer2String() -- used for debug
    local i
    local rxAnswer = "RX:"
    for i = 10, 25 do
        rxAnswer = rxAnswer .. string.format(" %02X", multiBuffer(i))
    end
    return rxAnswer
end

---------------- DSM Values <-> Int16 Manipulation --------------------------------------------------------

local function int16_LSB(number)  -- Less Significat byte
    local r,x = bit32.band(number, 0xFF)
    return r
end

local function int16_MSB(number) -- Most signifcant byte
    return bit32.rshift(number, 8)
end

local function Dsm_to_Int16(lsb, msb)  -- Componse an Int16 value
    return bit32.lshift(msb, 8) + lsb
end

local function Dsm_to_SInt16(lsb,msb) -- Componse a SIGNED Int16 value
    local value = bit32.lshift(msb, 8) + lsb
    if value >= 0x8000 then  -- Negative value??
        return value - 0x10000
    end
    return value
end

local function sInt16ToDsm(value) -- Convent to SIGNED DSM Value
    if value < 0 then
        value = 0x10000 + value
    end
    return value
end


-----------------------------------------------------------------------------------------------------------
-- Post Procssing Line from Raw values receive by RX or Simulation

local function isDisplayAttr(attr, bit)
    return (bit32.band(attr,bit)>0)
end

local function ExtractDisplayAttr(text1, attr)
    local text, pos = string.gsub(text1, "/c", "")
    if (pos>0) then -- CENTER
        attr = bit32.bor(attr, DISP_ATTR.CENTER)
    end

    text, pos = string.gsub(text, "/r", "")
    if (pos>0) then -- RIGHT
        attr = bit32.bor(attr, DISP_ATTR.RIGHT)
    end

    text, pos = string.gsub(text, "/p", "")
    if (pos>0) then -- Percent TEXT
        attr = bit32.bor(attr, DISP_ATTR.PERCENT)
    end

    text, pos = string.gsub(text, "/b", "")
    if (pos>0) then -- BOLD TEXT
        attr = bit32.bor(attr, DISP_ATTR.BOLD)
    end

    text, pos = string.gsub(text, "/M", "")
    if (pos>0) then -- FORCED MENU Button 
        attr = bit32.bor(attr, DISP_ATTR.FORCED_MENU)
    end

    return text, attr 
end

local function DSM_MenuPostProcessing(menu)
    menu.Text, menu.TextAttr =  ExtractDisplayAttr(menu.Text,menu.TextAttr or 0)
end

local function DSM_MenuLinePostProcessing(line)
    if (line.Text==nil) then
        line.Text   = Get_Text(line.TextId) -- Get Textual Line headeing text 
    end

    -- Text formatting options
    line.Text, line.TextAttr = ExtractDisplayAttr(line.Text,line.TextAttr or 0)

    if line.Type == LINE_TYPE.MENU then
        -- nothing to do on menu entries
        line.Val=nil
    elseif isListLine(line) then
        -- Original Range  for Debugging
        line.MinMaxOrig = "[" .. line.Min .. "->" .. line.Max .. "," .. line.Def .. "]"

        -- Normalize Min/Max to be relative to Zero
        line.TextStart = line.Min
        line.Def = line.Def - line.Min -- normalize default value 
        line.Max = line.Max - line.Min -- normalize max index
        line.Min = 0 -- min index
    else -- default to numerical value
        if isPercentValueLine(line) or isPercentValueLineByMinMax(line) then
            -- either explicit Percent or NO-Change value, but range is %Percent
            line.Format ="%"
            line.TextAttr = bit32.bor(line.TextAttr,DISP_ATTR.PERCENT)
        elseif (line.Type == LINE_TYPE.VALUE_DEGREES) then
            line.Format ="o"
            line.TextAttr = bit32.bor(line.TextAttr,DISP_ATTR.DEGREES)
        end
    end

    line.MinMaxDebug =  lineType2String(line.Type).."  "..(line.MinMaxOrig or "")
end

------------------------------------------------------------------------------------------------------------
local function DSM_send(...)
    local arg = { ... }
   
    for i = 1, #arg do
        multiBuffer(3 + i, arg[i])
    end
    multiBuffer(3, 0x70 + #arg)


    if (DEBUG_ON > 1) then 
        local str = ""
        for i = 1, #arg do
            str = str .. string.format("%02X ", arg[i]) 
        end
        LOG_write("DSM_SEND: [%s]\n", str) 
    end
end

------------------------------------------------------------------------------------------------------------

local function DSM_StartConnection()
    if (DEBUG_ON) then LOG_write("DSM_StartConnection()\n") end
    
    --Set protocol to talk to
    multiBuffer( 0, string.byte('D') )
    --test if value has been written
    if multiBuffer( 0 ) ~=  string.byte('D') then
    if (DEBUG_ON) then LOG_write("Not Enouth memory\n") end
        error("Not enough memory!")
        return 2
    end
    --Init TX buffer
    multiBuffer( 3, 0x00 )
    --Init RX buffer
    multiBuffer( 10, 0x00 )
    --Init telemetry
    multiBuffer( 0, string.byte('D') )
    multiBuffer( 1, string.byte('S') )
    multiBuffer( 2, string.byte('M') ) 

    return 0
end

local function DSM_ReleaseConnection()
    if (DEBUG_ON) then LOG_write("DSM_ReleaseConnection()\n") end
    multiBuffer(0, 0)
    DSM_Context.Phase = PHASE.EXIT_DONE
end

local function DSM_ChangePhase(newPhase)
    DSM_Context.Phase = newPhase
    Waiting_RX = 0
end

local function DSM_Value_Add(line, inc)
    if (DEBUG_ON) then LOG_write("%3.3f %s: DSM_Value_Add(%s,%s)\n", getElapsedTime(), phase2String(DSM_Context.Phase), inc, menuLine2String(line)) end
    local skipIncrement = false
    local values  = nil
    local origVal = line.Val

    -- Use local validation for LIST_MENU1 when the range is wide open 
    -- Also use if for some LIST_MENU0 that the Range seems incorrect
    if (isListLine(line)) then -- and line.Type==LINE_TYPE.LIST_MENU1 and line.Min==0 and line.Max==244) then
        values = Get_List_Values(line.TextId)
    end

    
    if (values~=nil) then  -- Inc/Dec based on a list of predefined Values Local to Script (values not contiguous), 
        -- locate current value in values array 
        -- Values are Zero normalized to the Start of the List (line.TextStart)
        for i = 1, #values do
            if ((values[i]-line.TextStart)==origVal) then
                skipIncrement = true
                if (inc==-1 and i > 1) then -- PREV
                    line.Val = values[i-1]-line.TextStart
                elseif (inc==1 and i < #values) then -- NEXT
                    line.Val = values[i+1]-line.TextStart
                end
                break
            end
        end
    end

    if not skipIncrement then
        -- Do it Sequentially
        line.Val = line.Val + inc

        if line.Val > line.Max then
            line.Val = line.Max
        elseif line.Val < line.Min then
            line.Val = line.Min
        end
    end

    if (origVal~=line.Val and isIncrementalValueUpdate(line)) then 
        -- Update RX value on every change 
        DSM_ChangePhase(PHASE.VALUE_CHANGING)
    end
end

local function DSM_Value_Default(line)
    local origVal = line.Val
    if (DEBUG_ON) then LOG_write("%3.3f %s: DSM_Value_Default(%s)\n", getElapsedTime(), phase2String(DSM_Context.Phase), menuLine2String(line)) end

    line.Val = line.Def
    if (origVal~=line.Val and isIncrementalValueUpdate(line)) then 
        -- Update RX value on every change 
        DSM_ChangePhase(PHASE.VALUE_CHANGING)
    end
end

local function DSM_Value_Write_Validate(line)
    if (DEBUG_ON) then LOG_write("%3.3f %s: DSM_Value_Write_Validate(%s)\n", getElapsedTime(), phase2String(DSM_Context.Phase), menuLine2String(line)) end
    DSM_ChangePhase(PHASE.VALUE_CHANGE_END) -- Update + Validate value in RX 
    DSM_Context.EditLine = nil   -- Exit Edit Mode (By clearing the line editing)
end

local function DSM_GotoMenu(menuId, lastSelectedLine)
    if (DEBUG_ON) then LOG_write("%3.3f %s: DSM_GotoMenu(0x%X,LastSelectedLine=%d)\n", getElapsedTime(), phase2String(DSM_Context.Phase), menuId, lastSelectedLine) end
    DSM_Context.Menu.MenuId = menuId
    DSM_Context.SelLine = lastSelectedLine
    -- Request to load the menu Again
    DSM_ChangePhase(PHASE.MENU_TITLE)
end

local function DSM_MoveSelectionLine(dir)
    local ctx = DSM_Context
    local menu = ctx.Menu
    local menuLines = ctx.MenuLines

    if (dir == 1) then -- NEXT
        if ctx.SelLine <= MAX_MENU_LINES then
            local num = ctx.SelLine
            for i = ctx.SelLine + 1, MAX_MENU_LINES, 1 do
                if isSelectableLine(menuLines[i]) then
                    ctx.SelLine = i
                    break
                end
            end

            if num == ctx.SelLine then
                if menu.NextId ~= 0 then -- Next
                    ctx.SelLine = NEXT_BUTTON
                elseif menu.PrevId ~= 0 then -- Prev
                    ctx.SelLine = PREV_BUTTON
                end
            end
        elseif menu.PrevId ~= 0 then -- Prev
            ctx.SelLine = PREV_BUTTON
        end
        return
    end

    if (dir == -1) then -- PREV 
        if ctx.SelLine == PREV_BUTTON and menu.NextId ~= 0 then
            ctx.SelLine = NEXT_BUTTON
        elseif ctx.SelLine > 0 then
            if ctx.SelLine > MAX_MENU_LINES then
                ctx.SelLine = NEXT_BUTTON
            end
            local num = ctx.SelLine
            for i = ctx.SelLine - 1, 0, -1 do
                if isSelectableLine(menuLines[i]) then
                    ctx.SelLine = i
                    break
                end
            end
            if num == ctx.SelLine then -- can't find previous selectable line, then SELECT  Back
                if (menu.BackId ~= 0) then ctx.SelLine = BACK_BUTTON end
            end
        else
            if (menu.BackId ~= 0) then ctx.SelLine = BACK_BUTTON end -- Back 
        end
    end
end
--------------------------------------------------------------------------------------------------------
-- REEQUEST Messages to RX

local function DSM_sendHeartbeat()
    -- keep connection open
    if (DEBUG_ON) then LOG_write("SEND DSM_sendHeartbeat()\n") end
    DSM_send(0x00, 0x04, 0x00, 0x00)
end

local function DSM_getRxVerson()
    if (DEBUG_ON) then LOG_write("SEND DSM_getRxVersion()\n") end
    DSM_send(0x11, 0x06, 0x00, 0x14, 0x00, 0x00)
end

local function DSM_getMainMenu()
    if (DEBUG_ON) then LOG_write("SEND DSM_getMainMenu()\n") end
    DSM_send(0x12, 0x06, 0x00, 0x14, 0x00, 0x00) -- first menu only
end

local function DSM_getMenu(menuId, latSelLine)
    if (DEBUG_ON) then LOG_write("SEND DSM_getMenu(MenuId=0x%X LastSelectedLine=%s)\n", menuId, latSelLine) end
    DSM_send(0x16, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, latSelLine)
end

local function DSM_getFirstMenuLine(menuId)
    if (DEBUG_ON) then LOG_write("SEND DSM_getFirstMenuLine(MenuId=0x%X)\n", menuId) end
    DSM_send(0x13, 0x04, int16_MSB(menuId), int16_LSB(menuId)) -- line 0
end

local function DSM_getNextMenuLine(menuId, curLine)
    if (DEBUG_ON) then LOG_write("SEND DSM_getNextLine(MenuId=0x%X,LastLine=%s)\n", menuId, curLine) end
    DSM_send(0x14, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, curLine) -- line X
end

local function DSM_getNextMenuValue(menuId, valId, text)
    if (DEBUG_ON) then LOG_write("SEND DSM_getNextMenuValue(MenuId=0x%X, LastValueId=0x%X) Extra: Text=\"%s\"\n", menuId, valId,
            text)
    end
    DSM_send(0x15, 0x06, int16_MSB(menuId), int16_LSB(menuId), int16_MSB(valId), int16_LSB(valId)) -- line X
end

local function DSM_updateMenuValue(valId, val, text, line)
    local value = sInt16ToDsm(val)
    if (DEBUG_ON) then LOG_write("SEND DSM_updateMenuValue(ValueId=0x%X,val=%d) Extra: Text=\"%s\" Value=%s\n", valId, val, text, lineValue2String(line)) end
    DSM_send(0x18, 0x06, int16_MSB(valId), int16_LSB(valId), int16_MSB(value), int16_LSB(value)) -- send current value
end

local function DSM_validateMenuValue(valId, text, line)
    if (DEBUG_ON) then LOG_write("SEND DSM_validateMenuValue(ValueId=0x%X) Extra: Text=\"%s\" Value=%s\n", valId, text, lineValue2String(line)) end
    DSM_send(0x19, 0x06, int16_MSB(valId), int16_LSB(valId)) -- validate
end

local function DSM_menuValueChangingWait(valId, text, line)
    if (DEBUG_ON) then LOG_write("SEND DSM_menuValueChangingWait(ValueId=0x%X) Extra: Text=\"%s\"  Value=%s\n", valId, text, lineValue2String(line)) end
    DSM_send(0x1A, 0x06, int16_MSB(valId), int16_LSB(valId))
end

-----------------------------------------------------------------------------------------------------------

local function DSM_SelLine_HACK()
    -- This hack was to be able to access some menus, that with using the default ctx.SelLine as it was,
    -- the menu start returning weird 0x05 Unknow lines, by overriding the ctx.SelLine to Zero or other value
    -- they started to work.
    -- Tested to work on the RX: AR631, AR637T, AR637TA

    local ctx = DSM_Context
    if (FORCED_HACK or ctx.RX.Id == RX.AR637T or ctx.RX.Id == RX.AR637TA or ctx.RX.Id == RX.AR631 or ctx.RX.Id == RX.AR630) then
        -- AR631/AR637 Hack for "First time Setup" or "First Time AS3X Setup", use 0 instead of the ctx.SelLine
        if (ctx.Menu.MenuId == 0x104F  or ctx.Menu.MenuId==0x1055) then
            if (DEBUG_ON) then LOG_write("First time Setup Menu HACK: Overrideing LastSelectedLine to ZERO\n") end
            if (DEBUG_ON) then LOG_write("%3.3f %s: ", getElapsedTime(), phase2String(ctx.Phase)) end
            ctx.SelLine = 0
        end
        -- AR631/AR637 Hack for "Relearn Servo Settings", use 1 instead of the ctx.SelLine=0
        -- REMOVE THE FIX FOR NOW.. Locks the Servos
        --if (false and ctx.Menu.MenuId == 0x1023) then  
        --    if (DEBUG_ON) then LOG_write("Relearn Servo Settings HACK: Overrideing LastSelectedLine to 1\n") end
        --    if (DEBUG_ON) then LOG_write("%3.3f %s: ", getElapsedTime(), phase2String(ctx.Phase)) end
        --    ctx.SelLine = 1
        --end
    end
end

local function DSM_sendRequest()  
    -- Send the proper Request message depending on the Phase 
    
    local ctx = DSM_Context
    if (DEBUG_ON) then LOG_write("%3.3f %s: ", getElapsedTime(), phase2String(ctx.Phase)) end

    if ctx.Phase == PHASE.RX_VERSION then -- request RX version
        DSM_getRxVerson()

    elseif ctx.Phase == PHASE.WAIT_CMD then -- keep connection open
        DSM_sendHeartbeat()

    elseif ctx.Phase == PHASE.MENU_TITLE then -- request menu title
        if ctx.Menu.MenuId == 0 then  -- First time loading a menu ?
            DSM_getMainMenu()
        else
            DSM_SelLine_HACK()
            DSM_getMenu(ctx.Menu.MenuId, ctx.SelLine) 

            if (ctx.Menu.MenuId == 0x0001) then  -- Executed the Reset Menu??
                if (DEBUG_ON) then LOG_write("RX Reset!!!\n") end
                -- Start again retriving RX info 
                ctx.Menu.MenuId = 0
                ctx.isReset = true                
                ctx.Phase = PHASE.RX_VERSION
            end
        end

    elseif ctx.Phase == PHASE.MENU_UNKNOWN_LINES then -- Still trying to figure out what are this menu lines are for 
        local curLine = ctx.CurLine
        if (DEBUG_ON) then LOG_write("CALL DSM_getNextUknownLine_0x05(LastLine=%s)\n", curLine) end
        local last_byte = { 0x40, 0x01, 0x02, 0x04, 0x00, 0x00 } -- unknown...
        DSM_send(0x20, 0x06, curLine, curLine, 0x00, last_byte[curLine + 1]) -- line X

    elseif ctx.Phase == PHASE.MENU_LINES then -- request next menu lines
        if ctx.CurLine == -1 then -- No previous menu line loaded ?
            DSM_getFirstMenuLine(ctx.Menu.MenuId)
        else
            DSM_getNextMenuLine(ctx.Menu.MenuId, ctx.CurLine)
        end

    elseif ctx.Phase == PHASE.MENU_VALUES then -- request menu values
        local line = ctx.MenuLines[ctx.CurLine]
        DSM_getNextMenuValue(ctx.Menu.MenuId, line.ValId, line.Text)

    elseif ctx.Phase == PHASE.VALUE_CHANGING then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line
        DSM_updateMenuValue(line.ValId, line.Val, line.Text, line)
        ctx.Phase = PHASE.VALUE_CHANGING_WAIT

    elseif ctx.Phase == PHASE.VALUE_CHANGING_WAIT then
        local line = ctx.MenuLines[ctx.SelLine]
        DSM_menuValueChangingWait(line.ValId, line.Text, line)

    elseif ctx.Phase == PHASE.VALUE_CHANGE_END then -- send value
        -- This is a 2 step operation.. Send the value first, then send the Verification.. Value_Changed_Step used for that
        -- on the validation, the RX will set a valid value if the value is invalid. A Menu_Value Message will come from the RX 

        local line = ctx.MenuLines[ctx.SelLine] -- Updat Value of SELECTED line
        if Value_Change_Step == 0 then  
            DSM_updateMenuValue(line.ValId, line.Val, line.Text, line)
            Value_Change_Step = 1
            Waiting_RX = 0 -- Keep on Transmitin State, since we want to send a ValidateMenuValue inmediatly after
        else  -- Validate the value
            DSM_validateMenuValue(line.ValId, line.Text, line)
            Value_Change_Step = 0
        end

    
    elseif ctx.Phase == PHASE.EXIT then
        if (DEBUG_ON) then LOG_write("CALL DSM_exitRequest()\n") end
        DSM_send(0x1F, 0x02, 0xAA)
    end
end

-----------------------------------------------------------------------------------------------------------
-- Parsing Responses

local function DSM_parseRxVersion()
    --ex: 0x09 0x01 0x00 0x15 0x02 0x22 0x01 0x00 0x14 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    local rxId = multiBuffer(13)
    DSM_Context.RX.Id = rxId
    DSM_Context.RX.Name = Get_RxName(rxId)
    DSM_Context.RX.Version = multiBuffer(14) .. "." .. multiBuffer(15) .. "." .. multiBuffer(16)
    if (DEBUG_ON) then LOG_write("RESPONSE Receiver=%s Version %s\n", DSM_Context.RX.Name, DSM_Context.RX.Version) end
end

local function DSM_parseMenu()
    --ex: 0x09 0x02 0x4F 0x10  0xA5 0x00 0x00 0x00 0x50 0x10 0x10 0x10 0x00 0x00 0x00 0x00
    --              MenuID     TextID    PrevID    NextID    BackID
    local ctx = DSM_Context
    local menu  = ctx.Menu
    menu.MenuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    menu.TextId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    menu.Text   = Get_Text(menu.TextId)
    menu.PrevId = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    menu.NextId = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))
    menu.BackId = Dsm_to_Int16(multiBuffer(20), multiBuffer(21))
    for i = 0, MAX_MENU_LINES do -- clear menu
        ctx.MenuLines[i] = { MenuId = 0, lineNum = 0, Type = 0, Text = "", TextId = 0, ValId = 0, Min=0, Max=0, Def=0, Val=nil }
    end
    ctx.CurLine = -1

    DSM_MenuPostProcessing(menu)

    if (DEBUG_ON) then LOG_write("RESPONSE Menu: %s\n", menu2String(menu)) end
    return menu
end


local function DSM_parseMenuLine()
    --ex: 0x09 0x03 0x00    0x10      0x00  0x1C  0xF9 0x00   0x10    0x10      0x00 0x00 0x00 0x00 0x03 0x00
    --ex: 0x09 0x03 0x61    0x10      0x00  0x6C  0x50 0x00   0x00    0x10      0x36 0x00 0x49 0x00 0x36 0x00
    --ex: 0x09 0x03 0x65    0x10      0x00  0x0C  0x51 0x00   0x00    0x10      0x00 0x00 0xF4 0x00 0x2E 0x00
    --              MenuLSB MenuMSB   line  Type  TextID      NextLSB NextMSB   Val_Min   Val_Max   Val_Def

    local ctx = DSM_Context
    local i = multiBuffer(14)
    local type = multiBuffer(15)
    local line = ctx.MenuLines[i]

    -- are we trying to override existing line
    if (line.Type > 0 and type == 0) then
        if (DEBUG_ON) then LOG_write("RESPONSE MenuLine: ERROR. Trying to Override: %s\n", menuLine2String(line)) end
        return line
    end

    ctx.CurLine = i

    line.lineNum = i
    line.MenuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    line.Type   = type
    line.TextId = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    line.Text   = nil -- Fill at Post processing
    line.ValId  = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))

    -- Singed int values
    line.Min = Dsm_to_SInt16(multiBuffer(20), multiBuffer(21))  
    line.Max = Dsm_to_SInt16(multiBuffer(22), multiBuffer(23)) 
    line.Def = Dsm_to_SInt16(multiBuffer(24), multiBuffer(25))

    DSM_MenuLinePostProcessing(line)

    if (DEBUG_ON) then LOG_write("RESPONSE MenuLine: %s\n", menuLine2String(line))  end
    return line
end

local function DSM_parseMenuValue()
    --ex: 0x09 0x04 0x53    0x10    0x00    0x10    0x00    0x00  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    --ex: 0x09 0x04 0x61    0x10    0x02    0x10    0x01    0x00  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    --              MenuLSB MenuMSB ValLSB  ValMSB  V_LSB   V_MSB

    -- Identify the line and update the value
    local ctx = DSM_Context
    local valId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    local value = Dsm_to_SInt16(multiBuffer(16), multiBuffer(17)) --Signed int 

    local updatedLine = nil
    for i = 0, MAX_MENU_LINES do -- Find the menu line for this value
        local line = ctx.MenuLines[i]
        if line ~= nil and line.Type ~= 0 then
            if line.Type ~= LINE_TYPE.MENU and line.ValId == valId then -- identifier of ValueId stored in the line
                line.Val = value
                ctx.CurLine = i
                updatedLine = line
                break
            end
        end
    end

    if (updatedLine == nil) then
        if (DEBUG_ON) then LOG_write("ERROR, Cant find Menu Line with ValID=%X to update\n", valId) end
    else
        if (DEBUG_ON) then LOG_write("RESPONSE MenuValue: UPDATED: %s\n", menuLine2String(updatedLine))
        end
    end
end

-- Creates a fake line do display an error in the GUI
local function DSM_Add_Error_Menu_Line(i, text)
    local ctx = DSM_Context
    local line = ctx.MenuLines[i]
    ctx.CurLine = i

    line.lineNum = i
    line.MenuId = ctx.Menu.MenuId
    line.Type = LINE_TYPE.MENU
    line.TextId = 0
    line.Text = text 
    line.ValId = ctx.Menu.MenuId

    -- Singed int values
    line.Min =0
    line.Max = 0
    line.Def = 0

    line.MinMaxOrig = ""
    line.Val = nil

    DSM_MenuLinePostProcessing(line)
end

------------------------------------------------------------------------------------------------------------
local function DSM_processResponse()
    local ctx = DSM_Context
    local cmd = multiBuffer(11) -- Response Command

    if (DEBUG_ON > 1) then LOG_write("%s: RESPONSE %s \n", phase2String(ctx.Phase), multiBuffer2String()) end
    if (DEBUG_ON and cmd > 0x00) then LOG_write("%3.3f %s: ", getElapsedTime(), phase2String(ctx.Phase)) end

    if cmd == 0x01 then -- read version
        DSM_parseRxVersion()
        Lib.Init_Text(DSM_Context.RX.Id)
        ctx.isReset = false  -- no longer resetting  
        ctx.Phase = PHASE.MENU_TITLE

    elseif cmd == 0x02 then -- read menu title
        local menu = DSM_parseMenu()

        -- Update Selected Line navigation
        if menu.NextId ~= 0 then
            ctx.SelLine = NEXT_BUTTON -- highlight Next
        else
            ctx.SelLine = BACK_BUTTON -- highlight Back
        end

        if (ctx.Menu.MenuId == 0x0001) then  -- Still in RESETTING MENU???
            -- Star from Start
            if (DEBUG_ON) then LOG_write("RX Reset:  Still not done, restart again!!!\n") end
            ctx.Menu.MenuId = 0
            ctx.Phase = PHASE.RX_VERSION
        else
            ctx.Phase = PHASE.MENU_LINES
        end
        

    elseif cmd == 0x03 then --  menu lines
        local line = DSM_parseMenuLine()

        -- Update Selected line navigation
        if (ctx.SelLine == BACK_BUTTON or ctx.SelLine == NEXT_BUTTON or ctx.SelLine == PREV_BUTTON)
            and isSelectableLine(line) then -- Auto select the current line
            ctx.SelLine = line.lineNum
        end

        ctx.Phase = PHASE.MENU_LINES

    elseif cmd == 0x04 then -- read menu values
        DSM_parseMenuValue()
        ctx.Phase = PHASE.MENU_VALUES

    elseif cmd == 0x05 then -- unknown... need to get through the lines...
        -- 0x09 0x05 0x01   0x01 0x00   0x00 0x00 0x00 0x07
        --           Line   MenuId                   ????
        local curLine = multiBuffer(12)
        if (DEBUG_ON) then LOG_write("RESPONSE MenuUknownLine_0x05: LineNum=%s  DATA=%s\n", curLine, multiBuffer2String()) end

        if (curLine==ctx.CurLine) then
            -- WEIRD BEHAVIOR
            -- We got the same line we already got. thi will continue
            -- on a loop and disconnect RX 
            DSM_Add_Error_Menu_Line(0,"\bError: Cannot Load Menu Lines from RX")
            if (DEBUG_ON) then LOG_write("ERROR: Received Same menu line\n") end
        end -- Got the next line.. keep requesting more
            
        ctx.CurLine = curLine
        ctx.Phase = PHASE.MENU_UNKNOWN_LINES

    elseif cmd == 0xA7 then -- answer to EXIT command
        if (DEBUG_ON) then LOG_write("RESPONSE Exit Confirm\n") end
        DSM_ReleaseConnection()

    elseif cmd == 0x00 then -- NULL response (or RX heartbeat)
        if (ctx.Phase == PHASE.WAIT_CMD) then -- Dont show null while waiting for command to no fill the logs
            if (DEBUG_ON > 1) then LOG_write("%3.3f %s: RESPONSE NULL\n", getElapsedTime(), phase2String(ctx.Phase)) end
        else
            if (DEBUG_ON) then LOG_write("%3.3f %s: RESPONSE NULL\n", getElapsedTime(), phase2String(ctx.Phase)) end
        end

        if (ctx.Phase == PHASE.VALUE_CHANGING) then
            ctx.Phase = PHASE.VALUE_CHANGING_WAIT
        end
    else
        if (DEBUG_ON) then LOG_write("RESPONSE Unknown Command (0x%X)  DATA=%s\n", cmd, multiBuffer2String()) end
    end

    return cmd
end

------------------------------------------------------------------------------------------------------------
local function DSM_Send_Receive()
    local context = DSM_Context

    if Waiting_RX == 0 then   -- Need to send a request
        Waiting_RX = 1
        DSM_sendRequest()

        multiBuffer(10, 0x00) -- Clear Response Buffer
        InactivityTime = getTime() + SEND_TIMEOUT  -- Reset Inactivity timeout 
    elseif multiBuffer(10) == 0x09 then -- RX data available
        local cmd = DSM_processResponse()

        multiBuffer(10, 0x00) -- Clear Response Buffer to know that we are done with the response

        if (cmd > 0x00) then -- Any non NULL response
            -- Only change to SEND mode if we received a valid response  (Ignore NULL Responses, that are really heartbeat i most cases)
            Waiting_RX = 0
            InactivityTime = getTime() + SEND_TIMEOUT  -- Reset Inactivity timeout 
            context.Refresh_Display = true
        end
    else
        -- Check if enouth time has passed from last transmit/receive activity
        if getTime() > InactivityTime then
            if (DEBUG_ON) then LOG_write("%3.3f %s: INACTIVITY TIMEOUT\n", getElapsedTime(), phase2String(context.Phase)) end

            InactivityTime = getTime() + SEND_TIMEOUT
            Waiting_RX = 0 -- Switch to Send mode to send heartbeat

            if context.Phase == PHASE.EXIT then -- Did not receive response to Exit_Request
                DSM_ReleaseConnection()
            end

            if context.Phase ~= PHASE.RX_VERSION and context.Phase ~= PHASE.VALUE_CHANGING_WAIT and
                context.Phase ~= PHASE.WAIT_CMD then
                -- Only change to WAIT_CMD if we are NOT already waiting for Data
                context.Phase = PHASE.WAIT_CMD
                context.Refresh_Display = true
            end

            if context.Phase == PHASE.RX_VERSION then
                -- Refresh screen again
                context.Refresh_Display = true
            end

            
        end
    end
end

-- Init
local function DSM_Init(toolName)
    local dateTime = getDateTime()
    local dateStr = dateTime.year.."-"..dateTime.mon.."-"..dateTime.day.."   "..dateTime.hour..":"..dateTime.min

    local ver, radio, maj, minor, rev, osname = getVersion()

    if (DEBUG_ON) then 
        LOG_write("---------------DSM New Session %s ----------------\n", toolName, dateStr)
        LOG_write("Radio Info:    %s\n", radio .. " " .. (osname or "OpenTx") .. "  " .. ver) 
        LOG_write("Date      :    %s\n", dateStr) 
        LOG_write("DsmLib Version :    %s\n", LIB_VERSION) 
    end

    DSM_Context.Phase = PHASE.RX_VERSION

    -- Phase Names
    PhaseText[PHASE.RX_VERSION]          = "RX_VERSION"
    PhaseText[PHASE.WAIT_CMD]            = "WAIT_CMD"
    PhaseText[PHASE.MENU_TITLE]          = "MENU_TITLE"
    PhaseText[PHASE.MENU_UNKNOWN_LINES]  = "MENU_UNKNOWN_LINES"
    PhaseText[PHASE.MENU_LINES]          = "MENU_LINES"
    PhaseText[PHASE.MENU_VALUES]         = "MENU_VALUES"
    PhaseText[PHASE.VALUE_CHANGING]      = "VALUE_CHANGING"
    PhaseText[PHASE.VALUE_CHANGING_WAIT] = "VALUE_CHANGING_WAIT"
    PhaseText[PHASE.VALUE_CHANGE_END]    = "VALUE_CHANGE_END"
    PhaseText[PHASE.EXIT]                = "EXIT"
    PhaseText[PHASE.EXIT_DONE]           = "EXIT_DONE"


    -- Line Types
    LineTypeText[LINE_TYPE.MENU]            = "M"
    LineTypeText[LINE_TYPE.LIST_MENU_NC]    = "LM_nc"
    LineTypeText[LINE_TYPE.LIST_MENU]       = "LM"
    LineTypeText[LINE_TYPE.LIST_MENU_TOG]   = "LM_tog"
    LineTypeText[LINE_TYPE.VALUE_NUM_I8_NC] = "V_nc"
    LineTypeText[LINE_TYPE.VALUE_PERCENT]   = "V_%"
    LineTypeText[LINE_TYPE.VALUE_DEGREES]   = "V_de"
    LineTypeText[LINE_TYPE.VALUE_NUM_I8]    = "V_i8"
    LineTypeText[LINE_TYPE.VALUE_NUM_I16]   = "V_i16"
    LineTypeText[LINE_TYPE.VALUE_NUM_SI16]  = "V_s16"
    LineTypeText[LINE_TYPE.LT_EMPTY]        = "Z"

    --RX names--
    RxName[0x0001] = "AR636B"
    RxName[0x0014] = "SPM4651T"
    RxName[0x0015] = "AR637T"
    RxName[0x0016] = "AR637TA"
    RxName[0x0018] = "FC6250HX"
    RxName[0x0019] = "AR630"
    RxName[0x001A] = "AR8360T"
    RxName[0x001E] = "AR631"
end

local function DSM_Init_Text(rxId)
    --Text to be displayed
    -- For menu lines who are not navigation to other menus (SubHeders or Plain text)
    -- you can use some formatting options:

    -- Text allightment:  /c = CENTER, /r = RIGHT
    -- Text effects:  /b = BOLD
    -- Text formatting: /p = PERCENT numbers (forced if not in Line Type=PERCENT)
    -- Navigaton: /M = Force to be a Menu button, when a menu navigates to itself, 
    --      is usually a message line.. but sometimes, we want to navigate to the same page to refresh values

    -- array List_Values:
    -- For some Menu LIST VALUES, special Lines of type:LIST_MENU1, the valod options seems not
    -- to be contiguos,  the array "Menu_List_Values" can help narrow down the 
    -- valid menu options. I think this should come from the RX, but cant find where.
    -- Most of the times, Limes of type LIST_MENU1 comes with a 0->244 value range that is not correct
    -- usually is Ihnibit + range of contiguos values, but cant seems to find in the RX data receive the values 
    -- to do it automatically

    List_Text[0x0001] = "Off"
    List_Text[0x0002] = "On"
    
    -- Ihn/Act List Options
    List_Text[0x0003] = "Inh"
    List_Text[0x0004] = "Act"

    -- Channel selection for SAFE MODE and GAINS on  FC6250HX
    List_Text[0x000C] = "Inhibit?" --?
    List_Text[0x000D] = "Gear"
    for i = 1, 7 do List_Text[0x000D + i] = "Aux" .. i end -- Aux channels

    -- Servo Output values.. 
    local servoOutputValues =  {0x0003,0x002D,0x002E,0x002F}  --Inh (GAP), 5.5ms, 11ms, 22ms. Fixing L_m1 with 0..244 range!
    List_Text[0x002D] = "5.5ms"
    List_Text[0x002E] = "11ms"
    List_Text[0x002F] = "22ms"

    -- Gain Values
    local gainValues = {0x0032,0x0033,0x0034}  -- 1X, 2X, 4X   -- Fixing L_m1 with 0..244 range!
    List_Text[0x0032] = "1 X"
    List_Text[0x0033] = "2 X"
    List_Text[0x0034] = "4 X"

    -- List of Channels for Safe, Gains, Panic, except FC6250HX that uses other range (0x00C..0x015)
    -- the valid range Starts with GEAR if enabled  (Thr,Ail,Ele,Rud are not valid, the RX reject them ) 
    -- Valid Values: Inhibit? (GAP), Gear,Aux1..Aux7,X-Plus-1..XPlus-8
    local channelValues = {0x0035,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049} 
    
    List_Text[0x0035] = "Inhibit?" --?
    List_Text[0x0036] = "Throttle"
    List_Text[0x0037] = "Aileron"
    List_Text[0x0038] = "Elevator"
    List_Text[0x0039] = "Rudder"
    List_Text[0x003A] = "Gear"
    for i = 1, 7 do List_Text[0x003A + i] = "Aux" .. i end -- Aux channels on AR637T

    for i = 1, 8 do -- 41..49  on AR637T  -- This don't seem OK
        List_Text[0x0041 + i] = "XPlus-" .. i
    end

    -- ****No longer overrides of previous XPlus values, since using different array
    -- for List_Text values

    Text[0x0040] = "Roll" 
    Text[0x0041] = "Pitch"
    Text[0x0042] = "Yaw"
    Text[0x0043] = "Gain /c/b" -- FC6250HX, AR631
    Text[0x0045] = "Differential"
    Text[0x0046] = "Priority"
    Text[0x0049] = "Output Setup" -- FC6250HX, AR631
    --******

    Text[0x004A] = "Failsafe"
    Text[0x004B] = "Main Menu"
    Text[0x004E] = "Position"

    Text[0x0050] = "Outputs";

    Text[0x0051] = "Output Channel 1"
    Text[0x0052] = "Output Channel 2"
    Text[0x0053] = "Output Channel 3"
    Text[0x0054] = "Output Channel 4"
    Text[0x0055] = "Output Channel 5"
    Text[0x0056] = "Output Channel 6"

    if (rxId ~= RX.FC6250HX) then  -- Restrictions for non FC6250HX
        List_Values[0x0051]=servoOutputValues
        List_Values[0x0052]=servoOutputValues
        List_Values[0x0053]=servoOutputValues
        List_Values[0x0054]=servoOutputValues
        List_Values[0x0055]=servoOutputValues
        List_Values[0x0056]=servoOutputValues
    end

    -- FailSafe Options 
    --Text[0x005E]="Inhibit"
    List_Text[0x005F] = "Hold Last"
    List_Text[0x0060] = "Preset"
    --Text[0x0061]="Custom"

    --FC6250HX 
    Text[0x0071] = "Proportional"
    Text[0x0072] = "Integral"
    Text[0x0073] = "Derivate"

    -- Flight mode channel selection
    Text[0x0078] = "FM Channel"
    if (rxId ~= RX.FC6250HX) then List_Values[0x0078]=channelValues end --FC6250HX uses other range

    Text[0x0080] = "Orientation"
    Text[0x0082] = "Heading"
    Text[0x0085] = "Frame Rate"
    Text[0x0086] = "System Setup"
    Text[0x0087] = "F-Mode Setup"
    Text[0x0088] = "Enabled F-Modes"

    -- Gain  channel selection
    Text[0x0089] = "Gain Channel"
    if (rxId ~= RX.FC6250HX) then List_Values[0x0089]=channelValues end --FC6250HX uses other range
    -- Gain Sensitivity 
    Text[0x008A] = "Gain Sensitivity/r";  List_Values[0x008A]=gainValues  -- Right Alight, (L_M1 was wide open range 0->244)

    Text[0x008B] = "Panic"
    Text[0x008E] = "Panic Delay"
    Text[0x0090] = "Apply"

    Text[0x0091] = "Begin" -- FC6250HX: Callibration Menu -> Begin..Start, Complete, Done
    Text[0x0092] = "Start"
    Text[0x0093] = "Complete"
    Text[0x0094] = "Done"

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

    List_Text[0x00B5] = "Inhibit"
    List_Text[0x00B6] = "FM1"
    List_Text[0x00B7] = "FM2"
    List_Text[0x00B8] = "FM3"
    List_Text[0x00B9] = "FM4"
    List_Text[0x00BA] = "FM5"
    List_Text[0x00BB] = "FM6"
    List_Text[0x00BC] = "FM7"
    List_Text[0x00BD] = "FM8"
    List_Text[0x00BE] = "FM9"
    List_Text[0x00BF] = "FM10"

    Text[0x00BE] = "Unknown_BE" -- Used in Reset menu (0x0001) while the RX is rebooting

    Text[0x00C7] = "Calibrate Sensor"
    Text[0x00C8] = "Complete" -- FC6250HX calibration complete 
    Text[0x00CA] = "SAFE/Panic Mode Setup"
    Text[0x00CD] = "Level model and capture attiude/M"; -- Different from List_Text , and force it to be a menu button

    -- RX Orientations for AR631/AR637  
    -- Optionally attach an Image to display
    List_Text[0x00CB] = "RX Pos 1";  List_Text_Img[0x00CB]  = "rx_pos_1.png|Pilot View: RX Label Up, Pins Back" 
    List_Text[0x00CC] = "RX Pos 2";  List_Text_Img[0x00CC]  = "rx_pos_2.png|Pilot View: RX Label Left, Pins Back" 
    List_Text[0x00CD] = "RX Pos 3";  List_Text_Img[0x00CD]  = "rx_pos_3.png|Pilot View: RX Label Down, Pins Back" 
    List_Text[0x00CE] = "RX Pos 4";  List_Text_Img[0x00CE]  = "rx_pos_4.png|Pilot View: RX Label Right, Pins Back" 
    List_Text[0x00CF] = "RX Pos 5";  List_Text_Img[0x00CF]  = "rx_pos_5.png|Pilot View: RX Label UP, Pins to Front" 
    List_Text[0x00D0] = "RX Pos 6";  List_Text_Img[0x00D0]  = "rx_pos_6.png|Pilot View: RX Label Left, Pins Front" 
    List_Text[0x00D1] = "RX Pos 7";  List_Text_Img[0x00D1]  = "rx_pos_7.png|Pilot View: RX Label Down, Pins Front" 
    List_Text[0x00D2] = "RX Pos 8";  List_Text_Img[0x00D2]  = "rx_pos_8.png|Pilot View: RX Label Right, Pins Front" 
    List_Text[0x00D3] = "RX Pos 9";  List_Text_Img[0x00D3]  = "rx_pos_9.png|Pilot View: RX Label Up, Pins Left" 
    List_Text[0x00D4] = "RX Pos 10"; List_Text_Img[0x00D4]  = "rx_pos_10.png|Pilot View: RX Label Back, Pins Left" 
    List_Text[0x00D5] = "RX Pos 11"; List_Text_Img[0x00D5]  = "rx_pos_11.png|Pilot View: RX Label Down, Pins Left" 
    List_Text[0x00D6] = "RX Pos 12"; List_Text_Img[0x00D6]  = "rx_pos_12.png|Pilot View: RX Label Front, Pins Left" 
    List_Text[0x00D7] = "RX Pos 13"; List_Text_Img[0x00D7]  = "rx_pos_13.png|Pilot View: RX Label Up, Pins Right" 
    List_Text[0x00D8] = "RX Pos 14"; List_Text_Img[0x00D8]  = "rx_pos_14.png|Pilot View: RX Label Back, Pins Right" 
    List_Text[0x00D9] = "RX Pos 15"; List_Text_Img[0x00D9]  = "rx_pos_15.png|Pilot View: RX Label Down, Pins Right" 
    List_Text[0x00DA] = "RX Pos 16"; List_Text_Img[0x00DA]  = "rx_pos_16.png|Pilot View: RX Label Front, Pins Right" 
    List_Text[0x00DB] = "RX Pos 17"; List_Text_Img[0x00DB]  = "rx_pos_17.png|Pilot View: RX Label Back, Pins Down" 
    List_Text[0x00DC] = "RX Pos 18"; List_Text_Img[0x00DC]  = "rx_pos_18.png|Pilot View: RX Label Left, Pins Down" 
    List_Text[0x00DD] = "RX Pos 19"; List_Text_Img[0x00DD]  = "rx_pos_19.png|Pilot View: RX Label Front, Pins Down" 
    List_Text[0x00DE] = "RX Pos 20"; List_Text_Img[0x00DE]  = "rx_pos_20.png|Pilot View: RX Label Right, Pins Down" 
    List_Text[0x00DF] = "RX Pos 21"; List_Text_Img[0x00DF]  = "rx_pos_21.png|Pilot View: RX Label Back, Pins Up" 
    List_Text[0x00E0] = "RX Pos 22"; List_Text_Img[0x00E0]  = "rx_pos_22.png|Pilot View: RX Label Left, Pins Up" 
    List_Text[0x00E1] = "RX Pos 23"; List_Text_Img[0x00E1]  = "rx_pos_23.png|Pilot View: RX Label Front, Pins Up" 
    List_Text[0x00E2] = "RX Pos 24"; List_Text_Img[0x00E2]  = "rx_pos_24.png|Pilot View: RX Label Right, Pins Up" 
    List_Text[0x00E3] = "RX Pos Invalid";  List_Text_Img[0x00E3]  = "rx_pos_25.png|Cannot detect orientation of RX" 

    Text[0x00D1] = "Unknown_D1"   -- TODO: Find the Spektrum Value  (Orientation Save&Reset final page  AR631) 
    --FC6250HX
    Text[0x00D2] = "Panic Channel" 
    if (rxId ~= RX.FC6250HX) then List_Values[0x00D2]=channelValues end --FC6250HX uses other range

    Text[0x00D3] = "Swashplate"
    Text[0x00D5] = "Agility"
    Text[0x00D8] = "Stop"
    Text[0x00DA] = "SAFE/c/b"  --SubTitle
    Text[0x00DB] = "Stability"
    Text[0x00DC] = "Deg. per sec"
    Text[0x00DD] = "Tail rotor"
    Text[0x00DE] = "Setup"
    Text[0x00DF] = "AFR"
    Text[0x00E0] = "Collective"
    Text[0x00E1] = "Subtrim"
    Text[0x00E2] = "Phasing"
    Text[0x00E4] = "E-Ring"
    
    Text[0x00E7] = "Left"
    Text[0x00E8] = "Right"

    List_Text[0x00F2] = "Fixed"
    List_Text[0x00F3] = "Adjustable"

    Text[0x00F9] = "Gyro settings"
    Text[0x00FE] = "Stick Priority/c/b " --SubTitle

    Text[0x0100] = "Make sure the model has been"
    Text[0x0101] = "configured, including wing type,"
    Text[0x0102] = "reversing, travel, trimmed, etc."
    Text[0x0103] = "before continuing setup."
    Text[0x0104] = "" -- empty??
    Text[0x0105] = "" -- empty??

    Text[0x0106] = "Any wing type, channel assignment,"
    Text[0x0107] = "subtrim, or servo reversing changes"
    Text[0x0108] = "require running through initial"
    Text[0x0109] = "setup again."
    Text[0x010A] = "" -- empty??
    Text[0x010B] = "" -- empty??
    
    Text[0x0190] = "DONT USE! Relearn Servo Settings"
    Text[0x019C] = "Enter Receiver Bind Mode"
    Text[0x01D7] = "SAFE Select Channel"
    Text[0x01DC] = "AS3X/c/b"       -- Subtitle, Center+bold 
    Text[0x01DD] = "AS3X Settings"
    Text[0x01DE] = "AS3X Gains"
    Text[0x01E0] = "Rate Gains/c/b" -- SubTitle, Center+bold 
    Text[0x01E2] = "SAFE Settings"
    Text[0x01E3] = "SAFE Gains"
    Text[0x01E6] = "Attitude Trim/c/b" -- SubTitle, Center+bold 
    Text[0x01E7] = "Envelope"
    Text[0x01E9] = "Roll Right"
    Text[0x01EA] = "Roll Left"
    Text[0x01EB] = "Pitch Down"
    Text[0x01EC] = "Pitch Up"
    Text[0x01EE] = "Throttle to Pitch"
    Text[0x01EF] = "Low Thr to Pitch/c/b" -- SubTitle, Center+bold 
    Text[0x01F0] = "High Thr to Pitch/c/b" -- SubTitle, Center+bold 
    Text[0x01F3] = "Threshold"
    Text[0x01F4] = "Angle"
    Text[0x01F6] = "Failsafe Angles/c/b" -- SubTitle, Center+bold 

    --Inh, Self-Level/Angle Dem, Envelope -- (L_M1 was wide open)
    Text[0x01F8] = "Safe Mode";    List_Values[0x01F8]=safeModeOptions 

    Text[0x01F9] = "SAFE Select/c/b "  -- SubTitle
    Text[0x01FC] = "Panic Flight Mode"
    Text[0x01FD] = "SAFE Failsafe FMode"
    Text[0x0208] = "Decay"
    Text[0x0209] = "Save to Backup"
    Text[0x020A] = "Restore from Backup"
    Text[0x020D] = "First Time SAFE Setup"

    -- First time safe setup Page 3 : 
    Text[0x020E] = "AS3X gains must be tuned"
    Text[0x020F] = "and active in SAFE Flight Modes"
    Text[0x0210] = "to help reduce wobble."
    Text[0x0211] = "" -- empty
    Text[0x0212] = "" -- empty
    Text[0x0213] = "" -- empty

    Text[0x021A] = "Set the model level,"
    Text[0x021B] = "and press Continue."
    Text[0x021C] = "" -- empty??
    Text[0x021D] = "" -- empty??

    Text[0x021F] = "Set the model on its nose,"
    Text[0x0220] = "and press Continue. If the"
    Text[0x0221] = "orientation on the next"
    Text[0x0222] = "screen is wrong go back"
    Text[0x0223] = "and try again."
    
    Text[0x0224] = "Continue"
    Text[0x0226] = "Angle Limits/c/b "
    Text[0x0227] = "Other settings"
    Text[0x0229] = "Set Orientation Manually"

    -- Factory Default Warning 
    Text[0x022B] = "WARNING!"
    Text[0x022C] = "This will reset the"
    Text[0x022D] = "configuration to factory"
    Text[0x022E] = "defaults. This does not"
    Text[0x022F] = "affect the backup config."
    Text[0x0230] = "" -- empty??

    -- Backup Warning
    Text[0x0231] = "This will overwrite the"
    Text[0x0232] = "backup memory with your"
    Text[0x0233] = "current configuartion."
    Text[0x0234] = "" -- blank line
    Text[0x0235] = "" -- blank line

    -- Restore from Backup Warning
    Text[0x0236] = "This will overwrite the"
    Text[0x0237] = "current config with"
    Text[0x0238] = "that which is in"
    Text[0x0239] = "the backup memory."
    Text[0x023A] = "" -- blank line

    Text[0x023D] = "Copy Flight Mode Settings"
    Text[0x023E] = "Source Flight Mode"
    Text[0x023F] = "Target Flight Mode"
    Text[0x0240] = "Utilities"

    Text[0x024C] = "Gains will be captured on"
    Text[0x024D] = "Captured gains will be"
    Text[0x024E] = "Gains on"

    Text[0x024F] = "were captured and changed"
    Text[0x0250] = "from Adjustable to Fixed"

    Text[0x0254] = "Postive = Up, Negative = Down"

    --Utilities, Copy flight mode  (Copy Confirmation, oveerriding FM)
    Text[0x0251] = "Are you sure you want to ovewrite the \"Target\""
    Text[0x0252] = "with the \"Source\" ? "
    Text[0x0253] = "" -- Blank

    -- First time safe setup Page 1  (maybe ask to select Flight Mode cannel)
    Text[0x0255] = "Before setting up SAFE"
    Text[0x0256] = "a Flight Mode channel"
    Text[0x0257] = "most be configured."

    --First time safe setup Page 2  (something related for flight mode)
    Text[0x025A] = "Select the desired flight mode"
    Text[0x025B] = "switch position to adjust settings"
    Text[0x025C] = "for each flight mode"
    Text[0x025D] = "" -- Blank
    Text[0x025E] = "" -- Blank

    --Utilities, Copy flight mode  (Confirm)
    Text[0x0259] = "YES" 
    Text[0x0260] = "WARNING: \"Target\""
    Text[0x0261] = "flight mode will be overwritten"
    Text[0x0262] = "by \"Source\""

    Text[0x0263] = "Fixed/Adjustable Gains /c/b"
    Text[0x0266] = "Heading Gain/c/b"
    Text[0x0267] = "Positive = Nose Up/Roll Right"
    Text[0x0268] = "Negative = Nose Down/Roll Left"
    Text[0x0269] = "SAFE - Throttle to Pitch"
    Text[0x026A] = "Use CAUTION for Yaw gain!/b" -- SubTitle

    Text[0x8000] = "Flight Mode/c/b"  --FC6250HX:   1=NORMAL 2= Stunt-1, 3=Stunt-2, 4=Hold
    Text[0x8001] = "Flight Mode/c/b"  -- WAS "Flight Mode 1".. This usually is a Flight Mode w value relative to 0 (AR631/AR637)
    Text[0x8002] = "Flight Mode 2/c/b"
    Text[0x8003] = "Flight Mode 3/c/b"
end

-- Adjust the displayed value for Flight mode as needed
local function GetFlightModeValue(textId, header, value)
    local out = value

    if (textId == 0x8000) then
        if (DSM_Context.RX.Id == RX.FC6250HX) then
            -- Helicopter Flights modes 
            if (value==1) then out = header .. "  1 / Normal" 
            elseif (value==2) then out = header .. " 2 / Stunt 1" 
            elseif (value==3) then out = header .. " 3 / Stunt 2" 
            elseif (value==4) then out = header .. "  4 / Hold" 
            else
                out = header .. " " .. value
            end
        else
            -- No adjustment needed
            out = header .. " " .. value
        end
    elseif (textId == 0x8001) then -- AR632/637
        -- Seems that we really have to add +1 to the value, so Flight Mode 0 is Really Flight Mode 1
        out = header .. " " .. (value + 1)
    else
        -- Default, return the value as we Have it 
        out = header .. " " .. value
    end
    return out
end

------------------------------------------------------------------------------------------------------------
-- Lib EXPORTS

-- Export Constants
Lib.PHASE       = PHASE
Lib.LINE_TYPE   = LINE_TYPE
Lib.RX          = RX
Lib.DISP_ATTR   = DISP_ATTR

Lib.BACK_BUTTON = BACK_BUTTON
Lib.NEXT_BUTTON = NEXT_BUTTON
Lib.PREV_BUTTON = PREV_BUTTON
Lib.MAX_MENU_LINES = MAX_MENU_LINES

-- Export Shared Context Variables
Lib.DSM_Context = DSM_Context

-- Export Functions
Lib.LOG_write = LOG_write
Lib.LOG_close = LOG_close
Lib.getElapsedTime = getElapsedTime
Lib.Get_Text = Get_Text
Lib.Get_List_Text = Get_List_Text
Lib.Get_List_Text_Img = Get_List_Text_Img

Lib.phase2String = phase2String
Lib.lineValue2String = lineValue2String
Lib.menu2String = menu2String
Lib.menuLine2String = menuLine2String

Lib.isSelectableLine = isSelectableLine
Lib.isEditableLine = isEditableLine
Lib.isListLine = isListLine
Lib.isPercentValueLine = isPercentValueLine
Lib.isPercentValueLineByMinMax = isPercentValueLineByMinMax
Lib.isNumberValueLine = isNumberValueLine
Lib.isDisplayAttr = isDisplayAttr
Lib.isFlightModeLine = isFlightModeLine
Lib.GetFlightModeValue = GetFlightModeValue

Lib.StartConnection = DSM_StartConnection
Lib.ReleaseConnection = DSM_ReleaseConnection
Lib.ChangePhase = DSM_ChangePhase
Lib.MenuPostProcessing = DSM_MenuPostProcessing
Lib.MenuLinePostProcessing = DSM_MenuLinePostProcessing
Lib.Value_Add = DSM_Value_Add
Lib.Value_Default = DSM_Value_Default
Lib.Value_Write_Validate = DSM_Value_Write_Validate
Lib.GotoMenu = DSM_GotoMenu
Lib.MoveSelectionLine = DSM_MoveSelectionLine
Lib.Send_Receive = DSM_Send_Receive
Lib.Init = DSM_Init
Lib.Init_Text = DSM_Init_Text

return Lib
