#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is a wrapper script used during migration of performance
tests to use the new recipe.  See crbug.com/757933

Non-telemetry tests now will all run with this script.  The flag
--migrated-test will indicate if this test is using the new recipe or not.
By default this script runs the legacy testing/scripts/run_gtest_perf_test.py.

"""

import argparse
import os
import subprocess
import sys

SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
  __file__))))

GTEST = os.path.join(SRC_DIR, 'testing', 'scripts', 'run_gtest_perf_test.py')
PERF = os.path.join(SRC_DIR, 'testing', 'scripts', 'run_performance_tests.py')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--migrated-test', type=bool, default=False)

  args, rest_args = parser.parse_known_args()

  if args.migrated_test:
    return subprocess.call([sys.executable, PERF] + rest_args)
  else:
    return subprocess.call([sys.executable, GTEST] + rest_args)


if __name__ == '__main__':
  sys.exit(main())
