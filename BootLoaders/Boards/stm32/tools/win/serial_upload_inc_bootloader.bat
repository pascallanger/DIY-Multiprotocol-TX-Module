@echo off
rem Note %~dp0 get path of this batch file
rem Need to change drive if My Documents is on a drive other than C:
set driverLetter=%~dp0
set driverLetter=%driverLetter:~0,2%
%driverLetter%
cd %~dp0

rem The lines below are needed to fix path issues with incorrect slashes before the bin file name
set fwpath=%3
set fwpath=%fwpath:/=\%

set blpath=%4
set blpath=%blpath:/=\%

rem Erase the flash
echo stm32flash -o -b 57600 %1
stm32flash -o -b 57600 %1
if %errorlevel% neq 0 exit /b %errorlevel%

rem Write the Multi bootloader
echo stm32flash.exe -v -g 0x8000000 -b 57600 -w %blpath% %1
stm32flash.exe -v -g 0x8000000 -b 57600 -w %blpath% %1
if %errorlevel% neq 0 exit /b %errorlevel%

rem Write the Multi firmware
echo stm32flash -v -s 8 -e 0 -g 0x8002000 -b 57600 -w %fwpath% %1
stm32flash -v -s 8 -e 0 -g 0x8002000 -b 57600 -w %fwpath% %1
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
