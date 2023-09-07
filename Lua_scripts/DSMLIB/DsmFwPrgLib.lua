---- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX/EdgeTx                                           #
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

------------------------------------------------------------------------------
-- This script library is a rewrite of the original DSM forward programming Lua 
-- Script.  The goal is to make it easier to understand, mantain, and to  
-- separate the GUI from the DSM Forward  programming engine/logic
-- in this way, GUIs can evolve independent. OpenTX Gui, EdgeTx GUI, Small Radios, etc.

-- Code is based on the code/work by: Pascal Langer (Author of the Multi-Module)  
-- Rewrite/Enhancements By: Francisco Arzu 
-- 
------------------------------------------------------------------------------
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


local Log, menuLib, modelLib, DEBUG_ON = ... -- Get Debug_ON from parameters.  -- 0=NO DEBUG, 1=HIGH LEVEL 2=MORE DETAILS 
local LIB_VERSION = "0.55"
local MSG_FILE = "/SCRIPTS/TOOLS/DSMLIB/msg_fwdp_en.txt"

local PHASE = menuLib.PHASE
local LINE_TYPE = menuLib.LINE_TYPE

local CH_TYPE = modelLib.CH_TYPE
local CH_MIX_TYPE = modelLib.CH_MIX_TYPE

local DISP_ATTR = menuLib.DISP_ATTR
local DSM_Context =  menuLib.DSM_Context
local MODEL = modelLib.MODEL

local MAX_MENU_LINES = menuLib.MAX_MENU_LINES
local BACK_BUTTON    = menuLib.BACK_BUTTON
local NEXT_BUTTON    = menuLib.NEXT_BUTTON
local PREV_BUTTON    = menuLib.PREV_BUTTON


local SEND_TIMEOUT = 2000 / 10 --  Home many 10ms intervals to wait on sending data to tx to keep connection open   (2s)
local TXInactivityTime = 0     -- Next time to do heartbeat after inactivity 
local RXInactivityTime = 0     -- To check RX disconnection 


local TxInfo_Type = 0
local TxInfo_Step = 0
local Change_Step = 0

local IS_EDGETX = false 

------------------------------------------------------------------------------------------------------------

local function multiBuffer2String() -- used for debug
    local i
    local rxAnswer = "RX:"
    for i = 10, 25 do
        rxAnswer = rxAnswer .. string.format(" %02X", multiBuffer(i))
    end
    return rxAnswer
end

---------------- DSM Values <-> Int16 Manipulation --------------------------------------------------------

local function int16_LSB(number)  -- Less Significat byte
    local r,x = bit32.band(number, 0xFF)
    return r
end

local function int16_MSB(number) -- Most signifcant byte
    return bit32.rshift(number, 8)
end

local function Dsm_to_Int16(lsb, msb)  -- Componse an Int16 value
    return bit32.lshift(msb, 8) + lsb
end

