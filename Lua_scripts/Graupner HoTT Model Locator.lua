---- #########################################################################
---- #                                                                       #
---- # Telemetry Widget script for FrSky Horus/Radio Master TX16s            #
---- # Copyright (C) EdgeTX                                                  #
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

-- Model Locator by RSSI
-- Offer Shmuely (based on code from Scott Bauer 6/21/2015)
-- Date: 2021
-- ver: 0.1

-- MHA (based on code from Offer Shmuely)
-- Date: 2022
-- ver: 0.11
-- changes: made version for Graupner HoTT. Uses real Rssi data scaled -15db to -115db to 100..0

-- This widget help to find a lost/crashed model based on the RSSI (if still available)
-- The widget produce audio representation (variometer style) of the RSSI from the lost model
-- The widget also  display the RSSI in a visible colorized bar (0-100%)

-- There are two way to use it
-- 1. The simple way:
--    walk toward the quad/plane that crashed,
--    as you get closer to your model the beeps will become more frequent with higher pitch (and a visual bar graph as well)
--    until you get close enough to find it visually

-- 2. the more accurate way:
--    turn the antenna straight away (i.e. to point from you, straight away)
--    try to find the weakest signal! (not the highest), i.e. the lowest RSSI you can find, this is the direction to the model.
--    now walk to the side (not toward the model), find again the weakest signal, this is also the direction to your model
--    triangulate the two lines, and it will be :-)

local delayMillis = 100
local nextPlayTime = getTime()
local img = Bitmap.open("/SCRIPTS/TOOLS/Model Locator (by RSSI).png")
local colorLcd = type(lcd.RGB) == "function"
local screenMode = "mono"
if colorLcd then
  if (LCD_W or 0) >= 720 and (LCD_H or 0) >= 320 then
    screenMode = "mk3"
  elseif (LCD_W or 0) >= 320 and (LCD_H or 0) >= 200 then
    screenMode = "color"
  end
end

--------------------------------------------------------------
local function log(s)
  --return;
  print("locator: " .. s)
end
--------------------------------------------------------------

-- init_func is called once when model is loaded
local function init()
  return 0
end

-- bg_func is called periodically when screen is not visible
local function bg()
  return 0
end

-- This function returns green at gvalue, red at rvalue and graduate in between
local function getRangeColor(value, red_value, green_value)
  if not colorLcd then
    return 0
  end
  local range = math.abs(green_value - red_value)
  if range == 0 then
    return lcd.RGB(0, 0xdf, 0)
  end
  if value == nil then
    return lcd.RGB(0, 0xdf, 0)
  end

  if green_value > red_value then
    if value > green_value then
      return lcd.RGB(0, 0xdf, 0)
    end
    if value < red_value then
      return lcd.RGB(0xdf, 0, 0)
    end
    g = math.floor(0xdf * (value - red_value) / range)
    r = 0xdf - g
    return lcd.RGB(r, g, 0)
  else
    if value > green_value then
      return lcd.RGB(0, 0xdf, 0)
    end
    if value < red_value then
      return lcd.RGB(0xdf, 0, 0)
    end
    r = math.floor(0xdf * (value - green_value) / range)
    g = 0xdf - r
    return lcd.RGB(r, g, 0)
  end
end

local function main(event)

  lcd.clear()

  local width = LCD_W or 480
  local height = LCD_H or 272
  local layout = screenMode == "mk3" and {
    valueX = math.floor(width * 0.40), valueY = 42,
    labelX = math.floor(width * 0.61), labelY = 92
  } or {
    valueX = 180, valueY = 30, labelX = 275, labelY = 73
  }
  local barBottom = height - 2
  local barHeight = math.floor(height * 0.26)
  local barStep = math.max(10, math.floor(width / 24))
  local barWidth = math.max(6, barStep - 5)
  
	-- fetch uplink rssi (HoTT sensor Rssi)
	local rssi = getValue("Rssi")

	-- calculate a percentage for the color bar (range -115db to -15db) 
	local rssiP = rssi							

	if(rssi ~= 0) then							-- rssi < 0 -> telemtry data received
		if(rssi >= -15) then					-- -15db to -1db => 100%
			rssiP = 100
		else
			if(rssi >=  -115) then			-- between -115db and -15db => 0% to 100%				
				rssiP = 100+rssi+15				
			else
				rssiP = 0									-- less than -115 => 0%
			end
		end
	end
	
  lcd.drawBitmap(img, math.floor(width * 0.52), math.floor(height * 0.18), 40)

  -- Title
  if colorLcd then
    lcd.drawFilledRectangle(0, 0, width, 30, COLOR_THEME_SECONDARY1)
    lcd.drawText(3, 5, "Graupner HoTT Rssi Model Locator", COLOR_THEME_PRIMARY2)
  else
    lcd.drawText(3, 5, "Graupner HoTT Rssi Model Locator", 0)
  end
  myColor = getRangeColor(rssi, 0, 100)
  if colorLcd then
    lcd.setColor(CUSTOM_COLOR, myColor)
  end

  -- draw current value
	local dx = 0
	if rssi < -99 then
		dx = -33
	end
	
	if rssi == 0 then	
  lcd.drawText(math.floor(width * 0.24), layout.labelY, "no telemetry", DBLSIZE + (colorLcd and CUSTOM_COLOR or 0))
	else
  lcd.drawNumber(layout.valueX+dx, layout.valueY, rssi, XXLSIZE + (colorLcd and CUSTOM_COLOR or 0))
  lcd.drawText(layout.labelX, layout.labelY, "db", colorLcd and CUSTOM_COLOR or 0)
	end

  -- draw main bar
  if colorLcd then
    lcd.setColor(CUSTOM_COLOR, YELLOW) -- RED / YELLOW
  end
  local xMin = 0
  local yMin = barBottom
  local xMax = width
  local h = 0
  local rssiAsX = (rssiP * xMax) / 100

  for xx = xMin, rssiAsX, barStep do
    if colorLcd then
      lcd.setColor(CUSTOM_COLOR, getRangeColor(xx, xMin, xMax - barStep * 2))
    end
    h = h + math.max(4, math.floor(barHeight / math.max(1, math.floor(xMax / barStep))))
    lcd.drawFilledRectangle(xx, yMin - h, barWidth, h, colorLcd and CUSTOM_COLOR or 0)
  end

  -- beep
  if getTime() >= nextPlayTime then
    playFile("/SCRIPTS/TOOLS/Model Locator (by RSSI).wav")
    nextPlayTime = getTime() + delayMillis - rssiP
  end

  return 0
end

return {init = init,run = main,background = bg}

