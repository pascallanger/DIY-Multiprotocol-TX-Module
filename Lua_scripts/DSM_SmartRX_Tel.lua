local toolName = "TNS|DSM Smart RX Telemetry|TNE"
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
  if (v==nil) then return v end

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
   


local function drawFlightLogScreen(event)
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

local as3xData = {[0]=0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local function drawAS3XSettings(event, page)
  local s0500 = getDecHexValue("0500")
  local s0502 = getDecHexValue("0502")
  local s0504 = getDecHexValue("0504")
  local s0506 = getDecHexValue("0506")
  local s0508 = getDecHexValue("0508")
  local s050B = getDecHexValue("050B")
  local s050D = getDecHexValue("050D")

  local d0500 = readValueById("0500") or 0
  local flags = bit32.rshift(d0500,8)
  local state = bit32.band(d0500,0xFF)

  local flagsMsg=""
  -- flags bits:  Safe Envelop, ?, Angle Demand, Stab 
  if (bit32.band(flags,0x1)~=0) then flagsMsg=flagsMsg.."AS3X Stab" end
  -- This one, only one should show
  if (bit32.band(flags,0x2)~=0) then flagsMsg=flagsMsg..", Angle Demand" 
  elseif (bit32.band(flags,0x8)~=0) then flagsMsg=flagsMsg..", Safe Envelope" 
  elseif (bit32.band(flags,0x4)~=0) then flagsMsg=flagsMsg..", AS3X Heading" end

  local d0502 = readValueById("0502") or 0  -- 0x?F?S 
  local fm =  bit32.band(bit32.rshift(d0502,8),0xF)  -- 0,1,2

  local axis =  bit32.band(d0502,0xF)  -- 0=Gains,1=Headings,2=Angle Limits (cointinus iterating to provide all values)

  local d0504 = readValueById("0504") or 0  
  local d0506 = readValueById("0506") or 0  
  local d0508 = readValueById("0508") or 0  

  local d0 = bit32.rshift(d0504,8)
  local d1 = bit32.band(d0504,0xFF)
  local d2 = bit32.rshift(d0506,8)
  local d3 = bit32.band(d0506,0xFF)
  local d4 = bit32.rshift(d0508,8)
  local d5 = bit32.band(d0508,0xFF)

  --axis: 0=Gains+Headings (RG,PG,YG,RH,PH,YH), 1=Safe Gains (R,P,Y),2=Angle Limits(L,R,U,D) 
  --Constantly changing from 0..2 to represent different data, thats why we have to store the values
  --in a script/global variable, and not local to the function
  local s = axis*6   
  as3xData[s+0] = d0
  as3xData[s+1] = d1
  as3xData[s+2] = d2
  as3xData[s+3] = d3
  as3xData[s+4] = d4
  as3xData[s+5] = d5


  lcd.clear()
  lcd.drawText (0,0, "AS3X/SAFE Settings", TEXT_SIZE + INVERS)

  local y = Y_DATA
  -- Flight Mode
  lcd.drawText (X_COL1_HEADER,y, "FM: "..(fm+1), TEXT_SIZE)
  lcd.drawText (X_COL1_DATA+X_DATA_LEN*0.3,y, "Flags: "..flags, TEXT_SIZE)
  lcd.drawText (X_COL2_HEADER+X_DATA_LEN*0.3,y, "State: "..state, TEXT_SIZE)

  y = y + Y_LINE_HEIGHT
  lcd.drawText (X_COL1_HEADER,y, flagsMsg, TEXT_SIZE)

  y = y + Y_LINE_HEIGHT

  if (page==1) then
    lcd.drawText (X_COL1_HEADER+X_DATA_LEN*0.3,y, "AS3X Gains", TEXT_SIZE+BOLD)
    lcd.drawText (X_COL2_HEADER+X_DATA_LEN*0.3,y, "AS3X Headings", TEXT_SIZE+BOLD)
    
    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Roll:", TEXT_SIZE)
    lcd.drawText (X_COL1_DATA+X_DATA_LEN,y, as3xData[0], TEXT_SIZE + RIGHT) -- Roll G 
    lcd.drawText (X_COL2_DATA+X_DATA_LEN,y, as3xData[3], TEXT_SIZE + RIGHT) -- Roll H 

    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Pitch:", TEXT_SIZE)
    lcd.drawText (X_COL1_DATA+X_DATA_LEN,y,as3xData[1], TEXT_SIZE + RIGHT)  -- Pitch G
    lcd.drawText (X_COL2_DATA+X_DATA_LEN,y, as3xData[4], TEXT_SIZE + RIGHT) -- Pitch H 

    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Yaw:", TEXT_SIZE)
    lcd.drawText (X_COL1_DATA+X_DATA_LEN,y, as3xData[2], TEXT_SIZE + RIGHT) -- Yaw G
    lcd.drawText (X_COL2_DATA+X_DATA_LEN,y, as3xData[5], TEXT_SIZE + RIGHT) -- Yaw H
  end


  if (page==2) then
    local x_data1 = X_COL1_DATA+X_DATA_LEN
    local x_data2 = X_COL2_HEADER+X_DATA_LEN*1.6

    lcd.drawText (X_COL1_HEADER+X_DATA_LEN*0.3,y, "SAFE Gains", TEXT_SIZE+BOLD)
    lcd.drawText (X_COL2_HEADER+X_DATA_LEN*0.1,y, "Angle Limits", TEXT_SIZE+BOLD)
  
    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Roll:", TEXT_SIZE)
    lcd.drawText (x_data1,y, as3xData[6], TEXT_SIZE + RIGHT)

    lcd.drawText (X_COL2_HEADER,y, "Roll R:", TEXT_SIZE)
    lcd.drawText (x_data2,y, as3xData[12], TEXT_SIZE + RIGHT)


    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Pitch:", TEXT_SIZE)
    lcd.drawText (x_data1,y,as3xData[7], TEXT_SIZE + RIGHT)

    lcd.drawText (X_COL2_HEADER,y, "Roll L:", TEXT_SIZE)
    lcd.drawText (x_data2,y,as3xData[13], TEXT_SIZE + RIGHT)


    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL1_HEADER,y, "Yaw:", TEXT_SIZE)
    lcd.drawText (x_data1,y, as3xData[8], TEXT_SIZE + RIGHT)

    lcd.drawText (X_COL2_HEADER,y, "Pitch U:", TEXT_SIZE)
    lcd.drawText (x_data2,y, as3xData[14], TEXT_SIZE + RIGHT)

    y = y + Y_LINE_HEIGHT
    lcd.drawText (X_COL2_HEADER,y, "Pitch D:", TEXT_SIZE)
    lcd.drawText (x_data2,y, as3xData[15], TEXT_SIZE + RIGHT)
  end

  -- Debug Values 
  if (DEBUG_ON) then
      local titles = {[0]=
      "0500","0502","0504","0506","0508","050B","050D"}

      local values = {[0]=
      s0500,s0502,s0504,s0506,s0508,s050B,s050D }

      y = Y_LINE_HEIGHT*2 + Y_HEADER
      for iParam=0,#titles do   -- ??
        -- labels
        local x = X_COL1_HEADER+250
        local val = titles[iParam]
        lcd.drawText (x, y, val, TEXT_SIZE)
        
        val = values[iParam] or "--" 
        x = X_COL1_DATA+250
        lcd.drawText (x, y, val, TEXT_SIZE)

        y = y + Y_LINE_HEIGHT
      end
  end 
