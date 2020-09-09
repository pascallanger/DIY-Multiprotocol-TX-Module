local toolName = "TNS|DSM Forward Programming|TNE"

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

local RX_VERSION, WAIT_CMD, MENU_TITLE, MENU_LINES, MENU_VALUES, VALUE_CHANGING, VALUE_CHANGING_WAIT, VALUE_CHANGED = 0, 1, 2, 3, 4, 5, 6, 7
local MENU, LIST_MENU_NOCHANGING, LIST_MENU2, PERCENTAGE_VALUE = 0x1C, 0x6C, 0x4C, 0xC0
local Phase = RX_VERSION
local Waiting_RX = 0
local Prev_Menu = 0
local Cur_Menu = -1
local Next_line = -1
local RX_Version = ""
local Menu_Title = ""
local Menu_Line = {}
local Next_Menu = {}
local Menu_Value = {}
local Menu_Type = {}
local Menu_Value_Max = {}
local Menu_Value_Min = {}
local Menu_Value_Def = {}
local Text = {}
local Retry=100
local Selected_Line=-1
local Menu_Edit = nil
local Blink = 0
local Value_Changed=0

local function conv_int16(number)
  if number >= 0x8000 then
    return number - 0x10000
  end
  return number
end

local function DSM_Release()
  multiBuffer( 0, 0 )
end

local function DSM_Send(...)
  local arg = {...}
  for i = 1 , #arg do
    multiBuffer( 3+i, arg[i])
  end
  multiBuffer( 3, 0x70+#arg)
end

local function DSM_Menu(event)
  local Speed = 0
  if event == EVT_VIRTUAL_NEXT then
    if Menu_Edit == nil then
      -- not changing a value
      if Selected_Line ~= -1 and Selected_Line < 6 then
        for line = Selected_Line + 1, 6, 1 do
          if Menu_Line[line] ~= "" and Next_Menu[line] ~= Cur_Menu then
            Selected_Line=line
            break
          end
        end
      end
    else
      -- need to inc the value
      if Menu_Value[Selected_Line] < Menu_Value_Max[Selected_Line] then
        Speed = getRotEncSpeed()
        if Speed == ROTENC_MIDSPEED then
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] + 5
        elseif Speed == ROTENC_HIGHSPEED then
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] + 15
        else
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] + 1
        end
        if Menu_Value[Selected_Line] > Menu_Value_Max[Selected_Line] then
          Menu_Value[Selected_Line] = Menu_Value_Max[Selected_Line]
        end
        if Menu_Type[Selected_Line] ~= LIST_MENU_NOCHANGING then
          Phase = VALUE_CHANGING
          Waiting_RX = 0
        end
      end
    end
  elseif event == EVT_VIRTUAL_PREV then
    if Menu_Edit == nil then
      if Selected_Line > 0 then
        for line = Selected_Line-1, 0, -1 do
          if Menu_Line[line] ~= "" and Next_Menu[line] ~= Cur_Menu then
            Selected_Line=line
            break
          end
        end
      end
    else
      -- need to dec the value
      if Menu_Value[Selected_Line] > Menu_Value_Min[Selected_Line] then
        Speed = getRotEncSpeed()
        if Speed == ROTENC_MIDSPEED then
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] - 5
        elseif Speed == ROTENC_HIGHSPEED then
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] - 15
        else
          Menu_Value[Selected_Line] = Menu_Value[Selected_Line] - 1
        end
        if Menu_Value[Selected_Line] < Menu_Value_Min[Selected_Line] then
          Menu_Value[Selected_Line] = Menu_Value_Min[Selected_Line]
        end
        if Menu_Type[Selected_Line] ~= LIST_MENU_NOCHANGING then
          Phase = VALUE_CHANGING
          Waiting_RX = 0
        end
      end
    end
  elseif event == EVT_VIRTUAL_ENTER then
    if Selected_Line ~= -1 then
      if Menu_Type[Selected_Line] == MENU then -- Menu entry
        if Next_Menu[Selected_Line] ~= nil then -- Next menu exist
          Cur_Menu = Next_Menu[Selected_Line]
          Phase = MENU_TITLE
          Waiting_RX = 0
        end
      else
        -- value entry
        if Menu_Edit == Selected_Line then
          Menu_Edit = nil
          Value_Changed=0
          Phase = VALUE_CHANGED
          Waiting_RX = 0
        else
          Menu_Edit = Selected_Line
        end
      end
    end
  elseif event == EVT_VIRTUAL_PREV_PAGE then
    killEvents(event)
    if Cur_Menu ~= 0 then
      Cur_Menu = Prev_Menu
      Phase = MENU_TITLE
      Waiting_RX = 0
    end
  end
