--- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX/EdgeTx                                           #
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

local Log, DEBUG_ON = ... -- Parameters


local MenuLib = {  }

local PHASE = {
    INIT = 0,
    RX_VERSION = 1,
    WAIT_CMD = 2,
    MENU_TITLE = 3,
    MENU_REQ_TX_INFO = 4,
    MENU_LINES = 5, 
    MENU_VALUES = 6,
    VALUE_CHANGING = 7, 
    VALUE_CHANGING_WAIT = 8, 
    VALUE_CHANGE_END = 9,
    EXIT = 10, 
    EXIT_DONE = 11
}

local LINE_TYPE = {
    MENU = 0x1C,
    LIST_MENU = 0x0C,  -- List: INC Change + Validate
    LIST_MENU_NC = 0x6C,  -- List:  No Incremental Change    
    LIST_MENU_NC2 = 0x6D,  -- List:  No Incremental Change   (Frame Rate Herz)  
    LIST_MENU_TOG = 0x4C, -- List:  Incremental Change,  sometimes bolean/Toggle menu (if only 2 values)
    LIST_MENU_ORI = 0xCC, -- List:  Incremental Change,  Orientation Heli

    VALUE_NUM_I8_NC = 0x60,  --  8 bit number, no incremental change
    VALUE_PERCENT = 0xC0, -- 8 bit number, Signed, percent
    VALUE_DEGREES  = 0xE0, -- 8 bit number, Signed, Degress
    VALUE_NUM_I8 = 0x40, -- 8 bit number
    VALUE_NUM_I16 = 0x41, -- 16 Bit number
    VALUE_NUM_SI16 = 0xC1, -- 16 bit number, Signed  
    
    LT_EMPTY = 0x00
}

-- Bug in Lua compiler, confusing with global BOLD and RIGHT
local DISP_ATTR = {
    _BOLD = 0x01,  _RIGHT=0x02, _CENTER=0x04, PERCENT = 0x10, DEGREES=0x20, FORCED_MENU = 0x40 
}

--RX IDs--
local RX = {
    AR636B   = 0x0001,
    SPM4651T = 0x0014,
    AR637T   = 0x0015,
    AR637TA  = 0x0016,
    FC6250HX = 0x0018,
    AR630    = 0x0019,
    AR8360T  = 0x001A,
    AR10360T = 0x001C,
    AR631    = 0x001E
}

local DSM_Context = {
    Phase = PHASE.INIT,
    Menu = { MenuId = 0, Text = "", TextId = 0, PrevId = 0, NextId = 0, BackId = 0 },
    MenuLines = {},
    RX = { Id=0, Name = "", Version = "" },
    Refresh_Display = true,
    SendDataToRX = 1,

    SelLine = 0,        -- Current Selected Line
    EditLine = nil,     -- Current Editing Line 
    CurLine = -1,       -- Current Line Requested/Parsed via h message protocol 
    isReset = false     -- false when starting from scracts, true when starting from Reset
}

function DSM_Context.isEditing() return DSM_Context.EditLine~=nil end

local MAX_MENU_LINES = 6
local BACK_BUTTON    = -1                   -- Tread it as a display line #-1
local NEXT_BUTTON    = MAX_MENU_LINES + 1   -- Tread it as a display line #7
local PREV_BUTTON    = MAX_MENU_LINES + 2   -- Tread it as a display line #7

-- Text Arrays for Display Text and Debuging 
local PhaseText = {}
local LineTypeText = {}

local Text = {}             -- Text for Menu and Menu Lines   (Headers only)
local List_Text = {}        -- Messages for List Options (values only)
local List_Text_Img = {}    -- If the Text has Attached Images
local List_Values = {}      -- Additiona restrictions on List Values when non contiguos  (L_M1 lines has this problem) 
local Flight_Mode = {[0]="Fligh Mode"}
local RxName = {}

local StartTime = 0

