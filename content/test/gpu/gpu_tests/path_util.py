# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys


def GetChromiumSrcDir():
  return os.path.abspath(os.path.join(
      os.path.dirname(__file__), os.pardir, os.pardir, os.pardir, os.pardir))


def GetGpuTestDir():
  return os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))


def AddDirToPathIfNeeded(*path_parts):
  path = os.path.abspath(os.path.join(*path_parts))
  if os.path.isdir(path) and path not in sys.path:
    sys.path.append(path)


def SetupTelemetryPaths():
  chromium_src_dir = GetChromiumSrcDir()

  perf_path = os.path.join(chromium_src_dir, 'tools', 'perf')
  absolute_perf_path = os.path.abspath(perf_path)

  sys.path.append(absolute_perf_path)
  from core import path_util

  telemetry_path = path_util.GetTelemetryDir()
  if telemetry_path not in sys.path:
    sys.path.append(telemetry_path)

  py_utils_path = os.path.join(
      chromium_src_dir, 'third_party', 'catapult', 'common', 'py_utils')
  if py_utils_path not in sys.path:
    sys.path.append(py_utils_path)

  pylint_path = os.path.join(
      chromium_src_dir, 'third_party', 'pylint')
  if pylint_path not in sys.path:
    sys.path.append(pylint_path)
