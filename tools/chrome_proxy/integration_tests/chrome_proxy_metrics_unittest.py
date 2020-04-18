# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import unittest

from common import chrome_proxy_metrics as common_metrics
from common import network_metrics_unittest as network_unittest
from integration_tests import chrome_proxy_metrics as metrics
from telemetry.testing import test_page_test_results

TEST_EXTRA_VIA_HEADER = '1.1 EXTRA_VIA_HEADER'

# Timeline events used in tests.
# An HTML not via proxy.
EVENT_HTML_DIRECT = network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.html1',
    response_headers={
        'Content-Type': 'text/html',
        'Content-Length': str(len(network_unittest.HTML_BODY)),
        },
    body=network_unittest.HTML_BODY)

# A BlockOnce response not via proxy.
EVENT_HTML_BLOCKONCE = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://check.googlezip.net/blocksingle/',
    response_headers={
        'Content-Type': 'text/html',
        'Content-Length': str(len(network_unittest.HTML_BODY)),
        },
    body=network_unittest.HTML_BODY))

# An HTML via proxy.
EVENT_HTML_PROXY_VIA = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.html2',
    response_headers={
        'Content-Type': 'text/html',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(len(network_unittest.HTML_BODY)),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    body=network_unittest.HTML_BODY,
    remote_port=443))

# An HTML via proxy with extra header.
EVENT_HTML_PROXY_EXTRA_VIA = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.html2',
    response_headers={
        'Content-Type': 'text/html',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(len(network_unittest.HTML_BODY)),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER + ", " +
        TEST_EXTRA_VIA_HEADER,
        },
    body=network_unittest.HTML_BODY,
    remote_port=443))

# An HTML via the HTTP fallback proxy.
EVENT_HTML_PROXY_VIA_HTTP_FALLBACK = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.html2',
    response_headers={
        'Content-Type': 'text/html',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(len(network_unittest.HTML_BODY)),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    body=network_unittest.HTML_BODY,
    remote_port=80))

# An image via proxy with Via header.
EVENT_IMAGE_PROXY_VIA = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Content-Type': 'image/jpeg',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(network_unittest.IMAGE_OCL),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    body=base64.b64encode(network_unittest.IMAGE_BODY),
    base64_encoded_body=True,
    remote_port=443))

# An image via the HTTP fallback proxy.
EVENT_IMAGE_PROXY_VIA_HTTP_FALLBACK = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Content-Type': 'image/jpeg',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(network_unittest.IMAGE_OCL),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    body=base64.b64encode(network_unittest.IMAGE_BODY),
    base64_encoded_body=True,
    remote_port=80))

# An image via proxy with Via header and it is cached.
EVENT_IMAGE_PROXY_CACHED = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Content-Type': 'image/jpeg',
        'Content-Encoding': 'gzip',
        'X-Original-Content-Length': str(network_unittest.IMAGE_OCL),
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    body=base64.b64encode(network_unittest.IMAGE_BODY),
    base64_encoded_body=True,
    served_from_cache=True))

# An image fetched directly.
EVENT_IMAGE_DIRECT = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Content-Type': 'image/jpeg',
        'Content-Encoding': 'gzip',
        },
    body=base64.b64encode(network_unittest.IMAGE_BODY),
    base64_encoded_body=True))

# A safe-browsing malware response.
EVENT_MALWARE_PROXY = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.malware',
    response_headers={
        'X-Malware-Url': '1',
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        'Location': 'http://test.malware',
        },
    status=307))

# An HTML via proxy with the Via header.
EVENT_IMAGE_BYPASS = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Chrome-Proxy': 'bypass=1',
        'Content-Type': 'text/html',
        'Via': '1.1 ' + common_metrics.CHROME_PROXY_VIA_HEADER,
        },
    status=502))

# An image fetched directly.
EVENT_IMAGE_DIRECT = (
    network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
    url='http://test.image',
    response_headers={
        'Content-Type': 'image/jpeg',
        'Content-Encoding': 'gzip',
        },
    body=base64.b64encode(network_unittest.IMAGE_BODY),
    base64_encoded_body=True))


