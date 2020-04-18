# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import ctypes
import ctypes.util
import sys


_CACHED_CMDLINE_LENGTH = None


def set_command_line(cmdline):
  """Replaces the commandline of this process as seen by ps."""

  # Get the current commandline.
  argc = ctypes.c_int()
  argv = ctypes.POINTER(ctypes.c_char_p)()
  ctypes.pythonapi.Py_GetArgcArgv(ctypes.byref(argc), ctypes.byref(argv))

  global _CACHED_CMDLINE_LENGTH
  if _CACHED_CMDLINE_LENGTH is None:
    # Each argument is terminated by a null-byte, so the length of the whole
    # thing in memory is the sum of all the argument byte-lengths, plus 1 null
    # byte for each.
    _CACHED_CMDLINE_LENGTH = sum(
        len(argv[i]) for i in xrange(0, argc.value)) + argc.value

  # Pad the cmdline string to the required length.  If it's longer than the
  # current commandline, truncate it.
  if len(cmdline) >= _CACHED_CMDLINE_LENGTH:
    new_cmdline = ctypes.c_char_p(cmdline[:_CACHED_CMDLINE_LENGTH-1] + '\0')
  else:
    new_cmdline = ctypes.c_char_p(cmdline.ljust(_CACHED_CMDLINE_LENGTH, '\0'))

  # Replace the old commandline.
  libc = ctypes.CDLL(ctypes.util.find_library('c'))
  libc.memcpy(argv.contents, new_cmdline, _CACHED_CMDLINE_LENGTH)
