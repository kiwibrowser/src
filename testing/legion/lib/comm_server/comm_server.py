# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Communications server.

This is the HTTP server class. The server is run in a separate thread allowing
the calling code to proceed normally after calling start(). shutdown() must
be called to tear the server down. Failure to do this will most likely end up
hanging the program.
"""

import BaseHTTPServer
import SocketServer
import threading

from legion.lib import common_lib
from legion.lib.comm_server import server_handler


class CommServer(SocketServer.ThreadingMixIn,
                     BaseHTTPServer.HTTPServer):
  """An extension of the HTTPServer class which handles requests in threads."""

  def __init__(self, address='', port=None):
    self._port = port or common_lib.GetUnusedPort()
    self._address = address
    BaseHTTPServer.HTTPServer.__init__(self,
                                       (self._address, self._port),
                                       server_handler.ServerHandler)

  @property
  def port(self):
    return self._port

  @property
  def address(self):
    return self._address

  def start(self):
    """Starts the server in another thread.

    The thread will stay active until shutdown() is called. There is no reason
    to hold a reference to the thread object.

    The naming convention used here (lowercase) is used to match the base
    server's naming convention.
    """
    threading.Thread(target=self.serve_forever).start()
