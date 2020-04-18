# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

import page_sets

from core import perf_benchmark
from telemetry import benchmark
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement


@benchmark.Owner(emails=['ulan@chromium.org'])
class OortOnlineTBMv2(perf_benchmark.PerfBenchmark):
  """OortOnline benchmark that measures WebGL and V8 performance.
  URL: http://oortonline.gl/#run
  Info: http://v8project.blogspot.de/2015/10/jank-busters-part-one.html
  """

  # Report only V8-specific and overall renderer memory values. Note that
  # detailed values reported by the OS (such as native heap) are excluded.
  _V8_AND_OVERALL_MEMORY_RE = re.compile(
      r'renderer_processes:'
      r'(reported_by_chrome:v8|reported_by_os:system_memory:[^:]+$)')

  page_set = page_sets.OortOnlineTBMPageSet

  def CreateCoreTimelineBasedMeasurementOptions(self):
    categories = [
      # Implicitly disable all categories.
      '-*',
      # V8.
      'blink.console',
      'disabled-by-default-v8.gc',
      'renderer.scheduler',
      'v8',
      'webkit.console',
      # Smoothness.
      'benchmark',
      'blink',
      'blink.console',
      'trace_event_overhead',
      'webkit.console',
      # Memory.
      'blink.console',
      'disabled-by-default-memory-infra'
    ]
    category_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        ','.join(categories))
    options = timeline_based_measurement.Options(category_filter)
    options.SetTimelineBasedMetrics([
        'gcMetric', 'memoryMetric', 'responsivenessMetric'])
    # Setting an empty memory dump config disables periodic dumps.
    options.config.chrome_trace_config.SetMemoryDumpConfig(
        chrome_trace_config.MemoryDumpConfig())
    return options

  @classmethod
  def Name(cls):
    return 'oortonline_tbmv2'

  @classmethod
  def ShouldAddValue(cls, name, _):
    if 'memory:chrome' in name:
      return bool(cls._V8_AND_OVERALL_MEMORY_RE.search(name))
    if 'animation ' in name:
      return 'throughput' in name or 'frameTimeDiscrepancy' in name
    return 'v8' in name
