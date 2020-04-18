#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from os.path import dirname
import platform
import sys
import tempfile
import unittest

import driver_env
import driver_log
import driver_temps
import driver_tools

def CanRunHost():
  # Some of the test+tools require running the host binaries, but that
  # does not work on some bots (e.g., the ARM bots).
  if platform.machine().startswith('arm'):
    return False
  return True

def SetupNaClDir(env):
  test_dir = os.path.abspath(dirname(__file__))
  nacl_dir = dirname(dirname(dirname(test_dir)))
  env.set('BASE_NACL', nacl_dir)

def SetupToolchainDir(env):
  test_dir = os.path.abspath(dirname(__file__))
  nacl_dir = dirname(dirname(dirname(test_dir)))
  os_name = driver_tools.GetOSName()

  toolchain_dir = os.path.join(nacl_dir, 'toolchain', '%s_x86' % os_name,
                               'pnacl_newlib_raw')
  env.set('BASE_TOOLCHAIN', toolchain_dir)

def SetupHostDir(env):
  # Some of the tools end up running one of the host binaries. Find the host
  # dir on the test system and inject it into the search path using the
  # implementation of -B
  test_dir = os.path.abspath(dirname(__file__))
  nacl_dir = dirname(dirname(dirname(test_dir)))

  os_shortname = driver_tools.GetOSName()
  host_dir = os.path.join(nacl_dir, 'toolchain',
                          '%s_x86' % os_shortname,
                          'pnacl_newlib_raw')
  driver_tools.AddHostBinarySearchPath(host_dir)

# A collection of override methods that mock driver_env.Environment.

# One thing is we prevent having to read a driver.conf file,
# so instead we have a base group of variables set for testing.
def TestEnvReset(self, more_overrides={}):
  # Call to "super" class method (assumed to be driver_env.Environment).
  # TODO(jvoung): We may want a different way of overriding things.
  driver_env.Environment.reset(self)
  # The overrides.
  self.set('PNACL_RUNNING_UNITTESTS', '1')
  SetupNaClDir(self)
  SetupToolchainDir(self)
  SetupHostDir(self)
  for k, v in more_overrides.iteritems():
    self.set(k, v)

def ApplyTestEnvOverrides(env, more_overrides={}):
  """Register all the override methods and reset the env to a testable state.
  """
  resetter = lambda self: TestEnvReset(self, more_overrides)
  driver_env.override_env('reset', resetter)
  env.reset()

# Utils to prevent driver exit.

class DriverExitException(Exception):
  pass

def FakeExit(i):
  raise DriverExitException('Stubbed out DriverExit!')

# Basic argument parsing.

def GetPlatformToTest():
  for arg in sys.argv:
    if arg.startswith('--platform='):
      return arg.split('=')[1]
  raise Exception('Unknown platform: "%s"' % arg)

# We would like to be able to use a temp file whether it is open or closed.
# However File's __enter__ method requires it to be open. So we override it
# to just return the fd regardless.
class TempWrapper(object):
  def __init__(self, fd, close=True):
    self.fd_ = fd
    if close:
      fd.close()
  def __enter__(self):
    return self.fd_
  def __exit__(self, exc_type, exc_value, traceback):
    return self.fd_.__exit__(exc_type, exc_value, traceback)
  def __getattr__(self, name):
    return getattr(self.fd_, name)

class DriverTesterCommon(unittest.TestCase):
  def setUp(self):
    super(DriverTesterCommon, self).setUp()
    self._tempfiles = []

  def tearDown(self):
    for t in self._tempfiles:
      if not t.closed:
        t.close()
      os.remove(t.name)
    driver_temps.TempFiles.wipe()
    super(DriverTesterCommon, self).tearDown()

  def getTemp(self, close=True, **kwargs):
    """ Get a temporary named file object.
    """
    # Set delete=False, so that we can close the files and
    # re-open them.  Windows sometimes does not allow you to
    # re-open an already opened temp file.
    t = tempfile.NamedTemporaryFile(delete=False, **kwargs)
    self._tempfiles.append(t)
    return TempWrapper(t, close=close)
