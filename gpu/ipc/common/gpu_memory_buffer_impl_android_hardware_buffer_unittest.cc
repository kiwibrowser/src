// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_memory_buffer_impl_android_hardware_buffer.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_test_template.h"

namespace gpu {
namespace {

INSTANTIATE_TYPED_TEST_CASE_P(GpuMemoryBufferImplAndroidHardwareBuffer,
                              GpuMemoryBufferImplTest,
                              GpuMemoryBufferImplAndroidHardwareBuffer);

INSTANTIATE_TYPED_TEST_CASE_P(GpuMemoryBufferImplAndroidHardwareBuffer,
                              GpuMemoryBufferImplCreateTest,
                              GpuMemoryBufferImplAndroidHardwareBuffer);

}  // namespace
}  // namespace gpu
