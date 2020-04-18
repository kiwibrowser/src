@echo off
setlocal

PATH=%CYGWIN%;%PATH%
REM Use the path to this file (httpd.cmd) to get the
REM path to httpd.py, so that we can run httpd.cmd from
REM any directory.  Pass up to 9 arguments to httpd.py.
python %~dp0\..\tools\httpd.py %1 %2 %3 %4 %5 %6 %7 %8 %9