end

local function drawAS3XSettingsP1(event)
  drawAS3XSettings(event, 1)
end

local function drawAS3XSettingsP2(event)
  drawAS3XSettings(event, 2)
end


local function doFloat(v)
  if v==nil then return 0.0 end

  local vs = string.format("%1.2f",v)
  
  return vs + 0.0
end


local ESC_Title={[0]="","RPM:","Volts:","Motor:","Mot Out:","Throttle:","FET Temp:", "BEC V:", "BEC T:", "BEC A:"}
local ESC_uom={[0]="","","V","A","%","%","C", "V","C","A"}
local ESC_Status={[0]=0,0,0,0,0,0,0,0,0,0,0}
local ESC_Min={[0]=0,0,0,0,0,0,0,0,0,0,0}
local ESC_Max={[0]=0,0,0,0,0,0,0,0,0,0,0}

local function drawESCStatus(event)
  lcd.clear()
  ESC_Status[1] = getValue("Erpm") -- RPM
  ESC_Status[2] = doFloat(getValue("EVIN")) -- Volts
  ESC_Status[3] = doFloat(getValue("ECUR")) -- Current
  ESC_Status[4] = doFloat(getValue("EOUT"))  -- % Output
  ESC_Status[5] = doFloat(getValue("ETHR")) -- Throttle % (EOUT)
  ESC_Status[6] = getValue("TFET")  -- Temp FET
  
  ESC_Status[7] = doFloat(getValue("VBEC")) -- Volts BEC
  ESC_Status[8] = getValue("TBEC")  -- Temp BEC
  ESC_Status[9] = doFloat(getValue("CBEC")) -- Current BEC

  for i=1,9 do
    if (ESC_Status~=nil) then
      if (ESC_Min[i]==0) then 
          ESC_Min[i]=ESC_Status[i]
      else 
          ESC_Min[i] = math.min(ESC_Min[i],ESC_Status[i])
      end

      ESC_Max[i] = math.max(ESC_Max[i],ESC_Status[i])
    end
  end
 
  lcd.drawText (0,0, "ESC", TEXT_SIZE+INVERS)

  local y = 0
  local x_data = X_COL1_DATA+X_DATA_LEN*1.5
  local x_data2 = X_COL2_DATA+X_DATA_LEN*0.5
  local x_data3 = x_data2 + X_DATA_LEN*0.8


  lcd.drawText (x_data,y , "Status", TEXT_SIZE+BOLD+RIGHT)
  lcd.drawText (x_data2,y, "Min", TEXT_SIZE+BOLD+RIGHT)
  lcd.drawText (x_data3,y, "Max", TEXT_SIZE+BOLD+RIGHT)
  
  y = Y_DATA
  for i=1,9 do
      lcd.drawText (X_COL1_HEADER,y, ESC_Title[i], TEXT_SIZE + BOLD)

      lcd.drawText (x_data,y, ESC_Status[i] or "--", TEXT_SIZE + RIGHT)
      lcd.drawText (x_data + X_DATA_SPACE,y, ESC_uom[i], TEXT_SIZE)

      lcd.drawText (x_data2,y, ESC_Min[i] or "--", TEXT_SIZE + RIGHT)
      lcd.drawText (x_data3,y, ESC_Max[i] or "--", TEXT_SIZE + RIGHT)
      y = y + Y_LINE_HEIGHT
  end
