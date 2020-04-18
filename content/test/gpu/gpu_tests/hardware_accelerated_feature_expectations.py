# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from gpu_tests.gpu_test_expectations import GpuTestExpectations

# See the GpuTestExpectations class for documentation.

class HardwareAcceleratedFeatureExpectations(GpuTestExpectations):
  def SetExpectations(self):
    # Accelerated 2D canvas is not available on Linux due to driver instability
    self.Fail('HardwareAcceleratedFeature_canvas_accelerated',
        ['linux'], bug=254724)
    self.Fail('HardwareAcceleratedFeature_canvas_accelerated',
        ['highsierra', 'nvidia'], bug=777590)
