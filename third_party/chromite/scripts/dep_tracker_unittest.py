# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for dep_tracker.py."""

from __future__ import print_function

import os

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib import unittest_lib
from chromite.scripts import dep_tracker

# Allow access private members for testing:
# pylint: disable=W0212


class MainTest(cros_test_lib.OutputTestCase):
  """Tests for the main() function."""

  def testHelp(self):
    """Test that --help is functioning."""
    argv = ['--help']

    with self.OutputCapturer() as output:
      # Running with --help should exit with code==0.
      self.AssertFuncSystemExitZero(dep_tracker.main, argv)

    # Verify that a message beginning with "usage: " was printed.
    stdout = output.GetStdout()
    self.assertTrue(stdout.startswith('usage: '))


class DepTrackerTest(cros_test_lib.TempDirTestCase):
  """Tests for the DepTracker() class."""

  def testSimpleDep(self):
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libabc.so'),
                          ['func_a', 'func_b', 'func_c'])
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'abc_main'),
                          undefined_symbols=['func_b'],
                          used_libs=['abc'],
                          executable=True)
    dt = dep_tracker.DepTracker(self.tempdir)
    dt.Init()
    dt.ComputeELFFileDeps()

    self.assertEquals(sorted(dt._files.keys()), ['abc_main', 'libabc.so'])

  def testFiletypeSet(self):
    """Tests that the 'ftype' member is set for ELF files first."""
    unittest_lib.BuildELF(os.path.join(self.tempdir, 'libabc.so'),
                          ['func_a', 'func_b', 'func_c'])
    osutils.WriteFile(os.path.join(self.tempdir, 'pyscript'),
                      "#!/usr/bin/python\nimport sys\nsys.exit(42)\n")
    dt = dep_tracker.DepTracker(self.tempdir)
    dt.Init()

    # ComputeELFFileDeps() should compute the file type of ELF files so we
    # don't need to parse them again.
    dt.ComputeELFFileDeps()
    self.assertTrue('ftype' in dt._files['libabc.so'])
    self.assertFalse('ftype' in dt._files['pyscript'])

    # ComputeFileTypes() shold compute the file type of every file.
    dt.ComputeFileTypes()
    self.assertTrue('ftype' in dt._files['libabc.so'])
    self.assertTrue('ftype' in dt._files['pyscript'])
