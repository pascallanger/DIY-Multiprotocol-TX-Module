@ECHO OFF
SETLOCAL EnableDelayedExpansion

SET BUILD_PATH=%1
SET PROJECT_NAME=%2
SET SKETCH_PATH=%3
SET MULTI_BOARD=%4
SET EXPORT_FLAG=%5

REM ECHO Multi board: %MULTI_BOARD%

SET MULTI_TYPE=otx

IF EXIST "%1\sketch\Multiprotocol.h" (
  REM ECHO Getting Multi-MODULE firmware version from "%1\sketch\Multiprotocol.h"
  FOR /F "tokens=* usebackq skip=2" %%A in (`find "#define VERSION_MAJOR" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%A") do SET MAJOR_VERSION=%%i
  FOR /F "tokens=* usebackq skip=2" %%B in (`find "#define VERSION_MINOR" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%B") do SET MINOR_VERSION=%%i
  FOR /F "tokens=* usebackq skip=2" %%C in (`find "#define VERSION_REVISION" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%C") do SET REVISION_VERSION=%%i
  FOR /F "tokens=* usebackq skip=2" %%D in (`find "#define VERSION_PATCH_LEVEL" "%1\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%D") do SET PATCH_VERSION=%%i
  SET MULTI_VER=!MAJOR_VERSION!.!MINOR_VERSION!.!REVISION_VERSION!.!PATCH_VERSION!
) ELSE (
  SET MULTI_VER=
)

REM ECHO Multi-MODULE firmware version: %MULTI_VER%

REM Copy the compiled file to the sketch folder with the version number in the file name
IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.hex" (
  REM ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y
  COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y >NUL
)

IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.bin" (
  REM ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y
  COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y >NUL
)
  
IF "%EXPORT_FLAG%"=="EXPORT" (
REM Copy the compiled file to the sketch folder with the version number in the file name
	IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.hex" (
	  COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y >NUL
	)

	IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.bin" (
	  COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y >NUL
	)

  IF EXIST "%SKETCH_PATH%\multi-%MULTI_TYPE%.bin" (
    DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.bin"  >NUL
  )
  IF EXIST "%SKETCH_PATH%\multi-%MULTI_TYPE%.hex" (
    DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.hex"  >NUL
  )
)
