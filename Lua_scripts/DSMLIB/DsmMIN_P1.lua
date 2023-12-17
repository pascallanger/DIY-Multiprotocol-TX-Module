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

local DATA_PATH           = "/MODELS/DSMDATA"
local TX_CHANNELS         = 12

local MV_DATA_END        = 1040


  local function hashName(mName)
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
  local function ST_LoadFileData() 
    local fname = hashName(MODEL.modelName)..".txt"
    MODEL.hashName = fname
  
    -- Clear Menu Data
    for i = 0, MV_DATA_END do
        M_DATA[i]=nil
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
        M_DATA[k+0]=v+0 -- do aritmentic to convert string to number
        i=i+1
    end
  
    -- Return 0 if no lines processed, 1 otherwise
    if (i > 0) then return 1 else return 0 end
  end

  local function getModuleChannelOrder(num) 
    --Determine fist 4 channels order
    local ch_n={}
    local st_n = {[0]= "R", "E", "T", "A" }
    local c_ord=num -- ch order
    if (c_ord == -1) then
      ch_n[0] = st_n[3]
      ch_n[1] = st_n[1]
      ch_n[2] = st_n[2]
      ch_n[3] = st_n[0]
    else
      ch_n[bit32.band(c_ord,3)] = st_n[3]
      c_ord = math.floor(c_ord/4)
      ch_n[bit32.band(c_ord,3)] = st_n[1]
      c_ord = math.floor(c_ord/4)
      ch_n[bit32.band(c_ord,3)] = st_n[2]
      c_ord = math.floor(c_ord/4)
      ch_n[bit32.band(c_ord,3)] = st_n[0]
    end
  
    local s = ""
    for i=0,3 do
      s=s..ch_n[i]
    end
    return s
  end
  
  local function ReadTxModelData()
    local TRANSLATE_AETR_TO_TAER=false
    local table = model.getInfo()   -- Get the model name 
    MODEL.modelName = table.name
  
    local module = model.getModule(0) -- Internal
    if (module==nil or module.Type~=6) then module = model.getModule(1) end -- External
    if (module~=nil) then
        if (module.Type==6 ) then -- MULTI-MODULE
            local chOrder = module.channelsOrder
            local s = getModuleChannelOrder(chOrder)
            LOG_write("MultiChannel Ch Order: [%s]  %s\n",chOrder,s) 
  
            if (s=="AETR") then TRANSLATE_AETR_TO_TAER=true 
            else TRANSLATE_AETR_TO_TAER=false 
            end
        end
    end
  
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
  
    if (TRANSLATE_AETR_TO_TAER) then 
        LOG_write("Applying  AETR -> TAER translation\n") 
        local ail = MODEL.modelOutputChannel[0]
        local elv = MODEL.modelOutputChannel[1]
        local thr = MODEL.modelOutputChannel[2]
  
        MODEL.modelOutputChannel[0] = thr
        MODEL.modelOutputChannel[1] = ail
        MODEL.modelOutputChannel[2] = elv
    end
  
    -- Create the Port Text to be used 
    LOG_write("Ports/Channels:\n") 
    for i = 0, TX_CHANNELS-1 do 
        local ch =  MODEL.modelOutputChannel[i]
        if (ch~=nil) then
            MODEL.TX_CH_TEXT[i] = ch.formatCh
            MODEL.PORT_TEXT[i] = string.format("P%i (%s) ",i+1,MODEL.TX_CH_TEXT[i])  
            LOG_write("Port%d %s [%d,%d] Rev=%d, Off=%d, ppmC=%d, syn=%d\n",i+1,MODEL.TX_CH_TEXT[i],math.floor(ch.min/10),math.floor(ch.max/10), ch.revert, ch.offset, ch.ppmCenter, ch.symetrical)
        end
    end
  end
  
  -- Main Program

  LOG_write("Reading Model Info\n")
  ReadTxModelData()
  local r = ST_LoadFileData()
  return r
  