local function Dsm_to_SInt16(lsb,msb) -- Componse a SIGNED Int16 value
    local value = bit32.lshift(msb, 8) + lsb
    if value >= 0x8000 then  -- Negative value??
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
local function DSM_send(...)
    local arg = { ... }
   
    for i = 1, #arg do
        multiBuffer(3 + i, arg[i])
    end
    multiBuffer(3, 0x70 + #arg)


    if (DEBUG_ON > 1) then 
        local str = ""
        for i = 1, #arg do
            str = str .. string.format("%02X ", arg[i]) 
        end
        LOG_write("DSM_SEND: [%s]\n", str) 
    end
end



-------------------------------------------------------------------------------------------------
local function DSM_StartConnection()
    if (DEBUG_ON) then Log.LOG_write("DSM_StartConnection()\n") end
    
    --Set protocol to talk to
    multiBuffer( 0, string.byte('D') )
    --test if value has been written
    if multiBuffer( 0 ) ~=  string.byte('D') then
    if (DEBUG_ON) then Log.LOG_write("Not Enouth memory\n") end
        error("Not enough memory!")
        return 2
    end
    --Init TX buffer
    multiBuffer( 3, 0x00 )
    --Init RX buffer
    multiBuffer( 10, 0x00 )
    --Init telemetry
    multiBuffer( 0, string.byte('D') )
    multiBuffer( 1, string.byte('S') )
    multiBuffer( 2, string.byte('M') ) 

    return 0
end

local function DSM_ReleaseConnection()
    if (DEBUG_ON) then Log.LOG_write("DSM_ReleaseConnection()\n") end
    multiBuffer(0, 0)
end

--------------------------------------------------------------------------------------------------------
-- REEQUEST Messages to RX

local function DSM_sendHeartbeat()
    -- keep connection open
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_sendHeartbeat()\n") end
    DSM_send(0x00, 0x04, 0x00, 0x00)
end

local function DSM_getRxVerson()
    local TX_CAP=0x14 -- Capabilities??
    local TX_MAX_CH = modelLib.TX_CHANNELS - 6   --number of channels after 6  (6Ch=0, 10ch=4, etc)
    
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getRxVersion(Ch:0x%X,Cap:0x%X)\n",TX_MAX_CH,TX_CAP) end
    DSM_send(0x11, 0x06, TX_MAX_CH, TX_CAP, 0x00, 0x00)
end

local function DSM_getMainMenu()
    local TX_CAP=0x14 -- Capabilities??
    local TX_MAX_CH = modelLib.TX_CHANNELS - 6   --number of channels after 6  (6Ch=0, 10ch=4, etc)
    
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getMainMenu(Ch:0x%X,Cap:0x%X) -- TX_Channels=%d\n",TX_MAX_CH,TX_CAP,TX_MAX_CH+6) end
    DSM_send(0x12, 0x06, TX_MAX_CH, TX_CAP, 0x00, 0x00) -- first menu only
end

local function DSM_getMenu(menuId, latSelLine)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getMenu(MenuId=0x%X LastSelectedLine=%s)\n", menuId, latSelLine) end
    DSM_send(0x16, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, latSelLine)
end

local function DSM_getFirstMenuLine(menuId)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getFirstMenuLine(MenuId=0x%X)\n", menuId) end
    DSM_send(0x13, 0x04, int16_MSB(menuId), int16_LSB(menuId)) -- line 0
end

local function DSM_getNextMenuLine(menuId, curLine)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getNextLine(MenuId=0x%X,LastLine=%s)\n", menuId, curLine) end
    DSM_send(0x14, 0x06, int16_MSB(menuId), int16_LSB(menuId), 0x00, curLine) -- line X
end

local function DSM_getNextMenuValue(menuId, valId, text)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_getNextMenuValue(MenuId=0x%X, LastValueId=0x%X) Extra: Text=\"%s\"\n", menuId, valId,
            text)
    end
    DSM_send(0x15, 0x06, int16_MSB(menuId), int16_LSB(menuId), int16_MSB(valId), int16_LSB(valId)) -- line X
end

local function DSM_updateMenuValue(valId, val, text, line)
    local value = sInt16ToDsm(val)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_updateValue(ValueId=0x%X,val=%d) Extra: Text=\"%s\" Value=%s\n", valId, val, text, menuLib.lineValue2String(line)) end
    DSM_send(0x18, 0x06, int16_MSB(valId), int16_LSB(valId), int16_MSB(value), int16_LSB(value)) -- send current value
end

local function DSM_validateMenuValue(valId, text, line)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_validateValue(ValueId=0x%X) Extra: Text=\"%s\" Value=%s\n", valId, text, menuLib.lineValue2String(line)) end
    DSM_send(0x19, 0x04, int16_MSB(valId), int16_LSB(valId)) 
end

local function DSM_editingValue(lineNum, text, line)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_editingValue(lineNo=0x%X) Extra: Text=\"%s\"  Val=%s\n", lineNum, text, menuLib.lineValue2String(line)) end
    DSM_send(0x1A, 0x04, int16_MSB(lineNum), int16_LSB(lineNum))
end

local function DSM_editingValueEnd(lineNum, text, line)
    if (DEBUG_ON) then Log.LOG_write("SEND DSM_editingValueEnd(lineNo=0x%X) Extra: Text=\"%s\"  Value=%s\n", lineNum, text, menuLib.lineValue2String(line)) end
    DSM_send(0x1B, 0x04, int16_MSB(lineNum), int16_LSB(lineNum))
end

-- Send the functionality of the RX channel Port (channel)
local function DSM_sendTxChInfo_20(portNo)
    local b1,b2 =  MODEL.DSM_ChannelInfo[portNo][0] or 0, MODEL.DSM_ChannelInfo[portNo][1] or 0

    if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxChInfo(#%d DATA= %02X %02X %02X %02X) %s\n", portNo,
        portNo, portNo, b1, b2, modelLib.channelType2String(b1,b2)) -- DATA part
    end
    DSM_send(0x20, 0x06, portNo, portNo, b1, b2) 
