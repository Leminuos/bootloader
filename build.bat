@echo off

call env.bat

cd sign_tool

if not exist %PEM_KEY% (
    python gen_key.py >> "%LOG_FILE%" 2>&1
)

type nul > "%LOG_FILE%"

python make_image.py -p >> "%LOG_FILE%" 2>&1
copy /Y .\key_data.c ..\first_boot\Bootloader\src\key_data.c >> "%LOG_FILE%" 2>&1
copy /Y .\key_data.c ..\second_boot\Bootloader\src\key_data.c >> "%LOG_FILE%" 2>&1

cd ..\application
make rebuild >> "%LOG_FILE%" 2>&1
copy /Y Build\application.bin ..\sign_tool\application.bin >> "%LOG_FILE%" 2>&1

cd ..\second_boot
make rebuild >> "%LOG_FILE%" 2>&1
copy /Y Build\second_boot.bin ..\sign_tool\second_boot.bin >> "%LOG_FILE%" 2>&1

cd ..\sign_tool
python make_image.py -f >> "%LOG_FILE%" 2>&1

firmware.exe -f firmware.bin