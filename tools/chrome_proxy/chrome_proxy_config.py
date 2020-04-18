# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir, 'perf'))

from chrome_telemetry_build import chromium_config

TELEMETRY_DIR = chromium_config.GetTelemetryDir()

_top_level_dir = os.path.dirname(os.path.realpath(__file__))

def Config(benchmark_subdirs):
  return chromium_config.ChromiumConfig(
      top_level_dir=_top_level_dir,
      benchmark_dirs=[os.path.join(_top_level_dir, subdir)
                      for subdir in benchmark_subdirs])
