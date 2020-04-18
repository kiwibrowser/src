# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Defines the handler for /messages/<NAME> paths.

The name of the message is expected to follow the /messages/ portion of the
path. The body of the request will contain the message, both when uploading
the message as well as retrieving it. The message will remain on the server
until the calling code does an explicit DELETE call to remove it.

The message is optional. This allows the caller to use this as a simple signal
server. The return code can always be used to tell if a message with that name
exists on the server (200 exists, 404 doesn't exist).

When uploading a body ensure the content-length header is passed correctly.
If the content-length isn't passed no data is read from the body. If its set
too low only part of the message will be read. If its set too high the server
will block waiting for more data to be uploaded.
"""

import re
import threading

from legion.lib.comm_server import base_handler


class MessageHandler(base_handler.BaseHandler):
  """Handles /messages/<NAME> requests."""

  _REGEX = '/messages/(?P<name>[a-zA-Z0-9_.-~]+)'
  _messages = {}
  _message_lock = threading.Lock()

  def _GetName(self, request):
    """Gets the message name from the URL."""
    match = re.match(self._REGEX, request.path)
    if not match:
      return None
    return match.group('name')

  def do_PUT(self, request):
    """Handles PUT requests."""
    name = self._GetName(request)
    if not name:
      return request.send_error(405, 'Key name required')
    with self._message_lock:
      self._messages[name] = request.rfile.read(
          int(request.headers.getheader('content-length', 0)))
      return request.send_response(200)

  def do_GET(self, request):
    """Handles GET requests."""
    name = self._GetName(request)
    if not name:
      return request.send_error(405, 'Key name required')
    elif name not in self._messages:
      return request.send_error(404, 'Key not found')
    with self._message_lock:
      request.send_response(200)
      request.send_header('Content-type', 'text/plain')
      request.send_header('Content-Length', str(len(self._messages[name])))
      request.end_headers()
      request.wfile.write(self._messages[name])

  def do_DELETE(self, request):
    """Handles DELETE requests."""
    name = self._GetName(request)
    if not name:
      return request.send_error(405, 'Key name required')
    with self._message_lock:
      if name in self._messages:
        del self._messages[name]
        return request.send_response(200)
      else:
        return request.send_error(404, 'Key not found')
