// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/video_accelerator/protected_buffer_manager_proxy.h"

#include "components/arc/video_accelerator/arc_video_accelerator_util.h"
#include "components/arc/video_accelerator/protected_buffer_manager.h"
#include "mojo/public/cpp/system/platform_handle.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "

namespace arc {

GpuArcProtectedBufferManagerProxy::GpuArcProtectedBufferManagerProxy(
    scoped_refptr<arc::ProtectedBufferManager> protected_buffer_manager)
    : protected_buffer_manager_(std::move(protected_buffer_manager)) {
  DCHECK(protected_buffer_manager_);
}

GpuArcProtectedBufferManagerProxy::~GpuArcProtectedBufferManagerProxy() {}

void GpuArcProtectedBufferManagerProxy::GetProtectedSharedMemoryFromHandle(
    mojo::ScopedHandle dummy_handle,
    GetProtectedSharedMemoryFromHandleCallback callback) {
  base::ScopedFD unwrapped_fd = UnwrapFdFromMojoHandle(std::move(dummy_handle));

  base::ScopedFD shmem_fd(
      protected_buffer_manager_
          ->GetProtectedSharedMemoryHandleFor(std::move(unwrapped_fd))
          .Release());

  std::move(callback).Run(mojo::WrapPlatformFile(shmem_fd.release()));
}

}  // namespace arc
