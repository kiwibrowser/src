#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper for runtest.py, makes it possible to control src-side
which file gets used and test the changes on trybots before landing."""


import argparse
import copy
import os
import subprocess
import sys


SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('args', nargs='*', help='Arguments to pass to runtest.py')
  args = parser.parse_args(argv)

  env = copy.copy(os.environ)
  # Reset PYTHONPATH to make sure we're not accidentally using
  # the buildbot-provided value and build-side modules. That would make
  # changes inside this directory not take effect on buildbot.
  pythonpath = []
  pythonpath.append(os.path.join(
      SRC_DIR, 'infra', 'scripts', 'legacy', 'scripts'))
  pythonpath.append(os.path.join(
      SRC_DIR, 'infra', 'scripts', 'legacy', 'site_config'))
  pythonpath.append(os.path.join(
      SRC_DIR, 'third_party'))
  env['PYTHONPATH'] = os.pathsep.join(pythonpath)

  return subprocess.call([
      sys.executable,
      os.path.join(SRC_DIR, 'infra', 'scripts', 'legacy',
                   'scripts', 'slave', 'runtest.py')
  ] + args.args, env=env)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
