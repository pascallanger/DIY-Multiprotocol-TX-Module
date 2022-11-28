local toolName = "TNS|DSM Forward Prog v0.5 (Color+Touch) |TNE"
local VERSION  = "v0.5"

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

local SIMULATION_ON = false   -- FALSE: use real communication to DSM RX (DEFAULT), TRUE: use a simulated version of RX 
local DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm.log)
local DEBUG_ON_LCD = false   -- Interactive Information on LCD of Menu data from RX 
local USE_SPECKTRUM_COLORS = true -- true: Use spectrum colors, false: use theme colors (default on OpenTX) 
local DSMLIB_PATH = "/SCRIPTS/TOOLS/DSMLIB/"
local IMAGE_PATH = DSMLIB_PATH .. "img/"

local dsmLib
if (SIMULATION_ON) then
  -- library with SIMILATION VERSION.  Works really well in Companion for GUI development
  dsmLib = assert(loadScript(DSMLIB_PATH.."DsmFwPrgSIMLib.lua"), "Not-Found: DSMLIB/DsmFwPrgSIMLib.lua")(DEBUG_ON)
else
  dsmLib = assert(loadScript(DSMLIB_PATH.."DsmFwPrgLib.lua"),"Not-Found: DSMLIB/DsmFwPrgLib.lua")(DEBUG_ON)
end



local PHASE = dsmLib.PHASE
local LINE_TYPE = dsmLib.LINE_TYPE
local DISP_ATTR   = dsmLib.DISP_ATTR
local DSM_Context = dsmLib.DSM_Context


local lastRefresh=0         -- Last time the screen was refreshed
local REFRESH_GUI_MS = 300/10   -- 300ms.. Screen Refresh Rate.. to not waste CPU time  (in 10ms units to be compatible with getTime())
local originalValue = nil

local touchButtonArea           = {}
local EDIT_BUTTON = {  DEFAULT=1001, DEC_10=1002, DEC_1=1003, INC_1=1004, INC_10=5, OK=1006, ESC=1007 }

local IS_EDGETX   = false     -- DEFAULT until Init changed it

local LCD_Y_MENU_TITLE      = 20
local LCD_W_MENU_TITLE      = LCD_W-100

local LCD_X_LINE_MENU       = 30
local LCD_W_LINE_MENU       = 350

local LCD_X_LINE_TITLE      = 30
local LCD_X_LINE_VALUE      = 230
local LCD_X_LINE_DEBUG      = 390


local LCD_Y_LINE_START       = LCD_Y_MENU_TITLE + 30
local LCD_Y_LINE_HEIGHT      = (DEBUG_ON_LCD and 23)  or 27   -- if DEBUG 23 else 27 

local LCD_Y_LOWER_BUTTONS    = LCD_Y_LINE_START + 3 + (7 * LCD_Y_LINE_HEIGHT)


-- TOOL HEADER 
local LCD_TOOL_HDR_COLOR      = MENU_TITLE_COLOR
local LCD_TOOL_HDR_BGCOLOR    = TITLE_BGCOLOR
-- MENU HEADER
local LCD_MENU_COLOR          = MENU_TITLE_COLOR 
local LCD_MENU_BGCOLOR        = MENU_TITLE_BGCOLOR
-- LINE SELECTED 
local LCD_SELECTED_COLOR      = TEXT_INVERTED_COLOR
local LCD_SELECTED_BGCOLOR    = TEXT_INVERTED_BGCOLOR
local LCD_EDIT_BGCOLOR        = MENU_TITLE_BGCOLOR -- WARNING_COLOR 
-- NORMAL TEXT  
local LCD_NORMAL_COLOR        = TEXT_COLOR
local LCD_DISABLE_COLOR       = TEXT_DISABLE_COLOR
local LCD_DEBUG_COLOR         = LINE_COLOR
-- NORMAL BOX FRAME COLOR 
local LCD_BOX_COLOR           = TEXT_DISABLE_COLOR  



--------------------- lcd.sizeText replacement -------------------------------------------------
-- EdgeTx dont have lcd.sizeText, so we do an equivalent one using the string length and 5px per character
local function my_lcd_sizeText(s)
  -- return: If IS_EDGETX then lcd.sizeText() else string.len()
  return (IS_EDGETX and lcd.sizeText(s)) or (string.len(s)*10)  
end


