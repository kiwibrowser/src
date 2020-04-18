@echo off
:: Copyright (c) 2012 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

:: This batch file will try to sync the root directory.

setlocal

:: Windows freaks out if a file is overwritten while it's being executed.  Copy
:: this script off to a temporary location and reinvoke from there before
:: running any git commands.
IF "%~nx0"=="update_depot_tools.bat" (
  COPY /Y "%~dp0update_depot_tools.bat" "%TEMP%\update_depot_tools_tmp.bat" >nul
  if errorlevel 1 goto :EOF
  "%TEMP%\update_depot_tools_tmp.bat" "%~dp0" %*
)

set DEPOT_TOOLS_DIR=%~1
SHIFT

IF EXIST "%DEPOT_TOOLS_DIR%.disable_auto_update" GOTO :EOF

set GIT_URL=https://chromium.googlesource.com/chromium/tools/depot_tools.git

:: Will download git and python.
call "%DEPOT_TOOLS_DIR%bootstrap\win\win_tools.bat"
if errorlevel 1 goto :EOF
:: Now clear errorlevel so it can be set by other programs later.
set errorlevel=

:: Shall skip automatic update?
IF "%DEPOT_TOOLS_UPDATE%" == "0" GOTO :EOF

:: We need .\.git\. to be able to sync.
IF EXIST "%DEPOT_TOOLS_DIR%.git\." GOTO :GIT_UPDATE
echo Error updating depot_tools, no revision tool found.
goto :EOF


:GIT_UPDATE
cd /d "%DEPOT_TOOLS_DIR%."
call git config remote.origin.fetch > NUL
for /F %%x in ('git config --get remote.origin.url') DO (
  IF not "%%x" == "%GIT_URL%" (
    echo Your depot_tools checkout is configured to fetch from an obsolete URL
    choice /N /T 60 /D N /M "Would you like to update it? [y/N]: "
    IF not errorlevel 2 (
      call git config remote.origin.url "%GIT_URL%"
    )
  )
)
call git fetch -q origin > NUL
call git rebase -q origin/master > NUL
if errorlevel 1 (
  echo Failed to update depot_tools.
  goto :EOF
)

:: Sync CIPD and CIPD client tools.
call "%~dp0\cipd_bin_setup.bat"
