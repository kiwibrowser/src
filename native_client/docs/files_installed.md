When you install the Native Client plug-in (`./scons firefox_install`), files
are added to platform-specific locations on your computer. This page lists those
files and their locations.

On Linux, the following files are copied into `$(HOME)/.mozilla/plugins`:

*   `sel_ldr`
*   `sel_ldr_bin`
*   `libnpGoogleNaClPlugin.so`
*   `libSDL-1.2.so.0`

On Mac, one or two directories are installed:

*   `$(HOME)/Library/Internet Plug-Ins/npGoogleNaClPlugin.bundle`
*   `$(HOME)/Library/Frameworks/SDL.framework` - _skipped if SDL is already
    installed in `/Library/Frameworks`_

On Windows, the following files are copied into `c:\Program Files\Mozilla
Firefox\plugins\`:

*   `sel_ldr.exe`
*   `SDL.dll`
*   `npGoogleNaClPlugin.dll`
