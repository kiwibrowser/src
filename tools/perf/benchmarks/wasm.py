# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

import page_sets

from telemetry import benchmark
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement


@benchmark.Owner(emails=['bradnelson@chromium.org'])
class Wasm(perf_benchmark.PerfBenchmark):
  """Runtime Stats benchmark for real world wasm apps."""

  options = {'pageset_repeat': 3}

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
      '--enable-blink-features=BlinkRuntimeCallStats')

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter()

    # "blink.console" is used for marking ranges in
    # cache_temperature.MarkTelemetryInternal.
    cat_filter.AddIncludedCategory('blink.console')

    # "navigation" and "blink.user_timing" are needed to capture core
    # navigation events.
    cat_filter.AddIncludedCategory('navigation')
    cat_filter.AddIncludedCategory('blink.user_timing')

    # "loading" is needed for first-meaningful-paint computation.
    cat_filter.AddIncludedCategory('loading')

    # "toplevel" category is used to capture TaskQueueManager events
    # necessary to compute time-to-interactive.
    cat_filter.AddIncludedCategory('toplevel')

    # V8 needed categories
    cat_filter.AddIncludedCategory('v8')
    cat_filter.AddDisabledByDefault('disabled-by-default-v8.runtime_stats')

    tbm_options = timeline_based_measurement.Options(
        overhead_level=cat_filter)
    tbm_options.SetTimelineBasedMetrics(['runtimeStatsMetric'])
    return tbm_options

  @classmethod
  def Name(cls):
    return 'wasm'

  def CreateStorySet(self, options):
    return page_sets.WasmRealWorldPagesStorySet()
