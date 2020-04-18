# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from common import TestDriver
from common import IntegrationTest
from decorators import ChromeVersionEqualOrAfterM


class Bypass(IntegrationTest):

  # Ensure Chrome does not use Data Saver for block-once, but does use Data
  # Saver for a subsequent request.
  def testBlockOnce(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.LoadURL('http://check.googlezip.net/blocksingle/')
      responses = t.GetHTTPResponses()
      self.assertEqual(2, len(responses))
      for response in responses:
        if response.url == "http://check.googlezip.net/image.png":
          self.assertHasChromeProxyViaHeader(response)
        else:
          self.assertNotHasChromeProxyViaHeader(response)

  # Ensure Chrome does not use Data Saver for block=0, which uses the default
  # proxy retry delay.
  def testBypass(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.LoadURL('http://check.googlezip.net/block/')
      for response in t.GetHTTPResponses():
        self.assertNotHasChromeProxyViaHeader(response)

      # Load another page and check that Data Saver is not used.
      t.LoadURL('http://check.googlezip.net/test.html')
      for response in t.GetHTTPResponses():
        self.assertNotHasChromeProxyViaHeader(response)

  # Ensure Chrome does not use Data Saver for HTTPS requests.
  def testHttpsBypass(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')

      # Load HTTP page and check that Data Saver is used.
      t.LoadURL('http://check.googlezip.net/test.html')
      responses = t.GetHTTPResponses()
      self.assertEqual(2, len(responses))
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)

      # Load HTTPS page and check that Data Saver is not used.
      t.LoadURL('https://check.googlezip.net/test.html')
      responses = t.GetHTTPResponses()
      self.assertEqual(2, len(responses))
      for response in responses:
        self.assertNotHasChromeProxyViaHeader(response)

  # Verify that CORS requests receive a block-once from the data reduction
  # proxy by checking that those requests are retried without data reduction
  # proxy.
  def testCorsBypass(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.LoadURL('http://www.gstatic.com/chrome/googlezip/cors/')

      # Navigate to a different page to verify that later requests are not
      # blocked.
      test_driver.LoadURL('http://check.googlezip.net/test.html')

      cors_requests = 0
      same_origin_requests = 0
      for response in test_driver.GetHTTPResponses():
        # The origin header implies that |response| is a CORS request.
        if ('origin' not in response.request_headers):
          self.assertHasChromeProxyViaHeader(response)
          same_origin_requests = same_origin_requests + 1
        else:
          self.assertNotHasChromeProxyViaHeader(response)
          cors_requests = cors_requests + 1
      # Verify that both CORS and same origin requests were seen.
      self.assertNotEqual(0, same_origin_requests)
      self.assertNotEqual(0, cors_requests)

  # Verify that when an origin times out using Data Saver, the request is
  # fetched directly and data saver is bypassed only for one request.
  def testOriginTimeoutBlockOnce(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')

      # Load URL that times out when the proxy server tries to access it.
      test_driver.LoadURL('http://chromeproxy-test.appspot.com/blackhole')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
          self.assertNotHasChromeProxyViaHeader(response)

      # Load HTTP page and check that Data Saver is used.
      test_driver.LoadURL('http://check.googlezip.net/test.html')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)

  # Verify that Chrome does not bypass the proxy when a response gets a missing
  # via header.
  @ChromeVersionEqualOrAfterM(67)
  def testMissingViaHeaderNoBypassExperiment(self):
    with TestDriver() as t:
      t.AddChromeArg('--enable-spdy-proxy-auth')
      t.AddChromeArg('--enable-features=DataReductionProxyRobustConnection'
        '<DataReductionProxyRobustConnection')
      t.AddChromeArg('--force-fieldtrials=DataReductionProxyRobustConnection/'
        'Enabled')
      t.AddChromeArg('--force-fieldtrial-params='
        'DataReductionProxyRobustConnection.Enabled:'
        'warmup_fetch_callback_enabled/true/'
        'bypass_missing_via_disabled/true')
      t.AddChromeArg('--disable-data-reduction-proxy-warmup-url-fetch')
      t.AddChromeArg('--data-reduction-proxy-http-proxies='
        # The chromeproxy-test server is a simple HTTP server. If it is served a
        # proxy-request, it will respond with a 404 error page. It will not set
        # the Via header on the response.
        'https://chromeproxy-test.appspot.com;http://compress.googlezip.net')

      # Loading this URL should not hit the actual check.googlezip.net origin.
      # Instead, the test server proxy should fully handle the request and will
      # respond with an error page.
      t.LoadURL("http://check.googlezip.net/test.html")
      for response in t.GetHTTPResponses():
        self.assertNotHasChromeProxyViaHeader(response)

      # Check the via bypass histograms are empty.
      histogram = t.GetHistogram(
        'DataReductionProxy.BypassedBytes.MissingViaHeader4xx')
      self.assertEqual(0, len(histogram))
      histogram = t.GetHistogram(
        'DataReductionProxy.BypassedBytes.MissingViaHeaderOther')
      self.assertEqual(0, len(histogram))

      # Check that the fetch used the proxy.
      histogram = t.GetHistogram('DataReductionProxy.ProxySchemeUsed')
      self.assertEqual(histogram['buckets'][0]['low'], 2)
      self.assertEqual(histogram['buckets'][0]['high'], 3)

  # Verify that when Chrome receives a 4xx response through a Data Reduction
  # Proxy that doesn't set a proper via header, Chrome bypasses all proxies and
  # retries the request over direct.
  def testMissingViaHeader4xxBypass(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')

      # Set the primary Data Reduction Proxy to be the test server, which does
      # not add any Via headers.
      test_driver.AddChromeArg('--data-reduction-proxy-http-proxies='
                               'https://chromeproxy-test.appspot.com;'
                               'http://compress.googlezip.net')

      # Load a page that will come back with a 4xx response code and without the
      # proper via header. Chrome should bypass all proxies and retry the
      # request.
      test_driver.LoadURL(
          'http://chromeproxy-test.appspot.com/default?respStatus=414')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertNotHasChromeProxyViaHeader(response)
        self.assertEqual(u'http/1.1', response.protocol)

      # Check that the BlockTypePrimary histogram has at least one entry in the
      # MissingViaHeader4xx category (which is enum value 4), to make sure that
      # the bypass was caused by the missing via header logic and not something
      # else. The favicon for this URL may also be fetched, but will return a
      # 404.
      histogram = test_driver.GetHistogram(
          "DataReductionProxy.BlockTypePrimary")
      self.assertNotEqual(0, histogram['count'])
      self.assertEqual(1, len(histogram['buckets']))
      self.assertEqual(5, histogram['buckets'][0]['high'])
      self.assertEqual(4, histogram['buckets'][0]['low'])

  # Verify that the Data Reduction Proxy understands the "exp" directive.
  def testExpDirectiveBypass(self):
    # If it was attempted to run with another experiment, skip this test.
    if common.ParseFlags().browser_args and ('--data-reduction-proxy-experiment'
        in common.ParseFlags().browser_args):
      self.skipTest('This test cannot be run with other experiments.')
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--data-reduction-proxy-experiment=test')

      # Verify that loading a page other than the specific exp directive test
      # page loads through the proxy without being bypassed.
      test_driver.LoadURL('http://check.googlezip.net/test.html')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)

      # Verify that loading the exp directive test page with "exp=test" triggers
      # a bypass.
      test_driver.LoadURL('http://check.googlezip.net/exp/')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertNotHasChromeProxyViaHeader(response)

    # Verify that loading the same test page without setting "exp=test" loads
    # through the proxy without being bypassed.
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')

      test_driver.LoadURL('http://check.googlezip.net/exp/')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)

  # Data Saver uses a HTTPS proxy by default, if that fails it will fall back to
  # a HTTP proxy.
  def testBadHTTPSFallback(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      # Set the primary (HTTPS) proxy to a bad one.
      # That will force Data Saver to the HTTP proxy for normal page requests.
      test_driver.AddChromeArg('--spdy-proxy-auth-origin='
                               'https://nonexistent.googlezip.net')
      test_driver.AddChromeArg('--data-reduction-proxy-http-proxies='
                               'http://compress.googlezip.net')

      test_driver.LoadURL('http://check.googlezip.net/fallback/')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertEqual(80, response.port)

  # Get the client type with the first request, then check bypass on the
  # appropriate test page
  def testClientTypeBypass(self):
    clientType = ''
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      # Page that should not bypass.
      test_driver.LoadURL('http://check.googlezip.net/test.html')
      responses = test_driver.GetHTTPResponses()
      self.assertNotEqual(0, len(responses))
      for response in responses:
        self.assertHasChromeProxyViaHeader(response)
        chrome_proxy_header = response.request_headers['chrome-proxy']
        chrome_proxy_directives = chrome_proxy_header.split(',')
        for directive in chrome_proxy_directives:
            if 'c=' in directive:
                clientType = directive[3:]

    clients = ['android', 'webview', 'ios', 'linux', 'win', 'chromeos']
    for client in clients:
      with TestDriver() as test_driver:
        test_driver.LoadURL('http://check.googlezip.net/chrome-proxy-header/'
                          'c_%s/' %client)
        responses = test_driver.GetHTTPResponses()
        self.assertEqual(2, len(responses))
        for response in responses:
          if client in clientType:
            self.assertNotHasChromeProxyViaHeader(response)

if __name__ == '__main__':
  IntegrationTest.RunAllTests()
