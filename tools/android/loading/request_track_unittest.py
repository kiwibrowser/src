# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import json
import sys
import unittest

from request_track import (TimeBetween, Request, CachingPolicy, RequestTrack,
                           Timing, _ParseStringToInt)


class TimeBetweenTestCase(unittest.TestCase):
  _REQUEST = Request.FromJsonDict({'url': 'http://bla.com',
                                   'request_id': '1234.1',
                                   'frame_id': '123.1',
                                   'initiator': {'type': 'other'},
                                   'timestamp': 2,
                                   'timing': {}})
  def setUp(self):
    super(TimeBetweenTestCase, self).setUp()
    self.first = copy.deepcopy(self._REQUEST)
    self.first.timing = Timing.FromDevToolsDict({'requestTime': 123456,
                                                 'receiveHeadersEnd': 100,
                                                 'loadingFinished': 500})
    self.second = copy.deepcopy(self._REQUEST)
    self.second.timing = Timing.FromDevToolsDict({'requestTime': 123456 + 1,
                                                  'receiveHeadersEnd': 200,
                                                  'loadingFinished': 600})

  def testTimeBetweenParser(self):
    self.assertEquals(900, TimeBetween(self.first, self.second, 'parser'))

  def testTimeBetweenScript(self):
    self.assertEquals(500, TimeBetween(self.first, self.second, 'script'))


class RequestTestCase(unittest.TestCase):
  def testContentType(self):
    r = Request()
    r.response_headers = {}
    self.assertEquals(None, r.GetContentType())
    r.response_headers = {'Content-Type': 'application/javascript'}
    self.assertEquals('application/javascript', r.GetContentType())
    # Case-insensitive match.
    r.response_headers = {'content-type': 'application/javascript'}
    self.assertEquals('application/javascript', r.GetContentType())
    # Parameters are filtered out.
    r.response_headers = {'Content-Type': 'application/javascript;bla'}
    self.assertEquals('application/javascript', r.GetContentType())
    # MIME type takes precedence over 'Content-Type' header.
    r.mime_type = 'image/webp'
    self.assertEquals('image/webp', r.GetContentType())
    r.mime_type = None
    # Test for 'ping' type.
    r.status = 204
    self.assertEquals('ping', r.GetContentType())
    r.status = None
    r.response_headers = {'Content-Type': 'application/javascript',
                          'content-length': '0'}
    self.assertEquals('ping', r.GetContentType())
    # Test for 'redirect' type.
    r.response_headers = {'Content-Type': 'application/javascript',
                          'location': 'http://foo',
                          'content-length': '0'}
    self.assertEquals('redirect', r.GetContentType())

  def testGetHTTPResponseHeader(self):
    r = Request()
    r.response_headers = {}
    self.assertEquals(None, r.GetHTTPResponseHeader('Foo'))
    r.response_headers = {'Foo': 'Bar', 'Baz': 'Foo'}
    self.assertEquals('Bar', r.GetHTTPResponseHeader('Foo'))
    r.response_headers = {'foo': 'Bar', 'Baz': 'Foo'}
    self.assertEquals('Bar', r.GetHTTPResponseHeader('Foo'))

  def testGetRawResponseHeaders(self):
    r = Request()
    r.protocol = 'http/1.1'
    r.status = 200
    r.status_text = 'Hello world'
    r.response_headers = {'Foo': 'Bar', 'Baz': 'Foo'}
    self.assertEquals('HTTP/1.1 200 Hello world\x00Baz: Foo\x00Foo: Bar\x00',
                      r.GetRawResponseHeaders())


