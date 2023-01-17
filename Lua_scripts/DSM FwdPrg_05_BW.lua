local toolName = "TNS|DSM Forward Prog v0.53 (Text B&W) |TNE"
local VERSION  = "v0.53"


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
------------------------------------------------------------------------------

local SIMULATION_ON = false  -- FALSE: use real communication to DSM RX (DEFAULT), TRUE: use a simulated version of RX 
local DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm.log)
local DEBUG_ON_LCD = false   -- Interactive Information on LCD of Menu data from RX 

local DSMLIB_PATH = "/SCRIPTS/TOOLS/DSMLIB/"

local dsmLib = assert(loadScript(DSMLIB_PATH.."DsmSetupLib.lua"), "Not-Found: DSMLIB/DsmSetupLib.lua")(DEBUG_ON,SIMULATION_ON)

local PHASE = dsmLib.PHASE
local LINE_TYPE = dsmLib.LINE_TYPE
local DISP_ATTR = dsmLib.DISP_ATTR

local DSM_Context = dsmLib.DSM_Context

local IS_EDGETX   = false     -- DEFAULT until Init changed it

local LCD_W_USABLE          = LCD_W-10
-- X for Menu Lines
local LCD_X_LINE_MENU       = 10  
-- X offsets for (Title: [Value] debugInfo) lines
local LCD_X_LINE_TITLE      = 10
local LCD_X_LINE_VALUE      = 230
local LCD_X_LINE_DEBUG      = 390

-- Line Height: make it smaller debugging info tp LCD (some space buttom) 
local LCD_Y_LINE_HEIGHT      = (DEBUG_ON_LCD and 23)  or 27   -- if DEBUG 23 else 27
-- Y offsets
local LCD_Y_MENU_TITLE       = 20
-- Y offet
local LCD_Y_LINE_FIRST       = LCD_Y_MENU_TITLE + 30
local LCD_Y_LOWER_BUTTONS    = LCD_Y_LINE_FIRST + 7 * LCD_Y_LINE_HEIGHT

local LCD_W_BUTTONS          = 47
local LCD_H_BUTTONS          = 25
local LCD_X_RIGHT_BUTTONS    = LCD_W - LCD_W_BUTTONS - 5

local TEXT_SIZE             = 0 -- NORMAL

local lastRefresh=0         -- Last time the screen was refreshed
local REFRESH_GUI_MS = 500/10   -- 500ms.. Screen Refresh Rate.. to not use unneded CPU time  (in 10ms units to be compatible with getTime())
local originalValue = nil

local warningScreenON = true

------------------------------------------------------------------------------------------------------------
local function GUI_SwitchToRX()
  -- Force to refresh DSM Info in MODEL (dsmLib pointing to the setup Script)
  local dsmChannelInfo, description = dsmLib.CreateDSMPortChannelInfo()
  
  dsmLib.ReleaseConnection()  
  dsmLib.LOG_close()

  SIMULATION_ON = false
  dsmLib = assert(loadScript(DSMLIB_PATH.."DsmFwPrgLib.lua"),"Not-Found: DSMLIB/DsmFwPrgLib.lua")(DEBUG_ON)
  DSM_Context = dsmLib.DSM_Context

  dsmLib.Init(toolName)  -- Initialize Library 
  dsmLib.SetDSMChannelInfo(dsmChannelInfo, description)  -- send the dsmChannelInfo to new instance library
  dsmLib.StartConnection()
  DSM_Context.Refresh_Display = true
end

local function GUI_SwitchToSIM()
  dsmLib.ReleaseConnection()  
  dsmLib.LOG_close()

  SIMULATION_ON = true
  dsmLib = assert(loadScript(DSMLIB_PATH.."DsmFwPrgSIMLib.lua"), "Not-Found: DSMLIB/DsmFwPrgSIMLib.lua")(DEBUG_ON)
  DSM_Context = dsmLib.DSM_Context

  dsmLib.Init(toolName)  -- Initialize Library 
  dsmLib.StartConnection()
  DSM_Context.Refresh_Display = true
end

local function openTx_lcd_sizeText(s)
  return string.len(s)*5
end

local function GUI_Diplay_Button(x,y,w,h,text,selected)
  local attr = (selected) and INVERS or 0    -- INVERS if line Selected
  if (TEXT_SIZE~=SMLSIZE) then
    lcd.drawText(x+5,y+2, text, attr + TEXT_SIZE)
    lcd.drawRectangle(x, y, w, h, LINE_COLOR)
  else -- SMALL Screen
    lcd.drawText(x,y, text, attr + TEXT_SIZE)
  end
