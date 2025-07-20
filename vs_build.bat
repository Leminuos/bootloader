@echo off

SET "ENV_BUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"

pushd update_firmware
call "%ENV_BUILD%\vcvarsall.bat" x64
msbuild Project2.sln /p:Configuration=Release /p:Platform=x64 /m
popd