// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_tracking.h"

#include "gpu/ipc/service/gpu_memory_manager.h"

namespace gpu {

GpuMemoryTrackingGroup::GpuMemoryTrackingGroup(
    base::ProcessId pid,
    gles2::MemoryTracker* memory_tracker,
    GpuMemoryManager* memory_manager)
    : pid_(pid),
      size_(0),
      hibernated_(false),
      memory_tracker_(memory_tracker),
      memory_manager_(memory_manager) {
}

GpuMemoryTrackingGroup::~GpuMemoryTrackingGroup() {
  memory_manager_->OnDestroyTrackingGroup(this);
}

void GpuMemoryTrackingGroup::TrackMemoryAllocatedChange(uint64_t old_size,
                                                        uint64_t new_size) {
  memory_manager_->TrackMemoryAllocatedChange(
      this, old_size, new_size);
}

bool GpuMemoryTrackingGroup::EnsureGPUMemoryAvailable(uint64_t size_needed) {
  return memory_manager_->EnsureGPUMemoryAvailable(size_needed);
}


}  // namespace gpu