end

local function DSM_Send_Receive()
  if Waiting_RX == 0 then
    Waiting_RX = 1
    -- Need to send a request
    if Phase == RX_VERSION then -- request RX version
      DSM_Send(0x11,0x06,0x00,0x14,0x00,0x00)
    elseif Phase == WAIT_CMD then -- keep connection open
      DSM_Send(0x00,0x04,0x00,0x00)
    elseif Phase == MENU_TITLE then -- request menu title
      if Cur_Menu == -1 then
        DSM_Send(0x12,0x06,0x00,0x14,0x00,0x00)
        Cur_Menu = 0
      else
        DSM_Send(0x16,0x06,0x10,Cur_Menu,0x00,0x01) -- last byte is 0 or 1=unknown...
      end
    elseif Phase == MENU_LINES then -- request menu lines
      if Cur_Line == -1 then
        DSM_Send(0x13,0x04,0x10,Cur_Menu) -- line 0
      elseif Cur_Line >= 0x80 then
        local last_byte={0x40,0x01,0x02,0x04,0x00,0x00} -- unknown...
        DSM_Send(0x20,0x06,Cur_Line-0x80,Cur_Line-0x80,0x00,last_byte[Cur_Line-0x80+1]) -- line X
      else
        DSM_Send(0x14,0x06,0x10,Cur_Menu,0x00,Cur_Line) -- line X
      end
    elseif Phase == MENU_VALUES then -- request menu values
      DSM_Send(0x15,0x06,0x10,Cur_Menu,0x10,Cur_Line) -- line X
    elseif Phase == VALUE_CHANGING then -- send value
      local value=Menu_Value[Selected_Line]
      if value < 0 then
        value = 0x10000 + value
      end
      DSM_Send(0x18,0x06,0x10,Selected_Line,bit32.rshift(value,8),bit32.band(value,0xFF)) -- send current value
      Phase = VALUE_CHANGING_WAIT
    elseif Phase == VALUE_CHANGED then -- send value
      if Value_Changed == 0 then
        local value=Menu_Value[Selected_Line]
        if value < 0 then
          value = 0x10000 + value
        end
        DSM_Send(0x18,0x06,0x10,Selected_Line,bit32.rshift(value,8),bit32.band(value,0xFF)) -- send current value
        Value_Changed = Value_Changed + 1
        Waiting_RX = 0
      elseif Value_Changed == 1 then
        DSM_Send(0x19,0x06,0x10,Selected_Line) -- validate
      --  Value_Changed = Value_Changed + 1
      --  Waiting_RX = 0
      --elseif Value_Changed == 2 then
      --  DSM_Send(0x1B,0x06,0x10,Selected_Line) -- validate again?
      --  Value_Changed = Value_Changed + 1
      end
    elseif Phase == VALUE_CHANGING_WAIT then
        DSM_Send(0x1A,0x06,0x10,Selected_Line)
    end
    multiBuffer(10,0x00);
    Retry = 50
  elseif multiBuffer(10) == 0x09 then
    -- Answer received
    --if multiBuffer(11) == 0x00 then -- waiting for commands?
    if multiBuffer(11) == 0x01 then -- read version
      RX_Version = multiBuffer(14).."."..multiBuffer(15).."."..multiBuffer(16)
      Phase = MENU_TITLE
    elseif multiBuffer(11) == 0x02 then -- read menu title
      Menu_Title = Text[multiBuffer(14)+multiBuffer(15)*256]
      if Menu_Title == nil then -- missing text...
        Menu_Title = "Unknown_"..string.format("%X",multiBuffer(14)+multiBuffer(15)*256)
      end
      for line = 0, 6 do  -- clear menu
        Menu_Line[line] = ""
        Next_Menu[line] = nil
        Menu_Type[line] = nil
      end
      Menu_Edit = nil
      Blink = 0
      Prev_Menu = multiBuffer(20)
      Cur_Line = -1
      Selected_Line = -1
      Phase = MENU_LINES
    elseif multiBuffer(11) == 0x03 then -- read menu lines
      Cur_Line = multiBuffer(14)
      Menu_Type[Cur_Line] = multiBuffer(15) -- not quite sure yet: 1C is text menu only, 4C/6C is text menu followed by text list, C0 is text menu followed by percentage value
      Menu_Line[Cur_Line] = Text[multiBuffer(16)+multiBuffer(17)*256]
      if Menu_Line[Cur_Line] == nil then -- missing text...
        Menu_Line[Cur_Line] = "Unknown_"..string.format("%X",multiBuffer(16)+multiBuffer(17)*256)
      end
      Next_Menu[Cur_Line] = multiBuffer(18)
      if Selected_Line == -1 and Next_Menu[Cur_Line] ~= Cur_Menu then -- Auto select first line of the menu
        Selected_Line = multiBuffer(14)
      end
      Menu_Value_Min[Cur_Line] = conv_int16(multiBuffer(20)+multiBuffer(21)*256)
      Menu_Value_Max[Cur_Line] = conv_int16(multiBuffer(22)+multiBuffer(23)*256)
      Menu_Value_Def[Cur_Line] = conv_int16(multiBuffer(24)+multiBuffer(25)*256)
      if Menu_Type[Cur_Line] == 0x1C then
        -- nothing to do on menu entries
      elseif Menu_Type[Cur_Line] == LIST_MENU_NOCHANGING or Menu_Type[Cur_Line] == LIST_MENU2 then
        Menu_Value[Cur_Line] = Menu_Value_Def[Cur_Line] - Menu_Value_Min[Cur_Line] -- use default value not sure if needed
        Menu_Value_Def[Cur_Line] = Menu_Value_Min[Cur_Line] -- pointer to the start of the list in Text
        Menu_Value_Max[Cur_Line] = Menu_Value_Max[Cur_Line] - Menu_Value_Min[Cur_Line] -- max index
        Menu_Value_Min[Cur_Line] = 0 -- min index
      else -- default to numerical value
        Menu_Value[Cur_Line] = Menu_Value_Def[Cur_Line] -- use default value not sure if needed
      end
      if Menu_Type[Cur_Line] ~= 0x1C then -- value to follow
        Menu_Line[Cur_Line] = Menu_Line[Cur_Line]..":"
      end
      Phase = MENU_LINES
    elseif multiBuffer(11) == 0x04 then -- read menu values
      Cur_Line = multiBuffer(14)
      Menu_Value[Cur_Line] = conv_int16(multiBuffer(16)+multiBuffer(17)*256)
      Phase = MENU_VALUES
    elseif multiBuffer(11) == 0x05 then -- unknown... need to get through the lines...
      Cur_Line = 0x80 + multiBuffer(12)
      Phase = MENU_LINES
    elseif multiBuffer(11) == 0x00 and Phase == VALUE_CHANGING then
      Phase = VALUE_CHANGING_WAIT
    end
    -- Data processed
    Waiting_RX = 0
    multiBuffer(10,0x00)
    Retry = 50
  else
    Retry = Retry - 1
    if Retry <= 0 then
      -- Retry the RX request
      Retry = 50
      Waiting_RX = 0
      if Phase ~= RX_VERSION and Phase ~= VALUE_CHANGING_WAIT then
        Phase = WAIT_CMD
      end
    end
  end
