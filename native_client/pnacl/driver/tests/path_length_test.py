#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of the pnacl driver.

This tests that we give useful errors when paths are too long, and don't
unnecessarily generate temp file names that are too long ourselves.

"""

import driver_log
from driver_env import env
import driver_test_utils
import driver_tools
import filetype

import cStringIO
import os
import shutil
import sys

class TestPathNames(driver_test_utils.DriverTesterCommon):
  def setUp(self):
    super(TestPathNames, self).setUp()
    driver_test_utils.ApplyTestEnvOverrides(env)
    self.backup_exit = sys.exit
    sys.exit = driver_test_utils.FakeExit
    self.cwd_backup = os.getcwd()
    cwd_len = len(self.cwd_backup)
    # Create a directory whose path will be exactly 235 chars long
    assert(cwd_len < 235)
    shorter_dir_len = 235 - cwd_len - 1
    self.ShorterTempDir = os.path.join(self.cwd_backup, 'a' * shorter_dir_len)
    assert(235 == len(self.ShorterTempDir),
           "ShorterTempDir isn't 235 chars: %d" % len(self.ShorterTempDir))
    os.mkdir(self.ShorterTempDir)
    # Create a directory whose path will be exactly 240 chars long
    dir_len = 240 - cwd_len - 1
    self.LongTempDir = os.path.join(self.cwd_backup, 'a' * dir_len)
    assert(240 == len(self.LongTempDir),
           "LongTempDir isn't 240 chars: %d" % len(self.LongTempDir))
    os.mkdir(self.LongTempDir)

  def tearDown(self):
    super(TestPathNames, self).tearDown()
    os.chdir(self.cwd_backup)
    sys.exit = self.backup_exit
    shutil.rmtree(self.LongTempDir)
    shutil.rmtree(self.ShorterTempDir)

  def WriteCFile(self, filename):
    with open(filename, 'w') as f:
      f.write('int main() { return 0; }')

  def AssertRaisesAndReturnOutput(self, exc, func, *args):
    capture_out = cStringIO.StringIO()
    driver_log.Log.CaptureToStream(capture_out)
    self.assertRaises(exc, func, *args)
    driver_log.Log.ResetStreams()
    return capture_out.getvalue()

  def test_PathWithSpaces(self):
    '''Test that the driver correctly handles paths containing spaces'''
    if not driver_test_utils.CanRunHost():
      return

    name = os.path.join(self.ShorterTempDir, 'a file')
    self.WriteCFile(name + '.c')
    driver_tools.RunDriver('pnacl-clang',
                           [name + '.c', '-c', '-o', name + '.o'])
    self.assertEqual('po', filetype.FileType(name + '.o'))

  def test_InputPathTooLong(self):
    '''Test that clang and ld reject input paths that are too long

     Test that compiling and linking paths shorter than 255 succeeds and paths
     longer than 255 fails.
     Operations in python (e.g. rename) on long paths don't work on Windows,
     so just run the tests on Linux/Mac (the checks are enabled in unittest
     mode)
     '''
    if not driver_test_utils.CanRunHost() or driver_tools.IsWindowsPython():
      return

    shortname = os.path.join(self.LongTempDir, 'a' * 10)
    longname = os.path.join(self.LongTempDir, 'a' * 32)

    assert len(shortname) + 2 < 255
    assert len(longname) + 2 > 255
    self.WriteCFile(shortname + '.c')

    driver_tools.RunDriver('pnacl-clang',
                           [shortname + '.c', '-c', '-o', shortname + '.o'])

    driver_tools.RunDriver('pnacl-ld',
                           [shortname + '.o'])

    os.rename(shortname + '.c', longname + '.c')
    os.rename(shortname + '.o', longname + '.o')


    output = self.AssertRaisesAndReturnOutput(
        driver_test_utils.DriverExitException,
        driver_tools.RunDriver,
        'pnacl-clang',
        [longname + '.c', '-c', '-o', longname + '.o'])

    self.assertIn('too long', output)

    output = self.AssertRaisesAndReturnOutput(
        driver_test_utils.DriverExitException,
        driver_tools.RunDriver,
        'pnacl-ld',
        [longname + '.o'])

    self.assertIn('too long', output)

  def test_ExpandedPathTooLong(self):
    '''Test that the expanded path is checked with a short relative path'''
    if not driver_test_utils.CanRunHost() or driver_tools.IsWindowsPython():
      return

    os.chdir(self.LongTempDir)

    shortname = 'a' * 10
    longname = 'a' * 32

    # Now we are in a state where the file can be referred to by a relative
    # path or a normalized absolute path. For 'shortname', both are short
    # enough
    assert len(os.path.join(self.LongTempDir, shortname)) + 2 < 255
    assert len(os.path.join(self.LongTempDir, longname)) + 2 > 255
    self.WriteCFile(shortname + '.c')

    # Test that using a relative almost-too-long path works
    driver_tools.RunDriver('pnacl-clang',
                           [shortname + '.c', '-c', '-o', shortname + '.o'])

    driver_tools.RunDriver('pnacl-ld',
                           [shortname + '.o'])

    # This name has a short-enough relative path and a short-enough normalized
    # final path, but the intermediate concatenation of pwd + rel path is too
    # long
    name_with_traversals = os.path.join('..',
                                        os.path.basename(self.LongTempDir),
                                        shortname)

    output = self.AssertRaisesAndReturnOutput(
        driver_test_utils.DriverExitException,
        driver_tools.RunDriver,
        'pnacl-clang',
        [name_with_traversals + '.c', '-c', '-o', shortname + '.o'])
    self.assertIn('expanded', output)

    # The previous test only gives a long input name. Also test that the output
    # name is checked.
    output = self.AssertRaisesAndReturnOutput(
        driver_test_utils.DriverExitException,
        driver_tools.RunDriver,
        'pnacl-clang',
        [shortname + '.c', '-c', '-o', name_with_traversals + '.o'])
    self.assertIn('expanded', output)


  def test_TempFileNotTooLong(self):
    '''Test that temp files with too-long names are not generated'''
    if not driver_test_utils.CanRunHost() or driver_tools.IsWindowsPython():
      return

    # This name is chosen such that the .c file has the maximum length. pnacl-ld
    # should not generate any temp names longer than that.
    shortname =  os.path.join(self.LongTempDir, 'a' * 12)

    self.WriteCFile(shortname + '.c')
    driver_tools.RunDriver('pnacl-clang',
                           [shortname + '.c', '-c', '-o', shortname + '.o'])

    driver_tools.RunDriver('pnacl-ld',
                           [shortname + '.o', '-o', shortname])

    # If it's impossible to generate a temp file short enough using our scheme
    # (i.e. the directory name is so long that 8 chars will be over the limit),
    # make sure we still fail the right way.
    longerdir = os.path.join(self.LongTempDir, 'a' * 8)
    os.mkdir(longerdir)
    longname = os.path.join(longerdir, 'a' * 3)
    os.rename(shortname + '.o', longname + '.o')

    output = self.AssertRaisesAndReturnOutput(
        driver_test_utils.DriverExitException,
        driver_tools.RunDriver,
        'pnacl-ld',
        [longname + '.o', '-o', longname])

    self.assertIn('.pexe is too long', output)
