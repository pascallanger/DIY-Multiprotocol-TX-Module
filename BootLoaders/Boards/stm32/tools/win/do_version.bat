@ECHO OFF
SETLOCAL EnableDelayedExpansion

REM SET DEBUG=1

SET BUILD_PATH=%1
SET PROJECT_NAME=%2
SET SKETCH_PATH=%3
SET MULTI_BOARD=%4
SET BOARD_VERSION=%5
SET EXPORT_FLAG=%6

REM Remove double-quotes from the paths
SET BUILD_PATH=%BUILD_PATH:"=%
SET SKETCH_PATH=%SKETCH_PATH:"=%

IF %MULTI_BOARD%==MULTI_NO_BOOT SET MULTI_TYPE=avr
IF %MULTI_BOARD%==MULTI_FLASH_FROM_TX SET MULTI_TYPE=avr
IF %MULTI_BOARD%==MULTI_STM32_NO_BOOT SET MULTI_TYPE=stm
IF %MULTI_BOARD%==MULTI_STM32_FLASH_FROM_TX SET MULTI_TYPE=stm
IF %MULTI_BOARD%==MULTI_ORANGERX SET MULTI_TYPE=orx

IF DEFINED DEBUG (
  ECHO.
	ECHO Sketch Path: "%SKETCH_PATH%"
	ECHO Multi Board: %MULTI_BOARD%
  ECHO Multi Board Type: %MULTI_TYPE%
  ECHO.
)

IF EXIST "%BUILD_PATH%\sketch\Multiprotocol.h" (
  IF DEFINED DEBUG ECHO Getting Multi firmware version from "%BUILD_PATH%\sketch\Multiprotocol.h"
  FOR /F "tokens=* usebackq" %%A in (`%SystemRoot%\system32\findstr.exe /C:"#define VERSION_MAJOR" "%BUILD_PATH%\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%A") do SET MAJOR_VERSION=%%i
  FOR /F "tokens=* usebackq" %%B in (`%SystemRoot%\system32\findstr.exe /C:"#define VERSION_MINOR" "%BUILD_PATH%\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%B") do SET MINOR_VERSION=%%i
  FOR /F "tokens=* usebackq" %%C in (`%SystemRoot%\system32\findstr.exe /C:"#define VERSION_REVISION" "%BUILD_PATH%\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%C") do SET REVISION_VERSION=%%i
  FOR /F "tokens=* usebackq" %%D in (`%SystemRoot%\system32\findstr.exe /C:"#define VERSION_PATCH_LEVEL" "%BUILD_PATH%\sketch\Multiprotocol.h"`) DO FOR /F "tokens=3" %%i in ("%%D") do SET PATCH_VERSION=%%i
  SET MULTI_VER=!MAJOR_VERSION!.!MINOR_VERSION!.!REVISION_VERSION!.!PATCH_VERSION!
) ELSE (
  SET MULTI_VER=
)

IF DEFINED DEBUG (
  ECHO.
  ECHO Multi Firmware Version: %MULTI_VER%
  ECHO.
)

REM Copy the compiled file to the sketch folder with the version number in the file name
IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.hex" (
  IF DEFINED DEBUG ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y
  COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y >NUL
)

IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.bin" (
  IF DEFINED DEBUG ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y
  COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%BUILD_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y >NUL
)
  
IF "%EXPORT_FLAG%"=="EXPORT" (
REM Copy the compiled file to the sketch folder with the version number in the file name
	IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.hex" (
		IF DEFINED DEBUG ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y
	  COPY "%BUILD_PATH%\%PROJECT_NAME%.hex" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.hex" /Y >NUL
	)

	IF EXIST "%BUILD_PATH%\%PROJECT_NAME%.bin" (
		IF DEFINED DEBUG ECHO COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y
	  COPY "%BUILD_PATH%\%PROJECT_NAME%.bin" "%SKETCH_PATH%\multi-%MULTI_TYPE%-%MULTI_VER%.bin" /Y >NUL
	)

  IF EXIST "%SKETCH_PATH%\multi-%MULTI_TYPE%.bin" (
		IF DEFINED DEBUG ECHO DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.bin"
    DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.bin"  >NUL
  )
  IF EXIST "%SKETCH_PATH%\multi-%MULTI_TYPE%.hex" (
		IF DEFINED DEBUG ECHO DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.hex"
    DEL "%SKETCH_PATH%\multi-%MULTI_TYPE%.hex"  >NUL
  )
)
