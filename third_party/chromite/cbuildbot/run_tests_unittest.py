# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the run_tests module."""

from __future__ import print_function

import mock
import os

from chromite.cbuildbot import run_tests
from chromite.lib import cros_test_lib
from chromite.lib import osutils


class RunTestsTest(cros_test_lib.MockTestCase):
  """Tests for the RunTests() func"""

  def testDryrun(self):
    """Verify dryrun doesn't do anything crazy."""
    self.PatchObject(run_tests, 'RunTest', side_effect=Exception('do not run'))
    ret = run_tests.RunTests(['/bin/false'], dryrun=True)
    self.assertTrue(ret)


class FindTestsTest(cros_test_lib.TempDirTestCase):
  """Tests for the FindTests() func"""

  def testNames(self):
    """We only look for *_unittest."""
    for f in ('foo', 'foo_unittests', 'bar_unittest', 'cow_unittest'):
      osutils.Touch(os.path.join(self.tempdir, f))
    found = run_tests.FindTests(search_paths=(self.tempdir,))
    self.assertEqual(sorted(found), ['./bar_unittest', './cow_unittest'])

  def testSubdirs(self):
    """We should recurse into subdirs."""
    for f in ('bar_unittest', 'somw/dir/a/cow_unittest'):
      osutils.Touch(os.path.join(self.tempdir, f), makedirs=True)
    found = run_tests.FindTests(search_paths=(self.tempdir,))
    self.assertEqual(sorted(found),
                     ['./bar_unittest', 'somw/dir/a/cow_unittest'])

  def testIgnores(self):
    """Verify we skip ignored dirs."""
    for f in ('foo', 'bar_unittest'):
      osutils.Touch(os.path.join(self.tempdir, f))
    # Make sure it works first.
    found = run_tests.FindTests(search_paths=(self.tempdir,))
    self.assertEqual(sorted(found), ['./bar_unittest'])
    # Mark the dir ignored.
    osutils.Touch(os.path.join(self.tempdir, '.testignore'))
    # Make sure we ignore it.
    found = run_tests.FindTests(search_paths=(self.tempdir,))
    self.assertEqual(list(found), [])


class SortTest(cros_test_lib.TempDirTestCase):
  """Tests for the SortTests() func"""

  def SortTests(self, tests, **kwargs):
    """Helper to set cache file to a local temp one"""
    kwargs['timing_cache_file'] = os.path.join(self.tempdir, 'cache.json')
    return run_tests.SortTests(tests, **kwargs)

  def testEmpty(self):
    """Verify handling of empty test lists"""
    self.SortTests([])
    self.SortTests([], jobs=100)

  def testSmallSet(self):
    """Do nothing when number of tests is lower than number of jobs."""
    tests = ['small', 'test', 'list', 'is', 'ignored']
    ret = self.SortTests(tests, jobs=100)
    self.assertEqual(tests, ret)

  def testOddSet(self):
    """Verify we can sort odd number of tests."""
    tests = ['1', '2', '3']
    ret = self.SortTests(tests, jobs=1)
    self.assertEqual(set(tests), set(ret))

  def testEvenSet(self):
    """Verify we can sort even number of tests."""
    tests = ['1', '2', '3', '4']
    ret = self.SortTests(tests, jobs=1)
    self.assertEqual(set(tests), set(ret))


class MainTest(cros_test_lib.MockOutputTestCase):
  """Tests for the main() func"""

  def setUp(self):
    self.PatchObject(run_tests, '_ReExecuteIfNeeded')
    self.PatchObject(run_tests, 'ChrootAvailable', return_value=True)

  def testList(self):
    """Verify --list works"""
    self.PatchObject(run_tests, 'RunTests', side_effect=Exception('do not run'))
    with self.OutputCapturer() as output:
      run_tests.main(['--list'])
      # Verify some reasonable number of lines showed up.
      self.assertGreater(len(output.GetStdoutLines()), 90)

  def testMisc(self):
    """Verify basic flags get passed down correctly"""
    m = self.PatchObject(run_tests, 'RunTests', return_value=True)
    run_tests.main(['--network'])
    m.assert_called_with(mock.ANY, jobs=mock.ANY, chroot_available=mock.ANY,
                         network=True, dryrun=False, failfast=False)
    run_tests.main(['--dry-run'])
    m.assert_called_with(mock.ANY, jobs=mock.ANY, chroot_available=mock.ANY,
                         network=False, dryrun=True, failfast=False)
    run_tests.main(['--jobs', '1000'])
    m.assert_called_with(mock.ANY, jobs=1000, chroot_available=mock.ANY,
                         network=False, dryrun=False, failfast=False)
    run_tests.main(['--failfast'])
    m.assert_called_with(mock.ANY, jobs=mock.ANY, chroot_available=mock.ANY,
                         network=False, dryrun=False, failfast=True)

  def testUnknownArg(self):
    """Verify we kick out unknown args"""
    self.PatchObject(run_tests, 'RunTests', side_effect=Exception('do not run'))
    bad_arg = '--foasdf'
    exit_code = None
    # Only run the main code w/the capturer enabled so we don't swallow
    # general test output.
    with self.OutputCapturer():
      try:
        run_tests.main([bad_arg])
      except SystemExit as e:
        exit_code = e.code
    self.assertNotEqual(exit_code, 0,
                        msg='run_tests wrongly accepted %s' % bad_arg)

  def testQuick(self):
    """Verify --quick filters out slow tests"""
    self.PatchObject(run_tests, 'RunTests', return_value=True)
    # Pick a test that is in SLOW_TESTS but not in SPECIAL_TESTS.
    slow_test = 'lib/patch_unittest'
    self.assertIn(slow_test, run_tests.SLOW_TESTS)
    self.assertNotIn(slow_test, run_tests.SPECIAL_TESTS)
    run_tests.main(['--quick'])
    self.assertIn(slow_test, run_tests.SPECIAL_TESTS)

  def testSpecificTests(self):
    """Verify user specified tests are run."""
    m = self.PatchObject(run_tests, 'RunTests', return_value=True)
    tests = ['./some/foo_unittest', './bar_unittest']
    run_tests.main(tests)
    m.assert_called_with(tests, jobs=mock.ANY, chroot_available=mock.ANY,
                         network=mock.ANY, dryrun=mock.ANY, failfast=mock.ANY)
