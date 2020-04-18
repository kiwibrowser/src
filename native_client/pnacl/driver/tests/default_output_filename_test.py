#!/usr/bin/python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

Test that the driver is able to pick a default filename if "-o" is not given.
"""

import os
import tempfile
import unittest

from driver_env import env
import driver_test_utils
import driver_tools
pnacl_driver = __import__("pnacl-driver")


class TestDefaultOutputFilename(driver_test_utils.DriverTesterCommon):

  def setUp(self):
    super(TestDefaultOutputFilename, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    env.set('SHARED', '0')
    # Driver mode flags to test.  Having no mode flag at all is expected to
    # generate an a.out file, but that is currently hard to test
    # without actually running clang, the bitcode linker, pnacl-llc, then
    # the native linker.
    self.driver_flags = ['-E', '-S', '-c']

  def test_bitcode_result(self):
    """ Test the default output filename when generating bitcode. """
    # Output filename should only take the basename, which will end up
    # dumping the output to the PWD instead of the directory of the source.
    # Thus the result for |base| should be the same as for |in_dir|.
    base = 'foo.c'
    in_dir = '/tmp/bar/foo.c'
    # Expected results for each of self.driver_flags.
    # -E is the only one that prints to stdout, via a '-'.
    expected_results = ['-', 'foo.ll', 'foo.o']
    for f, exp in zip(self.driver_flags, expected_results):
      outtype = pnacl_driver.DriverOutputTypes(f, False)
      self.assertTrue(driver_tools.DefaultOutputName(base, outtype) == exp)
      self.assertTrue(driver_tools.DefaultOutputName(in_dir, outtype) == exp)

  def test_native_result(self):
    """ Test the default output filename when generating native code. """
    base = 'foo.c'
    in_dir = '/tmp/bar/foo.c'
    expected_results = ['-', 'foo.s', 'foo.o']
    for f, exp in zip(self.driver_flags, expected_results):
      outtype = pnacl_driver.DriverOutputTypes(f, True)
      self.assertTrue(driver_tools.DefaultOutputName(base, outtype) == exp)
      self.assertTrue(driver_tools.DefaultOutputName(in_dir, outtype) == exp)
