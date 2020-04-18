# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper for inframon's command-line flag based configuration."""

from __future__ import print_function

import argparse
import contextlib
import multiprocessing
import os
import socket
import signal
import time
import Queue

from chromite.lib import cros_logging as logging
from chromite.lib import metrics
from chromite.lib import parallel

try:
  from infra_libs.ts_mon import config
  import googleapiclient.discovery
except (ImportError, RuntimeError) as e:
  config = None
  logging.warning('Failed to import ts_mon, monitoring is disabled: %s', e)


_WasSetup = False

FLUSH_INTERVAL = 60


@contextlib.contextmanager
def TrivialContextManager():
  """Context manager with no side effects."""
  yield


def SetupTsMonGlobalState(service_name,
                          indirect=False,
                          suppress_exception=True,
                          short_lived=False,
                          auto_flush=True,
                          debug_file=None,
                          task_num=0):
  """Uses a dummy argument parser to get the default behavior from ts-mon.

  Args:
    service_name: The name of the task we are sending metrics from.
    indirect: Whether to create a metrics.METRICS_QUEUE object and a separate
              process for indirect metrics flushing. Useful for forking,
              because forking would normally create a duplicate ts_mon thread.
    suppress_exception: True to silence any exception during the setup. Default
                        is set to True.
    short_lived: Whether this process is short-lived and should use the autogen
                 hostname prefix.
    auto_flush: Whether to create a thread to automatically flush metrics every
                minute.
    debug_file: If non-none, send metrics to this path instead of to PubSub.
    task_num: (Default 0) The task_num target field of the metrics to emit.
  """
  if not config:
    return TrivialContextManager()

  # The flushing subprocess calls .flush manually.
  if indirect:
    auto_flush = False

  # google-api-client has too much noisey logging.
  options = _GenerateTsMonArgparseOptions(
      service_name, short_lived, auto_flush, debug_file, task_num)

  if indirect:
    return _CreateTsMonFlushingProcess(options)
  else:
    _SetupTsMonFromOptions(options, suppress_exception)
    return TrivialContextManager()


def _SetupTsMonFromOptions(options, suppress_exception):
  """Sets up ts-mon global state given parsed argparse options.

  Args:
    options: An argparse options object containing ts-mon flags.
    suppress_exception: True to silence any exception during the setup. Default
                        is set to True.
  """
  googleapiclient.discovery.logger.setLevel(logging.WARNING)
  try:
    config.process_argparse_options(options)
    logging.notice('ts_mon was set up.')
    global _WasSetup  # pylint: disable=global-statement
    _WasSetup = True
  except Exception as e:
    logging.warning('Failed to configure ts_mon, monitoring is disabled: %s', e,
                    exc_info=True)
    if not suppress_exception:
      raise


def _GenerateTsMonArgparseOptions(service_name, short_lived,
                                  auto_flush, debug_file, task_num):
  """Generates an arg list for ts-mon to consume.

  Args:
    service_name: The name of the task we are sending metrics from.
    short_lived: Whether this process is short-lived and should use the autogen
                 hostname prefix.
    auto_flush: Whether to create a thread to automatically flush metrics every
                minute.
    debug_file: If non-none, send metrics to this path instead of to PubSub.
    task_num: Override the default task num of 0.
  """
  parser = argparse.ArgumentParser()
  config.add_argparse_options(parser)

  args = [
      '--ts-mon-target-type', 'task',
      '--ts-mon-task-service-name', service_name,
      '--ts-mon-task-job-name', service_name,
  ]

  if debug_file:
    args.extend(['--ts-mon-endpoint', 'file://' + debug_file])

  # Short lived processes will have autogen: prepended to their hostname and
  # use task-number=PID to trigger shorter retention policies under
  # chrome-infra@, and used by a Monarch precomputation to group across the
  # task number.
  # Furthermore, we assume they manually call ts_mon.Flush(), because the
  # ts_mon thread will drop messages if the process exits before it flushes.
  if short_lived:
    auto_flush = False
    fqdn = socket.getfqdn().lower()
    host = fqdn.split('.')[0]
    args.extend(['--ts-mon-task-hostname', 'autogen:' + host,
                 '--ts-mon-task-number', str(os.getpid())])
  elif task_num:
    args.extend(['--ts-mon-task-number', str(task_num)])

  args.extend(['--ts-mon-flush', 'auto' if auto_flush else 'manual'])
  return parser.parse_args(args=args)


@contextlib.contextmanager
def _CreateTsMonFlushingProcess(options):
  """Creates a separate process to flush ts_mon metrics.

  Useful for multiprocessing scenarios where we don't want multiple ts-mon
  threads send contradictory metrics. Instead, functions in
  chromite.lib.metrics will send their calls to a Queue, which is consumed by a
  dedicated flushing process.

  Args:
    options: An argparse options object to configure ts-mon with.

  Side effects:
    Sets chromite.lib.metrics.MESSAGE_QUEUE, which causes the metric functions
    to send their calls to the Queue instead of creating the metrics.
  """
  # If this is nested, we don't need to create another queue and another
  # message consumer. Do nothing to continue to use the existing queue.
  if metrics.MESSAGE_QUEUE or metrics.FLUSHING_PROCESS:
    return

  with parallel.Manager() as manager:
    message_q = manager.Queue()

    metrics.FLUSHING_PROCESS = multiprocessing.Process(
        target=lambda: _SetupAndConsumeMessages(message_q, options))
    metrics.FLUSHING_PROCESS.start()

    # this makes the chromite.lib.metric functions use the queue.
    # note - we have to do this *after* forking the ConsumeMessages process.
    metrics.MESSAGE_QUEUE = message_q

    try:
      yield message_q
    finally:
      _CleanupMetricsFlushingProcess()


