@echo off
:: Copyright (c) 2011 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

setlocal

:: If this batch file is run directly thru cygwin, we will get the wrong
:: version of python. To avoid this, if we detect cygwin, we need to then
:: invoke the shell script which will then re-invoke this batch file with
:: cygwin stripped out of the path.
:: Detect cygwin by trying to run bash.
bash --version >NUL 2>&1
if %ERRORLEVEL% == 0 (
    bash "%~dp0\scons" %* || exit 1
    goto end
)

:: Preserve a copy of the PATH (in case we need it later, mainly for cygwin).
set PRESCONS_PATH=%PATH%

:: Stop incessant CYGWIN complains about "MS-DOS style path"
set CYGWIN=nodosfilewarning %CYGWIN%

:: Run the included copy of scons.
python "%~dp0\scons.py" %*

:end