end

local function GUI_Display_Menu(menu)
  local ctx = DSM_Context
  local w=  LCD_W_USABLE - LCD_W_BUTTONS - 10  -- usable Width for the Menu/Lines

  -- Center Header
  local tw = openTx_lcd_sizeText(menu.Text) 
  local x = w/2 - tw/2  -- Center of Screen - Center of Text
  if (x < 0) then x=0 end -- in case text is too wide

  local bold = BOLD
  lcd.drawText(x,LCD_Y_MENU_TITLE,menu.Text,bold + TEXT_SIZE)

  -- Back
  if menu.BackId ~= 0 then
    GUI_Diplay_Button(LCD_X_RIGHT_BUTTONS,LCD_Y_MENU_TITLE,LCD_W_BUTTONS,LCD_H_BUTTONS,"Back",ctx.SelLine == dsmLib.BACK_BUTTON)
  end
  -- Next ?
  if menu.NextId ~= 0 then
    GUI_Diplay_Button(LCD_X_RIGHT_BUTTONS,LCD_Y_LOWER_BUTTONS,LCD_W_BUTTONS,LCD_H_BUTTONS,"Next",ctx.SelLine == dsmLib.NEXT_BUTTON)
  end
  -- Prev?
  if menu.PrevId ~= 0 then
    GUI_Diplay_Button(0,LCD_Y_LOWER_BUTTONS,LCD_W_BUTTONS,LCD_H_BUTTONS,"Prev",ctx.SelLine == dsmLib.PREV_BUTTON)
  end

  -- Debug into LCD 
  if (DEBUG_ON_LCD) then lcd.drawText(0,LCD_Y_MENU_TITLE,dsmLib.phase2String(ctx.Phase),TEXT_SIZE + WARNING_COLOR) end  -- Phase we are in 
  if (DEBUG_ON_LCD) then lcd.drawText(LCD_X_LINE_MENU,240,dsmLib.menu2String(menu),TEXT_SIZE + WARNING_COLOR) end  -- Menu Info
end

local function GUI_Display_Line_Menu(x,y,w,h,line,selected)
  local attr = (selected and INVERS) or 0    -- INVERS if line Selected
  local bold = 0
  local text = line.Text

  if dsmLib.isSelectableLine(line) then  
      -- Menu Line
      text = text .. "  >"  
  else  -- SubHeaders and plain text lines
      bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._BOLD) and BOLD) or 0  
    
      if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._RIGHT) then -- Right Align???
          local tw = openTx_lcd_sizeText(line.Text)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._CENTER) then -- Center??
          local tw = openTx_lcd_sizeText(line.Text) 
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_MENU)/2 - tw/2  -- Center - 1/2 Text
      end
      if (x < 0) then x=0 end -- in case text is too wide
  end

  lcd.drawText(x,y, text, attr + bold + TEXT_SIZE)

end
------------------------------------------------------------------------------------------------------------
local function GUI_Display_Line_Value(lineNum, line, value, selected, editing)
  local bold      = 0

  local y = LCD_Y_LINE_FIRST+(LCD_Y_LINE_HEIGHT*lineNum)
  local x = LCD_X_LINE_TITLE

  ---------- NAME Part 
  local header = line.Text
  -- ONLY do this for Flight Mode (Right Align or Centered)
  if (dsmLib.isFlightModeLine(line)) then
       -- Display Header + Value together
       header = dsmLib.GetFlightModeValue(line)

      -- Flight mode display attributes
      bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._BOLD) and BOLD) or 0

      if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._RIGHT) then -- Right Align
          local tw = openTx_lcd_sizeText(header)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR._CENTER) then -- Centered
          local tw = openTx_lcd_sizeText(header)
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_TITLE)/2 - tw/2  -- Center - 1/2 Text
      end
      if (x < 0) then x=0 end -- in case text is too wide
  else
    -- No Flight Mode, no effects here
    header = header .. ":"
  end

  lcd.drawText(x, y, header, bold + TEXT_SIZE) -- display Line Header

  --------- VALUE PART,  Skip for Flight Mode since already show the value 
  if not dsmLib.isFlightModeLine(line) then 
    local attrib    = 0

    if selected then
      attrib = INVERS
      if editing then -- blink editing entry
        attrib = attrib + BLINK
        value = "[" .. value .. "]"
      end
    end
    
    value = value .. "  " .. (line.Format or "")  -- Append % if needed
    lcd.drawText(LCD_X_LINE_VALUE,y, value, attrib + TEXT_SIZE) -- display value
  end

  if (DEBUG_ON_LCD) then  lcd.drawText(LCD_X_LINE_DEBUG,y, line.MinMaxDebug or "", TEXT_SIZE + WARNING_COLOR) end -- display debug