end

local function DSM_sendTxSubtrim_21(portNo)
    local usage = MODEL.DSM_ChannelInfo[portNo][1] or 0
    local leftTravel =   math.abs(math.floor(MODEL.modelOutputChannel[portNo].min/10))
    local rightTravel =  math.abs(math.floor(MODEL.modelOutputChannel[portNo].max/10))
    local subTrim     =  math.floor(MODEL.modelOutputChannel[portNo].offset/10)
    
    -- Center at 100%:  142-1906   (0 8E 07 72)
    local left  = 142
    local right = 1906

    if (bit32.band(usage,CH_TYPE.THR)>0) then 
       left = 0
       right = 2047
    end

    left =   math.floor (left  - (leftTravel  -100) *8.8  + subTrim*2)
    right =  math.floor (right + (rightTravel -100) *8.8  + subTrim*2)

    if (left<0) then left=0 end
    if (right>2027) then right=2047 end

    local b1,b2,b3,b4 = int16_MSB(left), int16_LSB(left), int16_MSB(right), int16_LSB(right) 

    if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxSubtrim(#%d DATA=%02X %02X %02X %02X) Range(%d - %d) ER L/R:(%d - %d)x8  ST:(%d)x2\n", portNo,
        b1,b2,b3,b4, left, right, leftTravel-100, rightTravel-100, subTrim) -- DATA part
   end
   DSM_send(0x21, 0x06, b1,b2,b3,b4) -- Port is not send anywhere, since the previous 0x20 type message have it.
end

local function DSM_sendTxServoTravel_23(portNo)
    local leftTravel =   math.abs(math.floor(MODEL.modelOutputChannel[portNo].min/10))
    local rightTravel =  math.abs(math.floor(MODEL.modelOutputChannel[portNo].max/10))
    local debugInfo   = string.format("Travel L/R (%d - %d)",leftTravel,rightTravel)

    if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxServoTravel(#%d DATA= %02X %02X %02X %02X) %s\n", portNo,
        0x00, leftTravel, 0x00, rightTravel, debugInfo) -- DATA part
    end
    DSM_send(0x23, 0x06, 0x00, leftTravel, 0x00, rightTravel)
end

local function DSM_sentTxInfo(menuId,portNo)
        -- TxInfo_Type=0    : AR636B Main Menu (Send port/Channel info + SubTrim + Travel)
        -- TxInfo_Type=1    : AR630-637 Famly Main Menu  (Only Send Port/Channel usage Msg 0x20)
        -- TxInfo_Type=1F   : AR630-637 Initial Setup/Relearn Servo Settings (Send port/Channel info + SubTrim + Travel +0x24/Unknown)

        if (TxInfo_Step == 0) then  
            -- AR630 family: Both TxInfo_Type (ManinMenu=0x1,   Other First Time Configuration = 0x1F)
            DSM_sendTxChInfo_20(portNo)

            if (TxInfo_Type == 0x1F) then
                TxInfo_Step = 1
            end 
            if (TxInfo_Type == 0x00) then
                TxInfo_Step = 2
            end 
        elseif (TxInfo_Step == 1) then
            DSM_sendTxServoTravel_23(portNo)
            TxInfo_Step = 2
        elseif (TxInfo_Step == 2) then
            DSM_sendTxSubtrim_21(portNo)

            if (TxInfo_Type == 0x00) then
                TxInfo_Step = 5 -- End Step 
            else 
                TxInfo_Step = 3
            end
        elseif (TxInfo_Step == 3) then
            -- 24,6: 0 83 5A B5 
            if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxInfo24(#%d DATA=0x24 0x06 %02X %02X %02X %02X)\n", portNo,
                0x00, 0x83, 0x5A, 0xB5) -- DATA part
            end
            DSM_send(0x24, 0x06, 0x00, 0x83, 0x5A, 0xB5) -- Still Uknown
            TxInfo_Step = 4
          
        elseif (TxInfo_Step == 4) then
            -- 24,6: 6 80 25 4B 
            if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxInfo24(#%d DATA=0x24 0x06 %02X %02X %02X %02X)\n", portNo,
                0x06, 0x80, 0x25, 0x4B) -- DATA part
            end
            DSM_send(0x24, 0x06, 0x06, 0x80, 0x25, 0x4B)  -- Still Uknown
            TxInfo_Step = 5
        elseif (TxInfo_Step == 5) then
            -- 22,4: 0 0 
            if (DEBUG_ON) then Log.LOG_write("CALL DSM_TxInfo_End(#%d)\n", portNo)
            end
            DSM_send(0x22, 0x04, 0x00, 0x00)
            TxInfo_Step = 0
        end

        if (TxInfo_Step > 0) then
            DSM_Context.SendDataToRX = 1 -- keep Transmitig
        end