------------------------------------------------------------------------------------------------------------
-- Get Elapsed Time since we started running the Script. Return a float in format: Seconds.Milliseconds
function MenuLib.getElapsedTime()
    local t = getTime()
    if (StartTime == 0) then StartTime = t end

    return ((t - StartTime) * 10) / 1000
end

-------------  Line Type helper functions ------------------------------------------------------------------

-- Check if the text are Flight modes, who will be treated different for Display
function MenuLib.isFlightModeLine(line)
    return (line.TextId >= 0x8000 and line.TextId <= 0x8003)
end

function MenuLib.isSelectableLine(line)   -- is the display line Selectable??
    -- values who are not selectable
    if (line.Type == 0) then return false end -- Empty Line
    if (line.Type == LINE_TYPE.MENU and line.ValId == line.MenuId and bit32.band(line.TextAttr, DISP_ATTR.FORCED_MENU)==0) then return false end -- Menu that navigates to Itself?
    if (line.Min==0 and line.Max==0 and line.Def==0) then return false end -- Values with no Range are only for display 
    if (line.Type == LINE_TYPE.VALUE_NUM_I8_NC and MenuLib.isFlightModeLine(line)) then return false end -- Flight mode is not Selectable
    return true
end

function MenuLib.isEditableLine(line) -- is the display line editable??
    -- values who are not editable
    if (line.Type == 0 or line.Type == LINE_TYPE.MENU) then return false end -- Menus are not editable
    if (line.Min==0 and line.Max==0 and line.Def==0) then return false end -- Values with no Range are only for display 
    if (line.Type == LINE_TYPE.VALUE_NUM_I8_NC and MenuLib.isFlightModeLine(line)) then return false end -- Flight mode is not Editable 
    -- any other is Editable
    return true
end

function MenuLib.isListLine(line)   -- is it a List of options??
    if (line.Type == LINE_TYPE.LIST_MENU_NC or line.Type == LINE_TYPE.LIST_MENU or 
        line.Type == LINE_TYPE.LIST_MENU_TOG or line.Type == LINE_TYPE.LIST_MENU_NC2 or
        line.Type == LINE_TYPE.LIST_MENU_ORI) then return true end
    return false
end

function MenuLib.isPercentValueLineByMinMax(line)
    return
    (line.Min == 0 and  line.Max == 100) or ( line.Min == -100 and  line.Max == 100) or 
    ( line.Min == 0 and  line.Max == 150) or ( line.Min == -150 and  line.Max == 150)
end

function MenuLib.isPercentValueLine(line)   -- is it a Percent value??
    if (line.Type == LINE_TYPE.VALUE_PERCENT)  then return true end
    return false
end

function MenuLib.isNumberValueLine(line)     -- is it a number ??
    if (MenuLib.isListLine(line) or line.Type == LINE_TYPE.MENU or line.Type == 0) then return false
    else return true end
end

function MenuLib.isIncrementalValueUpdate(line)
    if (line.Type == LINE_TYPE.LIST_MENU_NC or line.Type == LINE_TYPE.LIST_MENU_NC2 or
        line.Type == LINE_TYPE.VALUE_NUM_I8_NC or line.Type == LINE_TYPE.VALUE_DEGREES) then return false end
    return true
end

------------------------------------------------------------------------------------------------------------
function MenuLib.Get_Text(index)
    if (index >= 0x8000) then
        return Flight_Mode[0]
    end

    local out = Text[index]   -- Find in regular header first
    if out== nil then
        out = List_Text[index]  -- Try list values, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "Unknown_" .. string.format("%X", index)
    end
    return out
end

function MenuLib.Get_List_Text(index)
    local out = List_Text[index]   -- Try to find the message in List_Text
    if out == nil then
        out = Text[index]  -- Try list headers, don't think is necesary, but just playing Safe
    end
    if out == nil then -- unknown...
        out = "UnknownLT_" .. string.format("%X", index)
    end
    return out
end

function MenuLib.Get_List_Text_Img(index) 
    local out = List_Text_Img[index]
    return out
end

