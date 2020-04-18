# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from contrib.cluster_telemetry import ct_benchmarks_util
from contrib.cluster_telemetry import page_set

from core import perf_benchmark
from measurements import smoothness

def ScrollToEndOfPage(action_runner):
  action_runner.Wait(1)
  with action_runner.CreateGestureInteraction('ScrollAction'):
    action_runner.ScrollPage()


class SmoothnessCT(perf_benchmark.PerfBenchmark):
  """Measures smoothness performance for Cluster Telemetry."""

  options = {'upload_results': True}

  test = smoothness.Smoothness

  @classmethod
  def Name(cls):
    return 'smoothness_ct'

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    ct_benchmarks_util.AddBenchmarkCommandLineArgs(parser)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    ct_benchmarks_util.ValidateCommandLineArgs(parser, args)

  def CreateStorySet(self, options):
    return page_set.CTPageSet(
        options.urls_list, options.user_agent, options.archive_data_file,
        run_page_interaction_callback=ScrollToEndOfPage)