end

-----------------------------------------------------------------------------------------------------------

local function DSM_sendRequest()  
    -- Send the proper Request message depending on the Phase 
    
    local ctx = DSM_Context
    if (DEBUG_ON) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end

    if ctx.Phase == PHASE.RX_VERSION then -- request RX version
        DSM_getRxVerson()

    elseif ctx.Phase == PHASE.WAIT_CMD then -- keep connection open
        DSM_sendHeartbeat()

    elseif ctx.Phase == PHASE.MENU_TITLE then -- request menu title
        if ctx.Menu.MenuId == 0 then  -- First time loading a menu ?
            DSM_getMainMenu()
        else
            DSM_getMenu(ctx.Menu.MenuId, ctx.SelLine) 

            if (ctx.Menu.MenuId == 0x0001) then  -- Executed the Reset Menu??
                if (DEBUG_ON) then Log.LOG_write("RX Reset!!!\n") end
                -- Start again retriving RX info 
                ctx.Menu.MenuId = 0
                ctx.isReset = true                
                ctx.Phase = PHASE.RX_VERSION
            end
        end

    elseif ctx.Phase == PHASE.MENU_REQ_TX_INFO then 
        DSM_sentTxInfo(ctx.Menu.MenuId, ctx.CurLine)

    elseif ctx.Phase == PHASE.MENU_LINES then -- request next menu lines
        if ctx.CurLine == -1 then -- No previous menu line loaded ?
            DSM_getFirstMenuLine(ctx.Menu.MenuId)
        else
            DSM_getNextMenuLine(ctx.Menu.MenuId, ctx.CurLine)
        end

    elseif ctx.Phase == PHASE.MENU_VALUES then -- request menu values
        local line = ctx.MenuLines[ctx.CurLine]
        DSM_getNextMenuValue(ctx.Menu.MenuId, line.ValId, line.Text)

    elseif ctx.Phase == PHASE.VALUE_CHANGING then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Updated Value of SELECTED line  

        if (Change_Step==0) then
            DSM_updateMenuValue(line.ValId, line.Val, line.Text, line)

            if line.Type == menuLib.LINE_TYPE.LIST_MENU then -- Validation on every Step??
                Change_Step=1; ctx.SendDataToRX=1  -- Send inmediatly after 
            else
                ctx.Phase = PHASE.VALUE_CHANGING_WAIT 
            end
        else
            DSM_validateMenuValue(line.ValId, line.Text, line)
            Change_Step=0
            ctx.Phase = PHASE.VALUE_CHANGING_WAIT 
        end

    elseif ctx.Phase == PHASE.VALUE_CHANGING_WAIT then
        local line = ctx.MenuLines[ctx.SelLine]
        DSM_editingValue(line.lineNum, line.Text, line)
    elseif ctx.Phase == PHASE.VALUE_CHANGE_END then -- send value
        local line = ctx.MenuLines[ctx.SelLine] -- Update Value of SELECTED line

        if (Change_Step==0) then
            DSM_updateMenuValue(line.ValId, line.Val, line.Text, line)
            Change_Step=1; ctx.SendDataToRX=1  -- Send inmediatly after 
        elseif (Change_Step==1) then
            DSM_validateMenuValue(line.ValId, line.Text, line)
            Change_Step=2; ctx.SendDataToRX=1  -- Send inmediatly after 
        else
            DSM_editingValueEnd(line.lineNum, line.Text, line)
            Change_Step=0
            ctx.Phase = PHASE.WAIT_CMD
        end
    elseif ctx.Phase == PHASE.EXIT then
        if (DEBUG_ON) then Log.LOG_write("CALL DSM_TX_Exit()\n") end
        DSM_send(0x1F, 0x02, 0xFF, 0xFF) -- 0xAA
    end
end

-----------------------------------------------------------------------------------------------------------
-- Parsing Responses

