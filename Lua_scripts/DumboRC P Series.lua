local toolName = "TNS|DumboRC P series|TNE"

-- EdgeTX/Multi bridge:
-- Multi_Buffer[4] is RF payload length, [5..12] is RF payload.
-- Multi rewrites the 00 00 token and checksum from its hop state.
-- Special1 payload: 18 00 00 arg_l arg_h 00. Args: 1 poll, 2 Set GY EPA, 3 Set FS.
-- Special2 payload: 28 00 00 gyro0 gyro1 gyro2 gyro3 00.
-- Gyro word: bit0 enable, bit1 phase, bits2..5 SEN channel selector
-- (0=NULL, 1..7=CH2..CH8), bits6..13 SEN, bits14..21 PCA,
-- bits22..29 AGS, bits30..31 servo frequency (67HZ,250HZ,50HZ,300HZ).

local PROTO_RLINK = 74
local SUB_DUMBORC_P = 4

local TX_READY = 4
local TX_PAYLOAD = 5
local TX_MAX_PAYLOAD = 8
local RX_READY = 14
local RX_PAYLOAD = 15
local RX_MAX_PAYLOAD = 32

local servoHz = { "67HZ", "250HZ", "50HZ", "300HZ" }

local gyro = { enable = 0, phase = 0, senCh = 0, sen = 0, pca = 0, ags = 0, flags = 0 }
local lines = {}
local sel = 1
local top = 1
local edit = false
local blink = 0
local moduleOk = false
local haveValues = false

local function limit(v, mn, mx)
  if v < mn then return mn end
  if v > mx then return mx end
  return v
end

local function byte(v)
  return limit(math.floor(tonumber(v) or 0), 0, 255)
end

local function percent(v)
  return limit(math.floor(tonumber(v) or 0), 0, 100)
end

local function gyroRaw(v)
  return percent(v) * 2
end

local function gyroUi(v)
  return limit(math.floor((tonumber(v) or 0) / 2), 0, 100)
end

local function screenLines()
  if LCD_W >= 480 then return limit(math.floor(((LCD_H or 272) - 46) / 20), 8, 12) end
  return 6
end

local function drawTitle(title)
  if LCD_W >= 480 and lcd.drawFilledRectangle then
    lcd.drawFilledRectangle(0, 0, LCD_W, 30, TITLE_BGCOLOR or 0)
    lcd.drawText(3, 5, title, MENU_TITLE_COLOR or 0)
  elseif lcd.drawScreenTitle then
    lcd.drawScreenTitle(title, 0, 0)
  else
    lcd.drawText(0, 0, title, INVERS)
  end
end

local function findModule()
  for i = 0, 1 do
    local m = model.getModule(i)
    if m and m["Type"] == 6 and m["protocol"] == PROTO_RLINK and m["subProtocol"] == SUB_DUMBORC_P then
      return true
    end
  end
  return false
end

local function initBuffer()
  multiBuffer(0, string.byte("R"))
  if multiBuffer(0) ~= string.byte("R") then
    error("Not enough memory!")
    return
  end
  multiBuffer(1, string.byte("L"))
  multiBuffer(2, string.byte("n"))
  multiBuffer(3, string.byte("k"))
  multiBuffer(TX_READY, 0)
  multiBuffer(RX_READY, 0)
end

local function release()
  for i = 0, RX_PAYLOAD + RX_MAX_PAYLOAD - 1 do
    multiBuffer(i, 0)
  end
end

local function sendPayload(payload)
  local len = #payload
  if len < 1 or len > TX_MAX_PAYLOAD or multiBuffer(TX_READY) ~= 0 then
    return false
  end
  for i = 1, TX_MAX_PAYLOAD do
    multiBuffer(TX_PAYLOAD + i - 1, 0)
  end
  for i = 1, #payload do
    multiBuffer(TX_PAYLOAD + i - 1, byte(payload[i]))
  end
  multiBuffer(TX_READY, len)
  return true
end

local function sendSpecial1(arg)
  return sendPayload({ 0x18, 0, 0, arg % 256, math.floor(arg / 256), 0 })
end

local function gyroSelector()
  if gyro.senCh < 2 or gyro.senCh > 8 then return 0 end
  return gyro.senCh - 1
end

local function sendGyro()
  local sen = gyroRaw(gyro.sen)
  local pca = gyroRaw(gyro.pca)
  local ags = gyroRaw(gyro.ags)
  local b0 = gyro.enable + gyro.phase * 2 + gyroSelector() * 4 + (sen % 4) * 64
  local b1 = math.floor(sen / 4) + (pca % 4) * 64
  local b2 = math.floor(pca / 4) + (ags % 4) * 64
  local b3 = math.floor(ags / 4) + limit(gyro.flags, 0, 3) * 64
  sendPayload({ 0x28, 0, 0, b0, b1, b2, b3, 0 })
end