function MenuLib.Get_List_Values(index)
    local out = List_Values[index]
    return out
end

function MenuLib.Get_RxName(index) 
    local out = RxName[index]
    return out or ("RX_" .. string.format("%X", index))
end

----------- Debugging 2-String functions -------------------------------------------------------------------

function MenuLib.phase2String(index)
    local out = PhaseText[index]
    return out or ("Phase_" .. string.format("%X", index))
end

function MenuLib.lineType2String(index)
    local out = LineTypeText[index]
    return out or ("LT_" .. string.format("%X", index or 0xFF))
end

function MenuLib.lineValue2String(l)
    if (DEBUG_ON == 0) then
        return ""
    end
    if (l ~= nil and l.Val ~= nil) then
        local value = l.Val
        if MenuLib.isListLine(l) then
            value = value .. "|\"" .. MenuLib.Get_List_Text(l.Val + l.TextStart) .. "\""
        else
            value = value..(l.Format or "")
       end
        return value
    end
    return "nil"
end

function MenuLib.menu2String(m)
    local txt = "Menu[]"
    if (m ~= nil) then
        txt = string.format("M[Id=0x%X P=0x%X N=0x%X B=0x%X Text=\"%s\"[0x%X]]",
            m.MenuId, m.PrevId, m.NextId, m.BackId, m.Text, m.TextId)
    end
    return txt
end

function MenuLib.menuLine2String(l)
    local txt = "Line[]"
    if (l ~= nil) then
        local value = ""
        local range = ""
        if l.Type~=LINE_TYPE.MENU then
          value = "Val="..MenuLib.lineValue2String(l)
            if MenuLib.isListLine(l) then
               range = string.format("NL=(%s->%s,%s,S=%s) ",l.Min, l.Max, l.Def, l.TextStart ) 
               range = range .. (l.MinMaxOrig or "")
            else
                range = string.format("[%s->%s,%s]",l.Min, l.Max, l.Def) 
            end    
        end

        txt = string.format("L[#%s T=%s VId=0x%X Text=\"%s\"[0x%X] %s %s MId=0x%X A=0x%X]",
            l.lineNum, MenuLib.lineType2String(l.Type), l.ValId,
            l.Text, l.TextId,
            value,
           range,
            l.MenuId,
            l.TextAttr
        )
    end
    return txt
end

-----------------------------------------------------------------------------------------------------------
-- Post Procssing Line from Raw values receive by RX or Simulation

function MenuLib.isDisplayAttr(attr, bit)
    return (bit32.band(attr,bit)>0)
end

function MenuLib.ExtractDisplayAttr(text1, attr)
    local text = text1, pos;

    for i=1,2 do
        text, pos = string.gsub(text, "/c$", "")
        if (pos>0) then -- CENTER
            attr = bit32.bor(attr, DISP_ATTR._CENTER)
        end

        text, pos = string.gsub(text, "/r$", "")
        if (pos>0) then -- RIGHT
            attr = bit32.bor(attr, DISP_ATTR._RIGHT)
        end

        text, pos = string.gsub(text, "/p$", "")
        if (pos>0) then -- Percent TEXT
            attr = bit32.bor(attr, DISP_ATTR.PERCENT)
        end

        text, pos = string.gsub(text, "/b$", "")
        if (pos>0) then -- BOLD TEXT
            attr = bit32.bor(attr, DISP_ATTR._BOLD)
        end

        text, pos = string.gsub(text, "/m$", "")
        if (pos>0) then -- FORCED MENU Button 
            attr = bit32.bor(attr, DISP_ATTR.FORCED_MENU)
        end
    end

    return text, attr 
end

function MenuLib.MenuPostProcessing(menu)
    menu.Text, menu.TextAttr =  MenuLib.ExtractDisplayAttr(menu.Text,menu.TextAttr or 0)
end