local function DSM_parseRxVersion()
    --ex: 0x09 0x01 0x00 0x15 0x02 0x22 0x01 0x00 0x14  v2.22.1   00 14??   8ch SAFE
    --    0x09 0x01 0x00 0x18 0x05 0x06 0x34 0x00 0x12  v5.6.52   00 12???  6ch FC6250
    local rxId = multiBuffer(13)
    DSM_Context.RX.Id = rxId
    DSM_Context.RX.Name = menuLib.Get_RxName(rxId)
    DSM_Context.RX.Version = multiBuffer(14) .. "." .. multiBuffer(15) .. "." .. multiBuffer(16)
    if (DEBUG_ON) then Log.LOG_write("RESPONSE Receiver=%s Version %s  Cap:0x%02X\n", 
                                     DSM_Context.RX.Name, DSM_Context.RX.Version, multiBuffer(18)) end
end

local function DSM_parseMenu()
    --ex: 0x09 0x02 0x4F 0x10  0xA5 0x00 0x00 0x00 0x50 0x10 0x10 0x10 0x00 0x00 0x00 0x00
    --              MenuID     TextID    PrevID    NextID    BackID
    local ctx = DSM_Context
    local menu  = ctx.Menu
    menu.MenuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    menu.TextId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    menu.Text   = menuLib.Get_Text(menu.TextId)
    menu.PrevId = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    menu.NextId = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))
    menu.BackId = Dsm_to_Int16(multiBuffer(20), multiBuffer(21))
    for i = 0, MAX_MENU_LINES do -- clear menu
        ctx.MenuLines[i] = { MenuId = 0, lineNum = 0, Type = 0, Text = "", TextId = 0, ValId = 0, Min=0, Max=0, Def=0, Val=nil }
    end
    ctx.CurLine = -1

    menuLib.MenuPostProcessing(menu)

    if (DEBUG_ON) then Log.LOG_write("RESPONSE Menu: %s\n", menuLib.menu2String(menu)) end
    return menu
end


local function DSM_parseMenuLine()
    --ex: 0x09 0x03 0x00    0x10      0x00  0x1C  0xF9 0x00   0x10    0x10      0x00 0x00 0x00 0x00 0x03 0x00
    --ex: 0x09 0x03 0x61    0x10      0x00  0x6C  0x50 0x00   0x00    0x10      0x36 0x00 0x49 0x00 0x36 0x00
    --ex: 0x09 0x03 0x65    0x10      0x00  0x0C  0x51 0x00   0x00    0x10      0x00 0x00 0xF4 0x00 0x2E 0x00
    --              MenuLSB MenuMSB   line  Type  TextID      NextLSB NextMSB   Val_Min   Val_Max   Val_Def

    local ctx = DSM_Context
    local i = multiBuffer(14)
    local type = multiBuffer(15)
    local line = ctx.MenuLines[i]

    -- are we trying to override existing line
    if (line.Type > 0 and type == 0) then
        if (DEBUG_ON) then Log.LOG_write("RESPONSE MenuLine: ERROR. Trying to ZERO Override: %s\n", menuLib.menuLine2String(line)) end
        return ctx.MenuLines[ctx.CurLine]
    end

    ctx.CurLine = i

    line.lineNum = i
    line.MenuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    line.Type   = type
    line.TextId = Dsm_to_Int16(multiBuffer(16), multiBuffer(17))
    line.Text   = nil -- Fill at Post processing
    line.ValId  = Dsm_to_Int16(multiBuffer(18), multiBuffer(19))

    -- Singed int values
    line.Min = Dsm_to_SInt16(multiBuffer(20), multiBuffer(21))  
    line.Max = Dsm_to_SInt16(multiBuffer(22), multiBuffer(23)) 
    line.Def = Dsm_to_SInt16(multiBuffer(24), multiBuffer(25))

    line.Val=nil

    menuLib.MenuLinePostProcessing(line)

    if (DEBUG_ON) then Log.LOG_write("RESPONSE MenuLine: %s\n", menuLib.menuLine2String(line))  end

    if (line.MenuId~=ctx.Menu.MenuId) then 
        -- Going Back too fast: Stil receiving lines from previous menu
        ctx.Menu.MenuId = line.MenuId 
        Log.LOG_write("WARNING: Overriding current Menu from Line\n")
    end

    return line
end

