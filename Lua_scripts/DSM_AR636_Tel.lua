local toolName = "TNS|DSM AR636 Telemetry|TNE"
---- #########################################################################                                                                  #
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
-- Developer: Francisco Arzu
-- Original idea taken from DsmPID.lua.. don't know who is the author 
-- 

local DEBUG_ON = false
--

local TEXT_SIZE             = 0 -- NORMAL
local X_COL1_HEADER         = 6
local X_COL1_DATA           = 60
local X_COL2_HEADER         = 170
local X_COL2_DATA           = 220
local Y_LINE_HEIGHT         = 20
local Y_HEADER              = 0
local Y_DATA                = Y_HEADER + Y_LINE_HEIGHT*2 
local X_DATA_LEN            = 80 
local X_DATA_SPACE          = 5


local function getPage(iParam)
  -- get page from 0-based index
  -- {0,1,2,3}: cyclic (1), {4,5,6,7}: tail (2)
  local res = (math.floor(iParam/4)==0) and 0 or 1
  return res
end

local function round(v)
  -- round float
  local factor = 100
  return math.floor(v * factor + 0.5) / factor
end


local function readValue(sensor)
  -- read from sensor, round and return
  local v = getValue(sensor)
  --v = round(v)
  return v
end

local function readValueById(sensor)
  local i = getFieldInfo(sensor)
  if (i==nil) then return nil end 

  local v = getValue(i.id)
  return v
end



local function readBatValue(sensor)
  -- read from sensor, round and return
  local v = getValue(sensor)
  if (v==nil) then return "--" end

  return string.format("%2.2f",v)
end
 
local function readActiveParamValue(sensor)
  --  read and return a validated active parameter value
  local v = getValue(sensor)
  if (v<1 or v>8) then
    return -1
  end
  return v
end
   

local function drawPIDScreen()
  -- draw labels and params on screen

  local pageId = getValue("FLss")
  
  lcd.clear()
  -- if active gain does not validate then assume
  -- Gain Adjustment Mode is disabled
  if not (pageId==4401 or pageId==4402) then
    lcd.drawText(0,0,"BLADE Gain Adjustment", TEXT_SIZE +INVERS)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*1,"Enter Gain Adjustment Mode",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*2,"Stk: Low/R + Low/R + Panic (3 sec)",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*4,"Op: Right Stk:  Up/Down to select, Left/Right change value",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*5,"Panic to exit",TEXT_SIZE)
    return
  end

  local activePage = (pageId % 100)-1  --Last 2 digits, make it zero base 

  lcd.drawText (X_COL1_HEADER, Y_HEADER, "Cyclic (0-200)", TEXT_SIZE + INVERS)
  lcd.drawText (X_COL2_HEADER, Y_HEADER, "Tail (0-200)", TEXT_SIZE + INVERS)



  local p = readValue("FdeA")
  local i = readValue("FdeB")
  local d = readValue("FdeL")
  local r = readValue("FdeR")

  local titles = {[0]="P:", "I:", "D:", "Resp:", "P:","I:","D:", "Filt:"}
  local values = {[0]=p,i,d,r,p,i,d,r}

  local activeParam =  readActiveParamValue("Hold")-1

  for iParam=0,7 do
    -- highlight selected parameter
    local attr = (activeParam==iParam) and INVERS or 0
    -- circular index (per page)
    local perPageIndx = (iParam % 4)
    
    -- set y draw coord
    local y = (perPageIndx+1)*Y_LINE_HEIGHT+Y_DATA
    
    -- check if displaying cyclic params.
    local isCyclicPage = (getPage(iParam)==0)

    -- labels
    local x = isCyclicPage and X_COL1_HEADER or X_COL2_HEADER
    -- labels are P,I,D for both pages except for last param
    local val = titles[iParam]
    lcd.drawText (x, y, val, TEXT_SIZE)
    
    -- gains
    -- set all params for non-active page to '--' rather than 'last value'
    val = (getPage(iParam)==activePage) and values[iParam] or '--'
    x = isCyclicPage and X_COL1_DATA or X_COL2_DATA

    if (val~=16384) then  -- Active value
      lcd.drawText (x, y, val, attr + TEXT_SIZE)
    end
  end