end

local function DSM_Display()
  local line
  
  lcd.clear()
  if LCD_W == 480 then
    --Draw title
    lcd.drawFilledRectangle(0, 0, LCD_W, 30, TITLE_BGCOLOR)
    lcd.drawText(1, 5, "DSM Forward Programming", MENU_TITLE_COLOR)
    --Draw RX Menu
    if Phase == RX_VERSION then
      lcd.drawText(10,50,"No compatible DSM RX...", BLINK)
    else
      local attrib=0;
      lcd.drawText(50,32,Menu_Title)
      for line = 0, 6 do
        if line == Selected_Line then
          attrib = INVERS
        else
          attrib = 0
        end
        if Menu_Type[line] ~= nil and Menu_Type[line] ~= MENU then
          local text=""
          if Menu_Type[line] == LIST_MENU_NOCHANGING or Menu_Type[line] == LIST_MENU2 then
            text = Text[Menu_Value[line]+Menu_Value_Def[line]]
            if text == nil then
              text = "Unknown_"..Menu_Value[line]+Menu_Value_Def[line]
            end
          elseif Menu_Type[line] == PERCENTAGE_VALUE then
            text = Menu_Value[line].." %"
          else
            text = Menu_Value[line]
          end
          if Menu_Edit == Selected_Line then
            Blink = Blink + 1
            if Blink > 25 then
              attrib = 0
              if Blink > 50 then
                Blink = 0
              end
            end
          end
          lcd.drawText(240,32+20*(line+2),text,attrib)
          attrib = 0
        end
        lcd.drawText(10,32+20*(line+2),Menu_Line[line], attrib)
      end
      lcd.drawText(10,252,"RX v"..RX_Version)
    end
  else
    -- --Draw RX Menu on LCD_W=128
    -- if multiBuffer( 4 ) == 0xFF then
      -- lcd.drawText(2,17,"No compatible DSM RX...",SMLSIZE)
    -- else
      -- if Retry_128 ~= 0 then
        -- --Intro page
        -- Retry_128 = Retry_128 - 1
        -- lcd.drawScreenTitle("DSM Forward Programming",0,0)
        -- lcd.drawText(2,17,"Press Prev Page for previous Menu" ,SMLSIZE)
      -- else
        -- --Menu page
        -- for line = 0, 7, 1 do
          -- for i = 0, 21-1, 1 do
            -- value=multiBuffer( line*21+6+i )
            -- if value > 0x80 then
              -- value = value - 0x80
              -- lcd.drawText(2+i*6,1+8*line,string.char(value).." ",SMLSIZE+INVERS)
            -- else
              -- lcd.drawText(2+i*6,1+8*line,string.char(value),SMLSIZE)
            -- end
          -- end
        -- end
      -- end
    -- end
  end
