local toolName = "TNS|DSM Frwd Prog v0.54-beta (MIN)|TNE"

--local ModelParam = ... 

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

local VERSION             = "v0.54-MIN"
local LANGUAGE            = "en"
local DSMLIB_PATH         = "/SCRIPTS/TOOLS/DSMLIB/"

local LOG_FILE            = "/LOGS/dsm_min_log.txt"
local MSG_FILE            = DSMLIB_PATH.."msg_fwdp_" .. LANGUAGE .. ".txt"
local MSG_FILE_MIN        = DSMLIB_PATH.."MIN_msg_fwdp_" .. LANGUAGE .. ".txt"

-- Phase
local PH_RX_VER, PH_TITLE, PH_TX_INFO, PH_LINES, PH_VALUES = 1, 2, 3, 4, 5
local PH_VAL_CHANGING, PH_VAL_WAIT, PH_VAL_CHNG_END        = 6, 7, 8
local PH_WAIT_CMD, PH_EXIT_REQ, PH_EXIT_DONE               = 9, 10, 11

-- Line Types
local LT_MENU             = 0x1C 
local LT_LIST, LT_LIST_VALIDATE, LT_LIST_TOG = 0x6C, 0x0C, 0x4C
local LT_VALUE_NOCHANGING = 0x60
local LT_VALUE_PERCENT, LT_VALUE_DEGREES = 0xC0, 0xE0

local Phase               = PH_RX_VER
local Waiting_RX          = 0

local Text                = {}
local List_Text           = {}
local List_Text_Img       = {}
local Flight_Mode         = { [0] = "Flight Mode" }
local RxName              = {}

local InactivityTime      = 0
local Value_Change_Step   = 0
local TX_Info_Step        = 0
local TX_Info_Type        = 0
local originalValue       = 0

local ctx = {
  SelLine = 0,      -- Current Selected Line
  EditLine = nil,   -- Current Editing Line
  CurLine = -1,     -- Current Line Requested/Parsed via h message protocol
  isReset = false   -- false when starting from scracts, true when starting from Reset
}

local MODEL = {
  modelName = "",            -- The name of the model comming from OTX/ETX
  modelOutputChannel = {},   -- Output information from OTX/ETX
  AirWingTailDesc = "",
  
  --TX_CH_TEXT = {},
  --PORT_TEXT = {},
  DSM_ChannelInfo = {}       -- Data Created by DSM Configuration Script
}

local Menu                = { MenuId = 0, Text = "", TextId = 0, PrevId = 0, NextId = 0, BackId = 0 }
local MenuLines           = {}
local RX                  = { Name = "", Version = "" }

local logFile             = nil
local logCount            = 0

local LCD_X_LINE_TITLE    = 0
local LCD_X_LINE_VALUE    = 75

local LCD_W_BUTTONS       = 19
local LCD_H_BUTTONS       = 10

local LCD_X_MAX           = 128
local LCD_X_RIGHT_BUTTONS = LCD_X_MAX - LCD_W_BUTTONS - 1

local LCD_Y_LINE_HEIGHT   = 7
local LCD_Y_LOWER_BUTTONS = (8 * LCD_Y_LINE_HEIGHT) + 2

local TEXT_ATTR           = SMLSIZE

local function LOG_open()
  logFile = io.open(LOG_FILE, "w")   -- Truncate Log File
end

local function LOG_write(...)
  if (logFile == nil) then LOG_open() end
  local str = string.format(...)
  io.write(logFile, str)
end

local function LOG_close()
  if (logFile ~= nil) then io.close(logFile) end
end

---------------- DSM Values <-> Int16 Manipulation --------------------------------------------------------

local function int16_LSB(number) -- Less Significat byte
  local r, x = bit32.band(number, 0xFF)
  return r
end

local function int16_MSB(number) -- Most signifcant byte
  return bit32.rshift(number, 8)
end

local function Dsm_to_Int16(lsb, msb) -- Componse an Int16 value
  return bit32.lshift(msb, 8) + lsb
end

local function Dsm_to_SInt16(lsb, msb) -- Componse a SIGNED Int16 value
  local value = bit32.lshift(msb, 8) + lsb
  if value >= 0x8000 then             -- Negative value??
    return value - 0x10000
  end
  return value
end

local function sInt16ToDsm(value) -- Convent to SIGNED DSM Value
  if value < 0 then
    value = 0x10000 + value
  end
  return value
end