end


local function drawFlightLogScreen()
  -- draw labels and params on screen
  local h = getValue("Hold")
  local activeParam = h-1 -- H 
  
  lcd.clear()
  lcd.drawText (X_COL1_HEADER, Y_HEADER, "Flight Log", TEXT_SIZE + INVERS)

   -- read and return parameters
   local a = getValue("FdeA")
   local b = getValue("FdeB")
   local l = getValue("FdeL")
   local r = getValue("FdeR")
   local f = getValue("FLss")
 
   local titles  = {[0]="A:", "B:", "L:", "R:", "F:", "H:"}
   local values  = {[0]=a,b,l,r,f,h}

   local y = Y_LINE_HEIGHT+Y_DATA
   
   for iParam=0,3 do   -- A,B,L,R 
    -- highlight selected parameter  (rund)
    local attr = ((activeParam%4)==iParam) and INVERS or 0
    -- labels
    local x = X_COL1_HEADER
    local val = titles[iParam]
    lcd.drawText (x, y, val, TEXT_SIZE)
    
    -- Values
    val = values[iParam] 
    x = X_COL1_DATA + X_DATA_LEN
    if (val~=16384) then  -- Active value
        lcd.drawText (x, y, val, attr + TEXT_SIZE + RIGHT)
    end

    y = y + Y_LINE_HEIGHT
  end

  y = Y_LINE_HEIGHT+Y_DATA
  for iParam=4,5 do  -- F, H
    -- labels
    local x = X_COL2_HEADER
    local val = titles[iParam]
    lcd.drawText (x, y, val, TEXT_SIZE + BOLD)
    
    -- Values
    val = values[iParam] 
    x = X_COL2_DATA + X_DATA_LEN
    lcd.drawText (x, y, val, TEXT_SIZE + RIGHT + BOLD)

    y = y + Y_LINE_HEIGHT
  end

  -- Bat 
  y = y + Y_LINE_HEIGHT
  local bat = readBatValue("A2") or "--"
  lcd.drawText (X_COL2_HEADER, y, "Bat:", TEXT_SIZE)
  lcd.drawText (X_COL2_DATA + X_DATA_LEN, y, bat, TEXT_SIZE + RIGHT)
  lcd.drawText (X_COL2_DATA + X_DATA_LEN + X_DATA_SPACE, y, "v", TEXT_SIZE)

end

local function servoAdjustScreen()
  -- draw labels and params on screen
  local pageId = getValue("FLss") -- FLss  
  local activeParam = getValue("Hold")-1 -- Hold 
  
  lcd.clear()
  lcd.drawText (0, Y_HEADER, "BLADE Servo SubTrim", TEXT_SIZE + INVERS)

  if pageId~=1234 then
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*1,"Enter Servo Adjustment Mode",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*2,"Stk: Low/L + Low/R + Panic (3 sec)",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*4,"Op: R Stk:  Up/Down to select, Left/Right change value",TEXT_SIZE)
    lcd.drawText(X_COL1_HEADER,Y_LINE_HEIGHT*5,"Panic to exit",TEXT_SIZE)
    return
  end

  local a = getValue("FdeA")
  local b = getValue("FdeB")
  local l = getValue("FdeL")

  local titles  = {[0]="Servo1:", "Servo2:", "Servo3:"}
  local values  = {[0]=a,b,l}

  for iParam=0,#values do   -- S1,S2,S3 
    -- highlight selected parameter
    local attr = (activeParam==iParam) and INVERS or 0

    -- set y draw coord
    local y = (iParam+1)*Y_LINE_HEIGHT+Y_HEADER
    
    -- labels
    local x = X_COL1_HEADER
    local val = titles[iParam]
    lcd.drawText (x, y, val, TEXT_SIZE)
    
    val = values[iParam] 
    x = X_COL1_DATA
    if (val~=16384) then  -- Active value
      lcd.drawText (x, y, val, attr + TEXT_SIZE)
    end
  end
