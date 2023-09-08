local toolName = "TNS|DSM Forward Prog v0.55a (Color) |TNE"
local VERSION  = "v0.55a"

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

local SIMULATION_ON = true   -- false: dont show simulation menu, TRUE: show simulation menu 
local DEBUG_ON = 1           -- 0=NO DEBUG, 1=HIGH LEVEL 2=LOW LEVEL   (Debug logged into the /LOGS/dsm.log)
local USE_SPECKTRUM_COLORS = true -- true: Use spectrum colors, false: use theme colors (default on OpenTX) 
local DSMLIB_PATH = "/SCRIPTS/TOOLS/DSMLIB/"
local IMAGE_PATH = DSMLIB_PATH .. "img/"

local Log = assert(loadScript(DSMLIB_PATH.."DsmLogLib.lua"), "Not-Found: DSMLIB/DsmLogLib.lua")()
local menuLib = assert(loadScript(DSMLIB_PATH.."DsmMenuLib.lua"), "Not-Found: DSMLIB/DsmMenuLib.lua")(Log, DEBUG_ON)
local modelLib = assert(loadScript(DSMLIB_PATH.."DsmModelLib.lua"), "Not-Found: DSMLIB/DsmModelLib.lua")(Log, DEBUG_ON)
local menuProcessor  = assert(loadScript(DSMLIB_PATH.."DsmMainMenuLib.lua"), "Not-Found: DSMLIB/DsmMainMenuLib.lua")(Log, menuLib, modelLib, DEBUG_ON, SIMULATION_ON)

local PHASE = menuLib.PHASE
local LINE_TYPE = menuLib.LINE_TYPE
local DISP_ATTR   = menuLib.DISP_ATTR
local DSM_Context = menuLib.DSM_Context


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
local LCD_Y_LINE_HEIGHT      = 27 

local LCD_Y_LOWER_BUTTONS    = LCD_Y_LINE_START + 3 + (7 * LCD_Y_LINE_HEIGHT)

-- TOOL BG COLOR
local LCD_TOOL_BGCOLOR        = TEXT_BGCOLOR
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


local warningScreenON = true


--------------------- lcd.sizeText replacement -------------------------------------------------
-- EdgeTx dont have lcd.sizeText, so we do an equivalent one using the string length and 5px per character
local function my_lcd_sizeText(s)
  if (s==nil) then return 20 end
  -- return: If IS_EDGETX then lcd.sizeText() else string.len()
  return (IS_EDGETX and lcd.sizeText(s)) or (string.len(s)*10)  
end


local function GUI_SwitchToRX()
  -- Force to refresh DSM Info in MODEL (dsmLib pointing to the setup Script)
  -- local dsmChannelInfo, description = modelLib.CreateDSMPortChannelInfo()

  menuProcessor.done() 
  Log.LOG_close()  -- Reset the log
  Log.LOG_open()

  menuProcessor = nil
  collectgarbage("collect")
  
  modelLib.ReadTxModelData()
  modelLib.ST_LoadFileData()
  modelLib.CreateDSMPortChannelInfo()

  local dsmLib = assert(loadScript(DSMLIB_PATH.."DsmFwPrgLib.lua"),"Not-Found: DSMLIB/DsmFwPrgLib.lua")
  menuProcessor = dsmLib(Log, menuLib, modelLib, DEBUG_ON)

  dsmLib = nil
  collectgarbage("collect")

  menuProcessor.init(toolName)  -- Initialize Library 
  DSM_Context.Refresh_Display = true
end

local function GUI_SwitchToSIM()
  menuProcessor.done() 
  Log.LOG_close()

  menuProcessor = nil
  collectgarbage("collect")

  local simLib = assert(loadScript(DSMLIB_PATH.."DsmSimMenuLib.lua"), "Not-Found: DSMLIB/DsmSimMenuLib.lua")
  menuProcessor = simLib(Log, menuLib, modelLib, DEBUG_ON)

  simLib = nil
  collectgarbage("collect")

  menuProcessor.init(toolName)  -- Initialize Library 
  DSM_Context.Refresh_Display = true
end

