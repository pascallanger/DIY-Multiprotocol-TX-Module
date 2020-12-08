--
-- This telemetry script displays the Flight Log Gain
-- Parameters streamed from the Blade 150S Spektrum AR6335A
-- Flybarless Controller.

-- The script facilitates the setting of the FBL's
-- Gain Parameters including PID  for both
-- cyclic and tail.  It is similar to the Telemetry Based
-- Text Generator available on Spektrum transmitters.

-- Supporting similar Blade micros such as the Fusion 180
-- would possibly require minor modifications to this script. 

-- This script reads telemetry data from the Spektrum
-- receiver and thus functionality relies on data being
-- captured by the OpenTX transmitter.  A DSM
-- telemetry-ready module is required.  Please see the
-- MULTI-Module project at https://www.multi-module.org/.

-- The only supported display is the Taranis'.  It may work
-- with higher res screens.
--


-- Sensor names
local PSensor = "FdeA"
local ISensor = "FdeB"
local DSensor = "FdeL"
local RSensor = "FdeR"
local ActiveParamSensor = "Hold"

local tags = {"P", "I", "D"}


local function getPage(iParam)
  -- get page from 0-based index
  -- {0,1,2,3}: cyclic (1), {4,5,6,7}: tail (2)
  local res = (math.floor(iParam/4)==0) and 1 or 2
  return res
end

function round(v)
  -- round float
  local factor = 100
  return math.floor(v * factor + 0.5) / factor
end


local function readValue(sensor)
  -- read from sensor, round and return
  local v = getValue(sensor)
  v = round(v)
  return v
end
 
local function readActiveParamValue(sensor)
  --  read and return a validated active parameter value
  local v = getValue(sensor)
  if (v<1 or v>8) then
    return nil
  end
  return v
end
   
local function readParameters()
  -- read and return parameters
  local p = readValue(PSensor)
  local i = readValue(ISensor)
  local d = readValue(DSensor)
  local r = readValue(RSensor)
  local a = readActiveParamValue(ActiveParamSensor)
  return {p,i,d,r,a}
end

local function drawParameters()
  -- draw labels and params on screen
  local params = readParameters()
  local activeParam = params[5]
  
  -- if active gain does not validate then assume
  -- Gain Adjustment Mode is disabled
  if not activeParam then
    lcd.clear()
    lcd.drawText(20,30,"Please enter Gain Adjustment Mode")
    return
  end
  
  local activePage = getPage(activeParam-1)
  for iParam=0,7 do
    -- highlight selected parameter
    local attr = (activeParam==iParam+1) and 2 or 0
    -- circular index (per page)
    local perPageIndx = iParam % 4 + 1
    -- check if displaying cyclic params.
    local isCyclicPage = (getPage(iParam)==1)
    -- set y draw coord
    local y = perPageIndx*10+2
    
    -- labels
    local x = isCyclicPage and 6 or 120
    -- labels are P,I,D for both pages except for last param
    local val = iParam==3 and "Response" or
                  (iParam==7 and "Filtering" or tags[perPageIndx])
    lcd.drawText (x, y, val, attr)
    
    -- gains
    -- set all params for non-active page to '--' rather than 'last value'
    val = (getPage(iParam)==activePage) and params[perPageIndx] or '--'
    x = isCyclicPage and 70 or 180
    lcd.drawText (x, y, val, attr)
  end
end


local function run_func(event)
  -- TODO: calling clear() on every function call redrawing all labels is not ideal
  lcd.clear()
  lcd.drawText (8, 2, "Cyclic (0...200)")
  lcd.drawText (114, 2, "Tail (0...200)")
  drawParameters()
end

local function init_func() end
local function bg_func() end


return { run=run_func, background=bg_func, init=init_func  }
