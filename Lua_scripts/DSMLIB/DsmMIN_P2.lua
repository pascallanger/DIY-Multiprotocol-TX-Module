local MODEL, M_DATA, LOG_write = ...

--[[
local MODEL = {
    modelName = "",            -- The name of the model comming from OTX/ETX
    hashName = "",
    modelOutputChannel = {},   -- Output information from OTX/ETX
  
    TX_CH_TEXT= { [0]=""}, 
    PORT_TEXT = { [0]=""},
  
    DSM_ChannelInfo = {}       -- Data Created by DSM Configuration Script
  }

  -- MENU DATA Management
local M_DATA = {}            -- Store the variables used in the Menus.

--]]


local TX_CHANNELS         = 12

local AT_PLANE   = 0

local aircraft_type_text = {[0]="Plane","Heli","Glider","Drone"}

--[[
local WT_A1       = 0
local WT_A2       = 1
local WT_FLPR     = 2
local WT_A1_F1    = 3
local WT_A2_F1    = 4
local WT_A2_F2    = 5
--]]
local WT_ELEVON_A = 6
local WT_ELEVON_B = 7

local wing_type_text = {[0]="Normal","Dual Ail","Flapperon", "Ail + Flp","Dual Ail + Flp","Dual Ail/Flp","Elevon A","Elevon B"}

--[[
local TT_R1    = 0
local TT_R1_E1 = 1
local TT_R1_E2 = 2
local TT_R2_E1 = 3
local TT_R2_E2 = 4
--]]

local TT_VT_A  = 5
local TT_VT_B  = 6
local TT_TLRN_A = 7
local TT_TLRN_B = 8
local TT_TLRN_A_R2 = 9
local TT_TLRN_B_R2 = 10

local tail_type_text = {[0]="Rud Only","Normal","Rud + Dual Ele","Dual Rud + Elv","Dual Rud/Ele",
            "VTail A","VTail B","Taileron A","Taileron B","Taileron A + Dual Rud","Taileron B + Dual Rud"}


local MV_AIRCRAFT_TYPE = 1001
local MV_WING_TYPE     = 1002
local MV_TAIL_TYPE     = 1003
        
local MV_CH_BASE       = 1010
local MV_CH_THR        = 1010
local MV_CH_L_AIL      = 1011
local MV_CH_R_AIL      = 1012
local MV_CH_L_FLP      = 1013
local MV_CH_R_FLP      = 1014

local MV_CH_L_RUD      = 1015
local MV_CH_R_RUD      = 1016
local MV_CH_L_ELE      = 1017
local MV_CH_R_ELE      = 1018

local MV_PORT_BASE       = 1020

local MV_DATA_END        = 1040

--Channel Types --
local CT_NONE     = 0x00
local CT_AIL      = 0x01
local CT_ELE      = 0x02
local CT_RUD      = 0x04
local CT_REVERSE  = 0x20
local CT_THR      = 0x40
local CT_SLAVE    = 0x80

-- Seems like Reverse Mix is complement of the 3 bits
local CMT_NORM     = 0x00   -- 0000
local CMT_AIL      = 0x10   -- 0001 Taileron
local CMT_ELE      = 0x20   -- 0010 For VTIAL and Delta-ELEVON
local CMT_RUD      = 0x30   -- 0011 For VTIAL
local CMT_RUD_REV  = 0x40   -- 0100 For VTIAL
local CMT_ELE_REV  = 0x50   -- 0101 For VTIAL and Delta-ELEVON A
local CMT_AIL_REV  = 0x60   -- 0110 Taileron 
local CMT_NORM_REV = 0x70    -- 0111

local MT_NORMAL      = 0
local MT_REVERSE     = 1

local function channelType2String(byte1, byte2) 
    local s = ""
  
    if (byte2==0) then return s end;
    
    if (bit32.band(byte2,CT_AIL)>0) then s=s.."Ail" end
    if (bit32.band(byte2,CT_ELE)>0) then s=s.."Ele" end
    if (bit32.band(byte2,CT_RUD)>0) then s=s.."Rud" end
    if (bit32.band(byte2,CT_THR)>0) then s=s.."Thr" end
  
    if (bit32.band(byte2,CT_REVERSE)>0) then s=s.."-" end
  
    if (bit32.band(byte2,CT_SLAVE)>0) then s=s.." Slv" end
  
    if (byte1==CMT_NORM) then s=s.." " 
    elseif (byte1==CMT_AIL) then s=s.." M_Ail" 
    elseif (byte1==CMT_ELE) then s=s.." M_Ele" 
    elseif (byte1==CMT_RUD) then s=s.." M_Rud" 
    elseif (byte1==CMT_RUD_REV) then s=s.." M_Rud-" 
    elseif (byte1==CMT_ELE_REV) then s=s.." M_Ele-" 
    elseif (byte1==CMT_AIL_REV) then s=s.." M_Ail-" 
    elseif (byte1==CMT_NORM_REV) then s=s.." M-" 
    end
  
    return s;
  end
  
  -- This Creates the Servo Settings that will be used to pass to 
