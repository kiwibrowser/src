// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_CONFIG_GPU_CRASH_KEYS_H_
#define GPU_CONFIG_GPU_CRASH_KEYS_H_

#include "build/build_config.h"
#include "components/crash/core/common/crash_key.h"
#include "gpu/gpu_export.h"

namespace gpu {
namespace crash_keys {

// Keys that can be used for crash reporting.
#if !defined(OS_ANDROID)
extern GPU_EXPORT crash_reporter::CrashKeyString<16> gpu_vendor_id;
extern GPU_EXPORT crash_reporter::CrashKeyString<16> gpu_device_id;
#endif
extern GPU_EXPORT crash_reporter::CrashKeyString<64> gpu_driver_version;
extern GPU_EXPORT crash_reporter::CrashKeyString<16> gpu_pixel_shader_version;
extern GPU_EXPORT crash_reporter::CrashKeyString<16> gpu_vertex_shader_version;
#if defined(OS_MACOSX)
extern GPU_EXPORT crash_reporter::CrashKeyString<64> gpu_gl_version;
#elif defined(OS_POSIX)
extern GPU_EXPORT crash_reporter::CrashKeyString<256> gpu_vendor;
extern GPU_EXPORT crash_reporter::CrashKeyString<128> gpu_renderer;
#endif
extern GPU_EXPORT crash_reporter::CrashKeyString<4> gpu_gl_context_is_virtual;

}  // namespace crash_keys
}  // namespace gpu

#endif  // GPU_CONFIG_GPU_CRASH_KEYS_H_
