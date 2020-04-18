# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import logging

from common import chrome_proxy_metrics as metrics
from telemetry.core import exceptions
from telemetry.page import legacy_page_test


def WaitForViaHeader(tab, url="http://check.googlezip.net/test.html"):
  """Wait until responses start coming back with the Chrome Proxy via header.

  Poll |url| in |tab| until the Chrome Proxy via header is present in a
  response.

  This function is useful when testing with the Data Saver API, since Chrome
  won't actually start sending requests to the Data Reduction Proxy until the
  Data Saver API fetch completes. This function can be used to wait for the Data
  Saver API fetch to complete.
  """

  tab.Navigate('data:text/html;base64,%s' % base64.b64encode(
    '<html><body><script>'
    'window.via_header_found = false;'
    'function PollDRPCheck(url, wanted_via) {'
      'if (via_header_found) { return true; }'
      'try {'
        'var xmlhttp = new XMLHttpRequest();'
        'xmlhttp.open("GET",url,true);'
        'xmlhttp.onload=function(e) {'
          # Store the last response received for debugging, this will be shown
          # in telemetry dumps if the request fails or times out.
          'window.last_xhr_response_headers = xmlhttp.getAllResponseHeaders();'
          'var via=xmlhttp.getResponseHeader("via");'
          'if (via && via.indexOf(wanted_via) != -1) {'
            'window.via_header_found = true;'
          '}'
        '};'
        'xmlhttp.timeout=30000;'
        'xmlhttp.send();'
      '} catch (err) {'
        '/* Return normally if the xhr request failed. */'
      '}'
      'return false;'
    '}'
    '</script>'
    'Waiting for Chrome to start using the DRP...'
    '</body></html>'))

  # Ensure the page has finished loading before attempting the DRP check.
  tab.WaitForJavaScriptCondition('performance.timing.loadEventEnd', timeout=60)

  expected_via_header = metrics.CHROME_PROXY_VIA_HEADER
  if ChromeProxyValidation.extra_via_header:
    expected_via_header = ChromeProxyValidation.extra_via_header

  tab.WaitForJavaScriptCondition(
      'PollDRPCheck({{ url }}, {{ via_header }})',
      url=url, via_header=expected_via_header,
      timeout=60)


class ChromeProxyValidation(legacy_page_test.LegacyPageTest):
  """Base class for all chrome proxy correctness measurements."""

  # Value of the extra via header. |None| if no extra via header is expected.
  extra_via_header = None

  def __init__(self, metrics=None, clear_cache_before_each_run=True):
    super(ChromeProxyValidation, self).__init__(
        clear_cache_before_each_run=clear_cache_before_each_run)
    self._metrics = metrics
    self._page = None

  def CustomizeBrowserOptions(self, options):
    # Enable the chrome proxy (data reduction proxy).
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')
    self._is_chrome_proxy_enabled = True

    # Disable quic option, otherwise request headers won't be visible.
    options.AppendExtraBrowserArgs('--disable-quic')

  def DisableChromeProxy(self):
    self.options.browser_options.extra_browser_args.discard(
                '--enable-spdy-proxy-auth')
    self._is_chrome_proxy_enabled = False

  def WillNavigateToPage(self, page, tab):
    if self._is_chrome_proxy_enabled:
      WaitForViaHeader(tab)

    if self.clear_cache_before_each_run:
      tab.ClearCache(force=True)
    assert self._metrics
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    self._page = page
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    assert self._metrics
    self._metrics.Stop(page, tab)
    if ChromeProxyValidation.extra_via_header:
      self._metrics.AddResultsForExtraViaHeader(
          tab, results, ChromeProxyValidation.extra_via_header)
    self.AddResults(tab, results)

  def AddResults(self, tab, results):
    raise NotImplementedError
