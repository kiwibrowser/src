# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import multiprocessing
import os
import Queue
import resource
import signal

import psutil


def _LimitMemory(memory_share):
  """Limits the memory available to this process, to avoid OOM issues.

  Args:
    memory_share: (float) Share coefficient of the total physical memory that
                          the process can use.
  """
  total_memory = psutil.virtual_memory().total
  memory_limit = memory_share * total_memory
  resource.setrlimit(resource.RLIMIT_AS, (memory_limit, -1L))


def _MultiprocessingWrapper(queue, memory_share, function, args):
  """Helper function that sets a memory limit on the current process, then
  calls |function| on |args| and writes the results to |queue|.

  Args:
    queue: (multiprocessing.Queue) Queue where the results of the wrapped
           function are written.
    memory_share: (float) Share coefficient of the total physical memory that
                          the process can use.
    function: The wrapped function.
    args: (list) Arguments for the wrapped function.
  """
  try:
    if memory_share:
      _LimitMemory(memory_share)

    queue.put(function(*args))
  except Exception:
    queue.put(None)


def RunInSeparateProcess(function, args, logger, timeout_seconds,
                         memory_share=None):
  """Runs a function in a separate process, and kills it after the timeout is
  reached.

  Args:
    function: The function to run.
    args: (list) Arguments for the wrapped function.
    timeout_seconds: (float) Timeout in seconds after which the subprocess is
                     terminated.
    memory_share: (float) Set this parameter to limit the memory available to
                  the spawned subprocess. This is a ratio of the total system
                  memory (between 0 and 1).
  Returns:
    The result of the wrapped function, or None if the call failed.
  """
  queue = multiprocessing.Queue()
  process = multiprocessing.Process(target=_MultiprocessingWrapper,
                                    args=(queue, memory_share, function, args))
  process.daemon = True
  process.start()

  result = None

  try:
    logger.info('Wait for result.')
    # Note: If the subprocess somehow crashes (e.g. Python crashing), this
    # process will wait the full timeout. Could be avoided but probably not
    # worth the extra complexity.
    result = queue.get(block=True, timeout=timeout_seconds)
  except Queue.Empty:
    logger.warning('Subprocess timeout.')
    process.terminate()

  logger.info('Wait for process to terminate.')
  process.join(timeout=5)

  if process.is_alive():
    logger.warning('Process still alive, hard killing now.')
    os.kill(process.pid, signal.SIGKILL)

  return result
