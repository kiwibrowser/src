# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the retry_util module."""

from __future__ import print_function

import itertools
import mock
import os
import time

from chromite.lib import cros_build_lib
from chromite.lib import cros_test_lib
from chromite.lib import retry_util
from chromite.lib import osutils


class TestRetries(cros_test_lib.MockTempDirTestCase):
  """Tests of GenericRetry and relatives."""

  def testWithRetrySuccess(self):
    """Test basic retry success case."""
    @retry_util.WithRetry(max_retry=3)
    def _run():
      return 10
    self.assertEqual(10, _run())

  def testWithRetrySuccessAfterRetry(self):
    """Test basic retry success case, but failed at least once."""
    counter = itertools.count()
    @retry_util.WithRetry(max_retry=3)
    def _run():
      current = counter.next()
      # Failed twice, then success.
      if current < 2:
        raise Exception()
      return 10
    self.assertEqual(10, _run())

  def testWithRetryFail(self):
    """Test basic retry fail case."""
    @retry_util.WithRetry(max_retry=3)
    def _run():
      raise Exception('Retry fail')
    with self.assertRaisesRegexp(Exception, 'Retry fail'):
      _run()

  def testGenericRetry(self):
    """Test basic semantics of retry and success recording."""
    source = iter(xrange(5)).next

    def _TestMain():
      val = source()
      if val < 4:
        raise ValueError()
      return val

    handler = lambda ex: isinstance(ex, ValueError)

    callback_args = []
    with self.assertRaises(ValueError):
      retry_util.GenericRetry(
          handler, 3, _TestMain,
          status_callback=lambda *args: callback_args.append(args))
    self.assertEqual(callback_args,
                     [(0, False), (1, False), (2, False), (3, False)])

    callback_args = []
    self.assertEqual(
        4,
        retry_util.GenericRetry(
            handler, 1, _TestMain,
            status_callback=lambda *args: callback_args.append(args)))
    self.assertEqual(callback_args, [(0, True)])

    callback_args = []
    with self.assertRaises(StopIteration):
      retry_util.GenericRetry(
          handler, 3, _TestMain,
          status_callback=lambda *args: callback_args.append(args))
    self.assertEqual(callback_args, [(0, False)])

  def testGenericRetryBadArgs(self):
    """Test bad retry related arguments to GenericRetry raise ValueError."""
    def _AlwaysRaise():
      raise Exception('Not a ValueError')

    # |max_retry| must be non-negative number.
    with self.assertRaises(ValueError):
      retry_util.GenericRetry(lambda _: True, -1, _AlwaysRaise)

    # |backoff_factor| must be 1 or greator.
    with self.assertRaises(ValueError):
      retry_util.GenericRetry(lambda _: True, 3, _AlwaysRaise,
                              backoff_factor=0.9)

    # Sleep must be non-negative number.
    with self.assertRaises(ValueError):
      retry_util.GenericRetry(lambda _: True, 3, _AlwaysRaise, sleep=-1)

  def testRaisedException(self):
    """Test which exception gets raised by repeated failure."""

    def _GetTestMain():
      """Get function that fails once with ValueError, Then AssertionError."""
      source = itertools.count()
      def _TestMain():
        if source.next() == 0:
          raise ValueError()
        else:
          raise AssertionError()
      return _TestMain

    with self.assertRaises(ValueError):
      retry_util.GenericRetry(lambda _: True, 3, _GetTestMain())

    with self.assertRaises(AssertionError):
      retry_util.GenericRetry(lambda _: True, 3, _GetTestMain(),
                              raise_first_exception_on_failure=False)

  class CheckException(Exception):
    """Exception thrown from the below function."""

  def _RaiseCheckException(self, *_):
    raise TestRetries.CheckException()

  def testStatustCallbackExceptionForSuccess(self):
    """Exception from |status_callback| should be raised even on success."""
    with self.assertRaises(TestRetries.CheckException):
      retry_util.GenericRetry(lambda _: True, 1, lambda: None,
                              status_callback=self._RaiseCheckException)

  def testStatusCallbackExceptionForRetry(self):
    """Exception from |status_callback| should stop retry."""
    counter = [0]  # Counter to track how many times _functor is called.
    def _TestMain():
      counter[0] += 1
      raise Exception()  # Let it fail.

    with self.assertRaises(TestRetries.CheckException):
      retry_util.GenericRetry(lambda _: True, 10, _TestMain,
                              status_callback=self._RaiseCheckException)
    # Do not expect retry in case |status_callback| raises an exception.
    self.assertEqual(counter[0], 1)

  def testRetryExceptionBadArgs(self):
    """Verify we reject non-classes or tuples of classes"""
    with self.assertRaises(TypeError):
      retry_util.RetryException('', 3, map)
    with self.assertRaises(TypeError):
      retry_util.RetryException(123, 3, map)
    with self.assertRaises(TypeError):
      retry_util.RetryException(None, 3, map)
    with self.assertRaises(TypeError):
      retry_util.RetryException([None], 3, map)

  def testRetryException(self):
    """Verify we retry only when certain exceptions get thrown"""
    source = iter(xrange(6)).next
    def _TestMain():
      val = source()
      if val < 2:
        raise OSError()
      if val < 5:
        raise ValueError()
      return val
    with self.assertRaises(OSError):
      retry_util.RetryException((OSError, ValueError), 2, _TestMain)
    with self.assertRaises(ValueError):
      retry_util.RetryException((OSError, ValueError), 1, _TestMain)
    self.assertEqual(5, retry_util.RetryException(ValueError, 1, _TestMain))
    with self.assertRaises(StopIteration):
      retry_util.RetryException(ValueError, 3, _TestMain)

  def testRetryWithBackoff(self):
    sleep_history = []
    self.PatchObject(time, 'sleep', new=sleep_history.append)
    def _AlwaysFail():
      raise ValueError()
    with self.assertRaises(ValueError):
      retry_util.GenericRetry(lambda _: True, 5, _AlwaysFail, sleep=1,
                              backoff_factor=2)

    self.assertEqual(sleep_history, [1, 2, 4, 8, 16])

  def testBasicRetry(self):
    # pylint: disable=E1101
    path = os.path.join(self.tempdir, 'script')
    paths = {
        'stop': os.path.join(self.tempdir, 'stop'),
        'store': os.path.join(self.tempdir, 'store'),
    }
    osutils.WriteFile(
        path,
        "import sys\n"
        "val = int(open(%(store)r).read())\n"
        "stop_val = int(open(%(stop)r).read())\n"
        "open(%(store)r, 'w').write(str(val + 1))\n"
        "print val\n"
        "sys.exit(0 if val == stop_val else 1)\n" % paths)

    os.chmod(path, 0o755)

    def _SetupCounters(start, stop):
      sleep_mock.reset_mock()
      osutils.WriteFile(paths['store'], str(start))
      osutils.WriteFile(paths['stop'], str(stop))

    def _AssertCounters(sleep, sleep_cnt):
      calls = [mock.call(float(sleep * (x + 1))) for x in range(sleep_cnt)]
      sleep_mock.assert_has_calls(calls)

    sleep_mock = self.PatchObject(time, 'sleep')

    _SetupCounters(0, 0)
    command = ['python2', path]
    kwargs = {'redirect_stdout': True, 'print_cmd': False}
    self.assertEqual(cros_build_lib.RunCommand(command, **kwargs).output, '0\n')
    _AssertCounters(0, 0)

    func = retry_util.RunCommandWithRetries

    _SetupCounters(2, 2)
    self.assertEqual(func(0, command, sleep=0, **kwargs).output, '2\n')
    _AssertCounters(0, 0)

    _SetupCounters(0, 2)
    self.assertEqual(func(2, command, sleep=1, **kwargs).output, '2\n')
    _AssertCounters(1, 2)

    _SetupCounters(0, 1)
    self.assertEqual(func(1, command, sleep=2, **kwargs).output, '1\n')
    _AssertCounters(2, 1)

    _SetupCounters(0, 3)
    with self.assertRaises(cros_build_lib.RunCommandError):
      func(2, command, sleep=3, **kwargs)
    _AssertCounters(3, 2)