------------------------------------------------------------------------------------------------------------
local function Get_Text(index)
  local out = Text[index] or string.format("Unknown_%X", index) 
  if (index >= 0x8000) then
    out = Flight_Mode[0]
  end 
  return out
end

local function Get_Text_Value(index)
  local out = List_Text[index] or Get_Text(index)
  return out
end
------------------------------------------------------------------------------------------------------------
local function Get_RxName(index)
  local out = RxName[index] or string.format("Unknown_%X", index) 
  return out
end
------------------------------------------------------------------------------------------------------------
local function DSM_Release()
  multiBuffer(0, 0)
  Phase = PH_EXIT_DONE
end
------------------------------------------------------------------------------------------------------------
local function DSM_Send(...)
  local arg = { ... }
  for i = 1, #arg do
    multiBuffer(3 + i, arg[i])
  end
  multiBuffer(3, 0x70 + #arg)
end
------------------------------------------------------------------------------------------------------------

function ChangePhase(newPhase)
  Phase = newPhase
  Waiting_RX = 0
end

local function Value_Add(dir)
  local line = MenuLines[ctx.SelLine]

  Speed = getRotEncSpeed()
  if Speed == ROTENC_MIDSPEED then
    line.Val = line.Val + (5 * dir)
  elseif Speed == ROTENC_HIGHSPEED then
    line.Val = line.Val + (15 * dir)
  else
    line.Val = line.Val + dir
  end

  if line.Val > line.Max then
    line.Val = line.Max
  elseif line.Val < line.Min then
    line.Val = line.Min
  end

  ChangePhase(PH_VAL_CHANGING)
  Value_Change_Step = 0
end
------------------------------------------------------------------------------------------------------------

local function GotoMenu(menuId, lastSelectedLine)
  Menu.MenuId = menuId
  ctx.SelLine = lastSelectedLine
  -- Request to load the menu Again
  ChangePhase(PH_TITLE)
end

local function isSelectable(line)
  if (line.TextId == 0x00CD) then return true end                          -- Exceptiom: Level model and capture attitude
  if (line.Type == LT_MENU and line.ValId == line.MenuId) then return false end -- Menu to same page
  if (line.Type ~= LT_MENU and  line.Max == 0) then return false end            -- Read only data line 
  if (line.Type ~= 0 and line.TextId < 0x8000) then return true end          -- Not Flight Mode
  return false;
end

local function isListLine(line) 
  return line.Type==LT_LIST or line.Type == LT_LIST_VALIDATE or line.Type == LT_LIST_TOG
end

local function DSM_Menu(event)
  if event == EVT_VIRTUAL_EXIT then
    if Phase == PH_RX_VER then
      DSM_Release() -- Exit program
    else
      if ctx.EditLine ~= nil then   -- Editing a Line, need to  restore original value
        MenuLines[ctx.EditLine].Val = originalValue
        event = EVT_VIRTUAL_ENTER
      else
        ChangePhase(PH_EXIT_REQ)
      end
    end
  end

  if Phase == PH_RX_VER then return end -- nothing else to do 

  if event == EVT_VIRTUAL_NEXT then
    if ctx.EditLine ~= nil then
      Value_Add(1)
    else
      -- not changing a value

      if ctx.SelLine < 7 then -- On a regular line
        local num = ctx.SelLine -- Find the prev selectable 
        for i = ctx.SelLine + 1, 6, 1 do
          local line = MenuLines[i]
          if isSelectable(line) then
            ctx.SelLine = i
            break
          end
        end
        if num == ctx.SelLine then       -- No Selectable Line
          if Menu.NextId ~= 0 then
            ctx.SelLine = 7 -- Next 
          elseif Menu.PrevId ~= 0 then
            ctx.SelLine = 8 -- Prev
          end
        end
      elseif Menu.PrevId ~= 0 then
        ctx.SelLine = 8 -- Prev
      end
    end
  elseif event == EVT_VIRTUAL_PREV then
    if ctx.EditLine ~= nil then -- In Edit Mode
      Value_Add(-1)
    else
      if ctx.SelLine == 8 and Menu.NextId ~= 0 then
        ctx.SelLine = 7 -- Next 
      elseif ctx.SelLine > 0 then
        if ctx.SelLine > 6 then
          ctx.SelLine = 7 --NEXT 
        end
        local num = ctx.SelLine -- Find Prev Selectable line
        for i = ctx.SelLine - 1, 0, -1 do
          local line = MenuLines[i]
          if isSelectable(line) then
            ctx.SelLine = i
            break
          end
        end
        if num == ctx.SelLine then   -- No Selectable Line
          if (Menu.BackId > 0) then 
            ctx.SelLine = -1 -- Back 
          end
        end
      else
        ctx.SelLine = -1   -- Back
      end
    end
  elseif event == EVT_VIRTUAL_ENTER_LONG then
    if ctx.EditLine ~= nil then
      -- reset the value to default
      --if MenuLines[ctx.SelLine].Type ~= LIST_MENU_NOCHANGING then
      MenuLines[ctx.SelLine].Val = MenuLines[ctx.SelLine].Def
      ChangePhase(PH_VAL_CHANGING)
      Value_Change_Step = 0
      --end
    end
  elseif event == EVT_VIRTUAL_ENTER then
    if ctx.SelLine == -1 then    -- Back
      GotoMenu(Menu.BackId, 0x80)
    elseif ctx.SelLine == 7 then -- Next
      GotoMenu(Menu.NextId, 0x82)
    elseif ctx.SelLine == 8 then -- Prev
      GotoMenu(Menu.PrevId, 0x81)
    elseif ctx.SelLine >= 0 and MenuLines[ctx.SelLine].Type == LT_MENU then
      GotoMenu(MenuLines[ctx.SelLine].ValId, ctx.SelLine)  -- ValId is the next menu
    else
      -- value entry
      if ctx.EditLine ~= nil then
        ctx.EditLine = nil   -- Done Editting
        Value_Change_Step = 0
        ChangePhase(PH_VAL_CHNG_END)
      else   -- Start Editing
        ctx.EditLine = ctx.SelLine
        originalValue = MenuLines[ctx.SelLine].Val
        ChangePhase(PH_VAL_WAIT)
      end
    end
  end
end
------------------------------------------------------------------------------------------------------------
local function SendTxInfo(portNo)
  -- TxInfo_Type=0    : AR636 Main Menu (Send port/Channel info + SubTrim + Travel)
  -- TxInfo_Type=1    : AR630-637 Famly Main Menu  (Only Send Port/Channel usage Msg 0x20)
  -- TxInfo_Type=1F   : AR630-637 Initial Setup/Relearn Servo Settings (Send port/Channel info + SubTrim + Travel +0x24/Unknown)


  if (TX_Info_Step == 0) then  
      -- AR630 family: Both TxInfo_Type (ManinMenu=0x1,   Other First Time Configuration = 0x1F)
      local info = MODEL.DSM_ChannelInfo[portNo]
      DSM_Send(0x20, 0x06, portNo, portNo, info[0],info[1])
      LOG_write("DSM_TxInfo_20(Port=#%d, Port Use)\n", portNo)

      if (TX_Info_Type == 0x1F) then
          TX_Info_Step = 1
      elseif (TX_Info_Type == 0x00) then
          TX_Info_Step = 2
      end 
  elseif (TX_Info_Step == 1) then
    local info = MODEL.modelOutputChannel[portNo]
    local leftTravel =   math.abs(math.floor(info.min/10))
    local rightTravel =  math.abs(math.floor(info.max/10))

    DSM_Send(0x23, 0x06, 0x00, leftTravel, 0x00, rightTravel)
    LOG_write("DSM_TxInfo_23(Port=#%d,ServoTravel(L=%d - R=%d))\n", portNo,leftTravel,rightTravel)

    TX_Info_Step = 2
  elseif (TX_Info_Step == 2) then
    local data = {[0]= -- Start at 0
      {[0]= 0x0, 0x00, 0x07, 0xFF }, -- Ch1 Thr:     0 00 07 FF   Subtrim ??
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch2 Ail:     0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch3 Elev:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch4 Rud:     0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch5 Gear:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch6 Aux1:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch7 Aux2:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch8 Aux3:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch9 Aux4:    0 8E 07 72   Subtrim 0
      {[0]= 0x0, 0x8E, 0x07, 0x72 }, -- Ch10 Aux5:   0 8E 07 72   Subtrim 0
    }
    local info = data[portNo]
    local b1,b2,b3,b4 = info[0], info[1], info[2], info[3]

    DSM_Send(0x21, 0x06, b1,b2,b3,b4) -- Port is not send anywhere, since the previous 0x20 type message have it.
    LOG_write("DSM_TxInfo_21(Port=#%d, SubTrim)\n", portNo)

    if (TX_Info_Type == 0x00) then
        TX_Info_Step = 5 -- End Step 
    else 
        TX_Info_Step = 3
    end
  elseif (TX_Info_Step == 3) then
     LOG_write("DSM_TxInfo_24?(Port=#%d)\n", portNo)
     DSM_Send(0x24, 0x06, 0x00, 0x83, 0x5A, 0xB5) -- Still Uknown
     TX_Info_Step = 4
  elseif (TX_Info_Step == 4) then
     LOG_write("DSM_TxInfo_24?(Port=#%d)\n", portNo)
     DSM_Send(0x24, 0x06, 0x06, 0x80, 0x25, 0x4B)  -- Still Uknown
     TX_Info_Step = 5
  elseif (TX_Info_Step == 5) then
     LOG_write("DSM_TxInfo_22(Port=#%d, END of Data)\n", portNo)
     DSM_Send(0x22, 0x04, 0x00, 0x00)
     TX_Info_Step = 0  -- Done!!
  end

  if (TX_Info_Step > 0) then
    Waiting_RX = 0 -- keep Transmitig
  end
end

local function DSM_SendRequest()
  --LOG_write("DSM_SendRequest  Phase=%d\n",Phase)
  -- Need to send a request
  if Phase == PH_RX_VER then   -- request RX version
    DSM_Send(0x11, 0x06, 0x00, 0x14, 0x00, 0x00)
    LOG_write("GetVersion()\n")
  elseif Phase == PH_WAIT_CMD then     -- keep connection open
    DSM_Send(0x00, 0x04, 0x00, 0x00)
  elseif Phase == PH_TITLE then     -- request menu title
    local menuId = Menu.MenuId
    if menuId == 0 then
      DSM_Send(0x12, 0x06, 0x00, 0x14, 0x00, 0x00) -- first menu only
    else
      DSM_Send(0x16, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, ctx.SelLine)
      if (menuId == 0x0001) then -- Executed Save&Reset menu
        Phase = PH_RX_VER
        ctx.isReset = true
      end
    end
    LOG_write("GetMenu(M=0x%04X,L=%d)\n", menuId, ctx.SelLine)
  elseif Phase == PH_TX_INFO then -- TX Info
    SendTxInfo(ctx.CurLine)
  elseif Phase == PH_LINES then -- request menu lines
    local menuId = Menu.MenuId
    if ctx.CurLine == -1 then
      DSM_Send(0x13, 0x04, int16_MSB(menuId), int16_LSB(menuId)) -- GetFirstLine
    else
      DSM_Send(0x14, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, ctx.CurLine) -- line X
    end
    LOG_write("GetLines(LastLine=%d)\n", ctx.CurLine)
  elseif Phase == PH_VALUES then -- request menu values
    local menuId = Menu.MenuId
    local valId  = MenuLines[ctx.CurLine].ValId
    DSM_Send(0x15, 0x06,
      int16_MSB(menuId), int16_LSB(menuId),
      int16_MSB(valId), int16_LSB(valId))
    LOG_write("GetValues(LastVId=0x%04X)\n", valId)
  elseif Phase == PH_VAL_CHANGING then     -- send new value: Two steps, Update & Validate
    local line = MenuLines[ctx.SelLine]
    local valId = line.ValId
    if Value_Change_Step == 0 then
      local value = sInt16ToDsm(line.Val)
      DSM_Send(0x18, 0x06,
        int16_MSB(valId), int16_LSB(valId),
        int16_MSB(value), int16_LSB(value))  -- send current values
      LOG_write("ChangeValue(VId=0x%04X,Val=%d)\n", valId, value)

      if line.Type == LT_LIST_VALIDATE then -- Incremental Validation??
        Value_Change_Step = 1
        Waiting_RX = 0 -- Do SEND in the next step
      end
    else
      -- Validate Value
      DSM_Send(0x19, 0x04, int16_MSB(valId), int16_LSB(valId))
      Value_Change_Step = 0
      Phase = PH_VAL_WAIT
      LOG_write("ValidateValue(VId=0x%04X)\n", valId)
    end
  elseif Phase == PH_VAL_CHNG_END then  
    DSM_Send(0x1B, 0x04, 0x00, int16_LSB(ctx.SelLine))
    Value_Change_Step = 0
    Phase = PH_WAIT_CMD
    LOG_write("ValueChangeEnd(L=%d)\n", ctx.SelLine)
  elseif Phase == PH_VAL_WAIT then
    -- Value Changing Wait
    DSM_Send(0x1A, 0x04, 0x00, int16_LSB(ctx.SelLine))
    LOG_write("ValueChangeWait(L=%d)\n", ctx.SelLine)
  elseif Phase == PH_EXIT_REQ then -- EXIT Request 
    DSM_Send(0x1F, 0x02, 0xAA) 
  end
end

local function DSM_ProcessResponse()
  local cmd = multiBuffer(11)
  -- LOG_write("DSM_ProcessResponse BEGIN: Cmd=%x\n",cmd)
  if cmd == 0x01 then    -- read version
    RX.Name = Get_RxName(multiBuffer(13))
    RX.Version = multiBuffer(14) .. "." .. multiBuffer(15) .. "." .. multiBuffer(16)
    --ctx.isReset = false     -- no longer resetting
    Menu.MenuId = 0
    Phase = PH_TITLE
    LOG_write("Version: %s %s\n", RX.Name, RX.Version)
  elseif cmd == 0x02 then     -- read menu title
    local menu  = Menu

    menu.MenuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    menu.TextId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    menu.Text   = Get_Text(menu.TextId)
    menu.PrevId = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    menu.NextId = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))
    menu.BackId = Dsm_to_Int16(multiBuffer(20), multiBuffer(21))

    for i = 0, 6 do     -- clear menu
      MenuLines[i] = { MenuId = 0, Type = 0, TextId = 0, ValId = 0, Min = 0, Max = 0, Def = 0, Val = nil, Unit, Step }
    end
    ctx.CurLine = -1
    ctx.SelLine = -1     -- highlight Back

    LOG_write("Menu: Mid=0x%04X \"%s\"\n", menu.MenuId, menu.Text)

    if (menu.MenuId == 0x0001) then  -- Still in RESETTING MENU???
      --menu.MenuId = 0
      Phase = PH_RX_VER
    else
      Phase = PH_LINES
    end
  elseif cmd == 0x03 then     -- read menu lines
    local i      = multiBuffer(14)
    local type   = multiBuffer(15)
    local line   = MenuLines[i]

    ctx.CurLine  = i

    line.lineNum = i
    line.MenuId  = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    line.Type    = type
    line.TextId  = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    line.Text    = Get_Text(line.TextId)
    line.ValId   = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))

    -- Singed int values
    line.Min     = Dsm_to_SInt16(multiBuffer(20), multiBuffer(21))
    line.Max     = Dsm_to_SInt16(multiBuffer(22), multiBuffer(23))
    line.Def     = Dsm_to_SInt16(multiBuffer(24), multiBuffer(25))

    if line.Type == LT_MENU then
      -- nothing to do on menu entries
    elseif isListLine(line) then
      line.Val = nil                     --line.Def - line.Min -- use default value not sure if needed
      line.Def = line.Min                -- pointer to the start of the list in Text
      line.Max = line.Max - line.Min     -- max index
      line.Min = 0                       -- min index
    else                                 -- default to numerical value
      line.Val = nil                     --line.Def -- use default value not sure if needed
      if (line.Min == 0 and line.Max == 100) or (line.Min == -100 and line.Max == 100) or
         (line.Min == 0 and line.Max == 150) or (line.Min == -150 and line.Max == 150) then
          line.Type = LT_VALUE_PERCENT -- Override to Value Percent
      end 
    end

    if ctx.SelLine == -1 and isSelectable(line) then -- Auto select first selectable line of the menu
      ctx.SelLine = ctx.CurLine
    end

    LOG_write("Line: #%d Vid=0x%04X T=0x%02X \"%s\"\n", i, line.ValId, type, line.Text)
    Phase = PH_LINES
  elseif cmd == 0x04 then     -- read menu values
    -- Identify the line and update the value
    local valId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    local value = Dsm_to_SInt16(multiBuffer(16), multiBuffer(17))     --Signed int

    local updatedLine = nil
    for i = 0, 6 do     -- Find the menu line for this value
      local line = MenuLines[i]
      if line.Type ~= 0 then
        if line.Type ~= LT_MENU and line.ValId == valId then         -- identifier of ValueId stored in the line
          line.Val = value
          ctx.CurLine = i
          updatedLine = line

          local valueTxt = value
          if isListLine(line) then
            valueTxt = Get_Text_Value(line.Def + value) .. "  [" .. value .. "]"
          end

          LOG_write("Update Value: #%d  VId=0x%04X Value=%s\n", i, valId, valueTxt)
          break
        end
      end
    end

    if (updatedLine == nil) then
      LOG_write("Cannot Find Line for ValueId=%x\n", valId)
    end
    Phase = PH_VALUES
  elseif cmd == 0x05 then -- Request TX info
    ctx.CurLine = multiBuffer(12)
    TX_Info_Type = multiBuffer(13)
    TX_Info_Step = 0
    Phase = PH_TX_INFO
    LOG_write("TXInfoReq: Port=%d T=0x%02X\n", ctx.CurLine, TX_Info_Type)
  elseif cmd == 0xA7 then -- answer to EXIT command
    DSM_Release()
  elseif cmd == 0x00 and Phase == PH_VAL_CHANGING then
    Phase = PH_VAL_WAIT
  end

  --LOG_write("DSM_ProcessResponse END: Cmd=%x\n",cmd)
  return cmd
