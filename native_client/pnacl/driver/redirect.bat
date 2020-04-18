@echo off
:: Copyright (c) 2012 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

:: This file is installed for each driver file so that the driver can
:: be run from outside Cygwin. This file will be installed as bin/pnacl-*.bat
:: inside the PNaCl directory. For example, if you invoke:
::
::  C:\> path\to\pnacl\bin\pnacl-gcc hello.c -o hello.pexe
::
:: inside the Windows command prompt, then Windows runs pnacl-gcc.bat.
:: This script will in turn run:
:: "python path\to\pnacl\bin\pydir/loader.py pnacl-gcc hello.c -o hello.pexe"

setlocal

:: Stop incessant CYGWIN complains about "MS-DOS style path"
set CYGWIN=nodosfilewarning %CYGWIN%

:: Run the driver
:: For now, assume "python" is in the PATH.
python -OO "%~dp0\pydir\loader.py" "%~n0" %*

:end