local function GUI_SwitchSimulationOFF()
  dsmLib.ReleaseConnection()  
  dsmLib.LOG_close()

  SIMULATION_ON = false
  dsmLib = loadScript(DSMLIB_PATH .. "DsmFwPrgLib.lua")(DEBUG_ON)
  DSM_Context = dsmLib.DSM_Context

  dsmLib.Init(toolName)  -- Initialize Library 
  dsmLib.StartConnection()
  DSM_Context.Refresh_Display = true
end


--------------------- Toucch Button Helpers ------------------------------------------------------------
local function GUI_addTouchButton(x,y,w,h,line)
  -- Add new button info to end of the array
  touchButtonArea[#touchButtonArea+1] = {x=x, y=y, w=w, h=h, line=line}
end

local function GUI_getTouchButton(x,y)
  for i = 1, #touchButtonArea do
    local button = touchButtonArea[i]
    -- is the coordinate inside the button area??
    if (x >= button.x and x <= (button.x+button.w) and y >= button.y and (y <= button.y+button.h)) then
      return button.line
    end
  end
  return nil
end

local function GUI_clearTouchButtons()
  touchButtonArea           = {}
end

---------- Return Color to display Menu Lines ----------------------------------------------------------------
local function GUI_GetTextColor(lineNum)
  local ctx = DSM_Context
  local txtColor  = LCD_NORMAL_COLOR
  -- Gray Out any other line except the one been edited
  if (ctx.isEditing() and ctx.EditLine~=lineNum) then txtColor=LCD_DISABLE_COLOR end
  return txtColor
end

local function GUI_GetFrameColor(lineNum)  -- Frame Color for Value/Menu Boxes
  local ctx = DSM_Context
  local txtColor  = LCD_BOX_COLOR
  -- Gray Out any other line except the one been edited
  if (ctx.EditLine~=lineNum) then txtColor=LCD_DISABLE_COLOR end 
  return txtColor
end

--------------------------------------------------------------------------------------------------------
-- Display Text inside a Rectangle.  Inv: true means solid rectangle, false=only perimeter
local function GUI_Display_Boxed_Text(lineNum,x,y,w,h,text,inv, isNumber)
  local ctx = DSM_Context
  local txtColor  = GUI_GetTextColor(lineNum)
  local frameColor = GUI_GetFrameColor(lineNum)
  -- If editing this lineNum, chose EDIT Color, else SELECTED Color
  local selectedBGColor = (ctx.EditLine==lineNum and LCD_EDIT_BGCOLOR) or LCD_SELECTED_BGCOLOR 
  
  if (inv) then
    txtColor  = LCD_SELECTED_COLOR 
    lcd.drawFilledRectangle(x-5, y-2, w, h, selectedBGColor)
  else
    lcd.drawRectangle(x-5, y-2, w, h, frameColor)
  end
  if (isNumber) then
    print("DRAW NUMBER")
    lcd.drawNumber(x+w-10 , y, text, txtColor + RIGHT)
  else
    lcd.drawText(x , y, text, txtColor) 
  end
end

------ Display Pre/Next/Back buttons
local function GUI_Diplay_Button(x,y,w,h,text,selected)
  GUI_Display_Boxed_Text(-1,x,y,w,h,text,selected, false)
end

------ Display MENU type of lines (Navigation, SubHeaders, and plain text comments)
local function GUI_Display_Line_Menu(lineNum,line,selected)
  -- Menu Lines can be navidation to other Menus (if Selectable)
  -- Or SubHeaders or Messages

  local txtColor  = GUI_GetTextColor(lineNum)
  local y = LCD_Y_LINE_START+(LCD_Y_LINE_HEIGHT*lineNum)
  local x = LCD_X_LINE_MENU
 
  if dsmLib.isSelectableLine(line) then -- Draw Selectable Menus in Boxes
    GUI_Display_Boxed_Text(lineNum,x, y, LCD_W_LINE_MENU, LCD_Y_LINE_HEIGHT, line.Text,selected, false)
    GUI_addTouchButton(x, y, LCD_W_LINE_MENU, LCD_Y_LINE_HEIGHT,lineNum)
  else
    -- Non Selectable Menu Lines, plain text
    -- Can be use for sub headers or just regular text lines (like warnings)

    local bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.BOLD) and BOLD) or 0  

    if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.RIGHT) then -- Right Align???
        local tw = my_lcd_sizeText(line.Text)+4
        x =  LCD_X_LINE_VALUE - tw     -- Right 
    elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.CENTER) then -- Center??
        local tw = my_lcd_sizeText(line.Text) 
        x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_MENU)/2 - tw/2  -- Center - 1/2 Text
    end

    lcd.drawText(x, y, line.Text, txtColor + bold)  
  end
