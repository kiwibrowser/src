# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import time

from common import chrome_proxy_metrics
from common import network_metrics
from common.chrome_proxy_metrics import ChromeProxyMetricException
from telemetry.page import legacy_page_test
from telemetry.value import scalar
from telemetry.value import histogram_util
from metrics import Metric

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

  def AddResultsForHeaderValidation(self, tab, results):
    via_count = 0

    for resp in self.IterResponses(tab):
      if resp.IsValidByViaHeader():
        via_count += 1
      else:
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Via header (%s) is not valid (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))

    if via_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response through the proxy, but zero such '
          'responses were received.')
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'checked_via_header', 'count', via_count))

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

  def AddResultsForExtraViaHeader(self, tab, results, extra_via_header):
    extra_via_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        if resp.HasExtraViaHeader(extra_via_header):
          extra_via_count += 1
        else:
          raise ChromeProxyMetricException, (
              '%s: Should have via header %s.' % (resp.response.url,
                                                  extra_via_header))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'extra_via_header', 'count', extra_via_count))

  def GetClientTypeFromRequests(self, tab):
    """Get the Chrome-Proxy client type value from requests made in this tab.

    Returns:
        The client type value from the first request made in this tab that
        specifies a client type in the Chrome-Proxy request header. See
        ChromeProxyResponse.GetChromeProxyClientType for more details about the
        Chrome-Proxy client type. Returns None if none of the requests made in
        this tab specify a client type.
    """
    for resp in self.IterResponses(tab):
      client_type = resp.GetChromeProxyClientType()
      if client_type:
        return client_type
    return None

  def AddResultsForClientType(self, tab, results, client_type,
                              bypass_for_client_type):
    via_count = 0
    bypass_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        via_count += 1
        if client_type.lower() == bypass_for_client_type.lower():
          raise ChromeProxyMetricException, (
              '%s: Response for client of type "%s" has via header, but should '
              'be bypassed.' % (resp.response.url, bypass_for_client_type))
      elif resp.ShouldHaveChromeProxyViaHeader():
        bypass_count += 1
        if client_type.lower() != bypass_for_client_type.lower():
          raise ChromeProxyMetricException, (
              '%s: Response missing via header. Only "%s" clients should '
              'bypass for this page, but this client is "%s".' % (
                  resp.response.url, bypass_for_client_type, client_type))

    if via_count + bypass_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response that was eligible to be proxied, but '
          'zero such responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForLoFi(self, tab, results):
    lo_fi_request_count = 0
    lo_fi_response_count = 0

    for resp in self.IterResponses(tab):
      if 'favicon.ico' in resp.response.url:
        continue
      if not resp.response.url.endswith('png'):
        continue
      if not resp.response.request_headers:
        continue

      if resp.HasChromeProxyLoFiRequest():
        lo_fi_request_count += 1
      else:
        raise ChromeProxyMetricException, (
            '%s: LoFi not in request header.' % (resp.response.url))

      if resp.HasChromeProxyLoFiResponse():
        lo_fi_response_count += 1
      else:
        raise ChromeProxyMetricException, (
            '%s: LoFi not in response header.' % (resp.response.url))

      if resp.content_length > 100:
        raise ChromeProxyMetricException, (
            'Image %s is %d bytes. Expecting less than 100 bytes.' %
            (resp.response.url, resp.content_length))

    if lo_fi_request_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one LoFi request, but zero such requests were '
          'sent.')
    if lo_fi_response_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one LoFi response, but zero such responses were '
          'received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'lo_fi_request', 'count', lo_fi_request_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'lo_fi_response', 'count', lo_fi_response_count))
    super(ChromeProxyMetric, self).AddResults(tab, results)


  def AddResultsForLoFiCache(self, tab, results, is_lo_fi):
    request_count = 0
    response_count = 0

    for resp in self.IterResponses(tab):
      if not resp.response.url.endswith('png'):
        continue
      if not resp.response.request_headers:
        continue

      if is_lo_fi != resp.HasChromeProxyLoFiRequest():
        raise ChromeProxyMetricException, (
            '%s: LoFi %s expected in request header.' % (resp.response.url,
                    '' if is_lo_fi else 'not'))
      else:
        request_count += 1

      if is_lo_fi != resp.HasChromeProxyLoFiResponse():
        raise ChromeProxyMetricException, (
            '%s: LoFi %s expected in response header.' % (resp.response.url,
                    '' if is_lo_fi else 'not'))
      else:
        response_count += 1

      if is_lo_fi != (resp.content_length < 100):
        raise ChromeProxyMetricException, (
            'Image %s is %d bytes. Expecting %s than 100 bytes.' %
            (resp.response.url, resp.content_length,
             'less' if is_lo_fi else 'more'))

    if request_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one %s LoFi request, but zero such requests were '
          'sent.' % ('' if is_lo_fi else 'non'))
    if response_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one %s LoFi response, but zero such responses '
          'were received.' % ('' if is_lo_fi else 'non'))

    super(ChromeProxyMetric, self).AddResults(tab, results)

  def AddResultsForLitePage(self, tab, results):
    lite_page_request_count = 0
    lite_page_exp_request_count = 0
    lite_page_response_count = 0

    for resp in self.IterResponses(tab):
      if '/csi?' in resp.response.url:
        continue
      if 'favicon.ico' in resp.response.url:
        continue
      if resp.response.url.startswith('data:'):
        continue

      if resp.HasChromeProxyLitePageRequest():
        lite_page_request_count += 1

      if resp.HasChromeProxyLitePageExpRequest():
        lite_page_exp_request_count += 1

      if resp.HasChromeProxyLitePageResponse():
        lite_page_response_count += 1

      if resp.HasChromeProxyLoFiRequest():
        raise ChromeProxyMetricException, (
        '%s: Lo-Fi directive should not be in lite page request header.' %
        (resp.response.url))

    if lite_page_request_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one lite page request, but zero such requests were'
          ' sent.')
    if lite_page_exp_request_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one lite page exp=ignore_preview_blacklist request'
          ', but zero such requests were sent.')
    if lite_page_response_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one lite page response, but zero such responses '
          'were received.')

    results.AddValue(
        scalar.ScalarValue(
            results.current_page, 'lite_page_request',
            'count', lite_page_request_count))
    results.AddValue(
        scalar.ScalarValue(
            results.current_page, 'lite_page_exp_request',
            'count', lite_page_exp_request_count))
    results.AddValue(
        scalar.ScalarValue(
            results.current_page, 'lite_page_response',
            'count', lite_page_response_count))
    super(ChromeProxyMetric, self).AddResults(tab, results)

  def AddResultsForPassThrough(self, tab, results):
    compressed_count = 0
    compressed_size = 0
    pass_through_count = 0
    pass_through_size = 0

    for resp in self.IterResponses(tab):
      if 'favicon.ico' in resp.response.url:
        continue
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Should have Via header (%s) (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))
      if resp.HasChromeProxyPassThroughRequest():
        pass_through_count += 1
        pass_through_size = resp.content_length
      else:
        compressed_count += 1
        compressed_size = resp.content_length

    if pass_through_count != 1:
      raise ChromeProxyMetricException, (
          'Expected exactly one Chrome-Proxy-Accept-Transform identity request,'
          ' but %d such requests were sent.' % (pass_through_count))

    if compressed_count != 1:
      raise ChromeProxyMetricException, (
          'Expected exactly one compressed request, but %d such requests were '
          'received.' % (compressed_count))

    if compressed_size >= pass_through_size:
      raise ChromeProxyMetricException, (
          'Compressed image is %d bytes and identity image is %d. '
          'Expecting compressed image size to be less than identity '
          'image.' % (compressed_size, pass_through_size))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'compressed', 'count', compressed_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'compressed_size', 'bytes', compressed_size))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'pass_through', 'count', pass_through_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'pass_through_size', 'bytes', pass_through_size))

  def AddResultsForHTTPSBypass(self, tab, results):
    bypass_count = 0

    for resp in self.IterResponses(tab):
      # Only check https url's
      if "https://" not in resp.response.url:
        continue

      # If a Chrome Proxy Via appears fail the test
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Should not have Via header (%s) (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))
      bypass_count += 1

    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one https response was expected, but zero such '
          'responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForHTML5Test(self, tab, results):
    # Wait for the number of "points" of HTML5 compatibility to appear to verify
    # the HTML5 elements have loaded successfully.
    tab.WaitForJavaScriptCondition(
        'document.getElementsByClassName("pointsPanel")', timeout=15)

  def AddResultsForYouTube(self, tab, results):
    # Wait for the video to begin playing.
    tab.WaitForJavaScriptCondition(
        'window.playerState == YT.PlayerState.PLAYING', timeout=30)

  def AddResultsForBypass(self, tab, results, url_pattern=""):
    bypass_count = 0
    skipped_count = 0

    for resp in self.IterResponses(tab):
      # Only check the url's that contain the specified pattern.
      if url_pattern and url_pattern not in resp.response.url:
        skipped_count += 1
        continue

      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Should not have Via header (%s) (refer=%s, status=%d)' % (
                r.url, r.GetHeader('Via'), r.GetHeader('Referer'), r.status))
      bypass_count += 1

    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response to be bypassed, but zero such '
          'responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'skipped', 'count', skipped_count))

  def AddResultsForCorsBypass(self, tab, results):
    eligible_response_count = 0
    bypass_count = 0
    bypasses = {}
    for resp in self.IterResponses(tab):
      logging.warn('got a resource %s' % (resp.response.url))

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        eligible_response_count += 1
        if not resp.HasChromeProxyViaHeader():
          bypass_count += 1
        elif resp.response.status == 502:
          bypasses[resp.response.url] = 0

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        if not resp.HasChromeProxyViaHeader():
          if resp.response.status == 200:
            if (bypasses.has_key(resp.response.url)):
              bypasses[resp.response.url] = bypasses[resp.response.url] + 1

    for url in bypasses:
      if bypasses[url] == 0:
        raise ChromeProxyMetricException, (
            '%s: Got a 502 without a subsequent 200' % (url))
      elif bypasses[url] > 1:
        raise ChromeProxyMetricException, (
            '%s: Got a 502 and multiple 200s: %d' % (url, bypasses[url]))
    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'At least one response should be bypassed. '
          '(eligible_response_count=%d, bypass_count=%d)\n' % (
              eligible_response_count, bypass_count))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'cors_bypass', 'count', bypass_count))

  def AddResultsForBlockOnce(self, tab, results):
    eligible_response_count = 0
    via_proxy = 0
    visited_urls = []

    for resp in self.IterResponses(tab):
      # Add debug information in case of failure
      visited_urls.append(resp.response.url)

      # Block-once test URLs (Data Reduction Proxy always returns
      # block-once) should not have the Chrome-Compression-Proxy Via header.
      if (IsTestUrlForBlockOnce(resp.response.url)):
        eligible_response_count += 1
        if resp.HasChromeProxyViaHeader():
          raise ChromeProxyMetricException, (
              'Response has a Chrome-Compression-Proxy Via header: ' +
              resp.response.url)
      elif resp.ShouldHaveChromeProxyViaHeader():
        via_proxy += 1
        if not resp.HasChromeProxyViaHeader():
          # For all other URLs, confirm that via header is present if expected.
          raise ChromeProxyMetricException, (
              'Missing Chrome-Compression-Proxy Via header.' +
              resp.response.url)

    if via_proxy == 0:
      raise ChromeProxyMetricException, (
          'None of the requests went via data reduction proxy')

    if (eligible_response_count != 2):
      raise ChromeProxyMetricException, (
          'Did not make expected number of requests to whitelisted block-once'
          ' test URLs. Expected: 2, Actual: %s, Visited URLs: %s' %
          (eligible_response_count, visited_urls))

    results.AddValue(scalar.ScalarValue(results.current_page,
                                        'eligible_responses', 'count', 2))
    results.AddValue(scalar.ScalarValue(results.current_page,
                                        'via_proxy', 'count', via_proxy))

  def AddResultsForSafebrowsingOn(self, tab, results):
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'safebrowsing', 'timeout responses', 1))

  def AddResultsForSafebrowsingOff(self, tab, results):
    response_count = 0
    for resp in self.IterResponses(tab):
      # Data reduction proxy should return the real response for sites with
      # malware.
      response_count += 1
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            '%s: Safebrowsing feature should be off for desktop and webview.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))

    if response_count == 0:
      raise ChromeProxyMetricException, (
          'Safebrowsing test failed: No valid responses received')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'safebrowsing', 'responses', response_count))

  def AddResultsForHTTPFallback(self, tab, results):
    via_fallback_count = 0

    for resp in self.IterResponses(tab):
      if resp.ShouldHaveChromeProxyViaHeader():
        # All responses should have come through the HTTP fallback proxy, which
        # means that they should have the via header, and if a remote port is
        # defined, it should be port 80.
        if (not resp.HasChromeProxyViaHeader() or
            (resp.remote_port and resp.remote_port != 80)):
          r = resp.response
          raise ChromeProxyMetricException, (
              '%s: Should have come through the fallback proxy.\n'
              'Reponse: remote_port=%s status=(%d, %s)\nHeaders:\n %s' % (
                  r.url, str(resp.remote_port), r.status, r.status_text,
                  r.headers))
        via_fallback_count += 1

    if via_fallback_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response through the fallback proxy, but zero '
          'such responses were received.')
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via_fallback', 'count', via_fallback_count))

  def AddResultsForHTTPToDirectFallback(self, tab, results,
                                        fallback_response_host):
    via_fallback_count = 0
    bypass_count = 0
    responses = self.IterResponses(tab)

    # The first response(s) coming from fallback_response_host should be
    # through the HTTP fallback proxy.
    resp = next(responses, None)
    while resp and fallback_response_host in resp.response.url:
      if fallback_response_host in resp.response.url:
        if (not resp.HasChromeProxyViaHeader() or resp.remote_port != 80):
          r = resp.response
          raise ChromeProxyMetricException, (
              'Response for %s should have come through the fallback proxy.\n'
              'Response: remote_port=%s status=(%d, %s)\nHeaders:\n %s' % (
                  r.url, str(resp.remote_port), r.status, r.status_text,
                  r.headers))
        else:
          via_fallback_count += 1
      resp = next(responses, None)

    # All other responses should be bypassed.
    while resp:
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header.\n'
            'Response: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1
      resp = next(responses, None)

    # At least one response should go through the http proxy and be bypassed.
    if via_fallback_count == 0 or bypass_count == 0:
      raise ChromeProxyMetricException(
          'There should be at least one response through the fallback proxy '
          '(actual %s) and at least one bypassed response (actual %s)' %
          (via_fallback_count, bypass_count))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via_fallback', 'count', via_fallback_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForReenableAfterBypass(
      self, tab, results, bypass_seconds_min, bypass_seconds_max):
    """Verify results for a re-enable after bypass test.

    Args:
        tab: the tab for the test.
        results: the results object to add the results values to.
        bypass_seconds_min: the minimum duration of the bypass.
        bypass_seconds_max: the maximum duration of the bypass.
    """
    bypass_count = 0
    via_count = 0

    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1

    # Wait until 30 seconds before the bypass should expire, and fetch a page.
    # It should not have the via header because the proxy should still be
    # bypassed.
    time.sleep(bypass_seconds_min - 30)

    tab.ClearCache(force=True)
    before_metrics = ChromeProxyMetric()
    before_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=10)
    before_metrics.Stop(results.current_page, tab)

    for resp in before_metrics.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header; proxy should still '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1
    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response to be bypassed before the bypass '
          'expired, but zero such responses were received.')

    # Wait until 30 seconds after the bypass should expire, and fetch a page. It
    # should have the via header since the proxy should no longer be bypassed.
    time.sleep((bypass_seconds_max + 30) - (bypass_seconds_min - 30))

    tab.ClearCache(force=True)
    after_metrics = ChromeProxyMetric()
    after_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=10)
    after_metrics.Stop(results.current_page, tab)

    for resp in after_metrics.IterResponses(tab):
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should have via header; proxy should no longer '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        via_count += 1
    if via_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response through the proxy after the bypass '
          'expired, but zero such responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))

  def AddResultsForReenableAfterSetBypass(
      self, tab, results, bypass_seconds):
    """Verify results for a re-enable after bypass test.

    Args:
        tab: the tab for the test.
        results: the results object to add the results values to.
        bypass_seconds: the duration of the bypass
    """
    bypass_count = 0
    via_count = 0

    # Verify the bypass url was bypassed.
    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header.\n'
            'Reponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1

    # Navigate to a test page and verify it's being bypassed.
    tab.ClearCache(force=True)
    before_metrics = ChromeProxyMetric()
    before_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=10)
    before_metrics.Stop(results.current_page, tab)

    for resp in before_metrics.IterResponses(tab):
      if resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header; proxy should still '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        bypass_count += 1
    if bypass_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response to be bypassed before the bypass '
          'expired, but zero such responses were received.')

    # Wait for the bypass to expire, with the overhead of the previous steps
    # the bypass duration will have been exceeded after this delay.
    time.sleep(bypass_seconds)

    # Navigate to the test pass again and verify data saver is no longer
    # bypassed.
    tab.ClearCache(force=True)
    after_metrics = ChromeProxyMetric()
    after_metrics.Start(results.current_page, tab)
    tab.Navigate('http://chromeproxy-test.appspot.com/default')
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=10)
    after_metrics.Stop(results.current_page, tab)

    for resp in after_metrics.IterResponses(tab):
      if not resp.HasChromeProxyViaHeader():
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should have via header; proxy should no longer '
            'be bypassed.\nReponse: status=(%d, %s)\nHeaders:\n %s' % (
                r.url, r.status, r.status_text, r.headers))
      else:
        via_count += 1
    if via_count == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response through the proxy after the bypass '
          'expired, but zero such responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))

  def AddResultsForClientConfig(self, tab, results):
    resources_with_old_auth = 0
    resources_with_new_auth = 0

    super(ChromeProxyMetric, self).AddResults(tab, results)
    for resp in self.IterResponses(tab):
      if resp.GetChromeProxyRequestHeaderValue('s') != None:
        resources_with_new_auth += 1
      if resp.GetChromeProxyRequestHeaderValue('ps') != None:
        resources_with_old_auth += 1

    if resources_with_old_auth != 0:
      raise ChromeProxyMetricException, (
          'Expected zero responses with the old authentication scheme but '
          'received %d.' % resources_with_old_auth)

    if resources_with_new_auth == 0:
      raise ChromeProxyMetricException, (
          'Expected at least one response with the new authentication scheme, '
          'but zero such responses were received.')

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'new_auth', 'count', resources_with_new_auth))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'old_auth', 'count', resources_with_old_auth))

  def AddResultsForPingback(self, tab, results):
    # Force the pingback by loading a new page.
    tab.Navigate('http://check.googlezip.net/test.html')
    histogram_type = histogram_util.BROWSER_HISTOGRAM
    # This histogram should be synchronously created when the Navigate occurs.
    attempted = histogram_util.GetHistogramSum(
        histogram_type,
        'DataReductionProxy.Pingback.Attempted',
        tab)
    # Verify that a pingback URLFetcher was created.
    if attempted != 1:
      raise ChromeProxyMetricException, (
          'Expected one pingback attempt, but '
          'received %d.' % attempted)
    count = 0
    seconds_slept = 0
    # This test relies on the proxy server responding to the pingback after
    # receiving it. This should very likely take under 10 seconds for the
    # integration test.
    max_seconds_to_sleep = 10
    while count < 1 and seconds_slept < max_seconds_to_sleep:
      # This histogram will be created when the URLRequest either fails or
      # succeeds.
      count = histogram_util.GetHistogramCount(
        histogram_type,
        'DataReductionProxy.Pingback.Succeeded',
        tab)
      if count < 1:
        time.sleep(1)
        seconds_slept += 1

    # The pingback should always succeed. Successful pingbacks contribute to the
    # sum of samples in the histogram, whereas failures only contribute to the
    # count of samples. Since DataReductionProxy.Pingback.Succeeded is a boolean
    # histogram, the sum of all samples in that histogram is equal to the
    # number of successful pingbacks.
    succeeded = histogram_util.GetHistogramSum(
      histogram_type,
      'DataReductionProxy.Pingback.Succeeded',
      tab)
    if succeeded != 1 or count != 1:
      raise ChromeProxyMetricException, (
          'Expected 1 pingback success and no failures, but '
          'there were %d succesful pingbacks and %d failed pingback attempts'
          % (succeeded, count - succeeded))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'attempted', 'count', attempted))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'succeeded_count', 'count', count))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'succeeded_sum', 'count', succeeded))

  def AddResultsForQuicTransaction(self, tab, results):
    histogram_type = histogram_util.BROWSER_HISTOGRAM
    # This histogram should be synchronously created when the Navigate occurs.
    # Verify that histogram DataReductionProxy.Quic.ProxyStatus has no samples
    # in bucket >=1.
    fail_counts_proxy_status = histogram_util.GetHistogramSum(
        histogram_type,
        'DataReductionProxy.Quic.ProxyStatus',
        tab)
    if fail_counts_proxy_status != 0:
      raise ChromeProxyMetricException, (
          'fail_counts_proxy_status is %d.' % fail_counts_proxy_status)

    # Verify that histogram DataReductionProxy.Quic.ProxyStatus has at least 1
    # sample. This sample must be in bucket 0 (QUIC_PROXY_STATUS_AVAILABLE).
    success_counts_proxy_status = histogram_util.GetHistogramCount(
        histogram_type,
        'DataReductionProxy.Quic.ProxyStatus',
        tab)
    if success_counts_proxy_status <= 0:
      raise ChromeProxyMetricException, (
          'success_counts_proxy_status is %d.' % success_counts_proxy_status)

    # Navigate to one more page to ensure that established QUIC connection
    # is used for the next request. Give 1 second extra headroom for the QUIC
    # connection to be established.
    time.sleep(1)
    tab.Navigate('http://check.googlezip.net/test.html')

    proxy_usage_histogram_json = histogram_util.GetHistogram(histogram_type,
        'Net.QuicAlternativeProxy.Usage',
        tab)
    proxy_usage_histogram = json.loads(proxy_usage_histogram_json)

    # Bucket ALTERNATIVE_PROXY_USAGE_NO_RACE should have at least one sample.
    if proxy_usage_histogram['buckets'][0]['count'] <= 0:
      raise ChromeProxyMetricException, (
          'Number of samples in ALTERNATIVE_PROXY_USAGE_NO_RACE bucket is %d.'
             % proxy_usage_histogram['buckets'][0]['count'])

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'fail_counts_proxy_status', 'count',
        fail_counts_proxy_status))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'success_counts_proxy_status', 'count',
        success_counts_proxy_status))

  def AddResultsForBypassOnTimeout(self, tab, results):
    bypass_count = 0
    # Wait maximum of 120 seconds for test to complete. Should complete soon
    # after 90 second test server delay in case of failure, and much sooner in
    # case of success.
    tab.WaitForDocumentReadyStateToBeComplete(timeout=120)
    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader() and not resp.response.url.endswith(
          'favicon.ico'):
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should not have via header after HTTP timeout.\n'
            'Reponse: status=(%d)\nHeaders:\n %s' % (
                r.url, r.status, r.headers))
      elif not resp.response.url.endswith('favicon.ico'):
        bypass_count += 1
    if bypass_count == 0:
      raise ChromeProxyMetricException('No pages were tested!')
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'bypass', 'count', bypass_count))

  def AddResultsForBadHTTPSFallback(self, tab, results):
    via_count = 0
    tab.WaitForDocumentReadyStateToBeComplete(timeout=30)
    for resp in self.IterResponses(tab):
      if resp.HasChromeProxyViaHeader() and (resp.remote_port == 80
          or resp.remote_port == None):
        via_count += 1
      else:
        r = resp.response
        raise ChromeProxyMetricException, (
            'Response for %s should have via header and be on port 80 after '
            'bad proxy HTTPS response.\nReponse: status=(%d)\nport=(%d)\n'
            'Headers:\n %s' % (
                r.url, r.status, resp.remote_port, r.headers))
    if via_count == 0:
      raise ChromeProxyMetricException('No pages were tested!')
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'via', 'count', via_count))

