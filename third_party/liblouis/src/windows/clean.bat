@echo off
pushd "%CD%" 
cd %~dp0
erase *.obj
erase liblouis*.dll
popd
@echo on