function MenuLib.MenuLinePostProcessing(line)
    if (line.Text==nil) then
        line.Text   = MenuLib.Get_Text(line.TextId) -- Get Textual Line headeing text 
    end

    -- Text formatting options
    line.Text, line.TextAttr = MenuLib.ExtractDisplayAttr(line.Text,line.TextAttr or 0)

    if line.Type == LINE_TYPE.MENU then
        -- nothing to do on menu entries
        line.Val=nil
    elseif MenuLib.isListLine(line) then
        -- Original Range  for Debugging
        line.MinMaxOrig = "[" .. line.Min .. "->" .. line.Max .. "," .. line.Def .. "]"

        -- Normalize Min/Max to be relative to Zero
        line.TextStart = line.Min
        line.Def = line.Def - line.Min -- normalize default value 
        line.Max = line.Max - line.Min -- normalize max index
        line.Min = 0 -- min index
    else -- default to numerical value
        if MenuLib.isPercentValueLine(line) or MenuLib.isPercentValueLineByMinMax(line) then
            -- either explicit Percent or NO-Change value, but range is %Percent
            line.Format ="%"
            line.TextAttr = bit32.bor(line.TextAttr,DISP_ATTR.PERCENT)
        elseif (line.Type == LINE_TYPE.VALUE_DEGREES) then
            line.Format ="o"
            line.TextAttr = bit32.bor(line.TextAttr,DISP_ATTR.DEGREES)
        end
    end

    line.MinMaxDebug =  MenuLib.lineType2String(line.Type).."  "..(line.MinMaxOrig or "")
end


function MenuLib.ChangePhase(newPhase)
    DSM_Context.Phase = newPhase
    DSM_Context.SendDataToRX = 1
end

function MenuLib.Value_Add(line, inc)
    if (DEBUG_ON) then Log.LOG_write("%3.3f %s: DSM_Value_Add(%s,%s)\n", 
            MenuLib.getElapsedTime(), MenuLib.phase2String(DSM_Context.Phase), inc, MenuLib.menuLine2String(line)) end

    local skipIncrement = false
    local values  = nil
    local origVal = line.Val

    -- Use local validation for LIST_MENU1 when the range is wide open 
    -- Also use if for some LIST_MENU0 that the Range seems incorrect
    if (MenuLib.isListLine(line)) then -- and line.Type==LINE_TYPE.LIST_MENU1 and line.Min==0 and line.Max==244) then
        values = MenuLib.Get_List_Values(line.TextId)
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

    if (origVal~=line.Val and MenuLib.isIncrementalValueUpdate(line)) then 
        -- Update RX value on every change 
        MenuLib.ChangePhase(PHASE.VALUE_CHANGING)
    end
end

function MenuLib.Value_Default(line)
    local origVal = line.Val
    if (DEBUG_ON) then Log.LOG_write("%3.3f %s: DSM_Value_Default(%s)\n", 
        MenuLib.getElapsedTime(), MenuLib.phase2String(DSM_Context.Phase), MenuLib.menuLine2String(line)) end

    line.Val = line.Def
    if (origVal~=line.Val and MenuLib.isIncrementalValueUpdate(line)) then 
        -- Update RX value on every change 
        MenuLib.ChangePhase(PHASE.VALUE_CHANGING)
    end
end

function MenuLib.Value_Write_Validate(line)
    if (DEBUG_ON) then Log.LOG_write("%3.3f %s: DSM_Value_Write_Validate(%s)\n", 
        MenuLib.getElapsedTime(), MenuLib.phase2String(DSM_Context.Phase), MenuLib.menuLine2String(line)) end

    MenuLib.ChangePhase(PHASE.VALUE_CHANGE_END) -- Update + Validate value in RX 
    DSM_Context.EditLine = nil   -- Exit Edit Mode (By clearing the line editing)
end