class ChromeProxyMetricTest(unittest.TestCase):

  _test_proxy_info = {}

  def _StubGetProxyInfo(self, info):
    def stub(unused_tab, unused_url=''):  # pylint: disable=W0613
      return ChromeProxyMetricTest._test_proxy_info
    metrics.GetProxyInfoFromNetworkInternals = stub
    ChromeProxyMetricTest._test_proxy_info = info

  def testChromeProxyMetricForHeaderValidation(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([
        EVENT_HTML_DIRECT,
        EVENT_HTML_PROXY_VIA,
        EVENT_IMAGE_PROXY_CACHED,
        EVENT_IMAGE_DIRECT])

    results = test_page_test_results.TestPageTestResults(self)

    missing_via_exception = False
    try:
      metric.AddResultsForHeaderValidation(None, results)
    except common_metrics.ChromeProxyMetricException:
      missing_via_exception = True
    # Only the HTTP image response does not have a valid Via header.
    self.assertTrue(missing_via_exception)

    # Two events with valid Via headers.
    metric.SetEvents([
        EVENT_HTML_PROXY_VIA,
        EVENT_IMAGE_PROXY_CACHED])
    metric.AddResultsForHeaderValidation(None, results)
    results.AssertHasPageSpecificScalarValue('checked_via_header', 'count', 2)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForHeaderValidation(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

  def testChromeProxyMetricForExtraViaHeader(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_HTML_DIRECT,
                      EVENT_HTML_PROXY_EXTRA_VIA])
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResultsForExtraViaHeader(None, results, TEST_EXTRA_VIA_HEADER)
    # The direct page should not count an extra via header, but should also not
    # throw an exception.
    results.AssertHasPageSpecificScalarValue('extra_via_header', 'count', 1)

    metric.SetEvents([EVENT_HTML_PROXY_VIA])
    exception_occurred = False
    try:
      metric.AddResultsForExtraViaHeader(None, results, TEST_EXTRA_VIA_HEADER)
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # The response had the chrome proxy via header, but not the extra expected
    # via header.
    self.assertTrue(exception_occurred)

  def testChromeProxyMetricForBypass(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([
        EVENT_HTML_DIRECT,
        EVENT_HTML_PROXY_VIA,
        EVENT_IMAGE_PROXY_CACHED,
        EVENT_IMAGE_DIRECT])
    results = test_page_test_results.TestPageTestResults(self)

    bypass_exception = False
    try:
      metric.AddResultsForBypass(None, results)
    except common_metrics.ChromeProxyMetricException:
      bypass_exception = True
    # Two of the first three events have Via headers.
    self.assertTrue(bypass_exception)

    # Use directly fetched image only. It is treated as bypassed.
    metric.SetEvents([EVENT_IMAGE_DIRECT])
    metric.AddResultsForBypass(None, results)
    results.AssertHasPageSpecificScalarValue('bypass', 'count', 1)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForBypass(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

  def testChromeProxyMetricForCorsBypass(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_HTML_PROXY_VIA,
                      EVENT_IMAGE_BYPASS,
                      EVENT_IMAGE_DIRECT])
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResultsForCorsBypass(None, results)
    results.AssertHasPageSpecificScalarValue('cors_bypass', 'count', 1)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForCorsBypass(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

  def testChromeProxyMetricForBlockOnce(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_HTML_BLOCKONCE,
                      EVENT_HTML_BLOCKONCE,
                      EVENT_IMAGE_PROXY_VIA])
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResultsForBlockOnce(None, results)
    results.AssertHasPageSpecificScalarValue('eligible_responses', 'count', 2)

    metric.SetEvents([EVENT_HTML_BLOCKONCE,
                      EVENT_HTML_BLOCKONCE,
                      EVENT_IMAGE_DIRECT])
    exception_occurred = False
    try:
      metric.AddResultsForBlockOnce(None, results)
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # The second response was over direct, but was expected via proxy.
    self.assertTrue(exception_occurred)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForBlockOnce(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

  def testChromeProxyMetricForSafebrowsingOn(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_MALWARE_PROXY])
    results = test_page_test_results.TestPageTestResults(self)

    metric.AddResultsForSafebrowsingOn(None, results)
    results.AssertHasPageSpecificScalarValue(
        'safebrowsing', 'timeout responses', 1)

    # Clear results and metrics to test no response for safebrowsing
    results = test_page_test_results.TestPageTestResults(self)
    metric.SetEvents([])
    metric.AddResultsForSafebrowsingOn(None, results)
    results.AssertHasPageSpecificScalarValue(
        'safebrowsing', 'timeout responses', 1)

  def testChromeProxyMetricForHTTPFallback(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_HTML_PROXY_VIA_HTTP_FALLBACK,
                      EVENT_IMAGE_PROXY_VIA_HTTP_FALLBACK])
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResultsForHTTPFallback(None, results)
    results.AssertHasPageSpecificScalarValue('via_fallback', 'count', 2)

    metric.SetEvents([EVENT_HTML_PROXY_VIA,
                      EVENT_IMAGE_PROXY_VIA])
    exception_occurred = False
    try:
      metric.AddResultsForHTTPFallback(None, results)
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # The responses came through the SPDY proxy, but were expected through the
    # HTTP fallback proxy.
    self.assertTrue(exception_occurred)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForHTTPFallback(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

  def testChromeProxyMetricForHTTPToDirectFallback(self):
    metric = metrics.ChromeProxyMetric()
    metric.SetEvents([EVENT_HTML_PROXY_VIA_HTTP_FALLBACK,
                      EVENT_HTML_DIRECT,
                      EVENT_IMAGE_DIRECT])
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResultsForHTTPToDirectFallback(None, results, 'test.html2')
    results.AssertHasPageSpecificScalarValue('via_fallback', 'count', 1)
    results.AssertHasPageSpecificScalarValue('bypass', 'count', 2)

    metric.SetEvents([EVENT_HTML_PROXY_VIA,
                      EVENT_HTML_DIRECT])
    exception_occurred = False
    try:
      metric.AddResultsForHTTPToDirectFallback(None, results, 'test.html2')
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # The first response was expected through the HTTP fallback proxy.
    self.assertTrue(exception_occurred)

    metric.SetEvents([EVENT_HTML_PROXY_VIA_HTTP_FALLBACK,
                      EVENT_HTML_PROXY_VIA_HTTP_FALLBACK,
                      EVENT_IMAGE_PROXY_VIA_HTTP_FALLBACK])
    exception_occurred = False
    try:
      metric.AddResultsForHTTPToDirectFallback(None, results, 'test.html2')
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # All but the first response were expected to be over direct.
    self.assertTrue(exception_occurred)

    metric.SetEvents([EVENT_HTML_DIRECT,
                      EVENT_HTML_DIRECT,
                      EVENT_IMAGE_DIRECT])
    exception_occurred = False
    try:
      metric.AddResultsForHTTPToDirectFallback(None, results, 'test.html2')
    except common_metrics.ChromeProxyMetricException:
      exception_occurred = True
    # The first response was expected through the HTTP fallback proxy.
    self.assertTrue(exception_occurred)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForHTTPToDirectFallback(None, results, 'test.html2')
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