def _CleanupMetricsFlushingProcess():
  """Sends sentinal value to flushing process and .joins it."""
  # Now that there is no longer a process to listen to the Queue, re-set it
  # to None so that any future metrics are created within this process.
  message_q = metrics.MESSAGE_QUEUE
  flushing_process = metrics.FLUSHING_PROCESS
  metrics.MESSAGE_QUEUE = None
  metrics.FLUSHING_PROCESS = None

  # If the process has already died, we don't need to try to clean it up.
  if not flushing_process.is_alive():
    return

  # Send the sentinal value for "flush one more time and exit".
  try:
    message_q.put(None)
  # If the flushing process quits, the message Queue can become full.
  except IOError:
    if not flushing_process.is_alive():
      return

  logging.info("Waiting for ts_mon flushing process to finish...")
  flushing_process.join(timeout=FLUSH_INTERVAL*2)
  if flushing_process.is_alive():
    flushing_process.terminate()
  if flushing_process.exitcode:
    logging.warning("ts_mon_config flushing process did not exit cleanly.")
  logging.info("Finished waiting for ts_mon process.")


def _SetupAndConsumeMessages(message_q, options):
  """Sets up ts-mon, and starts a MetricConsumer loop.

  Args:
    message_q: The metric multiprocessing.Queue to read from.
    options: An argparse options object to configure ts-mon with.
  """
  # Configure ts-mon, but don't start up a sending thread.
  _SetupTsMonFromOptions(options, suppress_exception=True)
  if not _WasSetup:
    return

  return MetricConsumer(message_q).Consume()


class MetricConsumer(object):
  """Configures ts_mon and gets metrics from a message queue.

  This class is meant to be used in a subprocess. It configures itself
  to receive a SIGHUP signal when the parent process dies, and catches the
  signal in order to have a chance to flush any pending metrics one more time
  before quitting.
  """
  def __init__(self, message_q):
    # If our parent dies, finish flushing before exiting.
    self.reset_after_flush = []
    self.last_flush = 0
    self.pending = False
    self.message_q = message_q

    if parallel.ExitWithParent(signal.SIGHUP):
      signal.signal(signal.SIGHUP, lambda _sig, _stack: self._WaitToFlush())


  def Consume(self):
    """Emits metrics from self.message_q, flushing periodically.

    The loop is terminated by a None entry on the Queue, which is a friendly
    signal from the parent process that it's time to shut down. Before
    returning, we wait to flush one more time to make sure that all the
    metrics were sent.
    """
    message = self.message_q.get()
    while message:
      self._CallMetric(message)
      message = self._WaitForNextMessage()

    if self.pending:
      self._WaitToFlush()


  def _CallMetric(self, message):
    """Calls the metric method from |message|, ignoring exceptions."""
    try:
      cls = getattr(metrics, message.metric_name)
      metric = cls(*message.metric_args, **message.metric_kwargs)
      if message.reset_after:
        self.reset_after_flush.append(metric)
      getattr(metric, message.method)(
          *message.method_args,
          **message.method_kwargs)
      self.pending = True
    except Exception:
      logging.exception('Caught an exception while running %s',
                        _MethodCallRepr(message))


  def _WaitForNextMessage(self):
    """Waits for a new message, flushing every |FLUSH_INTERVAL| seconds."""
    while True:
      time_delta = self._FlushIfReady()
      try:
        timeout = FLUSH_INTERVAL - time_delta
        message = self.message_q.get(timeout=timeout)
        return message
      except Queue.Empty:
        pass


  def _WaitToFlush(self):
    """Sleeps until the next time we can call metrics.Flush(), then flushes."""
    time_delta = time.time() - self.last_flush
    time.sleep(max(0, FLUSH_INTERVAL - time_delta))
    metrics.Flush(reset_after=self.reset_after_flush)


  def _FlushIfReady(self):
    """Call metrics.Flush() if we are ready and have pending metrics.

    This allows us to only call flush every FLUSH_INTERVAL seconds.
    """
    now = time.time()
    time_delta = now - self.last_flush
    if time_delta > FLUSH_INTERVAL:
      self.last_flush = now
      time_delta = 0
      metrics.Flush(reset_after=self.reset_after_flush)
      self.pending = False
    return time_delta


def _MethodCallRepr(message):
  """Gives a string representation of |obj|.|method|(*|args|, **|kwargs|)

  Args:
    message: A MetricCall object.
  """
  if not message:
    return repr(message)
  obj = message.metric_name,
  method = message.method,
  args = message.method_args,
  kwargs = message.method_kwargs

  args_strings = (map(repr, args) +
                  [(str(k) + '=' + repr(v))
                   for k, v in kwargs.iteritems()])
  return '%s.%s(%s)' % (repr(obj), method, ', '.join(args_strings))
