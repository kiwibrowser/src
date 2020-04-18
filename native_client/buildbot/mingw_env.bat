:: Copyright (c) 2012 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

@echo off

:: Use cygwin for wget and unzip.
setlocal
if not exist "%~dp0..\cygwin" call "%~dp0cygwin_env.bat"
endlocal

setlocal
set HERMETIC_MINGW=mingw-w64-gcc-4.7.2-20120716
set HERMETIC_MSYS=MSYS-20111123
set MINGW_MIRROR=http://commondatastorage.googleapis.com/nativeclient-mirror/nacl/mingw-mirror
:: Sources can be downloaded at %MINGW_MIRROR%/%HERMETIC_MINGW%-src.zip
:: and %MINGW_MIRROR%/%HERMETIC_MSYS%-src.zip accordingly.

if exist "%~dp0..\mingw\%HERMETIC_MINGW%.installed" goto :skip_mingw_install
if not exist "%~dp0..\mingw" goto :dont_remove_mingw
rmdir /s /q "%~dp0..\mingw"
if errorlevel 1 goto :mingw_rmdir_fail
mkdir "%~dp0..\mingw"
:dont_remove_mingw
"%~dp0..\cygwin\bin\wget" %MINGW_MIRROR%/%HERMETIC_MINGW%.zip -O "%~dp0%HERMETIC_MINGW%.zip"
if errorlevel 1 goto :mingw_download_fail
"%~dp0..\cygwin\bin\unzip" "%~dp0%HERMETIC_MINGW%.zip" -d "%~dp0..\mingw"
if errorlevel 1 goto :mingw_unzip_fail
echo Sources can be found at %MINGW_MIRROR%/%HERMETIC_MINGW%-src.zip > "%~dp0..\mingw\%HERMETIC_MINGW%.installed"
:skip_mingw_install

if exist "%~dp0..\mingw\msys\%HERMETIC_MSYS%.installed" goto :skip_msys_install
if not exist "%~dp0..\mingw\msys" goto :dont_remove_msys
rmdir /s /q "%~dp0..\mingw\msys"
del "%~dp0..\mingw\Readme.txt"
if errorlevel 1 goto :msys_rmdir_fail
mkdir "%~dp0..\mingw\msys"
:dont_remove_msys
"%~dp0..\cygwin\bin\wget" %MINGW_MIRROR%/%HERMETIC_MSYS%.zip -O "%~dp0%HERMETIC_MSYS%.zip"
if errorlevel 1 goto :msys_download_fail
"%~dp0..\cygwin\bin\unzip" "%~dp0%HERMETIC_MSYS%.zip" -d "%~dp0..\mingw"
if errorlevel 1 goto :msys_unzip_fail
echo Sources can be found at %MINGW_MIRROR%/%HERMETIC_MSYS%-src.zip > "%~dp0..\mingw\msys\%HERMETIC_MSYS%.installed"
echo %~dp0..\mingw /mingw > "%~dp0..\mingw\msys\etc\fstab"
:skip_msys_install

endlocal
goto :end

:mingw_rmdir_fail
echo "Failed to remove MinGW directory"
goto :err
:msys_rmdir_fail
echo "Failed to remove MSYS directory"
goto :err
:mingw_unzip_fail
echo "Failed to unzip MinGW"
goto :err
:msys_unzip_fail
echo "Failed to unzip MSYS"
goto :err
:mingw_download_fail
echo "Failed to download MinGW"
goto :err
:msys_download_fail
echo "Failed to download MSYS"
goto :err

:err
endlocal
set ERRORLEVEL=1
:end