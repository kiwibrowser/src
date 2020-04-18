# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import decorators
from telemetry.internal.results import page_test_results
from telemetry.page import page as page_module
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.timeline import model as model_module
from telemetry.util import wpr_modes

from measurements import v8_gc_times


class V8GCTimesTestPageHelper(object):

  def __init__(self, page_set):
    self._page_set = page_set
    self._model = model_module.TimelineModel()
    self._renderer_process = self._model.GetOrCreateProcess(1)
    self._renderer_thread = self._renderer_process.GetOrCreateThread(2)
    self._renderer_thread.name = 'CrRendererMain'

  def AddEvent(self, category, name, thread_start, thread_duration,
               args=None, wall_start=None, wall_duration=None):
    wall_start = wall_start or thread_start
    wall_duration = wall_duration or thread_duration
    self._renderer_thread.BeginSlice(category, name, wall_start, thread_start,
                                     args=args)
    self._renderer_thread.EndSlice(wall_start + wall_duration,
                                   thread_start + thread_duration)

  class MockV8GCTimesPage(page_module.Page):

    def __init__(self, page_set):
      super(V8GCTimesTestPageHelper.MockV8GCTimesPage, self).__init__(
          'file://blank.html', page_set, page_set.base_dir, name='blank.html')

  def MeasureFakePage(self):
    # Create a fake page and add it to the page set.
    results = page_test_results.PageTestResults()
    page = V8GCTimesTestPageHelper.MockV8GCTimesPage(self._page_set)
    self._page_set.AddStory(page)

    # Pretend we're about to run the tests to silence lower level asserts.
    results.WillRunPage(page)

    v8_gc_times_metric = v8_gc_times.V8GCTimes()
    # pylint: disable=protected-access
    v8_gc_times_metric._renderer_process = self._renderer_process

    # Finalize the timeline import.
    self._model.FinalizeImport()

    # Measure the V8GCTimes metric and return the results
    # pylint: disable=protected-access
    v8_gc_times_metric._AddV8MetricsToResults(self._renderer_process, results)
    results.DidRunPage(page)
    return results


