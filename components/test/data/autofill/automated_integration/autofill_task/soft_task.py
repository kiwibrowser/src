# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import abc
import inspect
import os.path
import sys

# Local Imports
from .exceptions import ExpectationFailure


class SoftTask(object):
  """An extendable base task that provides soft/hard assertion functionality.
  """
  __metaclass__ = abc.ABCMeta

  def __init__(self):
    self._failed_expectations = []

  @abc.abstractmethod
  def set_up(self):
    """Sets up the task running environment.

    Note: Subclasses must implement this method.

    Raises:
      NotImplementedError: Subclass did not implement the method
    """
    raise NotImplementedError()

  @abc.abstractmethod
  def tear_down(self):
    """Tears down the task running environment.

    Any persistent changes made by set_up() must be reversed here.
    Note: Subclasses must implement this method.

    Raises:
      NotImplementedError: Subclass did not implement the method
    """
    raise NotImplementedError()

  @abc.abstractmethod
  def _run_task(self):
    """Executes the task.

    Note: Subclasses must implement this method.

    Raises:
      NotImplementedError: Subclass did not implement the method
    """
    raise NotImplementedError()

  def run(self):
    """Sets up, attempts to execute the task, and always tears down.

    Raises:
      ExpectationFailure: Task execution failed. Contains a
      NotImplementedError: Subclass did not implement the abstract methods
    """
    self.set_up()
    try:
      self._run_task()
    finally:
      # Attempt to tear down despite test failure
      self.tear_down()

  def expect_expression(self, expr, msg=None, stack=None):
    """Verifies an expression, logging a failure, but not abort the test.

    In order to ensure that none of the expected expressions have failed, one
    must call assert_expectations before the end of the test (which will fail
    it if any expectations were not met).
    """
    if not expr:
      self._log_failure(msg, stack=stack)

  def assert_expression(self, expr, msg=None, stack=None):
    """Perform a hard assertion but include failures from soft expressions too.

    Raises:
      ExpectationFailure: This or previous assertions did not pass. Contains a
        failure report.
    """
    if not expr:
      self._log_failure(msg, stack=stack)
      if self._failed_expectations:
        raise ExpectationFailure(self._report_failures())

  def assert_expectations(self):
    """Raise an assert if there were any failed expectations.

    Raises:
      ExpectationFailure: An expectation was not met. Contains a failure report.
    """
    if self._failed_expectations:
      raise ExpectationFailure(self._report_failures())

  def _log_failure(self, msg=None, frames=7, skip_frames=0, stack=None):
    """Generates a failure report from the current traceback.

    The generated failure report is added to an internal list. The reports
    are used by _report_failures

    Note: Since this is always called internally a minimum of two frames are
    dropped unless you provide a specific stack trace.

    Args:
      msg: Description of the failure.
      frames: The number of frames to include from the call to this function.
      skip_frames: The number of frames to skip. Useful if you have helper
        functions that you don't desire to be part of the trace.
      stack: A custom traceback to use instead of one from this function. No
        frames will be skipped by default.
    """
    if msg:
      failure_message = msg + '\n'
    else:
      failure_message = '\n'

    if stack is None:
      stack = inspect.stack()[skip_frames + 2:]
    else:
      stack = stack[skip_frames:]

    frames_to_use = min(frames, len(stack))
    stack = stack[:frames_to_use]

    for frame in stack:
      # First two frames are from logging
      if len(frame) == 4:
        # From the traceback module
        (filename, line, function_name, context) = frame
        context = '    %s\n' % context
      else:
        # Stack trace from the inspect module
        (filename, line, function_name, context_list) = frame[1:5]
        context = context_list[0]
      filename = os.path.basename(filename)

      failure_message += '  File "%s", line %s, in %s()\n%s' % (filename, line,
                                                                function_name,
                                                                context)

    self._failed_expectations.append(failure_message)

  def _report_failures(self, frames=1, skip_frames=0):
    """Generates a failure report for all failed expectations.

    Used in exception descriptions.

    Note: Since this is always called internally a minimum of two frames are
    dropped.

    Args:
      frames: The number of frames to include from the call to this function.
      skip_frames: The number of frames to skip. Useful if you have helper
        functions that you don't desire to be part of the trace.

    Returns:
      A string representation of all failed expectations.
    """
    if self._failed_expectations:
      # First two frames are from logging
      (filename, line, function_name) = inspect.stack()[skip_frames + 2][1:4]
      filename = os.path.basename(filename)

      report = [
          'Failed Expectations: %s\n' % len(self._failed_expectations),
          'assert_expectations() called from',
          '"%s" line %s, in %s()\n' % (filename, line, function_name)
      ]

      for i, failure in enumerate(self._failed_expectations, start=1):
        report.append('Expectation %d: %s' % (i, failure))

      self._failed_expectations = []
      return '\n'.join(report)
    else:
      return ''
