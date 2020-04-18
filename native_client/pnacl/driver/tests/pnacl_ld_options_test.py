#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that pnacl-ld manages linking and LLVM opt flags correctly.
"""

import cStringIO
import re
import unittest

from driver_env import env
import driver_log
import driver_test_utils
import driver_tools
import filetype

class TestLDOptions(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestLDOptions, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)

  def getBitcode(self, finalized=True):
    # Even --dry-run requires a file to exist, so make a fake bitcode file.
    # It even cares that the file is really bitcode.
    with self.getTemp(suffix='.ll', close=False) as t:
      with self.getTemp(suffix='.o') as o:
        t.write('''
define i32 @_start() {
  ret i32 0
}
''')
        t.close()
        driver_tools.RunDriver('pnacl-as', [t.name, '-o', o.name])
        return o


  def checkLDOptNumRuns(self, bitcode, flags, num_runs):
    """ Given a |bitcode| file the |arch| and additional pnacl-ld |flags|,
    check that opt will only be run |num_runs| times. """
    # TODO(jvoung): Get rid of INHERITED_DRIVER_ARGS, which leaks across runs.
    env.set('INHERITED_DRIVER_ARGS', '')
    temp_output = self.getTemp()
    capture_out = cStringIO.StringIO()
    driver_log.Log.CaptureToStream(capture_out)
    driver_tools.RunDriver('pnacl-ld',
                           ['--pnacl-driver-verbose',
                            '--dry-run',
                            bitcode.name,
                            '-o', temp_output.name] + flags)
    driver_log.Log.ResetStreams()
    out = capture_out.getvalue()
    split_out = out.splitlines()
    count = 0
    for line in split_out:
      if re.search('Running: .*opt( |\.exe)', line):
        count += 1
    self.assertEqual(num_runs, count)

  def test_num_opt_runs(self):
    """ Test that pnacl-ld will merge or not merge runs of LLVM's opt tool"""
    if driver_test_utils.CanRunHost():
      bitcode = self.getBitcode()
      # Test O2, static. Opt should run all passes in one go.
      self.checkLDOptNumRuns(
          bitcode,
          ['-O2', '-static'],
          1)
      # Test with separate runs per pass.
      self.checkLDOptNumRuns(
          bitcode,
          ['-O2', '-static', '--pnacl-run-passes-separately'],
          3)
      # Same with strip-all.
      self.checkLDOptNumRuns(
          bitcode,
          ['-O2', '-static', '--strip-all'],
          1)
      self.checkLDOptNumRuns(
          bitcode,
          ['-O2', '-static', '--strip-all', '--pnacl-run-passes-separately'],
          4)

  def test_finalize(self):
    """ Test that pnacl-ld will finalize the pexe when requested"""
    if not driver_test_utils.CanRunHost():
      return
    bitcode = self.getBitcode()
    with self.getTemp(suffix='.pexe') as temp_output:
      driver_tools.RunDriver(
        'pnacl-ld', ['--finalize', bitcode.name, '-o', temp_output.name])
      self.assertFalse(filetype.IsLLVMBitcode(temp_output.name))
      self.assertTrue(filetype.IsPNaClBitcode(temp_output.name))


if __name__ == '__main__':
  unittest.main()