end

------------------------------------------------------------------------------------------------------------
local function GUI_ShowBitmap(x,y,imgData)
  -- imgData format "bitmap.png|alt message"
  local f = string.gmatch(imgData, '([^%|]+)') -- Iterator over values split by '|'
  local imgName, imgMsg = f(), f()

  if (LCD_W > 128) then
    lcd.drawText(x, y, imgMsg or "", TEXT_SIZE)  -- Alternate Image MSG 
  else
    local f = string.gmatch(imgMsg, '([^%:]+)') -- Iterator over values split by ':'
    local msg1,msg2 = f(), f()
    lcd.drawText(x, y, (msg1 or "")..":", TEXT_SIZE)  -- Alternate Image MSG 
    lcd.drawText(x, y+10, msg2 or "", TEXT_SIZE)  -- Alternate Image MSG 
  end

  -- NO IMAGES in Text B&W 
  --local imgPath = IMAGE_PATH .. (imgName or "")
  --local bitmap  = Bitmap.open(imgPath)
  --if (bitmap~=nil) then
  --   lcd.drawBitmap(bitmap, x,y+20)
  --end
end

------------------------------------------------------------------------------------------------------------
local function GUI_Display()
  local ctx = DSM_Context
  lcd.clear()
  local header = "DSM Fwrd Programming      "

  if (TEXT_SIZE==SMLSIZE) then -- Small Screen no title
    header = ""
  end
 
    if ctx.Phase ~= PHASE.RX_VERSION then
      header = header .. ctx.RX.Name.." v"..ctx.RX.Version
    end

    --Draw title
    if (TEXT_SIZE~=SMLSIZE) then -- ignore tool title small size screens
        lcd.drawFilledRectangle(0, 0, LCD_W, 20, TITLE_BGCOLOR)
        lcd.drawText(5, 0, header, MENU_TITLE_COLOR  + TEXT_SIZE)
    else -- Small Screen
        lcd.drawText(20, LCD_Y_LOWER_BUTTONS+1, header, TEXT_SIZE)
    end
    --Draw RX Menu
    if ctx.Phase == PHASE.RX_VERSION then
      if (ctx.isReset) then
        lcd.drawText(LCD_X_LINE_TITLE,50,"Waiting for RX to Restart", BLINK + TEXT_SIZE)
      else
        lcd.drawText(LCD_X_LINE_TITLE,50,"No compatible DSM RX...", BLINK + TEXT_SIZE)
      end
    else
      local menu = ctx.Menu
      if menu.Text ~=  nil then

        GUI_Display_Menu(menu)

        for i = 0, dsmLib.MAX_MENU_LINES do
          local line = ctx.MenuLines[i]

          if i == ctx.SelLine then
            -- DEBUG: Display Selected Line info for ON SCREEN Debugging
            if (DEBUG_ON_LCD) then lcd.drawText(LCD_X_LINE_TITLE,255,dsmLib.menuLine2String(line),TEXT_SIZE + WARNING_COLOR) end
          end

          if line ~= nil and line.Type ~= 0 then
            if line.Type == LINE_TYPE.MENU then 
              -- Menu Line 
              GUI_Display_Line_Menu(LCD_X_LINE_MENU,LCD_Y_LINE_FIRST+(LCD_Y_LINE_HEIGHT*i), 350, LCD_Y_LINE_HEIGHT, line, i == ctx.SelLine)
            else  
              -- list/value line 
              local value = line.Val
              if line.Val ~= nil then
                if dsmLib.isListLine(line) then    -- for Lists of Strings, get the text
                  value = dsmLib.Get_List_Text(line.Val + line.TextStart) -- TextStart is the initial offset for text
                  local imgData = dsmLib.Get_List_Text_Img(line.Val + line.TextStart)   -- Complentary IMAGE for this value to Display??
                    
                  if (imgData and i == ctx.SelLine) then  -- Optional Image and Msg for selected value
                    GUI_ShowBitmap(LCD_X_LINE_TITLE,LCD_Y_LINE_FIRST+LCD_Y_LINE_HEIGHT, imgData)
                  end
                end

                GUI_Display_Line_Value(i, line, value, i == ctx.SelLine, i == ctx.EditLine)
              end
            end -- if ~MENU
          end -- if Line[i]~=nil
        end  -- for
      end 
    end
  
end

