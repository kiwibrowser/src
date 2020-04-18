# Copyright 2014 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import BaseHTTPServer
import httplib
import json
import logging
import threading


_STOP_EVENT = '/mockserver/__stop__'


class MockHandler(BaseHTTPServer.BaseHTTPRequestHandler):

  ### Public methods

  def send_json(self, data):
    """Sends a JSON response."""
    self.send_response(200)
    self.send_header('Content-type', 'application/json')
    self.end_headers()
    json.dump(data, self.wfile)

  def send_octet_stream(self, data):
    """Sends a binary response."""
    self.send_response(200)
    self.send_header('Content-type', 'application/octet-stream')
    self.end_headers()
    self.wfile.write(data)

  def read_body(self):
    """Reads the request body."""
    return self.rfile.read(int(self.headers['Content-Length']))

  def drop_body(self):
    """Reads the request body."""
    size = int(self.headers['Content-Length'])
    while size:
      chunk = min(4096, size)
      self.rfile.read(chunk)
      size -= chunk

  ### Overrides from BaseHTTPRequestHandler

  def do_OPTIONS(self):
    if self.path == _STOP_EVENT:
      self.server.parent._stopped = True
    self.send_octet_stream('')

  def log_message(self, fmt, *args):
    logging.info(
        '%s - - [%s] %s', self.address_string(), self.log_date_time_string(),
        fmt % args)


class MockServer(object):
  _HANDLER_CLS = None

  def __init__(self):
    assert issubclass(self._HANDLER_CLS, MockHandler), self._HANDLER_CLS
    self._closed = False
    self._stopped = False
    self._server = BaseHTTPServer.HTTPServer(
        ('127.0.0.1', 0), self._HANDLER_CLS)
    self._server.parent = self
    self._server.url = self.url = 'http://127.0.0.1:%d' % (
        self._server.server_port)
    self._thread = threading.Thread(target=self._run, name='httpd')
    self._thread.daemon = True
    self._thread.start()
    logging.info('%s', self.url)

  def close(self):
    assert not self._closed
    self._closed = True
    self._send_event(_STOP_EVENT)
    self._thread.join()

  def _run(self):
    while not self._stopped:
      self._server.handle_request()
    self._server.server_close()

  def _send_event(self, path):
    conn = httplib.HTTPConnection(
        '127.0.0.1:%d' % self._server.server_port, timeout=60)
    try:
      conn.request('OPTIONS', path)
      conn.getresponse()
    finally:
      conn.close()
