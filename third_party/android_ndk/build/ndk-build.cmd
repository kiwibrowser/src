@echo off
set NDK_ROOT=%~dp0\..
set PREBUILT_PATH=%NDK_ROOT%\prebuilt\windows-x86_64
if exist %PREBUILT_PATH% goto FOUND
set PREBUILT_PATH=%NDK_ROOT%\prebuilt\windows
:FOUND
"%PREBUILT_PATH%\bin\make.exe" -f "%NDK_ROOT%\build\core\build-local.mk" SHELL=cmd %*
