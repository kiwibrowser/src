# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import android_vr_perf_test
import webvr_latency_test

class AndroidWebVrLatencyTest(webvr_latency_test.WebVrLatencyTest,
                              android_vr_perf_test.AndroidVrPerfTest):
  """Android implementation of the WebVR latency test."""
  def __init__(self, args):
    super(AndroidWebVrLatencyTest, self).__init__(args)
