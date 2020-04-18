# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import unittest

import metrics
import request_track
import test_utils


class MetricsTestCase(unittest.TestCase):
  _BODY_SIZE = 14187
  _URL = 'http://www.example.com/'
  _REQUEST_HEADERS_SIZE = (len(_URL) + len('GET ') + 2
                           + len('Accept: Everything\r\n'))
  _RESPONSE_HEADERS_SIZE = 124
  _REQUEST = {
      'encoded_data_length': _BODY_SIZE,
      'request_id': '2291.1',
      'request_headers': {
          'Accept': 'Everything',
      },
      'response_headers': {
          'Age': '866',
          'Content-Length': str(_BODY_SIZE),
          'Etag': 'ABCD',
          'Date': 'Fri, 22 Apr 2016 08:56:19 -0200',
          'Vary': 'Accept-Encoding',
      },
      'timestamp': 5535648.730768,
      'timing': {
          'receive_headers_end': 47.0650000497699,
          'request_time': 5535648.73264,
      },
      'url': _URL,
      'status': 200,
      'wall_time': 1461322579.59422}

  def testTransferredDataRevisitNoCache(self):
    trace = self._MakeTrace()
    (uploaded, downloaded) = metrics.TransferredDataRevisit(trace, 10)
    self.assertEqual(self._REQUEST_HEADERS_SIZE, uploaded)
    self.assertEqual(self._BODY_SIZE + self._RESPONSE_HEADERS_SIZE, downloaded)

  def testTransferredDataRevisitNoCacheAssumeValidates(self):
    trace = self._MakeTrace()
    (uploaded, downloaded) = metrics.TransferredDataRevisit(trace, 10, True)
    self.assertEqual(self._REQUEST_HEADERS_SIZE, uploaded)
    not_modified_length = len('HTTP/1.1 304 NOT MODIFIED\r\n')
    self.assertEqual(not_modified_length, downloaded)

  def testTransferredDataRevisitCacheable(self):
    trace = self._MakeTrace()
    r = trace.request_track.GetEvents()[0]
    r.response_headers['Cache-Control'] = 'max-age=1000'
    (uploaded, downloaded) = metrics.TransferredDataRevisit(trace, 10)
    self.assertEqual(0, uploaded)
    self.assertEqual(0, downloaded)
    (uploaded, downloaded) = metrics.TransferredDataRevisit(trace, 1000)
    self.assertEqual(self._REQUEST_HEADERS_SIZE, uploaded)
    cache_control_length = len('Cache-Control: max-age=1000\r\n')
    self.assertEqual(
        self._BODY_SIZE + self._RESPONSE_HEADERS_SIZE + cache_control_length,
        downloaded)

  def testTransferSize(self):
    trace = self._MakeTrace()
    r = trace.request_track.GetEvents()[0]
    (_, downloaded) = metrics.TransferSize([r])
    self.assertEqual(self._BODY_SIZE + self._RESPONSE_HEADERS_SIZE,
                     downloaded)

  def testDnsRequestsAndCost(self):
    trace = self._MakeTrace()
    (count, cost) = metrics.DnsRequestsAndCost(trace)
    self.assertEqual(0, count)
    self.assertEqual(0, cost)
    r = trace.request_track.GetEvents()[0]
    r.timing.dns_end = 12
    r.timing.dns_start = 4
    (count, cost) = metrics.DnsRequestsAndCost(trace)
    self.assertEqual(1, count)
    self.assertEqual(8, cost)

  def testConnectionMetrics(self):
    requests = [request_track.Request.FromJsonDict(copy.deepcopy(self._REQUEST))
                for _ in xrange(3)]
    requests[0].url = 'http://chromium.org/'
    requests[0].protocol = 'http/1.1'
    requests[0].timing.connect_start = 12
    requests[0].timing.connect_end = 42
    requests[0].timing.ssl_start = 50
    requests[0].timing.ssl_end = 70
    requests[1].url = 'https://chromium.org/where-am-i/'
    requests[1].protocol = 'h2'
    requests[1].timing.connect_start = 22
    requests[1].timing.connect_end = 73
    requests[2].url = 'http://www.chromium.org/here/'
    requests[2].protocol = 'http/42'
    trace = test_utils.LoadingTraceFromEvents(requests)
    stats = metrics.ConnectionMetrics(trace)
    self.assertEqual(2, stats['connections'])
    self.assertEqual(81, stats['connection_cost_ms'])
    self.assertEqual(1, stats['ssl_connections'])
    self.assertEqual(20, stats['ssl_cost_ms'])
    self.assertEqual(1, stats['http11_requests'])
    self.assertEqual(1, stats['h2_requests'])
    self.assertEqual(0, stats['data_requests'])
    self.assertEqual(2, stats['domains'])

  @classmethod
  def _MakeTrace(cls):
    request = request_track.Request.FromJsonDict(copy.deepcopy(cls._REQUEST))
    return test_utils.LoadingTraceFromEvents([request])


if __name__ == '__main__':
  unittest.main()
