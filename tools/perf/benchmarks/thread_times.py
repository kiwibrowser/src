# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark

from benchmarks import silk_flags
from measurements import thread_times
import page_sets
from telemetry import benchmark
from telemetry import story


class _ThreadTimes(perf_benchmark.PerfBenchmark):

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    parser.add_option('--report-silk-details', action='store_true',
                      help='Report details relevant to silk.')

  @classmethod
  def Name(cls):
    return 'thread_times'

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del from_first_story_run  # unused
    # Default to only reporting per-frame metrics.
    return 'per_second' not in name

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForThreadTimes(options)

  def CreatePageTest(self, options):
    return thread_times.ThreadTimes(options.report_silk_details)


@benchmark.Owner(emails=['vmiura@chromium.org'])
class ThreadTimesKeySilkCases(_ThreadTimes):
  """Measures timeline metrics while performing smoothness action on key silk
  cases."""
  page_set = page_sets.KeySilkCasesPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'thread_times.key_silk_cases'

@benchmark.Owner(emails=['vmiura@chromium.org', 'sadrul@chromium.org'])
class ThreadTimesKeyHitTestCases(_ThreadTimes):
  """Measure timeline metrics while performing smoothness action on key hit
  testing cases."""
  page_set = page_sets.KeyHitTestCasesPageSet
  SUPPORTED_PLATFORMS = [
      story.expectations.ALL_ANDROID, story.expectations.ALL_LINUX
  ]

  @classmethod
  def Name(cls):
    return 'thread_times.key_hit_test_cases'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class ThreadTimesKeyMobileSitesSmooth(_ThreadTimes):
  """Measures timeline metrics while performing smoothness action on
  key mobile sites labeled with fast-path tag.
  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  page_set = page_sets.KeyMobileSitesSmoothPageSet
  options = {'story_tag_filter': 'fastpath'}
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'thread_times.key_mobile_sites_smooth'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class ThreadTimesSimpleMobileSites(_ThreadTimes):
  """Measures timeline metric using smoothness action on simple mobile sites
  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  page_set = page_sets.SimpleMobileSitesPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'thread_times.simple_mobile_sites'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class ThreadTimesCompositorCases(_ThreadTimes):
  """Measures timeline metrics while performing smoothness action on
  tough compositor cases, using software rasterization.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  page_set = page_sets.ToughCompositorCasesPageSet

  def SetExtraBrowserOptions(self, options):
    super(ThreadTimesCompositorCases, self).SetExtraBrowserOptions(options)
    silk_flags.CustomizeBrowserOptionsForSoftwareRasterization(options)

  @classmethod
  def Name(cls):
    return 'thread_times.tough_compositor_cases'


@benchmark.Owner(emails=['skyostil@chromium.org'])
class ThreadTimesKeyIdlePowerCases(_ThreadTimes):
  """Measures timeline metrics for sites that should be idle in foreground
  and background scenarios. The metrics are per-second rather than per-frame."""
  page_set = page_sets.KeyIdlePowerCasesPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'thread_times.key_idle_power_cases'

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del from_first_story_run  # unused
    # Only report per-second metrics.
    return 'per_frame' not in name and 'mean_frame' not in name


@benchmark.Owner(emails=['vmiura@chromium.org', 'sadrul@chromium.org'])
class ThreadTimesKeyNoOpCases(_ThreadTimes):
  """Measures timeline metrics for common interactions and behaviors that should
  have minimal cost. The metrics are per-second rather than per-frame."""
  page_set = page_sets.KeyNoOpCasesPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'thread_times.key_noop_cases'

  @classmethod
  def ShouldAddValue(cls, name, from_first_story_run):
    del from_first_story_run  # unused
    # Only report per-second metrics.
    return 'per_frame' not in name and 'mean_frame' not in name


@benchmark.Owner(emails=['bokan@chromium.org', 'nzolghadr@chromium.org'])
class ThreadTimesToughScrollingCases(_ThreadTimes):
  """Measure timeline metrics while performing smoothness action on tough
  scrolling cases."""
  page_set = page_sets.ToughScrollingCasesPageSet

  @classmethod
  def Name(cls):
    return 'thread_times.tough_scrolling_cases'
