Using printf (as described in the [Debugging Tips]
(http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/debugging.html))
is impractical on Windows because you can't effectively get standard I/O output
from the version of `sel_ldr` that's installed by default. The problem occurs
because on Windows, only the debug version of `sel_ldr` sends output to the
console. (Only the debug version is compiled with `/SUBSYSTEM:CONSOLE`.)

To install the debug version of `sel_ldr`, first build it by running
`.\scons.bat --mode=dbg-win`. Then run `.\scons.bat DBG=1 firefox_install`.
After the debug version is installed, a console window will appear for each
instance of `sel_ldr` created by the browser plugin. An option that will allow
redirecting the output from a Native Client module into a file is in
development.

_PENDING: This page should also give some tips for debugging with Visual Studio
and WinDbg._
