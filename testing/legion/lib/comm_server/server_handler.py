# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handler used directly by the server.

This handler routes the request to the correct subhandler based on the first
value in the URL path. For example, the MessageHandler has been added to the
class's _HANDLERS object and handles all requests destined for URL/messages/...

To extend this functionality implement a handler and add it to the _HANDLERS
object with the correct category. The category is defined as the first part of
the URL path (i.e. URL/<CATEGORY>). The handler will then be called any time a
request comes in with that category.
"""

import re
import SimpleHTTPServer

# Import all communications handlers
from legion.lib.comm_server import message_handler


class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  """Server handler class."""

  _HANDLERS = {
      'messages': message_handler.MessageHandler,
      }
  _REGEX = '/(?P<category>[a-zA-Z0-9_.-~]+)/'

  def log_message(self, *args, **kwargs):
    """Silence those pesky server-side print statements."""
    pass

  def _GetCategoryName(self):
    """Extracts and returns the category name."""
    match = re.match(self._REGEX, self.path)
    if not match:
      return
    return match.group('category')

  def _GetHandler(self):
    """Returns the category handler object if it exists."""
    category = self._GetCategoryName()
    if not category:
      return self.send_error(403, 'Category must be supplied in the form of '
                             '/category_name/...')
    handler = self._HANDLERS.get(category)
    if not handler:
      return self.send_error(405, 'No handler found for /%s/' % category)
    return handler()

  def do_GET(self):
    """Dispatches GET requests."""
    handler = self._GetHandler()
    if handler:
      handler.do_GET(self)

  def do_POST(self):
    """Dispatches POST requests."""
    handler = self._GetHandler()
    if handler:
      handler.do_POST(self)

  def do_PUT(self):
    """Dispatches PUT requests."""
    handler = self._GetHandler()
    if handler:
      handler.do_PUT(self)

  def do_DELETE(self):
    """Dispatches DELETE requests."""
    handler = self._GetHandler()
    if handler:
      handler.do_DELETE(self)
