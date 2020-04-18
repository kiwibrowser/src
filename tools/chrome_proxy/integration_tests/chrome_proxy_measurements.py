# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import logging
import urlparse

from common import chrome_proxy_measurements as measurements
from common.chrome_proxy_measurements import ChromeProxyValidation
from integration_tests import chrome_proxy_metrics as metrics
from metrics import loading
from telemetry.core import exceptions, util
from telemetry.page import legacy_page_test

class ChromeProxyBypassOnTimeout(ChromeProxyValidation):
  """Checks the client bypasses when endpoint site times out."""

  def __init__(self):
    super(ChromeProxyBypassOnTimeout, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyBypassOnTimeout, self).CustomizeBrowserOptions(
        options)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForBypassOnTimeout(tab, results)

class ChromeProxyDataSaving(legacy_page_test.LegacyPageTest):
  """Chrome proxy data saving measurement."""
  def __init__(self, *args, **kwargs):
    super(ChromeProxyDataSaving, self).__init__(*args, **kwargs)
    self._metrics = metrics.ChromeProxyMetric()
    self._enable_proxy = True

  def CustomizeBrowserOptions(self, options):
    if self._enable_proxy:
      options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')

  def WillNavigateToPage(self, page, tab):
    if self._enable_proxy:
      measurements.WaitForViaHeader(tab)
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    self._metrics.Stop(page, tab)
    self._metrics.AddResultsForDataSaving(tab, results)


