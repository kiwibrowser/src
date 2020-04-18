# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Utility classes for the parallelism framework."""

from __future__ import absolute_import

import multiprocessing
import Queue
import threading

ZERO_TASKS_TO_DO_ARGUMENT = ('There were no', 'tasks to do')


# Timeout for puts/gets to the global status queue, in seconds.
STATUS_QUEUE_OP_TIMEOUT = 5

# Maximum time to wait (join) on the SeekAheadThread after the ProducerThread
# completes, in seconds.
SEEK_AHEAD_JOIN_TIMEOUT = 60

# Maximum time to wait (join) on the UIThread after the Apply
# completes, in seconds.
UI_THREAD_JOIN_TIMEOUT = 60


class AtomicDict(object):
  """Thread-safe (and optionally process-safe) dictionary protected by a lock.

  If a multiprocessing.Manager is supplied on init, the dictionary is
  both process and thread safe. Otherwise, it is only thread-safe.
  """

  def __init__(self, manager=None):
    """Initializes the dict.

    Args:
      manager: multiprocessing.Manager instance (required for process safety).
    """
    if manager:
      self.lock = manager.Lock()
      self.dict = manager.dict()
    else:
      self.lock = threading.Lock()
      self.dict = {}

  def __getitem__(self, key):
    with self.lock:
      return self.dict[key]

  def __setitem__(self, key, value):
    with self.lock:
      self.dict[key] = value

  # pylint: disable=invalid-name
  def get(self, key, default_value=None):
    with self.lock:
      return self.dict.get(key, default_value)

  def delete(self, key):
    with self.lock:
      del self.dict[key]

  def values(self):
    with self.lock:
      return self.dict.values()

  def Increment(self, key, inc, default_value=0):
    """Atomically updates the stored value associated with the given key.

    Performs the atomic equivalent of
    dict[key] = dict.get(key, default_value) + inc.

    Args:
      key: lookup key for the value of the first operand of the "+" operation.
      inc: Second operand of the "+" operation.
      default_value: Default value if there is no existing value for the key.

    Returns:
      Incremented value.
    """
    with self.lock:
      val = self.dict.get(key, default_value) + inc
      self.dict[key] = val
      return val


class ProcessAndThreadSafeInt(object):
  """This class implements a process and thread-safe integer.

  It is backed either by a multiprocessing Value of type 'i' or an internal
  threading lock.  This simplifies the calling pattern for
  global variables that could be a Multiprocessing.Value or an integer.
  Without this class, callers need to write code like this:

  global variable_name
  if isinstance(variable_name, int):
    return variable_name
  else:
    return variable_name.value
  """

  def __init__(self, multiprocessing_is_available):
    self.multiprocessing_is_available = multiprocessing_is_available
    if self.multiprocessing_is_available:
      # Lock is implicit in multiprocessing.Value
      self.value = multiprocessing.Value('i', 0)
    else:
      self.lock = threading.Lock()
      self.value = 0

  def Reset(self, reset_value=0):
    if self.multiprocessing_is_available:
      self.value.value = reset_value
    else:
      with self.lock:
        self.value = reset_value

  def Increment(self):
    if self.multiprocessing_is_available:
      self.value.value += 1
    else:
      with self.lock:
        self.value += 1

  def Decrement(self):
    if self.multiprocessing_is_available:
      self.value.value -= 1
    else:
      with self.lock:
        self.value -= 1

  def GetValue(self):
    if self.multiprocessing_is_available:
      return self.value.value
    else:
      with self.lock:
        return self.value


# Pylint gets confused by the mixed lower and upper-case method names in
# AtomicDict.
# pylint: disable=invalid-name
def PutToQueueWithTimeout(queue, msg, timeout=STATUS_QUEUE_OP_TIMEOUT):
  """Puts an item to the status queue.

  If the queue is full, this function will timeout periodically and repeat
  until success. This avoids deadlock during shutdown by never making a fully
  blocking call to the queue, since Python signal handlers cannot execute
  in between instructions of the Python interpreter (see
  https://docs.python.org/2/library/signal.html for details).

  Args:
    queue: Queue class (typically the global status queue)
    msg: message to post to the queue.
    timeout: (optional) amount of time to wait before repeating put request.
  """
  put_success = False
  while not put_success:
    try:
      queue.put(msg, timeout=timeout)
      put_success = True
    except Queue.Full:
      pass
# pylint: enable=invalid-name
