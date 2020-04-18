:: Copyright (c) 2011 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

@echo off
setlocal
set CYGWIN_VERSION=cygwin_1_7_24-1_0
set HERMETIC_CYGWIN=hermetic_%CYGWIN_VERSION%
if exist "%~dp0..\cygwin\%CYGWIN_VERSION%.installed" goto :skip_cygwin_install
if exist "%~dp0..\toolchain_build\src\binutils" rmdir /s /q "%~dp0..\toolchain_build\src\binutils"
if exist "%~dp0..\toolchain_build\src\clang" rmdir /s /q "%~dp0..\toolchain_build\src\clang"
if exist "%~dp0..\toolchain_build\src\compiler-rt" rmdir /s /q "%~dp0..\toolchain_build\src\compiler-rt"
if exist "%~dp0..\toolchain_build\src\dummydir" rmdir /s /q "%~dp0..\toolchain_build\src\dummydir"
if exist "%~dp0..\toolchain_build\src\pnacl-gcc" rmdir /s /q "%~dp0..\toolchain_build\src\pnacl-gcc"
if exist "%~dp0..\toolchain_build\src\llvm" rmdir /s /q "%~dp0..\toolchain_build\src\llvm"
if exist "%~dp0..\toolchain_build\src\llvm-test-suite" rmdir /s /q "%~dp0..\toolchain_build\src\llvm-test-suite"
if exist "%~dp0..\tools\BUILD\.gcc-extras-version" del "%~dp0..\tools\BUILD\.gcc-extras-version"
if exist "%~dp0..\tools\BACKPORTS\binutils" rmdir /s /q "%~dp0..\tools\BACKPORTS\binutils"
if exist "%~dp0..\tools\BACKPORTS\gcc" rmdir /s /q "%~dp0..\tools\BACKPORTS\gcc"
if exist "%~dp0..\tools\BACKPORTS\gdb" rmdir /s /q "%~dp0..\tools\BACKPORTS\gdb"
if exist "%~dp0..\tools\BACKPORTS\glibc" rmdir /s /q "%~dp0..\tools\BACKPORTS\glibc"
if exist "%~dp0..\tools\BACKPORTS\linux-headers-for-nacl" rmdir /s /q "%~dp0..\tools\BACKPORTS\linux-headers-for-nacl"
if exist "%~dp0..\tools\BACKPORTS\newlib" rmdir /s /q "%~dp0..\tools\BACKPORTS\newlib"
if not exist "%~dp0..\cygwin" goto :dont_remove_cygwin
rmdir /s /q "%~dp0..\cygwin"
if errorlevel 1 goto :rmdir_fail
mkdir "%~dp0..\cygwin"
:dont_remove_cygwin
cscript //nologo //e:jscript "%~dp0get_file.js" https://commondatastorage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/%HERMETIC_CYGWIN%.exe "%~dp0%HERMETIC_CYGWIN%.exe"
if errorlevel 1 goto :download_fail
:download_success
start /WAIT %~dp0%HERMETIC_CYGWIN%.exe /DEVEL /S /D=%~dp0..\cygwin
if errorlevel 1 goto :install_fail
set CYGWIN=nodosfilewarning
"%~dp0..\cygwin\bin\touch" "%~dp0..\cygwin\%CYGWIN_VERSION%.installed"
if errorlevel 1 goto :install_fail
del /f /q "%~dp0%HERMETIC_CYGWIN%.exe"
:skip_cygwin_install
endlocal
set "PATH=%~dp0..\cygwin\bin;%PATH%"
goto :end
:rmdir_fail
endlocal
echo Failed to remove old version of cygwin
set ERRORLEVEL=1
goto :end
:download_fail
:: TODO(bradnelson): Check certs when this issue is resolved.
::     http://code.google.com/p/nativeclient/issues/detail?id=2931
c:\cygwin\bin\wget --no-check-certificate https://commondatastorage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/%HERMETIC_CYGWIN%.exe -O "%~dp0%HERMETIC_CYGWIN%.exe"
if errorlevel 1 goto :wget_fail
goto download_success
:wget_fail
c:\cygwin\bin\wget --no-check-certificate https://commondatastorage.googleapis.com/nativeclient-mirror/nacl/cygwin_mirror/%HERMETIC_CYGWIN%.exe -O "%~dp0%HERMETIC_CYGWIN%.exe"
if errorlevel 1 goto :cygwin_wget_fail
goto download_success
:cygwin_wget_fail
endlocal
echo Failed to download cygwin
set ERRORLEVEL=1
goto :end
:install_fail
endlocal
echo Failed to install cygwin
set ERRORLEVEL=1
goto :end
:end