PROXIED = 'proxied'
DIRECT = 'direct'


class ChromeProxyVideoMetric(network_metrics.NetworkMetric):
  """Metrics for video pages.

  Wraps the video metrics produced by videowrapper.js, such as the video
  duration and size in pixels. Also checks a few basic HTTP response headers
  such as Content-Type and Content-Length in the video responses.
  """

  def __init__(self, tab):
    super(ChromeProxyVideoMetric, self).__init__()
    with open(os.path.join(os.path.dirname(__file__), 'videowrapper.js')) as f:
      js = f.read()
      tab.ExecuteJavaScript(js)

  def Start(self, page, tab):
    tab.ExecuteJavaScript('window.__chromeProxyCreateVideoWrappers()')
    self.videoMetrics = None
    super(ChromeProxyVideoMetric, self).Start(page, tab)

  def Stop(self, page, tab):
    tab.WaitForJavaScriptCondition(
        'window.__chromeProxyVideoLoaded', timeout=30)
    m = tab.EvaluateJavaScript('window.__chromeProxyVideoMetrics')

    # Now wait for the video to stop playing.
    # Give it 2x the total duration to account for buffering.
    waitTime = 2 * m['video_duration']
    tab.WaitForJavaScriptCondition(
        'window.__chromeProxyVideoEnded', timeout=waitTime)

    # Load the final metrics.
    m = tab.EvaluateJavaScript('window.__chromeProxyVideoMetrics')
    self.videoMetrics = m
    # Cast this to an integer as it is often approximate (for an unknown reason)
    m['video_duration'] = int(m['video_duration'])
    super(ChromeProxyVideoMetric, self).Stop(page, tab)

  def ResponseFromEvent(self, event):
    return chrome_proxy_metrics.ChromeProxyResponse(event)

  def AddResults(self, tab, results):
    raise NotImplementedError

  def AddResultsForProxied(self, tab, results):
    return self._AddResultsShared(PROXIED, tab, results)

  def AddResultsForDirect(self, tab, results):
    return self._AddResultsShared(DIRECT, tab, results)

  def _AddResultsShared(self, kind, tab, results):
    def err(s):
      raise ChromeProxyMetricException, s

    # Should have played the video.
    if not self.videoMetrics['ready']:
      err('%s: video not played' % kind)

    # Should have an HTTP response for the video.
    wantContentType = 'video/webm' if kind == PROXIED else 'video/mp4'
    found = False
    for r in self.IterResponses(tab):
      resp = r.response
      if kind == DIRECT and r.HasChromeProxyViaHeader():
        err('%s: page has proxied Via header' % kind)
      if resp.GetHeader('Content-Type') != wantContentType:
        continue
      if found:
        err('%s: multiple video responses' % kind)
      found = True

      cl = resp.GetHeader('Content-Length')
      xocl = resp.GetHeader('X-Original-Content-Length')
      if cl != None:
        self.videoMetrics['content_length_header'] = int(cl)
      if xocl != None:
        self.videoMetrics['x_original_content_length_header'] = int(xocl)

      # Should have CL always.
      if cl == None:
        err('%s: missing ContentLength' % kind)
      # Proxied: should have CL < XOCL
      # Direct: should not have XOCL
      if kind == PROXIED:
        if xocl == None or int(cl) >= int(xocl):
          err('%s: bigger response (%s > %s)' % (kind, str(cl), str(xocl)))
      else:
        if xocl != None:
          err('%s: has XOriginalContentLength' % kind)

    if not found:
      err('%s: missing video response' % kind)

    # Finally, add all the metrics to the results.
    for (k, v) in self.videoMetrics.iteritems():
      k = "%s_%s" % (k, kind)
      results.AddValue(scalar.ScalarValue(results.current_page, k, "", v))