local function DSM_parseMenuValue()
    --ex: 0x09 0x04 0x53    0x10    0x00    0x10    0x00    0x00  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    --ex: 0x09 0x04 0x61    0x10    0x02    0x10    0x01    0x00  0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00
    --              MenuLSB MenuMSB ValLSB  ValMSB  V_LSB   V_MSB

    -- Identify the line and update the value
    local ctx = DSM_Context
    local menuId = Dsm_to_Int16(multiBuffer(12), multiBuffer(13))
    local valId = Dsm_to_Int16(multiBuffer(14), multiBuffer(15))
    local value = Dsm_to_SInt16(multiBuffer(16), multiBuffer(17)) --Signed int 

    local updatedLine = nil
    for i = 0, MAX_MENU_LINES do -- Find the menu line for this value
        local line = ctx.MenuLines[i]
        if line ~= nil and line.Type ~= 0 then
            if line.Type ~= LINE_TYPE.MENU and line.ValId == valId then -- identifier of ValueId stored in the line
                line.Val = value
                ctx.CurLine = i
                updatedLine = line
                break
            end
        end
    end

    if (updatedLine == nil) then
        if (DEBUG_ON) then Log.LOG_write("RESPONSE MenuValue: ERROR, Cant find Menu Line with MenuId=%X, ValID=%X to update\n", menuId, valId) end
    else
        if (DEBUG_ON) then Log.LOG_write("RESPONSE MenuValue: UPDATED: %s\n", menuLib.menuLine2String(updatedLine))
        end
    end
    return updatedLine ~= nil
end

local function DSM_parseReqTxInfo() 
    local portNo = multiBuffer(12) 
    TxInfo_Type   = multiBuffer(13)
    if (DEBUG_ON) then Log.LOG_write("RESPONSE ReqTXChannelInfo(#%d %s InfoType=0x%0X)\n", 
                                        portNo, MODEL.PORT_TEXT[portNo], TxInfo_Type) end

    TxInfo_Step = 0

    return portNo
end

------------------------------------------------------------------------------------------------------------
local function DSM_processResponse()
    local ctx = DSM_Context
    local cmd = multiBuffer(11) -- Response Command

    if (DEBUG_ON > 1) then Log.LOG_write("%s: RESPONSE %s \n", menuLib.phase2String(ctx.Phase), multiBuffer2String()) end
    if (DEBUG_ON and cmd > 0x00) then Log.LOG_write("%3.3f %s: ", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end

    if cmd == 0x01 then -- read version
        DSM_parseRxVersion()
        --Lib.Init_Text(DSM_Context.RX.Id)
        ctx.isReset = false  -- no longer resetting  
        ctx.Phase = PHASE.MENU_TITLE
        ctx.Menu.MenuId = 0

    elseif cmd == 0x02 then -- read menu title
        local menu = DSM_parseMenu()

        -- Update Selected Line navigation
        if menu.NextId ~= 0 then
            ctx.SelLine = NEXT_BUTTON -- highlight Next
        else
            ctx.SelLine = BACK_BUTTON -- highlight Back
        end

        if (ctx.Menu.MenuId == 0x0001) then  -- Still in RESETTING MENU???
            -- Star from Start
            if (DEBUG_ON) then Log.LOG_write("RX Reset:  Still not done, restart again!!!\n") end
            ctx.Menu.MenuId = 0
            ctx.Phase = PHASE.RX_VERSION
        else
            ctx.Phase = PHASE.MENU_LINES
        end
        

    elseif cmd == 0x03 then --  menu lines
        local line = DSM_parseMenuLine()

        -- Update Selected line navigation
        if (ctx.SelLine == BACK_BUTTON or ctx.SelLine == NEXT_BUTTON or ctx.SelLine == PREV_BUTTON)
            and menuLib.isSelectableLine(line) then -- Auto select the current line
            ctx.SelLine = line.lineNum
        end

        ctx.Phase = PHASE.MENU_LINES

    elseif cmd == 0x04 then -- read menu values
        if DSM_parseMenuValue() then 
            ctx.Phase = PHASE.MENU_VALUES
        else
            ctx.Phase = PHASE.WAIT_CMD
        end
        
    elseif cmd == 0x05 then -- Request TX Info
        local portNo = DSM_parseReqTxInfo()         
        ctx.CurLine = portNo
        ctx.Phase = PHASE.MENU_REQ_TX_INFO

    elseif cmd == 0xA7 then -- answer to EXIT command
        if (DEBUG_ON) then Log.LOG_write("RESPONSE RX Exit\n") end
        if (ctx.Phase==PHASE.EXIT) then -- Expected RX Exit
            ctx.Phase = PHASE.EXIT_DONE
        else -- UnExpected RX Exit
            DSM_ReleaseConnection()
            error("RX Connection Drop")
        end

    elseif cmd == 0x00 then -- NULL response (or RX heartbeat)
        -- 09 00 01 00 00 00 00 00 00 00 00 00 00 00 00
        -- 09 00 7E 00 20 9E 28 00 20 9E 28 00 20 8D 7E : After TX Heartbeat one of this (FC6250)
        -- 09 00 18 00 20 08 00 00 00 08 00 00 00 98 AE  AR8360T
        if (DEBUG_ON) then Log.LOG_write("%3.3f %s: RESPONSE RX Heartbeat  --Context: 0x%02X\n", 
                    menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase), multiBuffer(12)) end
    else
        if (DEBUG_ON) then Log.LOG_write("RESPONSE Unknown Command (0x%X)  DATA=%s\n", cmd, multiBuffer2String()) end
    end

    return cmd
