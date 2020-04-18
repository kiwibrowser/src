# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from gpu_tests.gpu_test_expectations import GpuTestExpectations

# See the GpuTestExpectations class for documentation.

class TraceTestExpectations(GpuTestExpectations):
  def SetExpectations(self):
    # Sample Usage:
    # self.Fail('trace_test.Canvas2DRedBox',
    #     ['mac', 'amd', ('nvidia', 0x1234)], bug=123)
    # TODO(kbr): flakily timing out on this configuration.
    self.Flaky('TraceTest_*', ['linux', 'intel', 'debug'], bug=648369)

    # Device traces are not supported on all machines.
    self.Skip('DeviceTraceTest_*')
