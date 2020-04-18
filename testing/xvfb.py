#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs tests with Xvfb and Openbox on Linux and normally on other platforms."""

import os
import os.path
import platform
import signal
import subprocess
import sys
import threading

import test_env


def _kill(proc, send_signal):
  """Kills |proc| and ignores exceptions thrown for non-existent processes."""
  try:
    os.kill(proc.pid, send_signal)
  except OSError:
    pass


def kill(proc, timeout_in_seconds=10):
  """Tries to kill |proc| gracefully with a timeout for each signal."""
  if not proc or not proc.pid:
    return

  _kill(proc, signal.SIGTERM)
  thread = threading.Thread(target=proc.wait)
  thread.start()

  thread.join(timeout_in_seconds)
  if thread.is_alive():
    print >> sys.stderr, 'Xvfb running after SIGTERM, trying SIGKILL.'
    _kill(proc, signal.SIGKILL)

  thread.join(timeout_in_seconds)
  if thread.is_alive():
    print >> sys.stderr, 'Xvfb running after SIGTERM and SIGKILL; good luck!'


def run_executable(cmd, env, stdoutfile=None):
  """Runs an executable within Xvfb on Linux or normally on other platforms.

  If |stdoutfile| is provided, symbolization via script is disabled and stdout
  is written to this file as well as to stdout.

  Returns the exit code of the specified commandline, or 1 on failure.
  """

  # It might seem counterintuitive to support a --no-xvfb flag in a script
  # whose only job is to start xvfb, but doing so allows us to consolidate
  # the logic in the layers of buildbot scripts so that we *always* use
  # xvfb by default and don't have to worry about the distinction, it
  # can remain solely under the control of the test invocation itself.
  use_xvfb = True
  if '--no-xvfb' in cmd:
    use_xvfb = False
    cmd.remove('--no-xvfb')

  if sys.platform == 'linux2' and use_xvfb:
    if env.get('_CHROMIUM_INSIDE_XVFB') == '1':
      openbox_proc = None
      xcompmgr_proc = None
      try:
        # Some ChromeOS tests need a window manager.
        openbox_proc = subprocess.Popen('openbox', stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, env=env)

        # Some tests need a compositing wm to make use of transparent visuals.
        xcompmgr_proc = subprocess.Popen('xcompmgr', stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, env=env)

        return test_env.run_executable(cmd, env, stdoutfile)
      except OSError as e:
        print >> sys.stderr, 'Failed to start Xvfb or Openbox: %s' % str(e)
        return 1
      finally:
        kill(openbox_proc)
        kill(xcompmgr_proc)
    else:
      env['_CHROMIUM_INSIDE_XVFB'] = '1'
      if stdoutfile:
        env['_XVFB_EXECUTABLE_STDOUTFILE'] = stdoutfile
      xvfb_script = __file__
      if xvfb_script.endswith('.pyc'):
        xvfb_script = xvfb_script[:-1]
      return subprocess.call(['xvfb-run', '-a', "--server-args=-screen 0 "
                              "1280x800x24 -ac -nolisten tcp -dpi 96 "
                              "+extension RANDR",
                              xvfb_script] + cmd, env=env)
  else:
    return test_env.run_executable(cmd, env, stdoutfile)


def main():
  USAGE = 'Usage: xvfb.py [command [--no-xvfb] args...]'
  if len(sys.argv) < 2:
    print >> sys.stderr, USAGE
    return 2

  # If the user still thinks the first argument is the execution directory then
  # print a friendly error message and quit.
  if os.path.isdir(sys.argv[1]):
    print >> sys.stderr, (
        'Invalid command: \"%s\" is a directory' % sys.argv[1])
    print >> sys.stderr, USAGE
    return 3

  stdoutfile = os.environ.get('_XVFB_EXECUTABLE_STDOUTFILE')
  if stdoutfile:
    del os.environ['_XVFB_EXECUTABLE_STDOUTFILE']
  return run_executable(sys.argv[1:], os.environ.copy(), stdoutfile)


if __name__ == "__main__":
  sys.exit(main())
