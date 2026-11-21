local toolName = "TNS|FrSky ACCESS RX|TNE"

-- Receiver options of a FrSky ACCESS receiver bound to the MULTI FrSkyA
-- protocol, as the EdgeTX receiver options page of an ACCESS module.
--
-- EdgeTX/Multi bridge (multiBuffer "Pipe"):
--   Multi_Buffer[4]      1..7: message piece ready to send
--   Multi_Buffer[5..11]  type, offset << 4 | length, up to 5 bytes
--   Multi_Buffer[14]     length of the message from the module, 0 = none
--   Multi_Buffer[15..]   type, length, data
-- Messages as EdgeTX's PXX2 ones. Type 5 = receiver settings:
--   flag0  receiver slot, 0x40 = write
--   flag1  0x80 telemetry off, 0x40 read only, 0x20 S.Bus 24, 0x10 fast PWM,
--          0x08 F.Port, 0x04 telemetry 25 mW, 0x02 PWM on CH5/CH6, 0x01 F.Port2
--   then one byte per output: the channel (0 = CH1), or on receivers with a
--   configurable telemetry port (SR10+, R10+, SR12+, R12+) 0x40 S.Port,
--   0x80 S.Bus out, 0xC0 F.Bus, 0xA0 S.Bus in (output 1 only); with PWM on
--   CH5/CH6 (capability 0x04), outputs 5 / 6 = last channel + 1: S.Port / S.Bus
--   out (as EdgeTX)
-- Type 6 = hardware info: request = receiver index; answer = index, model ID,
--   hw version (2 bytes), sw version (2 bytes), variant, capabilities
--   (0x01 F.Port, 0x02 telemetry 25 mW, 0x04 PWM on CH5/CH6, 0x08 F.Port2,
--   0x10 racing mode, 0x20 S.Bus 24)

local PROTO_FRSKYA = 112

local TX_READY = 4
local TX_PAYLOAD = 5
local TX_MAX_PAYLOAD = 7
local RX_READY = 14
local RX_PAYLOAD = 15
local RX_MAX_PAYLOAD = 32
local TYPE_RX_SETTINGS = 5
local TYPE_HW_INFO = 6
local SLOT = 0
local RETRY = 200           -- 2 s, in 10 ms ticks
local WRITE_WAIT = 250      -- the receiver restarts after a write
local colorLcd = type(lcd.RGB) == "function"

local settings = nil        -- { flag1 = n, outputs = { ch, ... } }
local hwInfo = nil          -- { name = "RX6R 2.1.8", id = n, caps = n }
local channels = 16         -- the module's channel count
local lines = {}
local sel = 1
local top = 1
local edit = false
local blink = 0
local moduleOk = false
local queue = {}
local state = "idle"        -- idle, info, reading, writing
local status = ""
local deadline = 0
local tries = 0

local function limit(v, mn, mx)
  if v < mn then return mn end
  if v > mx then return mx end
  return v
end

local function bit(v, b)
  return math.floor(v / b) % 2
end

local function setBit(v, b, on)
  if bit(v, b) == 1 then v = v - b end
  if on then v = v + b end
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

local function findModule()
  for i = 0, 1 do
    local m = model.getModule(i)
    if m and m["Type"] == 6 and m["protocol"] == PROTO_FRSKYA then
      channels = limit(m["channelsCount"] or 16, 1, 24)
      return true
    end
  end
  return false
end

local function initBuffer()
  multiBuffer(0, string.byte("P"))
  if multiBuffer(0) ~= string.byte("P") then
    error("Not enough memory!")
    return
  end
  multiBuffer(1, string.byte("i"))
  multiBuffer(2, string.byte("p"))
  multiBuffer(3, string.byte("e"))
  multiBuffer(TX_READY, 0)
  multiBuffer(RX_READY, 0)
end

local function release()
  for i = 0, RX_PAYLOAD + RX_MAX_PAYLOAD - 1 do
    multiBuffer(i, 0)
  end
end

