local toolName = "TNS|DSM Forward Prog v0.5 (OTX B&W) |TNE"

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

local SIMULATION_ON = true  -- FALSE: use real communication to DSM RX (DEFAULT), TRUE: use a simulated version of RX 
local DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm.log)
local DEBUG_ON_LCD = false   -- Interactive Information on LCD of Menu data from RX 


local dsmLib
if (SIMULATION_ON) then
  -- library with SIMILATION VERSION.  Works really well in Companion for GUI development
  dsmLib = loadScript("/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgSIMLib.lua")(DEBUG_ON)
else
  dsmLib = loadScript("/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lua")(DEBUG_ON)
end

local PHASE = dsmLib.PHASE
local LINE_TYPE = dsmLib.LINE_TYPE
local DISP_ATTR = dsmLib.DISP_ATTR

local DSM_Context = dsmLib.DSM_Context

local LCD_X_LINE_MENU       = 10
local LCD_X_LINE_TITLE      = 10
local LCD_X_LINE_VALUE      = 230
local LCD_X_LINE_DEBUG      = 390

local LCD_Y_MENU_TITLE       = 20
local LCD_Y_LINE_START       = LCD_Y_MENU_TITLE + 30
local LCD_Y_LINE_HEIGHT      = (DEBUG_ON_LCD and 23)  or 27   -- if DEBUG 23 else 27 

local LCD_Y_LOWER_BUTTONS    = LCD_Y_LINE_START + 7 * LCD_Y_LINE_HEIGHT

local lastRefresh=0         -- Last time the screen was refreshed
local REFRESH_GUI_MS = 500/10   -- 500ms.. Screen Refresh Rate.. to not use unneded CPU time  (in 10ms units to be compatible with getTime())
local originalValue = nil

------------------------------------------------------------------------------------------------------------
local function GUI_SwitchSimulationOFF()
  dsmLib.ReleaseConnection()  
  dsmLib.LOG_close()

  SIMULATION_ON = false
  dsmLib = loadScript("/SCRIPTS/TOOLS/DSMLIB/DsmFwPrgLib.lua")(DEBUG_ON)
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
  lcd.drawText(x+5,y+2, text, attr)
  lcd.drawRectangle(x, y, w, h, LIGHTGREY)
end

local function GUI_Display_Menu(menu)
  local ctx = DSM_Context
  local w=  LCD_W-100  -- usable Width for the Menu/Lines

  -- Center Header
  local tw = openTx_lcd_sizeText(menu.Text) 
  local x = w/2 - tw/2  -- Center of Screen - Center of Text
  lcd.drawText(x,LCD_Y_MENU_TITLE,menu.Text,BOLD)  -- orig MIDSIZE

  -- Back
  if menu.BackId ~= 0 then
    GUI_Diplay_Button(437-5,LCD_Y_MENU_TITLE,47,25,"Back",ctx.SelLine == dsmLib.BACK_BUTTON)
  end
  -- Next ?
  if menu.NextId ~= 0 then
    GUI_Diplay_Button(437-5,LCD_Y_LOWER_BUTTONS,47,25,"Next",ctx.SelLine == dsmLib.NEXT_BUTTON)
  end
  -- Prev?
  if menu.PrevId ~= 0 then
    GUI_Diplay_Button(0,LCD_Y_LOWER_BUTTONS,47,25,"Prev",ctx.SelLine == dsmLib.PREV_BUTTON)
  end

  -- Debug into LCD 
  if (DEBUG_ON_LCD) then lcd.drawText(0,LCD_Y_MENU_TITLE,dsmLib.phase2String(ctx.Phase),SMLSIZE + BLUE) end  -- Phase we are in 
  if (DEBUG_ON_LCD) then lcd.drawText(LCD_X_LINE_MENU,240,dsmLib.menu2String(menu),SMLSIZE + BLUE) end  -- Menu Info
end

local function GUI_Display_Line_Menu(x,y,w,h,line,selected)
  local attr = (selected) and INVERS or 0    -- INVERS if line Selected
  local bold = 0
  local text = line.Text

  if dsmLib.isSelectableLine(line) then  
      -- Menu Line
      text = text .. "  -->"  --OPENTX
  else  -- SubHeaders and plain text lines
      bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.BOLD) and BOLD) or 0  
      if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.RIGHT) then -- Right Align???
          local tw = openTx_lcd_sizeText(line.Text)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.CENTER) then -- Center??
          local tw = openTx_lcd_sizeText(line.Text) 
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_MENU)/2 - tw/2  -- Center - 1/2 Text
      end
  end

  lcd.drawText(x,y, text, attr + bold)

end
------------------------------------------------------------------------------------------------------------
local function GUI_Display_Line_Value(lineNum, line, value, selected, editing)
  local bold      = 0

  local y = LCD_Y_LINE_START+(LCD_Y_LINE_HEIGHT*lineNum)
  local x = LCD_X_LINE_TITLE

  ---------- NAME Part 
  local header = line.Text
  -- ONLY do this for Flight Mode (Right Align or Centered)
  if (dsmLib.isFlightModeText(line.TextId)) then
      -- Display Header + Value together
      header = header .. " " .. value

      -- Flight mode display attributes
      bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.BOLD) and BOLD) or 0

      if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.RIGHT) then -- Right Align
          local tw = openTx_lcd_sizeText(header)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.CENTER) then -- Centered
          local tw = openTx_lcd_sizeText(header)
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_TITLE)/2 - tw/2  -- Center - 1/2 Text
      end
  else
    -- No Flight Mode, no effects here
    header = header .. ":"
  end

  lcd.drawText(x, y, header, bold) -- display Line Header

  --------- VALUE PART,  Skip for Flight Mode since already show the value 
  if not dsmLib.isFlightModeText(line.TextId) then 
    local attrib    = 0
    value = value .. (line.Format or "")  -- Append % if needed

    if selected then
      attrib = INVERS
      if editing then -- blink editing entry
        attrib = attrib + BLINK
        value = "[ " .. value .. " ]"
      end
    end
    
    lcd.drawText(LCD_X_LINE_VALUE,y, value, attrib) -- display value
  end

  if (DEBUG_ON_LCD) then  lcd.drawText(LCD_X_LINE_DEBUG,y, line.MinMaxDebug or "", SMLSIZE) end -- display debug
