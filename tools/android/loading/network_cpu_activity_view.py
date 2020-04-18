#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Graphs the CPU and network activity during a load."""

import numpy as np
import matplotlib
from matplotlib import pylab as plt
import sys

import activity_lens
import loading_trace
import network_activity_lens


def _CpuActivityTimeline(cpu_lens, start_msec, end_msec, granularity):
  cpu_timestamps = np.arange(start_msec, end_msec, granularity)
  busy_percentage = []
  print len(cpu_timestamps)
  for i in range(len(cpu_timestamps) - 1):
    (start, end) = (cpu_timestamps[i], cpu_timestamps[i + 1])
    duration = end - start
    busy_ms = cpu_lens.MainRendererThreadBusyness(start, end)
    busy_percentage.append(100 * busy_ms / float(duration))
  return (cpu_timestamps[:-1], np.array(busy_percentage))


def GraphTimelines(trace):
  """Creates a figure of Network and CPU activity for a trace.

  Args:
    trace: (LoadingTrace)

  Returns:
    A matplotlib.pylab.figure.
  """
  cpu_lens = activity_lens.ActivityLens(trace)
  network_lens = network_activity_lens.NetworkActivityLens(trace)
  matplotlib.rc('font', size=14)
  figure, (network, cpu) = plt.subplots(2, sharex = True, figsize=(14, 10))
  figure.suptitle('Network and CPU Activity - %s' % trace.url)
  upload_timeline = network_lens.uploaded_bytes_timeline
  download_timeline = network_lens.downloaded_bytes_timeline
  start_time = upload_timeline[0][0]
  end_time = upload_timeline[0][-1]
  times = np.array(upload_timeline[0]) - start_time
  network.step(times, np.cumsum(download_timeline[1]) / 1e6, label='Download')
  network.step(times, np.cumsum(upload_timeline[1]) / 1e6, label='Upload')
  network.legend(loc='lower right')
  network.set_xlabel('Time (ms)')
  network.set_ylabel('Total Data Transferred (MB)')

  (cpu_timestamps, cpu_busyness) = _CpuActivityTimeline(
      cpu_lens, start_time, end_time, 100)
  cpu.step(cpu_timestamps - start_time, cpu_busyness)
  cpu.set_ylim(ymin=0, ymax=100)
  cpu.set_xlabel('Time (ms)')
  cpu.set_ylabel('Main Renderer Thread Busyness (%)')
  return figure


def main():
  filename = sys.argv[1]
  trace = loading_trace.LoadingTrace.FromJsonFile(filename)
  figure = GraphTimelines(trace, filename + '.pdf')
  output_filename = filename + '.pdf'
  figure.savefig(output_filename, dpi=300)


if __name__ == '__main__':
  main()
