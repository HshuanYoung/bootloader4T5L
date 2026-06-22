@echo off
cd /d "%~dp0"
if not exist ".\Objects" mkdir ".\Objects"
if not exist ".\Listings" mkdir ".\Listings"
"..\tools\srec_cat.exe" -Disable_Sequence_Warning ".\Objects\T5L51.hex" -Intel -o ".\T5L51_BOOTLOADER.bin" -Binary
if exist "..\T5L51_BOOTLOADER.bin" del "..\T5L51_BOOTLOADER.bin"
move ".\T5L51_BOOTLOADER.bin" "..\"
echo.
