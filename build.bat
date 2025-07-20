@echo off

set PEM_KEY = private_key

cd sign_tool

if not exist "%PEM_KEY%" (
    python gen_key.py
)

python make_image.py -p
copy /Y .\key_data.c ..\first_boot\Bootloader\src\key_data.c
copy /Y .\key_data.c ..\second_boot\Bootloader\src\key_data.c

cd ..\application
make rebuild
copy /Y Build\application.bin ..\sign_tool\application.bin

cd ..\second_boot
make rebuild
copy /Y Build\second_boot.bin ..\sign_tool\second_boot.bin

cd ..\sign_tool
python make_image.py -f
copy /Y .\firmware.bin ..\update_firmware\x64\Debug\firmware.bin
