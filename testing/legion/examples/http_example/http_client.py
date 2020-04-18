# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import httplib
import sys


def GetArgs():
  """Returns the specified command line args."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--server', required=True)
  parser.add_argument('--port', required=True, type=int)
  return parser.parse_args()


def main():
  """Get the webpage and assert the text == 'SUCCESS!'."""
  args = GetArgs()
  conn = httplib.HTTPConnection(args.server, args.port)
  conn.request('GET', '/')
  response = conn.getresponse().read()
  assert response == 'SUCCESS!', '%s != SUCCESS!' % response


if __name__ == '__main__':
  sys.exit(main())
