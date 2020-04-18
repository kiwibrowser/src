# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

from measurements import smoothness
import page_sets
from telemetry import benchmark
from telemetry import story as story_module

@benchmark.Owner(emails=['chiniforooshan@chromium.org', 'sadrul@chromium.org'])
class CrosUiSmoothnessBenchmark(perf_benchmark.PerfBenchmark):
  """Measures ChromeOS UI smoothness."""
  test = smoothness.Smoothness
  page_set = page_sets.CrosUiCasesPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_CHROMEOS]

  @classmethod
  def Name(cls):
    return 'cros_ui_smoothness'