local function decodeGyroBytes(b0, b1, b2, b3)
  b0 = byte(b0); b1 = byte(b1); b2 = byte(b2); b3 = byte(b3)
  gyro.enable = b0 % 2
  gyro.phase = math.floor(b0 / 2) % 2
  local selector = math.floor(b0 / 4) % 16
  gyro.senCh = selector == 0 and 0 or limit(selector + 1, 2, 8)
  gyro.sen = gyroUi(math.floor(b0 / 64) + (b1 % 64) * 4)
  gyro.pca = gyroUi(math.floor(b1 / 64) + (b2 % 64) * 4)
  gyro.ags = gyroUi(math.floor(b2 / 64) + (b3 % 64) * 4)
  gyro.flags = math.floor(b3 / 64) % 4
  haveValues = true
end

local function pollRx()
  local len = multiBuffer(RX_READY)
  if not len or len == 0 then return end
  if len > RX_MAX_PAYLOAD then
    multiBuffer(RX_READY, 0)
    return
  end

  local p = {}
  for i = 1, len do
    p[i] = multiBuffer(RX_PAYLOAD + i - 1)
  end
  multiBuffer(RX_READY, 0)

  if len == 7 and p[1] == 0x01 then
    decodeGyroBytes(p[3], p[4], p[5], p[6])
  end
end

local function chText()
  if gyro.senCh == 0 then return "NULL" end
  return "CH" .. gyro.senCh
end

local function rebuildLines()
  lines = {
    { "Poll RX", "action", function() sendSpecial1(1) end },
    { "Gyro", "toggle", "enable" },
    { "Phase", "toggle", "phase" },
    { "SEN Ch", "channel", "senCh" },
    { "SEN", "percent", "sen" },
    { "PCA", "percent", "pca" },
    { "AGS", "percent", "ags" },
    { "Servo Hz", "flags", "flags" },
    { "Send Gyro", "action", sendGyro },
    { "Set GY EPA", "action", function() sendSpecial1(2) end },
    { "Set FS", "action", function() sendSpecial1(3) end },
  }
end

local function lineValue(line)
  if line[2] == "action" then return ">" end
  if not haveValues then return "--" end
  if line[2] == "toggle" then return gyro[line[3]] == 0 and "Off" or "On" end
  if line[2] == "channel" then return chText() end
  if line[2] == "percent" then return tostring(percent(gyro[line[3]])) end
  if line[2] == "flags" then return servoHz[gyro.flags + 1] or tostring(gyro.flags) end
  return ""
end

local function changeValue(dir, fast)
  local line = lines[sel]
  if not line then return end
  local step = fast and 10 or 1
  haveValues = true

  if line[2] == "toggle" then
    gyro[line[3]] = gyro[line[3]] == 0 and 1 or 0
  elseif line[2] == "channel" then
    local vals = { 0, 2, 3, 4, 5, 6, 7, 8 }
    local pos = 1
    for i = 1, #vals do
      if vals[i] == gyro.senCh then pos = i end
    end
    gyro.senCh = vals[limit(pos + dir, 1, #vals)]
  elseif line[2] == "percent" then
    gyro[line[3]] = percent(gyro[line[3]] + dir * step)
  elseif line[2] == "flags" then
    gyro.flags = limit(gyro.flags + dir, 0, 3)
  end
end

local function handleEvent(event)
  if event == EVT_VIRTUAL_NEXT then
    if edit then changeValue(1, false) else sel = limit(sel + 1, 1, #lines) end
  elseif event == EVT_VIRTUAL_PREV then
    if edit then changeValue(-1, false) else sel = limit(sel - 1, 1, #lines) end
  elseif event == EVT_VIRTUAL_NEXT_PAGE then
    if edit then changeValue(1, true) else sel = limit(sel + screenLines(), 1, #lines) end
  elseif event == EVT_VIRTUAL_PREV_PAGE then
    if edit then changeValue(-1, true) else sel = limit(sel - screenLines(), 1, #lines) end
  elseif event == EVT_VIRTUAL_ENTER then
    local line = lines[sel]
    if line and line[2] == "action" then
      line[3]()
    elseif line then
      edit = not edit
    end
  end

  local count = screenLines()
  if sel < top then top = sel end
  if sel >= top + count then top = sel - count + 1 end
end

local function draw()
  lcd.clear()
  drawTitle("DumboRC P series")

  local font = LCD_W >= 480 and 0 or SMLSIZE
  local x = 2
  local y = LCD_W >= 480 and 34 or 9
  local dy = LCD_W >= 480 and 20 or 8
  local valueX = LCD_W >= 480 and 150 or 82

  if not moduleOk then
    if LCD_W >= 480 then
      lcd.drawText(x, y + dy, "Select Multi RadLink/Dumbo_P", font + BLINK)
    else
      lcd.drawText(x, y + dy, "Select RadLink", font + BLINK)
      lcd.drawText(x, y + dy * 2, "Dumbo_P", font + BLINK)
    end
    return
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
  moduleOk = findModule()
  rebuildLines()
  initBuffer()
end

local function run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif event == EVT_VIRTUAL_EXIT then
    release()
    return 2
  end
  pollRx()
  handleEvent(event)
  draw()
  return 0
end

return { init = init, run = run }
