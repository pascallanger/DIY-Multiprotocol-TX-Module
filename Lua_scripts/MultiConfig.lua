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
-- Multi buffer for Config description
-- To start operation:
--   Write 0xFF at address 4 will request the buffer to be cleared
--   Write "Conf" at address 0..3
-- Read
--   Read at address 12 gives the current config page
--   Read at address 13..172 gives the current data of the page = 8 lines * 20 caracters
-- Write
--   Write at address 5..11 the command
--   Write 0x01 at address 4 will send the command to the module
--###############################################################################

local function Config_Release()
end

local function Config_Page( event )
end

local function Config_Menu( event )
end

local function Config_Draw_LCD()
  local i
  local value
  local line
  local result
  local offset=0
  
  lcd.clear()

  if LCD_W == 480 then
    --Draw title
    lcd.drawFilledRectangle(0, 0, LCD_W, 30, TITLE_BGCOLOR)
    if multiBuffer(13) == 0x00 then
      lcd.drawText(1, 5, "Multi Config", MENU_TITLE_COLOR)
      lcd.drawText(10,50,"No Config telemetry...", BLINK)
    else
      lcd.drawText(1, 5, "Multi Config v" .. string.char(multiBuffer(13)) .. "." .. string.char(multiBuffer(14)) .. "." .. string.char(multiBuffer(15)) .. "." .. string.char(multiBuffer(16)), MENU_TITLE_COLOR)
      --Draw RX Menu
      for line = 1, 7, 1 do
        for i = 0, 20-1, 1 do
          value=multiBuffer( line*20+13+i )
          if value > 0x00 and value < 0x80 then
            lcd.drawText(10+i*16,32+20*line,string.char(value))
            --lcd.drawText(10+i*16,32+20*line,string.char(value).."   ",INVERS)
          else
          end
        end
      end
    end
  else
    --Draw RX Menu on LCD_W=128
    -- if multiBuffer( 4 ) == 0xFF then
      -- lcd.drawText(2,17,"No Config telemetry...",SMLSIZE)
    -- else
      -- if Timer_128 ~= 0 then
        --Intro page
        -- Timer_128 = Timer_128 - 1
        -- lcd.drawScreenTitle("Graupner Hott",0,0)
        -- lcd.drawText(2,17,"Configuration of RX" .. sensor_name[Config_Sensor+1] ,SMLSIZE)
        -- lcd.drawText(2,37,"Press menu to cycle Sensors" ,SMLSIZE)
      -- else
        --Menu page
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
local function Config_Init()
  --Set protocol to talk to
  multiBuffer( 0, string.byte('C') )
  --test if value has been written
  if multiBuffer( 0 ) ~=  string.byte('C') then
    error("Not enough memory!")
    return 2
  end
  --Request init of the buffer
  multiBuffer( 4, 0xFF )
  --Continue buffer init
  multiBuffer( 1, string.byte('o') )
  multiBuffer( 2, string.byte('n') )
  multiBuffer( 3, string.byte('f') )
end

-- Main
local function Config_Run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif event == EVT_VIRTUAL_EXIT then
    Config_Release()
    return 2
  else
    if event == EVT_VIRTUAL_PREV_PAGE then
      killEvents(event)
      Config_Page( event )
    elseif event == EVT_VIRTUAL_ENTER then
      Config_Menu( event )
    elseif event == EVT_VIRTUAL_PREV then
      Config_Menu( event )
    elseif event == EVT_VIRTUAL_NEXT then
      Config_Menu( event )
    elseif event == EVT_VIRTUAL_NEXT_PAGE then
      Config_Page( event )
    end
    Config_Draw_LCD()
    return 0
  end
end

return { init=Config_Init, run=Config_Run }
