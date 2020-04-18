@echo off
:: Copyright 2017 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

REM Execute whatever is printed by autoninja.py.
REM Also print it to reassure that the right settings are being used.
FOR /f "usebackq tokens=*" %%a in (`python %~dp0autoninja.py "%*"`) do echo %%a & %%a
@if errorlevel 1 goto buildfailure
@if "%NINJA_SUMMARIZE_BUILD%" == "1" python %~dp0post_build_ninja_summary.py %*
exit /b
:buildfailure
REM Return an error code of 1 so that if a developer types:
REM "autoninja chrome && chrome" then chrome won't run if the build fails.
cmd /c exit 1
