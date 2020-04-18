# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script for automatically measuring FPS for VR via VrCore perf logging.

Android only.
VrCore has a developer option to log performance data to logcat. This test
visits various WebVR URLs and collects the FPS data reported by VrCore.
"""

# Needs to be imported first in order to add the parent directory to path.
import vrcore_fps_test_config
import vrcore_fps_test
import vr_test_arg_parser


def main():
  parser = vr_test_arg_parser.VrTestArgParser()
  parser.AddFpsOptions()
  args = parser.ParseArgumentsAndSetLogLevel()
  test = vrcore_fps_test.VrCoreFpsTest(args)
  test.RunTests()


if __name__ == '__main__':
  main()
