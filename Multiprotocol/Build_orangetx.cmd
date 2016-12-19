@echo off
if "%AVR32_HOME%"=="" (
echo.
echo You must install winavr to compile Multi for OrangeTX: https://sourceforge.net/projects/winavr/
echo.
pause
exit /b
)
if exist MultiOrange.cpp.orangetx ren *.orangetx *.
if exist .dep (make clean)
md .dep
make
if exist MultiOrange.hex (
avr-objcopy -I ihex MultiOrange.hex -O binary MultiOrange.bin
echo.
echo Compilation OK.
echo Use MultiOrange.hex or MultiOrange.bin to program your OrangeTX module.
echo.
)
pause
