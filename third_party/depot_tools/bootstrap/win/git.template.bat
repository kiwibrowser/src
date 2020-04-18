@echo off
setlocal
if not defined EDITOR set EDITOR=notepad
set PATH=%~dp0${GIT_BIN_RELDIR}\cmd;%~dp0;%PATH%
"%~dp0${GIT_BIN_RELDIR}\${GIT_PROGRAM}" %*
