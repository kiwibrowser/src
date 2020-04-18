# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import json
import os
import shutil
import sys
import tempfile
import unittest

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..', '..'))
sys.path.append(os.path.join(_SRC_DIR, 'tools', 'android', 'loading'))

import options
from trace_test import test_server
from trace_test import webserver_test


OPTIONS = options.OPTIONS


class WebServerTestCase(unittest.TestCase):
  def setUp(self):
    OPTIONS.ParseArgs('', extra=[('--noisy', False)])
    self._temp_dir = tempfile.mkdtemp()
    self._server = webserver_test.WebServer(self._temp_dir, self._temp_dir)

  def tearDown(self):
    self.assertTrue(self._server.Stop())
    shutil.rmtree(self._temp_dir)

  def StartServer(self):
    self._server.Start()

  def WriteFile(self, path, file_content):
    with open(os.path.join(self._temp_dir, path), 'w') as file_output:
      file_output.write(file_content)

  def Request(self, path):
    host, port = self._server.Address().split(':')
    connection = httplib.HTTPConnection(host, int(port))
    connection.request('GET', path)
    response = connection.getresponse()
    connection.close()
    return response

  def testWebserverBasic(self):
    self.WriteFile('test.html',
               '<!DOCTYPE html><html><head><title>Test</title></head>'
               '<body><h1>Test Page</h1></body></html>')
    self.StartServer()

    response = self.Request('test.html')
    self.assertEqual(200, response.status)

    response = self.Request('/test.html')
    self.assertEqual(200, response.status)

    response = self.Request('///test.html')
    self.assertEqual(200, response.status)

  def testWebserver404(self):
    self.StartServer()

    response = self.Request('null')
    self.assertEqual(404, response.status)
    self.assertEqual('text/html', response.getheader('content-type'))

  def testContentType(self):
    self.WriteFile('test.html',
               '<!DOCTYPE html><html><head><title>Test</title></head>'
               '<body><h1>Test Page</h1></body></html>')
    self.WriteFile('blobfile',
               'whatever')
    self.StartServer()

    response = self.Request('test.html')
    self.assertEqual(200, response.status)
    self.assertEqual('text/html', response.getheader('content-type'))

    response = self.Request('blobfile')
    self.assertEqual(500, response.status)

  def testCustomResponseHeader(self):
    self.WriteFile('test.html',
               '<!DOCTYPE html><html><head><title>Test</title></head>'
               '<body><h1>Test Page</h1></body></html>')
    self.WriteFile('test2.html',
               '<!DOCTYPE html><html><head><title>Test 2</title></head>'
               '<body><h1>Test Page 2</h1></body></html>')
    self.WriteFile(test_server.RESPONSE_HEADERS_PATH,
               json.dumps({'test2.html': [['Cache-Control', 'no-store']]}))
    self.StartServer()

    response = self.Request('test.html')
    self.assertEqual(200, response.status)
    self.assertEqual('text/html', response.getheader('content-type'))
    self.assertEqual(None, response.getheader('cache-control'))

    response = self.Request('test2.html')
    self.assertEqual(200, response.status)
    self.assertEqual('text/html', response.getheader('content-type'))
    self.assertEqual('no-store', response.getheader('cache-control'))

    response = self.Request(test_server.RESPONSE_HEADERS_PATH)
    self.assertEqual(200, response.status)
    self.assertEqual('application/json', response.getheader('content-type'))
    self.assertEqual(None, response.getheader('cache-control'))


if __name__ == '__main__':
  unittest.main()
