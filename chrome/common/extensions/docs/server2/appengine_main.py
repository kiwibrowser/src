#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

# Add the original server location to sys.path so we are able to import
# modules from there.
SERVER_PATH = 'chrome/common/extensions/docs/server2'
if os.path.abspath(SERVER_PATH) not in sys.path:
  sys.path.append(os.path.abspath(SERVER_PATH))

import webapp2

from app_engine_handler import AppEngineHandler

def main():
  webapp2.WSGIApplication([('/.*', AppEngineHandler)], debug=False).run()

if __name__ == '__main__':
  main()