class ChromeProxyHeaders(ChromeProxyValidation):
  """Correctness measurement for response headers."""

  def __init__(self):
    super(ChromeProxyHeaders, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHeaderValidation(tab, results)


class ChromeProxyBypass(ChromeProxyValidation):
  """Correctness measurement for bypass responses."""

  def __init__(self):
    super(ChromeProxyBypass, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForBypass(tab, results)


class ChromeProxyHTTPSBypass(ChromeProxyValidation):
  """Correctness measurement for bypass responses."""

  def __init__(self):
    super(ChromeProxyHTTPSBypass, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHTTPSBypass(tab, results)


class ChromeProxyYouTube(ChromeProxyValidation):
  """Correctness measurement for youtube video playback."""

  def __init__(self):
    super(ChromeProxyYouTube, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForYouTube(tab, results)


class ChromeProxyHTML5Test(ChromeProxyValidation):
  """Correctness measurement for html5test page."""

  def __init__(self):
    super(ChromeProxyHTML5Test, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHTML5Test(tab, results)


class ChromeProxyCorsBypass(ChromeProxyValidation):
  """Correctness measurement for bypass responses for CORS requests."""

  def __init__(self):
    super(ChromeProxyCorsBypass, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def ValidateAndMeasurePage(self, page, tab, results):
    # The test page sets window.xhrRequestCompleted to true when the XHR fetch
    # finishes.
    tab.WaitForJavaScriptCondition('window.xhrRequestCompleted', timeout=300)
    super(ChromeProxyCorsBypass,
          self).ValidateAndMeasurePage(page, tab, results)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForCorsBypass(tab, results)


class ChromeProxyBlockOnce(ChromeProxyValidation):
  """Correctness measurement for block-once responses."""

  def __init__(self):
    super(ChromeProxyBlockOnce, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForBlockOnce(tab, results)


class ChromeProxySafebrowsingOn(ChromeProxyValidation):
  """Correctness measurement for safebrowsing."""

  def __init__(self):
    super(ChromeProxySafebrowsingOn, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForSafebrowsingOn(tab, results)

class ChromeProxySafebrowsingOff(ChromeProxyValidation):
  """Correctness measurement for safebrowsing."""

  def __init__(self):
    super(ChromeProxySafebrowsingOff, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForSafebrowsingOff(tab, results)

_FAKE_PROXY_AUTH_VALUE = 'aabbccdd3b7579186c1b0620614fdb1f0000ffff'
_TEST_SERVER = 'chromeproxy-test.appspot.com'
_TEST_SERVER_DEFAULT_URL = 'http://' + _TEST_SERVER + '/default'


# We rely on the chromeproxy-test server to facilitate some of the tests.
# The test server code is at <TBD location> and runs at _TEST_SERVER
#
# The test server allow request to override response status, headers, and
# body through query parameters. See GetResponseOverrideURL.
def GetResponseOverrideURL(url=_TEST_SERVER_DEFAULT_URL, respStatus=0,
                           respHeader="", respBody=""):
  """ Compose the request URL with query parameters to override
  the chromeproxy-test server response.
  """

  queries = []
  if respStatus > 0:
    queries.append('respStatus=%d' % respStatus)
  if respHeader:
    queries.append('respHeader=%s' % base64.b64encode(respHeader))
  if respBody:
    queries.append('respBody=%s' % base64.b64encode(respBody))
  if len(queries) == 0:
    return url
  "&".join(queries)
  # url has query already
  if urlparse.urlparse(url).query:
    return url + '&' + "&".join(queries)
  else:
    return url + '?' + "&".join(queries)

class ChromeProxyBadHTTPSFallback(ChromeProxyValidation):
  """Checks the client falls back to HTTP proxy when HTTPS proxy errors."""

  def __init__(self):
    super(ChromeProxyBadHTTPSFallback, self).__init__(
        metrics=metrics.ChromeProxyMetric())
    self._is_chrome_proxy_enabled = True

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyBadHTTPSFallback, self).CustomizeBrowserOptions(
        options)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForBadHTTPSFallback(tab, results)

class ChromeProxyHTTPFallbackProbeURL(ChromeProxyValidation):
  """Correctness measurement for proxy fallback.

  In this test, the probe URL does not return 'OK'. Chrome is expected
  to use the fallback proxy.
  """

  def __init__(self):
    super(ChromeProxyHTTPFallbackProbeURL, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyHTTPFallbackProbeURL,
          self).CustomizeBrowserOptions(options)
    # Set the secure proxy check URL to the google.com favicon, which will be
    # interpreted as a secure proxy check failure since the response body is not
    # "OK". The google.com favicon is used because it will load reliably fast,
    # and there have been problems with chromeproxy-test.appspot.com being slow
    # and causing tests to flake.
    options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-secure-proxy-check-url='
        'http://www.google.com/favicon.ico')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHTTPFallback(tab, results)


class ChromeProxyHTTPFallbackViaHeader(ChromeProxyValidation):
  """Correctness measurement for proxy fallback.

  In this test, the configured proxy is the chromeproxy-test server which
  will send back a response without the expected Via header. Chrome is
  expected to use the fallback proxy and add the configured proxy to the
  bad proxy list.
  """

  def __init__(self):
    super(ChromeProxyHTTPFallbackViaHeader, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyHTTPFallbackViaHeader,
          self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs('--ignore-certificate-errors')
    # Set the primary Data Reduction Proxy to be the test server. The test
    # doesn't know if Chrome is configuring the DRP using the Data Saver API or
    # not, so the appropriate flags are set for both cases.
    options.AppendExtraBrowserArgs(
        '--spdy-proxy-auth-origin=http://%s' % _TEST_SERVER)
    options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-http-proxies='
        'http://%s;http://compress.googlezip.net' % _TEST_SERVER)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHTTPFallback(tab, results)


class ChromeProxyClientType(ChromeProxyValidation):
  """Correctness measurement for Chrome-Proxy header client type directives."""

  def __init__(self):
    super(ChromeProxyClientType, self).__init__(
        metrics=metrics.ChromeProxyMetric())
    self._chrome_proxy_client_type = None

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyClientType, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs('--disable-quic')

  def AddResults(self, tab, results):
    # Get the Chrome-Proxy client type from the first page in the page set, so
    # that the client type value can be used to determine which of the later
    # pages in the page set should be bypassed.
    if not self._chrome_proxy_client_type:
      client_type = self._metrics.GetClientTypeFromRequests(tab)
      if client_type:
        self._chrome_proxy_client_type = client_type

    self._metrics.AddResultsForClientType(tab,
                                          results,
                                          self._chrome_proxy_client_type,
                                          self._page.bypass_for_client_type)


class ChromeProxyLoFi(ChromeProxyValidation):
  """Correctness measurement for Lo-Fi in Chrome-Proxy header."""

  def __init__(self):
    super(ChromeProxyLoFi, self).__init__(metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyLoFi, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs('--data-reduction-proxy-lo-fi=always-on')
    # Disable server experiments such as tamper detection.
    options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-server-experiments-disabled')
    options.AppendExtraBrowserArgs('--disable-quic')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForLoFi(tab, results)

class ChromeProxyCacheLoFiDisabled(ChromeProxyValidation):
  """
  Correctness measurement for Lo-Fi placeholder is not loaded from cache when a
  page is reloaded with LoFi disabled. First a test page is opened with LoFi and
  chrome proxy enabled. This allows Chrome to cache the LoFi placeholder image.
  The browser is restarted with LoFi disabled and the same test page is loaded.
  This second page load should not pick the LoFi placeholder from cache and
  original image should be loaded. This test should be run with
  --profile-type=default command line for the same user profile and cache to be
  used across the two page loads.
  """

  def __init__(self):
    super(ChromeProxyCacheLoFiDisabled, self).__init__(
            metrics=metrics.ChromeProxyMetric(),
            clear_cache_before_each_run=False)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForLoFiCache(tab, results, self._is_lo_fi_enabled)

  def WillStartBrowser(self, platform):
    super(ChromeProxyCacheLoFiDisabled, self).WillStartBrowser(platform)
    self.options.AppendExtraBrowserArgs('--disable-quic')
    if not self._page:
      # First page load, enable LoFi and chrome proxy. Disable server
      # experiments such as tamper detection.
      self.options.AppendExtraBrowserArgs(
            '--data-reduction-proxy-lo-fi=always-on')
      self.options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-server-experiments-disabled')
      self._is_lo_fi_enabled = True
    else:
      # Second page load, disable LoFi. Chrome proxy is still enabled. Disable
      # server experiments such as tamper detection.
      self.options.browser_options.extra_browser_args.discard(
            '--data-reduction-proxy-lo-fi=always-on')
      self.options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-server-experiments-disabled')
      self._is_lo_fi_enabled = False

  def WillNavigateToPage(self, page, tab):
    super(ChromeProxyCacheLoFiDisabled, self).WillNavigateToPage(page, tab)
    if self._is_lo_fi_enabled:
      # Clear cache for the first page to pick LoFi image from server.
      tab.ClearCache(force=True)

  def DidNavigateToPage(self, page, tab):
    if not self._is_lo_fi_enabled:
      tab.ExecuteJavaScript('window.location.reload()')
      util.WaitFor(tab.HasReachedQuiescence, 3)

class ChromeProxyCacheProxyDisabled(ChromeProxyValidation):
  """
  Correctness measurement for Lo-Fi placeholder is not loaded from cache when a
  page is reloaded with data reduction proxy disabled. First a test page is
  opened with LoFi and chrome proxy enabled. This allows Chrome to cache the
  LoFi placeholder image. The browser is restarted with chrome proxy disabled
  and the same test page is loaded. This second page load should not pick the
  LoFi placeholder from cache and original image should be loaded. This test
  should be run with --profile-type=default command line for the same user
  profile and cache to be used across the two page loads.
  """

  def __init__(self):
    super(ChromeProxyCacheProxyDisabled, self).__init__(
            metrics=metrics.ChromeProxyMetric(),
            clear_cache_before_each_run=False)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForLoFiCache(tab, results,
                                         self._is_chrome_proxy_enabled)

  def WillStartBrowser(self, platform):
    super(ChromeProxyCacheProxyDisabled, self).WillStartBrowser(platform)
    self.options.AppendExtraBrowserArgs('--disable-quic')
    if not self._page:
      # First page load, enable LoFi and chrome proxy. Disable server
      # experiments such as tamper detection.
      self.options.AppendExtraBrowserArgs(
            '--data-reduction-proxy-lo-fi=always-on')
      self.options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-server-experiments-disabled')
    else:
      # Second page load, disable chrome proxy. LoFi is still enabled.
      self.DisableChromeProxy()

  def WillNavigateToPage(self, page, tab):
    super(ChromeProxyCacheProxyDisabled, self).WillNavigateToPage(page, tab)
    if self._is_chrome_proxy_enabled:
      # Clear cache for the first page to pick LoFi image from server.
      tab.ClearCache(force=True)

  def DidNavigateToPage(self, page, tab):
    if not self._is_chrome_proxy_enabled:
      tab.ExecuteJavaScript('window.location.reload()')
      util.WaitFor(tab.HasReachedQuiescence, 3)

class ChromeProxyLitePage(ChromeProxyValidation):
  """Correctness measurement for lite pages in the Chrome-Proxy header."""

  def __init__(self):
    super(ChromeProxyLitePage, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyLitePage, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-lo-fi=always-on')
    options.AppendExtraBrowserArgs(
        '--enable-data-reduction-proxy-lite-page')
    options.AppendExtraBrowserArgs('--disable-quic')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForLitePage(tab, results)

class ChromeProxyExpDirective(ChromeProxyValidation):
  """Correctness measurement for experiment directives in Chrome-Proxy header.

  This test verifies that "exp=test" in the Chrome-Proxy request header
  causes a bypass on the experiment test page.
  """

  def __init__(self):
    super(ChromeProxyExpDirective, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyExpDirective, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs('--data-reduction-proxy-experiment=test')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForBypass(tab, results, url_pattern='/exp/')

class ChromeProxyPassThrough(ChromeProxyValidation):
  """Correctness measurement for Chrome-Proxy-Accept-Transform identity
  directives.

  This test verifies that "identity" in the Chrome-Proxy-Accept-Transform
  request header causes a resource to be loaded without Data Reduction Proxy
  transformations.
  """

  def __init__(self):
    super(ChromeProxyPassThrough, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyPassThrough, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs('--disable-quic')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForPassThrough(tab, results)

class ChromeProxyHTTPToDirectFallback(ChromeProxyValidation):
  """Correctness measurement for HTTP proxy fallback to direct."""

  def __init__(self):
    super(ChromeProxyHTTPToDirectFallback, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyHTTPToDirectFallback,
          self).CustomizeBrowserOptions(options)
    # Set the primary proxy to something that will fail to be resolved so that
    # this test will run using the HTTP fallback proxy. The test doesn't know if
    # Chrome is configuring the DRP using the Data Saver API or not, so the
    # appropriate flags are set for both cases.
    options.AppendExtraBrowserArgs(
        '--spdy-proxy-auth-origin=http://nonexistent.googlezip.net')
    options.AppendExtraBrowserArgs(
        '--data-reduction-proxy-http-proxies='
        'http://nonexistent.googlezip.net;http://compress.googlezip.net')

  def WillNavigateToPage(self, page, tab):
    super(ChromeProxyHTTPToDirectFallback, self).WillNavigateToPage(page, tab)
    # Attempt to load a page through the nonexistent primary proxy in order to
    # cause a proxy fallback, and have this test run starting from the HTTP
    # fallback proxy.
    tab.Navigate(_TEST_SERVER_DEFAULT_URL)
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)

  def AddResults(self, tab, results):
    self._metrics.AddResultsForHTTPToDirectFallback(tab, results, _TEST_SERVER)


class ChromeProxyReenableAfterBypass(ChromeProxyValidation):
  """Correctness measurement for re-enabling proxies after bypasses.

  This test loads a page that causes all data reduction proxies to be bypassed
  for 1 to 5 minutes, then waits 5 minutes and verifies that the proxy is no
  longer bypassed.
  """

  def __init__(self):
    super(ChromeProxyReenableAfterBypass, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForReenableAfterBypass(
        tab, results, self._page.bypass_seconds_min,
        self._page.bypass_seconds_max)


class ChromeProxyReenableAfterSetBypass(ChromeProxyValidation):
  """Correctness test for re-enabling proxies after bypasses with set duration.

  This test loads a page that causes all data reduction proxies to be bypassed
  for 20 seconds.
  """

  def __init__(self):
    super(ChromeProxyReenableAfterSetBypass, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    self._metrics.AddResultsForReenableAfterSetBypass(
        tab, results, self._page.BYPASS_SECONDS)


class ChromeProxySmoke(ChromeProxyValidation):
  """Smoke measurement for basic chrome proxy correctness."""

  def __init__(self):
    super(ChromeProxySmoke, self).__init__(metrics=metrics.ChromeProxyMetric())

  def AddResults(self, tab, results):
    # Map a page name to its AddResults func.
    page_to_metrics = {
        'header validation': [self._metrics.AddResultsForHeaderValidation],
        'compression: image': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'compression: javascript': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'compression: css': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'bypass': [self._metrics.AddResultsForBypass],
        }
    if not self._page.name in page_to_metrics:
      raise page_test.MeasurementFailure(
          'Invalid page name (%s) in smoke. Page name must be one of:\n%s' % (
          self._page.name, page_to_metrics.keys()))
    for add_result in page_to_metrics[self._page.name]:
      add_result(tab, results)

class ChromeProxyQuicSmoke(legacy_page_test.LegacyPageTest):
  """Smoke measurement for basic chrome proxy correctness when using a
  proxy that supports QUIC."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyQuicSmoke, self).__init__(*args, **kwargs)
    self._metrics = metrics.ChromeProxyMetric()
    self._enable_proxy = True

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyQuicSmoke, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs(
      '--enable-quic')
    options.AppendExtraBrowserArgs(
      '--data-reduction-proxy-http-proxies=https://proxy.googlezip.net:443')
    options.AppendExtraBrowserArgs(
      '--force-fieldtrials=DataReductionProxyUseQuic/Enabled')
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')

  def WillNavigateToPage(self, page, tab):
    if self._enable_proxy:
      measurements.WaitForViaHeader(tab)
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    self._metrics.Stop(page, tab)
    page_to_metrics = {
        'header validation': [self._metrics.AddResultsForHeaderValidation],
        'compression: image': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'compression: javascript': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'compression: css': [
            self._metrics.AddResultsForHeaderValidation,
            self._metrics.AddResultsForDataSaving,
            ],
        'bypass': [self._metrics.AddResultsForBypass],
        }
    if not page.name in page_to_metrics:
      raise page_test.MeasurementFailure(
          'Invalid page name (%s) in QUIC smoke. '
          'Page name must be one of:\n%s' % (
          page.name, page_to_metrics.keys()))
    for add_result in page_to_metrics[page.name]:
      add_result(tab, results)

PROXIED = metrics.PROXIED
DIRECT = metrics.DIRECT

class ChromeProxyClientConfig(ChromeProxyValidation):
  """Chrome proxy client configuration service validation."""

  def __init__(self):
    super(ChromeProxyClientConfig, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyClientConfig, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs(
      '--enable-data-reduction-proxy-config-client')
    options.AppendExtraBrowserArgs('--disable-quic')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForClientConfig(tab, results)

class ChromeProxyVideoValidation(legacy_page_test.LegacyPageTest):
  """Validation for video pages.

  Measures pages using metrics.ChromeProxyVideoMetric. Pages can be fetched
  either direct from the origin server or via the proxy. If a page is fetched
  both ways, then the PROXIED and DIRECT measurements are compared to ensure
  the same video was loaded in both cases.
  """

  def __init__(self):
    super(ChromeProxyVideoValidation, self).__init__(
        clear_cache_before_each_run=True)
    # The type is _allMetrics[url][PROXIED,DIRECT][metricName] = value,
    # where (metricName,value) is a metric computed by videowrapper.js.
    self._allMetrics = {}

  def WillNavigateToPage(self, page, tab):
    if page.use_chrome_proxy:
      measurements.WaitForViaHeader(tab)
    super(ChromeProxyVideoValidation, self).WillNavigateToPage(page, tab)

  def DidNavigateToPage(self, page, tab):
    self._currMetrics = metrics.ChromeProxyVideoMetric(tab)
    self._currMetrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    assert self._currMetrics
    self._currMetrics.Stop(page, tab)
    if page.url not in self._allMetrics:
      self._allMetrics[page.url] = {}

    # Verify this page.
    if page.use_chrome_proxy:
      self._currMetrics.AddResultsForProxied(tab, results)
      self._allMetrics[page.url][PROXIED] = self._currMetrics.videoMetrics
    else:
      self._currMetrics.AddResultsForDirect(tab, results)
      self._allMetrics[page.url][DIRECT] = self._currMetrics.videoMetrics
    self._currMetrics = None

    # Compare proxied and direct results for this url, if they exist.
    m = self._allMetrics[page.url]
    if PROXIED in m and DIRECT in m:
      self._CompareProxiedAndDirectMetrics(page.url, m[PROXIED], m[DIRECT])

  def _CompareProxiedAndDirectMetrics(self, url, pm, dm):
    """Compare metrics from PROXIED and DIRECT fetches.

    Compares video metrics computed by videowrapper.js for pages that were
    fetch both PROXIED and DIRECT.

    Args:
        url: The url for the page being tested.
        pm: Metrics when loaded by the Flywheel proxy.
        dm: Metrics when loaded directly from the origin server.

    Raises:
        ChromeProxyMetricException on failure.
    """
    def err(s):
      raise ChromeProxyMetricException, s

    if not pm['ready']:
      err('Proxied page did not load video: %s' % page.url)
    if not dm['ready']:
      err('Direct page did not load video: %s' % page.url)

    # Compare metrics that should match for PROXIED and DIRECT.
    for x in ('video_height', 'video_width', 'video_duration',
              'decoded_frames'):
      if x not in pm:
        err('Proxied page has no %s: %s' % (x, page.url))
      if x not in dm:
        err('Direct page has no %s: %s' % (x, page.url))
      if pm[x] != dm[x]:
        err('Mismatch for %s (proxied=%s direct=%s): %s' %
            (x, str(pm[x]), str(dm[x]), page.url))

    # Proxied XOCL should match direct CL.
    pxocl = pm['x_original_content_length_header']
    dcl = dm['content_length_header']
    if pxocl != dcl:
      err('Mismatch for content length (proxied=%s direct=%s): %s' %
          (str(pxocl), str(dcl), page.url))

class ChromeProxyInstrumentedVideoValidation(legacy_page_test.LegacyPageTest):
  """Tests a specially instrumented page for correct video transcoding."""

  def __init__(self):
    super(ChromeProxyInstrumentedVideoValidation, self).__init__(
        clear_cache_before_each_run=True)
    self._metrics = metrics.ChromeProxyInstrumentedVideoMetric()

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')

  def WillNavigateToPage(self, page, tab):
    measurements.WaitForViaHeader(tab)
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    self._metrics.Stop(page, tab)
    self._metrics.AddResults(tab, results)

class ChromeProxyPingback(ChromeProxyValidation):
  """Chrome proxy pageload metrics pingback service validation."""

  def __init__(self):
    super(ChromeProxyPingback, self).__init__(
        metrics=metrics.ChromeProxyMetric())

  def CustomizeBrowserOptions(self, options):
    super(ChromeProxyPingback, self).CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs(
      '--enable-data-reduction-proxy-force-pingback')
    options.AppendExtraBrowserArgs(
      '--enable-stats-collection-bindings')

  def AddResults(self, tab, results):
    self._metrics.AddResultsForPingback(tab, results)

class ChromeProxyQuicTransaction(legacy_page_test.LegacyPageTest):
  """Chrome quic proxy usage validation when connecting to a proxy that
  supports QUIC."""

  def __init__(self, *args, **kwargs):
    super(ChromeProxyQuicTransaction, self).__init__(*args, **kwargs)
    self._metrics = metrics.ChromeProxyMetric()
    self._enable_proxy = True

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
      '--enable-quic')
    options.AppendExtraBrowserArgs(
      '--data-reduction-proxy-http-proxies=https://proxy.googlezip.net:443')
    options.AppendExtraBrowserArgs(
      '--force-fieldtrials=DataReductionProxyUseQuic/Enabled')
    options.AppendExtraBrowserArgs('--enable-spdy-proxy-auth')
    options.AppendExtraBrowserArgs(
      '--enable-stats-collection-bindings')

  def WillNavigateToPage(self, page, tab):
    if self._enable_proxy:
      measurements.WaitForViaHeader(tab)
    tab.ClearCache(force=True)
    self._metrics.Start(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    # Wait for the load event.
    tab.WaitForJavaScriptCondition(
        'performance.timing.loadEventStart', timeout=300)
    self._metrics.Stop(page, tab)
    self._metrics.AddResultsForQuicTransaction(tab, results)
