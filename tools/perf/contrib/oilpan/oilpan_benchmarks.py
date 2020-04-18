# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from core import perf_benchmark

from benchmarks import blink_perf
from benchmarks import silk_flags

from telemetry import benchmark
from telemetry import story

import page_sets

from contrib.oilpan import oilpan_gc_times


class OilpanGCTimesBlinkPerfStress(perf_benchmark.PerfBenchmark):
  tag = 'blink_perf_stress'
  test = oilpan_gc_times.OilpanGCTimesForInternals

  @classmethod
  def Name(cls):
    return 'oilpan_gc_times.blink_perf_stress'

  def CreateStorySet(self, options):
    path = os.path.join(blink_perf.BLINK_PERF_BASE_DIR, 'blink_gc')
    return blink_perf.CreateStorySetFromPath(path, blink_perf.SKIPPED_FILE)


@benchmark.Owner(emails=['peria@chromium.org'])
class OilpanGCTimesSmoothnessAnimation(perf_benchmark.PerfBenchmark):
  test = oilpan_gc_times.OilpanGCTimesForSmoothness
  page_set = page_sets.ToughAnimationCasesPageSet

  @classmethod
  def Name(cls):
    return 'oilpan_gc_times.tough_animation_cases'


class OilpanGCTimesKeySilkCases(perf_benchmark.PerfBenchmark):
  test = oilpan_gc_times.OilpanGCTimesForSmoothness
  page_set = page_sets.KeySilkCasesPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_ANDROID]

  @classmethod
  def Name(cls):
    return 'oilpan_gc_times.key_silk_cases'


class OilpanGCTimesSyncScrollKeyMobileSites(perf_benchmark.PerfBenchmark):
  tag = 'sync_scroll'
  test = oilpan_gc_times.OilpanGCTimesForSmoothness
  page_set = page_sets.KeyMobileSitesSmoothPageSet
  SUPPORTED_PLATFORMS = [story.expectations.ALL_ANDROID]

  def SetExtraBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForSyncScrolling(options)

  @classmethod
  def Name(cls):
    return 'oilpan_gc_times.sync_scroll.key_mobile_sites_smooth'
