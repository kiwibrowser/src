@echo off

set SHARE_NAME=\\chrome-dev\chrome
set DRIVE_LETTER=%1
set PATH=%SystemRoot%;%SystemRoot%\system32

net use %DRIVE_LETTER%
if errorlevel 1 goto DRIVE_NOT_MAPPED

net use %DRIVE_LETTER% | find "%SHARE_NAME%" > nul
if not errorlevel 1 goto DRIVE_ALREADY_MAPPED

:WRONG_DRIVE_MAPPED
echo %DRIVE_LETTER% Drive mapped to wrong share, disconnecting..
net use %DRIVE_LETTER% /DELETE
goto MAPDRIVE

:DRIVE_ALREADY_MAPPED
echo %DRIVE_LETTER% Drive already mapped..
goto END

:DRIVE_NOT_MAPPED
echo %DRIVE_LETTER% Drive not mapped..
goto MAPDRIVE

:MAPDRIVE
echo Mapping %DRIVE_LETTER% to %SHARE_NAME%
net use %DRIVE_LETTER% %SHARE_NAME%

:END