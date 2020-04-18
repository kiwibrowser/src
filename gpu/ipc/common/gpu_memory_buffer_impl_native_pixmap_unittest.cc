// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_memory_buffer_impl_native_pixmap.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl_test_template.h"

namespace gpu {
namespace {

INSTANTIATE_TYPED_TEST_CASE_P(GpuMemoryBufferImplNativePixmap,
                              GpuMemoryBufferImplTest,
                              GpuMemoryBufferImplNativePixmap);

}  // namespace
}  // namespace gpu
