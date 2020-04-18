#!/usr/bin/python
# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""A tiny web server.

This is intended to be used for testing, and
only run from within the
googleclient/native_client
"""


import BaseHTTPServer
import logging
import os
import SimpleHTTPServer
import SocketServer
import sys

logging.getLogger().setLevel(logging.INFO)

# Using 'localhost' means that we only accept connections
# via the loop back interface.
SERVER_PORT = 5103
SERVER_HOST = ''

# We only run from the native_client directory, so that not too much
# is exposed via this HTTP server.  Everything in the directory is
# served, so there should never be anything potentially sensitive in
# the serving directory, especially if the machine might be a
# multi-user machine and not all users are trusted.  We only serve via
# the loopback interface.

SAFE_DIR_COMPONENTS = ['native_client']
SAFE_DIR_SUFFIX = apply(os.path.join, SAFE_DIR_COMPONENTS)


def SanityCheckDirectory():
  if os.getcwd().endswith(SAFE_DIR_SUFFIX):
    return
  logging.error('httpd.py should only be run from the %s', SAFE_DIR_SUFFIX)
  logging.error('directory for testing purposes.')
  logging.error('We are currently in %s', os.getcwd())
  sys.exit(1)


# the sole purpose of this class is to make the BaseHTTPServer threaded
class ThreadedServer(SocketServer.ThreadingMixIn,
                     BaseHTTPServer.HTTPServer):
  pass


def Run(server_address,
        server_class=ThreadedServer,
        handler_class=SimpleHTTPServer.SimpleHTTPRequestHandler):
  httpd = server_class(server_address, handler_class)
  logging.info('started server on port %d', httpd.server_address[1])
  httpd.serve_forever()


if __name__ == '__main__':
  SanityCheckDirectory()
  if len(sys.argv) > 1:
    Run((SERVER_HOST, int(sys.argv[1])))
  else:
    Run((SERVER_HOST, SERVER_PORT))