class ParseStringToIntTestCase(unittest.TestCase):
  def runTest(self):
    MININT = -sys.maxint - 1
    # Same test cases as in string_number_conversions_unittest.cc
    CASES = [
        ("0", 0),
        ("42", 42),
        ("-2147483648", -2147483648),
        ("2147483647", 2147483647),
        ("-2147483649", -2147483649),
        ("-99999999999", -99999999999),
        ("2147483648", 2147483648),
        ("99999999999", 99999999999),
        ("9223372036854775807", sys.maxint),
        ("-9223372036854775808", MININT),
        ("09", 9),
        ("-09", -9),
        ("", 0),
        (" 42", 42),
        ("42 ", 42),
        ("0x42", 0),
        ("\t\n\v\f\r 42", 42),
        ("blah42", 0),
        ("42blah", 42),
        ("blah42blah", 0),
        ("-273.15", -273),
        ("+98.6", 98),
        ("--123", 0),
        ("++123", 0),
        ("-+123", 0),
        ("+-123", 0),
        ("-", 0),
        ("-9223372036854775809", MININT),
        ("-99999999999999999999", MININT),
        ("9223372036854775808", sys.maxint),
        ("99999999999999999999", sys.maxint)]
    for string, expected_int in CASES:
      parsed_int = _ParseStringToInt(string)
      self.assertEquals(expected_int, parsed_int)


class CachingPolicyTestCase(unittest.TestCase):
  _REQUEST = {
      'encoded_data_length': 14726,
      'request_id': '2291.1',
      'response_headers': {
          'Age': '866',
          'Content-Length': '14187',
          'Date': 'Fri, 22 Apr 2016 08:56:19 -0200',
          'Vary': 'Accept-Encoding',
      },
      'timestamp': 5535648.730768,
      'timing': {
          'connect_end': 34.0510001406074,
          'connect_start': 21.6859998181462,
          'dns_end': 21.6859998181462,
          'dns_start': 0,
          'loading_finished': 58.76399949193001,
          'receive_headers_end': 47.0650000497699,
          'request_time': 5535648.73264,
          'send_end': 34.6099995076656,
          'send_start': 34.2979999259114
      },
      'url': 'http://www.example.com/',
      'status': 200,
      'wall_time': 1461322579.59422}

  def testHasValidators(self):
    r = self._MakeRequest()
    self.assertFalse(CachingPolicy(r).HasValidators())
    r.response_headers['Last-Modified'] = 'Yesterday all my troubles'
    self.assertTrue(CachingPolicy(r).HasValidators())
    r = self._MakeRequest()
    r.response_headers['ETAG'] = 'ABC'
    self.assertTrue(CachingPolicy(r).HasValidators())

  def testIsCacheable(self):
    r = self._MakeRequest()
    self.assertTrue(CachingPolicy(r).IsCacheable())
    r.response_headers['Cache-Control'] = 'Whatever,no-store'
    self.assertFalse(CachingPolicy(r).IsCacheable())

  def testPolicyNoStore(self):
    r = self._MakeRequest()
    r.response_headers['Cache-Control'] = 'Whatever,no-store'
    self.assertEqual(CachingPolicy.FETCH, CachingPolicy(r).PolicyAtDate(0))

  def testPolicyMaxAge(self):
    r = self._MakeRequest()
    r.response_headers['Cache-Control'] = 'whatever,max-age=  1000,whatever'
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,
        CachingPolicy(r).PolicyAtDate(r.wall_time))
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 10000))
    # Take current age into account.
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 500))
    # Max-Age before Expires.
    r.response_headers['Expires'] = 'Thu, 21 Apr 2016 00:00:00 -0200'
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,
        CachingPolicy(r).PolicyAtDate(r.wall_time))
    # Max-Age < age
    r.response_headers['Cache-Control'] = 'whatever,max-age=100crap,whatever'
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 2))

  def testPolicyExpires(self):
    r = self._MakeRequest()
    # Already expired
    r.response_headers['Expires'] = 'Thu, 21 Apr 2016 00:00:00 -0200'
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time))
    r.response_headers['Expires'] = 'Thu, 25 Apr 2016 00:00:00 -0200'
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,\
        CachingPolicy(r).PolicyAtDate(r.wall_time))
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 86400))
    self.assertEqual(CachingPolicy.VALIDATION_SYNC,
                     CachingPolicy(r).PolicyAtDate(r.wall_time + 86400 * 5))

  def testStaleWhileRevalidate(self):
    r = self._MakeRequest()
    r.response_headers['Cache-Control'] = (
        'whatever,max-age=1000,stale-while-revalidate=2000')
    self.assertEqual(
        CachingPolicy.VALIDATION_ASYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 200))
    self.assertEqual(
        CachingPolicy.VALIDATION_ASYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 2000))
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 3100))
    # must-revalidate overrides stale-while-revalidate.
    r.response_headers['Cache-Control'] += ',must-revalidate'
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 200))

  def test301NeverExpires(self):
    r = self._MakeRequest()
    r.status = 301
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 2000))

  def testLastModifiedHeuristic(self):
    r = self._MakeRequest()
    # 8 hours ago.
    r.response_headers['Last-Modified'] = 'Fri, 22 Apr 2016 00:56:19 -0200'
    del r.response_headers['Age']
    self.assertEqual(
        CachingPolicy.VALIDATION_NONE,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 60))
    self.assertEqual(
        CachingPolicy.VALIDATION_SYNC,
        CachingPolicy(r).PolicyAtDate(r.wall_time + 3600))

  @classmethod
  def _MakeRequest(cls):
    return Request.FromJsonDict(copy.deepcopy(cls._REQUEST))


