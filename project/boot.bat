@echo off
cd /d "%~dp0"
if not exist ".\Objects" mkdir ".\Objects"
if not exist ".\Listings" mkdir ".\Listings"
"..\tools\srec_cat.exe" -Disable_Sequence_Warning ".\Objects\T5L51.hex" -Intel -o ".\T5L51_SHJ_BOOT.bin" -Binary
if exist "..\T5L51_SHJ_BOOT.bin" del "..\T5L51_SHJ_BOOT.bin"
move ".\T5L51_SHJ_BOOT.bin" "..\"
echo.