end

------ Display NAME : VALUES type of lines 
local function GUI_Display_Line_Value(lineNum, line, value, selected, editing)
  -- This Displays Name and Value Pairs
  local txtColor  = GUI_GetTextColor(lineNum)
  local bold      = 0
  local y = LCD_Y_LINE_START+(LCD_Y_LINE_HEIGHT*lineNum)
  local x = LCD_X_LINE_TITLE

  ---------- NAME Part 
  local header = line.Text
  -- ONLY do this for Flight Mode (Right Align or Centered)
  if (dsmLib.isFlightModeLine(line)) then
      -- Display Header + Value together
      header = dsmLib.GetFlightModeValue(line.TextId,header,value)

      -- Bold Text???
      bold = (dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.BOLD) and BOLD) or 0

      if dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.RIGHT) then -- Right Align
          local tw = my_lcd_sizeText(header)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif dsmLib.isDisplayAttr(line.TextAttr,DISP_ATTR.CENTER) then -- Centered
          local tw = my_lcd_sizeText(header)
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_TITLE)/2 - tw/2  -- Center - 1/2 Text
      end
  else
    -- No Flight Mode, no effects here
    header = header .. ":"
  end

  lcd.drawText(x, y, header, txtColor + bold) -- display Line Header

  --------- VALUE PART,  Skip for Flight Mode since already show the value 
  if not dsmLib.isFlightModeLine(line) then     
    if dsmLib.isSelectableLine(line) then 
      --if (editing) then -- Any Special color/effect when editing??
      --  value = "["..value .. "]"
      --end
      -- Can select/edit value, Box it 
      local tw = math.max(my_lcd_sizeText(value)+10,45) -- Width of the Text in the lcd
      GUI_Display_Boxed_Text(lineNum,LCD_X_LINE_VALUE,y,tw,LCD_Y_LINE_HEIGHT,value,selected, not dsmLib.isListLine(line))
      GUI_addTouchButton(LCD_X_LINE_VALUE,y,tw,LCD_Y_LINE_HEIGHT,lineNum)

      lcd.drawText(LCD_X_LINE_VALUE+tw+5, y,  (line.Format or ""), txtColor + bold)
    else -- Not Editable, Plain Text 
      lcd.drawText(LCD_X_LINE_VALUE, y, value, txtColor)
    end
  end
  
  -- Debug info for line Value RANGE when Debug on LCD
  if (DEBUG_ON_LCD) then  lcd.drawText(LCD_X_LINE_DEBUG, y, line.MinMaxDebug or "", SMLSIZE + LCD_DEBUG_COLOR) end -- display debug Min/Max
end

local function GUI_Display_Menu(menu)
  local ctx = DSM_Context
  local w=  LCD_W_MENU_TITLE

  -- Center Header
  local tw = my_lcd_sizeText(menu.Text) 
  local x = w/2 - tw/2  -- Center of Screen - Center of Text

  lcd.drawFilledRectangle(0, LCD_Y_MENU_TITLE-2, w, LCD_Y_LINE_HEIGHT-2, LCD_MENU_BGCOLOR)
  lcd.drawText(x,LCD_Y_MENU_TITLE,menu.Text,  LCD_MENU_COLOR + BOLD) 

  -- Back Button
  if menu.BackId ~= 0 then
    GUI_Diplay_Button(437-5,LCD_Y_MENU_TITLE+3,47,LCD_Y_LINE_HEIGHT,"Back",ctx.SelLine == dsmLib.BACK_BUTTON)
    GUI_addTouchButton(437-5,LCD_Y_MENU_TITLE+3,47,LCD_Y_LINE_HEIGHT,dsmLib.BACK_BUTTON)
  end
  -- Next Button
  if menu.NextId ~= 0 then
    GUI_Diplay_Button(437-5,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,"Next",ctx.SelLine == dsmLib.NEXT_BUTTON)
    GUI_addTouchButton(437-5,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,dsmLib.NEXT_BUTTON)
  end
  -- Prev Button
  if menu.PrevId ~= 0 then
    GUI_Diplay_Button(10,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,"Prev",ctx.SelLine == dsmLib.PREV_BUTTON)
    GUI_addTouchButton(10,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,dsmLib.PREV_BUTTON)
  end

  -- Debug on LCD, Show the menu Indo and Phase we are on 
  if (DEBUG_ON_LCD) then lcd.drawText(0,LCD_Y_MENU_TITLE,dsmLib.phase2String(ctx.Phase),SMLSIZE+LCD_DEBUG_COLOR) end  -- Phase we are in 
  if (DEBUG_ON_LCD) then lcd.drawText(0,240,dsmLib.menu2String(menu),SMLSIZE+LCD_DEBUG_COLOR) end  -- Menu Info
