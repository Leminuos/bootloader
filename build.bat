@echo off

call env.bat
type nul > "%LOG_FILE%"

if not exist sign_tool\firmware.exe (
    call vs_build.bat >> "%LOG_FILE%" 2>&1
    copy /Y update_firmware\x64\Release\firmware.exe sign_tool\firmware.exe >> "%LOG_FILE%" 2>&1
)

cd sign_tool

if not exist %PEM_KEY% (
    python gen_key.py >> "%LOG_FILE%" 2>&1
)

python make_image.py -p >> "%LOG_FILE%" 2>&1
copy /Y .\key_data.c ..\second_boot\Bootloader\src\key_data.c >> "%LOG_FILE%" 2>&1

cd ..\application
make rebuild >> "%LOG_FILE%" 2>&1
copy /Y Build\application.bin ..\sign_tool\application.bin >> "%LOG_FILE%" 2>&1

cd ..\second_boot
make rebuild >> "%LOG_FILE%" 2>&1
copy /Y Build\second_boot.bin ..\sign_tool\second_boot.bin >> "%LOG_FILE%" 2>&1

cd ..\sign_tool
python make_image.py -f >> "%LOG_FILE%" 2>&1
del key_data.c
del second_boot.bin

if "%LOAD_TYPE%"=="LOAD_FULL" (
    firmware.exe -f firmware.bin
)

if "%LOAD_TYPE%"=="LOAD_APPL" (
    firmware.exe -f application.bin 0x10000
)
