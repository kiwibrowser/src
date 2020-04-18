# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark
import page_sets

from telemetry import benchmark
from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.web_perf import timeline_based_measurement
from telemetry.web_perf.metrics import startup


class _StartupPerfBenchmark(perf_benchmark.PerfBenchmark):
  """Measures time to start Chrome."""

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs([
        '--enable-stats-collection-bindings'
    ])

  def CreateCoreTimelineBasedMeasurementOptions(self):
    startup_category_filter = (
        chrome_trace_category_filter.ChromeTraceCategoryFilter(
            filter_string='startup,blink.user_timing'))
    options = timeline_based_measurement.Options(
        overhead_level=startup_category_filter)
    options.SetLegacyTimelineBasedMetrics(
        [startup.StartupTimelineMetric()])
    return options


@benchmark.Owner(emails=['pasko@chromium.org',
                         'chrome-android-perf-status@chromium.org'])
class StartWithUrlColdTBM(_StartupPerfBenchmark):
  """Measures time to start Chrome cold with startup URLs."""

  page_set = page_sets.StartupPagesPageSet
  options = {'pageset_repeat': 5}
  SUPPORTED_PLATFORMS = [story.expectations.ANDROID_NOT_WEBVIEW]

  def SetExtraBrowserOptions(self, options):
    options.clear_sytem_cache_for_browser_and_profile_on_start = True
    super(StartWithUrlColdTBM, self).SetExtraBrowserOptions(options)

  @classmethod
  def Name(cls):
    return 'start_with_url.cold.startup_pages'


@benchmark.Owner(emails=['pasko@chromium.org',
                         'chrome-android-perf-status@chromium.org'])
class StartWithUrlWarmTBM(_StartupPerfBenchmark):
  """Measures stimetime to start Chrome warm with startup URLs."""

  page_set = page_sets.StartupPagesPageSet
  options = {'pageset_repeat': 11}
  SUPPORTED_PLATFORMS = [story.expectations.ANDROID_NOT_WEBVIEW]

  @classmethod
  def Name(cls):
    return 'start_with_url.warm.startup_pages'

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del name  # unused
    # Ignores first results because the first invocation is actualy cold since
    # we are loading the profile for the first time.
    return not from_first_story_run


@benchmark.Owner(emails=['pasko@chromium.org',
                         'chrome-android-perf-status@chromium.org'])
class ExperimentalStartWithUrlCold(perf_benchmark.PerfBenchmark):
  """Measures time to start Chrome cold with startup URLs (TBMv2 version)."""
  # TODO(pasko): also add the .warm version of the TBMv2 benchmark after the
  # cold one graduates from experimental.

  page_set = page_sets.ExperimentalStartupPagesPageSet
  options = {'pageset_repeat': 11}
  SUPPORTED_PLATFORMS = [story.expectations.ANDROID_NOT_WEBVIEW]

  @classmethod
  def Name(cls):
    # TODO(pasko): Actually make it 'coldish', that is, purge the OS page cache
    # only for files of the Chrome application, its profile and disk caches.
    return 'experimental.startup.android.coldish'

  def SetExtraBrowserOptions(self, options):
    options.clear_sytem_cache_for_browser_and_profile_on_start = True
    super(ExperimentalStartWithUrlCold, self).SetExtraBrowserOptions(options)

  def CreateCoreTimelineBasedMeasurementOptions(self):
    startup_category_filter = (
        chrome_trace_category_filter.ChromeTraceCategoryFilter(
            filter_string=('navigation,loading,net,netlog,network,'
                'offline_pages,startup,toplevel,Java,EarlyJava')))
    options = timeline_based_measurement.Options(
        overhead_level=startup_category_filter)
    options.config.enable_chrome_trace = True
    options.SetTimelineBasedMetrics(['androidStartupMetric'])
    return options
