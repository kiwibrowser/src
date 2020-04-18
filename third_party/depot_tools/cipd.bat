@echo off
:: Copyright (c) 2016 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

setlocal

:: To allow this powershell script to run if it was a byproduct of downloading
:: and unzipping the depot_tools.zip distribution, we clear the Zone.Identifier
:: alternate data stream. This is equivalent to clicking the "Unblock" button
:: in the file's properties dialog.
set errorlevel=
if not exist "%~dp0.cipd_client.exe" (
  echo.>%~dp0cipd.ps1:Zone.Identifier

  powershell -NoProfile -ExecutionPolicy RemoteSigned -Command "%~dp0cipd.ps1" < nul
  if not errorlevel 0 goto :END
)

for /f %%i in (%~dp0cipd_client_version) do set CIPD_CLIENT_VER=%%i
"%~dp0.cipd_client.exe" selfupdate -version "%CIPD_CLIENT_VER%"
if not errorlevel 0 goto :END

"%~dp0.cipd_client.exe" %*

:END
endlocal & (
  set EXPORT_ERRORLEVEL=%ERRORLEVEL%
)
exit /b %EXPORT_ERRORLEVEL%