-------------------------------------------------------------------------------------------------------------
local function GUI_RotEncVal(dir) -- return encoder speed to inc or dec values
  local inc = 0
  local Speed = getRotEncSpeed()

  if Speed == ROTENC_MIDSPEED then
    inc = (5 * dir)
  elseif Speed == ROTENC_HIGHSPEED then
    inc = (15 * dir)
  else
    inc = dir
  end

  return inc
end

------------------------------------------------------------------------------------------------------------
local function GUI_HandleEvent(event, touchState)
  local ctx = DSM_Context
  local menu = ctx.Menu
  local menuLines = ctx.MenuLines

  if event == EVT_VIRTUAL_EXIT then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_EXIT\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.Phase == PHASE.RX_VERSION then
      dsmLib.ReleaseConnection()
    else
      if ctx.isEditing() then  -- Editing a Line, need to  restore original value
        ctx.MenuLines[ctx.EditLine].Val = originalValue        
        dsmLib.Value_Write_Validate(menuLines[ctx.EditLine])
      else
        dsmLib.ChangePhase(PHASE.EXIT) -- Exit
      end
    end
    return
  end
  
  if event == EVT_VIRTUAL_NEXT then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_NEXT\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then  -- Editing a Line, need to inc the value
      local line=ctx.MenuLines[ctx.EditLine]
      dsmLib.Value_Add(line, GUI_RotEncVal(1))
    else  -- not editing, move selected line to NEXT
      dsmLib.MoveSelectionLine(1)
    end
    return
  end

  if event == EVT_VIRTUAL_PREV then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_PREV\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then  -- Editiing a line, need to dec the value
      local line=ctx.MenuLines[ctx.EditLine]
      dsmLib.Value_Add(line, GUI_RotEncVal(-1))
    else  -- not editing, move selected line to PREV
      dsmLib.MoveSelectionLine(-1)
    end
    return
  end

  if event == EVT_VIRTUAL_ENTER_LONG then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_ENTER_LONG\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then
    -- reset the value to default
      dsmLib.Value_Default( menuLines[ctx.EditLine])   -- Update RX value as needed
    end
    return
  end

  if event == EVT_VIRTUAL_ENTER then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_ENTER\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.SelLine == dsmLib.BACK_BUTTON then -- Back
      dsmLib.GotoMenu(menu.BackId,0)
    elseif ctx.SelLine == dsmLib.NEXT_BUTTON then -- Next
      dsmLib.GotoMenu(menu.NextId,0)
    elseif ctx.SelLine == dsmLib.PREV_BUTTON then -- Prev
      dsmLib.GotoMenu(menu.PrevId,0)
    elseif menuLines[ctx.SelLine].ValId ~= 0 then  
      if menuLines[ctx.SelLine].Type == LINE_TYPE.MENU then -- Next menu exist
        if (menuLines[ctx.SelLine].ValId==0xFFF1) then
          -- SPECIAL Simulation menu to Simulator 
          GUI_SwitchToSIM()
        elseif (menuLines[ctx.SelLine].ValId==0xFFF2) then
            -- SPECIAL Simulation menu to go to RX 
            GUI_SwitchToRX()
        else
          dsmLib.GotoMenu(menuLines[ctx.SelLine].ValId, ctx.SelLine)  -- ValId is the MenuId to navigate to
        end
      else
        -- Editing a Line???? 
        if ctx.isEditing() then
          -- Change the Value and exit edit 
          dsmLib.Value_Write_Validate(menuLines[ctx.SelLine])
        else
          -- enter Edit the current line  
          ctx.EditLine = ctx.SelLine
          originalValue = menuLines[ctx.SelLine].Val
          dsmLib.ChangePhase(PHASE.VALUE_CHANGING_WAIT)
        end
      end
    end
  end
end

local function init_screen_pos()
    -- osName in OpenTX is nil, otherwise is EDGETX 
    local ver, radio, maj, minor, rev, osname = getVersion()
    if (osname==nil) then osname = "OpenTX" end -- OTX 2.3.14 and below returns nil

    IS_EDGETX = string.sub(osname,1,1) =='E'

    if LCD_W == 480 then -- TX16
        -- use defaults in the script header
    elseif LCD_W == 128 then --TX12  (128x64) -- Still needs some work on the vertical
      DEBUG_ON_LCD = false -- no space for this
      TEXT_SIZE             = SMLSIZE
      LCD_W_USABLE          = 128

      LCD_W_BUTTONS          = 16
      LCD_H_BUTTONS          = 10
      LCD_X_RIGHT_BUTTONS    = 128 - LCD_W_BUTTONS - 3

      LCD_X_LINE_MENU       = 0  
      -- X offsets for (Title: [Value] debugInfo) lines
      LCD_X_LINE_TITLE      = 0
      LCD_X_LINE_VALUE      = 75
      LCD_X_LINE_DEBUG      = 110

      LCD_Y_LINE_HEIGHT      = 7
      LCD_Y_MENU_TITLE       = 0
      LCD_Y_LINE_FIRST       = LCD_Y_MENU_TITLE + 8
      LCD_Y_LOWER_BUTTONS    = LCD_Y_LINE_FIRST + (7 * LCD_Y_LINE_HEIGHT)
    end
