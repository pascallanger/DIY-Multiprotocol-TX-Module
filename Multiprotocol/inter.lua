-- Protocol Selected
local PROTO_PAGE = 0
local CONFIRMATION_PAGE = 1

-- Navigation variables
local page = PROTO_PAGE
local dirty = true
local edit = false
local field = 0
local fieldsMax = 0

-- Model settings
local ProtoId = 01
local ProtoSub = 0
local ProtoSubNb = 0

-- Common functions
local lastBlink = 0
local function blinkChanged()
  local time = getTime() % 128
  local blink = (time - time % 64) / 64
  if blink ~= lastBlink then
    lastBlink = blink
    return true
  else
    return false
  end
end

local function fieldIncDec(event, value, max, force)
  if edit or force==true then
    if event == EVT_PLUS_BREAK then
      value = (value + max)
      dirty = true
    elseif event == EVT_MINUS_BREAK then
      value = (value + max + 2)
      dirty = true
    end
    value = (value % (max+1))
  end
  return value
end

local function valueIncDec(event, value, min, max)
  if edit then
    if event == EVT_PLUS_FIRST or event == EVT_PLUS_REPT then
      if value < max then
        value = (value + 1)
        dirty = true
      end
    elseif event == EVT_MINUS_FIRST or event == EVT_MINUS_REPT then
      if value > min then
        value = (value - 1)
        dirty = true
      end
    end
  end
  return value
end

local function navigate(event, fieldMax, prevPage, nextPage)
  if event == EVT_ENTER_BREAK then
    edit = not edit
    dirty = true
  elseif edit then
    if event == EVT_EXIT_BREAK then
      edit = false
      dirty = true  
    elseif not dirty then
      dirty = blinkChanged()
    end
  else
    if event == EVT_PAGE_BREAK then     
      page = nextPage
      field = 0
      dirty = true
    elseif event == EVT_PAGE_LONG then
      page = prevPage
      field = 0
      killEvents(event);
      dirty = true
    else
      field = fieldIncDec(event, field, fieldMax, true)
	end
  end
end

local function getFieldFlags(position)
  flags = 0
  if field == position then
    flags = INVERS
    if edit then
      flags = INVERS + BLINK
    end
  end
  return flags
end

local function channelIncDec(event, value)
  if not edit and event==EVT_MENU_BREAK then
    servoPage = value
    dirty = true
  else
    value = valueIncDec(event, value, 0, 15)
  end
  return value
end

-- Init function
local function init()
end

-- Protocol Menu
local ProtoItems = {,"4""FLYSKY","HUBSAN","FRSKYD","HISKY","V2X2","DSM","DEVO","YD717","KN","SYMAX","SLT","CX10","CG023","BAYANG","FRSKYX","ESKY","MT99XX","MJXQ","SHENQI","FY326","SFHSS","J6PRO","FQ777","ASSAN","FRSKYV","HONTAI","OPENLRS","AFHDS2A","Q2X2","HM830","JOYSWAY","WK2X01","SKYARTEC","CFLIE","H377","ESKY150","BLUEFLY","NE260","INAV","Q303","FBL100","UDI"}
local SubProtoItemsA = {"FLYSKY","V9X9","V6X6","V912","CX20"}
local SubProtoItemsB = {"H107","H301","H501"}
local SubProtoItemsC = {"HISKY","HK310"}
local SubProtoItemsD = {"V2X2","JXD506"}
local SubProtoItemsE = {"DSM2_22","DSM2_11","DSMX_22","DSMX_11","AUTO"}
local SubProtoItemsF = {"YD717","SKYWLKR","SYMAX4","XINXUN","NIHUI"}
local SubProtoItemsG = {"WLTOYS","FEILUN"}
local SubProtoItemsH = {"SYMAX","SYMAX5C"}
local SubProtoItemsI = {"GREEN","BLUE","DM007","JC3015_1","JC3015_2","MK33041"}
local SubProtoItemsJ = {"CG023","YD829","H8_3D"}
local SubProtoItemsK = {"BAYANG","H8S3D"}
local SubProtoItemsL = {"CH_16","CH_8"}
local SubProtoItemsM = {"MT99","H7","YZ","LS","FY805"}
local SubProtoItemsN = {"WLH08","X600","X800","H26D","H26WH","E010"}
local SubProtoItemsO = {"FY326","FY319"}
local SubProtoItemsP = {"HONTAI","JJRCX1","FQ777_951","X5C1"}
local SubProtoItemsQ = {"PWM_IBUS","PPM_IBUS","PWM_SBUS","PPM_SBUS"}
local SubProtoItemsR = {"Q2X2","Q242","Q282","Q222"}
local SubProtoItemsS = {"WK2801","WK2401","WK2601"}
local SubProtoItemsT = {"FBL100","HP100"}
local SubProtoItemsU = {"U816_V1","U816_V2","U839_2014"}

