// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_driver_bug_workarounds.h"

#include <algorithm>

#include "base/logging.h"

namespace {
// Construct GpuDriverBugWorkarounds from a set of enabled workaround IDs.
void IntSetToWorkarounds(const std::vector<int32_t>& enabled_workarounds,
                         gpu::GpuDriverBugWorkarounds* workarounds) {
  DCHECK(workarounds);
  for (auto ID : enabled_workarounds) {
    switch (ID) {
#define GPU_OP(type, name)    \
  case gpu::type:             \
    workarounds->name = true; \
    break;
      GPU_DRIVER_BUG_WORKAROUNDS(GPU_OP)
#undef GPU_OP
      default:
        NOTIMPLEMENTED();
    }
  }
  if (workarounds->max_texture_size_limit_4096)
    workarounds->max_texture_size = 4096;

  if (workarounds->max_fragment_uniform_vectors_32)
    workarounds->max_fragment_uniform_vectors = 32;
  if (workarounds->max_varying_vectors_16)
    workarounds->max_varying_vectors = 16;
  if (workarounds->max_vertex_uniform_vectors_256)
    workarounds->max_vertex_uniform_vectors = 256;

  if (workarounds->max_copy_texture_chromium_size_1048576)
    workarounds->max_copy_texture_chromium_size = 1048576;
  if (workarounds->max_copy_texture_chromium_size_262144)
    workarounds->max_copy_texture_chromium_size = 262144;
}

GLint LowerMax(GLint max0, GLint max1) {
  if (max0 > 0 && max1 > 0)
    return std::min(max0, max1);
  if (max0 > 0)
    return max0;
  return max1;
}

}  // anonymous namespace

namespace gpu {

GpuDriverBugWorkarounds::GpuDriverBugWorkarounds() = default;

GpuDriverBugWorkarounds::GpuDriverBugWorkarounds(
    const std::vector<int>& enabled_driver_bug_workarounds) {
  IntSetToWorkarounds(enabled_driver_bug_workarounds, this);
}

GpuDriverBugWorkarounds::GpuDriverBugWorkarounds(
    const GpuDriverBugWorkarounds& other) = default;

GpuDriverBugWorkarounds::~GpuDriverBugWorkarounds() = default;

std::vector<int32_t> GpuDriverBugWorkarounds::ToIntSet() const {
  std::vector<int32_t> result;
#define GPU_OP(type, name) \
  if (name)                \
    result.push_back(type);
  GPU_DRIVER_BUG_WORKAROUNDS(GPU_OP)
#undef GPU_OP
  return result;
}

void GpuDriverBugWorkarounds::Append(const GpuDriverBugWorkarounds& extra) {
#define GPU_OP(type, name) name |= extra.name;
  GPU_DRIVER_BUG_WORKAROUNDS(GPU_OP)
#undef GPU_OP

  max_texture_size = LowerMax(max_texture_size, extra.max_texture_size);
  max_fragment_uniform_vectors = LowerMax(max_fragment_uniform_vectors,
                                          extra.max_fragment_uniform_vectors);
  max_varying_vectors =
      LowerMax(max_varying_vectors, extra.max_varying_vectors);
  max_vertex_uniform_vectors =
      LowerMax(max_vertex_uniform_vectors, extra.max_vertex_uniform_vectors);
  max_copy_texture_chromium_size = LowerMax(
      max_copy_texture_chromium_size, extra.max_copy_texture_chromium_size);
}

}  // namespace gpu