end

------------------------------------------------------------------------------------------------------------
local function DSM_Send_Receive()
    local ctx = DSM_Context

    --  Receive part: Process incoming messages if there is nothing to send 
    if ctx.SendDataToRX == 0 and multiBuffer(10) == 0x09 then 
        local cmd = DSM_processResponse()

        multiBuffer(10, 0x00) -- Clear Response Buffer to know that we are done with the response
        RXInactivityTime = getTime() + SEND_TIMEOUT*4  -- Reset RX Inactivity timeout 

        if (cmd > 0x00) then -- RX HeartBeat ??
            -- Only change to SEND mode if we received a valid response  (Ignore heartbeat)
            ctx.SendDataToRX = 1
            ctx.Refresh_Display = true
        end
    else
        if  (getTime() > RXInactivityTime and ctx.Phase ~= PHASE.RX_VERSION and ctx.Phase ~= PHASE.EXIT_DONE) then
            if (ctx.isEditing()) then -- If Editing, Extend Timeout
                RXInactivityTime = getTime() + SEND_TIMEOUT*4 
            else
                if (DEBUG_ON) then Log.LOG_write("%3.3f %s: RX INACTIVITY TIMEOUT\n", menuLib.getElapsedTime(), menuLib.phase2String(ctx.Phase)) end
                DSM_ReleaseConnection()
                error("RX Disconnected")
            end
        end
    end

    -----TX Part --------------------------------------
    if ctx.SendDataToRX == 1 then   -- Need to send a request
        ctx.SendDataToRX = 0
        DSM_sendRequest()
        TXInactivityTime = getTime() + SEND_TIMEOUT  -- Reset Inactivity timeout 
    else
        -- Check if enouth time has passed from last transmit/receive activity
        if getTime() > TXInactivityTime then
            ctx.SendDataToRX = 1 -- Switch to Send mode to send heartbeat
            ctx.Refresh_Display = true

            -- Only change to WAIT_CMD if we are NOT wating for RX version
            if ctx.Phase ~= PHASE.RX_VERSION then
                -- Phase = If IsEditing then VALUE_CHANGING_WAIT else WAIT_CMD
                ctx.Phase = (ctx.isEditing() and PHASE.VALUE_CHANGING_WAIT) or PHASE.WAIT_CMD
            end
        end
    end
end

-- Init
local function DSM_Init(toolName)
    local dateTime = getDateTime()
    local dateStr = dateTime.year.."-"..dateTime.mon.."-"..dateTime.day.."   "..dateTime.hour..":"..dateTime.min

    local ver, radio, maj, minor, rev, osname = getVersion()

    if (osname==nil) then osname = "OpenTX" end -- OTX 2.3.14 and below returns nil

    IS_EDGETX = string.sub(osname,1,1) == 'E'

    if (DEBUG_ON) then 
        Log.LOG_write("---------------DSM New Session %s ----------------\n", toolName, dateStr)
        Log.LOG_write("Radio Info:    %s\n", radio .. " " .. (osname or "OpenTx") .. "  " .. ver) 
        Log.LOG_write("Date      :    %s\n", dateStr) 
        Log.LOG_write("DsmLib Version :    %s\n", LIB_VERSION) 
    end
end

