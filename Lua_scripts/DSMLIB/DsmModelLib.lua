
local Log, DEBUG_ON = ...

local DATA_PATH = "/MODELS/DSMDATA" -- Path to store model settings files
local TX_CHANNELS = 12

-- MODEL information from ETX/OTX
local ModelLib = {}

local MODEL = {
    modelName = "",            -- The name of the model comming from OTX/ETX
    modelOutputChannel = {},   -- Output information from OTX/ETX
    AirWingTailDesc = "",
    
    TX_CH_TEXT = {},
    PORT_TEXT = {},
    DSM_ChannelInfo = {}       -- Data Created by DSM Configuration Script
}

--Channel Types --
local CH_TYPE = {
    NONE     = 0x00,
    AIL      = 0x01,
    ELE      = 0x02,
    RUD      = 0x04,

    REVERSE  = 0x20,
    THR      = 0x40,
    SLAVE    = 0x80,
}

-- Seems like Reverse Mix is complement of the 3 bits
local CH_MIX_TYPE = {
    MIX_NORM     = 0x00,   -- 0000
    MIX_AIL      = 0x10,   -- 0001 Taileron
    MIX_ELE      = 0x20,   -- 0010 For VTIAL and Delta-ELEVON
    MIX_RUD      = 0x30,   -- 0011 For VTIAL

    MIX_RUD_REV  = 0x40,   -- 0100 For VTIAL
    MIX_ELE_REV  = 0x50,   -- 0101 For VTIAL and Delta-ELEVON A
    MIX_AIL_REV  = 0x60,   -- 0110 Taileron 
    MIX_NORM_REV = 0x70    -- 0111
}

local AIRCRAFT_TYPE = {
    PLANE   = 0,
    HELI    = 1,
    GLIDER  = 2,
    DRONE   = 3
}
local aircraft_type_text = {[0]="Plane","Heli","Glider","Drone"}

local WING_TYPE = {
    AIL_1       = 0, --1
    AIL_2       = 1, --2
    FLAPERON    = 2, --2
    AIL_1_FLP_1 = 3, --2
    AIL_2_FLP_1 = 4, --3
    AIL_2_FLP_2 = 5, --4
    ELEVON_A    = 6, --2
    ELEVON_B    = 7  --2
}

local wing_type_text = {[0]="Normal","Dual Ail","Flapperon", "Ail + Flp","Dual Ail + Flp","Dual Ail/Flp","Elevon A","Elevon B"}

local TAIL_TYPE = {
    RUD_1        = 0,  -- 1
    RUD_1_ELEV_1 = 1,  -- 2
    RUD_1_ELEV_2 = 2,  -- 3
    RUD_2_ELEV_1 = 3,  -- 3
    RUD_2_ELEV_2 = 4,  -- 4
    VTAIL_A      = 5,  -- 2
    VTAIL_B      = 6,  -- 2
    TRAILERON_A  = 7,  -- 3
    TRAILERON_B  = 8,  -- 3
    TRAILERON_A_R2  = 9,  -- 3
    TRAILERON_B_R2  = 10  -- 3
}
local tail_type_text = {[0]="Rud Only","Normal","Rud + Dual Ele","Dual Rud + Elv","Dual Rud/Ele",
                        "VTail A","VTail B","Taileron A","Taileron B","Taileron A + 2x Rud","Taileron B + 2x Rud"}

local CH_MODE_TYPE = {
    NORMAL      = 0,
    REVERSE     = 1,
    USE_TX      = 3
}

local PORT = {
    PORT1 = 0,
    PORT2 = 1,
    PORT3 = 2,
    PORT4 = 3,
    PORT5 = 4,
    PORT6 = 5,
    PORT7 = 6,
    PORT8 = 7,
    PORT9 = 8,
    PORT10 = 9,
    PORT11 = 10,
    PORT12 = 11
}

