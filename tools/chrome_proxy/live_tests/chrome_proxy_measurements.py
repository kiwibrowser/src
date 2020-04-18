# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import chrome_proxy_metrics as metrics
from common import chrome_proxy_measurements as measurements
from telemetry.core import exceptions
from telemetry.page import legacy_page_test

class ChromeProxyLatencyBase(legacy_page_test.LegacyPageTest):
  """Chrome latency measurement."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyLatencyBase, self).__init__(*args, **kwargs)
    self._metrics = metrics.ChromeProxyMetric()

  def WillNavigateToPage(self, page, tab):
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    self._metrics.Stop(page, tab)
    self._metrics.AddResultsForLatency(tab, results)


class ChromeProxyLatency(ChromeProxyLatencyBase):
  """Chrome proxy latency measurement."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyLatency, self).__init__(*args, **kwargs)

  def CustomizeBrowserOptions(self, options):
    # NOTE: When using the Data Saver API, the first few requests for this test
    # could go over direct instead of through the Data Reduction Proxy if the
    # Data Saver API fetch is slow to finish. This test can't just use
    # measurements.WaitForViaHeader(tab) since that would affect the results of
    # the latency measurement, e.g. Chrome would have a hot proxy connection.
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')


class ChromeProxyLatencyDirect(ChromeProxyLatencyBase):
  """Direct connection latency measurement."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyLatencyDirect, self).__init__(*args, **kwargs)


class ChromeProxyDataSavingBase(legacy_page_test.LegacyPageTest):
  """Chrome data saving measurement."""
  def __init__(self, *args, **kwargs):
    super(ChromeProxyDataSavingBase, self).__init__(*args, **kwargs)
    self._metrics = metrics.ChromeProxyMetric()

  def WillNavigateToPage(self, page, tab):
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    self._metrics.Stop(page, tab)
    self._metrics.AddResultsForDataSaving(tab, results)


class ChromeProxyDataSaving(ChromeProxyDataSavingBase):
  """Chrome proxy data saving measurement."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyDataSaving, self).__init__(*args, **kwargs)

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')

  def WillNavigateToPage(self, page, tab):
    measurements.WaitForViaHeader(tab)
    super(ChromeProxyDataSaving, self).WillNavigateToPage(page, tab)


class ChromeProxyDataSavingDirect(ChromeProxyDataSavingBase):
  """Direct connection data saving measurement."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyDataSavingDirect, self).__init__(*args, **kwargs)
