# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from common import TestDriver
from common import IntegrationTest
from decorators import ChromeVersionBeforeM
from decorators import ChromeVersionEqualOrAfterM

class LoFi(IntegrationTest):

  #  Checks that the compressed image is below a certain threshold.
  #  The test page is uncacheable otherwise a cached page may be served that
  #  doesn't have the correct via headers.
  @ChromeVersionBeforeM(65)
  def testLoFiOldFlags(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=always-on')
      # Disable server experiments such as tamper detection.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        if (self.checkLoFiResponse(response, True)):
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

      # Verify Lo-Fi previews info bar recorded
      histogram = test_driver.GetHistogram('Previews.InfoBarAction.LoFi', 5)
      self.assertEqual(1, histogram['count'])

  # Checks that LoFi images are served when LoFi slow connections are used and
  # the network quality estimator returns Slow2G.
  @ChromeVersionBeforeM(65)
  def testLoFiSlowConnectionOldFlags(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=slow-connections-'
                               'only')
      # Disable server experiments such as tamper detection.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/Slow2G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        if (self.checkLoFiResponse(response, True)):
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

      # Verify Lo-Fi previews info bar recorded
      histogram = test_driver.GetHistogram('Previews.InfoBarAction.LoFi', 5)
      self.assertEqual(1, histogram['count'])

  # Checks that LoFi images are served when LoFi slow connections are used and
  # the network quality estimator returns Slow2G.
  @ChromeVersionEqualOrAfterM(65)
  def testLoFiOnSlowConnection(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      # Disable server experiments such as tamper detection.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/Slow2G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        if (self.checkLoFiResponse(response, True)):
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

      # Verify Lo-Fi previews info bar recorded
      histogram = test_driver.GetHistogram('Previews.InfoBarAction.LoFi', 5)
      self.assertEqual(1, histogram['count'])

  # Checks that LoFi images are NOT served when the network quality estimator
  # returns fast connection type.
  @ChromeVersionBeforeM(65)
  def testLoFiFastConnectionOldFlags(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=slow-connections-'
                               'only')
      # Disable server experiments such as tamper detection.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/4G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if response.url.endswith('html'):
          # Main resource should accept transforms but not be transformed.
          self.assertEqual('lite-page',
            response.request_headers['chrome-proxy-accept-transform'])
          self.assertNotIn('chrome-proxy-content-transform',
            response.response_headers)
          if 'chrome-proxy' in response.response_headers:
            self.assertNotIn('page-policies',
                             response.response_headers['chrome-proxy'])
        else:
          # No subresources should accept transforms.
          self.assertNotIn('chrome-proxy-accept-transform',
            response.request_headers)

      # Verify no Lo-Fi previews info bar recorded
      histogram = test_driver.GetHistogram('Previews.InfoBarAction.LoFi', 5)
      self.assertEqual(histogram, {})

  # Checks that LoFi images are NOT served when the network quality estimator
  # returns fast connection.
  @ChromeVersionEqualOrAfterM(65)
  def testLoFiFastConnection(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      # Disable server experiments such as tamper detection.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/4G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if response.url.endswith('html'):
          # Main resource should accept transforms but not be transformed.
          self.assertEqual('lite-page',
            response.request_headers['chrome-proxy-accept-transform'])
          self.assertNotIn('chrome-proxy-content-transform',
            response.response_headers)
          if 'chrome-proxy' in response.response_headers:
            self.assertNotIn('page-policies',
                             response.response_headers['chrome-proxy'])
        else:
          # No subresources should accept transforms.
          self.assertNotIn('chrome-proxy-accept-transform',
            response.request_headers)

      # Verify no Lo-Fi previews info bar recorded
      histogram = test_driver.GetHistogram('Previews.InfoBarAction.LoFi', 5)
      self.assertEqual(histogram, {})

  # Checks that LoFi images are not served, but the if-heavy CPAT header is
  # added when LoFi slow connections are used and the network quality estimator
  # returns 4G.
  # If-heavy stopped being added in M61.
  @ChromeVersionBeforeM(61)
  def testLoFiIfHeavyFastConnection(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=slow-connections-'
                               'only')
      # Disable server experiments.
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')

      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/4G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      non_lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        self.assertIn('chrome-proxy-accept-transform', response.request_headers)
        actual_cpat_headers = \
          response.request_headers['chrome-proxy-accept-transform'].split(';')
        self.assertIn('empty-image', actual_cpat_headers)
        self.assertIn('if-heavy', actual_cpat_headers)
        if (not self.checkLoFiResponse(response, False)):
          non_lofi_responses = non_lofi_responses + 1

      # Verify that non Lo-Fi image responses were seen.
      self.assertNotEqual(0, non_lofi_responses)

  # Checks that Lo-Fi placeholder images are not loaded from cache on page
  # reloads when Lo-Fi mode is disabled or data reduction proxy is disabled.
  # First a test page is opened with Lo-Fi and chrome proxy enabled. This allows
  # Chrome to cache the Lo-Fi placeholder image. The browser is restarted with
  # chrome proxy disabled and the same test page is loaded. This second page
  # load should not pick the Lo-Fi placeholder from cache and original image
  # should be loaded. Finally, the browser is restarted with chrome proxy
  # enabled and Lo-Fi disabled and the same test page is loaded. This third page
  # load should not pick the Lo-Fi placeholder from cache and original image
  # should be loaded.
  @ChromeVersionBeforeM(65)
  def testLoFiCacheBypassOldFlags(self):
    # If it was attempted to run with another experiment, skip this test.
    if common.ParseFlags().browser_args and ('--data-reduction-proxy-experiment'
        in common.ParseFlags().browser_args):
      self.skipTest('This test cannot be run with other experiments.')
    with TestDriver() as test_driver:
      # First page load, enable Lo-Fi and chrome proxy. Disable server
      # experiments such as tamper detection. This test should be run with
      # --profile-type=default command line for the same user profile and cache
      # to be used across the two page loads.
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=always-on')
      test_driver.AddChromeArg('--profile-type=default')
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')

      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        if (self.checkLoFiResponse(response, True)):
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

      # Second page load with the chrome proxy off.
      test_driver._StopDriver()
      test_driver.RemoveChromeArg('--enable-spdy-proxy-auth')
      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        responses = responses + 1
        self.assertNotHasChromeProxyViaHeader(response)
        self.checkLoFiResponse(response, False)

      # Verify that responses were seen.
      self.assertNotEqual(0, responses)

      # Third page load with the chrome proxy on and Lo-Fi off.
      test_driver._StopDriver()
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.RemoveChromeArg('--data-reduction-proxy-lo-fi=always-on')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=disabled')
      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        responses = responses + 1
        self.assertHasChromeProxyViaHeader(response)
        self.checkLoFiResponse(response, False)

      # Verify that responses were seen.
      self.assertNotEqual(0, responses)

  # Checks that Lo-Fi placeholder images are not loaded from cache on page
  # reloads when Lo-Fi mode is disabled or data reduction proxy is disabled.
  # First a test page is opened with Lo-Fi and chrome proxy enabled. This allows
  # Chrome to cache the Lo-Fi placeholder image. The browser is restarted with
  # chrome proxy disabled and the same test page is loaded. This second page
  # load should not pick the Lo-Fi placeholder from cache and original image
  # should be loaded. Finally, the browser is restarted with chrome proxy
  # enabled and Lo-Fi disabled and the same test page is loaded. This third page
  # load should not pick the Lo-Fi placeholder from cache and original image
  # should be loaded.
  @ChromeVersionEqualOrAfterM(65)
  def testLoFiCacheBypass(self):
    # If it was attempted to run with another experiment, skip this test.
    if common.ParseFlags().browser_args and ('--data-reduction-proxy-experiment'
        in common.ParseFlags().browser_args):
      self.skipTest('This test cannot be run with other experiments.')
    with TestDriver() as test_driver:
      # First page load, enable Lo-Fi and chrome proxy. Disable server
      # experiments such as tamper detection. This test should be run with
      # --profile-type=default command line for the same user profile and cache
      # to be used across the two page loads.
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--profile-type=default')
      test_driver.AddChromeArg('--data-reduction-proxy-server-experiments-'
                               'disabled')
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/Slow2G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        if (self.checkLoFiResponse(response, True)):
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

      # Second page load with the chrome proxy off.
      test_driver._StopDriver()
      test_driver.RemoveChromeArg('--enable-spdy-proxy-auth')
      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        responses = responses + 1
        self.assertNotHasChromeProxyViaHeader(response)
        self.checkLoFiResponse(response, False)

      # Verify that responses were seen.
      self.assertNotEqual(0, responses)

      # Third page load with the chrome proxy on and Lo-Fi off.
      test_driver._StopDriver()
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.RemoveChromeArg('--enable-features='
                                  'DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--disable-features='
                               'DataReductionProxyDecidesTransform')
      test_driver.LoadURL('http://check.googlezip.net/cacheable/test.html')

      responses = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('png'):
          continue
        if not response.request_headers:
          continue
        responses = responses + 1
        self.assertHasChromeProxyViaHeader(response)
        self.checkLoFiResponse(response, False)

      # Verify that responses were seen.
      self.assertNotEqual(0, responses)

  # Checks that LoFi images are served and the force empty image experiment
  # directive is provided when LoFi is always-on without Lite Pages enabled.
  @ChromeVersionEqualOrAfterM(61)
  @ChromeVersionBeforeM(65)
  def testLoFiForcedExperimentOldFlags(self):
    # If it was attempted to run with another experiment, skip this test.
    if common.ParseFlags().browser_args and ('--data-reduction-proxy-experiment'
        in common.ParseFlags().browser_args):
      self.skipTest('This test cannot be run with other experiments.')
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg('--data-reduction-proxy-lo-fi=always-on')

      # Ensure fast network (4G) to ensure force flag ignores ECT.
      test_driver.AddChromeArg('--force-fieldtrial-params='
                               'NetworkQualityEstimator.Enabled:'
                               'force_effective_connection_type/4G')
      test_driver.AddChromeArg('--force-fieldtrials=NetworkQualityEstimator/'
                               'Enabled')

      test_driver.LoadURL('http://check.googlezip.net/static/index.html')

      lofi_responses = 0
      for response in test_driver.GetHTTPResponses():
        self.assertIn('exp=force_page_policies_empty_image',
          response.request_headers['chrome-proxy'])
        if response.url.endswith('html'):
          # Verify that the server provides the fallback directive
          self.assertIn('chrome-proxy', response.response_headers)
          self.assertIn('page-policies=empty-image',
                        response.response_headers['chrome-proxy'])
          continue
        if response.url.endswith('png'):
          self.checkLoFiResponse(response, True)
          lofi_responses = lofi_responses + 1

      # Verify that Lo-Fi responses were seen.
      self.assertNotEqual(0, lofi_responses)

  # Checks that Client LoFi resource requests have the Intervention header.
  @ChromeVersionEqualOrAfterM(61)
  def testClientLoFiInterventionHeader(self):
    with TestDriver() as test_driver:
      test_driver.AddChromeArg('--enable-spdy-proxy-auth')
      test_driver.AddChromeArg('--enable-features='
                               'Previews,DataReductionProxyDecidesTransform')
      test_driver.AddChromeArg(
          '--force-fieldtrial-params=NetworkQualityEstimator.Enabled:'
          'force_effective_connection_type/2G,'
          'PreviewsClientLoFi.Enabled:'
          'max_allowed_effective_connection_type/4G')
      test_driver.AddChromeArg(
          '--force-fieldtrials=NetworkQualityEstimator/Enabled/'
          'PreviewsClientLoFi/Enabled')

      test_driver.LoadURL('https://check.googlezip.net/static/index.html')

      intervention_headers = 0
      for response in test_driver.GetHTTPResponses():
        if not response.url.endswith('html'):
          self.assertIn('intervention', response.request_headers)
          intervention_headers = intervention_headers + 1

      self.assertNotEqual(0, intervention_headers)

if __name__ == '__main__':
  IntegrationTest.RunAllTests()