end

------------------------------------------------------------------------------------------------------------
-- Display the EDIT mode buttons when editing a value 

local function GUI_Display_Edit_Buttons(line)
  GUI_clearTouchButtons() -- Only this buttons can be touched
  local x = 15 -- Inittial X position
  local w = 55 -- Width of the buttons

  local showPrev = line.Val > line.Min
  local showNext = line.Val < line.Max

  GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"ESC",true)
  GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.ESC)

  x=x+w+10
  GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"  Def",true)
  GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.DEFAULT)

  x=x+w+10
  if (not dsmLib.isListLine(line)) then
    GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"  <<  ",showPrev)
    GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.DEC_10)
  end

  x=x+w+10
  GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"    <",showPrev)
  GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.DEC_1)

  x=x+w+10
  GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"    >",showNext)
  GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.INC_1)

  x=x+w+10
  if (not dsmLib.isListLine(line)) then
    GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"   >>",showNext)
    GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.INC_10)
  end
    
  x=x+w+10
  GUI_Diplay_Button(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,"   OK",true)
  GUI_addTouchButton(x,LCD_Y_LOWER_BUTTONS,w,LCD_Y_LINE_HEIGHT,EDIT_BUTTON.OK)

end

local function GUI_ShowBitmap(x,y,imgData)
    -- imgData format "bitmap.png|alt message"
    local f = string.gmatch(imgData, '([^%|]+)') -- Iterator over values split by '|'
    local imgName, imgMsg = f(), f()

    lcd.drawText(x, y, imgMsg or "")  -- Alternate Image MSG 

    local imgPath = IMAGE_PATH .. (imgName or "")
    local bitmap  = Bitmap.open(imgPath)
    if (bitmap~=nil) then
       lcd.drawBitmap(bitmap, x,y+20)
    end
end

------------------------------------------------------------------------------------------------------------
local function GUI_Display()
  local ctx = DSM_Context
  lcd.clear()
  GUI_clearTouchButtons()
 
  if LCD_W == 480 then
    local header = "DSM Forward Programming "..VERSION.."                   "
    if ctx.Phase ~= PHASE.RX_VERSION then
      header = header .. "RX "..ctx.RX.Name.." v"..ctx.RX.Version
    end

    --Draw title
    lcd.drawFilledRectangle(0, 0, LCD_W, 17, LCD_TOOL_HDR_BGCOLOR)
    lcd.drawText(5, 0, header,  LCD_TOOL_HDR_COLOR + SMLSIZE)
    --Draw RX Menu
    if ctx.Phase == PHASE.RX_VERSION then
      if (ctx.isReset) then
        lcd.drawText(LCD_X_LINE_TITLE,100,"Waiting for RX to Restart", BLINK)
      else
        lcd.drawText(LCD_X_LINE_TITLE,100,"No compatible DSM RX...", BLINK)
      end
    else
      local menu = ctx.Menu
      

      if menu.Text ~=  nil then
        GUI_Display_Menu(menu)

        for i = 0, dsmLib.MAX_MENU_LINES do
          local line = ctx.MenuLines[i]

          if i == ctx.SelLine then
            -- DEBUG: Display Selected Line info for ON SCREEN Debugging
            if (DEBUG_ON_LCD) then lcd.drawText(0,255,dsmLib.menuLine2String(line),SMLSIZE + LCD_DEBUG_COLOR) end
          end

          if line ~= nil and line.Type ~= 0 then
            if line.Type == LINE_TYPE.MENU then 
              GUI_Display_Line_Menu(i, line, i == ctx.SelLine)
            else  
              if line.Val ~= nil then
                local value = line.Val

                if dsmLib.isListLine(line) then    -- for Lists of Strings, get the text
                  value = dsmLib.Get_List_Text(line.Val + line.TextStart) -- TextStart is the initial offset for text
                  local imgData = dsmLib.Get_List_Text_Img(line.Val + line.TextStart)   -- Complentary IMAGE for this value to Display??
                    
                  if (imgData) then  -- Optional Image and Msg for value
                    GUI_ShowBitmap(LCD_X_LINE_TITLE,LCD_Y_LINE_START, imgData)
                  end
                end

                GUI_Display_Line_Value(i, line, value, i == ctx.SelLine, i == ctx.EditLine)
              end
            end -- if ~MENU
          end -- if Line[i]~=nil
        end  -- for

        if IS_EDGETX and ctx.isEditing()  then
          -- Display Touch button for Editing values
          GUI_Display_Edit_Buttons(ctx.MenuLines[ctx.EditLine])
        end
      end 
    end
  else
    -- Different Resolution.. Maybe just adjusting some of the constants will work, adjust it in DSM_Init??
    -- LCD_X_LINE_TITLE,  LCD_Y_LINE_START, etc
    lcd.drawText(LCD_X_LINE_TITLE,100,"Only supported in Color Radios of 480 resolution", BLINK)
  end