function MenuLib.GotoMenu(menuId, lastSelectedLine)
    if (DEBUG_ON) then Log.LOG_write("%3.3f %s: DSM_GotoMenu(0x%X,LastSelectedLine=%d)\n", 
        MenuLib.getElapsedTime(), MenuLib.phase2String(DSM_Context.Phase), menuId, lastSelectedLine) end

    DSM_Context.Menu.MenuId = menuId
    DSM_Context.SelLine = lastSelectedLine
    -- Request to load the menu Again
    MenuLib.ChangePhase(PHASE.MENU_TITLE)
end

function MenuLib.MoveSelectionLine(dir)
    local ctx = DSM_Context
    local menu = ctx.Menu
    local menuLines = ctx.MenuLines

    if (dir == 1) then -- NEXT
        if ctx.SelLine <= MAX_MENU_LINES then
            local num = ctx.SelLine
            for i = ctx.SelLine + 1, MAX_MENU_LINES, 1 do
                if MenuLib.isSelectableLine(menuLines[i]) then
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
                if MenuLib.isSelectableLine(menuLines[i]) then
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


-- Clear each line of the menu 
function MenuLib.clearMenuLines() 
    local ctx = DSM_Context
    for i = 0, MAX_MENU_LINES do -- clear menu
        ctx.MenuLines[i] = { MenuId = 0, lineNum = 0, Type = 0, Text = "", TextId = 0, ValId = 0, Min=0, Max=0, Def=0, TextStart=0, Val=nil }
    end
end

-- Post processing needed for each menu 
function MenuLib.PostProcessMenu()
    local ctx = DSM_Context

    if (ctx.Menu.Text==nil) then
        ctx.Menu.Text = MenuLib.Get_Text(ctx.Menu.TextId)
        MenuLib.MenuPostProcessing (ctx.Menu)
    end

    --if (DEBUG_ON) then Log.LOG_write("SIM RESPONSE Menu: %s\n", MenuLib.menu2String(ctx.Menu)) end

    for i = 0, MenuLib.MAX_MENU_LINES do -- clear menu
        local line = ctx.MenuLines[i]
        if (line.Type~=0) then
            line.MenuId = ctx.Menu.MenuId
            line.lineNum = i
            MenuLib.MenuLinePostProcessing(line) -- Do the same post processing as if they come from the RX
            --if (DEBUG_ON) then Log.LOG_write("SIM RESPONSE MenuLine: %s\n", MenuLib.menuLine2String(line))  end
        end

    end
end

function MenuLib.GetFlightModeValue(line)
    local ret = line.Text.." "
    local val = line.Val

    if (val==nil) then return ret.."--" end

    -- Adjust the displayed value for Flight mode line as needed
    if (DSM_Context.RX.Id == RX.FC6250HX) then
        -- Helicopter Flights modes 
        if (val==0)     then ret = ret .. "1 / HOLD" 
        elseif (val==1) then ret = ret .. "2 / Normal" 
        elseif (val==2) then ret = ret .. "3 / Stunt 1" 
        elseif (val==3) then ret = ret .. "4 / Stunt 2" 
        elseif (val==4) then ret = ret .. "5 / Panic" 
        else
            ret = ret .. " " .. (val + 1)
        end
    else
        -- No adjustment needed
        if (val==190) then
            ret=ret.."Err:Out of Range"
        else
            ret=ret..(val + 1)
        end
        
    end    
    return ret
end