end

-- Init
local function DSM_Init()
  --Set protocol to talk to
  multiBuffer( 0, string.byte('D') )
  --test if value has been written
  if multiBuffer( 0 ) ~=  string.byte('D') then
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
  --Text to be displayed -> need to use a file instead?
  Text[0x0036]="Throttle"
  Text[0x0037]="Aileron"
  Text[0x0038]="Elevator"
  Text[0x0039]="Rudder"
  Text[0x003A]="Gear"
  for i=1,7 do -- 3B..41
    Text[0x003A+i]="Aux"..i
  end
  for i=1,8 do -- 41..49
    Text[0x0041+i]="XPlus-"..i
  end
  Text[0x004A]="Failsafe"
  Text[0x004B]="Main Menu"
  Text[0x004E]="Position"
  Text[0x0050]="Outputs"
  Text[0x005F]="Hold Last"
  Text[0x0060]="Preset"
  Text[0x0090]="Apply"
  Text[0x0093]="Complete"
  Text[0x0094]="Done"
  Text[0x0097]="Factory Reset"
  Text[0x009A]="Capture Failsafe Positions"
  Text[0x009C]="Custom Failsafe"
  Text[0x00A5]="First Time Setup"
  Text[0x00F9]="Gyro settings"
  Text[0x0100]="Make sure the model has been"
  Text[0x0101]="configured, including wing type,"
  Text[0x0102]="reversing, travel, trimmed, etc."
  Text[0x0103]="before continuing setup."
  Text[0x0104]=""
  Text[0x0105]=""

  Text[0x019C]="Enter Receiver Bind Mode"
  Text[0x020A]="Restore from Backup"
  Text[0x0209]="Save to Backup"
  Text[0x0227]="Other settings"
  Text[0x022B]="WARNING!"
  Text[0x022C]="This will reset the"
  Text[0x022D]="configuration to factory"
  Text[0x022E]="defaults. This does not"
  Text[0x022F]="affect the backup config."
  Text[0x0230]=""
  Text[0x0231]="This will overwrite the"
  Text[0x0232]="backup memory with your"
  Text[0x0233]="current configuartion."
  Text[0x0234]=""
  Text[0x0235]=""
  Text[0x0236]="This will overwrite the"
  Text[0x0237]="current config with"
  Text[0x0238]="that which is in"
  Text[0x0239]="the backup memory."
  Text[0x023A]=""
  
  --Lines to be displayed
  for line = 0, 6 do  -- clear lines
    Menu_Line[line]=""
  end
end

-- Main
local function DSM_Run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif event == EVT_VIRTUAL_EXIT then
    DSM_Release()
    return 2
  else
    DSM_Menu(event)
    DSM_Send_Receive()
    DSM_Display()
    return 0
  end
end

return { init=DSM_Init, run=DSM_Run }