local function drawEngineMenu()
  lcd.clear()
  lcd.drawText(1, 0, "What do you want ?", 0)
  lcd.drawFilledRectangle(0, 0, LCD_W, 8, GREY_DEFAULT+FILL_WHITE)
  lcd.drawNumber(12, 10, ProtoId, getFieldFlags(0)) 
  lcd.drawLine(LCD_W/2-10, 8, LCD_W/2-10, LCD_H, DOTTED, 0)
  lcd.drawText(17, 10, ProtoItems[ProtoId+1], 0)
  
  elseif ProtoId == 179999999999 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  if ProtoId == 0 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 4
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsA[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsA
  elseif ProtoId == 1 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 2
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsB[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsB
  elseif ProtoId == 2 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 3 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsC[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsC
  elseif ProtoId == 4 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsD[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsD
  elseif ProtoId == 5 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 4
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsE[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsE
  elseif ProtoId == 6 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 7 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 4
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsF[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsF
  elseif ProtoId == 8 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsG[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsG
  elseif ProtoId == 9 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsH[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsH
  elseif ProtoId == 10 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 11 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 5
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsI[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsI
  elseif ProtoId == 12 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 2
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsJ[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsJ
  elseif ProtoId == 13 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsK[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsK
  elseif ProtoId == 14 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsL[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsL
  elseif ProtoId == 15 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 16 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 4
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsM[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsM
  elseif ProtoId == 17 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 5
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsN[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsN
  elseif ProtoId == 18 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 19 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsO[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsO
  elseif ProtoId == 20 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 21 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 22 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 23 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 24 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 25 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 3
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsP[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsP
  elseif ProtoId == 26 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 27 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 3
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsQ[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsQ
  elseif ProtoId == 28 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 3
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsR[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsR
  elseif ProtoId == 39 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 47 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 48 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 2
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsS[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsS
  elseif ProtoId == 49 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 50 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 51 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 52 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 53 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 54 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 56 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 57 then
    ProtoSub = 0
    fieldsMax = 0
    ProtoSubNb = 0
    SubProtoItems = {""}
  elseif ProtoId == 58 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 1
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsT[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsT
  elseif ProtoId == 59 then
    ProtoSub = 0
    fieldsMax = 1
    ProtoSubNb = 2
    lcd.drawNumber(12, 20, ProtoSub, getFieldFlags(1))
    lcd.drawText(17, 20, SubProtoItemsU[ProtoSub+1], getFieldFlags(1))
    SubProtoItems = SubProtoItemsU
  end
end
local function engineMenu(event)
  if dirty then
    dirty = false
    drawEngineMenu()
  end

  navigate(event, fieldsMax, page, page+1)

  if field==0 then
    ProtoId = fieldIncDec(event, ProtoId, 41)
  elseif field==1 then
    ProtoSub = fieldIncDec(event, ProtoSub, ProtoSubNb)
  end
end


-- Confirmation Menu
local function drawNextLine(x, y, label, channel)
  lcd.drawText(x, y, label, 0);
  if channel ~= nil then
	lcd.drawSource(x+52, y, MIXSRC_CH1+channel, 0)
  end
  y = y + 8
  if y > 50 then
    y = 12
    x = 120
  end
  return x, y
end
local function drawConfirmationMenu()
  local x = 22
  local y = 12
  lcd.clear()
  lcd.drawText(48, 1, "Install switch?", 0);
  lcd.drawFilledRectangle(0, 0, LCD_W, 9, 0)
    x, y = drawNextLine(x, y, "Protocol: "..ProtoItems[ProtoId+1], nil)
	x, y = drawNextLine(x, y, "Sub-Protocol: "..SubProtoItems[ProtoSub+1], nil)
  lcd.drawText(48, LCD_H-8, "[Enter Long] to confirm", 0);
  fieldsMax = 0
end

local function applySettings()
--  model.defaultInputs()
--  model.deleteMixes()
  if ProtoId == 0 then
    if ProtoSub == 0 then	--proto FLYSKY 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto V9X9 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto V6X6 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="XCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="YCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto V912 
      model.insertMix(4, model.getMixesCount(4), { name="BTMBTN", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="TOPBTN", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto CX20 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 1 then
    if ProtoSub == 0 then	--proto H107 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto H301 
      model.insertMix(4, model.getMixesCount(4), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="STAB", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto H501 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="GPS_HOLD", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="ALT_HOLD", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="SNAPSHOT", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 2 then
    if ProtoSub == 0 then	--proto FRSKYD 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 3 then
    if ProtoSub == 0 then	--proto HISKY 
    end
    if ProtoSub == 1 then	--proto HK310 
    end
  elseif ProtoId == 4 then
    if ProtoSub == 0 then	--proto V2X2 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="MAG_CAL_X", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="MAG_CAL_Y", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto JXD506 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 5 then
    if ProtoSub == 0 then	--proto DSM2_22 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto DSM2_11 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto DSMX_22 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto DSMX_11 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto AUTO 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
    end
  elseif ProtoId == 6 then
    if ProtoSub == 0 then	--proto DEVO 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 7 then
    if ProtoSub == 0 then	--proto YD717 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto SKYWLKR 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto SYMAX4 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto XINXUN 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto NIHUI 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGHT", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 8 then
    if ProtoSub == 0 then	--proto WLTOYS 
      model.insertMix(4, model.getMixesCount(4), { name="DR", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="THOLD", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="IDLEUP", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="GYRO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="Ttrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="Atrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="Etrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto FEILUN 
      model.insertMix(4, model.getMixesCount(4), { name="DR", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="THOLD", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="IDLEUP", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="GYRO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="Ttrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="Atrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="Etrim", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 9 then
    if ProtoSub == 0 then	--proto SYMAX 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto SYMAX5C 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 10 then
    if ProtoSub == 0 then	--proto SLT 
      model.insertMix(4, model.getMixesCount(4), { name="GEAR", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="PITCH", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto VISTA 
    end
  elseif ProtoId == 11 then
    if ProtoSub == 0 then	--proto GREEN 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="RATE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto BLUE 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="RATE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto DM007 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="MODE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto JC3015_1 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="MODE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 5 then	--proto JC3015_2 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="MODE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="DFLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 6 then	--proto MK33041 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="MODE", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 12 then
    if ProtoSub == 0 then	--proto CG023 
    end
    if ProtoSub == 1 then	--proto YD829 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto H8_3D 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LIGTH", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="OPT1", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="OPT2", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="SNAPSHOT", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CAL2", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="GIMBAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 13 then
    if ProtoSub == 0 then	--proto BAYANG 
    end
    if ProtoSub == 1 then	--proto H8S3D 
    end
  elseif ProtoId == 14 then
    if ProtoSub == 0 then	--proto CH_16 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(14, model.getMixesCount(14), { name="----", source=92, weight=100, multiplex=ADD })
      model.insertMix(15, model.getMixesCount(15), { name="----", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto CH_8 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 15 then
    if ProtoSub == 0 then	--proto ESKY 
      model.insertMix(4, model.getMixesCount(4), { name="GYRO", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="PITCH", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 16 then
    if ProtoSub == 0 then	--proto MT99 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto H7 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto YZ 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto LS 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="INVERT", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto FY805 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 17 then
    if ProtoSub == 0 then	--proto WLH08 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto X600 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto X800 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto H26D 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 4 then	--proto E010 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 5 then	--proto H26WH 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="AUTOFLIP", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="PAN", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="TILT", source=92, weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 18 then
    if ProtoSub == 0 then	--proto SHENQI 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 19 then
    if ProtoSub == 0 then	--proto FY326 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="Calibrate", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="Expert", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto FY319 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="Calibrate", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="Expert", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 20 then
    if ProtoSub == 0 then	--proto SFHSS 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 21 then
    if ProtoSub == 0 then	--proto J6PRO 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 22 then
    if ProtoSub == 0 then	--proto FQ777 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="SNAPSHOT", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 23 then
    if ProtoSub == 0 then	--proto ASSAN 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 24 then
    if ProtoSub == 0 then	--proto FRSKYV 
    end
  elseif ProtoId == 25 then
    if ProtoSub == 0 then	--proto HONTAI 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto JJRCX1 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="ARM", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto X5C1 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto FQ777_951 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 26 then
    if ProtoSub == 0 then	--proto OPENLRS 
    end
  elseif ProtoId == 27 then
    if ProtoSub == 0 then	--proto PWM_IBUS 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="CH13", source=defaultChannel(12), weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
      model.insertMix(14, model.getMixesCount(14), { name="Failsave T", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto PPM_IBUS 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="CH13", source=defaultChannel(12), weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
      model.insertMix(14, model.getMixesCount(14), { name="Failsave T", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto PWM_SBUS 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="CH13", source=defaultChannel(12), weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
      model.insertMix(14, model.getMixesCount(14), { name="Failsave T", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 3 then	--proto PPM_SBUS 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="CH9", source=defaultChannel(8), weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="CH10", source=defaultChannel(9), weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="CH11", source=defaultChannel(10), weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="CH12", source=defaultChannel(11), weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="CH13", source=defaultChannel(12), weight=100, multiplex=ADD })
      model.insertMix(13, model.getMixesCount(13), { name="Reset/Bind", source=92, weight=100, multiplex=ADD })
      model.insertMix(14, model.getMixesCount(14), { name="Failsave T", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 28 then
    if ProtoSub == 0 then	--proto Q2X2 
    end
    if ProtoSub == 10 then	--proto Q282 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="XCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="YCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 8 then	--proto Q222 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="MODULE2", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="MODULE1", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="XCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="YCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 9 then	--proto Q242 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="PICTURE", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="XCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="YCAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(12, model.getMixesCount(12), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 39 then
    if ProtoSub == 0 then	--proto HM830 
      model.insertMix(4, model.getMixesCount(4), { name="Bouton", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 47 then
    if ProtoSub == 0 then	--proto JOYSWAY 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 48 then
    if ProtoSub == 0 then	--proto WK2801 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto WK2601 
      model.insertMix(4, model.getMixesCount(4), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto WK2401 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 49 then
    if ProtoSub == 0 then	--proto SKYARTEC 
      model.insertMix(4, model.getMixesCount(4), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name=" ?", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 50 then
    if ProtoSub == 0 then	--proto CFLIE 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 51 then
    if ProtoSub == 0 then	--proto H377 
      model.insertMix(4, model.getMixesCount(4), { name="CH5", source=defaultChannel(4), weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="CH6", source=defaultChannel(5), weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="CH7", source=defaultChannel(6), weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="CH8", source=defaultChannel(7), weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 52 then
    if ProtoSub == 0 then	--proto ESKY150 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 53 then
    if ProtoSub == 0 then	--proto BLUEFLY 
      model.insertMix(4, model.getMixesCount(4), { name="GEAR", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="PITCH", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 54 then
    if ProtoSub == 0 then	--proto NE260 
      model.insertMix(4, model.getMixesCount(4), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 56 then
    if ProtoSub == 0 then	--proto INAV 
    end
  elseif ProtoId == 57 then
    if ProtoSub == 0 then	--proto Q303 
      model.insertMix(4, model.getMixesCount(4), { name="AHOLD", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="SNAPSHOT", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="HEADLESS", source=91, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="RTH", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="GIMBAL", source=92, weight=100, multiplex=ADD })
      model.insertMix(11, model.getMixesCount(11), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 58 then
    if ProtoSub == 0 then	--proto FBL100 
      model.insertMix(4, model.getMixesCount(4), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name=" ?", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto HP100 
      model.insertMix(4, model.getMixesCount(4), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name=" ? ", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name=" ?", source=92, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="", source=92, weight=100, multiplex=ADD })
    end
  elseif ProtoId == 59 then
    if ProtoSub == 0 then	--proto U816_V1 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP 360", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="MODE 2", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 1 then	--proto U816_V2 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP 360", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="MODE 2", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="", source=92, weight=100, multiplex=ADD })
    end
    if ProtoSub == 2 then	--proto U839_2014 
      model.insertMix(4, model.getMixesCount(4), { name="FLIP 360", source=92, weight=100, multiplex=ADD })
      model.insertMix(5, model.getMixesCount(5), { name="FLIP", source=97, weight=100, multiplex=ADD })
      model.insertMix(6, model.getMixesCount(6), { name="VIDEO", source=92, weight=100, multiplex=ADD })
      model.insertMix(7, model.getMixesCount(7), { name="LED", source=95, weight=100, multiplex=ADD })
      model.insertMix(8, model.getMixesCount(8), { name="MODE 2", source=92, weight=100, multiplex=ADD })
      model.insertMix(9, model.getMixesCount(9), { name="---", source=92, weight=100, multiplex=ADD })
      model.insertMix(10, model.getMixesCount(10), { name="", source=92, weight=100, multiplex=ADD })
    end
  end

end

local function confirmationMenu(event)
  if dirty then
    dirty = false
    drawConfirmationMenu()
  end

  navigate(event, fieldsMax, PROTO_PAGE, page)

  if event == EVT_EXIT_BREAK then
    return 2
  elseif event == EVT_ENTER_LONG then
    killEvents(event)
    applySettings()
    return 2
  else
    return 0
  end
end

-- Main
local function run(event)
  if event == nil then
    error("Cannot be run as a model script!")
  end
  lcd.lock()
  if page == PROTO_PAGE then
    engineMenu(event)
  elseif page == CONFIRMATION_PAGE then
    return confirmationMenu(event)
  end
  return 0
end

return { init=init, run=run }
