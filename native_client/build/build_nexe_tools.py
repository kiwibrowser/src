#!/usr/bin/python
# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for running build commands.

A set of utilities for running build commands.
"""

import errno
import os
import subprocess
import sys
import tempfile
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import pynacl.platform

class Error(Exception):
  pass


def FixPath(path):
  # On Windows, |path| can be a long relative path: ..\..\..\..\out\Foo\bar...
  # If the full path -- os.path.join(os.getcwd(), path) -- is longer than 255
  # characters, then any operations that open or check for existence will fail.
  # We can't use os.path.abspath here, because that calls into a Windows
  # function that still has the path length limit. Instead, we'll cheat and
  # normalize the path lexically.
  path = os.path.normpath(os.path.join(os.getcwd(), path))
  if pynacl.platform.IsWindows():
    if len(path) > 255:
      raise Error('Path "%s" is too long (%d characters), and will fail.' % (
          path, len(path)))
  return path


def IsFile(path):
  return os.path.isfile(FixPath(path))


def MakeDir(outdir):
  outdir = FixPath(outdir)
  if outdir and not os.path.exists(outdir):
    # There may be a race creating this directory, so ignore failure.
    try:
      os.makedirs(outdir)
    except OSError:
      pass


def RemoveFile(path):
  os.remove(FixPath(path))


class CommandRunner(object):
  """Basic commandline runner that can run and log commands."""

  def __init__(self, options):
    self.deferred_log = []
    self.commands_are_scripts = False
    self.verbose = options.verbose

  def SetCommandsAreScripts(self, v):
    self.commands_are_scripts = v

  def Log(self, msg):
    if self.verbose:
      sys.stderr.write(str(msg) + '\n')
    else:
      self.deferred_log.append(str(msg) + '\n')

  def EmitDeferredLog(self):
    for line in self.deferred_log:
      sys.stderr.write(line)
    self.deferred_log = []

  def CleanOutput(self, out):
    if IsFile(out):
      # Since nobody can remove a file opened by somebody else on Windows,
      # we will retry removal.  After trying certain times, we gives up
      # and reraise the WindowsError.
      retry = 0
      while True:
        try:
          RemoveFile(out)
          return
        except WindowsError, inst:
          # When the errno is errno.ENOENT, we consider the output file
          # has been already removed somewhere.
          if inst.errno == errno.ENOENT:
            return
          if retry > 5:
            raise Error('FAILED to CleanOutput: %s : %s' % (out, inst))
          self.Log('WindowsError %s while removing %s retry=%d' %
                   (inst, out, retry))
        sleep_time = 2**retry
        sleep_time = sleep_time if sleep_time < 10 else 10
        time.sleep(sleep_time)
        retry += 1

  def Run(self, cmd_line, get_output=False, normalize_slashes=True,
          possibly_script=True, **kwargs):
    """Helper which runs a command line.

    Returns the error code if get_output is False.
    Returns the output if get_output is True.
    """
    if normalize_slashes:
      # Use POSIX style path on Windows for POSIX based toolchains
      # (just for arguments, not for the path to the command itself).
      # If Run() is not invoking a POSIX based toolchain there is no
      # need to do this normalization.
      cmd_line = ([cmd_line[0]] +
                  [cmd.replace('\\', '/') for cmd in cmd_line[1:]])
    # Windows has a command line length limitation of 8191 characters,
    # so store commands in a response file ("@foo") if needed.
    temp_file = None
    if len(' '.join(cmd_line)) > 8000:
      with tempfile.NamedTemporaryFile(delete=False) as temp_file:
        temp_file.write(' '.join(cmd_line[1:]))
      cmd_line = [cmd_line[0], '@' + temp_file.name]

    self.Log(' '.join(cmd_line))
    try:
      runner = subprocess.check_output if get_output else subprocess.call
      if (possibly_script and self.commands_are_scripts and
          pynacl.platform.IsWindows()):
        # Executables that are scripts and not binaries don't want to run
        # on Windows without a shell.
        result = runner(' '.join(cmd_line), shell=True, **kwargs)
      else:
        result = runner(cmd_line, **kwargs)
    except Exception as err:
      raise Error('%s\nFAILED: %s' % (' '.join(cmd_line), str(err)))
    finally:
      if temp_file is not None:
        RemoveFile(temp_file.name)
    return result