end

-------------------------------------------------------------------------------------------------------------
local function GUI_RotEncVal(dir) -- return encoder speed to inc or dec values
  local inc = 0
  local Speed = getRotEncSpeed()

  if Speed == ROTENC_MIDSPEED then  inc = (5 * dir)
  elseif Speed == ROTENC_HIGHSPEED then  inc = (15 * dir)
  else  inc = dir  end

  return inc
end

------------------------------------------------------------------------------------
-- Translate Tap/Touch of EDIT buttons to equivalent Key events
local function GUI_Translate_Edit_Buttons(button)
    local event = EVT_TOUCH_TAP
    local editInc = nil

    if (button==EDIT_BUTTON.ESC) then   -- ESC 
      event = EVT_VIRTUAL_EXIT
    elseif (button==EDIT_BUTTON.DEFAULT) then   -- Default 
      event = EVT_VIRTUAL_ENTER_LONG
    elseif (button==EDIT_BUTTON.DEC_10) then --  -10
        event = EVT_VIRTUAL_PREV
        editInc = -10
    elseif (button==EDIT_BUTTON.DEC_1) then  -- -1
      event = EVT_VIRTUAL_PREV
      editInc = -1
    elseif (button==EDIT_BUTTON.INC_1) then  -- +1
      event = EVT_VIRTUAL_NEXT
      editInc = 1
    elseif (button==EDIT_BUTTON.INC_10) then  -- + 10 
      event = EVT_VIRTUAL_NEXT
      editInc = 10
    elseif (button==EDIT_BUTTON.OK) then  -- OK 
      event = EVT_VIRTUAL_ENTER
    else

    end
    
    return event, editInc
end