end
------------------------------------------------------------------------------------------------------------
local function GUI_Display()
  local ctx = DSM_Context
  lcd.clear()
 
  if LCD_W == 480 then
    local header = "DSM Fwrd Programming      "
    if ctx.Phase ~= PHASE.RX_VERSION then
      header = header .. "RX "..ctx.RX.Name.." v"..ctx.RX.Version
    end

    --Draw title
    lcd.drawFilledRectangle(0, 0, LCD_W, 20, TITLE_BGCOLOR)
    lcd.drawText(5, 0, header, MENU_TITLE_COLOR)
    --Draw RX Menu
    if ctx.Phase == PHASE.RX_VERSION then
      lcd.drawText(LCD_X_LINE_TITLE,100,"No compatible DSM RX...", BLINK)
    else
      local menu = ctx.Menu
      if menu.Text ~=  nil then

        GUI_Display_Menu(menu)

        for i = 0, dsmLib.MAX_MENU_LINES do
          local line = ctx.MenuLines[i]

          if i == ctx.SelLine then
            -- DEBUG: Display Selected Line info for ON SCREEN Debugging
            if (DEBUG_ON_LCD) then lcd.drawText(LCD_X_LINE_TITLE,255,dsmLib.menuLine2String(line),SMLSIZE+BLUE) end
          end

          if line ~= nil and line.Type ~= 0 then
            if line.Type == LINE_TYPE.MENU then 
              -- Menu Line 
              GUI_Display_Line_Menu(LCD_X_LINE_MENU,LCD_Y_LINE_START+(LCD_Y_LINE_HEIGHT*i), 350, LCD_Y_LINE_HEIGHT, line, i == ctx.SelLine)
            else  
              -- list/value line 
              local value = line.Val
              if line.Val ~= nil then
                if dsmLib.isListLine(line) then    -- for Lists of Strings, get the text
                  value = dsmLib.Get_Text(line.Val + line.TextStart) -- TextStart is the initial offset for text
                  local imgValue = dsmLib.Get_Text_Img(line.Val + line.TextStart)   -- Complentary IMAGE for this value to Display??
                    
                  if (imgValue) then  -- Optional Image for a Value
                    --TODO: Pending feature.. create images and put bitmap instead of a message
                    --Display the image/Alternate Text 
                    lcd.drawText(LCD_X_LINE_TITLE, LCD_Y_LINE_START+LCD_Y_LINE_HEIGHT, "Img:"..imgValue)
                  end
                end

                GUI_Display_Line_Value(i, line, value, i == ctx.SelLine, i == ctx.EditLine)
              end
            end -- if ~MENU
          end -- if Line[i]~=nil
        end  -- for
      end 
    end
  else
    -- Different Resolution
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
        dsmLib.ChangePhase(PHASE.VALUE_CHANGE_END)  -- Update+Validate value in RX 
        ctx.EditLine = nil   -- Exit Edit Mode (By clearing the line editing)
      else
        dsmLib.ChangePhase(PHASE.EXIT)
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
      dsmLib.GotoMenu(menu.BackId)
    elseif ctx.SelLine == dsmLib.NEXT_BUTTON then -- Next
      dsmLib.GotoMenu(menu.NextId)
    elseif ctx.SelLine == dsmLib.PREV_BUTTON then -- Prev
      dsmLib.GotoMenu(menu.PrevId)
    elseif menuLines[ctx.SelLine].ValId ~= 0 then  
      if menuLines[ctx.SelLine].Type == LINE_TYPE.MENU then -- Next menu exist
        if (SIMULATION_ON and menuLines[ctx.SelLine].ValId==0xFFFF) then 
          -- SPECIAL Simulation menu to Exit Simulation and
          -- comunicate with Real RX 
          GUI_SwitchSimulationOFF()
        else
          dsmLib.GotoMenu(menuLines[ctx.SelLine].ValId)  -- ValId is the MenuId to navigate to
        end
      else
        -- Editing a Line???? 
        if ctx.isEditing() then
          -- Change the Value and exit edit 
          ctx.EditLine = nil
          dsmLib.ChangePhase(PHASE.VALUE_CHANGE_END)
        else
          -- enter Edit the current line  
          ctx.EditLine = ctx.SelLine
          originalValue = menuLines[ctx.SelLine].Val
        end
      end
    end
  end
end

------------------------------------------------------------------------------------------------------------
-- Init
local function DSM_Init()
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

  GUI_HandleEvent(event)

  dsmLib.Send_Receive()  -- Handle Send and Receive DSM Forward Programming Messages

  local refreshInterval =  REFRESH_GUI_MS
  -- When using LCD BLINK attribute, we need faster refresh for BLINK to SHOW on LCD 
  if (ctx.EditLine or (ctx.Phase == PHASE.RX_VERSION)) then  -- Editing or Requesting RX Version?
    ctx.Refresh_Display=true  
    refreshInterval = 20 -- 200ms
  end

  -- Refresh display only if needed and no faster than 500ms, utilize more CPU to speedup DSM communications
  if (ctx.Refresh_Display and (getTime()-lastRefresh) > refreshInterval) then --300ms from last refresh 
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