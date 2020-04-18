# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for `tasks.ProcessPoolTaskManager`."""

from __future__ import print_function

import datetime
import os
import time
import unittest

from chromite.lib import cros_test_lib
from chromite.lib import osutils
from chromite.lib.workqueue import tasks


_INTERVAL = float(0xdeadbeef)

# N.B. The latency numbers need to be really generous for the sake of
# builders that may be loaded when they run these tests.
_START_LATENCY = 120.0
_REAP_LATENCY = 1.0


def _TaskHandler(task_file):
  """Test handler under control of the unit tests.

  This handler creates the file named by `task_file`, and then waits
  forever until the file is deleted.  This gives test cases the power
  to decide when a task will terminate.

  Args:
    task_file: Name of the file used to control this method's
      termination.

  Returns:
    the passed in `task_file` argument.
  """
  osutils.Touch(task_file)
  while os.path.exists(task_file):
    time.sleep(0.001)
  return task_file


class ProcessPoolTaskManagerTests(cros_test_lib.TempDirTestCase):
  """Test cases for all `ProcessPoolTaskManager` methods."""

  # REQUEST_IDS - fake request ids used for `StartTask()`.
  _REQUEST_IDS = ['a', 'b']

  def setUp(self):
    self._task_manager = tasks.ProcessPoolTaskManager(
        len(self._REQUEST_IDS), _TaskHandler, _INTERVAL)
    self._pending_tasks = {}
    self._expected = set()
    self._stop_count = 0

  def tearDown(self):
    self._task_manager.Close()

  def _StopTask(self, rqid):
    """Stop a task and return its expected result."""
    rqfile = self._pending_tasks.pop(rqid)
    os.remove(rqfile)
    self._stop_count += 1
    return rqfile

  def _StartTask(self, rqid):
    """Call the task manager's `StartTask()` method."""
    rqfile = os.path.join(self.tempdir, rqid)
    self._pending_tasks[rqid] = rqfile
    prior_len = len(self._task_manager)
    self._task_manager.StartTask(rqid, rqfile)
    after_len = len(self._task_manager)
    self.assertEqual(prior_len + 1, after_len)
    # Wait to confirm that the task we started is actually running.
    # This code is complicated because it has to pass on release
    # builders that can be heavily loaded when we do this.  The
    # sleep() call is meant to solve two problems:
    #  + Reduce overall CPU load.
    #  + Make sure that the system scheduler sees a chance to schedule
    #    the child process that we're waiting on.
    # Note that the length of the sleep call here can't be too long, or
    # the unit tests will be obnoxiously slow.  However, on a loaded
    # system, we want to be patient, so _START_LATENCY can be big.
    t_end = (datetime.datetime.now()
             + datetime.timedelta(seconds=_START_LATENCY))
    msg = ('Starting task "%s" timed out '
           'after %.2f seconds.' % (rqid, _START_LATENCY))
    while not os.path.exists(rqfile):
      time.sleep(0.02)
      self.assertLessEqual(datetime.datetime.now(), t_end, msg)

  def _TerminateTask(self, rqid):
    """Call the task manager's `TerminateTask()` method."""
    has_capacity = self._task_manager.HasCapacity()
    prior_len = len(self._task_manager)
    self._task_manager.TerminateTask(rqid)
    self._StopTask(rqid)
    after_len = len(self._task_manager)
    self.assertEqual(prior_len, after_len)
    self.assertEqual(has_capacity, self._task_manager.HasCapacity())

  def _EndTask(self, rqid):
    """Stop a task and remember its expected return value."""
    has_capacity = self._task_manager.HasCapacity()
    prior_len = len(self._task_manager)
    self._expected.add((rqid, self._StopTask(rqid)))
    after_len = len(self._task_manager)
    self.assertEqual(prior_len, after_len)
    self.assertEqual(has_capacity, self._task_manager.HasCapacity())

  def _CheckReap(self):
    """Call `Reap()` and check the return conditions."""
    prior_capacity = self._task_manager.HasCapacity()
    prior_size = len(self._task_manager)
    time.sleep(_REAP_LATENCY)
    actual = set(self._task_manager.Reap())
    self.assertEqual(actual, self._expected)
    self.assertEqual(prior_size,
                     len(self._task_manager) + self._stop_count)
    if self._stop_count or prior_capacity:
      self.assertTrue(self._task_manager.HasCapacity())
    else:
      self.assertFalse(self._task_manager.HasCapacity())
    self._expected.clear()
    self._stop_count = 0

  def _StartAll(self):
    """Start one task for every entry in `self._REQUEST_IDS`."""
    self.assertEqual(len(self._task_manager), 0)
    for rqid in self._REQUEST_IDS:
      self._StartTask(rqid)
    self.assertEqual(len(self._task_manager),
                     len(self._REQUEST_IDS))
    self.assertFalse(self._task_manager.HasCapacity())
    self._CheckReap()

  def _ReapSingly(self, in_order):
    """Reap all tasks one at a time."""
    self._StartAll()
    for rqid in sorted(self._REQUEST_IDS, reverse=not in_order):
      self._EndTask(rqid)
      self._CheckReap()

  def _ReapAll(self, in_order):
    """Reap all tasks with a single call to `Reap()`."""
    self._StartAll()
    for rqid in sorted(self._REQUEST_IDS, reverse=not in_order):
      self._EndTask(rqid)
    self._CheckReap()

  def _TerminateSingly(self, in_order):
    """Terminate all tasks checking `Reap()` after each termination."""
    self._StartAll()
    for rqid in sorted(self._REQUEST_IDS, reverse=not in_order):
      self._TerminateTask(rqid)
      self._CheckReap()

  def _TerminateAll(self, in_order):
    """Terminate all tasks checking `Reap()` only at the end."""
    self._StartAll()
    for rqid in sorted(self._REQUEST_IDS, reverse=not in_order):
      self._TerminateTask(rqid)
    self._CheckReap()

  def testEmpty(self):
    self.assertTrue(self._task_manager.HasCapacity())
    self.assertEqual(len(self._task_manager), 0)
    self._CheckReap()

  def testTasksInSequence(self):
    for rqid in self._REQUEST_IDS:
      self._StartTask(rqid)
      self._CheckReap()
      self._EndTask(rqid)
      self._CheckReap()

  def testReapInOrder(self):
    for _ in range(0, 2):
      self._ReapSingly(True)

  def testReapReversed(self):
    for _ in range(0, 2):
      self._ReapSingly(False)

  def testReapAllInOrder(self):
    for _ in range(0, 2):
      self._ReapAll(True)

  def testReapAllReversed(self):
    for _ in range(0, 2):
      self._ReapAll(False)

  def testTerminateInOrder(self):
    for _ in range(0, 2):
      self._TerminateSingly(True)

  def testTerminateReversed(self):
    for _ in range(0, 2):
      self._TerminateSingly(False)

  def testTerminateAllInOrder(self):
    for _ in range(0, 2):
      self._TerminateAll(True)

  def testTerminateAllReversed(self):
    for _ in range(0, 2):
      self._TerminateAll(False)

  def testEndThenTerminate(self):
    self._StartAll()
    self._EndTask(self._REQUEST_IDS[0])
    self._TerminateTask(self._REQUEST_IDS[1])
    self._CheckReap()

  def testTerminateThenEnd(self):
    self._StartAll()
    self._TerminateTask(self._REQUEST_IDS[0])
    self._EndTask(self._REQUEST_IDS[1])
    self._CheckReap()

  def testTerminateThenRun(self):
    self._StartTask(self._REQUEST_IDS[0])
    self._CheckReap()
    self._TerminateTask(self._REQUEST_IDS[0])
    self._CheckReap()
    self._StartTask(self._REQUEST_IDS[1])
    self._CheckReap()
    self._EndTask(self._REQUEST_IDS[1])
    self._CheckReap()

  def testRunThenTerminate(self):
    self._StartTask(self._REQUEST_IDS[1])
    self._CheckReap()
    self._EndTask(self._REQUEST_IDS[1])
    self._CheckReap()
    self._StartTask(self._REQUEST_IDS[0])
    self._CheckReap()
    self._TerminateTask(self._REQUEST_IDS[0])
    self._CheckReap()


if __name__ == '__main__':
  unittest.main()
