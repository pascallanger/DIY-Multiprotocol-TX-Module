local LogLib = {  }

local LOG_FILE = "/LOGS/dsm_log.txt"
local logFile  = nil
local logCount=0

function LogLib.LOG_open()  
    logFile = io.open(LOG_FILE, "w")  -- Truncate Log File 
end

function LogLib.LOG_write(...)
    if (logFile==nil) then LogLib.LOG_open() end
    local str = string.format(...)

    if (str==nil) then return end

    io.write(logFile, str)

    str = string.gsub(str,"\n"," ") -- Elimitate return from line, since print will do it
    print(str)

    if (logCount > 10) then  -- Close an re-open the file
        io.close(logFile)
        logFile = io.open(LOG_FILE, "a")
        logCount =0
    end
end

function LogLib.LOG_close()
    if (logFile~=nil) then io.close(logFile) end
end

return LogLib