end


local function DSM_Send_Receive()
  if Waiting_RX == 0 then
    Waiting_RX = 1
    DSM_SendRequest()
    multiBuffer(10, 0x00);
    InactivityTime = getTime() + 200 -- Reset Inactivity timeout
    -- -- -- -- -- -- -- -- -- -- -- -- receive part -- -- -- -- -- -- -- -- -- -- -- -- --
  elseif multiBuffer(10) == 0x09 then
    local cmd = DSM_ProcessResponse()
    -- Data processed
    multiBuffer(10, 0x00)
    if (cmd > 0x00) then -- Any non NULL response
      -- Only change to SEND mode if we received a valid response  (Ignore NULL Responses, that are really heartbeat i most cases)
      Waiting_RX = 0
      InactivityTime = getTime() + 200   -- Reset Inactivity timeout
    end
  else                                   -- No Send or Receive,
    -- Check if enouth time has passed from last transmit/receive activity
    if getTime() > InactivityTime then
      InactivityTime = getTime() + 200
      Waiting_RX = 0   -- Switch to Send mode to send heartbeat

      if Phase == PH_EXIT_REQ then
        DSM_Release()
      end

      if Phase ~= PH_RX_VER and Phase ~= PH_VAL_WAIT then
        Phase = PH_WAIT_CMD
      end
    end
  end
