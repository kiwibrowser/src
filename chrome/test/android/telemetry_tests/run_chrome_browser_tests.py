#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

CHROMIUM_SRC_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
PERF_DIR = os.path.join(CHROMIUM_SRC_DIR, 'tools', 'perf')
sys.path.append(PERF_DIR)

from chrome_telemetry_build import chromium_config
sys.path.append(chromium_config.GetTelemetryDir())

from telemetry import project_config
from telemetry.testing import browser_test_runner


def main():
  options = browser_test_runner.TestRunOptions()
  config = project_config.ProjectConfig(
      top_level_dir=os.path.dirname(__file__),
      benchmark_dirs=[os.path.join(os.path.dirname(__file__), 'browser_tests')],
      default_chrome_root=CHROMIUM_SRC_DIR)
  return browser_test_runner.Run(config, options, sys.argv[1:])


if __name__ == '__main__':
  sys.exit(main())