class ChromeProxyInstrumentedVideoMetric(Metric):
  """Metric for pages instrumented to evaluate video transcoding."""

  def __init__(self):
    super(ChromeProxyInstrumentedVideoMetric, self).__init__()

  def Stop(self, page, tab):
    waitTime = tab.EvaluateJavaScript('test.waitTime')
    tab.WaitForJavaScriptCondition('test.metrics.complete', timeout=waitTime)
    super(ChromeProxyInstrumentedVideoMetric, self).Stop(page, tab)

  def AddResults(self, tab, results):
    metrics = tab.EvaluateJavaScript('test.metrics')
    for (k, v) in metrics.iteritems():
      results.AddValue(scalar.ScalarValue(results.current_page, k, '', v))
    try:
      complete = metrics['complete']
      failed = metrics['failed']
      if not complete:
        raise ChromeProxyMetricException, 'Test not complete'
      if failed:
        raise ChromeProxyMetricException, 'failed'
    except KeyError:
      raise ChromeProxyMetricException, 'No metrics found'

# Returns whether |url| is a block-once test URL. Data Reduction Proxy has been
# configured to always return block-once for these URLs.
def IsTestUrlForBlockOnce(url):
  return (url == 'http://check.googlezip.net/blocksingle/' or
      url == ('http://chromeproxy-test.appspot.com/default?respBody=T0s='
              '&respHeader=eyJBY2Nlc3MtQ29udHJvbC1BbGxvdy1PcmlnaW4iOlsiKiJ'
              'dfQ==&respStatus=200&flywheelAction=block-once'))