function MenuLib.Init()
    print("MenuLib.Init()")
    -- Phase Names
    PhaseText[PHASE.INIT]                = "INIT"
    PhaseText[PHASE.RX_VERSION]          = "RX_VERSION"
    PhaseText[PHASE.WAIT_CMD]            = "WAIT_CMD"
    PhaseText[PHASE.MENU_TITLE]          = "MENU_TITLE"
    PhaseText[PHASE.MENU_REQ_TX_INFO]    = "MENU_REQ_TX_INFO"
    PhaseText[PHASE.MENU_LINES]          = "MENU_LINES"
    PhaseText[PHASE.MENU_VALUES]         = "MENU_VALUES"
    PhaseText[PHASE.VALUE_CHANGING]      = "VALUE_CHANGING"
    PhaseText[PHASE.VALUE_CHANGING_WAIT] = "VALUE_EDITING"
    PhaseText[PHASE.VALUE_CHANGE_END]    = "VALUE_CHANGE_END"
    PhaseText[PHASE.EXIT]                = "EXIT"
    PhaseText[PHASE.EXIT_DONE]           = "EXIT_DONE"


    -- Line Types
    LineTypeText[LINE_TYPE.MENU]            = "M"
    LineTypeText[LINE_TYPE.LIST_MENU_NC]    = "LM_nc"
    LineTypeText[LINE_TYPE.LIST_MENU]       = "LM"
    LineTypeText[LINE_TYPE.LIST_MENU_TOG]   = "LM_tog"
    LineTypeText[LINE_TYPE.LIST_MENU_NC2]   = "LM_nc2"
    LineTypeText[LINE_TYPE.LIST_MENU_ORI]   = "LM_ori"
    LineTypeText[LINE_TYPE.VALUE_NUM_I8_NC] = "V_nc"
    LineTypeText[LINE_TYPE.VALUE_PERCENT]   = "V_%"
    LineTypeText[LINE_TYPE.VALUE_DEGREES]   = "V_de"
    LineTypeText[LINE_TYPE.VALUE_NUM_I8]    = "V_i8"
    LineTypeText[LINE_TYPE.VALUE_NUM_I16]   = "V_i16"
    LineTypeText[LINE_TYPE.VALUE_NUM_SI16]  = "V_s16"
    LineTypeText[LINE_TYPE.LT_EMPTY]        = "Z" 

    DSM_Context.Phase = PHASE.RX_VERSION
end

function MenuLib.clearAllText()
    local function clearTable(t)
        for i, v in ipairs(t) do t[i] = nil end
    end

    clearTable(Text)
    clearTable(List_Text)
    clearTable(List_Text_Img)
    clearTable(List_Values)
end 

function MenuLib.LoadTextFromFile(fileName, mem)
    local function rtrim(s)
      local n = string.len(s)
      while n > 0 and string.find(s, "^%s", n) do n = n - 1 end
      return string.sub(s, 1, n)
    end
  
    --print(string.format("Loading messages from [%s]",fileName))
    local dataFile = io.open(fileName, "r")   -- read File
    -- cannot read file???
    assert(dataFile, "Cannot load Message file:" .. fileName)
  
    local data = io.read(dataFile, mem * 1024) -- read up to 10k characters (newline char also counts!)
    io.close(dataFile)
  
    collectgarbage("collect")
  
    local lineNo = 0
    for line in string.gmatch(data, "[^\r\n]+") do
      lineNo = lineNo + 1
      --print(string.format("Line [%d]: %s",lineNo,line))
  
      -- Remove Comments
      local s = string.find(line, "--", 1, true)
      if (s ~= nil) then
        line = string.sub(line, 1, s - 1)
      end
  
      line = rtrim(line)
  
      if (string.len(line) > 0) then
        local a, b, c = string.match(line, "%s*(%a*)%s*|%s*(%w*)%s*|(.*)%s*")
        --print(string.format("[%s] [%s] [%s]",a,b,c))
        if (a ~= nil) then
          local index = tonumber(b)
  
          if (index == nil) then
            assert(false, string.format("%s:%d: Invalid Hex num [%s]", fileName, lineNo, b))
          elseif (a == "T") then
            Text[index] = c
          elseif (a == "LT") then
            List_Text[index] = c
          elseif (a == "LI") then
            List_Text_Img[index] = c
          elseif (a == "FM") then
            Flight_Mode[0] = c
          elseif (a == "RX") then
            RxName[index] = c
          else
            assert(false, string.format("%s:%d: Invalid Line Type [%s]", fileName, lineNo, a))
          end
        end
      end
      if (lineNo % 50 == 0) then
        collectgarbage("collect")
      end
    end -- For
  
    --print(string.format("Loaded [%d] messages",lineNo))
    data = nil
end

