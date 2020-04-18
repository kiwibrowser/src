# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script for automatically measuring motion-to-photon latency for VR.

Doing so requires two specialized pieces of hardware. The first is a Motopho,
which when used with a VR flicker app, finds the delay between movement and
the test device's screen updating in response to the movement. The second is
a set of servos, which physically moves the test device and Motopho during the
latency test.
"""
# Must be first import in order to add parent directory to system path.
import latency_test_config
import android_webvr_latency_test
import vr_test_arg_parser

import sys


def main():
  parser = vr_test_arg_parser.VrTestArgParser()
  parser.AddLatencyOptions()
  args = parser.ParseArgumentsAndSetLogLevel()
  latency_test = None
  if args.platform == 'android':
    latency_test = android_webvr_latency_test.AndroidWebVrLatencyTest(args)
  elif args.platform == 'win':
    raise NotImplementedError('WebVR not currently supported on Windows')
  else:
    raise RuntimeError('Given platform %s not recognized' % args.platform)
  latency_test.RunTests()


if __name__ == '__main__':
  sys.exit(main())
