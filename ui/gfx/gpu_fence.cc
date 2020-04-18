// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence.h"

#include "base/logging.h"

#if defined(OS_LINUX) || defined(OS_ANDROID)
#include <sync/sync.h>
#endif

namespace gfx {

GpuFence::GpuFence(const GpuFenceHandle& handle) : type_(handle.type) {
  switch (type_) {
    case GpuFenceHandleType::kEmpty:
      break;
    case GpuFenceHandleType::kAndroidNativeFenceSync:
#if defined(OS_POSIX)
      owned_fd_.reset(handle.native_fd.fd);
#else
      NOTREACHED();
#endif
      break;
  }
}

GpuFence::~GpuFence() = default;

GpuFenceHandle GpuFence::GetGpuFenceHandle() const {
  gfx::GpuFenceHandle handle;
  switch (type_) {
    case GpuFenceHandleType::kEmpty:
      break;
    case GpuFenceHandleType::kAndroidNativeFenceSync:
#if defined(OS_POSIX)
      handle.type = gfx::GpuFenceHandleType::kAndroidNativeFenceSync;
      handle.native_fd = base::FileDescriptor(owned_fd_.get(),
                                              /*auto_close=*/false);
#else
      NOTREACHED();
#endif
      break;
  }
  return handle;
}

ClientGpuFence GpuFence::AsClientGpuFence() {
  return reinterpret_cast<ClientGpuFence>(this);
}

// static
GpuFence* GpuFence::FromClientGpuFence(ClientGpuFence gpu_fence) {
  return reinterpret_cast<GpuFence*>(gpu_fence);
}

void GpuFence::Wait() {
  switch (type_) {
    case GpuFenceHandleType::kEmpty:
      break;
    case GpuFenceHandleType::kAndroidNativeFenceSync:
#if defined(OS_LINUX) || defined(OS_ANDROID)
      static const int kInfiniteSyncWaitTimeout = -1;
      DCHECK_GE(owned_fd_.get(), 0);
      if (sync_wait(owned_fd_.get(), kInfiniteSyncWaitTimeout) < 0) {
        LOG(FATAL) << "Failed while waiting for gpu fence fd";
      }
#else
      NOTREACHED();
#endif
      break;
  }
}

}  // namespace gfx
