# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import httplib
import os
import shutil
import tempfile
import unittest

from device_setup import _WprHost
from options import OPTIONS
from trace_test.webserver_test import WebServer
from wpr_backend import WprUrlEntry, WprRequest, ExtractRequestsFromLog


LOADING_DIR = os.path.dirname(__file__)


class MockWprResponse(object):
  def __init__(self, headers):
    self.original_headers = headers

class WprUrlEntryTest(unittest.TestCase):

  @classmethod
  def _CreateWprUrlEntry(cls, headers):
    wpr_response = MockWprResponse(headers)
    return WprUrlEntry('GET http://a.com/', wpr_response)

  def testExtractUrl(self):
    self.assertEquals('http://aa.bb/c',
                      WprUrlEntry._ExtractUrl('GET http://aa.bb/c'))
    self.assertEquals('http://aa.b/c',
                      WprUrlEntry._ExtractUrl('POST http://aa.b/c'))
    self.assertEquals('http://a.bb/c',
                      WprUrlEntry._ExtractUrl('WHATEVER http://a.bb/c'))
    self.assertEquals('https://aa.bb/c',
                      WprUrlEntry._ExtractUrl('GET https://aa.bb/c'))
    self.assertEquals('http://aa.bb',
                      WprUrlEntry._ExtractUrl('GET http://aa.bb'))
    self.assertEquals('http://aa.bb',
                      WprUrlEntry._ExtractUrl('GET http://aa.bb FOO BAR'))

  def testGetResponseHeadersDict(self):
    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('header1', 'value1'),
                                     ('header0', 'value2'),
                                     ('header2', 'value3'),
                                     ('header0', 'value4'),
                                     ('HEadEr3', 'VaLue4')])
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(4, len(headers))
    self.assertEquals('value0,value2,value4', headers['header0'])
    self.assertEquals('value1', headers['header1'])
    self.assertEquals('value3', headers['header2'])
    self.assertEquals('VaLue4', headers['header3'])

  def testSetResponseHeader(self):
    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('header1', 'value1')])
    entry.SetResponseHeader('new_header0', 'new_value0')
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(3, len(headers))
    self.assertEquals('new_value0', headers['new_header0'])
    self.assertEquals('new_header0', entry._wpr_response.original_headers[2][0])

    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('header1', 'value1'),
                                     ('header2', 'value1'),])
    entry.SetResponseHeader('header1', 'new_value1')
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(3, len(headers))
    self.assertEquals('new_value1', headers['header1'])
    self.assertEquals('header1', entry._wpr_response.original_headers[1][0])

    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('hEADEr1', 'value1'),
                                     ('header2', 'value1'),])
    entry.SetResponseHeader('header1', 'new_value1')
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(3, len(headers))
    self.assertEquals('new_value1', headers['header1'])
    self.assertEquals('hEADEr1', entry._wpr_response.original_headers[1][0])

    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('header1', 'value1'),
                                     ('header2', 'value2'),
                                     ('header1', 'value3'),
                                     ('header3', 'value4'),
                                     ('heADer1', 'value5')])
    entry.SetResponseHeader('header1', 'new_value2')
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(4, len(headers))
    self.assertEquals('new_value2', headers['header1'])
    self.assertEquals('header1', entry._wpr_response.original_headers[1][0])
    self.assertEquals('header3', entry._wpr_response.original_headers[3][0])
    self.assertEquals('value4', entry._wpr_response.original_headers[3][1])

    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('heADer1', 'value1'),
                                     ('header2', 'value2'),
                                     ('HEader1', 'value3'),
                                     ('header3', 'value4'),
                                     ('header1', 'value5')])
    entry.SetResponseHeader('header1', 'new_value2')
    headers = entry.GetResponseHeadersDict()
    self.assertEquals(4, len(headers))
    self.assertEquals('new_value2', headers['header1'])
    self.assertEquals('heADer1', entry._wpr_response.original_headers[1][0])
    self.assertEquals('header3', entry._wpr_response.original_headers[3][0])
    self.assertEquals('value4', entry._wpr_response.original_headers[3][1])

  def testDeleteResponseHeader(self):
    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('header1', 'value1'),
                                     ('header0', 'value2'),
                                     ('header2', 'value3')])
    entry.DeleteResponseHeader('header1')
    self.assertNotIn('header1', entry.GetResponseHeadersDict())
    self.assertEquals(2, len(entry.GetResponseHeadersDict()))
    entry.DeleteResponseHeader('header0')
    self.assertNotIn('header0', entry.GetResponseHeadersDict())
    self.assertEquals(1, len(entry.GetResponseHeadersDict()))

    entry = self._CreateWprUrlEntry([('header0', 'value0'),
                                     ('hEAder1', 'value1'),
                                     ('header0', 'value2'),
                                     ('heaDEr2', 'value3')])
    entry.DeleteResponseHeader('header1')
    self.assertNotIn('header1', entry.GetResponseHeadersDict())
    self.assertEquals(2, len(entry.GetResponseHeadersDict()))

  def testRemoveResponseHeaderDirectives(self):
    entry = self._CreateWprUrlEntry([('hEAder0', 'keyWOrd0,KEYword1'),
                                     ('heaDER1', 'value1'),
                                     ('headeR2', 'value3')])
    entry.RemoveResponseHeaderDirectives('header0', {'keyword1', 'keyword0'})
    self.assertNotIn('header0', entry.GetResponseHeadersDict())

    entry = self._CreateWprUrlEntry([('heADEr0', 'keYWOrd0'),
                                     ('hEADERr1', 'value1'),
                                     ('HEAder0', 'keywoRD1,keYwoRd2'),
                                     ('hEADer2', 'value3')])
    entry.RemoveResponseHeaderDirectives('header0', {'keyword1'})
    self.assertEquals(
        'keYWOrd0,keYwoRd2', entry.GetResponseHeadersDict()['header0'])
    self.assertEquals(3, len(entry._wpr_response.original_headers))
    self.assertEquals(
        'keYWOrd0,keYwoRd2', entry._wpr_response.original_headers[0][1])


