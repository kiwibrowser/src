# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittest for cros_test_lib (tests for tests? Who'd a thunk it)."""

from __future__ import print_function

import mock
import os
import sys
import time
import unittest

from chromite.lib import cros_test_lib
from chromite.lib import cros_build_lib
from chromite.lib import osutils
from chromite.lib import partial_mock
from chromite.lib import timeout_util


# pylint: disable=W0212,W0233

# Convenience alias
Dir = cros_test_lib.Directory


class CrosTestCaseTest(cros_test_lib.TestCase):
  """Test the cros_test_lib.TestCase."""

  def testAssertStartsWith(self):
    s = "abcdef"
    prefix = "abc"
    self.assertStartsWith(s, prefix)
    prefix = "def"
    self.assertRaises(AssertionError, self.assertStartsWith, s, prefix)

  def testAssertEndsWith(self):
    s = "abcdef"
    suffix = "abc"
    self.assertRaises(AssertionError, self.assertEndsWith, s, suffix)
    suffix = "def"
    self.assertEndsWith(s, suffix)


class TruthTableTest(cros_test_lib.TestCase):
  """Test TruthTable functionality."""

  def _TestTableSanity(self, tt, lines):
    """Run the given truth table through basic sanity checks.

    Args:
      tt: A TruthTable object.
      lines: The expect input lines, in order (list of tuples).
    """
    # Check that more than one iterable can be used at once.
    iter1 = iter(tt)
    iter2 = iter(tt)
    self.assertEquals(lines[0], iter1.next())
    self.assertEquals(lines[0], iter2.next())
    self.assertEquals(lines[1], iter2.next())

    # Check that iteration again works again.
    for ix, line in enumerate(tt):
      self.assertEquals(lines[ix], line)

    # Check direct access of input lines.
    for i in xrange(len(tt)):
      self.assertEquals(lines[i], tt.GetInputs(i))

    # Check assertions on bad input to GetInputs.
    self.assertRaises(ValueError, tt.GetInputs, -1)
    self.assertRaises(ValueError, tt.GetInputs, len(tt))

  def testTwoDimensions(self):
    """Test TruthTable behavior for two boolean inputs."""
    tt = cros_test_lib.TruthTable(inputs=[(True, True), (True, False)])
    self.assertEquals(len(tt), pow(2, 2))

    # Check truth table output.
    self.assertFalse(tt.GetOutput((False, False)))
    self.assertFalse(tt.GetOutput((False, True)))
    self.assertTrue(tt.GetOutput((True, False)))
    self.assertTrue(tt.GetOutput((True, True)))

    # Check assertions on bad input to GetOutput.
    self.assertRaises(TypeError, tt.GetOutput, True)
    self.assertRaises(ValueError, tt.GetOutput, (True, True, True))

    # Check iteration over input lines.
    lines = list(tt)
    self.assertEquals((False, False), lines[0])
    self.assertEquals((False, True), lines[1])
    self.assertEquals((True, False), lines[2])
    self.assertEquals((True, True), lines[3])

    self._TestTableSanity(tt, lines)

  def testFourDimensions(self):
    """Test TruthTable behavior for four boolean inputs."""
    false1 = (True, True, True, False)
    false2 = (True, False, True, False)
    true1 = (False, True, False, True)
    true2 = (True, True, False, False)
    tt = cros_test_lib.TruthTable(inputs=(false1, false2), input_result=False)
    self.assertEquals(len(tt), pow(2, 4))

    # Check truth table output.
    self.assertFalse(tt.GetOutput(false1))
    self.assertFalse(tt.GetOutput(false2))
    self.assertTrue(tt.GetOutput(true1))
    self.assertTrue(tt.GetOutput(true2))

    # Check assertions on bad input to GetOutput.
    self.assertRaises(TypeError, tt.GetOutput, True)
    self.assertRaises(ValueError, tt.GetOutput, (True, True, True))

    # Check iteration over input lines.
    lines = list(tt)
    self.assertEquals((False, False, False, False), lines[0])
    self.assertEquals((False, False, False, True), lines[1])
    self.assertEquals((False, True, True, True), lines[7])
    self.assertEquals((True, True, True, True), lines[15])

    self._TestTableSanity(tt, lines)


class VerifyTarballTest(cros_test_lib.MockTempDirTestCase):
  """Test tarball verification functionality."""

  TARBALL = 'fake_tarball'

  def setUp(self):
    self.rc_mock = self.StartPatcher(cros_test_lib.RunCommandMock())

  def _MockTarList(self, files):
    """Mock out tarball content list call.

    Args:
      files: A list of contents to return.
    """
    self.rc_mock.AddCmdResult(
        partial_mock.ListRegex('tar -tf'), output='\n'.join(files))

  def testNormPath(self):
    """Test path normalization."""
    tar_contents = ['./', './foo/', './foo/./a', './foo/./b']
    dir_struct = [Dir('.', []), Dir('foo', ['a', 'b'])]
    self._MockTarList(tar_contents)
    cros_test_lib.VerifyTarball(self.TARBALL, dir_struct)

  def testDuplicate(self):
    """Test duplicate detection."""
    tar_contents = ['a', 'b', 'a']
    dir_struct = ['a', 'b']
    self._MockTarList(tar_contents)
    self.assertRaises(AssertionError, cros_test_lib.VerifyTarball, self.TARBALL,
                      dir_struct)