local function GUI_SwitchToSetupMenu()
  menuProcessor.done() 

  menuProcessor = nil
  collectgarbage("collect")

  local setupLib = assert(loadScript(DSMLIB_PATH.."DsmSetupMenuLib.lua"), "Not-Found: DSMLIB/DsmSetupMenuLib.lua")
  menuProcessor = setupLib(Log, menuLib, modelLib, DEBUG_ON, SIMULATION_ON)

  setupLib = nil
  collectgarbage("collect")

  menuProcessor.init(toolName)  -- Initialize Library 
  DSM_Context.Refresh_Display = true
end

local function GUI_SwitchToMainMenu()
  print("SWITCHING TO MAIN MENU")
  menuProcessor.done() 

  menuProcessor = nil
  collectgarbage("collect")

  local mainMenuLib =  assert(loadScript(DSMLIB_PATH.."DsmMainMenuLib.lua"), "Not-Found: DSMLIB/DsmMainMenuLib.lua")
  menuProcessor = mainMenuLib(Log, menuLib, modelLib, DEBUG_ON, SIMULATION_ON)

  mainMenuLib = nil
  collectgarbage("collect")

  menuProcessor.init(toolName)  -- Initialize Library 
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
 
  if menuLib.isSelectableLine(line) then -- Draw Selectable Menus in Boxes
    GUI_Display_Boxed_Text(lineNum,x, y, LCD_W_LINE_MENU, LCD_Y_LINE_HEIGHT, line.Text,selected, false)
    GUI_addTouchButton(x, y, LCD_W_LINE_MENU, LCD_Y_LINE_HEIGHT,lineNum)
  else
    -- Non Selectable Menu Lines, plain text
    -- Can be use for sub headers or just regular text lines (like warnings)

    local bold = (menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._BOLD) and BOLD) or 0  

    if menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._RIGHT) then -- Right Align???
        local tw = my_lcd_sizeText(line.Text)+4
        x =  LCD_X_LINE_VALUE - tw     -- Right 
    elseif menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._CENTER) then -- Center??
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
  if (menuLib.isFlightModeLine(line)) then
      -- Display Header + Value together
      header = menuLib.GetFlightModeValue(line)

      -- Bold Text???
      bold = (menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._BOLD) and BOLD) or 0

      if menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._RIGHT) then -- Right Align
          local tw = my_lcd_sizeText(header)+4
          x =  LCD_X_LINE_VALUE - tw     -- Right 
      elseif menuLib.isDisplayAttr(line.TextAttr,DISP_ATTR._CENTER) then -- Centered
          local tw = my_lcd_sizeText(header)
          x =  x + (LCD_X_LINE_VALUE - LCD_X_LINE_TITLE)/2 - tw/2  -- Center - 1/2 Text
      end
  else
    -- No Flight Mode, no effects here
    header = header .. ":"
  end

  lcd.drawText(x, y, header, txtColor + bold) -- display Line Header


  --------- VALUE PART,  Skip for Flight Mode since already show the value
  if (value==nil) then return end 
  
  if not menuLib.isFlightModeLine(line) then
    if menuLib.isSelectableLine(line) then 
      --if (editing) then -- Any Special color/effect when editing??
      --  value = "["..value .. "]"
      --end
      -- Can select/edit value, Box it 
      local tw = math.max(my_lcd_sizeText(value)+10,45) -- Width of the Text in the lcd
      GUI_Display_Boxed_Text(lineNum,LCD_X_LINE_VALUE,y,tw,LCD_Y_LINE_HEIGHT,value,selected, not menuLib.isListLine(line))
      GUI_addTouchButton(LCD_X_LINE_VALUE,y,tw,LCD_Y_LINE_HEIGHT,lineNum)

      lcd.drawText(LCD_X_LINE_VALUE+tw+5, y,  (line.Format or ""), txtColor + bold)
    else -- Not Editable, Plain Text 
      lcd.drawText(LCD_X_LINE_VALUE, y, value, txtColor)
    end
  end
  
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
    GUI_Diplay_Button(437-5,LCD_Y_MENU_TITLE+3,47,LCD_Y_LINE_HEIGHT,"Back",ctx.SelLine == menuLib.BACK_BUTTON)
    GUI_addTouchButton(437-5,LCD_Y_MENU_TITLE+3,47,LCD_Y_LINE_HEIGHT,menuLib.BACK_BUTTON)
  end
  -- Next Button
  if menu.NextId ~= 0 then
    GUI_Diplay_Button(437-5,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,"Next",ctx.SelLine == menuLib.NEXT_BUTTON)
    GUI_addTouchButton(437-5,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,menuLib.NEXT_BUTTON)
  end
  -- Prev Button
  if menu.PrevId ~= 0 then
    GUI_Diplay_Button(10,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,"Prev",ctx.SelLine == menuLib.PREV_BUTTON)
    GUI_addTouchButton(10,LCD_Y_LOWER_BUTTONS,47,LCD_Y_LINE_HEIGHT,menuLib.PREV_BUTTON)
  end
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
  if (not menuLib.isListLine(line)) then
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
  if (not menuLib.isListLine(line)) then
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
  lcd.clear(LCD_TOOL_BGCOLOR)
  GUI_clearTouchButtons()
 
  if LCD_W ~= 480 then
    -- Different Resolution.. Maybe just adjusting some of the constants will work, adjust it in DSM_Init??
    -- LCD_X_LINE_TITLE,  LCD_Y_LINE_START, etc
    lcd.drawText(LCD_X_LINE_TITLE,100,"Only supported in Color Radios of 480 resolution", BLINK)
    return
  end

  local header = "DSM Forward Programming "..VERSION.."                   "
  if ctx.Phase ~= PHASE.RX_VERSION then
    header = header .. ctx.RX.Name.." v"..ctx.RX.Version
  end

  --Draw title
  lcd.drawFilledRectangle(0, 0, LCD_W, 17, LCD_TOOL_HDR_BGCOLOR)
  lcd.drawText(5, 0, header,  LCD_TOOL_HDR_COLOR + SMLSIZE)

  -- Getting RX Version
  if ctx.Phase == PHASE.RX_VERSION then
    if (ctx.isReset) then
      lcd.drawText(LCD_X_LINE_TITLE,100, menuLib.Get_Text(0x301), BLINK) -- Resetting...
    else
      lcd.drawText(LCD_X_LINE_TITLE,100,menuLib.Get_Text(0x300), BLINK) -- Not valid RX
    end
    return
  end


  local menu = ctx.Menu  
  if menu.Text ==  nil then return end

  -----   Draw RX Menu  --------- 
  GUI_Display_Menu(menu)

  -- Sending TX Information???
  if (ctx.Phase==PHASE.MENU_REQ_TX_INFO) then
    --lcd.drawFilledRectangle(x-5, y-2, w, h, selectedBGColor)
    --lcd.drawRectangle(x-5, y-2, w, h, frameColor)
    lcd.drawText(LCD_X_LINE_TITLE,100, "Sending CH"..(ctx.CurLine+1)) -- Channel Info 
    return
  end

  for i = 0, menuLib.MAX_MENU_LINES do
    local line = ctx.MenuLines[i]

    if line ~= nil and line.Type ~= 0 then
      if line.Type == LINE_TYPE.MENU then 
        GUI_Display_Line_Menu(i, line, i == ctx.SelLine)
      else
        local value = nil
          if line.Val ~= nil then
            value = line.Val
            if menuLib.isListLine(line) then    -- for Lists of Strings, get the text
              value = menuLib.Get_List_Text(line.Val + line.TextStart) -- TextStart is the initial offset for text
              -- Complentary IMAGE for this value to Display??
              local offset = 0
              if (line.Type==LINE_TYPE.LIST_MENU_ORI) then offset = offset + 0x100 end --FH6250 hack

              local imgData = menuLib.Get_List_Text_Img(line.Val + line.TextStart + offset)   
                
              if (imgData and i == ctx.SelLine) then  -- Optional Image and Msg for selected value
                GUI_ShowBitmap(LCD_X_LINE_TITLE,LCD_Y_LINE_START, imgData)
              end
            end
          end -- if Line[i]~=nil
          GUI_Display_Line_Value(i, line, value, i == ctx.SelLine, i == ctx.EditLine)
        end
      end -- if ~MENU
  end  -- for

  if IS_EDGETX and ctx.isEditing()  then
    -- Display Touch button for Editing values
    GUI_Display_Edit_Buttons(ctx.MenuLines[ctx.EditLine])
  end
