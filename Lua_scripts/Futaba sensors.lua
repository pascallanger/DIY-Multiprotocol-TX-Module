local toolName = "TNS|Futaba S.BUS2 sensors|TNE"

-- Sets up the S.BUS2 sensors of a Futaba receiver on a MULTI-Module (T-FHSS): EdgeTX's Futaba telemetry (radio/src/telemetry/futaba.h)
-- shows a slot with data that no sensor claims as a raw sensor "S<slot>"
-- (ID 0x1310, instance = the slot). S.BUS2 sensors don't tell their type
-- on the bus, so this tool creates the typed sensors: ID = the type,
-- instance = its first slot, subId = the value (types with several). Once
-- a typed sensor claims a slot, EdgeTX decodes it. With model.setSensor()
-- and model.deleteSensor() (EdgeTX/edgetx#8105): sensors are created directly, the
-- raw ones of the slots taken deleted, a slot's type can be changed.
-- Before: setTelemetryValue(), which needs "Discover new sensors" on (the
-- model's telemetry page), and old sensors are deleted on that page.

local RAW = 0x1310
local canSet = type(model.setSensor) == "function" and type(model.deleteSensor) == "function"
local colorLcd = type(lcd.RGB) == "function"
local SLOTS = 32

-- Types (futaba.h): ID, name, slots, values { name, unit, prec }; the name
-- gets the type's count (Tmp1, Tmp2...)
local TYPES = {
  { id = 0x1311, name = "Temp (SBS-01T)", slots = 1, values = { { "Tmp", UNIT_CELSIUS, 0 } } },
  { id = 0x1312, name = "Temp 125 (SBS-01TE)", slots = 1, values = { { "Tmp", UNIT_CELSIUS, 0 } } },
  { id = 0x1313, name = "RPM (SBS-01RM)", slots = 1, values = { { "RPM", UNIT_RPMS, 0 } } },
  { id = 0x1314, name = "Voltage (SBS-01V)", slots = 2,
    values = { { "Vlt", UNIT_VOLTS, 1 }, { "Vlt", UNIT_VOLTS, 1 } } },
  { id = 0x1315, name = "Current (SBS-01C)", slots = 3,
    values = { { "Cur", UNIT_AMPS, 2 }, { "Vcu", UNIT_VOLTS, 2 }, { "Cap", UNIT_MAH, 0 } } },
}

local function typeById(id)
  for i = 1, #TYPES do
    if TYPES[i].id == id then return i end
  end
  return nil
end

-- Presets: { slot offset in the group, type index, names }; ESCape32: its
-- telemetry ID 1..4 = the group of 8 slots, slots 2..7 of it
local PRESETS = {
  { name = "ESCape32", groups = 4, sensors = {
    { 2, 1, { "TEsc" } }, { 3, 1, { "TMot" } }, { 4, 5, { "Curr", "VEsc", "Capa" } }, { 7, 3, { "RPM" } } } },
}

local claims = {}    -- slot -> { type = i, first = slot, sensors = { index... } } (all slots of a typed sensor)
local raws = {}      -- slot -> { name = raw sensor name, index = n }
local pending = {}   -- slot -> type index chosen here, not created yet
local lines = {}
local sel, top, edit, blink = 1, 1, false, 0
local preset, presetGroup = 1, 1
local status = ""

local function limit(v, mn, mx)
  if v < mn then return mn end
  if v > mx then return mx end
  return v
end

local function screenLines()
  if colorLcd then return limit(math.floor(((LCD_H or 272) - 46) / 20), 8, 12) end
  return 6
end

local function drawTitle(title)
  if colorLcd and lcd.drawFilledRectangle then
    lcd.drawFilledRectangle(0, 0, LCD_W, 30, COLOR_THEME_SECONDARY1)
    lcd.drawText(3, 5, title, COLOR_THEME_PRIMARY2)
  elseif lcd.drawScreenTitle then
    lcd.drawScreenTitle(title, 0, 0)
  else
    lcd.drawText(0, 0, title, INVERS)
  end
end

-- The model's sensors: the typed ones' slots, the raw ones
local function scan()
  claims, raws = {}, {}
  for i = 0, 59 do
    local s = model.getSensor(i)
    if s and s.type == 0 and s.id then
      local t = typeById(s.id)
      if t and s.instance > 0 then
        local c = claims[s.instance]
        if not (c and c.first == s.instance and c.type == t) then
          c = { type = t, first = s.instance, sensors = {} }
          for k = 0, TYPES[t].slots - 1 do claims[s.instance + k] = c end
        end
        c.sensors[#c.sensors + 1] = i
      elseif s.id == RAW and s.instance > 0 then
        raws[s.instance] = { name = s.name, index = i }
      end
    end
  end
end

local function rebuildLines()
  lines = {}
  for slot = 1, SLOTS - 1 do
    if claims[slot] or raws[slot] or pending[slot] then
      lines[#lines + 1] = { kind = "slot", slot = slot }
    end
  end
  lines[#lines + 1] = { kind = "preset" }
  lines[#lines + 1] = { kind = "create" }
  sel = limit(sel, 1, #lines)
end

-- Slot covered by a pending multi-slot type starting before it
local function pendingCover(slot)
  for s = slot - 1, math.max(1, slot - 2), -1 do
    local t = pending[s]
    if t and s + TYPES[t].slots > slot then return s end
  end
  return nil
end

local function slotText(slot)
  local t = pending[slot]
  if t then return "* " .. TYPES[t].name end
  if pending[slot] == false then return "* raw" end
  local c = claims[slot]
  if c then
    return (c.first == slot and "" or "  ") .. TYPES[c.type].name
  end
  local cov = pendingCover(slot)
  if cov then return "  (slot " .. cov .. ")" end
  local v = raws[slot] and getValue(raws[slot].name)
  if type(v) == "number" then return string.format("raw %04X", v) end
  return "raw"
end

local function lineText(line)
  if line.kind == "slot" then return "Slot " .. line.slot, slotText(line.slot) end
  if line.kind == "preset" then
    local p = PRESETS[preset]
    return "Preset", p.name .. " ID " .. presetGroup
  end
  return "Create sensors", ">"
end

local function applyPreset()
  local p = PRESETS[preset]
  local base = 8 * (presetGroup - 1)
  for _, s in ipairs(p.sensors) do
    local slot = base + s[1]
    if not claims[slot] or canSet then pending[slot] = s[2] end
  end
  status = "Preset set: Create sensors"
  rebuildLines()
end

-- pending[slot]: a type index, or false = back to raw (its typed sensor
-- deleted); nil = unchanged
local function changeSlot(slot, dir)
  local c = claims[slot]
  if c and not canSet then
    status = "Has a sensor: delete it first"
    return
  end
  if c and c.first ~= slot then
    status = "Part of slot " .. c.first
    return
  end
  local cur = pending[slot]
  if cur == nil then cur = c and c.type or 0 elseif cur == false then cur = 0 end
  local t = cur + dir
  if t < 0 then t = #TYPES end
  if t > #TYPES then t = 0 end
  if c and t == c.type then pending[slot] = nil
  elseif t == 0 then pending[slot] = c and false or nil
  else pending[slot] = t end
end

-- Names: the type's base + a count (the preset's own names if given)
local function nextName(base, used)
  for n = 1, 9 do
    local name = base .. n
    if not used[name] then
      used[name] = true
      return name
    end
  end
  return base
end

-- With model.setSensor(): a free sensor index
local function freeIndex()
  for i = 0, 59 do
    local s = model.getSensor(i)
    if s and s.name == "" then return i end
  end
  return nil
end

local function createSensor(id, sub, slot, unit, prec, name)
  if not canSet then
    return setTelemetryValue(id, sub, slot, 0, unit, prec, name)
  end
  local i = freeIndex()
  return i ~= nil and model.setSensor(i, { name = name, id = id, subId = sub,
    instance = slot, unit = unit, prec = prec })
end

-- With model.deleteSensor(): the old sensors of the slots a change takes
local function deleteOld()
  if not canSet then return end
  local del = {}
  for slot, t in pairs(pending) do
    local c = claims[slot]
    if c and c.first == slot then
      for _, i in ipairs(c.sensors) do del[i] = true end
    end
    local n = t and TYPES[t].slots or 1
    for k = 0, n - 1 do
      if t and raws[slot + k] then del[raws[slot + k].index] = true end
    end
  end
  for i in pairs(del) do model.deleteSensor(i) end
end

local function createSensors()
  local used, made, failed = {}, 0, 0
  local changed = next(pending) ~= nil
  deleteOld()
  for i = 0, 59 do
    local s = model.getSensor(i)
    if s and s.name and s.name ~= "" then used[s.name] = true end
  end
  local names = {}
  local p = PRESETS[preset]
  for _, s in ipairs(p.sensors) do names[8 * (presetGroup - 1) + s[1]] = s[3] end
  for slot = 1, SLOTS - 1 do
    local t = pending[slot]
    if t then
      local ty = TYPES[t]
      for sub, v in ipairs(ty.values) do
        local name = names[slot] and names[slot][sub]
        if name and not used[name] then used[name] = true else name = nextName(v[1], used) end
        if createSensor(ty.id, sub - 1, slot, v[2], v[3], name) then
          made = made + 1
        else
          failed = failed + 1
        end
      end
    end
  end
  pending = {}
  scan()
  rebuildLines()
  if failed > 0 then
    status = canSet and "No free sensor" or "Turn on Discover new sensors"
  elseif made > 0 then
    status = made .. " sensors created"
  elseif changed and canSet then
    status = "Sensors deleted"
  else
    status = "Nothing to create"
  end
end

local function handleEvent(event)
  local line = lines[sel]
  local nextEvent = edit and EVT_VIRTUAL_INC or EVT_VIRTUAL_NEXT
  local prevEvent = edit and EVT_VIRTUAL_DEC or EVT_VIRTUAL_PREV
  if event == nextEvent or event == prevEvent then
    local dir = event == nextEvent and 1 or -1
    if not edit then
      sel = limit(sel + dir, 1, #lines)
    elseif line.kind == "slot" then
      changeSlot(line.slot, dir)
    elseif line.kind == "preset" then
      presetGroup = limit(presetGroup + dir, 1, PRESETS[preset].groups)
    end
  elseif event == EVT_VIRTUAL_ENTER then
    if line.kind == "create" then
      createSensors()
    elseif line.kind == "preset" and edit then
      edit = false
      applyPreset()
    else
      edit = not edit
      if not edit then rebuildLines() end
    end
  end
  local count = screenLines()
  if sel < top then top = sel end
  if sel >= top + count then top = sel - count + 1 end
end

local function draw()
  lcd.clear()
  drawTitle("Futaba S.BUS2 sensors")
  local font = colorLcd and 0 or SMLSIZE
  local x = 2
  local y = colorLcd and 34 or 9
  local dy = colorLcd and 20 or 8
  local valueX = colorLcd and 150 or 50
  if status ~= "" then
    lcd.drawText(LCD_W - 2, colorLcd and 5 or 0, status, (colorLcd and COLOR_THEME_PRIMARY2 or SMLSIZE + INVERS) + RIGHT)
  end
  if #lines <= 2 then
    lcd.drawText(x, y + dy * (screenLines() - 1), "No slot data yet (raw S<n> sensors)", font)
  end
  for i = top, math.min(#lines, top + screenLines() - 1) do
    local attr = font
    if i == sel then
      attr = attr + INVERS
      if edit then
        blink = (blink + 1) % 30
        if blink > 15 then attr = font end
      end
    end
    local label, value = lineText(lines[i])
    local yy = y + (i - top) * dy
    lcd.drawText(x, yy, label, attr)
    lcd.drawText(valueX, yy, value, attr)
  end
end

local scanTime = 0

local function init()
  scan()
  rebuildLines()
end

local function run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif event == EVT_VIRTUAL_EXIT and not edit then
    return 2
  elseif event == EVT_VIRTUAL_EXIT then
    edit = false
    rebuildLines()
  end
  -- new raw slots show up as they are discovered
  if not edit and getTime() > scanTime then
    scanTime = getTime() + 100
    scan()
    rebuildLines()
  end
  handleEvent(event)
  draw()
  return 0
end

return { init = init, run = run }