class WprHostTest(unittest.TestCase):
  def setUp(self):
    OPTIONS.ParseArgs([])
    self._server_address = None
    self._wpr_http_port = None
    self._tmp_directory = tempfile.mkdtemp(prefix='tmp_test_')

  def tearDown(self):
    shutil.rmtree(self._tmp_directory)

  def _TmpPath(self, name):
    return os.path.join(self._tmp_directory, name)

  def _LogPath(self):
    return self._TmpPath('wpr.log')

  def _ArchivePath(self):
    return self._TmpPath('wpr')

  @contextlib.contextmanager
  def RunWebServer(self):
    assert self._server_address is None
    with WebServer.Context(
        source_dir=os.path.join(LOADING_DIR, 'trace_test', 'tests'),
        communication_dir=self._tmp_directory) as server:
      self._server_address = server.Address()
      yield

  @contextlib.contextmanager
  def RunWpr(self, record):
    assert self._server_address is not None
    assert self._wpr_http_port is None
    with _WprHost(self._ArchivePath(), record=record,
                  out_log_path=self._LogPath()) as (http_port, https_port):
      del https_port # unused
      self._wpr_http_port = http_port
      yield http_port

  def DoHttpRequest(self, path, expected_status=200, destination='wpr'):
    assert self._server_address is not None
    if destination == 'wpr':
      assert self._wpr_http_port is not None
      connection = httplib.HTTPConnection('127.0.0.1', self._wpr_http_port)
    elif destination == 'server':
      connection = httplib.HTTPConnection(self._server_address)
    else:
      assert False
    try:
      connection.request(
          "GET", '/' + path, headers={'Host': self._server_address})
      response = connection.getresponse()
    finally:
      connection.close()
    self.assertEquals(expected_status, response.status)

  def _GenRawWprRequest(self, path):
    assert self._wpr_http_port is not None
    url = 'http://127.0.0.1:{}/web-page-replay-{}'.format(
        self._wpr_http_port, path)
    return WprRequest(is_served=True, method='GET', is_wpr_host=True, url=url)

  def GenRawRequest(self, path, is_served):
    assert self._server_address is not None
    return WprRequest(is_served=is_served, method='GET', is_wpr_host=False,
        url='http://{}/{}'.format(self._server_address, path))

  def AssertWprParsedRequests(self, ref_requests):
    all_ref_requests = []
    all_ref_requests.append(self._GenRawWprRequest('generate-200'))
    all_ref_requests.extend(ref_requests)
    all_ref_requests.append(self._GenRawWprRequest('generate-200'))
    all_ref_requests.append(self._GenRawWprRequest('command-exit'))
    requests = ExtractRequestsFromLog(self._LogPath())
    self.assertEquals(all_ref_requests, requests)
    self._wpr_http_port = None

  def testExtractRequestsFromLog(self):
    with self.RunWebServer():
      with self.RunWpr(record=True):
        self.DoHttpRequest('1.html')
        self.DoHttpRequest('2.html')
        ref_requests = [
            self.GenRawRequest('1.html', is_served=True),
            self.GenRawRequest('2.html', is_served=True)]
    self.AssertWprParsedRequests(ref_requests)

    with self.RunWpr(record=False):
      self.DoHttpRequest('2.html')
      self.DoHttpRequest('1.html')
      ref_requests = [
          self.GenRawRequest('2.html', is_served=True),
          self.GenRawRequest('1.html', is_served=True)]
    self.AssertWprParsedRequests(ref_requests)

  def testExtractRequestsFromLogHaveCorrectIsServed(self):
    with self.RunWebServer():
      with self.RunWpr(record=True):
        self.DoHttpRequest('4.html', expected_status=404)
        ref_requests = [self.GenRawRequest('4.html', is_served=True)]
    self.AssertWprParsedRequests(ref_requests)

    with self.RunWpr(record=False):
      self.DoHttpRequest('4.html', expected_status=404)
      self.DoHttpRequest('5.html', expected_status=404)
      ref_requests = [self.GenRawRequest('4.html', is_served=True),
                      self.GenRawRequest('5.html', is_served=False)]
    self.AssertWprParsedRequests(ref_requests)

  def testExtractRequestsFromLogHaveCorrectIsWprHost(self):
    PATH = 'web-page-replay-generate-200'
    with self.RunWebServer():
      self.DoHttpRequest(PATH, expected_status=404, destination='server')
      with self.RunWpr(record=True):
        self.DoHttpRequest(PATH)
      ref_requests = [self.GenRawRequest(PATH, is_served=True)]
    self.AssertWprParsedRequests(ref_requests)


if __name__ == '__main__':
  unittest.main()
