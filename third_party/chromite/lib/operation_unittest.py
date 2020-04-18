# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittest for operation"""

from __future__ import print_function

import multiprocessing
import os
import sys

from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import operation
from chromite.lib import parallel


class TestWrapperProgressBarOperation(operation.ProgressBarOperation):
  """Inherit from operation.ProgressBarOperation for testing."""
  def ParseOutput(self, output=None):
    print("Calling ParseOutput")
    print(self._stdout.read())


class FakeParallelEmergeOperation(operation.ParallelEmergeOperation):
  """Fake for operation.ParallelEmergeOperation."""
  def __init__(self, queue):
    super(FakeParallelEmergeOperation, self).__init__()
    self._queue = queue

  def ParseOutput(self, output=None):
    super(FakeParallelEmergeOperation, self).ParseOutput()
    self._queue.put('advance')


class FakeException(Exception):
  """Fake exception used for testing exception handling."""


class ProgressBarOperationTest(cros_test_lib.MockTestCase,
                               cros_test_lib.OutputTestCase,
                               cros_test_lib.LoggingTestCase):
  """Test the Progress Bar Operation class."""
  # pylint: disable=protected-access

  def setUp(self):
    terminal_width = 20
    self._terminal = self.PatchObject(
        operation.ProgressBarOperation, '_GetTerminalSize',
        return_value=operation._TerminalSize(100, terminal_width))
    self.PatchObject(os, 'isatty', return_value=True)

  def _VerifyProgressBar(self, width, percent, expected_shaded,
                         expected_unshaded):
    """Helper to test progress bar with different percentages and lengths."""
    terminal_width = width + (
        operation.ProgressBarOperation._PROGRESS_BAR_BORDER_SIZE)
    self._terminal.return_value = operation._TerminalSize(100, terminal_width)
    op = operation.ProgressBarOperation()
    with self.OutputCapturer() as output:
      op.ProgressBar(percent)
    stdout = output.GetStdout()

    #Check that the shaded and unshaded regions are the expected size.
    self.assertEqual(stdout.count('#'), expected_shaded)
    self.assertEqual(stdout.count('-'), expected_unshaded)

  def testProgressBar(self):
    """Test progress bar at different percentages."""
    self._VerifyProgressBar(10, 0.7, 7, 3)
    self._VerifyProgressBar(10, 0, 0, 10)
    self._VerifyProgressBar(10, 1, 10, 0)
    self._VerifyProgressBar(1, 0.9, 0, 1)
    # If width of progress bar is less than _PROGRESS_BAR_BORDER_SIZE, the width
    # defaults to 1.
    self._VerifyProgressBar(-5, 0, 0, 1)
    self._VerifyProgressBar(-5, 1, 1, 0)

  def testWaitUntilComplete(self):
    """Test WaitUntilComplete returns False if background task isn't complete.

    As the background task is not started in this test, we expect it not to
    complete.
    """
    op = operation.ProgressBarOperation()
    self.assertFalse(op.WaitUntilComplete(0))

  def testCaptureOutputInBackground(self):
    """Test CaptureOutputInBackground puts finished in reasonable time."""
    def func():
      print('hi')

    op = operation.ProgressBarOperation()
    op.CaptureOutputInBackground(func)

    # This function should really finish in < 1 sec. However, we wait for a
    # longer time so the test does not fail on highly loaded builders.
    self.assertTrue(op.WaitUntilComplete(10))

  def testRun(self):
    """Test that ParseOutput is called and foo is run in background."""
    expected_output = 'hi'
    def func():
      print(expected_output)

    op = TestWrapperProgressBarOperation()
    with self.OutputCapturer():
      op.Run(func, update_period=0.05)

    # Check that foo is executed and its output is captured.
    self.AssertOutputContainsLine(expected_output)
    # Check that ParseOutput is executed at least once. It can be called twice:
    #   Once in the while loop.
    #   Once after the while loop.
    #   However, it is possible for func to execute and finish before the while
    #   statement is executed even once in which case ParseOutput would only be
    #   called once.
    self.AssertOutputContainsLine('Calling ParseOutput')

  def testExceptionHandling(self):
    """Test exception handling."""
    def func():
      print('foo')
      print('bar', file=sys.stderr)
      raise FakeException()

    op = TestWrapperProgressBarOperation()
    with self.OutputCapturer():
      try:
        with cros_test_lib.LoggingCapturer() as logs:
          op.Run(func)
      except parallel.BackgroundFailure:
        pass

    # Check that the output was dumped correctly.
    self.AssertLogsContain(logs, 'Something went wrong.')
    self.AssertOutputContainsLine('Captured stdout was')
    self.AssertOutputContainsLine('Captured stderr was')
    self.AssertOutputContainsLine('foo')
    self.AssertOutputContainsLine('bar', check_stderr=True)

  def testLogLevel(self):
    """Test that the log level of the function running is set correctly."""
    func_log_level = logging.DEBUG
    test_log_level = logging.NOTICE
    expected_output = 'hi'
    def func():
      if logging.getLogger().getEffectiveLevel() == func_log_level:
        print(expected_output)

    logging.getLogger().setLevel(test_log_level)
    op = TestWrapperProgressBarOperation()
    with self.OutputCapturer():
      op.Run(func, update_period=0.05, log_level=func_log_level)

    # Check that OutputCapturer contains the expected output. This means that
    # the log level was changed.
    self.AssertOutputContainsLine(expected_output)
    # Check that the log level was restored after the function executed.
    self.assertEqual(logging.getLogger().getEffectiveLevel(), test_log_level)

  def testParallelEmergeOperationParseOutputTotalNotFound(self):
    """Test that ParallelEmergeOperation.ParseOutput if total is not set."""
    def func():
      print('hi')

    op = operation.ParallelEmergeOperation()
    with self.OutputCapturer():
      op.Run(func)

    # Check that the output is empty.
    self.AssertOutputContainsLine('hi', check_stderr=True, invert=True)

  def testParallelEmergeOperationParseOutputTotalIsZero(self):
    """Test that ParallelEmergeOperation.ParseOutput if total is zero."""
    def func():
      print('Total: 0 packages.')

    op = operation.ParallelEmergeOperation()
    with self.OutputCapturer():
      with cros_test_lib.LoggingCapturer() as logs:
        op.Run(func)

    # Check that no progress bar is printed.
    self.AssertOutputContainsLine('%', check_stderr=True, invert=True)
    # Check logs contain message.
    self.AssertLogsContain(logs, 'No packages to build.')

  def testParallelEmergeOperationParseOutputTotalNonZero(self):
    """Test that ParallelEmergeOperation.ParseOutput's progress bar updates."""
    def func(queue):
      print('Total: 2 packages.')
      for _ in xrange(2):
        queue.get()
        print('Completed ')

    queue = multiprocessing.Queue()
    op = FakeParallelEmergeOperation(queue)
    with self.OutputCapturer():
      op.Run(func, queue, update_period=0.005)

    # Check that progress bar prints correctly at 0%, 50%, and 100%.
    self.AssertOutputContainsLine('0%')
    self.AssertOutputContainsLine('50%')
    self.AssertOutputContainsLine('100%')