end

local function Unsigned_to_SInt16(value) 
  if value >= 0x8000 then  -- Negative value??
      return value - 0x10000
  end
  return value
end

local function getDegreesValue(sensor)
  local i = getFieldInfo(sensor)
  if (i==nil) then return "-unk-" end 

  local v = getValue(i.id)
  if v==nil then return "---" end
  local vs = Unsigned_to_SInt16(v)

  return string.format("%0.1f o",vs/10)
end


local function getDecHexValue(sensor)
  local i = getFieldInfo(sensor)
  if (i==nil) then return "-unk-" end 

  local v = getValue(i.id)
  if v==nil then return "---" end
  local vs = Unsigned_to_SInt16(v)

  return string.format("%d (0x%04X)",vs,v)
end




local function drawVersionScreen()
  local paramV  =  getValue("FdeA")
  local B       =  getValue("FdeB")
  local rxId    =  getValue("FdeL")
  local firmware = getValue("FLss")
  local prodId  =  getValue("Hold")
  local bat     =  readBatValue("A2")
  
  lcd.clear()
  lcd.drawText (0, Y_HEADER, "BLADE Version", TEXT_SIZE + INVERS)

  --Product ID
  local val = "ID_".. prodId

  if (prodId==243) then val = "Blade 230 V1"
  elseif (prodId==250) then val = "Blade 230 V2 (not Smart)" 
  elseif (prodId==149) then val = "Blade 250 CFX" 
  end

  local y = Y_DATA
  local x_data1 = X_COL1_DATA+X_DATA_LEN
  lcd.drawText (X_COL1_HEADER, y, "Prod:", TEXT_SIZE)
  lcd.drawText (x_data1, y, val,  TEXT_SIZE)

  -- RX
  val = "ID_"..rxId
  if (rxId==1) then val = "AR636"
  end

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER, y, "RX:", TEXT_SIZE)
  lcd.drawText (x_data1, y, val,  TEXT_SIZE)

  -- Firmware
  val = string.format("%0.2f",firmware/100)
  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER, y, "Firmware:", TEXT_SIZE)
  lcd.drawText (x_data1, y, val,  TEXT_SIZE)

  -- ParamV
  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER, y, "Params:", TEXT_SIZE)
  lcd.drawText (x_data1, y, paramV,  TEXT_SIZE)

  -- Bat
  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER, y, "Bat:", TEXT_SIZE)
  lcd.drawText (x_data1, y, bat,  TEXT_SIZE)

  y = y + Y_LINE_HEIGHT
  lcd.drawText(X_COL1_HEADER,y,"Press Panic for 3s",TEXT_SIZE)

  y = y + Y_LINE_HEIGHT
  lcd.drawText(X_COL1_HEADER,y,"Usually Panic is Ch7 on a switch and Revesed",TEXT_SIZE)

end

local function parseFlightMode(v)
  -- FlightMode   (Hex:  MMSGG)  MM=Flight Mode, S=Status (0= off, 1=init, 2=Hold, 3=Running) GG=???
  if v==nil then return "---" end
  local fm = bit32.rshift(v, 12)
  local status = bit32.band(bit32.rshift(v, 8),0xF)

  local res = " "..fm.."  "

  if (fm==0) then res = res .. " NORMAL" 
  elseif (fm==1) then res = res .. " INTERMEDIATE" 
  elseif (fm==2) then res = res .. " ADVANCED" 
  elseif (fm==5) then res = res .. " PANIC" 
  end

  if (status==2) then res=res .. "    HOLD" end

  if (DEBUG_ON) then
    res = res .. string.format("    (0x%04X)",v)
  end

  return res
end