end
------------------------------------------------------------------------------------------------------------

local function showBitmap(x, y, imgDesc)
  local f = string.gmatch(imgDesc, '([^%|]+)')   -- Iterator over values split by '|'
  local imgName, imgMsg = f(), f()

  f = string.gmatch(imgMsg or "", '([^%:]+)')   -- Iterator over values split by ':'
  local p1, p2 = f(), f()

  lcd.drawText(x, y, p1 or "", TEXT_ATTR)                     -- Alternate Image MSG
  lcd.drawText(x, y + LCD_Y_LINE_HEIGHT, p2 or "", TEXT_ATTR) -- Alternate Image MSG
end


local function drawButton(x, y, text, active)
  local attr = TEXT_ATTR
  if (active) then attr = attr + INVERS end
  lcd.drawText(x, y, text, attr)
end

local ver_rx_count = 0

local function DSM_Display()
  lcd.clear()
  --Draw RX Menu
  if Phase == PH_RX_VER then
    lcd.drawText(1, 0, "DSM Frwd Prog "..VERSION, INVERS)

    local msgId = 0x300 -- Waiting for RX
    if (ctx.isReset) then msgId=0x301 end -- Waiting for Reset
    lcd.drawText(0, 3 * LCD_Y_LINE_HEIGHT, Get_Text(msgId), BLINK) 
    return
  end

    -- display Program version or RX version
  local msg = RX.Name .. " v" .. RX.Version
  if (ver_rx_count < 100) then
      msg = RX.Name .. " v" .. RX.Version
      ver_rx_count = ver_rx_count + 1
  else
      msg = "Frwd Prog "..VERSION
      ver_rx_count = ver_rx_count + 1
      if (ver_rx_count > 200) then ver_rx_count=0 end
  end
  lcd.drawText(30, LCD_Y_LOWER_BUTTONS, msg, TEXT_ATTR) 

  if Menu.MenuId == 0 then return end; -- No Title yet

  -- Got a Menu
  lcd.drawText(1, 0, Menu.Text, TEXT_ATTR + INVERS)

  local y = LCD_Y_LINE_HEIGHT + 2
  for i = 0, 6 do
    local attrib = TEXT_ATTR
    if (i == ctx.SelLine) then attrib = attrib + INVERS end     -- Selected Line

    local line = MenuLines[i]

    if line ~= nil and line.Type ~= 0 then
      local heading = Get_Text(line.TextId)

      if (line.TextId >= 0x8000) then     -- Flight mode
        heading = "   " .. Flight_Mode[0] .. " " 
        if (line.Val==nil) then heading = heading .. "--" else heading = heading .. ((line.Val or 0) + 1) end
      else
        local text = "-"
        if line.Type ~= LT_MENU then       -- list/value
          if line.Val ~= nil then
            if isListLine(line) then
              local textId = line.Val + line.Def
              text = Get_Text_Value(textId)

              local imgDesc = List_Text_Img[textId]

              if (imgDesc and i == ctx.SelLine) then             -- Optional Image and Msg for selected value
                showBitmap(0, 20, imgDesc)
              end
            elseif (line.Type == LT_VALUE_PERCENT) then
              text = line.Val .. " %"
            elseif (line.Type == LT_VALUE_DEGREES) then 
              text = line.Val .. " @"
            else
              text = line.Val
            end
          end -- if is Value

          if (ctx.EditLine == i) then  -- Editing a Line
            attrib = BLINK + INVERS + TEXT_ATTR
          end
          lcd.drawText(LCD_X_MAX, y, text, attrib + RIGHT) -- display value
          attrib = TEXT_ATTR
        end
      end  -- Flight mode
      lcd.drawText(0, y, heading, attrib) -- display text
    end
    y = y + LCD_Y_LINE_HEIGHT
  end     -- for

  if Menu.BackId ~= 0 then
    drawButton(LCD_X_RIGHT_BUTTONS, 0, "Back", ctx.SelLine == -1)
  end

  if Menu.NextId ~= 0 then
    drawButton(LCD_X_RIGHT_BUTTONS, LCD_Y_LOWER_BUTTONS, "Next", ctx.SelLine == 7)
  end

  if Menu.PrevId ~= 0 then
    drawButton(0, LCD_Y_LOWER_BUTTONS, "Prev", ctx.SelLine == 8)
  end

