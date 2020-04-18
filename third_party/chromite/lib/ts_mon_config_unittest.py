# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for chromite.lib.metrics."""

from __future__ import print_function

import itertools
import multiprocessing
import Queue

from chromite.lib import cros_test_lib
from chromite.lib import metrics
from chromite.lib import ts_mon_config

from infra_libs.ts_mon.common import interface


# pylint: disable=protected-access
DEFAULT_OPTIONS = ts_mon_config._GenerateTsMonArgparseOptions(
    'unittest', False, False, None, 0)


class TestConsumeMessages(cros_test_lib.MockTestCase):
  """Test that ConsumeMessages works correctly."""

  def setUp(self):
    # Every call to "time.time()" will look like 1 second has passed.
    # Nb. we only want to mock out ts_mon_config's view of time, otherwise
    # things like Process.join(10) won't sleep.
    self.time_mock = self.PatchObject(ts_mon_config, 'time')
    self.time_mock.time.side_effect = itertools.count(0)
    self.flush_mock = self.PatchObject(ts_mon_config.metrics, 'Flush')
    self.PatchObject(ts_mon_config, '_SetupTsMonFromOptions')
    self.PatchObject(ts_mon_config, '_WasSetup', True)
    self.mock_metric = self.PatchObject(metrics, 'Boolean')


  def testNoneEndsProcess(self):
    """Putting None on the Queue should immediately end the consumption loop."""
    q = Queue.Queue()
    q.put(None)

    ts_mon_config._SetupAndConsumeMessages(q, DEFAULT_OPTIONS)

    ts_mon_config._SetupTsMonFromOptions.assert_called_once_with(
        DEFAULT_OPTIONS, suppress_exception=True)
    self.assertFalse(ts_mon_config.time.time.called)
    self.assertFalse(ts_mon_config.metrics.Flush.called)

  def testConsumeOneMetric(self):
    """Tests that sending one metric calls flush once."""
    q = Queue.Queue()
    q.put(metrics.MetricCall('Boolean', [], {},
                             'mock_name', ['arg1'], {'kwarg1': 'value'},
                             False))
    q.put(None)

    ts_mon_config._SetupAndConsumeMessages(q, DEFAULT_OPTIONS)

    self.assertEqual(2, ts_mon_config.time.time.call_count)
    ts_mon_config.time.sleep.assert_called_once_with(
        ts_mon_config.FLUSH_INTERVAL - 1)
    ts_mon_config.metrics.Flush.assert_called_once_with(reset_after=[])
    self.mock_metric.return_value.mock_name.assert_called_once_with(
        'arg1', kwarg1='value')

  def testConsumeTwoMetrics(self):
    """Tests that sending two metrics only calls flush once."""
    q = Queue.Queue()
    q.put(metrics.MetricCall('Boolean', [], {},
                             'mock_name1', ['arg1'], {'kwarg1': 'value'},
                             False))
    q.put(metrics.MetricCall('Boolean', [], {},
                             'mock_name2', ['arg2'], {'kwarg2': 'value'},
                             False))
    q.put(None)

    ts_mon_config._SetupAndConsumeMessages(q, DEFAULT_OPTIONS)

    self.assertEqual(3, ts_mon_config.time.time.call_count)
    ts_mon_config.time.sleep.assert_called_once_with(
        ts_mon_config.FLUSH_INTERVAL - 2)
    ts_mon_config.metrics.Flush.assert_called_once_with(reset_after=[])
    self.mock_metric.return_value.mock_name1.assert_called_once_with(
        'arg1', kwarg1='value')
    self.mock_metric.return_value.mock_name2.assert_called_once_with(
        'arg2', kwarg2='value')

  def testFlushingProcessExits(self):
    """Tests that _CreateTsMonFlushingProcess cleans up the process."""
    processes = []
    original_process_function = multiprocessing.Process
    def SaveProcess(*args, **kwargs):
      p = original_process_function(*args, **kwargs)
      processes.append(p)
      return p

    self.PatchObject(multiprocessing, 'Process', SaveProcess)

    with ts_mon_config._CreateTsMonFlushingProcess(DEFAULT_OPTIONS) as q:
      q.put(metrics.MetricCall('Boolean', [], {},
                               '__class__', [], {},
                               False))

    # wait a bit for the process to close, since multiprocessing.Queue and
    # Process.join() is not synchronous.
    processes[0].join(5)

    self.assertEqual(0, processes[0].exitcode)

  def testCatchesException(self):
    """Tests that the _SetupAndConsumeMessages loop catches exceptions."""
    q = Queue.Queue()

    class RaisesException(object):
      """Class to raise an exception"""
      def raiseException(self, *_args, **_kwargs):
        raise Exception()

    metrics.RaisesException = RaisesException
    q.put(metrics.MetricCall('RaisesException', [], {},
                             'raiseException', ['arg1'], {'kwarg1': 'value1'},
                             False))
    q.put(None)

    exception_log = self.PatchObject(ts_mon_config.logging, 'exception')

    ts_mon_config._SetupAndConsumeMessages(q, DEFAULT_OPTIONS)

    self.assertEqual(1, exception_log.call_count)
    # time.time is called once because we check if we need to Flush() before
    # receiving the None message.
    self.assertEqual(1, ts_mon_config.time.time.call_count)
    self.assertEqual(0, ts_mon_config.time.sleep.call_count)
    self.assertEqual(0, ts_mon_config.metrics.Flush.call_count)

  def testResetAfter(self):
    """Tests that metrics with reset_after set are cleared after."""
    q = Queue.Queue()

    q.put(metrics.MetricCall('Boolean', [], {},
                             'mock_name', ['arg1'], {'kwarg1': 'value1'},
                             reset_after=True))
    q.put(None)

    ts_mon_config._SetupAndConsumeMessages(q, DEFAULT_OPTIONS)

    self.assertEqual(
        [self.mock_metric.return_value],
        ts_mon_config.metrics.Flush.call_args[1]['reset_after'])
    self.mock_metric.return_value.mock_name.assert_called_once_with(
        'arg1', kwarg1='value1')

  def testSubprocessQuitsWhenNotSetup(self):
    self.PatchObject(ts_mon_config.logging, 'exception')
    self.PatchObject(ts_mon_config, '_WasSetup', False)
    ts_mon_config._SetupAndConsumeMessages(None, DEFAULT_OPTIONS)
    self.assertEqual(False, ts_mon_config._WasSetup)
    # The entry should not have been consumed by _ConsumeMessages
    self.assertEqual(0, ts_mon_config.logging.exception.call_count)

  def testSetOnceMetricKeepsEmitting(self):
    """Tests that a metric which is set once emits many times if left alone."""
    self.PatchObject(ts_mon_config, 'FLUSH_INTERVAL', 0)
    self.time_mock.time.side_effect = [1, 2, 3, 4, 5]
    q = Queue.Queue()
    q.put(metrics.MetricCall('Boolean', [], {},
                             '__class__', [], {},
                             False))
    try:
      ts_mon_config.MetricConsumer(q).Consume()
    except StopIteration:
      pass # No more time calls left.
    self.assertEqual(self.flush_mock.call_count, 5)


class TestSetupTsMonGlobalState(cros_test_lib.MockTestCase):
  """Test that SetupTsMonGlobalState works correctly."""

  def testTaskNumArgument(self):
    """The task_num argument should set the task_num in ts-mon."""
    ts_mon_config.SetupTsMonGlobalState('unittest', auto_flush=False,
                                        task_num=42)
    self.assertEqual(interface.state.target.task_num, 42)

  def testTaskNumWithIndirect(self):
    """The task_num argument should propagate to the flushing subprocess."""
    create_flushing_process = self.PatchObject(
        ts_mon_config, '_CreateTsMonFlushingProcess')
    ts_mon_config.SetupTsMonGlobalState('unittest', indirect=True, task_num=42)
    options = ts_mon_config._GenerateTsMonArgparseOptions(
        'unittest', False, False, None, 42)
    create_flushing_process.assert_called_once_with(options)
