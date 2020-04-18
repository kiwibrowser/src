#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that diagnostic flags work fine, even with --pnacl-* flags.
"""
import cStringIO
import os
import unittest

from driver_env import env
import driver_log
import driver_test_utils
import driver_tools
pnacl_driver = __import__('pnacl-driver')

# Pick a particular --pnacl-* flag to test. Don't use --pnacl-driver-verbose to
# test this, because --pnacl-driver-verbose has a side-effect that isn't cleared
# by the env.pop() of RunDriver().
pnacl_flag = '--pnacl-disable-abi-check'

def exists(substring, list_of_strings):
  for string in list_of_strings:
    if substring in string:
      return True
  return False

class TestDiagnosticFlags(unittest.TestCase):

  def setUp(self):
    super(TestDiagnosticFlags, self).setUp()

  def set_driver_rev_file(self, rev_file_name):
    p = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                     rev_file_name)
    more_overrides = {'DRIVER_REV_FILE': p}
    driver_test_utils.ApplyTestEnvOverrides(env, more_overrides)

  def check_flags(self, flags, pnacl_flag):
    """ Check that pnacl_flag doesn't get passed to clang.

    It should only be in the 'Driver invocation:' part that we print out.
    """
    capture_out = cStringIO.StringIO()
    driver_log.Log.CaptureToStream(capture_out)
    driver_tools.RunDriver('pnacl-clang', flags + [pnacl_flag])
    driver_log.Log.ResetStreams()
    out = capture_out.getvalue()
    lines = out.splitlines()
    self.assertTrue('Driver invocation:' in lines[0])
    self.assertTrue(pnacl_flag in lines[0])
    for line in lines[1:]:
      self.assertTrue(pnacl_flag not in line)
    for flag in flags:
      self.assertTrue(exists(flag, lines[1:]))

  def test_git_version(self):
    """Ensure the git hash is returned when invoking `--version`."""
    if not driver_test_utils.CanRunHost():
      return
    self.set_driver_rev_file('TEST_REV_GIT')
    self.check_flags(['--version'], pnacl_flag)
    self.assertEqual('ebba42f04c6a61f2a92e0a15ab835ef2371b6836',
                     pnacl_driver.ReadDriverRevision())

  def test_git_version_2(self):
    """Ensure the git hash is obtained when the URL doesn't end with `.git`."""
    if not driver_test_utils.CanRunHost():
      return
    self.set_driver_rev_file('TEST_REV_GIT2')
    self.check_flags(['--version'], pnacl_flag)
    self.assertEqual('587c9f0a79e679a27304980809e4f0bfd2a6c3c2',
                     pnacl_driver.ReadDriverRevision())

  def test_with_pnacl_flag(self):
    """ Test that we do not pass the made-up --pnacl-* flags along to clang.

    Previously, when we ran --version and other "diagnostic" flags, there
    was a special invocation of clang, where we did not filter out the
    --pnacl-* driver-only flags.
    """
    if not driver_test_utils.CanRunHost():
      return
    self.check_flags(['-v'], pnacl_flag)
    self.check_flags(['-v', '-E', '-xc', os.devnull], pnacl_flag)
    self.check_flags(['-print-file-name=libc'], pnacl_flag)
    self.check_flags(['-print-libgcc-file-name'], pnacl_flag)
    self.check_flags(['-print-multi-directory'], pnacl_flag)
    self.check_flags(['-print-multi-lib'], pnacl_flag)
    self.check_flags(['-print-multi-os-directory'], pnacl_flag)

if __name__ == '__main__':
  unittest.main()
