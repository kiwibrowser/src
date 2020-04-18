# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.page import legacy_page_test
from telemetry.timeline.model import TimelineModel
from telemetry.timeline import tracing_config
from telemetry.value import list_of_scalar_values
from telemetry.value import scalar


_CR_RENDERER_MAIN = 'CrRendererMain'
_RUN_SMOOTH_ACTIONS = 'RunSmoothAllActions'


def _AddTracingResults(thread, results):
  _GC_REASONS = ['precise', 'conservative', 'idle', 'forced']
  _GC_STAGES = ['mark', 'lazy_sweep', 'complete_sweep']

  def GetGcReason(event, async_slices):
    args = event.args

    # Old format
    if 'precise' in args:
      if args['forced']:
        return 'forced'
      return 'precise' if args['precise'] else 'conservative'
    if args['gcReason'] == 'ConservativeGC':
      return 'conservative'
    if args['gcReason'] == 'PreciseGC':
      return 'precise'
    if args['gcReason'] == 'ForcedGCForTesting':
      for s in async_slices:
        if s.start <= event.start and event.end <= s.end:
          return 'forced'
      # Ignore this forced GC being out of target ranges
      return None
    if args['gcReason'] == 'IdleGC':
      return 'idle'
    return None  # Unknown

  def DumpMetric(page, name, values, unit, results):
    if values[name]:
      results.AddValue(list_of_scalar_values.ListOfScalarValues(
          page, name, unit, values[name]))
      results.AddValue(scalar.ScalarValue(
          page, name + '_max', unit, max(values[name])))
      results.AddValue(scalar.ScalarValue(
          page, name + '_total', unit, sum(values[name])))

  events = thread.all_slices
  async_slices = [s for s in thread.async_slices
                  if s.name == 'BlinkGCTimeMeasurement']

  # Prepare
  values = {}
  for reason in _GC_REASONS:
    for stage in _GC_STAGES:
      values['oilpan_%s_%s' % (reason, stage)] = []

  # Parse trace events
  reason = None
  mark_time = 0
  lazy_sweep_time = 0
  complete_sweep_time = 0
  for event in events:
    duration = event.thread_duration or event.duration
    if event.name == 'BlinkGC.AtomicPhaseMarking':
      if reason is not None:
        values['oilpan_%s_mark' % reason].append(mark_time)
        values['oilpan_%s_lazy_sweep' % reason].append(lazy_sweep_time)
        values['oilpan_%s_complete_sweep' % reason].append(complete_sweep_time)

      reason = GetGcReason(event, async_slices)
      mark_time = duration
      lazy_sweep_time = 0
      complete_sweep_time = 0
      continue
    if event.name == 'BlinkGC.LazySweepInIdle':
      lazy_sweep_time += duration
      continue
    if event.name == 'BlinkGC.CompleteSweep':
      complete_sweep_time += duration
      continue

  if reason is not None:
    values['oilpan_%s_mark' % reason].append(mark_time)
    values['oilpan_%s_lazy_sweep' % reason].append(lazy_sweep_time)
    values['oilpan_%s_complete_sweep' % reason].append(complete_sweep_time)

  page = results.current_page
  unit = 'ms'

  # Dump each metric
  for reason in _GC_REASONS:
    for stage in _GC_STAGES:
      DumpMetric(page, 'oilpan_%s_%s' % (reason, stage), values, unit, results)

  # Summarize each stage
  for stage in _GC_STAGES:
    total_time = 0
    for reason in _GC_REASONS:
      total_time += sum(values['oilpan_%s_%s' % (reason, stage)])
    results.AddValue(
        scalar.ScalarValue(page, 'oilpan_%s' % stage, unit, total_time))

  # Summarize sweeping time
  total_sweep_time = 0
  for stage in ['lazy_sweep', 'complete_sweep']:
    sweep_time = 0
    for reason in _GC_REASONS:
      sweep_time += sum(values['oilpan_%s_%s' % (reason, stage)])
    key = 'oilpan_%s' % stage
    results.AddValue(scalar.ScalarValue(page, key, unit, sweep_time))
    total_sweep_time += sweep_time
  results.AddValue(
      scalar.ScalarValue(page, 'oilpan_sweep', unit, total_sweep_time))

  gc_time = 0
  for key in values:
    gc_time += sum(values[key])
  results.AddValue(scalar.ScalarValue(page, 'oilpan_gc', unit, gc_time))


class _OilpanGCTimesBase(legacy_page_test.LegacyPageTest):

  def __init__(self, action_name=''):
    super(_OilpanGCTimesBase, self).__init__(action_name)

  def WillNavigateToPage(self, page, tab):
    del page  # unused
    # FIXME: Remove webkit.console when blink.console lands in chromium and
    # the ref builds are updated. crbug.com/386847
    config = tracing_config.TracingConfig()
    for c in ['webkit.console', 'blink.console', 'blink_gc']:
      config.chrome_trace_config.category_filter.AddIncludedCategory(c)
    config.enable_chrome_trace = True
    tab.browser.platform.tracing_controller.StartTracing(config, timeout=1000)

  def ValidateAndMeasurePage(self, page, tab, results):
    del page  # unused
    timeline_data = tab.browser.platform.tracing_controller.StopTracing()[0]
    timeline_model = TimelineModel(timeline_data)
    threads = timeline_model.GetAllThreads()
    for thread in threads:
      if thread.name == _CR_RENDERER_MAIN:
        _AddTracingResults(thread, results)

  def DidRunPage(self, platform):
    if platform.tracing_controller.is_tracing_running:
      platform.tracing_controller.StopTracing()


class OilpanGCTimesForSmoothness(_OilpanGCTimesBase):

  def __init__(self):
    super(OilpanGCTimesForSmoothness, self).__init__()
    self._interaction = None

  def DidNavigateToPage(self, page, tab):
    del page  # unused
    self._interaction = tab.action_runner.CreateInteraction(_RUN_SMOOTH_ACTIONS)
    self._interaction.Begin()

  def ValidateAndMeasurePage(self, page, tab, results):
    self._interaction.End()
    super(OilpanGCTimesForSmoothness, self).ValidateAndMeasurePage(
        page, tab, results)


class OilpanGCTimesForBlinkPerf(_OilpanGCTimesBase):

  def __init__(self):
    super(OilpanGCTimesForBlinkPerf, self).__init__()
    with open(os.path.join(os.path.dirname(__file__), '..', '..', 'benchmarks',
                           'blink_perf.js'), 'r') as f:
      self._blink_perf_js = f.read()

  def WillNavigateToPage(self, page, tab):
    page.script_to_evaluate_on_commit = self._blink_perf_js
    super(OilpanGCTimesForBlinkPerf, self).WillNavigateToPage(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    tab.WaitForJavaScriptCondition('testRunner.isDone', timeout=600)
    super(OilpanGCTimesForBlinkPerf, self).ValidateAndMeasurePage(
        page, tab, results)


class OilpanGCTimesForInternals(OilpanGCTimesForBlinkPerf):

  def __init__(self):
    super(OilpanGCTimesForInternals, self).__init__()

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    # 'expose-internals-for-testing' can be enabled on content shell.
    assert 'content-shell' in options.browser_type
    options.AppendExtraBrowserArgs(['--expose-internals-for-testing',
                                    '--js-flags=--expose-gc'])
