# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from common import chrome_proxy_metrics
from common import network_metrics
from common.chrome_proxy_metrics import ChromeProxyMetricException
from telemetry.page import legacy_page_test
from telemetry.value import scalar


class ChromeProxyMetric(network_metrics.NetworkMetric):
  """A Chrome proxy timeline metric."""

  def __init__(self):
    super(ChromeProxyMetric, self).__init__()
    self.compute_data_saving = True

  def SetEvents(self, events):
    """Used for unittest."""
    self._events = events

  def ResponseFromEvent(self, event):
    return chrome_proxy_metrics.ChromeProxyResponse(event)

  def AddResults(self, tab, results):
    raise NotImplementedError

  def AddResultsForDataSaving(self, tab, results):
    resources_via_proxy = 0
    resources_from_cache = 0
    resources_direct = 0

    super(ChromeProxyMetric, self).AddResults(tab, results)
    for resp in self.IterResponses(tab):
      if resp.response.served_from_cache:
        resources_from_cache += 1
      if resp.HasChromeProxyViaHeader():
        resources_via_proxy += 1
      else:
        resources_direct += 1

    if resources_from_cache + resources_via_proxy + resources_direct == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response, but zero responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_via_proxy', 'count',
        resources_via_proxy))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_from_cache', 'count',
        resources_from_cache))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'resources_direct', 'count', resources_direct))

  def AddResultsForLatency(self, tab, results):
    # TODO(bustamante): This is a hack to workaround crbug.com/467174,
    #   once fixed just pull down window.performance.timing object and
    #   reference that everywhere.
    load_event_start = tab.EvaluateJavaScript(
        'window.performance.timing.loadEventStart')
    navigation_start = tab.EvaluateJavaScript(
        'window.performance.timing.navigationStart')
    dom_content_loaded_event_start = tab.EvaluateJavaScript(
        'window.performance.timing.domContentLoadedEventStart')
    fetch_start = tab.EvaluateJavaScript(
        'window.performance.timing.fetchStart')
    request_start = tab.EvaluateJavaScript(
        'window.performance.timing.requestStart')
    domain_lookup_end = tab.EvaluateJavaScript(
        'window.performance.timing.domainLookupEnd')
    domain_lookup_start = tab.EvaluateJavaScript(
        'window.performance.timing.domainLookupStart')
    connect_end = tab.EvaluateJavaScript(
        'window.performance.timing.connectEnd')
    connect_start = tab.EvaluateJavaScript(
        'window.performance.timing.connectStart')
    response_end = tab.EvaluateJavaScript(
        'window.performance.timing.responseEnd')
    response_start = tab.EvaluateJavaScript(
        'window.performance.timing.responseStart')

    # NavigationStart relative markers in milliseconds.
    load_start = (float(load_event_start) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'load_start', 'ms', load_start))

    dom_content_loaded_start = (
      float(dom_content_loaded_event_start) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'dom_content_loaded_start', 'ms',
        dom_content_loaded_start))

    fetch_start = (float(fetch_start) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'fetch_start', 'ms', fetch_start,
        important=False))

    request_start = (float(request_start) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'request_start', 'ms', request_start,
        important=False))

    response_start = (float(response_start) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'response_start', 'ms', response_start,
        important=False))

    response_end = (float(response_end) - navigation_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'response_end', 'ms', response_end,
        important=False))

    # Phase measurements in milliseconds.
    domain_lookup_duration = (float(domain_lookup_end) - domain_lookup_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'domain_lookup_duration', 'ms',
        domain_lookup_duration, important=False))

    connect_duration = (float(connect_end) - connect_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'connect_duration', 'ms', connect_duration,
        important=False))

    request_duration = (float(response_start) - request_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'request_duration', 'ms', request_duration,
        important=False))

    response_duration = (float(response_end) - response_start)
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'response_duration', 'ms', response_duration,
        important=False))
