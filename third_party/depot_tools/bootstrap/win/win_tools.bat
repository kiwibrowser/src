@echo off
:: Copyright (c) 2017 The Chromium Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

:: This script will determine if python or git binaries need updates. It
:: returns !0 as failure

:: Note: we set EnableDelayedExpansion so we can perform string manipulations
:: in our manifest parsing loop. This only works on Windows XP+.
setlocal EnableDelayedExpansion

:: Get absolute root directory (.js scripts don't handle relative paths well).
pushd %~dp0..\..
set WIN_TOOLS_ROOT_DIR=%CD%
popd

:: Extra arguments to pass to our "win_tools.py" script.
set WIN_TOOLS_EXTRA_ARGS=

:: Determine if we're running a bleeding-edge installation.
if not exist "%WIN_TOOLS_ROOT_DIR%\.bleeding_edge" (
  set CIPD_MANIFEST=manifest.txt
) else (
  set CIPD_MANIFEST=manifest_bleeding_edge.txt
  set WIN_TOOLS_EXTRA_ARGS=%WIN_TOOLS_EXTRA_ARGS% --bleeding-edge
)

:: Parse our CIPD manifest and identify the "cpython" version. We do this by
:: reading it line-by-line, identifying the line containing "cpython", and
:: stripping all text preceding "version:". This leaves us with the version
:: string.
::
:: This method requires EnableDelayedExpansion, and extracts the Python version
:: from our CIPD manifest. Variables referenced using "!" instead of "%" are
:: delayed expansion variables.
for /F "tokens=*" %%A in (%~dp0%CIPD_MANIFEST%) do (
  set LINE=%%A
  if not "x!LINE:cpython=!" == "x!LINE!" set PYTHON_VERSION=!LINE:*version:=!
)
if "%PYTHON_VERSION%" == "" (
  @echo Could not extract Python version from manifest.
  set ERRORLEVEL=1
  goto :END
)

:: We will take the version string, replace "." with "_", and surround it with
:: "win-tools-<PYTHON_VERSION>_bin" so that it matches "win_tools.py"'s cleanup
:: expression and ".gitignore".
::
:: We incorporate PYTHON_VERSION into the "win_tools" directory name so that
:: new installations don't interfere with long-running Python processes if
:: Python is upgraded.
set WIN_TOOLS_NAME=win_tools-%PYTHON_VERSION:.=_%_bin
set WIN_TOOLS_PATH=%WIN_TOOLS_ROOT_DIR%\%WIN_TOOLS_NAME%
set WIN_TOOLS_EXTRA_ARGS=%WIN_TOOLS_EXTRA_ARGS% --win-tools-name "%WIN_TOOLS_NAME%"

:: Install our CIPD packages. The CIPD client self-bootstraps.
:: See "//cipd.bat" and "//cipd.ps1" for more information.
set CIPD_EXE=%WIN_TOOLS_ROOT_DIR%\cipd.bat
call "%CIPD_EXE%" ensure -log-level warning -ensure-file "%~dp0%CIPD_MANIFEST%" -root "%WIN_TOOLS_PATH%"
if errorlevel 1 goto :END

:: This executes "win_tools.py" using the bundle's Python interpreter.
set WIN_TOOLS_PYTHON_BIN=%WIN_TOOLS_PATH%\python\bin\python.exe
call "%WIN_TOOLS_PYTHON_BIN%" "%~dp0win_tools.py" %WIN_TOOLS_EXTRA_ARGS%


:END
set EXPORT_ERRORLEVEL=%ERRORLEVEL%
endlocal & (
  set ERRORLEVEL=%EXPORT_ERRORLEVEL%
)
exit /b %ERRORLEVEL%
