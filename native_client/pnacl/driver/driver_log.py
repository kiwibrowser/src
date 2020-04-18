#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import pathtools

#TODO: DriverOpen/Close are in here because this is a low level lib without
# any dependencies. Maybe they should go in another low level lib, or maybe
# this should become a low level lib that's more general than just log

def DriverOpen(filename, mode, fail_ok = False):
  try:
    fp = open(pathtools.tosys(filename), mode)
  except Exception:
    if not fail_ok:
      Log.Fatal("%s: Unable to open file", pathtools.touser(filename))
      DriverExit(1)
    else:
      return None
  return fp

def DriverClose(fp):
  fp.close()

def FixArch(arch):
  arch = arch.lower()
  archfix = { 'x86'   : 'X8632',
              'x86-32': 'X8632',
              'x86_32': 'X8632',
              'x8632' : 'X8632',
              'i686'  : 'X8632',
              'ia32'  : 'X8632',
              '386'   : 'X8632',
              '686'   : 'X8632',

              'amd64' : 'X8664',
              'x64'   : 'X8664',
              'x86_64': 'X8664',
              'x86-64': 'X8664',
              'x8664' : 'X8664',

              'arm'   : 'ARM',
              'armv7' : 'ARM',
              'armv7a': 'ARM',

              'mips32': 'MIPS32',
              'mips'  : 'MIPS32',
              'mipsel': 'MIPS32',

              'x86-32-linux': 'X8632_LINUX',
              'x86-64-linux': 'X8664_LINUX',
              'x86-32-mac': 'X8632_MAC',
              'x86-32-nonsfi': 'X8632_NONSFI',
              'arm-nonsfi': 'ARM_NONSFI',
              }
  if arch not in archfix:
    Log.Fatal('Unrecognized arch "%s"!', arch)
  return archfix[arch]

driver_exit_funcs = []
def AtDriverExit(func):
  driver_exit_funcs.append(func)

# Completely terminate the driver and all module layers.
#
# Inside a single driver module, this should only be used for
# abnormal/unexpected exit. For normal exit-on-success, return 0
# from main().
#
# The only place this may be used with retcode 0 is in the final
# exit (in loader.py).
def DriverExit(retcode, is_final_exit=False):
  assert(is_final_exit or retcode != 0)
  for func in driver_exit_funcs:
    func()
  sys.exit(retcode)

######################################################################
#
# Logging
#
######################################################################

class LogManager(object):
  def __init__(self):
    self._error_out = [sys.stderr]
    self._debug_out = []
    self._script_name = ''
    self._capture_output = False
    self._orig_err = self._error_out
    self._orig_debug = self._debug_out

  def CaptureToStream(self, s):
    # Provide a way to capture output for testing.
    self._capture_output = True
    self._orig_err = self._error_out
    self._orig_debug = self._debug_out
    self._error_out = [s]
    self._debug_out = [s]

  def ResetStreams(self):
    # Reset the streams back to normal.
    self._error_out = self._orig_err
    self._debug_out = self._orig_debug
    self._capture_output = False

  def IncreaseVerbosity(self):
    # Ignore if capturing output for test.
    if self._capture_output:
      return
    if not sys.stderr in self._debug_out:
      self._debug_out.append(sys.stderr)

  def SetScriptName(self, script_name):
    self._script_name = script_name

  def Info(self, m, *args):
    self.Print(self._debug_out, m, *args)

  def Error(self, m, *args):
    self.Print(self._error_out, m, *args)

  def FatalWithResult(self, ret, msg, *args):
    if self._script_name:
      msg = '%s: %s' % (self._script_name, msg)
    self.Print(self._error_out, msg, *args)
    DriverExit(ret)

  def Warning(self, m, *args):
    m = 'Warning: ' + m
    self.Print(self._error_out, m, *args)

  def Fatal(self, m, *args):
    # Note, using keyword args and arg lists while trying to keep
    # the m and *args parameters next to each other does not work
    self.FatalWithResult(-1, m, *args)

  def Print(self, outs, m, *args):
    # NOTE: m may contain '%' if no args are given
    if args:
      m = m % args
    for o in outs:
      print >> o, m


def StringifyCommand(cmd):
  """ Return a string for reproducing the command "cmd". """
  ret = ''
  grouping = 0
  for a in cmd:
    if grouping == 0 and len(ret) > 0:
      ret += " \\\n    "
    elif grouping > 0:
      ret += " "
    if grouping == 0:
      grouping = 1
      if a.startswith('-') and len(a) == 2:
        grouping = 2
    ret += a
    grouping -= 1
  return ret

Log = LogManager()
