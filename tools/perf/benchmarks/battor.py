# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement
import page_sets
from telemetry import benchmark
from telemetry import story


# TODO(rnephew): Remove BattOr naming from all benchmarks once the BattOr tests
# are the primary means of benchmarking power.
class _BattOrBenchmark(perf_benchmark.PerfBenchmark):

  def CreateCoreTimelineBasedMeasurementOptions(self):
    category_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='toplevel')
    options = timeline_based_measurement.Options(category_filter)
    options.config.chrome_trace_config.category_filter.AddFilterString('rail')
    options.config.enable_atrace_trace = True
    options.config.atrace_config.categories = ['sched']
    options.config.enable_battor_trace = True
    options.config.enable_chrome_trace = True
    options.config.enable_cpu_trace = True
    options.SetTimelineBasedMetrics(
        ['powerMetric', 'clockSyncLatencyMetric', 'cpuTimeMetric'])
    return options


@benchmark.Owner(emails=['charliea@chromium.org'])
class BattOrTrivialPages(_BattOrBenchmark):
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MAC]

  def CreateStorySet(self, options):
    # We want it to wait for 30 seconds to be comparable to legacy power tests.
    return page_sets.TrivialSitesStorySet(wait_in_seconds=30)

  @classmethod
  def Name(cls):
    return 'battor.trivial_pages'


@benchmark.Owner(emails=['charliea@chromium.org'])
class BattOrSteadyStatePages(_BattOrBenchmark):
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MAC]

  def CreateStorySet(self, options):
    # We want it to wait for 30 seconds to be comparable to legacy power tests.
    return page_sets.IdleAfterLoadingStories(wait_in_seconds=30)

  @classmethod
  def Name(cls):
    return 'battor.steady_state'