-- Forward programming
local function CreateDSMPortChannelInfo()
    local function ApplyWingMixA(b2)
        -- ELEVON
        if (b2==CT_AIL+CT_ELE) then return CMT_ELE end; -- 0x03
        if (b2==CT_AIL+CT_ELE+CT_SLAVE) then return CMT_NORM end; -- 0x83
    end

    local function ApplyWingMixB(b2)
        -- ELEVON 
        if (b2==CT_AIL+CT_ELE) then return CMT_NORM end; -- 0x03
        if (b2==CT_AIL+CT_ELE+CT_SLAVE) then return CMT_ELE end; -- 0x83
   end

    local function ApplyTailMixA(b2)
        -- VTAIL
        -- Default normal/reverse behaviour 
        if (b2==CT_RUD+CT_ELE) then return CMT_NORM end; -- 0x06
        if (b2==CT_RUD+CT_ELE+CT_SLAVE) then return CMT_ELE end; -- 0x86

        --TAILERON
        -- Default normal/reverse behaviour 
        if (b2==CT_AIL+CT_ELE) then return CMT_NORM end; -- 0x03
        if (b2==CT_AIL+CT_ELE+CT_SLAVE) then return CMT_AIL end; -- 0x83
    end

    local function ApplyTailMixB(b2)
        -- VTAIL 
        -- Default normal/reverse behaviour 
        if (b2==CT_RUD+CT_ELE) then return CMT_NORM end; -- 0x06
        if (b2==CT_RUD+CT_ELE+CT_SLAVE) then return CMT_RUD end; -- 0x86

        --TAILERON
        if (b2==CT_AIL+CT_ELE) then return CMT_AIL end; -- 0x03
        if (b2==CT_AIL+CT_ELE+CT_SLAVE) then return CMT_NORM end; -- 0x83
    end

    local function reverseMix(b)
        if (b==CMT_NORM) then return CMT_NORM_REV end;
        if (b==CMT_AIL) then return CMT_AIL_REV end;
        if (b==CMT_ELE) then return CMT_ELE_REV end;
        if (b==CMT_RUD) then return CMT_RUD_REV end;
        return b
    end

    local DSM_Ch = MODEL.DSM_ChannelInfo 

    for i=0, TX_CHANNELS-1 do
        DSM_Ch[i] = {[0]= CMT_NORM, CT_NONE, nil}  -- Initialize with no special function 
    end

    --local aircraftType = M_DATA[MV_AIRCRAFT_TYPE]
    local wingType = M_DATA[MV_WING_TYPE]
    local tailType = M_DATA[MV_TAIL_TYPE]

    local thrCh  =  M_DATA[MV_CH_THR]
    local lAilCh =  M_DATA[MV_CH_L_AIL]
    local rAilCh =  M_DATA[MV_CH_R_AIL]

    local lElevCh = M_DATA[MV_CH_L_ELE]
    local rElevCh = M_DATA[MV_CH_R_ELE]

    local lRudCh = M_DATA[MV_CH_L_RUD]
    local rRudCh = M_DATA[MV_CH_R_RUD]

    -- Channels in menu vars are Zero base, Channel info is 1 based 
    
    -- THR 
    if (thrCh~=nil and thrCh < 10) then DSM_Ch[thrCh][1]= CT_THR end

    -- AIL (Left and Right)
    if (lAilCh~=nil) then DSM_Ch[lAilCh][1] = CT_AIL  end
    if (rAilCh~=nil) then DSM_Ch[rAilCh][1] = CT_AIL+CT_SLAVE end
    -- ELE (Left and Right)
    if (lElevCh~=nil) then DSM_Ch[lElevCh][1] = CT_ELE end
    if (rElevCh~=nil) then DSM_Ch[rElevCh][1] = CT_ELE+CT_SLAVE end
    -- RUD (Left and Right)
    if (lRudCh~=nil) then DSM_Ch[lRudCh][1] = CT_RUD end
    if (rRudCh~=nil) then DSM_Ch[rRudCh][1] = CT_RUD+CT_SLAVE end

    -- VTAIL: RUD + ELE
    if (tailType==TT_VT_A) then 
        DSM_Ch[lElevCh][1] = CT_RUD+CT_ELE
        DSM_Ch[rElevCh][1] = CT_RUD+CT_ELE+CT_SLAVE
    elseif (tailType==TT_VT_B) then
        DSM_Ch[lElevCh][1] = CT_RUD+CT_ELE+CT_SLAVE
        DSM_Ch[rElevCh][1] = CT_RUD+CT_ELE
    end

    -- TAILERRON: 2-ELE + AIL
    if (tailType==TT_TLRN_A or tailType==TT_TLRN_A_R2) then 
        DSM_Ch[lElevCh][1] = CT_AIL+CT_ELE
        DSM_Ch[rElevCh][1] = CT_AIL+CT_ELE+CT_SLAVE
    elseif (tailType==TT_TLRN_B or tailType==TT_TLRN_B_R2) then
        DSM_Ch[lElevCh][1] = CT_AIL+CT_ELE+CT_SLAVE
        DSM_Ch[rElevCh][1] = CT_AIL+CT_ELE
    end

    ---- ELEVON :  AIL + ELE 
    if (wingType==WT_ELEVON_A) then 
        DSM_Ch[lAilCh][1] = CT_AIL+CT_ELE
        DSM_Ch[rAilCh][1] = CT_AIL+CT_ELE+CT_SLAVE
    elseif (wingType==WT_ELEVON_B) then
        DSM_Ch[lAilCh][1] = CT_AIL+CT_ELE+CT_SLAVE
        DSM_Ch[rAilCh][1] = CT_AIL+CT_ELE
    end

   ------MIXES ---------

    -- TAIL Mixes (Elevator and VTail)
    if (tailType==TT_VT_A or tailType==TT_TLRN_A or tailType==TT_TLRN_A_R2) then 
        DSM_Ch[lElevCh][0] = ApplyTailMixA(DSM_Ch[lElevCh][1])
        DSM_Ch[rElevCh][0] = ApplyTailMixA(DSM_Ch[rElevCh][1])
    elseif (tailType==TT_VT_B or tailType==TT_TLRN_B or tailType==TT_TLRN_B_R2) then
        DSM_Ch[lElevCh][0] = ApplyTailMixB(DSM_Ch[lElevCh][1])
        DSM_Ch[rElevCh][0] = ApplyTailMixB(DSM_Ch[rElevCh][1])
    end

     ---- ELEVON :  AIL + ELE 
     if (wingType==WT_ELEVON_A) then 
        DSM_Ch[lAilCh][0] = ApplyWingMixA(DSM_Ch[lAilCh][1])
        DSM_Ch[rAilCh][0] = ApplyWingMixA(DSM_Ch[rAilCh][1])
    elseif (wingType==WT_ELEVON_B) then
        DSM_Ch[lAilCh][0] = ApplyWingMixB(DSM_Ch[lAilCh][1])
        DSM_Ch[rAilCh][0] = ApplyWingMixB(DSM_Ch[rAilCh][1])
    end

    -- Apply Gyro Reverse as needed for each channel as long as it is used 
    for i=0, TX_CHANNELS-1 do
        if (M_DATA[MV_PORT_BASE+i]==MT_REVERSE and DSM_Ch[i][1]>0) then
            DSM_Ch[i][0]=reverseMix(DSM_Ch[i][0])
            DSM_Ch[i][1]=DSM_Ch[i][1]+CT_REVERSE
        end
    end

    -- Show how it looks
    for i=0, 9 do
        local b1,b2 =  DSM_Ch[i][0], DSM_Ch[i][1]
        local s1 =  channelType2String(b1,b2)
        local s = string.format("%s (%02X %02X)  %s\n", MODEL.PORT_TEXT[i],
                    b1, b2,s1)
        DSM_Ch[i][2]=s1
        LOG_write(s) 
    end

    --MODEL.AirWingTailDesc = string.format("Aircraft(%s) Wing(%s) Tail(%s)",aircraft_type_text[aircraftType],wing_type_text[wingType],tail_type_text[tailType])
end
  
  -- Main Program

  LOG_write("Creating DSMPort Info\n")
  CreateDSMPortChannelInfo()
  

