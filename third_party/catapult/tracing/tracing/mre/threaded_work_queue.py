# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import threading
import traceback
import Queue


class ThreadedWorkQueue(object):

  def __init__(self, num_threads):
    self._num_threads = num_threads

    self._main_thread_tasks = None
    self._any_thread_tasks = None

    self._running = False
    self._stop = False
    self._stop_result = None

    self.Reset()

  @property
  def is_running(self):
    return self._running

  def Run(self):
    if self.is_running:
      raise Exception('Already running')

    self._running = True
    self._stop = False
    self._stop_result = None

    if self._num_threads == 1:
      self._RunSingleThreaded()
    else:
      self._RunMultiThreaded()

    self._main_thread_tasks = Queue.Queue()
    self._any_thread_tasks = Queue.Queue()

    r = self._stop_result
    self._stop_result = None
    self._running = False

    return r

  def Stop(self, stop_result=None):
    if not self.is_running:
      raise Exception('Not running')

    if self._stop:
      return False
    self._stop_result = stop_result
    self._stop = True
    return True

  def Reset(self):
    assert not self.is_running
    self._main_thread_tasks = Queue.Queue()
    self._any_thread_tasks = Queue.Queue()

  def PostMainThreadTask(self, cb, *args, **kwargs):
    def RunTask():
      cb(*args, **kwargs)
    self._main_thread_tasks.put(RunTask)

  def PostAnyThreadTask(self, cb, *args, **kwargs):
    def RunTask():
      cb(*args, **kwargs)
    self._any_thread_tasks.put(RunTask)

  def _TryToRunOneTask(self, queue, block=False):
    if block:
      try:
        task = queue.get(True, 0.1)
      except Queue.Empty:
        return
    else:
      if queue.empty():
        return
      task = queue.get()

    try:
      task()
    except KeyboardInterrupt as ex:
      raise ex
    except Exception:  # pylint: disable=broad-except
      traceback.print_exc()
    finally:
      queue.task_done()

  def _RunSingleThreaded(self):
    while True:
      if self._stop:
        break
      self._TryToRunOneTask(self._any_thread_tasks)
      self._TryToRunOneTask(self._main_thread_tasks)

  def _RunMultiThreaded(self):
    threads = []
    for _ in range(self._num_threads):
      t = threading.Thread(target=self._ThreadMain)
      t.setDaemon(True)
      t.start()
      threads.append(t)

    while True:
      if self._stop:
        break
      self._TryToRunOneTask(self._main_thread_tasks)

    for t in threads:
      t.join()

  def _ThreadMain(self):
    while True:
      if self._stop:
        break
      self._TryToRunOneTask(self._any_thread_tasks, block=True)