end


local function load_msg_from_file(fileName, mem, Text, List_Text, List_Text_Img, RxName, Flight_Mode)
  local function rtrim(s)
    local n = string.len(s)
    while n > 0 and string.find(s, "^%s", n) do n = n - 1 end
    return string.sub(s, 1, n)
  end

  --print(string.format("Loading messages from [%s]",fileName))
  local dataFile = io.open(fileName, "r")   -- read File
  -- cannot read file???
  assert(dataFile, "Cannot load Message file:" .. fileName)

  local data = io.read(dataFile, mem * 1024) -- read up to 10k characters (newline char also counts!)
  io.close(dataFile)

  collectgarbage("collect")

  local lineNo = 0
  for line in string.gmatch(data, "[^\r\n]+") do
    lineNo = lineNo + 1
    --print(string.format("Line [%d]: %s",lineNo,line))

    -- Remove Comments
    local s = string.find(line, "--", 1, true)
    if (s ~= nil) then
      line = string.sub(line, 1, s - 1)
    end

    line = rtrim(line)

    if (string.len(line) > 0) then
      local a, b, c = string.match(line, "%s*(%a*)%s*|%s*(%w*)%s*|(.*)%s*")
      --print(string.format("[%s] [%s] [%s]",a,b,c))
      if (a ~= nil) then
        local index = tonumber(b)

        if (index == nil) then
          assert(false, string.format("%s:%d: Invalid Hex num [%s]", fileName, lineNo, b))
        elseif (a == "T") then
          Text[index] = c
        elseif (a == "LT") then
          List_Text[index] = c
        elseif (a == "LI") then
          List_Text_Img[index] = c
        elseif (a == "FM") then
          Flight_Mode[0] = c
        elseif (a == "RX") then
          RxName[index] = c
        else
          assert(false, string.format("%s:%d: Invalid Line Type [%s]", fileName, lineNo, a))
        end
      end
    end
    if (lineNo % 50 == 0) then
      collectgarbage("collect")
    end
  end -- For

  --print(string.format("Loaded [%d] messages",lineNo))
  data = nil