class MockTestCaseTest(cros_test_lib.TestCase):
  """Tests MockTestCase functionality."""

  class MyMockTestCase(cros_test_lib.MockTestCase):
    """Helper class for testing MockTestCase."""
    def testIt(self):
      pass

  class Mockable(object):
    """Helper test class intended for having values mocked out."""
    TO_BE_MOCKED = 0
    TO_BE_MOCKED2 = 10
    TO_BE_MOCKED3 = 20

  def GetPatcher(self, attr, val):
    return mock.patch('%s.MockTestCaseTest.Mockable.%s' % (__name__, attr),
                      new=val)

  def testPatchRemovalError(self):
    """Verify that patch removal during tearDown is robust to Exceptions."""
    tc = self.MyMockTestCase('testIt')
    patcher = self.GetPatcher('TO_BE_MOCKED', -100)
    patcher2 = self.GetPatcher('TO_BE_MOCKED2', -200)
    patcher3 = self.GetPatcher('TO_BE_MOCKED3', -300)
    patcher3.start()
    tc.setUp()
    tc.StartPatcher(patcher)
    tc.StartPatcher(patcher2)
    patcher.stop()
    self.assertEquals(self.Mockable.TO_BE_MOCKED2, -200)
    self.assertEquals(self.Mockable.TO_BE_MOCKED3, -300)
    self.assertRaises(RuntimeError, tc.tearDown)
    # Make sure that even though exception is raised for stopping 'patcher', we
    # continue to stop 'patcher2', and run patcher.stopall().
    self.assertEquals(self.Mockable.TO_BE_MOCKED2, 10)
    self.assertEquals(self.Mockable.TO_BE_MOCKED3, 20)


class TestCaseTest(unittest.TestCase):
  """Tests TestCase functionality."""

  def testTimeout(self):
    """Test that test cases are interrupted when they are hanging."""

    class TimeoutTestCase(cros_test_lib.TestCase):
      """Test case that raises a TimeoutError because it takes too long."""

      TEST_CASE_TIMEOUT = 1

      def testSleeping(self):
        """Sleep for 2 minutes. This should raise a TimeoutError."""
        time.sleep(2 * 60)
        raise AssertionError('Test case should have timed out.')

    # Run the test case, verifying it raises a TimeoutError.
    test = TimeoutTestCase(methodName='testSleeping')
    self.assertRaises(timeout_util.TimeoutError, test.testSleeping)


class OutputTestCaseTest(cros_test_lib.OutputTestCase,
                         cros_test_lib.TempDirTestCase):
  """Tests OutputTestCase functionality."""

  def testStdoutAndStderr(self):
    """Check capturing stdout and stderr."""
    with self.OutputCapturer():
      print('foo')
      print('bar', file=sys.stderr)
    self.AssertOutputContainsLine('foo')
    self.AssertOutputContainsLine('bar', check_stdout=False, check_stderr=True)

  def testStdoutReadDuringCapture(self):
    """Check reading stdout mid-capture."""
    with self.OutputCapturer():
      print('foo')
      self.AssertOutputContainsLine('foo')
      print('bar')
      self.AssertOutputContainsLine('bar')
    self.AssertOutputContainsLine('foo')
    self.AssertOutputContainsLine('bar')

  def testClearCaptured(self):
    """Check writing data, clearing it, then writing more data."""
    with self.OutputCapturer() as cap:
      print('foo')
      self.AssertOutputContainsLine('foo')
      cap.ClearCaptured()
      self.AssertOutputContainsLine('foo', invert=True)
      print('bar')
    self.AssertOutputContainsLine('bar')

  def testRunCommandCapture(self):
    """Check capturing RunCommand() subprocess output."""
    with self.OutputCapturer():
      cros_build_lib.RunCommand(['sh', '-c', 'echo foo; echo bar >&2'],
                                mute_output=False)
    self.AssertOutputContainsLine('foo')
    self.AssertOutputContainsLine('bar', check_stdout=False, check_stderr=True)

  def testCapturingStdoutAndStderrToFile(self):
    """Check that OutputCapturer captures to a named file."""
    stdout_path = os.path.join(self.tempdir, 'stdout')
    stderr_path = os.path.join(self.tempdir, 'stderr')
    with self.OutputCapturer(stdout_path=stdout_path, stderr_path=stderr_path):
      print('foo')
      print('bar', file=sys.stderr)

    # Check that output can be read by OutputCapturer.
    self.AssertOutputContainsLine('foo')
    self.AssertOutputContainsLine('bar', check_stdout=False, check_stderr=True)
    # Verify that output is actually written to the correct files.
    self.assertEqual('foo\n', osutils.ReadFile(stdout_path))
    self.assertEqual('bar\n', osutils.ReadFile(stderr_path))