local MEMU_VAR = {
        AIRCRAFT_TYPE = 1001,
        WING_TYPE     = 1002,
        TAIL_TYPE     = 1003,
        
        CH_BASE       = 1010,
        CH_THR        = 1010,

        CH_L_AIL      = 1011,
        CH_R_AIL      = 1012,
        CH_L_FLP      = 1013,
        CH_R_FLP      = 1014,

        CH_L_RUD      = 1015,
        CH_R_RUD      = 1016,
        CH_L_ELE      = 1017,
        CH_R_ELE      = 1018,

        PORT_BASE       = 1020,
        PORT1_MODE      = 1020,
        PORT2_MODE      = 1021,
        PORT3_MODE      = 1022,
        PORT4_MODE      = 1023,
        PORT5_MODE      = 1024,
        PORT6_MODE      = 1025,
        PORT7_MODE      = 1026,
        PORT8_MODE      = 1027,
        PORT9_MODE      = 1028,
        PORT10_MODE     = 1029,
        PORT11_MODE     = 1030,
        PORT12_MODE     = 1031,

        DATA_END        = 1040
}


-- MENU DATA Management
local MENU_DATA = {}            -- Store the variables used in the Menus.


---- DSM_ChannelInfo ---------------------------------
-- First byte describe Special  Mixing (Vtail/Elevon = 0x20)
--VTAIL
--(0x00 0x06) CH_TYPE.ELE+CH_TYPE.RUD  (0x02+0x04 = 0x06)
--(0x20 0x86) CH_TYPE.ELE+CH_TYPE.RUD+CH_TYPE.SLAVE  (0x02+0x04+0x80 = 0x86)

-- The 2nd byte describes the functionality of the port 
-- 
-- Single   Example: CH_TYPE.AIL (0x01) Aileron
-- Reverse  Example: CH_TYPE.AIL+CH_TYPE.REVERSE (0x01+0x20=0x21) Reverse Aileron
-- Slave    Example: CH_TYPE.AIL+CH_TYPE.SLAVE (0x01+0x80) -- 2nd servo Aileron

-- Elevon   Example: CH_TYPE.AIL+CH_TYPE.ELE  (0x01+0x02 = 0x03) -- Elevon
-- Elevon   Example: CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE  (0x01+0x02+0x80 = 0x83) -- Slave Elevon

-- RudElv (VTail) Example: CH_TYPE.ELE+CH_TYPE.RUD  (0x02+0x04 = 0x06) -- Rudevator
-- RudElv (VTail) Example: CH_TYPE.ELE+CH_TYPE.RUD+CH_TYPE.SLAVE  (0x02+0x04+0x80 = 0x86) -- Rudevator Slave

-- DEFAULT Simple Plane Port configuration (The Configuration tool will overrride this)
MODEL.DSM_ChannelInfo= {[0]= -- Start array at position 0
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.THR},    -- Ch1 Thr  (0x40)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.AIL},    -- Ch2 Ail  (0x01)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.ELE},    -- Ch2 ElE  (0x02)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.RUD},    -- Ch4 Rud  (0x04)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch5 Gear (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch6 Aux1 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch7 Aux2 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch8 Aux3 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch9 Aux4 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch10 Aux5 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE},   -- Ch11 Aux6 (0x00)
                        {[0]=  CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE}    -- Ch12 Aux7 (0x00)
                    }   