------------------------------------------------------------------------------------------------------------
-- Handle Events comming from the GUI
local function GUI_HandleEvent(event, touchState)
  local ctx = DSM_Context
  local menu = ctx.Menu
  local menuLines = ctx.MenuLines
  local editInc   = nil 

  if (IS_EDGETX) then
    if (event == EVT_TOUCH_TAP and ctx.isEditing()) then -- Touch and Editing 
      local button = GUI_getTouchButton(touchState.x, touchState.y)
      if (button) then
        event, editInc = GUI_Translate_Edit_Buttons(button)
      end
    end

    if (event == EVT_TOUCH_TAP or event == EVT_TOUCH_FIRST) and not ctx.isEditing() then  -- Touch and NOT editing
      if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_TOUCH_TAP %d,%d\n",dsmLib.phase2String(ctx.Phase),touchState.x, touchState.y) end
        local button = GUI_getTouchButton(touchState.x, touchState.y)
        if button then
          -- Found a valid line
          ctx.SelLine = button
          ctx.Refresh_Display=true
          if event == EVT_TOUCH_TAP then  -- EVT_TOUCH_FIRST only move focus
            event = EVT_VIRTUAL_ENTER
          end
        end
    end
  end -- IS_EDGETX

  if event == EVT_VIRTUAL_EXIT then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_EXIT\n",dsmLib.phase2String(ctx.Phase)) end
    if ctx.Phase == PHASE.RX_VERSION then
      dsmLib.ReleaseConnection()  -- Just Exit the Script 
    else
      if ctx.isEditing() then  -- Editing a Line, need to  restore original value
        local line = ctx.MenuLines[ctx.EditLine]
        line.Val = originalValue
        dsmLib.Value_Write_Validate(line)
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
      dsmLib.Value_Add(line, editInc or GUI_RotEncVal(1))
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
      dsmLib.Value_Add(line, editInc or GUI_RotEncVal(-1))
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
      dsmLib.Value_Default(menuLines[ctx.EditLine])  -- Update value in RX if needed
    end
    return
  end

  if event == EVT_VIRTUAL_ENTER then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then dsmLib.LOG_write("%s: EVT_VIRTUAL_ENTER,  SelLine=%d\n",dsmLib.phase2String(ctx.Phase), ctx.SelLine) end
    if ctx.SelLine == dsmLib.BACK_BUTTON then -- Back
      dsmLib.GotoMenu(menu.BackId,0)
    elseif ctx.SelLine == dsmLib.NEXT_BUTTON then -- Next
      dsmLib.GotoMenu(menu.NextId,0)
    elseif ctx.SelLine == dsmLib.PREV_BUTTON then -- Prev
      dsmLib.GotoMenu(menu.PrevId,0)
    elseif menuLines[ctx.SelLine].ValId ~= 0 then  -- Menu or Value

      if menuLines[ctx.SelLine].Type == LINE_TYPE.MENU then -- Navigate to Menu
        if (SIMULATION_ON and menuLines[ctx.SelLine].ValId==0xFFFF) then
          -- SPECIAL Simulation menu to Exit Simulation
          GUI_SwitchSimulationOFF()
        else
          dsmLib.GotoMenu(menuLines[ctx.SelLine].ValId, ctx.SelLine)  -- ValId is the MenuId to navigate to
        end
      else -- Enter on a Value
        if ctx.isEditing() then   -- already editing a Line???? 
          dsmLib.Value_Write_Validate(menuLines[ctx.SelLine])
        else    -- Edit the current value  
          ctx.EditLine = ctx.SelLine
          originalValue = menuLines[ctx.SelLine].Val
        end
      end
    end
  end
end

local function init_colors()
  -- osName in OpenTX is nil, otherwise is EDGETX 
  local ver, radio, maj, minor, rev, osname = getVersion()
  if (osname==nil) then osname = "OpenTX" end -- OTX 2.3.14 and below returns nil

  IS_EDGETX = string.sub(osname,1,1) == 'E'

  if (IS_EDGETX and USE_SPECKTRUM_COLORS) then
      -- SPECKTRUM COLORS (only works on EDGETX)
      -- TOOL HEADER 
      LCD_TOOL_HDR_COLOR      = MENU_TITLE_COLOR
      LCD_TOOL_HDR_BGCOLOR    = TITLE_BGCOLOR
      -- MENU HEADER
      LCD_MENU_COLOR          = WHITE
      LCD_MENU_BGCOLOR        = DARKGREY
      -- LINE SELECTED 
      LCD_SELECTED_COLOR      = WHITE
      LCD_SELECTED_BGCOLOR    = ORANGE
      LCD_EDIT_BGCOLOR        = RED  
      -- NORMAL TEXT  
      LCD_NORMAL_COLOR        = BLACK
      LCD_DISABLE_COLOR       = LIGHTGREY
      LCD_DEBUG_COLOR         = BLUE
      -- NORMAL BOX FRAME COLOR 
      LCD_BOX_COLOR           = LIGHTGREY  
  end
end

------------------------------------------------------------------------------------------------------------
-- Init
local function DSM_Init()
  init_colors()
  dsmLib.Init(toolName)  -- Initialize Library 
  return dsmLib.StartConnection()
end

------------------------------------------------------------------------------------------------------------
-- Main
local function DSM_Run(event,touchState)
  local ctx = DSM_Context

  if event == nil then
    error("Cannot be run as a model script!")
    dsmLib.LOG_close()
    return 2
  end

  GUI_HandleEvent(event,touchState)

  dsmLib.Send_Receive()  -- Handle Send and Receive DSM Forward Programming Messages

  local refreshInterval =  REFRESH_GUI_MS

  -- When using LCD BLINK attribute, we need faster refresh for BLINK to SHOW on LCD 
  if (ctx.Phase == PHASE.RX_VERSION) then  -- Requesting RX Message Version usea BLINK?
    ctx.Refresh_Display=true  
    refreshInterval = 20 -- 200ms
  end

  if (not IS_EDGETX) then -- OPENTX NEEDS REFRESH ON EVERY CYCLE
    GUI_Display()
  -- Refresh display only if needed and no faster than 300ms, utilize more CPU to speedup DSM communications
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