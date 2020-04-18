#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

Test the -x language mechanism (force file type).
"""

from driver_env import env
import driver_log
import driver_temps
import driver_test_utils
import driver_tools
import filetype
import pathtools

import cStringIO
import os
import re
import sys
import tempfile
import unittest


class TestForceFileType(unittest.TestCase):

  def setUp(self):
    driver_test_utils.ApplyTestEnvOverrides(env)
    # Reset some internal state.
    filetype.ClearFileTypeCaches()
    filetype.SetForcedFileType(None)

  def tearDown(self):
    # Wipe other temp files that are normally wiped by DriverExit.
    # We don't want anything to exit, so we do not call DriverExit manually.
    driver_temps.TempFiles.wipe()
    # Reset some internal state.
    filetype.ClearFileTypeCaches()
    filetype.SetForcedFileType(None)

  def test_ForceFunction(self):
    """Test the internal functions directly."""
    made_up_file1 = 'dummy1'
    filetype.SetForcedFileType('c')
    self.assertEqual(filetype.GetForcedFileType(), 'c')
    filetype.ForceFileType(made_up_file1)
    self.assertEqual(filetype.FileType(made_up_file1), 'c')
    filetype.ForceFileType(made_up_file1)

    made_up_file2 = 'dummy2'
    filetype.SetForcedFileType('cpp')
    filetype.ForceFileType(made_up_file2)
    self.assertEqual(filetype.FileType(made_up_file2), 'cpp')
    filetype.ForceFileType(made_up_file1)

  def checkXFlagOutput(self, flags, expected):
    ''' Given |flags| for pnacl-clang, check that clang outputs the |expected|.
    '''
    if not driver_test_utils.CanRunHost():
      return
    temp_out = pathtools.normalize(tempfile.NamedTemporaryFile().name)
    driver_temps.TempFiles.add(temp_out)
    driver_tools.RunDriver('pnacl-clang', flags + ['-o', temp_out])
    output = open(temp_out, 'r').read()
    for e in expected:
      self.assertTrue(re.search(e, output),
                      msg='Searching for regex %s in %s' % (e, output))

  def test_DashXC(self):
    """Test that pnacl-clang handles basic use of -xc correctly."""
    self.checkXFlagOutput(['-xc', '-E', os.devnull, '-dM'],
                          ['__STDC_VERSION__'])

  def test_DashXCPP(self):
    """Test that pnacl-clang handles basic use of -x c++ correctly."""
    self.checkXFlagOutput(['-x', 'c++', '-E', os.devnull, '-dM'],
                          ['__cplusplus'])

  def test_DriverXPositionDependence(self):
    """Test that the -x paramater is indeed position dependent."""
    # Try with a -x c right before the input
    self.checkXFlagOutput(['-xc', '-x', 'c++', '-E', '-x', 'c',
                           os.devnull, '-dM'],
                          ['__STDC_VERSION__'])
    # Now try with the -x c after the input, instead.
    self.checkXFlagOutput(['-xc', '-x', 'c++', '-E',
                           os.devnull, '-x', 'c', '-dM'],
                          ['__cplusplus'])

  def test_DriverXNONE(self):
    """Test that "-x none" clears the file type."""
    if not driver_test_utils.CanRunHost():
      return
    # Add a string stream to capture output.
    capture_out = cStringIO.StringIO()
    driver_log.Log.CaptureToStream(capture_out)

    # Major hack to capture the exit (prevent Log.Fatal from blowin up).
    backup_exit = sys.exit
    sys.exit = driver_test_utils.FakeExit
    self.assertRaises(driver_test_utils.DriverExitException,
                      driver_tools.RunDriver,
                      'pnacl-clang',
                      ['-E', '-x', 'c', '-x', 'none', os.devnull, '-dM'])
    driver_log.Log.ResetStreams()
    out = capture_out.getvalue()
    sys.exit = backup_exit
    self.assertTrue('Unrecognized file type' in out)


if __name__ == '__main__':
  unittest.main()
