@echo off

echo Installing MULTI-Module DFU Bootloader Driver...
"%~dp0wdi-simple" --vid 0x1EAF --pid 0x0003 --type 2 --name "MULTI-Module DFU Bootloader" --dest "%~dp0MULTI-DFU-Bootloader" -b
echo.

echo Installing MULTI-Module USB Serial Driver...
"%~dp0wdi-simple" --vid 0x1EAF --pid 0x0004 --type 3 --name "MULTI-Module USB Serial" --dest "%~dp0MULTI-USB-Serial" -b
echo.

pause
