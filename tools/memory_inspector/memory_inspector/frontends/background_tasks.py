# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Background tasks for the www_server module.

This module has the logic for handling background tasks for the www frontend.
Long term actions (like periodic tracing), cannot be served synchronously in the
context of a /ajax/endpoint request (would timeout HTTP). Instead, for such long
operations, an instance of |BackgroundTask| is created here and the server
returns just its id. The client can later poll the status of the asynchronous
task to check for its progress.

From a technical viewpoint, each background task is just a python thread
which communicates its progress updates through a Queue. The messages enqueued
are tuples with the following format: (completion_ratio%, 'message string').
"""

import datetime
import itertools
import Queue
import threading
import time

from memory_inspector.core import backends
from memory_inspector.data import file_storage


_task_id_generator = itertools.count(1)
_tasks = {}  #id (int) -> |BackgroundTask| instance.


def StartTracer(process, storage_path, interval, count, trace_native_heap):
  assert(isinstance(process, backends.Process))
  task = BackgroundTask(
      TracerMain_,
      storage_path=storage_path,
      backend_name=process.device.backend.name,
      device_id=process.device.id,
      pid=process.pid,
      interval=interval,
      count=count,
      trace_native_heap=trace_native_heap)
  task.start()
  task_id = _task_id_generator.next()
  _tasks[task_id] = task
  return task_id


def Get(task_id):
  return _tasks.get(task_id)


def TracerMain_(log, storage_path, backend_name, device_id, pid, interval,
    count, trace_native_heap):
  """Entry point for the background periodic tracer task."""
  # Initialize storage.
  storage = file_storage.Storage(storage_path)

  # Initialize the backend.
  backend = backends.GetBackend(backend_name)
  for k, v in storage.LoadSettings(backend_name).iteritems():
    backend.settings[k] = v

  # Initialize the device.
  device = backends.GetDevice(backend_name, device_id)
  for k, v in storage.LoadSettings(device_id).iteritems():
    device.settings[k] = v

  # Start periodic tracing.
  process = device.GetProcess(pid)
  log.put((1, 'Starting trace (%d dumps x %s s.). Device: %s, process: %s' % (
      count, interval, device.name, process.name)))
  datetime_str = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M')
  archive_name = '%s - %s - %s' % (datetime_str, device.name, process.name)
  archive = storage.OpenArchive(archive_name, create=True)
  heaps_to_symbolize = []

  for i in xrange(1, count + 1):  # [1, count] range is easier to handle.
    process = device.GetProcess(pid)
    if not process:
      log.put((100, 'Process %d died.' % pid))
      return 1
    # Calculate the completion rate proportionally to 80%. We keep the remaining
    # 20% for the final symbolization step (just an approximate estimation).
    completion = 80 * i / count
    log.put((completion, 'Dumping trace %d of %d' % (i, count)))
    archive.StartNewSnapshot()
    # Freeze the process, so that the mmaps and the heap dump are consistent.
    process.Freeze()
    try:
      if trace_native_heap:
        nheap = process.DumpNativeHeap()
        log.put((completion,
                 'Dumped %d native allocations' % len(nheap.allocations)))

      # TODO(primiano): memdump has the bad habit of sending SIGCONT to the
      # process. Fix that, so we are the only one in charge of controlling it.
      mmaps = process.DumpMemoryMaps()
      log.put((completion, 'Dumped %d memory maps' % len(mmaps)))
      archive.StoreMemMaps(mmaps)

      if trace_native_heap:
        nheap.RelativizeStackFrames(mmaps)
        nheap.CalculateResidentSize(mmaps)
        archive.StoreNativeHeap(nheap)
        heaps_to_symbolize += [nheap]
    finally:
      process.Unfreeze()

    if i < count:
      time.sleep(interval)

  if heaps_to_symbolize:
    log.put((90, 'Symbolizing'))
    symbols = backend.ExtractSymbols(
        heaps_to_symbolize, device.settings['native_symbol_paths'] or '')
    expected_symbols_count = len(set.union(
        *[set(x.stack_frames.iterkeys()) for x in heaps_to_symbolize]))
    log.put((99, 'Symbolization complete. Got %d symbols (%.1f%%).' % (
        len(symbols), 100.0 * len(symbols) / expected_symbols_count)))
    archive.StoreSymbols(symbols)

  log.put((100, 'Trace complete.'))
  return 0


class BackgroundTask(threading.Thread):
  def __init__(self, entry_point, *args, **kwargs):
    self._log_queue = Queue.Queue()
    self._progress_log = []  # A list of tuples [(50%, 'msg1'), (100%, 'msg2')].
    super(BackgroundTask, self).__init__(
        target=entry_point,
        args=((self._log_queue,) + args),  # Just propagate all args.
        kwargs=kwargs)
    self.daemon = True

  def run(self):
    try:
      super(BackgroundTask, self).run()
    except Exception, e:
      self._log_queue.put((
          100, 'An error occurred (%s). See the log for more details.' % e))
      raise

  def GetProgress(self):
    """ Returns a tuple (completion_rate, message). """
    while True:
      try:
        self._progress_log += [self._log_queue.get(block=False)]
      except Queue.Empty:
        break
    return self._progress_log