local function DSM_Init_Text_Exceptions()
    -- Apply special restrictions to some Values 

    local function getTxChText(ch)
        return " ("..(MODEL.TX_CH_TEXT[ch] or "--")..")"
    end

    local List_Values = menuLib.List_Values
    local List_Text = menuLib.List_Text
    local Text = menuLib.Text

    Log.LOG_write("Initializing TEXT Exception\n")

    -- Channel selection for SAFE MODE and GAINS on  FC6250HX
    --List_Text[0x000C] = "Inhibit?" --?
    for i = 0, 7 do 
        List_Text[0x000D + i] = "Ch"..(i+5) ..getTxChText(i+4) 
    end -- Aux channels (Ch5 and Greater)

    -- Servo Output values.. 
    local servoOutputValues =  {0x0003,0x002D,0x002E,0x002F}  --Inh (GAP), 5.5ms, 11ms, 22ms. Fixing L_m1 with 0..244 range!
    --List_Text[0x002D] = "5.5ms"
    --[0x002E] = "11ms"
    --List_Text[0x002F] = "22ms"

    -- Gain Values
    local gainValues = {0x0032,0x0033,0x0034}  -- 1X, 2X, 4X   -- Fixing L_m1 with 0..244 range!
    --List_Text[0x0032] = "1 X"
    --[0x0033] = "2 X"
   -- List_Text[0x0034] = "4 X"

    -- List of Channels for Safe, Gains, Panic, except FC6250HX that uses other range (0x00C..0x015)
    -- the valid range Starts with GEAR if enabled  (Thr,Ail,Ele,Rud are not valid, the RX reject them ) 
    -- Valid Values: Inhibit? (GAP), Gear,Aux1..Aux7,X-Plus-1..XPlus-8
    local channelValues = {0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
                           0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049} 
    
    --List_Text[0x0035] = "Inhibit?" 
    for i = 0, 11 do 
        List_Text[0x0036 + i] = "Ch"..(i+1) .. getTxChText(i) 
    end -- Channels on  AR637T

    for i = 1, 8 do -- 41..49
        List_Text[0x0041 + i] = "Ch"..(i+12)
    end

    -- Flight mode channel selection
    --Text[0x0078] = "FM Channel"
    List_Values[0x0078]=channelValues  

    -- Gain  channel selection
    --Text[0x0089] = "Gain Channel"
    List_Values[0x0089]=channelValues

    -- Gain Sensitivity selection
    --Text[0x008A] = "Gain Sensitivity/r";  
    List_Values[0x008A]=gainValues  -- Right Alight, (L_M1 was wide open range 0->244)

    -- Safe mode options, Ihnibit + this values 
    local safeModeOptions = {0x0003,0x00B0,0x00B1}  -- inh (gap), "Self-Level/Angle Dem, Envelope
    --List_Text[0x00B0] = "Self-Level/Angle Dem"
    --List_Text[0x00B1] = "Envelope"

    --Text[0x00D2] = "Panic Channel" 
    List_Values[0x00D2]=channelValues
   
    --Inh, Self-Level/Angle Dem, Envelope -- (L_M was wide open range 0->244)
    --Text[0x01F8] = "Safe Mode";    
    List_Values[0x01F8]=safeModeOptions 
end

-- Initial Setup
local function FP_Init(toolname)
    DSM_Context.Phase = PHASE.INIT
    
    DSM_Init(toolname)
    menuLib.clearAllText()
end

local initStep=0
local FileState = {}

local function Message_Init()
    lcd.clear()
    if (initStep == 0) then
        if (IS_EDGETX) then
            -- Load all messages at once
            menuLib.LoadTextFromFile(MSG_FILE,13)
            initStep=1

        else
            -- load messages incrementally to avoid "CPU Limit"
            lcd.drawText(30, 50, "Loading Msg file: "..(FileState.lineNo or 0))
            if (menuLib.INC_LoadTextFromFile(MSG_FILE, FileState)==1) then
              initStep=1
            end
        end
    elseif (initStep == 1) then
      DSM_Init_Text_Exceptions()

      DSM_Context.Phase = PHASE.RX_VERSION
      DSM_StartConnection()
    end
end

local function FP_Run()
    if (DSM_Context.Phase==PHASE.INIT) then 
        Message_Init()
        return 0
    end
    
    return DSM_Send_Receive()
end

local function FP_Done()
    local ctx = menuLib.DSM_Context
    ctx.Phase = PHASE.EXIT_DONE
    DSM_ReleaseConnection()
end

return { init=FP_Init, run=FP_Run, done=FP_Done }