local function drawAlpha6Monitor()
  lcd.clear()

  local RxStatus  = readValueById("2402")   -- FlightMode   (Hex:  MMSGG)  MM=Flight Mode, S=Status (0=init, 2=Ready, 3=Sensor Fault) GG=???
  
  local ARoll   = getDegreesValue("2406") --Att Roll
  local APitch  = getDegreesValue("2408") --Att Pitch
  local AYaw    = getDegreesValue("240B") --Att Yaw


  lcd.drawText (0,0, "BLADE Alpha6 Monitor", TEXT_SIZE+INVERS)

  local y = Y_DATA
  local x_data1 = X_COL1_DATA+X_DATA_LEN
  local x_data2 = X_COL1_DATA+X_DATA_LEN*2
  local x_data3 = X_COL1_DATA+X_DATA_LEN*3

  -- Flight Mode
  lcd.drawText (0,y, "F-Mode:"..parseFlightMode(RxStatus), TEXT_SIZE)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (x_data1,y, "Attitude", TEXT_SIZE+BOLD + RIGHT)
  lcd.drawText (x_data2,y, "Gyro", TEXT_SIZE+BOLD + RIGHT)
  lcd.drawText (x_data3,y, "Gain", TEXT_SIZE+BOLD + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Rol:", TEXT_SIZE)
  lcd.drawText (x_data1,y, ARoll, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, "-", TEXT_SIZE  + RIGHT)
  lcd.drawText (x_data3,y, "-", TEXT_SIZE + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Pitch:", TEXT_SIZE)
  lcd.drawText (x_data1,y, APitch, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, "-", TEXT_SIZE + RIGHT)
  lcd.drawText (x_data3,y, "-", TEXT_SIZE + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Yaw:", TEXT_SIZE)
  lcd.drawText (x_data1,y, AYaw, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, "-", TEXT_SIZE + RIGHT)
  lcd.drawText (x_data3,y, "-", TEXT_SIZE + RIGHT)

  y = y + Y_LINE_HEIGHT + Y_LINE_HEIGHT
  lcd.drawText (0,y, "Bat: "..readBatValue("A2").." v", TEXT_SIZE)


  -- Debug Values 
  if (DEBUG_ON) then  
    local s2400 = getDecHexValue("2400")
    local s2402 = getDecHexValue("2402")
    local s2404 = getDecHexValue("2404")

    local s240D = getDecHexValue("240D")
    
    local s1G00 = getDecHexValue("1G00")
    local s1G02 = getDecHexValue("1G02")
    local s1G04 = getDecHexValue("1G04")
    local s1G06 = getDecHexValue("1G06")
    local s1G08 = getDecHexValue("1G08")
    local s1G0B = getDecHexValue("1G0B")
    local s1G0D = getDecHexValue("1G0D")
  
    local titles = {[0]=
    "2400","2402/FM-S-?",
    "2404","240D",
    "1G00","1G02","1G04",
    "1G06","1G08","1G0B","1G0D"}

    local values = {[0]=
                s2400,s2402,s2404,s240D,
                s1G00,s1G02,s1G04,
                s1G06,s1G08,s1G0B,s1G0D}


    -- draw labels and params on screen

      y = Y_LINE_HEIGHT*2 + Y_HEADER
      for iParam=0,#titles do   -- ??
        -- labels
        local x = X_COL1_HEADER+220
        local val = titles[iParam]
        lcd.drawText (x, y, val, TEXT_SIZE)
        
        val = values[iParam] 
        x = X_COL1_DATA+250
        lcd.drawText (x, y, val, TEXT_SIZE)

        y = y + Y_LINE_HEIGHT
      end
  end 
end

local function readAlpha3arameters()
 
end




local function drawAS3XMonitor()
  lcd.clear()
  local s1G00 = getDecHexValue("1G00")
  local s1G02 = getDecHexValue("1G02")
  local s1G04 = getDecHexValue("1G04")
  local s1G06 = getDecHexValue("1G06")
  local s1G08 = getDecHexValue("1G08")
  local s1G0B = getDecHexValue("1G0B")
  local s1G0D = getDecHexValue("1G0D")

  local s6C00 = getDecHexValue("6C00")
  local s6C02 = getDecHexValue("6C02")
  local s6C04 = getDecHexValue("6C04")
  
  

  local RRoll  = bit32.rshift(getValue("1G00") or 0,8)
  local RPitch = bit32.band(getValue("1G00") or 0,0xFF)
  local RYaw   = bit32.rshift(getValue("1G02") or 0,8)

  local HRoll  = bit32.band(getValue("1G02") or 0,0xFF)
  local HPitch = bit32.rshift(getValue("1G04") or 0,8)
  local HYaw   = bit32.band(getValue("1G04") or 0,0xFF)

  local ARoll  = bit32.rshift(getValue("1G06") or 0,8)
  local APitch = bit32.band(getValue("1G06") or 0,0xFF)
  local AYaw   = bit32.rshift(getValue("1G08") or 0,8)


  lcd.drawText (0,0, "Plane AR636 AS3X Gains", TEXT_SIZE+INVERS)

  local y = Y_DATA
  local x_data1 = X_COL1_DATA+X_DATA_LEN
  local x_data2 = X_COL1_DATA+X_DATA_LEN*2
  local x_data3 = X_COL1_DATA+X_DATA_LEN*3.1

  -- Flight Mode
  --lcd.drawText (0,y, "F-Mode:   "..(nil or "--"), TEXT_SIZE)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (x_data1,y, "Rate", TEXT_SIZE+BOLD + RIGHT)
  lcd.drawText (x_data2,y, "Head", TEXT_SIZE+BOLD + RIGHT)
  lcd.drawText (x_data3+X_DATA_SPACE*3,y, "Actual", TEXT_SIZE+BOLD + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Roll %:", TEXT_SIZE)
  lcd.drawText (x_data1,y, RRoll, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, HRoll, TEXT_SIZE  + RIGHT)
  lcd.drawText (x_data3,y, ARoll, TEXT_SIZE + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Pitch %:", TEXT_SIZE)
  lcd.drawText (x_data1,y, RPitch, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, HPitch, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data3,y, APitch, TEXT_SIZE + RIGHT)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, "Yaw %:", TEXT_SIZE)
  lcd.drawText (x_data1,y, RYaw, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data2,y, HYaw, TEXT_SIZE + RIGHT)
  lcd.drawText (x_data3,y, AYaw, TEXT_SIZE + RIGHT)


  -- Debug Values 
  if (DEBUG_ON) then
      local Alpha3Tags = {[0]=
      "1G00/RA+RE","1G02/RY+HA","1G04R HP+HY","1G06/AR+AP","1G08/AY+?","1G0B","1G0D","6C00","6C02","6C04"}

      local params = {[0]=
      s1G00,s1G02,s1G04,s1G06,s1G08,s1G0B,s1G0D,s6C00,s6C02,s6C04 }

      y = Y_LINE_HEIGHT*2 + Y_HEADER
      for iParam=0,#Alpha3Tags do   -- ??
        -- labels
        local x = X_COL1_HEADER+220
        local val = Alpha3Tags[iParam]
        lcd.drawText (x, y, val, TEXT_SIZE)
        
        val = params[iParam] 
        x = X_COL1_DATA+250
        lcd.drawText (x, y, val, TEXT_SIZE)

        y = y + Y_LINE_HEIGHT
      end
  end 
end

local function openTelemetryRaw(i2cId)
  --Init telemetry  (Spectrun Telemetry Raw STR)
  multiBuffer( 0, string.byte('S') )
  multiBuffer( 1, string.byte('T') )
  multiBuffer( 2, string.byte('R') ) 
  multiBuffer( 3, i2cId ) -- Monitor this teemetry data
  multiBuffer( 4, 0 ) -- Allow to get Data
end

local function closeTelemetryRaw()
  multiBuffer(0, 0) -- Destroy the STR header 
  multiBuffer(3, 0) -- Not requesting any Telementry ID
end

local lineText = {nil}
local I2C_TEXT_GEN = 0x0C

local function drawTextGen(event)
  if (multiBuffer(0)~=string.byte('S')) then -- First time run???
    openTelemetryRaw(I2C_TEXT_GEN) -- I2C_ID for TEXT_GEN
    lineText = {nil}
  end

  -- Proces TEXT GEN Telementry message
  if multiBuffer( 4 ) == I2C_TEXT_GEN then -- Specktrum Telemetry ID of data received
    local instanceNo = multiBuffer( 5 )
    local lineNo = multiBuffer( 6 )
    local line = ""
    for i=0,13 do
      line = line .. string.char(multiBuffer( 7 + i ))
    end

    multiBuffer( 4, 0 ) -- Clear Semaphore, to notify that we fully process the current message
    lineText[lineNo]=line
  end

  lcd.clear()
  -- Header
  if (lineText[0]) then
    lcd.drawText (X_COL1_HEADER,0,  "   "..lineText[0].."   ", TEXT_SIZE + BOLD + INVERS)
  else
    lcd.drawText (X_COL1_HEADER,0, "TextGen", TEXT_SIZE+INVERS)
  end

  -- Menu lines
  local y = Y_DATA
  for i=1,8 do
    if (lineText[i]) then
      lcd.drawText (X_COL1_HEADER,y,  lineText[i], TEXT_SIZE)
    end
    y = y + Y_LINE_HEIGHT
  end

  if event == EVT_VIRTUAL_EXIT then -- Exit?? Clear menu data
    closeTelemetryRaw()
  end
end

local telPage = 1
local telPageSelected = 0
local pageTitle = {[0]="Main", "Blade Version", "Blade Servo Adjust","Blade Gyro Adjust", "Blade Alpha6 Monitor", "Plane AS3X Monitor", "TextGen", "Flight Log"}

local function drawMainScreen(event) 
  lcd.clear()
  lcd.drawText (X_COL1_HEADER, Y_HEADER, "Main Telemetry (AR636)", TEXT_SIZE + INVERS)

  for iParam=1,#pageTitle do    
    -- highlight selected parameter
    local attr = (telPage==iParam) and INVERS or 0

    -- set y draw coord
    local y = (iParam-1)*Y_LINE_HEIGHT+Y_DATA 
    
    -- labels
    local x = X_COL1_HEADER
    local val = pageTitle[iParam]
    lcd.drawText (x, y, val, attr + TEXT_SIZE)
  end

  if event == EVT_VIRTUAL_PREV then
    if (telPage>1) then telPage = telPage - 1 end
  elseif event == EVT_VIRTUAL_NEXT then
    if (telPage<#pageTitle) then telPage = telPage + 1 end
  elseif event == EVT_VIRTUAL_ENTER then
    telPageSelected = telPage
  end
end


local pageDraw  = {[0]=drawMainScreen, drawVersionScreen, servoAdjustScreen,drawPIDScreen, drawAlpha6Monitor,  drawAS3XMonitor, drawTextGen, drawFlightLogScreen}

local function run_func(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  end

  -- draw specific page 
  pageDraw[telPageSelected](event)

  if event == EVT_VIRTUAL_EXIT then
    if (telPageSelected==0) then return 1 end  -- on Main?? Exit Script 
    telPageSelected = 0  -- any page, return to Main 
  end

  return 0
end

local function init_func()
  
  if (LCD_W <= 128 or LCD_H <=64) then -- Smaller Screens 
    TEXT_SIZE = SMLSIZE 
    X_COL1_HEADER         = 0
    X_COL1_DATA           = 20

    X_COL2_HEADER         = 60
    X_COL2_DATA           = 90

    X_DATA_LEN            = 28 
    X_DATA_SPACE          = 1


    Y_LINE_HEIGHT         = 8
    Y_DATA                = Y_HEADER + Y_LINE_HEIGHT

  end
end

return { run=run_func,  init=init_func  }
