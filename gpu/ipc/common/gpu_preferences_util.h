// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_PREFERENCES_UTIL_H_
#define GPU_IPC_COMMON_GPU_PREFERENCES_UTIL_H_

#include <string>

#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/gpu_util_export.h"

namespace gpu {

// Encode struct into a string so it can be passed as a commandline switch.
GPU_UTIL_EXPORT std::string GpuPreferencesToSwitchValue(
    const GpuPreferences& prefs);

// Decode the encoded string back to GpuPrefences struct.
// If return false, |output_prefs| won't be touched.
GPU_UTIL_EXPORT bool SwitchValueToGpuPreferences(const std::string& data,
                                                 GpuPreferences* output_prefs);

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_PREFERENCES_UTIL_H_
