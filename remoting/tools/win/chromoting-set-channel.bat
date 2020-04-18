@echo off

REM Copyright (c) 2012 The Chromium Authors. All rights reserved.
REM Use of this source code is governed by a BSD-style license that can be
REM found in the LICENSE file.

set CHANNEL=%1

REM Check if we are running as an Administrator.
REM Based on method described at:
REM http://stackoverflow.com/questions/4051883/batch-script-how-to-check-for-admin-rights
net session >nul 2>&1
if not %errorlevel% equ 0 (
  echo This script updates the registry and needs to be run as Administrator.
  echo Right-click "Command Prompt" and select "Run as Administrator" and run
  echo this script from there.
  goto :eof
)

REM Make sure the argument specifies a valid channel.
if "_%CHANNEL%_"=="_beta_" goto validarg
if "_%CHANNEL%_"=="_stable_" goto validarg
goto usage

:validarg
set SYSTEM32=%SystemRoot%\system32
if "_%PROCESSOR_ARCHITECTURE%_"=="_AMD64_" set SYSTEM32=%SystemRoot%\syswow64

set REGKEY="HKLM\SOFTWARE\Google\Update\ClientState\{B210701E-FFC4-49E3-932B-370728C72662}"
set VALUENAME=ap

if "_%CHANNEL%_"=="_stable_" (
  %SYSTEM32%\reg.exe delete %REGKEY% /v %VALUENAME% /f
  echo ********************
  echo You're not done yet!
  echo ********************
  echo You must now UNINSTALL and RE-INSTALL the latest version of Chrome
  echo Remote Desktop to get your machine back on the stable channel.
  echo Thank you!
) else (
  %SYSTEM32%\reg.exe add %REGKEY% /v %VALUENAME% /d %CHANNEL% /f
  echo Switch to %CHANNEL% channel complete.
  echo You will automatically get %CHANNEL% binaries during the next update.
)
goto :eof

:usage
echo Usage: %0 ^<channel^>
echo where ^<channel^> is 'beta' or 'stable'.
