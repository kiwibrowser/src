// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_CONFIG_GPU_MODE_H_
#define GPU_CONFIG_GPU_MODE_H_

namespace gpu {

// What the GPU process is running for.
enum class GpuMode {
  UNKNOWN,
  // The GPU process is running with hardare acceleration.
  HARDWARE_ACCELERATED,
  // The GPU process is running for SwiftShader WebGL.
  SWIFTSHADER,
  // The GPU process is running for the display compositor (OOP-D only).
  DISPLAY_COMPOSITOR,
  // The GPU process is disabled and won't start (not OOP-D only).
  DISABLED,
};

}  // namespace gpu

#endif  // GPU_CONFIG_GPU_MODE_H_
