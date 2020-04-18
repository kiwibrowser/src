# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import unittest

from common import chrome_proxy_metrics as common_metrics
from common import network_metrics_unittest as network_unittest
from live_tests import chrome_proxy_metrics as metrics
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


class ChromeProxyMetricTest(unittest.TestCase):

  def testChromeProxyMetricForDataSaving(self):
    metric = metrics.ChromeProxyMetric()
    events = [
        EVENT_HTML_DIRECT,
        EVENT_HTML_PROXY_VIA,
        EVENT_IMAGE_PROXY_CACHED,
        EVENT_IMAGE_DIRECT]
    metric.SetEvents(events)

    self.assertTrue(len(events), len(list(metric.IterResponses(None))))
    results = test_page_test_results.TestPageTestResults(self)

    metric.AddResultsForDataSaving(None, results)
    results.AssertHasPageSpecificScalarValue('resources_via_proxy', 'count', 2)
    results.AssertHasPageSpecificScalarValue('resources_from_cache', 'count', 1)
    results.AssertHasPageSpecificScalarValue('resources_direct', 'count', 2)

    # Passing in zero responses should cause a failure.
    metric.SetEvents([])
    no_responses_exception = False
    try:
      metric.AddResultsForDataSaving(None, results)
    except common_metrics.ChromeProxyMetricException:
      no_responses_exception = True
    self.assertTrue(no_responses_exception)

