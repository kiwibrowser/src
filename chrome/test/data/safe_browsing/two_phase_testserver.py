#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Testserver for the two phase upload protocol."""

import base64
import BaseHTTPServer
import hashlib
import os
import sys
import urlparse

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

sys.path.append(os.path.join(BASE_DIR, '..', '..', '..', '..', 'net',
                             'tools', 'testserver'))
import testserver_base


class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
  def ReadRequestBody(self):
    """This function reads the body of the current HTTP request, handling
    both plain and chunked transfer encoded requests."""

    if self.headers.getheader('transfer-encoding') == 'chunked':
      return ''

    length = int(self.headers.getheader('content-length'))
    return self.rfile.read(length)

  def do_GET(self):
    print 'GET', self.path
    self.send_error(400, 'GET not supported')

  def do_POST(self):
    request_body = self.ReadRequestBody()
    print 'POST', repr(self.path), repr(request_body)

    kStartHeader = 'x-goog-resumable'
    if kStartHeader not in self.headers:
      self.send_error(400, 'Missing header: ' + kStartHeader)
      return
    if self.headers.get(kStartHeader) != 'start':
      self.send_error(400, 'Invalid %s header value: %s' % (
          kStartHeader, self.headers.get(kStartHeader)))
      return

    metadata_hash = hashlib.sha1(request_body).hexdigest()
    _, _, url_path, _, query, _ = urlparse.urlparse(self.path)
    query_args = urlparse.parse_qs(query)

    if query_args.get('p1close'):
      self.close_connection = 1
      return

    put_url = 'http://%s:%d/put?%s,%s,%s' % (self.server.server_address[0],
                                          self.server.server_port,
                                          url_path,
                                          metadata_hash,
                                          base64.urlsafe_b64encode(query))
    self.send_response(int(query_args.get('p1code', [201])[0]))
    self.send_header('Location', put_url)
    self.end_headers()

  def do_PUT(self):
    _, _, url_path, _, query, _ = urlparse.urlparse(self.path)
    if url_path != '/put':
      self.send_error(400, 'invalid path on 2nd phase: ' + url_path)
      return

    initial_path, metadata_hash, config_query_b64 = query.split(',', 2)
    config_query = urlparse.parse_qs(base64.urlsafe_b64decode(config_query_b64))

    request_body = self.ReadRequestBody()
    print 'PUT', repr(self.path), len(request_body), 'bytes'

    if config_query.get('p2close'):
      self.close_connection = 1
      return

    self.send_response(int(config_query.get('p2code', [200])[0]))
    self.end_headers()
    self.wfile.write('%s\n%s\n%s\n' % (
        initial_path,
        metadata_hash,
        hashlib.sha1(request_body).hexdigest()))


class ServerRunner(testserver_base.TestServerRunner):
  """TestServerRunner for safebrowsing_test_server.py."""

  def create_server(self, server_data):
    server = BaseHTTPServer.HTTPServer((self.options.host, self.options.port),
                                       RequestHandler)
    print 'server started on port %d...' % server.server_port
    server_data['port'] = server.server_port

    return server


if __name__ == '__main__':
  sys.exit(ServerRunner().main())