end

-------------------------------------------------------------------------------------------------------------
local function GUI_RotEncVal(line, dir) -- return encoder speed to inc or dec values

  if menuLib.isListLine(line) then return dir end

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
      if (DEBUG_ON) then Log.LOG_write("%s: EVT_TOUCH_TAP %d,%d\n",menuLib.phase2String(ctx.Phase),touchState.x, touchState.y) end
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
    if (DEBUG_ON) then Log.LOG_write("%s: EVT_VIRTUAL_EXIT\n",menuLib.phase2String(ctx.Phase)) end
    if ctx.Phase == PHASE.RX_VERSION then
      menuLib.ChangePhase(PHASE.EXIT_DONE) -- Just Exit the Script 
    else
      if ctx.isEditing() then  -- Editing a Line, need to  restore original value
        local line = ctx.MenuLines[ctx.EditLine]
        line.Val = originalValue
        menuLib.Value_Write_Validate(line)
      elseif (menu.BackId > 0 ) then -- Back??
          ctx.SelLine = menuLib.BACK_BUTTON 
          event = EVT_VIRTUAL_ENTER
      else
        menuLib.ChangePhase(PHASE.EXIT)
      end
    end
  end

  if ctx.Phase == PHASE.RX_VERSION then return end
  
  if event == EVT_VIRTUAL_NEXT then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then Log.LOG_write("%s: EVT_VIRTUAL_NEXT\n",menuLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then  -- Editing a Line, need to inc the value
      local line=ctx.MenuLines[ctx.EditLine]
      menuLib.Value_Add(line, editInc or GUI_RotEncVal(line, 1))
    else  -- not editing, move selected line to NEXT
      menuLib.MoveSelectionLine(1)
    end
    return
  end

  if event == EVT_VIRTUAL_PREV then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then Log.LOG_write("%s: EVT_VIRTUAL_PREV\n",menuLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then  -- Editiing a line, need to dec the value
      local line=ctx.MenuLines[ctx.EditLine]
      menuLib.Value_Add(line, editInc or GUI_RotEncVal(line, -1))
    else  -- not editing, move selected line to PREV
      menuLib.MoveSelectionLine(-1)
    end
    return
  end

  if event == EVT_VIRTUAL_ENTER_LONG then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then Log.LOG_write("%s: EVT_VIRTUAL_ENTER_LONG\n",menuLib.phase2String(ctx.Phase)) end
    if ctx.isEditing() then
      -- reset the value to default
      menuLib.Value_Default(menuLines[ctx.EditLine])  -- Update value in RX if needed
    end
    return
  end

  if event == EVT_VIRTUAL_ENTER then
    ctx.Refresh_Display=true
    if (DEBUG_ON) then Log.LOG_write("%s: EVT_VIRTUAL_ENTER,  SelLine=%d\n",menuLib.phase2String(ctx.Phase), ctx.SelLine) end
    if ctx.SelLine == menuLib.BACK_BUTTON then -- Back
      if (menu.BackId==0xFFF9) then
        -- SPECIAL Main Menu
        GUI_SwitchToMainMenu()
      else
        menuLib.GotoMenu(menu.BackId,0x80)
      end
    elseif ctx.SelLine == menuLib.NEXT_BUTTON then -- Next
      menuLib.GotoMenu(menu.NextId,0x82)
    elseif ctx.SelLine == menuLib.PREV_BUTTON then -- Prev
      menuLib.GotoMenu(menu.PrevId,0x81)
    elseif menuLines[ctx.SelLine].ValId ~= 0 then  -- Menu or Value

      if menuLines[ctx.SelLine].Type == LINE_TYPE.MENU then -- Navigate to Menu
        if (menuLines[ctx.SelLine].ValId==0xFFF1) then
          -- SPECIAL Simulation menu to Simulator 
          GUI_SwitchToSIM()
        elseif (menuLines[ctx.SelLine].ValId==0xFFF2) then
            -- SPECIAL Simulation menu to go to RX 
            GUI_SwitchToRX()
        elseif (menuLines[ctx.SelLine].ValId==0xFFF3) then
          -- SPECIAL Settup Menu
          GUI_SwitchToSetupMenu()
        elseif (menuLines[ctx.SelLine].ValId==0xFFF9) then
          -- SPECIAL Settup Menu
          GUI_SwitchToMainMenu()
        else
          menuLib.GotoMenu(menuLines[ctx.SelLine].ValId, ctx.SelLine)  -- ValId is the MenuId to navigate to
        end
      else -- Enter on a Value
        if ctx.isEditing() then   -- already editing a Line???? 
          menuLib.Value_Write_Validate(menuLines[ctx.SelLine])
        else    -- Edit the current value  
          ctx.EditLine = ctx.SelLine
          originalValue = menuLines[ctx.SelLine].Val
          menuLib.ChangePhase(PHASE.VALUE_CHANGING_WAIT)
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
      LCD_TOOL_BGCOLOR        = LIGHTWHITE
      -- TOOL HEADER 
      LCD_TOOL_HDR_COLOR      = WHITE
      LCD_TOOL_HDR_BGCOLOR    = DARKBLUE
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

local function GUI_Warning(event,touchState)
  lcd.clear(LCD_TOOL_BGCOLOR)
  local header = "DSM Forward Programming "..VERSION.."                   "
  --Draw title
  lcd.drawFilledRectangle(0, 0, LCD_W, 17, LCD_TOOL_HDR_BGCOLOR)
  lcd.drawText(5, 0, header,  LCD_TOOL_HDR_COLOR + SMLSIZE)

  lcd.drawText(100,20,"INFO", BOLD)
  lcd.drawText(5,40,"DSM Forward programing shares TX Servo/Output settings", 0)
  lcd.drawText(5,60,"with the RX. Make sure you setup your plane first in ", 0)
  lcd.drawText(5,80,"the TX before your start Fwrd programming your RX.", 0)
  lcd.drawText(5,100,"Wing & Tail type can be configured using this tool.", 0)

  lcd.drawText(5,150,"TX Gyro Servo settings are sent to the RX during 'Initial Setup'", 0)
  lcd.drawText(5,170,"as well as when using RX 'Relearn Servo Settings'", 0)
  lcd.drawText(5,200,"ALWAYS TEST Gyro reactions after this conditions before flying.", BOLD)

  lcd.drawText(100,250,"    OK     ", INVERS + BOLD)

  if event == EVT_VIRTUAL_EXIT or event == EVT_VIRTUAL_ENTER or event == EVT_TOUCH_TAP then
    warningScreenON = false
  end

  return 0
end

------------------------------------------------------------------------------------------------------------
-- Init
local function DSM_Init()
  Log.LOG_open()

  init_colors()
  modelLib.ReadTxModelData()
  modelLib.ST_LoadFileData()
  modelLib.CreateDSMPortChannelInfo()
  
  menuLib.Init() 
  menuProcessor.init()
  return 0
end

------------------------------------------------------------------------------------------------------------
-- Main
local function DSM_Run(event,touchState)
  local ctx = DSM_Context

  if event == nil then
    error("Cannot be run as a model script!")
    Log.LOG_close()
    return 2
  end

  if (warningScreenON) then
    return GUI_Warning(event,touchState)
  end

  GUI_HandleEvent(event,touchState)

  local ret = menuProcessor.run()  -- Handle Send and Receive DSM Forward Programming Messages

  if ctx.Phase == PHASE.INIT then return 0 end
  

  local refreshInterval =  REFRESH_GUI_MS

  -- When using LCD BLINK attribute, we need faster refresh for BLINK to SHOW on LCD 
  if (ctx.Phase == PHASE.RX_VERSION or ctx.Phase==PHASE.MENU_REQ_TX_INFO) then  -- Requesting RX Message Version usea BLINK?
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
    Log.LOG_close()
    return 2
  else
    return 0
  end
end


return { init=DSM_Init, run=DSM_Run }