end


local function drawBATStatus(event)
  local Title={[0]="","Bat:","Temp:","Rem :","Curr:","Used:","Imbal:","Cycles:", "RX:", "BCpT?:"}
  local uom={[0]="","V","C","%","mAh","mAh","mV","", "V",""}
  local Values={[0]=0,0,0,0,0,0,0,0,0,0,0}
  local CellValues={[0]=0,0,0,0,0,0,0,0,0,0,0}

  lcd.clear()

  local ESC_Volts = getValue("EVIN") or 0 -- Volts
  local ESC_Current = getValue("ECUR") or 0 -- Current
  
  Values[1] = 0 -- compute later
  Values[2] = getValue("BTmp") -- Current  (C)
  Values[3] = nil -- Remaining???
  Values[4] = getValue("BCur") -- Current  (mAh)
  Values[5] = getValue("BUse") -- Current Used  (mAh)
  Values[6] = getValue("CLMa") -- 0.0  (mV)  Imbalance
  Values[7] = getValue("Cycl") -- Cycles
  Values[8] = readBatValue("A2") -- v
  Values[9] = getValue("BCpT") -- Current  (mAh) ????


  --- Total Voltange Calculation
  local VTotal=0
  for i=1,10 do
      CellValues[i] = getValue("Cel"..i)
      VTotal = VTotal + CellValues[i]
  end

  if (VTotal==0) then -- No Inteligent Battery,use intelligent ESC if any
    VTotal = ESC_Volts
    Values[4] = string.format("%d",ESC_Current * 1000)
  end

  Values[1] = string.format("%2.2f",VTotal)

  --- SCREEN
  
  lcd.drawText (X_COL1_HEADER,0, "Battery Stats", TEXT_SIZE+INVERS)
  


  local y = Y_DATA
  local x_data = X_COL1_DATA+X_DATA_LEN+X_DATA_SPACE*3
  for i=2,9 do
      lcd.drawText (X_COL1_HEADER, y, Title[i], TEXT_SIZE + BOLD)
      lcd.drawText (x_data, y, Values[i] or "--", TEXT_SIZE + RIGHT)
      lcd.drawText (x_data+X_DATA_SPACE, y, uom[i], TEXT_SIZE)
      y = y + Y_LINE_HEIGHT
  end

  y = Y_DATA
  x_data = X_COL2_DATA+X_DATA_LEN+X_DATA_SPACE*5
  for i=1,8 do
      if ((CellValues[i] or 0) > 0) then
        lcd.drawText (X_COL2_HEADER+X_DATA_LEN/2,y, "Cel "..i..":", TEXT_SIZE + BOLD)
        lcd.drawText (x_data,y, string.format("%2.2f",CellValues[i] or 0), TEXT_SIZE + RIGHT)
        lcd.drawText (x_data+X_DATA_SPACE,y, "v", TEXT_SIZE)
      end
      y = y + Y_LINE_HEIGHT
  end

  lcd.drawText (X_COL2_HEADER+X_DATA_LEN/2,0, Title[1], TEXT_SIZE + INVERS + BOLD)
  lcd.drawText (x_data,0, string.format("%2.2f",Values[1] or 0), TEXT_SIZE + INVERS+ RIGHT)
  lcd.drawText (x_data+X_DATA_SPACE, 0, uom[1], TEXT_SIZE + INVERS)
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
local pageTitle = {[0]="Main", "AS3X Settings", "SAFE Settings", "ESC Status", "Battery Status","TextGen","Flight Log"}

local function drawMainScreen(event) 
  lcd.clear()
  lcd.drawText (X_COL1_HEADER, Y_HEADER, "Main Telemetry (Smart RXs)", TEXT_SIZE + INVERS)

  for iParam=1,#pageTitle do    
    -- highlight selected parameter
    local attr = (telPage==iParam) and INVERS or 0

    -- set y draw coord
    local y = (iParam)*Y_LINE_HEIGHT+Y_DATA 
    
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


local pageDraw  = {[0]=drawMainScreen, drawAS3XSettingsP1, drawAS3XSettingsP2, drawESCStatus, drawBATStatus, drawTextGen, drawFlightLogScreen}

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
