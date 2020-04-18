# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from contrib.oilpan import oilpan_gc_times

from telemetry.internal.results import page_test_results
from telemetry.page import page as page_module
from telemetry.testing import options_for_unittests
from telemetry.testing import page_test_test_case
from telemetry.timeline import model
from telemetry.timeline import slice as slice_data

import mock  # pylint: disable=import-error


class TestOilpanGCTimesPage(page_module.Page):

  def __init__(self, page_set):
    super(TestOilpanGCTimesPage, self).__init__(
        'file://blank.html', page_set, page_set.base_dir, name='blank.html')

  def RunPageInteractions(self, action_runner):
    with action_runner.CreateGestureInteraction('ScrollAction'):
      action_runner.ScrollPage()


class OilpanGCTimesTestData(object):

  def __init__(self, thread_name):
    self._model = model.TimelineModel()
    self._renderer_process = self._model.GetOrCreateProcess(1)
    self._renderer_thread = self._renderer_process.GetOrCreateThread(2)
    self._renderer_thread.name = thread_name
    self._results = page_test_results.PageTestResults()

  @property
  def results(self):
    return self._results

  def AddSlice(self, name, timestamp, duration, args):
    new_slice = slice_data.Slice(
        None,
        'category',
        name,
        timestamp,
        duration,
        timestamp,
        duration,
        args)
    self._renderer_thread.all_slices.append(new_slice)
    return new_slice

  def AddAsyncSlice(self, name, timestamp, duration, args):
    new_slice = slice_data.Slice(
        None,
        'category',
        name,
        timestamp,
        duration,
        timestamp,
        duration,
        args)
    self._renderer_thread.async_slices.append(new_slice)
    return new_slice

  def ClearResults(self):
    self._results = page_test_results.PageTestResults()


