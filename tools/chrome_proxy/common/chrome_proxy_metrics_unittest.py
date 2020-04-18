# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import unittest

from common import chrome_proxy_metrics as metrics
from common import network_metrics_unittest as network_unittest


class ChromeProxyMetricTest(unittest.TestCase):

  def testChromeProxyResponse(self):
    # An https non-proxy response.
    resp = metrics.ChromeProxyResponse(
        network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
            url='https://test.url',
            response_headers={
                'Content-Type': 'text/html',
                'Content-Length': str(len(network_unittest.HTML_BODY)),
                'Via': 'some other via',
                },
            body=network_unittest.HTML_BODY))
    self.assertFalse(resp.ShouldHaveChromeProxyViaHeader())
    self.assertFalse(resp.HasChromeProxyViaHeader())
    self.assertTrue(resp.IsValidByViaHeader())

    # A proxied JPEG image response
    resp = metrics.ChromeProxyResponse(
        network_unittest.NetworkMetricTest.MakeNetworkTimelineEvent(
            url='http://test.image',
            response_headers={
                'Content-Type': 'image/jpeg',
                'Content-Encoding': 'gzip',
                'Via': '1.1 ' + metrics.CHROME_PROXY_VIA_HEADER,
                'X-Original-Content-Length': str(network_unittest.IMAGE_OCL),
                },
            body=base64.b64encode(network_unittest.IMAGE_BODY),
            base64_encoded_body=True))
    self.assertTrue(resp.ShouldHaveChromeProxyViaHeader())
    self.assertTrue(resp.HasChromeProxyViaHeader())
    self.assertTrue(resp.IsValidByViaHeader())

