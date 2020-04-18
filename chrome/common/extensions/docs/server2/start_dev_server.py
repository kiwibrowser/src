#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import signal
import shutil
import subprocess
import sys

import build_server

SERVER_PATH = sys.path[0]
SRC_PATH = os.path.join(SERVER_PATH, os.pardir, os.pardir, os.pardir, os.pardir,
    os.pardir)
FILENAMES = ['app.yaml', 'appengine_main.py']

def CleanUp(signal, frame):
  for filename in FILENAMES:
    os.remove(os.path.join(SRC_PATH, filename))

if len(sys.argv) < 2:
  print 'usage: start_dev_server.py <location of dev_appserver.py> [options]'
  exit(0)

signal.signal(signal.SIGINT, CleanUp)

build_server.main()
for filename in FILENAMES:
  shutil.copy(os.path.join(SERVER_PATH, filename),
      os.path.join(SRC_PATH, filename))
args = [sys.executable] + sys.argv[1:] + [SRC_PATH]
subprocess.call(args)