function ModelLib.printChannelSummary(a,w,t)
    -- Summary
    print("CHANNEL INFORMATION")
    print("Aircraft:".. (aircraft_type_text[MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]] or "--"))
    print("Wing Type:".. (wing_type_text[MENU_DATA[MEMU_VAR.WING_TYPE]] or "--"))
    print("Tail Type:".. (tail_type_text[MENU_DATA[MEMU_VAR.TAIL_TYPE]] or "--"))
    print("Thr:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_THR]  or 30)] or "--"))   -- use fake ch30 for non existing channels 
    print("LAil:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_AIL] or 30)] or "--"))
    print("RAil:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_AIL] or 30)] or "--"))
    print("LFlp:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_FLP] or 30)] or "--"))
    print("RFlp:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_FLP] or 30)] or "--"))
    print("LEle:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_ELE] or 30)] or "--"))
    print("REle:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_ELE] or 30)] or "--"))
    print("LRud:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_L_RUD] or 30)] or "--"))
    print("RRud:".. (MODEL.PORT_TEXT[(MENU_DATA[MEMU_VAR.CH_R_RUD] or 30)] or "--"))   
end

function ModelLib.printServoReverseInfo()
    print("SERVO Normal/Reverse INFORMATION")
    for i=0, TX_CHANNELS-1 do
        local s="--"
        if (MENU_DATA[MEMU_VAR.PORT1_MODE+i] or 0) == 0 then s="NORMAL" else s="REVERSE" end
        print(string.format("Port%d:  %s", i+1, s))
    end
end

function ModelLib.channelType2String(byte1, byte2) 
    local s = ""

    if (byte2==0) then return s end;
    
    if (bit32.band(byte2,CH_TYPE.AIL)>0) then s=s.."Ail" end
    if (bit32.band(byte2,CH_TYPE.ELE)>0) then s=s.."Ele" end
    if (bit32.band(byte2,CH_TYPE.RUD)>0) then s=s.."Rud" end
    if (bit32.band(byte2,CH_TYPE.THR)>0) then s=s.."Thr" end

    if (bit32.band(byte2,CH_TYPE.REVERSE)>0) then s=s.."-" end

    if (bit32.band(byte2,CH_TYPE.SLAVE)>0) then s=s.." Slv" end

    if (byte1==CH_MIX_TYPE.MIX_NORM) then s=s.." " 
    elseif (byte1==CH_MIX_TYPE.MIX_AIL) then s=s.." M_Ail" 
    elseif (byte1==CH_MIX_TYPE.MIX_ELE) then s=s.." M_Ele" 
    elseif (byte1==CH_MIX_TYPE.MIX_RUD) then s=s.." M_Rud" 
    elseif (byte1==CH_MIX_TYPE.MIX_RUD_REV) then s=s.." M_Rud-" 
    elseif (byte1==CH_MIX_TYPE.MIX_ELE_REV) then s=s.." M_Ele-" 
    elseif (byte1==CH_MIX_TYPE.MIX_AIL_REV) then s=s.." M_Ail-" 
    elseif (byte1==CH_MIX_TYPE.MIX_NORM_REV) then s=s.." M-" 
    end

    return s;
end


-------------------------------------------------------------------------------------------------
-- Read the model information from OTX/ETX

local function getModuleChannelOrder(num) 
        --Determine fist 4 channels order
    local channel_names={}
    local stick_names = {[0]= "R", "E", "T", "A" }
    local ch_order=num
    if (ch_order == -1) then
        channel_names[0] = stick_names[3]
        channel_names[1] = stick_names[1]
        channel_names[2] = stick_names[2]
        channel_names[3] = stick_names[0]
    else
        channel_names[bit32.band(ch_order,3)] = stick_names[3]
        ch_order = math.floor(ch_order/4)
        channel_names[bit32.band(ch_order,3)] = stick_names[1]
        ch_order = math.floor(ch_order/4)
        channel_names[bit32.band(ch_order,3)] = stick_names[2]
        ch_order = math.floor(ch_order/4)
        channel_names[bit32.band(ch_order,3)] = stick_names[0]
    end

    local s = ""
    for i=0,3 do
        s=s..channel_names[i]
    end
    return s
end

function ModelLib.ReadTxModelData()
  local TRANSLATE_AETR_TO_TAER=false
  local table = model.getInfo()   -- Get the model name 
  MODEL.modelName = table.name

  local module = model.getModule(0) -- Internal
  if (module==nil or module.Type~=6) then module = model.getModule(1) end -- External
  if (module~=nil) then
      if (module.Type==6 ) then -- MULTI-MODULE
          local chOrder = module.channelsOrder
          local s = getModuleChannelOrder(chOrder)
          Log.LOG_write("MultiChannel Ch Order: [%s]  %s\n",chOrder,s) 

          if (s=="AETR") then TRANSLATE_AETR_TO_TAER=true 
          else TRANSLATE_AETR_TO_TAER=false 
          end
      end
  end

  Log.LOG_write("MODEL NAME = %s\n",MODEL.modelName) 

  -- Read Ch1 to Ch10
  local i= 0
  for i = 0, TX_CHANNELS-1 do 
      local ch = model.getOutput(i) -- Zero base 
      if (ch~=nil) then
          MODEL.modelOutputChannel[i] = ch
          if (string.len(ch.name)==0) then 
              ch.formatCh = string.format("TX:Ch%i",i+1)
          else
              ch.formatCh = string.format("TX:Ch%i/%s",i+1,ch.name or "--")
          end
      end
  end

  -- Translate AETR to TAER
  -- TODO: Check if there is a way to know how to the TX is configured, since if it is 
  -- already TAER, is not needed 

  if (TRANSLATE_AETR_TO_TAER) then 
      Log.LOG_write("Applying  AETR -> TAER translation\n") 
      local ail = MODEL.modelOutputChannel[0]
      local elv = MODEL.modelOutputChannel[1]
      local thr = MODEL.modelOutputChannel[2]

      MODEL.modelOutputChannel[0] = thr
      MODEL.modelOutputChannel[1] = ail
      MODEL.modelOutputChannel[2] = elv
  end

  -- Create the Port Text to be used 
  Log.LOG_write("Ports/Channels:\n") 
  for i = 0, TX_CHANNELS-1 do 
      local ch =  MODEL.modelOutputChannel[i]
      if (ch~=nil) then
          MODEL.TX_CH_TEXT[i] = ch.formatCh
          MODEL.PORT_TEXT[i] = string.format("P%i (%s) ",i+1,MODEL.TX_CH_TEXT[i])
          
          Log.LOG_write("Port%d %s [%d,%d] Rev=%d, Off=%d, ppmC=%d, syn=%d\n",i+1,MODEL.TX_CH_TEXT[i],math.floor(ch.min/10),math.floor(ch.max/10), ch.revert, ch.offset, ch.ppmCenter, ch.symetrical)
      end
  end
end

-----------------------  FILE MANAGEMENT ---------------------------------------------
-- Create a fairly unique name for a model..combination of name and a hash 
-- TODO: Check with ETX why we can't get the filename used to store the model info
-- Improvement request??

function ModelLib.hashName(mName)
    local c=10000;

    local prefix = string.gsub(mName,"%.","_") -- Change any "." to "_"
    prefix = string.gsub(prefix,"% ","_") -- Change any space to "_"
    prefix = string.sub(prefix,1,5) -- Take the first 5 characters

    -- Simple Hash of the Model Name adding each character 
    for i = 1, #mName do
        local ch = string.byte(mName,i,i)
        c=c+ch
    end

    return (prefix .. c) -- Return Prefix + Hash
end

-- Load Menu Data from a file 
function ModelLib.ST_LoadFileData() 
    local fname = ModelLib.hashName(MODEL.modelName)..".txt"

    -- Clear Menu Data
    for i = 0, MEMU_VAR.DATA_END do
        MENU_DATA[i]=nil
    end

    print("Loading File:"..fname)

    local dataFile = io.open(DATA_PATH .. "/".. fname, "r")  -- read File 
    -- cannot read file???
    if (dataFile==nil) then return 0 end

    local line = io.read(dataFile, 5000)
    io.close(dataFile)

    if #line == 0 then return 0 end -- No data??

    -- Process the input, each line is "Var_Id : Value" format 
    -- Store it into MANU_DATA
    local i=0
    for k, v in string.gmatch(line, "(%d+):(%d+)") do
        --print(string.format("Read  MENU_DATA[%d]:[%d]",k, v))
        MENU_DATA[k+0]=v+0 -- do aritmentic to convert string to number
        i=i+1
    end
    
    local currAircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
    local currWingType = MENU_DATA[MEMU_VAR.WING_TYPE]
    local currTailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

    print("Validation")
    print(string.format("AIRCRAFT_TYPE(%d)=%s", MEMU_VAR.AIRCRAFT_TYPE,aircraft_type_text[currAircraftType]))
    print(string.format("WING_TYPE(%d)=%s", MEMU_VAR.WING_TYPE, wing_type_text[currWingType]))
    print(string.format("TAIL_TYPE(%d)=%s", MEMU_VAR.TAIL_TYPE, tail_type_text[currTailType]))
   
    ModelLib.printChannelSummary()
    ModelLib.printServoReverseInfo()

    -- Return 0 if no lines processed, 1 otherwise
    if (i > 0) then return 1 else return 0 end
end

-- Saves MENU_DATA to a file
function ModelLib.ST_SaveFileData() 
    local fname = ModelLib.hashName(MODEL.modelName)..".txt"

    print("Saving File:"..fname)
    local dataFile = assert(io.open(DATA_PATH .. "/" .. fname, "w"),"Please create "..DATA_PATH.." folder")  -- write File 
    
    -- Foreach MENU_DATA with a value write Var_Id:Value into file
    for i = 0, MEMU_VAR.DATA_END do
        if (MENU_DATA[i]~=nil) then
            --print(string.format("Write MENU_DATA[%s] : %s",i,MENU_DATA[i]))
            io.write(dataFile,string.format("%s:%s\n",i,MENU_DATA[i]))
        end
    end
    io.close(dataFile)
end

-- This Creates the Servo Settings that will be used to pass to 
-- Forward programming
function ModelLib.CreateDSMPortChannelInfo()
    local function ApplyWingMixA(b2)
        -- ELEVON
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_ELE end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x83
    end

    local function ApplyWingMixB(b2)
        -- ELEVON 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_ELE end; -- 0x83
   end

    local function ApplyTailMixA(b2)
        -- VTAIL
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x06
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_ELE end; -- 0x86

        --TAILERON
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_AIL end; -- 0x83
    end

    local function ApplyTailMixB(b2)
        -- VTAIL 
        -- Default normal/reverse behaviour 
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x06
        if (b2==CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_RUD end; -- 0x86

        --TAILERON
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE) then return CH_MIX_TYPE.MIX_AIL end; -- 0x03
        if (b2==CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE) then return CH_MIX_TYPE.MIX_NORM end; -- 0x83
    end

    local function reverseMix(b)
        if (b==CH_MIX_TYPE.MIX_NORM) then return CH_MIX_TYPE.MIX_NORM_REV end;
        if (b==CH_MIX_TYPE.MIX_AIL) then return CH_MIX_TYPE.MIX_AIL_REV end;
        if (b==CH_MIX_TYPE.MIX_ELE) then return CH_MIX_TYPE.MIX_ELE_REV end;
        if (b==CH_MIX_TYPE.MIX_RUD) then return CH_MIX_TYPE.MIX_RUD_REV end;
        return b
    end



    local DSM_ChannelInfo = MODEL.DSM_ChannelInfo 

    for i=0, TX_CHANNELS-1 do
        DSM_ChannelInfo[i] = {[0]= CH_MIX_TYPE.MIX_NORM, CH_TYPE.NONE} -- Initialize with no special function 
    end

    local aircraftType = MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE]
    local wingType = MENU_DATA[MEMU_VAR.WING_TYPE]
    local tailType = MENU_DATA[MEMU_VAR.TAIL_TYPE]

    local thrCh  =  MENU_DATA[MEMU_VAR.CH_THR]
    local lAilCh =  MENU_DATA[MEMU_VAR.CH_L_AIL]
    local rAilCh =  MENU_DATA[MEMU_VAR.CH_R_AIL]
    local lflapCh = MENU_DATA[MEMU_VAR.CH_L_FLP]
    local rflapCh = MENU_DATA[MEMU_VAR.CH_R_FLP]

    local lElevCh = MENU_DATA[MEMU_VAR.CH_L_ELE]
    local rElevCh = MENU_DATA[MEMU_VAR.CH_R_ELE]

    local lRudCh = MENU_DATA[MEMU_VAR.CH_L_RUD]
    local rRudCh = MENU_DATA[MEMU_VAR.CH_R_RUD]

    -- Channels in menu vars are Zero base, Channel info is 1 based 
    
    -- THR 
    if (thrCh~=nil and thrCh < 10) then DSM_ChannelInfo[thrCh][1]= CH_TYPE.THR end

    -- AIL (Left and Right)
    if (lAilCh~=nil) then DSM_ChannelInfo[lAilCh][1] = CH_TYPE.AIL  end
    if (rAilCh~=nil) then DSM_ChannelInfo[rAilCh][1] = CH_TYPE.AIL+CH_TYPE.SLAVE end
    -- ELE (Left and Right)
    if (lElevCh~=nil) then DSM_ChannelInfo[lElevCh][1] = CH_TYPE.ELE end
    if (rElevCh~=nil) then DSM_ChannelInfo[rElevCh][1] = CH_TYPE.ELE+CH_TYPE.SLAVE end
    -- RUD (Left and Right)
    if (lRudCh~=nil) then DSM_ChannelInfo[lRudCh][1] = CH_TYPE.RUD end
    if (rRudCh~=nil) then DSM_ChannelInfo[rRudCh][1] = CH_TYPE.RUD+CH_TYPE.SLAVE end

    -- VTAIL: RUD + ELE
    if (tailType==TAIL_TYPE.VTAIL_A or tailType==TAIL_TYPE.VTAIL_B) then 
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.RUD+CH_TYPE.ELE+CH_TYPE.SLAVE
    end

    -- TAILERRON: 2-ELE + AIL
    if (tailType==TAIL_TYPE.TRAILERON_A or tailType==TAIL_TYPE.TRAILERON_A_R2 or
        tailType==TAIL_TYPE.TRAILERON_B or tailType==TAIL_TYPE.TRAILERON_B_R2) then 
        DSM_ChannelInfo[lElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
        DSM_ChannelInfo[rElevCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
    end

    ---- ELEVON :  AIL + ELE 
    if (wingType==WING_TYPE.ELEVON_A or wingType==WING_TYPE.ELEVON_B) then 
        DSM_ChannelInfo[lAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE
        DSM_ChannelInfo[rAilCh][1] = CH_TYPE.AIL+CH_TYPE.ELE+CH_TYPE.SLAVE
    end

    ------MIXES ---------

    -- TAIL Mixes (Elevator and VTail)
    if (tailType==TAIL_TYPE.VTAIL_A or tailType==TAIL_TYPE.TRAILERON_A or tailType==TAIL_TYPE.TRAILERON_A_R2) then 
        DSM_ChannelInfo[lElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[rElevCh][1])
    elseif (tailType==TAIL_TYPE.VTAIL_B or tailType==TAIL_TYPE.TRAILERON_B or tailType==TAIL_TYPE.TRAILERON_B_R2) then 
        DSM_ChannelInfo[lElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[lElevCh][1])
        DSM_ChannelInfo[rElevCh][0] = ApplyTailMixA(DSM_ChannelInfo[rElevCh][1])
    end

     ---- Wing Mixes 
     if (wingType==WING_TYPE.ELEVON_A) then 
        DSM_ChannelInfo[lAilCh][0] = ApplyWingMixA(DSM_ChannelInfo[lAilCh][1])
        DSM_ChannelInfo[rAilCh][0] = ApplyWingMixA(DSM_ChannelInfo[rAilCh][1])
    elseif (wingType==WING_TYPE.ELEVON_B) then
        DSM_ChannelInfo[lAilCh][0] = ApplyWingMixB(DSM_ChannelInfo[lAilCh][1])
        DSM_ChannelInfo[rAilCh][0] = ApplyWingMixB(DSM_ChannelInfo[rAilCh][1])
    end

    -- Apply Gyro Reverse as needed for each channel as long as it is used 
    for i=0, TX_CHANNELS-1 do
        if (MENU_DATA[MEMU_VAR.PORT_BASE+i]==CH_MODE_TYPE.REVERSE and DSM_ChannelInfo[i][1]>0) then
            DSM_ChannelInfo[i][0]=reverseMix(DSM_ChannelInfo[i][0])
            DSM_ChannelInfo[i][1]=DSM_ChannelInfo[i][1]+CH_TYPE.REVERSE
        end
    end

    -- Show how it looks
    for i=0, 9 do
        local b1,b2 =  DSM_ChannelInfo[i][0], DSM_ChannelInfo[i][1]
        print(string.format("%s (%02X %02X)  %s", MODEL.PORT_TEXT[i],
             b1, b2, ModelLib.channelType2String(b1,b2)))
    end

    MODEL.AirWingTailDesc = string.format("Aircraft(%s) Wing(%s) Tail(%s)",aircraft_type_text[aircraftType],wing_type_text[wingType],tail_type_text[tailType])
end

function ModelLib.ST_PlaneWingInit(wingType) 
    print("Change Plane WingType:"..wing_type_text[wingType])

    MENU_DATA[MEMU_VAR.WING_TYPE] = wingType

    -- Clear all Wing Data 
    MENU_DATA[MEMU_VAR.CH_L_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_R_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_L_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_R_FLP] = nil

    MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT1
    
    -- Default Channel Assisgments for each Wing type

    if (wingType==WING_TYPE.AIL_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.FLAPERON) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_1_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT6
    elseif (wingType==WING_TYPE.AIL_2_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
    elseif (wingType==WING_TYPE.AIL_2_FLP_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_R_FLP] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT7
    elseif (wingType==WING_TYPE.ELEVON_A) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT3
    elseif (wingType==WING_TYPE.ELEVON_B) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    else -- Assume normal 
       print("ERROR: Invalid Wing Type")
    end 


    ModelLib.printChannelSummary()
end

function ModelLib.ST_PlaneTailInit(tailType) 
    if (MENU_DATA[MEMU_VAR.WING_TYPE]==WING_TYPE.ELEVON_A or
        MENU_DATA[MEMU_VAR.WING_TYPE]==WING_TYPE.ELEVON_B) then
        tailType = TAIL_TYPE.RUD_1 -- Delta only have ruder  
    end

    print("Change Plane Tail Type:"..tail_type_text[tailType])

    -- Clear all data for Tail 
    MENU_DATA[MEMU_VAR.TAIL_TYPE] = tailType
    MENU_DATA[MEMU_VAR.CH_L_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_R_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_L_RUD]  = nil
    MENU_DATA[MEMU_VAR.CH_R_RUD] = nil

    -- Setup Channels for different Tail types 
    if (tailType == TAIL_TYPE.RUD_1) then
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_2) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_2_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_RUD] = PORT.PORT5
    elseif (tailType == TAIL_TYPE.RUD_2_ELEV_2) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_RUD] = PORT.PORT6
    elseif (tailType == TAIL_TYPE.VTAIL_A or tailType == TAIL_TYPE.VTAIL_B) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    elseif (tailType == TAIL_TYPE.TRAILERON_A or tailType==TAIL_TYPE.TRAILERON_A_R2 or
            tailType == TAIL_TYPE.TRAILERON_B or tailType==TAIL_TYPE.TRAILERON_B_R2) then
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    else -- Assume Normal 
        print("ERROR:invalid Tail Type")
    end

    if (tailType == TAIL_TYPE.TRAILERON_A_R2 or tailType==TAIL_TYPE.TRAILERON_B_R2) then
        MENU_DATA[MEMU_VAR.CH_R_RUD] = PORT.PORT7
    end

    ModelLib.printChannelSummary()
end

function ModelLib.ST_GliderWingInit(wingType) 
    print("Change Glider WingType:"..wing_type_text[wingType])

    MENU_DATA[MEMU_VAR.WING_TYPE] = wingType

    -- Clear all Wing Data 
    MENU_DATA[MEMU_VAR.CH_L_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_R_AIL] = nil
    MENU_DATA[MEMU_VAR.CH_L_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_R_FLP] = nil
    MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT6
    
    -- Default Channel Assisgments for each Wing type

    if (wingType==WING_TYPE.AIL_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
    elseif (wingType==WING_TYPE.AIL_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.AIL_2_FLP_1) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
    elseif (wingType==WING_TYPE.AIL_2_FLP_2) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_L_FLP] = PORT.PORT5
        MENU_DATA[MEMU_VAR.CH_R_FLP] = PORT.PORT6
        MENU_DATA[MEMU_VAR.CH_THR] = PORT.PORT7
    elseif (wingType==WING_TYPE.ELEVON_A) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT1
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT2
    elseif (wingType==WING_TYPE.ELEVON_B) then
        MENU_DATA[MEMU_VAR.CH_L_AIL] = PORT.PORT2
        MENU_DATA[MEMU_VAR.CH_R_AIL] = PORT.PORT1
    else -- Assume normal 
        print("ERROR: Invalid Wing Type")
    end 

    ModelLib.printChannelSummary()
end

function ModelLib.ST_GliderTailInit(tailType) 
    if (MENU_DATA[MEMU_VAR.WING_TYPE]==WING_TYPE.ELEVON_A) then
        tailType = TAIL_TYPE.RUD_1 -- Delta only have ruder  
    end

    print("Change Glider Tail Type:"..tail_type_text[tailType])

    -- Clear all data for Tail 
    MENU_DATA[MEMU_VAR.TAIL_TYPE] = tailType
    MENU_DATA[MEMU_VAR.CH_L_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_R_ELE] = nil
    MENU_DATA[MEMU_VAR.CH_L_RUD]  = nil
    MENU_DATA[MEMU_VAR.CH_R_RUD] = nil

    -- Setup Channels for different Tail types 
    if (tailType == TAIL_TYPE.RUD_1) then
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.RUD_1_ELEV_1) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_L_RUD] = PORT.PORT4
    elseif (tailType == TAIL_TYPE.VTAIL_A) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT4
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT3
    elseif (tailType == TAIL_TYPE.VTAIL_B) then
        MENU_DATA[MEMU_VAR.CH_L_ELE] = PORT.PORT3
        MENU_DATA[MEMU_VAR.CH_R_ELE] = PORT.PORT4
    else -- Assume Normal 
        print("ERROR: Invalid Tail Type")
    end

    ModelLib.printChannelSummary()
end


function ModelLib.ST_AircraftInit(aircraftType)
    MENU_DATA[MEMU_VAR.AIRCRAFT_TYPE] = aircraftType

    print("Change Aircraft:".. aircraft_type_text[aircraftType])

    -- Setup Default Aircraft Wing/Tail 
    if (aircraftType==AIRCRAFT_TYPE.PLANE) then
        ModelLib.ST_PlaneWingInit(WING_TYPE.AIL_1)
        ModelLib.ST_PlaneTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    elseif (aircraftType==AIRCRAFT_TYPE.GLIDER) then
        ModelLib.ST_GliderWingInit(WING_TYPE.AIL_1)
        ModelLib.ST_GliderTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    else 
        ModelLib.ST_PlaneWingInit(WING_TYPE.AIL_1)
        ModelLib.ST_PlaneTailInit(TAIL_TYPE.RUD_1_ELEV_1)
    end

   
end


-- Setup Initial Default Data for the Menus
function ModelLib.ST_Default_Data()
    print("Initializing Menu DATA")
    ModelLib.ST_AircraftInit(AIRCRAFT_TYPE.PLANE)

    print("Initializing Servo Reverse from TX output settings")

    MENU_DATA[MEMU_VAR.PORT1_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT1].revert
    MENU_DATA[MEMU_VAR.PORT2_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT2].revert
    MENU_DATA[MEMU_VAR.PORT3_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT3].revert
    MENU_DATA[MEMU_VAR.PORT4_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT4].revert
    MENU_DATA[MEMU_VAR.PORT5_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT5].revert
    MENU_DATA[MEMU_VAR.PORT6_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT6].revert
    MENU_DATA[MEMU_VAR.PORT7_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT7].revert
    MENU_DATA[MEMU_VAR.PORT8_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT8].revert
    MENU_DATA[MEMU_VAR.PORT9_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT9].revert
    MENU_DATA[MEMU_VAR.PORT10_MODE] = CH_MODE_TYPE.NORMAL + MODEL.modelOutputChannel[PORT.PORT10].revert

    ModelLib.printServoReverseInfo()

end


ModelLib.TX_CHANNELS = TX_CHANNELS
ModelLib.MODEL = MODEL
ModelLib.CH_TYPE = CH_TYPE
ModelLib.CH_MODE_TYPE = CH_MODE_TYPE
ModelLib.AIRCRAFT_TYPE = AIRCRAFT_TYPE
ModelLib.WING_TYPE = WING_TYPE
ModelLib.TAIL_TYPE = TAIL_TYPE
ModelLib.MENU_DATA = MENU_DATA
ModelLib.MEMU_VAR  = MEMU_VAR
ModelLib.PORT      = PORT
ModelLib.DATA_PATH = DATA_PATH

ModelLib.aircraft_type_text = aircraft_type_text
ModelLib.wing_type_text = wing_type_text
ModelLib.tail_type_text = tail_type_text


return ModelLib