end

local function GUI_Warning(event)
  lcd.clear()
  local header = "DSM Forward Programming "..VERSION.."                   "
  --Draw title
  if (LCD_W > 128) then
    lcd.drawFilledRectangle(0, 0, LCD_W, 17, TITLE_BGCOLOR)
    lcd.drawText(5, 0, header,  MENU_TITLE_COLOR  + TEXT_SIZE)

    lcd.drawText(100,20,"INFO", BOLD)
    lcd.drawText(5,40,"DSM Forward programing shares TX Servo/Output settings", TEXT_SIZE)
    lcd.drawText(5,60,"with the RX. Make sure you setup your plane first in ", TEXT_SIZE)
    lcd.drawText(5,80,"the TX before your start programming your RX.", TEXT_SIZE)
    lcd.drawText(5,100,"Wing & Tail type can be configured using this tool.", TEXT_SIZE)

    lcd.drawText(5,150,"TX Servo settings are sent to the RX during 'Initial Setup'", TEXT_SIZE)
    lcd.drawText(5,170,"as well as when using RX menu 'Relearn Servo Settings'", TEXT_SIZE)
    lcd.drawText(5,200,"ALWAYS TEST Gyro reactions after this conditions before flying.", BOLD+TEXT_SIZE)

    lcd.drawText(100,250,"    OK     ", INVERS + BOLD + TEXT_SIZE)
  else
    lcd.drawText(0,15,"Make sure you setup your plane", TEXT_SIZE)
    lcd.drawText(0,22,"first. Wing and Tail type.", TEXT_SIZE)

    lcd.drawText(0,30,"TX Servo settings are sent to ", TEXT_SIZE)
    lcd.drawText(0,37,"the RX during 'Initial Setup' and ", TEXT_SIZE)
    lcd.drawText(0,45,"ALWAYS TEST Gyro reactions", TEXT_SIZE)
    lcd.drawText(0,52,"before flying!!!", TEXT_SIZE)

    lcd.drawText(10,0,"    OK     ", INVERS + BOLD + TEXT_SIZE)
  end

  if event == EVT_VIRTUAL_EXIT or event == EVT_VIRTUAL_ENTER then
    warningScreenON = false
  end

  return 0
end

------------------------------------------------------------------------------------------------------------
-- Init
local function DSM_Init()
  init_screen_pos()
  dsmLib.Init(toolName)  -- Initialize Library 
  return dsmLib.StartConnection()
end


------------------------------------------------------------------------------------------------------------
-- Main


local function DSM_Run(event)
  local ctx = DSM_Context

  if event == nil then
    error("Cannot be run as a model script!")
    dsmLib.LOG_close()
    return 2
  end

  if (warningScreenON) then
    return GUI_Warning(event)
  end

  GUI_HandleEvent(event)

  dsmLib.Send_Receive()  -- Handle Send and Receive DSM Forward Programming Messages

  local refreshInterval =  REFRESH_GUI_MS
  -- When using LCD BLINK attribute, we need faster refresh for BLINK to SHOW on LCD 
  if (ctx.EditLine or (ctx.Phase == PHASE.RX_VERSION)) then  -- Editing or Requesting RX Version?
    ctx.Refresh_Display=true  
    refreshInterval = 20 -- 200ms
  end

  if (not IS_EDGETX) then -- OPENTX NEEDS REFRESH ON EVERY CYCLE
    GUI_Display()
  -- Refresh display only if needed and no faster than 500ms, utilize more CPU to speedup DSM communications
  elseif (ctx.Refresh_Display and (getTime()-lastRefresh) > refreshInterval) then --300ms from last refresh 
    GUI_Display()
    ctx.Refresh_Display=false
    lastRefresh=getTime()
  end
  
  if ctx.Phase == PHASE.EXIT_DONE then
    dsmLib.LOG_close()
    return 2
  else
    return 0
  end
end


return { init=DSM_Init, run=DSM_Run }