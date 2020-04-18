// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_crash_keys.h"

namespace gpu {
namespace crash_keys {

#if !defined(OS_ANDROID)
crash_reporter::CrashKeyString<16> gpu_vendor_id("gpu-venid");
crash_reporter::CrashKeyString<16> gpu_device_id("gpu-devid");
#endif
crash_reporter::CrashKeyString<64> gpu_driver_version("gpu-driver");
crash_reporter::CrashKeyString<16> gpu_pixel_shader_version("gpu-psver");
crash_reporter::CrashKeyString<16> gpu_vertex_shader_version("gpu-vsver");
#if defined(OS_MACOSX)
crash_reporter::CrashKeyString<64> gpu_gl_version("gpu-glver");
#elif defined(OS_POSIX)
crash_reporter::CrashKeyString<256> gpu_vendor("gpu-gl-vendor");
crash_reporter::CrashKeyString<128> gpu_renderer("gpu-gl-renderer");
#endif
crash_reporter::CrashKeyString<4> gpu_gl_context_is_virtual(
    "gpu-gl-context-is-virtual");

}  // namespace crash_keys
}  // namespace gpu