class V8GCTimesTests(page_test_test_case.PageTestTestCase):

  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF

  def testWithNoTraceEvents(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    results = test_page_helper.MeasureFakePage()
    self._AssertResultsEqual(_GetEmptyResults(), _ActualValues(results))

  def testWithNoGarbageCollectionEvents(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    test_page_helper.AddEvent(
        'toplevel', 'PostMessage', thread_start=0, thread_duration=14,
        wall_start=5, wall_duration=35)

    results = test_page_helper.MeasureFakePage()
    expected = _GetEmptyResults()
    expected['duration'] = ('ms', 35)
    expected['cpu_time'] = ('ms', 14)

    self._AssertResultsEqual(expected, _ActualValues(results))

  def testWithGarbageCollectionEvents(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    test_page_helper.AddEvent(
        'toplevel', 'PostMessage', thread_start=0, thread_duration=77,
        wall_start=5, wall_duration=88)
    test_page_helper.AddEvent('v8', 'V8.GCScavenger', 5, 4)
    test_page_helper.AddEvent('v8', 'V8.GCScavenger', 15, 3)
    test_page_helper.AddEvent('v8', 'V8.GCIncrementalMarking', 23, 4)
    test_page_helper.AddEvent('v8', 'V8.GCIncrementalMarking', 34, 2)
    test_page_helper.AddEvent('v8', 'V8.GCFinalizeMC', 38, 2)
    test_page_helper.AddEvent('v8', 'V8.GCFinalizeMC', 42, 3)
    test_page_helper.AddEvent('v8', 'V8.GCFinalizeMCReduceMemory', 46, 4)
    test_page_helper.AddEvent('v8', 'V8.GCFinalizeMCReduceMemory', 51, 5)
    test_page_helper.AddEvent('v8', 'V8.GCCompactor', 62, 4)
    test_page_helper.AddEvent('v8', 'V8.GCCompactor', 72, 5)

    results = test_page_helper.MeasureFakePage()
    expected = _GetEmptyResults()
    expected['duration'] = ('ms', 88)
    expected['cpu_time'] = ('ms', 77)
    expected['v8_gc_incremental_marking'] = ('ms', 6.0)
    expected['v8_gc_incremental_marking_average'] = ('ms', 3.0)
    expected['v8_gc_incremental_marking_count'] = ('count', 2)
    expected['v8_gc_incremental_marking_max'] = ('ms', 4.0)
    expected['v8_gc_incremental_marking_outside_idle'] = ('ms', 6.0)
    expected['v8_gc_finalize_incremental'] = ('ms', 5.0)
    expected['v8_gc_finalize_incremental_average'] = ('ms', 2.5)
    expected['v8_gc_finalize_incremental_count'] = ('count', 2)
    expected['v8_gc_finalize_incremental_max'] = ('ms', 3.0)
    expected['v8_gc_finalize_incremental_outside_idle'] = ('ms', 5.0)
    expected['v8_gc_finalize_incremental_reduce_memory'] = ('ms', 9.0)
    expected['v8_gc_finalize_incremental_reduce_memory_average'] = ('ms', 4.5)
    expected['v8_gc_finalize_incremental_reduce_memory_count'] = ('count', 2)
    expected['v8_gc_finalize_incremental_reduce_memory_max'] = ('ms', 5.0)
    expected['v8_gc_finalize_incremental_reduce_memory_outside_idle'] = (
        'ms', 9.0)
    expected['v8_gc_scavenger'] = ('ms', 7.0)
    expected['v8_gc_scavenger_average'] = ('ms', 3.5)
    expected['v8_gc_scavenger_count'] = ('count', 2)
    expected['v8_gc_scavenger_max'] = ('ms', 4.0)
    expected['v8_gc_scavenger_outside_idle'] = ('ms', 7.0)
    expected['v8_gc_mark_compactor'] = ('ms', 9.0)
    expected['v8_gc_mark_compactor_average'] = ('ms', 4.5)
    expected['v8_gc_mark_compactor_count'] = ('count', 2)
    expected['v8_gc_mark_compactor_max'] = ('ms', 5.0)
    expected['v8_gc_mark_compactor_outside_idle'] = ('ms', 9.0)
    expected['v8_gc_total'] = ('ms', 36.0)
    expected['v8_gc_total_outside_idle'] = ('ms', 36.0)

    self._AssertResultsEqual(expected, _ActualValues(results))

  def testWithIdleTaskGarbageCollectionEvents(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    test_page_helper.AddEvent(
        'toplevel', 'PostMessage', thread_start=0, thread_duration=57,
        wall_start=5, wall_duration=68)

    test_page_helper.AddEvent('v8', 'V8.GCScavenger', 5, 4)
    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        15, 4, {'allotted_time_ms': 12})
    test_page_helper.AddEvent('v8', 'V8.GCScavenger', 15, 3)

    test_page_helper.AddEvent('v8', 'V8.GCIncrementalMarking', 23, 4)
    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        34, 3, {'allotted_time_ms': 12})
    test_page_helper.AddEvent('v8', 'V8.GCIncrementalMarking', 34, 2)

    test_page_helper.AddEvent('v8', 'V8.GCCompactor', 42, 4)
    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        52, 6, {'allotted_time_ms': 12})
    test_page_helper.AddEvent('v8', 'V8.GCCompactor', 52, 5)

    results = test_page_helper.MeasureFakePage()
    expected = _GetEmptyResults()
    expected['duration'] = ('ms', 68)
    expected['cpu_time'] = ('ms', 57)
    expected['v8_gc_incremental_marking'] = ('ms', 6.0)
    expected['v8_gc_incremental_marking_average'] = ('ms', 3.0)
    expected['v8_gc_incremental_marking_count'] = ('count', 2)
    expected['v8_gc_incremental_marking_max'] = ('ms', 4.0)
    expected['v8_gc_incremental_marking_outside_idle'] = ('ms', 4.0)
    expected['v8_gc_incremental_marking_percentage_idle'] = \
        ('idle%', 100 * 2 / 6.0)
    expected['v8_gc_scavenger'] = ('ms', 7.0)
    expected['v8_gc_scavenger_average'] = ('ms', 3.5)
    expected['v8_gc_scavenger_count'] = ('count', 2)
    expected['v8_gc_scavenger_max'] = ('ms', 4.0)
    expected['v8_gc_scavenger_outside_idle'] = ('ms', 4.0)
    expected['v8_gc_scavenger_percentage_idle'] = ('idle%', 100 * 3 / 7.0)
    expected['v8_gc_mark_compactor'] = ('ms', 9.0)
    expected['v8_gc_mark_compactor_average'] = ('ms', 4.5)
    expected['v8_gc_mark_compactor_count'] = ('count', 2)
    expected['v8_gc_mark_compactor_max'] = ('ms', 5.0)
    expected['v8_gc_mark_compactor_outside_idle'] = ('ms', 4.0)
    expected['v8_gc_mark_compactor_percentage_idle'] = ('idle%', 100 * 5 / 9.0)
    expected['v8_gc_total'] = ('ms', 22.0)
    expected['v8_gc_total_outside_idle'] = ('ms', 12.0)
    expected['v8_gc_total_percentage_idle'] = ('idle%', 100 * 10 / 22.0)

    self._AssertResultsEqual(expected, _ActualValues(results))

  def testWithIdleTaskOverruns(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    test_page_helper.AddEvent(
        'toplevel', 'PostMessage', thread_start=0, thread_duration=80,
        wall_start=5, wall_duration=92)

    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        15, 15, {'allotted_time_ms': 8})
    test_page_helper.AddEvent('v8', 'V8.GCScavenger', 15, 14)

    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        34, 15, {'allotted_time_ms': 6})
    test_page_helper.AddEvent('v8', 'V8.GCIncrementalMarking', 34, 14)

    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        52, 23, {'allotted_time_ms': 9})
    test_page_helper.AddEvent('v8', 'V8.GCCompactor', 52, 22)

    results = test_page_helper.MeasureFakePage()
    expected = _GetEmptyResults()
    expected['duration'] = ('ms', 92)
    expected['cpu_time'] = ('ms', 80)
    expected['v8_gc_incremental_marking'] = ('ms', 14.0)
    expected['v8_gc_incremental_marking_average'] = ('ms', 14.0)
    expected['v8_gc_incremental_marking_count'] = ('count', 1)
    expected['v8_gc_incremental_marking_max'] = ('ms', 14.0)
    expected['v8_gc_incremental_marking_outside_idle'] = ('ms', 8.0)
    expected['v8_gc_incremental_marking_idle_deadline_overrun'] = ('ms', 8.0)
    expected['v8_gc_incremental_marking_percentage_idle'] = \
        ('idle%', 100 * 6 / 14.0)
    expected['v8_gc_scavenger'] = ('ms', 14.0)
    expected['v8_gc_scavenger_average'] = ('ms', 14.0)
    expected['v8_gc_scavenger_count'] = ('count', 1)
    expected['v8_gc_scavenger_max'] = ('ms', 14.0)
    expected['v8_gc_scavenger_outside_idle'] = ('ms', 6.0)
    expected['v8_gc_scavenger_idle_deadline_overrun'] = ('ms', 6.0)
    expected['v8_gc_scavenger_percentage_idle'] = ('idle%', 100 * 8 / 14.0)
    expected['v8_gc_mark_compactor'] = ('ms', 22.0)
    expected['v8_gc_mark_compactor_average'] = ('ms', 22.0)
    expected['v8_gc_mark_compactor_count'] = ('count', 1)
    expected['v8_gc_mark_compactor_max'] = ('ms', 22.0)
    expected['v8_gc_mark_compactor_outside_idle'] = ('ms', 13.0)
    expected['v8_gc_mark_compactor_idle_deadline_overrun'] = ('ms', 13.0)
    expected['v8_gc_mark_compactor_percentage_idle'] = ('idle%', 100 * 9 / 22.0)
    expected['v8_gc_total'] = ('ms', 50.0)
    expected['v8_gc_total_outside_idle'] = ('ms', 27.0)
    expected['v8_gc_total_idle_deadline_overrun'] = ('ms', 27.0)
    expected['v8_gc_total_percentage_idle'] = ('idle%', 100 * 23 / 50.0)

    self._AssertResultsEqual(expected, _ActualValues(results))

  def testWithIdleTaskWallDurationOverruns(self):
    test_page_helper = V8GCTimesTestPageHelper(
        self.CreateEmptyPageSet())

    test_page_helper.AddEvent(
        'toplevel', 'PostMessage', thread_start=0, thread_duration=80,
        wall_start=5, wall_duration=92)

    test_page_helper.AddEvent(
        'renderer.scheduler', 'SingleThreadIdleTaskRunner::RunTask',
        15, 15, {'allotted_time_ms': 8})
    test_page_helper.AddEvent(
        'v8', 'V8.GCScavenger', thread_start=15, thread_duration=4,
        wall_start=15, wall_duration=14)

    results = test_page_helper.MeasureFakePage()
    expected = _GetEmptyResults()
    expected['duration'] = ('ms', 92)
    expected['cpu_time'] = ('ms', 80)
    expected['v8_gc_scavenger'] = ('ms', 4.0)
    expected['v8_gc_scavenger_average'] = ('ms', 4.0)
    expected['v8_gc_scavenger_count'] = ('count', 1)
    expected['v8_gc_scavenger_max'] = ('ms', 4.0)
    expected_outside_idle = 4.0 - (4.0 * 8 / 14)
    expected['v8_gc_scavenger_outside_idle'] = ('ms', expected_outside_idle)
    expected['v8_gc_scavenger_idle_deadline_overrun'] = ('ms', 6.0)
    expected['v8_gc_scavenger_percentage_idle'] = \
        ('idle%', 100 * (4.0 - expected_outside_idle) / 4.0)
    expected['v8_gc_total'] = expected['v8_gc_scavenger']
    expected['v8_gc_total_outside_idle'] = \
        expected['v8_gc_scavenger_outside_idle']
    expected['v8_gc_total_idle_deadline_overrun'] = \
        expected['v8_gc_scavenger_idle_deadline_overrun']
    expected['v8_gc_total_percentage_idle'] = \
        expected['v8_gc_scavenger_percentage_idle']

    self._AssertResultsEqual(expected, _ActualValues(results))

  def _AssertResultsEqual(self, expected, actual):
    for key in expected.iterkeys():
      self.assertIn(key, actual.keys())
      self.assertEqual(expected[key], actual[key],
                       'Result for [' + key + '] - expected ' +
                       str(expected[key]) + ' but got ' + str(actual[key]))

  @decorators.Disabled('win')  # crbug.com/416502
  def testCleanUpTrace(self):
    self.TestTracingCleanedUp(v8_gc_times.V8GCTimes, self._options)


