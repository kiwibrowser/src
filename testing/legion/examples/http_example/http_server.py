# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import SimpleHTTPServer
import SocketServer
import sys
import threading
import time


class Handler(SimpleHTTPServer.SimpleHTTPRequestHandler):

  def do_GET(self):
    self.wfile.write('SUCCESS!')


def GetArgs():
  """Returns the specified command line args."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--port', required=True, type=int)
  parser.add_argument('--timeout', type=int, default=60)
  return parser.parse_args()


def main():
  """Run a webserver until the process is killed."""
  server = None
  args = GetArgs()
  try:
    server = SocketServer.TCPServer(('', args.port), Handler)
    thread = threading.Thread(target=server.serve_forever)
    thread.start()
    start = time.time()
    while time.time() < start + args.timeout:
      time.sleep(1)
  finally:
    if server:
      server.shutdown()


if __name__ == '__main__':
  sys.exit(main())