-- A message in pieces of up to 5 bytes: type, offset << 4 | length, data
local function queueMessage(msgType, data)
  local len = #data
  local pos = 0
  repeat
    local piece = { msgType, pos * 16 + len }
    for i = pos + 1, math.min(pos + 5, len) do
      piece[#piece + 1] = data[i]
    end
    queue[#queue + 1] = piece
    pos = pos + 5
  until pos >= len
end

local function sendQueue()
  if #queue == 0 or multiBuffer(TX_READY) ~= 0 then return end
  local piece = table.remove(queue, 1)
  for i = 1, TX_MAX_PAYLOAD do
    multiBuffer(TX_PAYLOAD + i - 1, piece[i] or 0)
  end
  multiBuffer(TX_READY, #piece)
end

-- 2.4 GHz ACCESS receiver model IDs; TD / TW (TANDEM / TWIN) and R9 (900 MHz)
-- receivers are left out
local rxNames = {
  [1]="X8R", [2]="RX8R", [3]="RX8R Pro", [4]="RX6R", [5]="RX4R", [6]="G-RX8", [7]="G-RX6",
  [8]="X6R", [9]="X4R", [10]="X4RSB", [11]="XSR", [12]="XSR M", [13]="RXSR", [14]="S6R",
  [15]="S8R", [16]="XM", [17]="XM+", [18]="XMR", [28]="Archer X", [64]="Archer Plus SR10+",
  [65]="Archer Plus R10+", [66]="Archer Plus GR8", [67]="Archer Plus R8",
  [68]="Archer Plus SR8", [69]="Archer Plus GR6", [70]="Archer Plus R6",
  [71]="R6M(ESC DC)", [72]="Archer Plus RS", [73]="Rs Mini", [74]="R6FB", [75]="GR6FB",
  [76]="Archer Plus SR12+", [77]="Archer Plus R12+", [78]="R6 Mini E", [79]="SR6 Mini",
  [80]="SR6 Mini E", [81]="SR6BL15A", [82]="SR6Lite", [83]="R6Lite", [84]="RS-Lite",
  [85]="R2SIM" }

-- Version bytes: major, minor << 4 | revision; shown with major + 1 as EdgeTX
local function versionText(major, b)
  if major == 0xFF then return "---" end
  return (major + 1) .. "." .. math.floor(b / 16) .. "." .. (b % 16)
end

local function readInfo()
  queue = {}
  queueMessage(TYPE_HW_INFO, { SLOT })
  state = "info"
  status = "Reading..."
  tries = 1
  deadline = getTime() + RETRY
end

local function readSettings()
  queue = {}
  queueMessage(TYPE_RX_SETTINGS, { SLOT })
  state = "reading"
  status = "Reading..."
  tries = 1
  deadline = getTime() + RETRY
end

local function writeSettings()
  if not settings then return end
  local data = { 0x40 + SLOT, settings.flag1 }
  for i = 1, #settings.outputs do
    data[#data + 1] = settings.outputs[i]
  end
  queue = {}
  queueMessage(TYPE_RX_SETTINGS, data)
  state = "writing"
  status = "Writing..."
  edit = false
  deadline = getTime() + WRITE_WAIT
end

local function pollRx()
  local len = multiBuffer(RX_READY)
  if not len or len == 0 then return end
  local p = {}
  for i = 1, math.min(len, RX_MAX_PAYLOAD) do
    p[i] = multiBuffer(RX_PAYLOAD + i - 1)
  end
  multiBuffer(RX_READY, 0)
  if len < 3 or p[2] + 2 > len then return end
  if p[1] == TYPE_HW_INFO and state == "info" and p[2] >= 7 then
    local caps = 0
    for i = p[2] + 2, 10, -1 do caps = caps * 256 + p[i] end
    hwInfo = { name = (rxNames[p[4]] or ("ID " .. p[4])) .. " " .. versionText(p[7], p[8]), id = p[4], caps = caps }
    readSettings()
    return
  end
  if p[1] ~= TYPE_RX_SETTINGS or len < 4 or state ~= "reading" then return end
  settings = { flag1 = p[4], outputs = {} }
  for i = 5, p[2] + 2 do
    settings.outputs[#settings.outputs + 1] = p[i]
  end
  state = "idle"
  status = bit(settings.flag1, 0x40) == 1 and "Read only" or ""
end

local function timers()
  if state == "idle" or getTime() < deadline then return end
  if state == "writing" then
    readSettings()            -- read back what the receiver kept
  elseif tries < 5 then
    queue = {}
    queueMessage(state == "info" and TYPE_HW_INFO or TYPE_RX_SETTINGS, { SLOT })
    tries = tries + 1
    deadline = getTime() + RETRY
  else
    state = "idle"
    status = "No answer"
  end
end

local portModes = { "S.Port", "F.Port", "F.Port2" }

local function portMode()
  if bit(settings.flag1, 0x01) == 1 then return 3 end
  if bit(settings.flag1, 0x08) == 1 then return 2 end
  return 1
end

-- Rows as EdgeTX's receiver options: the optional ones by capability
local function hasCap(b)
  return hwInfo == nil or bit(hwInfo.caps, b) == 1
end

local function rebuildLines()
  lines = { { hwInfo and hwInfo.name or "Read", "action", readInfo } }
  lines[#lines + 1] = { "Fast PWM", "flag", 0x10 }
  lines[#lines + 1] = { "Telemetry", "flag", 0x80, true }
  if hasCap(0x02) then lines[#lines + 1] = { "Telem 25mW", "flag", 0x04 } end
  if hasCap(0x01) or hasCap(0x08) then lines[#lines + 1] = { "Port", "port" } end
  if hasCap(0x20) then lines[#lines + 1] = { "S.Bus 24", "flag", 0x20 } end
  if hasCap(0x04) then lines[#lines + 1] = { "PWM CH5/CH6", "flag", 0x02 } end
  if settings then
    for i = 1, #settings.outputs do
      lines[#lines + 1] = { "Output " .. i, "output", i }
    end
  end
  lines[#lines + 1] = { "Write", "action", writeSettings }
  sel = limit(sel, 1, #lines)
end

-- Output pin modes (EdgeTX pxx2.h CH_MAP_*, RECEIVER_OPTION_D_TELE_PORT)
local CH_MAP_SPORT, CH_MAP_SBUS_OUT, CH_MAP_SBUS_IN, CH_MAP_FBUS = 0x40, 0x80, 0xA0, 0xC0
local telePortIds = { [64] = true, [65] = true, [76] = true, [77] = true }

local function hasTelePort()
  return hwInfo ~= nil and telePortIds[hwInfo.id] == true
end

-- The values output i can take, in order, and their names
local function outputChoices(i)
  local list = {}
  for ch = 0, channels - 1 do list[#list + 1] = ch end
  if hasTelePort() then
    list[#list + 1] = CH_MAP_SPORT
    list[#list + 1] = CH_MAP_SBUS_OUT
    list[#list + 1] = CH_MAP_FBUS
    if i == 1 then list[#list + 1] = CH_MAP_SBUS_IN end
  elseif hwInfo and bit(hwInfo.caps, 0x04) == 1 and (i == 5 or i == 6) then
    list[#list + 1] = channels
  end
  return list
end

local function outputName(i, v)
  if v < channels then return "CH" .. (v + 1) end
  if hasTelePort() then
    if v == CH_MAP_SPORT then return "S.Port" end
    if v == CH_MAP_SBUS_OUT then return "S.Bus out" end
    if v == CH_MAP_FBUS then return "F.Bus" end
    if v == CH_MAP_SBUS_IN and i == 1 then return "S.Bus in" end
  elseif v == channels and (i == 5 or i == 6) then
    return i == 5 and "S.Port" or "S.Bus out"
  end
  if v < 24 then return "CH" .. (v + 1) end
  return string.format("0x%02X", v)
end

local function lineValue(line)
  if line[2] == "action" then return ">" end
  if not settings then return "--" end
  if line[2] == "flag" then
    local on = bit(settings.flag1, line[3]) == 1
    if line[4] then on = not on end
    return on and "On" or "Off"
  end
  if line[2] == "port" then return portModes[portMode()] end
  if line[2] == "output" then return outputName(line[3], settings.outputs[line[3]]) end
  return ""
end

local function changeValue(dir)
  local line = lines[sel]
  if not line or not settings then return end
  if line[2] == "flag" then
    local on = dir > 0
    if line[4] then on = not on end
    settings.flag1 = setBit(settings.flag1, line[3], on)
  elseif line[2] == "port" then
    local modes = { 1 }
    if hasCap(0x01) then modes[#modes + 1] = 2 end
    if hasCap(0x08) then modes[#modes + 1] = 3 end
    local pos = 1
    for i = 1, #modes do
      if modes[i] == portMode() then pos = i end
    end
    local m = modes[limit(pos + dir, 1, #modes)]
    settings.flag1 = setBit(settings.flag1, 0x08, m == 2)
    settings.flag1 = setBit(settings.flag1, 0x01, m == 3)
  elseif line[2] == "output" then
    local i = line[3]
    local list = outputChoices(i)
    local pos = nil
    for k = 1, #list do
      if list[k] == settings.outputs[i] then pos = k end
    end
    if pos == nil then pos = dir > 0 and 0 or #list + 1 end  -- not offered here: first / last
    settings.outputs[i] = list[limit(pos + dir, 1, #list)]
  end
end

local function handleEvent(event)
  local nextEvent = edit and EVT_VIRTUAL_INC or EVT_VIRTUAL_NEXT
  local prevEvent = edit and EVT_VIRTUAL_DEC or EVT_VIRTUAL_PREV
  if event == nextEvent then
    if edit then changeValue(1) else sel = limit(sel + 1, 1, #lines) end
  elseif event == prevEvent then
    if edit then changeValue(-1) else sel = limit(sel - 1, 1, #lines) end
  elseif event == EVT_VIRTUAL_ENTER then
    local line = lines[sel]
    if line and line[2] == "action" then
      if state == "idle" then line[3]() end
    elseif line and settings then
      edit = not edit
    end
  end
  local count = screenLines()
  if sel < top then top = sel end
  if sel >= top + count then top = sel - count + 1 end
end

local function draw()
  lcd.clear()
  drawTitle("FrSky ACCESS RX")
  local font = colorLcd and 0 or SMLSIZE
  local x = 2
  local y = colorLcd and 34 or 9
  local dy = colorLcd and 20 or 8
  local valueX = colorLcd and 150 or 70

  if not moduleOk then
    lcd.drawText(x, y + dy, "Select Multi FrSkyA", font + BLINK)
    return
  end
  if status ~= "" then
    lcd.drawText(LCD_W - 2, colorLcd and 5 or 0, status, (colorLcd and COLOR_THEME_PRIMARY2 or SMLSIZE + INVERS) + RIGHT)
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
    local yy = y + (i - top) * dy
    lcd.drawText(x, yy, lines[i][1], attr)
    lcd.drawText(valueX, yy, lineValue(lines[i]), attr)
  end
end

local function init()
  moduleOk = type(multiBuffer) == "function" and findModule()
  rebuildLines()
  if moduleOk then
    initBuffer()
    readInfo()
  end
end

local function run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif event == EVT_VIRTUAL_EXIT and not edit then
    if moduleOk then release() end
    return 2
  elseif event == EVT_VIRTUAL_EXIT then
    edit = false
  end
  if moduleOk then
    local had = settings
    local hadInfo = hwInfo
    pollRx()
    timers()
    sendQueue()
    if settings ~= had or hwInfo ~= hadInfo then rebuildLines() end
    handleEvent(event)
  end
  draw()
  return 0
end

return { init = init, run = run }
