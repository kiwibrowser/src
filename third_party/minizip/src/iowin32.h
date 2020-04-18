/* iowin32.h -- IO base function header for compress/uncompress .zip
   Version 1.2.0, September 16th, 2017
   part of the MiniZip project

   Copyright (C) 2012-2017 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip
   Copyright (C) 2009-2010 Mathias Svensson
     Modifications for Zip64 support
     http://result42.com
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#ifndef _IOWIN32_H
#define _IOWIN32_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void fill_win32_filefunc(zlib_filefunc_def *pzlib_filefunc_def);
void fill_win32_filefunc64(zlib_filefunc64_def *pzlib_filefunc_def);
void fill_win32_filefunc64A(zlib_filefunc64_def *pzlib_filefunc_def);
void fill_win32_filefunc64W(zlib_filefunc64_def *pzlib_filefunc_def);

#ifdef __cplusplus
}
#endif

#endif
