@setlocal
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
cl static_initializers.cc /EHsc /I "c:\Program Files (x86)\Microsoft Visual Studio 14.0\DIA SDK\include" /link Ole32.lib OleAut32.lib dbghelp.lib