def _ActualValues(results):
  return dict(list(
      (v.name, (v.units, v.value))
      for v in results.all_page_specific_values
  ))


def _GetEmptyResults():
  return {'cpu_time': ('ms', 0.0),
          'duration': ('ms', 0.0),
          'v8_gc_incremental_marking': ('ms', 0.0),
          'v8_gc_incremental_marking_average': ('ms', 0.0),
          'v8_gc_incremental_marking_count': ('count', 0),
          'v8_gc_incremental_marking_max': ('ms', 0.0),
          'v8_gc_incremental_marking_idle_deadline_overrun': ('ms', 0.0),
          'v8_gc_incremental_marking_outside_idle': ('ms', 0.0),
          'v8_gc_incremental_marking_percentage_idle': ('idle%', 0.0),
          'v8_gc_finalize_incremental': ('ms', 0.0),
          'v8_gc_finalize_incremental_average': ('ms', 0.0),
          'v8_gc_finalize_incremental_count': ('count', 0),
          'v8_gc_finalize_incremental_max': ('ms', 0.0),
          'v8_gc_finalize_incremental_idle_deadline_overrun': ('ms', 0.0),
          'v8_gc_finalize_incremental_outside_idle': ('ms', 0.0),
          'v8_gc_finalize_incremental_percentage_idle': ('idle%', 0.0),
          'v8_gc_finalize_incremental_reduce_memory': ('ms', 0.0),
          'v8_gc_finalize_incremental_reduce_memory_average': ('ms', 0.0),
          'v8_gc_finalize_incremental_reduce_memory_count': ('count', 0),
          'v8_gc_finalize_incremental_reduce_memory_max': ('ms', 0.0),
          'v8_gc_finalize_incremental_reduce_memory_idle_deadline_overrun':
              ('ms', 0.0),
          'v8_gc_finalize_incremental_reduce_memory_outside_idle': ('ms', 0.0),
          'v8_gc_finalize_incremental_reduce_memory_percentage_idle':
              ('idle%', 0.0),
          'v8_gc_mark_compactor': ('ms', 0.0),
          'v8_gc_mark_compactor_average': ('ms', 0.0),
          'v8_gc_mark_compactor_count': ('count', 0),
          'v8_gc_mark_compactor_max': ('ms', 0.0),
          'v8_gc_mark_compactor_idle_deadline_overrun': ('ms', 0.0),
          'v8_gc_mark_compactor_outside_idle': ('ms', 0.0),
          'v8_gc_mark_compactor_percentage_idle': ('idle%', 0.0),
          'v8_gc_scavenger': ('ms', 0.0),
          'v8_gc_scavenger_average': ('ms', 0.0),
          'v8_gc_scavenger_count': ('count', 0),
          'v8_gc_scavenger_max': ('ms', 0.0),
          'v8_gc_scavenger_idle_deadline_overrun': ('ms', 0.0),
          'v8_gc_scavenger_outside_idle': ('ms', 0.0),
          'v8_gc_scavenger_percentage_idle': ('idle%', 0.0),
          'v8_gc_total': ('ms', 0.0),
          'v8_gc_total_idle_deadline_overrun': ('ms', 0.0),
          'v8_gc_total_outside_idle': ('ms', 0.0)}
