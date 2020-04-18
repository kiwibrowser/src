# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from metrics import network
from metrics import power
from telemetry.core import util
from telemetry.page import legacy_page_test


class Power(legacy_page_test.LegacyPageTest):
  """Measures power draw and idle wakeups during the page's interactions."""

  def __init__(self):
    super(Power, self).__init__()
    self._power_metric = None
    self._network_metric = None

  def WillStartBrowser(self, platform):
    self._power_metric = power.PowerMetric(platform)
    self._network_metric = network.NetworkMetric(platform)

  def WillNavigateToPage(self, page, tab):
    self._network_metric.Start(page, tab)

  def DidNavigateToPage(self, page, tab):
    self._power_metric.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    self._network_metric.Stop(page, tab)
    self._power_metric.Stop(page, tab)
    self._network_metric.AddResults(tab, results)
    self._power_metric.AddResults(tab, results)

  def DidRunPage(self, platform):
    del platform  # unused
    self._power_metric.Close()


class LoadPower(Power):

  def WillNavigateToPage(self, page, tab):
    self._network_metric.Start(page, tab)
    self._power_metric.Start(page, tab)

  def DidNavigateToPage(self, page, tab):
    pass


class QuiescentPower(legacy_page_test.LegacyPageTest):
  """Measures power draw and idle wakeups after the page finished loading."""

  # Amount of time to measure, in seconds.
  SAMPLE_TIME = 30

  def ValidateAndMeasurePage(self, page, tab, results):
    if not tab.browser.platform.CanMonitorPower():
      return

    util.WaitFor(tab.HasReachedQuiescence, 60)

    metric = power.PowerMetric(tab.browser.platform)
    metric.Start(page, tab)

    time.sleep(QuiescentPower.SAMPLE_TIME)

    metric.Stop(page, tab)
    metric.AddResults(tab, results)
