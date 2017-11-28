@ECHO OFF

FOR /F "tokens=* usebackq skip=2" %%A in (`find "#define VERSION_MAJOR" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%A") do SET MAJOR_VERSION=%%i
FOR /F "tokens=* usebackq skip=2" %%B in (`find "#define VERSION_MINOR" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%B") do SET MINOR_VERSION=%%i
FOR /F "tokens=* usebackq skip=2" %%C in (`find "#define VERSION_REVISION" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%C") do SET REVISION_VERSION=%%i
FOR /F "tokens=* usebackq skip=2" %%D in (`find "#define VERSION_PATCH_LEVEL" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%D") do SET PATCH_VERSION=%%i

SET MULTI_VER=%MAJOR_VERSION%.%MINOR_VERSION%.%REVISION_VERSION%.%PATCH_VERSION%
ECHO %MULTI_VER%
