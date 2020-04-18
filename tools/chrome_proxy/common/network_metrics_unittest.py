# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import unittest

from common import network_metrics
from telemetry.testing import test_page_test_results
from telemetry.timeline import event


HTML_BODY = """<!DOCTYPE HTML>
  <html>
  <head> </head>
 <body>
 <div id="test"> TEST HTML</div>
 </body>
 </html>"""
IMAGE_BODY = """fake image data"""
GZIPPED_HTML_LEN = network_metrics.HTTPResponse.GetGizppedBodyLength(HTML_BODY)
# Make up original content length for the image.
IMAGE_OCL = 3 * len(IMAGE_BODY)


class NetworkMetricTest(unittest.TestCase):
  @staticmethod
  def MakeNetworkTimelineEvent(
      url, response_headers, body=None, base64_encoded_body=False,
      served_from_cache=False, request_headers=None, status=200,
      remote_port=None):
    if not request_headers:
      request_headers = {}
    e = event.TimelineEvent('network', 'HTTPResponse', 0, 0)
    e.args = {}
    e.args['requestId'] = 0
    e.args['response'] = {
        'status': status,
        'url': url,
        'headers': response_headers,
        'requestHeaders': request_headers,
        'remotePort': remote_port,
        }
    e.args['body'] = body
    e.args['base64_encoded_body'] = base64_encoded_body
    e.args['served_from_cache'] = served_from_cache
    return e

  def testHTTPResponse(self):
    url = 'http://test.url'
    self.assertLess(GZIPPED_HTML_LEN, len(HTML_BODY))

    # A plain text HTML response
    resp = network_metrics.HTTPResponse(self.MakeNetworkTimelineEvent(
        url=url,
        response_headers={
            'Content-Type': 'text/html',
            'Content-Length': str(len(HTML_BODY)),
            },
        body=HTML_BODY))
    self.assertEqual(url, resp.response.url)
    body, base64_encoded = resp.response.GetBody()
    self.assertEqual(HTML_BODY, body)
    self.assertFalse(base64_encoded)
    self.assertEqual('text/html', resp.response.GetHeader('Content-Type'))

    self.assertEqual(len(HTML_BODY), resp.content_length)
    self.assertEqual(None, resp.response.GetHeader('Content-Encoding'))
    self.assertFalse(resp.has_original_content_length)
    self.assertEqual(0.0, resp.data_saving_rate)

    # A gzipped HTML response
    resp = network_metrics.HTTPResponse(self.MakeNetworkTimelineEvent(
        url=url,
        response_headers={
            'Content-Type': 'text/html',
            'Content-Encoding': 'gzip',
            'X-Original-Content-Length': str(len(HTML_BODY)),
            },
        body=HTML_BODY))
    body, base64_encoded = resp.response.GetBody()
    self.assertFalse(base64_encoded)
    self.assertEqual(GZIPPED_HTML_LEN, resp.content_length)
    self.assertEqual('gzip', resp.response.GetHeader('Content-Encoding'))
    self.assertTrue(resp.has_original_content_length)
    self.assertEqual(len(HTML_BODY), resp.original_content_length)
    self.assertEqual(
        float(len(HTML_BODY) - GZIPPED_HTML_LEN) / len(HTML_BODY),
        resp.data_saving_rate)

    # A JPEG image response.
    resp = network_metrics.HTTPResponse(self.MakeNetworkTimelineEvent(
        url='http://test.image',
        response_headers={
            'Content-Type': 'image/jpeg',
            'Content-Encoding': 'gzip',
            'X-Original-Content-Length': str(IMAGE_OCL),
            },
        body=base64.b64encode(IMAGE_BODY),
        base64_encoded_body=True))
    body, base64_encoded = resp.response.GetBody()
    self.assertTrue(base64_encoded)
    self.assertEqual(IMAGE_BODY, base64.b64decode(body))
    self.assertEqual(len(IMAGE_BODY), resp.content_length)
    self.assertTrue(resp.has_original_content_length)
    self.assertEqual(IMAGE_OCL, resp.original_content_length)
    self.assertFalse(resp.response.served_from_cache)
    self.assertEqual(float(IMAGE_OCL - len(IMAGE_BODY)) / IMAGE_OCL,
                     resp.data_saving_rate)

    # A JPEG image response from cache.
    resp = network_metrics.HTTPResponse(self.MakeNetworkTimelineEvent(
        url='http://test.image',
        response_headers={
            'Content-Type': 'image/jpeg',
            'Content-Encoding': 'gzip',
            'X-Original-Content-Length': str(IMAGE_OCL),
            },
        body=base64.b64encode(IMAGE_BODY),
        base64_encoded_body=True,
        served_from_cache=True))
    self.assertEqual(len(IMAGE_BODY), resp.content_length)
    self.assertTrue(resp.has_original_content_length)
    self.assertEqual(IMAGE_OCL, resp.original_content_length)
    # Cached resource has zero saving.
    self.assertTrue(resp.response.served_from_cache)
    self.assertEqual(0.0, resp.data_saving_rate)

  def testNetworkMetricResults(self):
    events = [
        # A plain text HTML.
        self.MakeNetworkTimelineEvent(
            url='http://test.html1',
            response_headers={
                'Content-Type': 'text/html',
                'Content-Length': str(len(HTML_BODY)),
                },
            body=HTML_BODY),
        # A compressed HTML.
        self.MakeNetworkTimelineEvent(
            url='http://test.html2',
            response_headers={
                'Content-Type': 'text/html',
                'Content-Encoding': 'gzip',
                'X-Original-Content-Length': str(len(HTML_BODY)),
                },
            body=HTML_BODY),
        # A base64 encoded image.
        self.MakeNetworkTimelineEvent(
            url='http://test.image',
            response_headers={
                'Content-Type': 'image/jpeg',
                'Content-Encoding': 'gzip',
                'X-Original-Content-Length': str(IMAGE_OCL),
                },
            body=base64.b64encode(IMAGE_BODY),
            base64_encoded_body=True),
        ]
    metric = network_metrics.NetworkMetric()
    metric._events = events
    metric.compute_data_saving = True

    self.assertTrue(len(events), len(list(metric.IterResponses(None))))
    results = test_page_test_results.TestPageTestResults(self)
    metric.AddResults(None, results)

    cl = len(HTML_BODY) + GZIPPED_HTML_LEN + len(IMAGE_BODY)
    results.AssertHasPageSpecificScalarValue('content_length', 'bytes', cl)

    ocl = len(HTML_BODY) + len(HTML_BODY) + IMAGE_OCL
    results.AssertHasPageSpecificScalarValue(
        'original_content_length', 'bytes', ocl)

    saving_percent = float(ocl - cl) * 100/ ocl
    results.AssertHasPageSpecificScalarValue(
        'data_saving', 'percent', saving_percent)
