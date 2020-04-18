# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from telemetry.value import scalar

from metrics import Metric


class CpuMetric(Metric):
  """Calculates CPU load over a span of time."""

  def __init__(self, browser):
    super(CpuMetric, self).__init__()
    self._browser = browser
    self._start_cpu = None
    self._stop_cpu = None

  def DidStartBrowser(self, browser):
    # Save the browser object so that cpu_stats can be accessed later.
    self._browser = browser

  def Start(self, page, tab):
    if not self._browser.supports_cpu_metrics:
      logging.warning('CPU metrics not supported.')
      return

    self._start_cpu = self._browser.cpu_stats

  def Stop(self, page, tab):
    if not self._browser.supports_cpu_metrics:
      return

    assert self._start_cpu, 'Must call Start() first'
    self._stop_cpu = self._browser.cpu_stats

  # Optional argument trace_name is not in base class Metric.
  # pylint: disable=arguments-differ
  def AddResults(self, tab, results, trace_name='cpu_utilization'):
    if not self._browser.supports_cpu_metrics:
      return

    assert self._stop_cpu, 'Must call Stop() first'
    cpu_stats = _SubtractCpuStats(self._stop_cpu, self._start_cpu)

    # FIXME: Renderer process CPU times are impossible to compare correctly.
    # http://crbug.com/419786#c11
    if 'Renderer' in cpu_stats:
      del cpu_stats['Renderer']

    # Add a result for each process type.
    for process_type in cpu_stats:
      trace_name_for_process = '%s_%s' % (trace_name, process_type.lower())
      cpu_percent = 100 * cpu_stats[process_type]
      results.AddValue(scalar.ScalarValue(
          results.current_page, 'cpu_utilization.%s' % trace_name_for_process,
          '%', cpu_percent, important=False))


def _SubtractCpuStats(cpu_stats, start_cpu_stats):
  """Computes average cpu usage over a time period for different process types.

  Each of the two cpu_stats arguments is a dict with the following format:
      {'Browser': {'CpuProcessTime': ..., 'TotalTime': ...},
       'Renderer': {'CpuProcessTime': ..., 'TotalTime': ...}
       'Gpu': {'CpuProcessTime': ..., 'TotalTime': ...}}

  The 'CpuProcessTime' fields represent the number of seconds of CPU time
  spent in each process, and total time is the number of real seconds
  that have passed (this may be a Unix timestamp).

  Returns:
    A dict of process type names (Browser, Renderer, etc.) to ratios of cpu
    time used to total time elapsed.
  """
  cpu_usage = {}
  for process_type in cpu_stats:
    assert process_type in start_cpu_stats, 'Mismatching process types'
    # Skip any process_types that are empty.
    if (not cpu_stats[process_type]) or (not start_cpu_stats[process_type]):
      continue
    cpu_process_time = (cpu_stats[process_type]['CpuProcessTime'] -
                        start_cpu_stats[process_type]['CpuProcessTime'])
    total_time = (cpu_stats[process_type]['TotalTime'] -
                  start_cpu_stats[process_type]['TotalTime'])
    # Fix overflow for 32-bit jiffie counter, 64-bit counter will not overflow.
    # Linux kernel starts with a value close to an overflow, so correction is
    # necessary. Assume jiffie counter is at 100 Hz.
    if total_time < 0:
      total_time += 2 ** 32 / 100.
    # Assert that the arguments were given in the correct order.
    assert total_time > 0 and total_time < 2 ** 31 / 100., (
        'Expected total_time > 0, was: %d' % total_time)
    cpu_usage[process_type] = float(cpu_process_time) / total_time
  return cpu_usage
