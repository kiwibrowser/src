# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from benchmarks import media_router_dialog_metric
from benchmarks import media_router_cpu_memory_metric
from telemetry.page import legacy_page_test


class MediaRouterDialogTest(legacy_page_test.LegacyPageTest):
  """Performs a measurement of Media Route dialog latency."""

  def __init__(self):
    super(MediaRouterDialogTest, self).__init__()
    self._metric = media_router_dialog_metric.MediaRouterDialogMetric()

  def DidNavigateToPage(self, page, tab):
    self._metric.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    self._metric.Stop(page, tab)
    self._metric.AddResults(tab, results)


class MediaRouterCPUMemoryTest(legacy_page_test.LegacyPageTest):
  """Performs a measurement of Media Route CPU/memory usage."""

  def __init__(self):
    super(MediaRouterCPUMemoryTest, self).__init__()
    self._metric = media_router_cpu_memory_metric.MediaRouterCPUMemoryMetric()

  def ValidateAndMeasurePage(self, page, tab, results):
    self._metric.AddResults(tab, results)