function MenuLib.INC_LoadTextFromFile(fileName, FileState)
    -----------------------
    local function rtrim(s)
        local n = string.len(s)
        while n > 0 and string.find(s, "^%s", n) do n = n - 1 end
        return string.sub(s, 1, n)
    end
  
  local function GetTextInfoFromFile(pos)
    local dataFile = io.open(fileName, "r")   
    io.seek(dataFile,pos)
    local buff = io.read(dataFile, 100)
    io.close(dataFile)
  
    local line=""
    local index=""
    local type=""
  
    local pipe=0
    local comment=0
    local newPos = pos
  
    -- Parse the line: 
    -- Format:  TT|0x999|Text -- Comment
    for i=1,#buff do
      newPos=newPos+1
      local ch = string.sub(buff,i,i)
  
      if (pipe < 2 and ch=="|") then pipe=pipe+1 -- Count pipes pos  (Type | Index | .....)
      elseif (ch=="\r") then -- Ignore CR
      elseif (ch=="\n") then break -- LF, end of line
      elseif (ch=="-") then  -- March comments
        comment=comment+1
        if (comment==2) then pipe=6 end -- Comment part of line
      else
        -- regular char
        comment=0
        if (pipe==0) then type=type..ch  -- in TT (Type)
        elseif (pipe==1) then index=index..ch  -- in Index
        elseif (pipe<6) then line=line..ch end -- in Text
      end -- Regular char 
    end -- For
  
    return type, index, rtrim(line), newPos 
  end

  -----------------------------------------------------------

    if (FileState.state==nil) then -- Initial State
      FileState.state=1
      FileState.lineNo=0
      FileState.filePos=0
    end
  
    if FileState.state==1 then
      for l=1,10 do -- do 10 lines at a time 
        local type, sIndex, text
        local lineStart = FileState.filePos
  
        type, sIndex, text, FileState.filePos = GetTextInfoFromFile(FileState.filePos)
  
        --print(string.format("T=%s, I=%s, T=%s LS=%d, FP=%d",type,sIndex,text,lineStart, FileState.filePos))
  
        if (lineStart==FileState.filePos) then -- EOF
            FileState.state=2 --EOF State 
            return 1
        end
        FileState.lineNo = FileState.lineNo + 1
  
        type = rtrim(type)
  
        if (string.len(type) > 0 and string.len(sIndex) > 0) then
            local index = tonumber(sIndex)
  
            if (index == nil) then
              assert(false, string.format("%s:%d: Invalid Hex num [%s]", fileName, FileState.lineNo, sIndex))
            elseif (type == "T") then
              Text[index] =  text
            elseif (type == "LT") then
              List_Text[index] = text
            elseif (type == "LI") then
              List_Text_Img[index] = text
            elseif (type == "FM") then
              Flight_Mode[0] = text
            elseif (type == "RX") then
              RxName[index] = text
            else
              assert(false, string.format("%s:%d: Invalid Line Type [%s]", fileName, FileState.lineNo, type))
            end
        end
      end -- for 
    end -- if
    
    return 0
  end


-- Export some Constants and Variables
MenuLib.PHASE = PHASE
MenuLib.LINE_TYPE = LINE_TYPE
MenuLib.DISP_ATTR = DISP_ATTR
MenuLib.RX = RX

MenuLib.MAX_MENU_LINES = MAX_MENU_LINES
MenuLib.BACK_BUTTON    = BACK_BUTTON
MenuLib.NEXT_BUTTON    = NEXT_BUTTON
MenuLib.PREV_BUTTON    = PREV_BUTTON

MenuLib.DSM_Context = DSM_Context

MenuLib.Text = Text
MenuLib.List_Text = List_Text
MenuLib.List_Text_Img = List_Text_Img
MenuLib.List_Values = List_Values

MenuLib.LOG_open   = Log.LOG_open
MenuLib.LOG_write  = Log.LOG_write
MenuLib.LOG_Close  = Log.LOG_close


return MenuLib