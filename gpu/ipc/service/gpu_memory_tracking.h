// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_MEMORY_TRACKING_H_
#define GPU_IPC_SERVICE_GPU_MEMORY_TRACKING_H_

#include <stdint.h>

#include "base/process/process.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/ipc/service/gpu_ipc_service_export.h"

namespace gpu {

class GpuMemoryManager;

// All decoders in a context group point to a single GpuMemoryTrackingGroup,
// which tracks GPU resource consumption for the entire context group.
class GPU_IPC_SERVICE_EXPORT GpuMemoryTrackingGroup {
 public:
  ~GpuMemoryTrackingGroup();
  void TrackMemoryAllocatedChange(uint64_t old_size, uint64_t new_size);
  bool EnsureGPUMemoryAvailable(uint64_t size_needed);
  base::ProcessId GetPid() const {
    return pid_;
  }
  uint64_t GetSize() const { return size_; }
  gles2::MemoryTracker* GetMemoryTracker() const {
    return memory_tracker_;
  }

 private:
  friend class GpuMemoryManager;

  GpuMemoryTrackingGroup(base::ProcessId pid,
                         gles2::MemoryTracker* memory_tracker,
                         GpuMemoryManager* memory_manager);

  base::ProcessId pid_;
  uint64_t size_;

  // Set and used only during the Manage function, to determine which
  // non-surface clients should be hibernated.
  bool hibernated_;

  gles2::MemoryTracker* memory_tracker_;
  GpuMemoryManager* memory_manager_;
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_MEMORY_TRACKING_H_