class OilpanGCTimesTest(page_test_test_case.PageTestTestCase):
  """Smoke test for Oilpan GC pause time measurements.

     Runs OilpanGCTimes measurement on some simple pages and verifies
     that all metrics were added to the results.  The test is purely functional,
     i.e. it only checks if the metrics are present and non-zero.
  """
  _KEY_MARK = 'BlinkGC.AtomicPhaseMarking'
  _KEY_LAZY_SWEEP = 'BlinkGC.LazySweepInIdle'
  _KEY_COMPLETE_SWEEP = 'BlinkGC.CompleteSweep'
  _KEY_MEASURE = 'BlinkGCTimeMeasurement'
  # Do not add 'forced' in reasons to measure.
  _GC_REASONS = ['precise', 'conservative', 'idle']

  def setUp(self):
    self._options = options_for_unittests.GetCopy()

  # Disable for accessing private API of _OilpanGCTimesBase.
  # pylint: disable=protected-access
  def testForParsingOldFormat(self):
    def getMetric(results, name):
      metrics = results.FindAllPageSpecificValuesNamed(name)
      self.assertEquals(1, len(metrics))
      return metrics[0].GetBuildbotValue()

    data = self._GenerateDataForParsingOldFormat()

    measurement = oilpan_gc_times._OilpanGCTimesBase()

    tab = mock.MagicMock()
    with mock.patch(
        'contrib.oilpan.oilpan_gc_times.TimelineModel') as MockTimelineModel:
      MockTimelineModel.return_value = data._model
      measurement.ValidateAndMeasurePage(None, tab, data.results)

    results = data.results
    self.assertEquals(3, len(getMetric(results, 'oilpan_precise_mark')))
    self.assertEquals(3, len(getMetric(results, 'oilpan_precise_lazy_sweep')))
    self.assertEquals(3, len(getMetric(results,
                                       'oilpan_precise_complete_sweep')))
    self.assertEquals(1, len(getMetric(results, 'oilpan_conservative_mark')))
    self.assertEquals(1, len(getMetric(results,
                                       'oilpan_conservative_lazy_sweep')))
    self.assertEquals(1, len(getMetric(results,
                                       'oilpan_conservative_complete_sweep')))
    self.assertEquals(2, len(getMetric(results, 'oilpan_forced_mark')))
    self.assertEquals(2, len(getMetric(results, 'oilpan_forced_lazy_sweep')))
    self.assertEquals(2, len(getMetric(results,
                                       'oilpan_forced_complete_sweep')))

  # Disable for accessing private API of _OilpanGCTimesBase.
  # pylint: disable=protected-access
  def testForParsing(self):
    def getMetric(results, name):
      metrics = results.FindAllPageSpecificValuesNamed(name)
      self.assertEquals(1, len(metrics))
      return metrics[0].GetBuildbotValue()

    data = self._GenerateDataForParsing()

    measurement = oilpan_gc_times._OilpanGCTimesBase()
    measurement._timeline_model = data._model
    tab = mock.MagicMock()
    with mock.patch(
        'contrib.oilpan.oilpan_gc_times.TimelineModel') as MockTimelineModel:
      MockTimelineModel.return_value = data._model
      measurement.ValidateAndMeasurePage(None, tab, data.results)

    results = data.results
    self.assertEquals(4, len(getMetric(results, 'oilpan_precise_mark')))
    self.assertEquals(4, len(getMetric(results, 'oilpan_precise_lazy_sweep')))
    self.assertEquals(4, len(getMetric(results,
                                       'oilpan_precise_complete_sweep')))
    self.assertEquals(5, len(getMetric(results, 'oilpan_conservative_mark')))
    self.assertEquals(5, len(getMetric(results,
                                       'oilpan_conservative_lazy_sweep')))
    self.assertEquals(5, len(getMetric(results,
                                       'oilpan_conservative_complete_sweep')))
    self.assertEquals(1, len(getMetric(results, 'oilpan_forced_mark')))
    self.assertEquals(1, len(getMetric(results, 'oilpan_forced_lazy_sweep')))
    self.assertEquals(1, len(getMetric(results,
                                       'oilpan_forced_complete_sweep')))
    self.assertEquals(2, len(getMetric(results, 'oilpan_idle_mark')))
    self.assertEquals(2, len(getMetric(results, 'oilpan_idle_lazy_sweep')))
    self.assertEquals(2, len(getMetric(results,
                                       'oilpan_idle_complete_sweep')))

  def testForSmoothness(self):
    ps = self.CreateStorySetFromFileInUnittestDataDir(
        'create_many_objects.html')
    measurement = oilpan_gc_times.OilpanGCTimesForSmoothness()
    results = self.RunMeasurement(measurement, ps, options=self._options)
    self.assertFalse(results.had_failures)

    gc_events = []
    for gc_reason in self._GC_REASONS:
      label = 'oilpan_%s_mark' % gc_reason
      gc_events.extend(results.FindAllPageSpecificValuesNamed(label))
    self.assertLess(0, len(gc_events))

  def testForBlinkPerf(self):
    ps = self.CreateStorySetFromFileInUnittestDataDir(
        'create_many_objects.html')
    measurement = oilpan_gc_times.OilpanGCTimesForBlinkPerf()
    results = self.RunMeasurement(measurement, ps, options=self._options)
    self.assertFalse(results.had_failures)

    gc_events = []
    for gc_reason in self._GC_REASONS:
      label = 'oilpan_%s_mark' % gc_reason
      gc_events.extend(results.FindAllPageSpecificValuesNamed(label))
    self.assertLess(0, len(gc_events))

  def _GenerateDataForEmptyPageSet(self):
    page_set = self.CreateEmptyPageSet()
    page = TestOilpanGCTimesPage(page_set)
    page_set.AddStory(page)

    data = OilpanGCTimesTestData('CrRendererMain')
    # Pretend we are about to run the tests to silence lower level asserts.
    data.results.WillRunPage(page)

    return data

  def _GenerateDataForParsingOldFormat(self):
    data = self._GenerateDataForEmptyPageSet()
    data.AddSlice(self._KEY_MARK, 1, 1, {'precise': True, 'forced': False})
    data.AddSlice(self._KEY_LAZY_SWEEP, 2, 2, {})
    data.AddSlice(self._KEY_LAZY_SWEEP, 7, 4, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 16, 6, {})
    data.AddSlice(self._KEY_MARK, 22, 7, {'precise': True, 'forced': False})
    data.AddSlice(self._KEY_LAZY_SWEEP, 29, 8, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 46, 10, {})
    data.AddSlice(self._KEY_MARK, 56, 11, {'precise': False, 'forced': False})
    data.AddSlice(self._KEY_LAZY_SWEEP, 67, 12, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 92, 14, {})
    data.AddSlice(self._KEY_MARK, 106, 15, {'precise': True, 'forced': False})
    data.AddSlice(self._KEY_LAZY_SWEEP, 121, 16, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 154, 18, {})
    data.AddSlice(self._KEY_MARK, 172, 19, {'precise': False, 'forced': True})
    data.AddSlice(self._KEY_LAZY_SWEEP, 211, 21, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 232, 22, {})
    data.AddSlice(self._KEY_MARK, 254, 23, {'precise': True, 'forced': True})
    data.AddSlice(self._KEY_LAZY_SWEEP, 301, 25, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 326, 26, {})
    return data

  def _GenerateDataForParsing(self):
    data = self._GenerateDataForEmptyPageSet()
    data.AddSlice(self._KEY_MARK, 1, 1,
                  {'lazySweeping': True, 'gcReason': 'ConservativeGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 2, 2, {})
    data.AddSlice(self._KEY_LAZY_SWEEP, 7, 4, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 16, 6, {})
    data.AddSlice(self._KEY_MARK, 22, 7,
                  {'lazySweeping': True, 'gcReason': 'PreciseGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 29, 8, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 46, 10, {})
    data.AddSlice(self._KEY_MARK, 56, 11,
                  {'lazySweeping': False, 'gcReason': 'ConservativeGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 67, 12, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 92, 14, {})
    data.AddSlice(self._KEY_MARK, 106, 15,
                  {'lazySweeping': False, 'gcReason': 'PreciseGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 121, 16, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 154, 18, {})
    data.AddSlice(self._KEY_MARK, 172, 19,
                  {'lazySweeping': False, 'gcReason': 'ForcedGCForTesting'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 211, 21, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 232, 22, {})
    data.AddSlice(self._KEY_MARK, 254, 23,
                  {'lazySweeping': False, 'gcReason': 'IdleGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 301, 25, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 326, 26, {})

    # Following events are covered with 'BlinkGCTimeMeasurement' event.
    first_measure = data.AddSlice(self._KEY_MARK, 352, 27,
        {'lazySweeping': False, 'gcReason': 'ConservativeGC'})
    data.AddSlice(self._KEY_MARK, 380, 28,
                  {'lazySweeping': True, 'gcReason': 'ConservativeGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 408, 29, {})
    data.AddSlice(self._KEY_LAZY_SWEEP, 437, 30, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 467, 31, {})
    data.AddSlice(self._KEY_MARK, 498, 32,
                  {'lazySweeping': True, 'gcReason': 'PreciseGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 530, 33, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 563, 34, {})
    data.AddSlice(self._KEY_MARK, 597, 35,
                  {'lazySweeping': False, 'gcReason': 'ConservativeGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 632, 36, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 667, 37, {})
    data.AddSlice(self._KEY_MARK, 704, 38,
                  {'lazySweeping': False, 'gcReason': 'PreciseGC'})
    data.AddSlice(self._KEY_LAZY_SWEEP, 742, 39, {})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 781, 40, {})
    data.AddSlice(self._KEY_MARK, 821, 41,
                  {'lazySweeping': False, 'gcReason': 'ForcedGCForTesting'})
    data.AddSlice(self._KEY_COMPLETE_SWEEP, 862, 42, {})
    data.AddSlice(self._KEY_MARK, 904, 43,
                  {'lazySweeping': False, 'gcReason': 'IdleGC'})
    last_measure = data.AddSlice(self._KEY_COMPLETE_SWEEP, 947, 44, {})

    # Async event
    async_dur = last_measure.end - first_measure.start
    data.AddAsyncSlice(self._KEY_MEASURE, first_measure.start, async_dur, {})

    return data
