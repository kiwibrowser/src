#!/usr/bin/env python
# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
from third_party import colorama

IS_TTY = None
OUT_TYPE = 'unknown'

def init():
  # should_wrap instructs colorama to wrap stdout/stderr with an ASNI colorcode
  # interpreter that converts them to SetConsoleTextAttribute calls. This only
  # should be True in cases where we're connected to cmd.exe's console. Setting
  # this to True on non-windows systems has no effect.
  should_wrap = False
  global IS_TTY, OUT_TYPE
  IS_TTY = sys.stdout.isatty()
  if IS_TTY:
    # Yay! We detected a console in the normal way. It doesn't really matter
    # if it's windows or not, we win.
    OUT_TYPE = 'console'
    should_wrap = True
  elif sys.platform.startswith('win'):
    # assume this is some sort of file
    OUT_TYPE = 'file (win)'

    import msvcrt
    import ctypes
    h = msvcrt.get_osfhandle(sys.stdout.fileno())
    # h is the win32 HANDLE for stdout.
    ftype = ctypes.windll.kernel32.GetFileType(h)
    if ftype == 2: # FILE_TYPE_CHAR
      # This is a normal cmd console, but we'll only get here if we're running
      # inside a `git command` which is actually git->bash->command. Not sure
      # why isatty doesn't detect this case.
      OUT_TYPE = 'console (cmd via msys)'
      IS_TTY = True
      should_wrap = True
    elif ftype == 3: # FILE_TYPE_PIPE
      OUT_TYPE = 'pipe (win)'
      # This is some kind of pipe on windows. This could either be a real pipe
      # or this could be msys using a pipe to emulate a pty. We use the same
      # algorithm that msys-git uses to determine if it's connected to a pty or
      # not.

      # This function and the structures are defined in the MSDN documentation
      # using the same names.
      def NT_SUCCESS(status):
        # The first two bits of status are the severity. The success
        # severities are 0 and 1, and the !success severities are 2 and 3.
        # Therefore since ctypes interprets the default restype of the call
        # to be an 'C int' (which is guaranteed to be signed 32 bits), All
        # success codes are positive, and all !success codes are negative.
        return status >= 0

      class UNICODE_STRING(ctypes.Structure):
        _fields_ = [('Length', ctypes.c_ushort),
                    ('MaximumLength', ctypes.c_ushort),
                    ('Buffer', ctypes.c_wchar_p)]

      class OBJECT_NAME_INFORMATION(ctypes.Structure):
        _fields_ = [('Name', UNICODE_STRING),
                    ('NameBuffer', ctypes.c_wchar_p)]

      buf = ctypes.create_string_buffer('\0', 1024)
      # Ask NT what the name of the object our stdout HANDLE is. It would be
      # possible to use GetFileInformationByHandleEx, but it's only available
      # on Vista+. If you're reading this in 2017 or later, feel free to
      # refactor this out.
      #
      # The '1' here is ObjectNameInformation
      if NT_SUCCESS(ctypes.windll.ntdll.NtQueryObject(h, 1, buf, len(buf)-2,
                    None)):
        out = OBJECT_NAME_INFORMATION.from_buffer(buf)
        name = out.Name.Buffer.split('\\')[-1]
        IS_TTY = name.startswith('msys-') and '-pty' in name
        if IS_TTY:
          OUT_TYPE = 'bash (msys)'
    else:
      # A normal file, or an unknown file type.
      pass
  else:
    # This is non-windows, so we trust isatty.
    OUT_TYPE = 'pipe or file'

  colorama.init(wrap=should_wrap)

if __name__ == '__main__':
  init()
  print 'IS_TTY:', IS_TTY
  print 'OUT_TYPE:', OUT_TYPE
