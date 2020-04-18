# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from benchmarks import smoothness,thread_times
import page_sets
from telemetry import benchmark

# pylint: disable=protected-access

def CustomizeBrowserOptionsForOopRasterization(options):
  """Enables flags needed for out of process rasterization."""
  options.AppendExtraBrowserArgs('--force-gpu-rasterization')
  options.AppendExtraBrowserArgs('--enable-oop-rasterization')


@benchmark.Owner(emails=['enne@chromium.org'])
class SmoothnessOopRasterizationTop25(smoothness._Smoothness):
  """Measures rendering statistics for the top 25 with oop rasterization.
  """
  tag = 'oop_rasterization'
  page_set = page_sets.Top25SmoothPageSet

  def SetExtraBrowserOptions(self, options):
    CustomizeBrowserOptionsForOopRasterization(options)

  @classmethod
  def Name(cls):
    return 'smoothness.oop_rasterization.top_25_smooth'


@benchmark.Owner(emails=['enne@chromium.org'])
class ThreadTimesOopRasterKeyMobile(thread_times._ThreadTimes):
  """Measure timeline metrics for key mobile pages while using out of process
  raster."""
  tag = 'oop_rasterization'
  page_set = page_sets.KeyMobileSitesSmoothPageSet
  options = {'story_tag_filter': 'fastpath'}

  def SetExtraBrowserOptions(self, options):
    super(ThreadTimesOopRasterKeyMobile, self).SetExtraBrowserOptions(options)
    CustomizeBrowserOptionsForOopRasterization(options)

  @classmethod
  def Name(cls):
    return 'thread_times.oop_rasterization.key_mobile'