end


local function clean_msg(Text, Flight_Mode)
  local function clean_line(c)
    if (c==nil) then return c end
    local pos
    c, pos = string.gsub(c, "/b$", "")
    c, pos = string.gsub(c, "/c$", "")
    c, pos = string.gsub(c, "/r$", "")
    c, pos = string.gsub(c, "/p$", "")
    c, pos = string.gsub(c, "/m$", "")
    return c
  end

  -- Clean the line of special markers that are only used in color vesion
  for i = 0, 0x0300 do
    Text[i] = clean_line(Text[i])
    collectgarbage("collect")
  end

  for i = 0, #Flight_Mode do
    -- Clean the line of special markers that are only used in color vesion
    Flight_Mode[i] = clean_line(Flight_Mode[i])
  end
end


local function DSM_Init_Model()
  MODEL.DSM_ChannelInfo= {[0]= -- Start array at position 0
                        {[0]=  0x00, 0x40},    -- Ch1 Thr  (0x40)
                        {[0]=  0x00, 0x01},    -- Ch2 Ail  (0x01)
                        {[0]=  0x00, 0x02},    -- Ch2 ElE  (0x02)
                        {[0]=  0x00, 0x04},    -- Ch4 Rud  (0x04)
                        {[0]=  0x00, 0x00},    -- Ch5 Gear (0x00)
                        {[0]=  0x00, 0x00},    -- Ch6 Aux1 (0x00)
                        {[0]=  0x00, 0x00},   -- Ch7 Aux2 (0x00)
                        {[0]=  0x00, 0x00},   -- Ch8 Aux3 (0x00)
                        {[0]=  0x00, 0x00},   -- Ch9 Aux4 (0x00)
                        {[0]=  0x00, 0x00}    -- Ch10 Aux5 (0x00)
                    }

  MODEL.modelOutputChannel = {[0]=
                       {min=1000, max=1000},  -- Ch1
                       {min=1000, max=1000},  -- Ch2
                       {min=1000, max=1000},  -- Ch3
                       {min=1000, max=1000},  -- Ch4
                       {min=1000, max=1000},  -- Ch5
                       {min=1000, max=1000},  -- Ch6
                       {min=1000, max=1000},  -- Ch7
                       {min=1000, max=1000},  -- Ch8
                       {min=1000, max=1000},  -- Ch9
                       {min=1000, max=1000}   -- Ch10
                    }