class RequestTrackTestCase(unittest.TestCase):
  _REQUEST_WILL_BE_SENT = {
      'method': 'Network.requestWillBeSent',
      'params': {
          'documentURL': 'http://example.com/',
          'frameId': '32493.1',
          'initiator': {
              'type': 'other'
              },
          'loaderId': '32493.3',
          'request': {
              'headers': {
                  'Accept': 'text/html',
                  'Upgrade-Insecure-Requests': '1',
                  'User-Agent': 'Mozilla/5.0'
                  },
              'initialPriority': 'VeryHigh',
              'method': 'GET',
              'mixedContentType': 'none',
              'url': 'http://example.com/'
              },
          'requestId': '32493.1',
          'timestamp': 5571441.535053,
          'type': 'Document',
          'wallTime': 1452691674.08878}}
  _REDIRECT = {
      'method': 'Network.requestWillBeSent',
      'params': {
          'documentURL': 'http://www.example.com/',
          'frameId': '32493.1',
          'initiator': {
              'type': 'other'
              },
          'loaderId': '32493.3',
          'redirectResponse': {
              'connectionId': 18,
              'connectionReused': False,
              'encodedDataLength': 198,
              'fromDiskCache': False,
              'fromServiceWorker': False,
              'headers': {},
              'headersText': 'HTTP/1.1 301 Moved Permanently\r\n',
              'mimeType': 'text/html',
              'protocol': 'http/1.1',
              'remoteIPAddress': '216.146.46.10',
              'remotePort': 80,
              'requestHeaders': {
                  'Accept': 'text/html',
                  'User-Agent': 'Mozilla/5.0'
                  },
              'securityState': 'neutral',
              'status': 301,
              'statusText': 'Moved Permanently',
              'timing': {
                  'connectEnd': 137.435999698937,
                  'connectStart': 51.1459996923804,
                  'dnsEnd': 51.1459996923804,
                  'dnsStart': 0,
                  'proxyEnd': -1,
                  'proxyStart': -1,
                  'receiveHeadersEnd': 228.187000378966,
                  'requestTime': 5571441.55002,
                  'sendEnd': 138.841999694705,
                  'sendStart': 138.031999580562,
                  'sslEnd': -1,
                  'sslStart': -1,
                  'workerReady': -1,
                  'workerStart': -1
                  },
              'url': 'http://example.com/'
              },
          'request': {
              'headers': {
                  'Accept': 'text/html',
                  'User-Agent': 'Mozilla/5.0'
                  },
              'initialPriority': 'VeryLow',
              'method': 'GET',
              'mixedContentType': 'none',
              'url': 'http://www.example.com/'
              },
          'requestId': '32493.1',
          'timestamp': 5571441.795948,
          'type': 'Document',
          'wallTime': 1452691674.34968}}
  _RESPONSE_RECEIVED = {
      'method': 'Network.responseReceived',
      'params': {
          'frameId': '32493.1',
          'loaderId': '32493.3',
          'requestId': '32493.1',
          'response': {
              'connectionId': 26,
              'connectionReused': False,
              'encodedDataLength': -1,
              'fromDiskCache': False,
              'fromServiceWorker': False,
              'headers': {
                  'Age': '67',
                  'Cache-Control': 'max-age=0,must-revalidate',
                  },
              'headersText': 'HTTP/1.1 200 OK\r\n',
              'mimeType': 'text/html',
              'protocol': 'http/1.1',
              'requestHeaders': {
                  'Accept': 'text/html',
                    'Host': 'www.example.com',
                    'User-Agent': 'Mozilla/5.0'
                },
                'status': 200,
                'timing': {
                    'connectEnd': 37.9800004884601,
                    'connectStart': 26.8250005319715,
                    'dnsEnd': 26.8250005319715,
                    'dnsStart': 0,
                    'proxyEnd': -1,
                    'proxyStart': -1,
                    'receiveHeadersEnd': 54.9750002101064,
                    'requestTime': 5571441.798671,
                    'sendEnd': 38.3980004116893,
                    'sendStart': 38.1810003891587,
                    'sslEnd': -1,
                    'sslStart': -1,
                    'workerReady': -1,
                    'workerStart': -1
                },
                'url': 'http://www.example.com/'
            },
            'timestamp': 5571441.865639,
            'type': 'Document'}}
  _DATA_RECEIVED_1 = {
      "method": "Network.dataReceived",
      "params": {
          "dataLength": 1803,
          "encodedDataLength": 1326,
          "requestId": "32493.1",
          "timestamp": 5571441.867347}}
  _DATA_RECEIVED_2 = {
      "method": "Network.dataReceived",
      "params": {
          "dataLength": 32768,
          "encodedDataLength": 32768,
          "requestId": "32493.1",
          "timestamp": 5571441.893121}}
  _SERVED_FROM_CACHE = {
      "method": "Network.requestServedFromCache",
      "params": {
          "requestId": "32493.1"}}
  _LOADING_FINISHED = {'method': 'Network.loadingFinished',
                       'params': {
                           'encodedDataLength': 101829,
                           'requestId': '32493.1',
                           'timestamp': 5571441.891189}}
  _LOADING_FAILED = {'method': 'Network.loadingFailed',
                     'params': {
                         'canceled': False,
                         'blockedReason': None,
                         'encodedDataLength': 101829,
                         'errorText': 'net::ERR_TOO_MANY_REDIRECTS',
                         'requestId': '32493.1',
                         'timestamp': 5571441.891189,
                         'type': 'Document'}}

  def setUp(self):
    self.request_track = RequestTrack(None)

  def testParseRequestWillBeSent(self):
    msg = RequestTrackTestCase._REQUEST_WILL_BE_SENT
    request_id = msg['params']['requestId']
    self.request_track.Handle('Network.requestWillBeSent', msg)
    self.assertTrue(request_id in self.request_track._requests_in_flight)
    (_, status) = self.request_track._requests_in_flight[request_id]
    self.assertEquals(RequestTrack._STATUS_SENT, status)

  def testRejectsUnknownMethod(self):
    with self.assertRaises(AssertionError):
      self.request_track.Handle(
          'unknown', RequestTrackTestCase._REQUEST_WILL_BE_SENT)

  def testHandleRedirect(self):
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REDIRECT)
    self.assertEquals(1, len(self.request_track._requests_in_flight))
    self.assertEquals(1, len(self.request_track.GetEvents()))
    redirect_request = self.request_track.GetEvents()[0]
    self.assertTrue(redirect_request.request_id.endswith(
        RequestTrack._REDIRECT_SUFFIX + '.1'))
    request = self.request_track._requests_in_flight.values()[0][0]
    self.assertEquals('redirect', request.initiator['type'])
    self.assertEquals(
        redirect_request.request_id,
        request.initiator[Request.INITIATING_REQUEST])
    self.assertEquals(0, self.request_track.inconsistent_initiators_count)

  def testMultipleRedirects(self):
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REDIRECT)
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REDIRECT)
    self.assertEquals(1, len(self.request_track._requests_in_flight))
    self.assertEquals(2, len(self.request_track.GetEvents()))
    first_redirect_request = self.request_track.GetEvents()[0]
    self.assertTrue(first_redirect_request.request_id.endswith(
        RequestTrack._REDIRECT_SUFFIX + '.1'))
    second_redirect_request = self.request_track.GetEvents()[1]
    self.assertTrue(second_redirect_request.request_id.endswith(
        RequestTrack._REDIRECT_SUFFIX + '.2'))
    self.assertEquals('redirect', second_redirect_request.initiator['type'])
    self.assertEquals(
        first_redirect_request.request_id,
        second_redirect_request.initiator[Request.INITIATING_REQUEST])
    request = self.request_track._requests_in_flight.values()[0][0]
    self.assertEquals('redirect', request.initiator['type'])
    self.assertEquals(
        second_redirect_request.request_id,
        request.initiator[Request.INITIATING_REQUEST])
    self.assertEquals(0, self.request_track.inconsistent_initiators_count)

  def testInconsistentInitiators(self):
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)
    request = copy.deepcopy(RequestTrackTestCase._REDIRECT)
    request['params']['initiator']['type'] = 'script'
    self.request_track.Handle('Network.requestWillBeSent', request)
    self.assertEquals(1, self.request_track.inconsistent_initiators_count)

  def testRejectDuplicates(self):
    msg = RequestTrackTestCase._REQUEST_WILL_BE_SENT
    self.request_track.Handle('Network.requestWillBeSent', msg)
    with self.assertRaises(AssertionError):
      self.request_track.Handle('Network.requestWillBeSent', msg)

  def testIgnoreCompletedDuplicates(self):
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)
    self.request_track.Handle('Network.responseReceived',
                              RequestTrackTestCase._RESPONSE_RECEIVED)
    self.request_track.Handle('Network.loadingFinished',
                              RequestTrackTestCase._LOADING_FINISHED)
    # Should not raise an AssertionError.
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)

  def testSequenceOfGeneratedResponse(self):
    self.request_track.Handle('Network.requestServedFromCache',
                              RequestTrackTestCase._SERVED_FROM_CACHE)
    self.request_track.Handle('Network.loadingFinished',
                              RequestTrackTestCase._LOADING_FINISHED)
    self.assertEquals(0, len(self.request_track.GetEvents()))

  def testInvalidSequence(self):
    msg1 = RequestTrackTestCase._REQUEST_WILL_BE_SENT
    msg2 = RequestTrackTestCase._LOADING_FINISHED
    self.request_track.Handle('Network.requestWillBeSent', msg1)
    with self.assertRaises(AssertionError):
      self.request_track.Handle('Network.loadingFinished', msg2)

  def testValidSequence(self):
    self._ValidSequence(self.request_track)
    self.assertEquals(1, len(self.request_track.GetEvents()))
    self.assertEquals(0, len(self.request_track._requests_in_flight))
    r = self.request_track.GetEvents()[0]
    self.assertEquals('32493.1', r.request_id)
    self.assertEquals('32493.1', r.frame_id)
    self.assertEquals('32493.3', r.loader_id)
    self.assertEquals('http://example.com/', r.document_url)
    self.assertEquals('http://example.com/', r.url)
    self.assertEquals('http/1.1', r.protocol)
    self.assertEquals('GET', r.method)
    response = RequestTrackTestCase._RESPONSE_RECEIVED['params']['response']
    self.assertEquals(response['requestHeaders'], r.request_headers)
    self.assertEquals(response['headers'], r.response_headers)
    self.assertEquals('VeryHigh', r.initial_priority)
    request_will_be_sent = (
        RequestTrackTestCase._REQUEST_WILL_BE_SENT['params'])
    self.assertEquals(request_will_be_sent['timestamp'], r.timestamp)
    self.assertEquals(request_will_be_sent['wallTime'], r.wall_time)
    self.assertEquals(request_will_be_sent['initiator'], r.initiator)
    self.assertEquals(request_will_be_sent['type'], r.resource_type)
    self.assertEquals(False, r.served_from_cache)
    self.assertEquals(False, r.from_disk_cache)
    self.assertEquals(False, r.from_service_worker)
    timing = Timing.FromDevToolsDict(response['timing'])
    loading_finished = RequestTrackTestCase._LOADING_FINISHED['params']
    loading_finished_offset = r._TimestampOffsetFromStartMs(
        loading_finished['timestamp'])
    timing.loading_finished = loading_finished_offset
    self.assertEquals(timing, r.timing)
    self.assertEquals(200, r.status)
    self.assertEquals(
        loading_finished['encodedDataLength'], r.encoded_data_length)
    self.assertEquals(False, r.failed)

  def testDataReceived(self):
    self._ValidSequence(self.request_track)
    self.assertEquals(1, len(self.request_track.GetEvents()))
    r = self.request_track.GetEvents()[0]
    self.assertEquals(2, len(r.data_chunks))
    self.assertEquals(
        RequestTrackTestCase._DATA_RECEIVED_1['params']['encodedDataLength'],
        r.data_chunks[0][1])
    self.assertEquals(
        RequestTrackTestCase._DATA_RECEIVED_2['params']['encodedDataLength'],
        r.data_chunks[1][1])

  def testDuplicatedResponseReceived(self):
    msg1 = RequestTrackTestCase._REQUEST_WILL_BE_SENT
    msg2 = copy.deepcopy(RequestTrackTestCase._RESPONSE_RECEIVED)
    msg2_other_timestamp = copy.deepcopy(msg2)
    msg2_other_timestamp['params']['timestamp'] += 12
    msg2_different = copy.deepcopy(msg2)
    msg2_different['params']['response']['encodedDataLength'] += 1
    self.request_track.Handle('Network.requestWillBeSent', msg1)
    self.request_track.Handle('Network.responseReceived', msg2)
    # Should not raise an AssertionError.
    self.request_track.Handle('Network.responseReceived', msg2)
    self.assertEquals(1, self.request_track.duplicates_count)
    with self.assertRaises(AssertionError):
      self.request_track.Handle('Network.responseReceived', msg2_different)

  def testLoadingFailed(self):
    self.request_track.Handle('Network.requestWillBeSent',
                              RequestTrackTestCase._REQUEST_WILL_BE_SENT)
    self.request_track.Handle('Network.responseReceived',
                              RequestTrackTestCase._RESPONSE_RECEIVED)
    self.request_track.Handle('Network.loadingFailed',
                              RequestTrackTestCase._LOADING_FAILED)
    r = self.request_track.GetEvents()[0]
    self.assertTrue(r.failed)
    self.assertEquals('net::ERR_TOO_MANY_REDIRECTS', r.error_text)

  def testCanSerialize(self):
    self._ValidSequence(self.request_track)
    json_dict = self.request_track.ToJsonDict()
    _ = json.dumps(json_dict)  # Should not raise an exception.

  def testCanDeserialize(self):
    self._ValidSequence(self.request_track)
    self.request_track.duplicates_count = 142
    self.request_track.inconsistent_initiators_count = 123
    json_dict = self.request_track.ToJsonDict()
    request_track = RequestTrack.FromJsonDict(json_dict)
    self.assertEquals(self.request_track, request_track)

  def testMaxAge(self):
    rq = Request()
    self.assertEqual(-1, rq.MaxAge())
    rq.response_headers = {}
    self.assertEqual(-1, rq.MaxAge())
    rq.response_headers[
        'Cache-Control'] = 'private,s-maxage=0,max-age=0,must-revalidate'
    self.assertEqual(0, rq.MaxAge())
    rq.response_headers[
        'Cache-Control'] = 'private,s-maxage=0,no-store,max-age=100'
    self.assertEqual(-1, rq.MaxAge())
    rq.response_headers[
        'Cache-Control'] = 'private,s-maxage=0'
    self.assertEqual(-1, rq.MaxAge())
    # Case-insensitive match.
    rq.response_headers['cache-control'] = 'max-age=600'
    self.assertEqual(600, rq.MaxAge())


  @classmethod
  def _ValidSequence(cls, request_track):
    request_track.Handle(
        'Network.requestWillBeSent', cls._REQUEST_WILL_BE_SENT)
    request_track.Handle('Network.responseReceived', cls._RESPONSE_RECEIVED)
    request_track.Handle('Network.dataReceived', cls._DATA_RECEIVED_1)
    request_track.Handle('Network.dataReceived', cls._DATA_RECEIVED_2)
    request_track.Handle('Network.loadingFinished', cls._LOADING_FINISHED)

if __name__ == '__main__':
  unittest.main()
