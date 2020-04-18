#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Helper for generating compile DBs for clang tooling. On non-Windows platforms,
this is pretty straightforward. On Windows, the tool does a bit of extra work to
integrate the content of response files, force clang tooling to run in clang-cl
mode, etc.
"""

import argparse
import json
import os
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
tool_dir = os.path.abspath(os.path.join(script_dir, '../pylib'))
sys.path.insert(0, tool_dir)

from clang import compile_db


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-p',
      required=True,
      help='Path to build directory')
  args = parser.parse_args()

  print json.dumps(compile_db.GenerateWithNinja(args.p))


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
