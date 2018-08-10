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

echo stm32flash -v -g %2 -b 57600 -w %fwpath% %1 
echo.

stm32flash -v -g %2 -b 57600 -w %fwpath% %1 