end

------------------------------------------------------------------------------------------------------------
-- Init
local function DSM_Init()
  LOG_open()
  LOG_write("-------- NEW SESSION --------------------\n")

  DSM_Init_Model()

  --[[
  if (ModelParam~=nil) then
    LOG_write("Got MODEL PARAMETER... copying\n")
    MODEL.DSM_ChannelInfo = ModelParam.DSM_ChannelInfo
  else
    LOG_write("NO-PARMETER --- Create DEFAULT")
  end
  --]]

  collectgarbage("collect")
  LOG_write("Mem before msg =%d\n",collectgarbage("count"))
  load_msg_from_file(MSG_FILE, 10, Text, List_Text, List_Text_Img, RxName, Flight_Mode)
  collectgarbage("collect")
  LOG_write("Mem after msg =%d\n",collectgarbage("count"))
  load_msg_from_file(MSG_FILE_MIN, 4, Text, List_Text, List_Text_Img, RxName, Flight_Mode)
  collectgarbage("collect")
  LOG_write("Mem after msg2 =%d\n",collectgarbage("count"))
  clean_msg(Text,Flight_Mode)
  collectgarbage("collect")

  --Set protocol to talk to
  multiBuffer(0, string.byte('D'))
  --test if value has been written
  if multiBuffer(0) ~= string.byte('D') then
    error("Not enough memory!")
    return 2
  end

  --Init TX buffer
  multiBuffer(3, 0x00)
  --Init RX buffer
  multiBuffer(10, 0x00)
  --Init telemetry
  multiBuffer(0, string.byte('D'))
  multiBuffer(1, string.byte('S'))
  multiBuffer(2, string.byte('M'))


  if (LCD_W > 128) then
    TEXT_ATTR = 0
    LCD_Y_LINE_HEIGHT = 25
    LCD_X_MAX         = 300
    LCD_X_RIGHT_BUTTONS = LCD_X_MAX - 30

    LCD_Y_LOWER_BUTTONS = (8 * LCD_Y_LINE_HEIGHT) + 2
  end


end
------------------------------------------------------------------------------------------------------------
-- Main
local function DSM_Run(event)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  else
    DSM_Display()
    DSM_Menu(event)
    DSM_Send_Receive()
  end
  if Phase == PH_EXIT_DONE then
    LOG_close()
    return 2
  else
    return 0
  end
end

return { init = DSM_Init, run = DSM_Run }
