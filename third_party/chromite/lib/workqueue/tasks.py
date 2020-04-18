# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Task manager classes for work queues."""

from __future__ import print_function

import abc
import multiprocessing

from chromite.lib import cros_logging as logging


def _ExecuteTask(handler, request_data):
  """Wrapper for the task handler function."""
  root_logger = logging.getLogger()
  for h in list(root_logger.handlers):
    root_logger.removeHandler(h)
  try:
    return handler(request_data)
  except Exception as e:
    return e


class TaskManager(object):
  """Abstract base class for task management.

  `TaskManager` is responsible for managing individual work queue
  requests from the time that they're scheduled to run, until they
  complete or are aborted.
  """

  __metaclass__ = abc.ABCMeta

  def __init__(self, handler, sample_interval):
    self.sample_interval = sample_interval
    self._handler = handler

  @abc.abstractmethod
  def StartTick(self):
    """Start the polling cycle in `WorkQueueService.ProcessRequests()`.

    The work queue service's server polling loop will call this function
    once per loop iteration, to mark the nominal start of the polling
    cycle.
    """

  @abc.abstractmethod
  def HasCapacity(self):
    """Return whether there is capacity to start more tasks.

    Returns:
      A true value if there is enough capacity for at least one
      additional call to `StartTask()`.
    """
    return False

  @abc.abstractmethod
  def StartTask(self, request_id, request_data):
    """Start work on a new task request.

    Args:
      request_id: Identifier for the task, used by `TerminateTask()`
        and `Reap()`.
      request_data: Argument to be passed to the task handler.
    """

  @abc.abstractmethod
  def TerminateTask(self, request_id):
    """Terminate a running task.

    A terminated task will be forgotten, and will never be returned
    by `Reap()`.

    Args:
      request_id: Identifier of the task to be terminated.
    """

  @abc.abstractmethod
  def Reap(self):
    """Generator to return results of all completed tasks.

    Yields:
      A `(request_id, return_value)` tuple.
    """
    pass


class ProcessPoolTaskManager(TaskManager):
  """A task manager implemented with `multiprocessing.Pool`."""

  def __init__(self, max_tasks, handler, sample_interval):
    super(ProcessPoolTaskManager, self).__init__(handler, sample_interval)
    self._pool = multiprocessing.Pool(max_tasks)
    self._max_tasks = max_tasks
    self._pending_results = {}
    self._pending_aborts = set()

  def __len__(self):
    return len(self._pending_results)

  def StartTick(self):
    pass

  def HasCapacity(self):
    return len(self) < self._max_tasks

  def StartTask(self, request_id, request_data):
    self._pending_results[request_id] = (
        self._pool.apply_async(_ExecuteTask,
                               (self._handler, request_data)))

  def TerminateTask(self, request_id):
    self._pending_aborts.add(request_id)

  def Reap(self):
    for request_id, result in self._pending_results.items():
      if result.ready():
        del self._pending_results[request_id]
        if request_id in self._pending_aborts:
          self._pending_aborts.remove(request_id)
        else:
          yield request_id, result.get()

  def Close(self):
    self._pool.terminate()
    self._pool.join()
