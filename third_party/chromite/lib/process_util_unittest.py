# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the process_util.py module."""

from __future__ import print_function

import os
import signal

from chromite.lib import cros_test_lib
from chromite.lib import process_util


def _SpawnChild(exit_code=None, kill_signal=None):
  """Create a child, have it exit/killed, and return its status."""
  assert exit_code is not None or kill_signal is not None

  pid = os.fork()
  if pid == 0:
    # Make sure this child never returns.
    while True:
      if exit_code is not None:
        # pylint: disable=W0212
        os._exit(exit_code)
      else:
        os.kill(os.getpid(), kill_signal)

  return os.waitpid(pid, 0)[1]


class GetExitStatusTests(cros_test_lib.TestCase):
  """Tests for GetExitStatus()"""

  def testExitNormal(self):
    """Verify normal exits get decoded."""
    status = _SpawnChild(exit_code=0)
    ret = process_util.GetExitStatus(status)
    self.assertEqual(ret, 0)

  def testExitError(self):
    """Verify error exits (>0 && <128) get decoded."""
    status = _SpawnChild(exit_code=10)
    ret = process_util.GetExitStatus(status)
    self.assertEqual(ret, 10)

  def testExitWeird(self):
    """Verify weird exits (>=128) get decoded."""
    status = _SpawnChild(exit_code=150)
    ret = process_util.GetExitStatus(status)
    self.assertEqual(ret, 150)

  def testSIGUSR1(self):
    """Verify normal kill signals get decoded."""
    status = _SpawnChild(kill_signal=signal.SIGUSR1)
    ret = process_util.GetExitStatus(status)
    self.assertEqual(ret, 128 + signal.SIGUSR1)

  def testSIGKILL(self):
    """Verify harsh signals get decoded."""
    status = _SpawnChild(kill_signal=signal.SIGKILL)
    ret = process_util.GetExitStatus(status)
    self.assertEqual(ret, 128 + signal.SIGKILL)


class ExitAsStatusTests(cros_test_lib.TestCase):
  """Tests for ExitAsStatus()"""

  def _Tester(self, exit_code=None, kill_signal=None):
    """Helper func for testing ExitAsStatus()

    Create a child to mimic the grandchild.
    Create a grandchild and have it exit/killed.
    Assert behavior based on exit/signal behavior.
    """
    pid = os.fork()
    if pid == 0:
      # Let the grandchild exit/kill itself.
      # The child should mimic the grandchild.
      status = _SpawnChild(exit_code=exit_code, kill_signal=kill_signal)
      try:
        process_util.ExitAsStatus(status)
      except SystemExit as e:
        # pylint: disable=W0212
        os._exit(e.code)
      raise AssertionError('ERROR: should have exited!')

    # The parent returns the child's status.
    status = os.waitpid(pid, 0)[1]
    if exit_code is not None:
      self.assertFalse(os.WIFSIGNALED(status))
      self.assertTrue(os.WIFEXITED(status))
      self.assertEqual(os.WEXITSTATUS(status), exit_code)
    else:
      self.assertFalse(os.WIFEXITED(status))
      self.assertTrue(os.WIFSIGNALED(status))
      self.assertEqual(os.WTERMSIG(status), kill_signal)

  def testExitNormal(self):
    """Verify normal exits get decoded."""
    self._Tester(exit_code=0)

  def testExitError(self):
    """Verify error exits (>0 && <128) get decoded."""
    self._Tester(exit_code=10)

  def testExitWeird(self):
    """Verify weird exits (>=128) get decoded."""
    self._Tester(exit_code=150)

  def testSIGUSR1(self):
    """Verify normal kill signals get decoded."""
    self._Tester(kill_signal=signal.SIGUSR1)

  def testSIGKILL(self):
    """Verify harsh signals get decoded."""
    self._Tester(kill_signal=signal.SIGKILL)
