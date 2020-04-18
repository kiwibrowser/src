// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/content_video_view_overlay.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "gpu/ipc/common/gpu_surface_lookup.h"

namespace media {

// static
std::unique_ptr<AndroidOverlay> ContentVideoViewOverlay::Create(
    int surface_id,
    AndroidOverlayConfig config) {
  return std::make_unique<ContentVideoViewOverlay>(surface_id,
                                                   std::move(config));
}

ContentVideoViewOverlay::ContentVideoViewOverlay(int surface_id,
                                                 AndroidOverlayConfig config)
    : surface_id_(surface_id), config_(std::move(config)), weak_factory_(this) {
  if (ContentVideoViewOverlayAllocator::GetInstance()->AllocateSurface(this)) {
    // We have the surface -- post a callback to our OnSurfaceAvailable.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ContentVideoViewOverlay::OnSurfaceAvailable,
                              weak_factory_.GetWeakPtr(), true));
  }
}

ContentVideoViewOverlay::~ContentVideoViewOverlay() {
  // Deallocate the surface.  It's okay if we don't own it.
  // Note that this only happens once any codec is done with us.
  ContentVideoViewOverlayAllocator::GetInstance()->DeallocateSurface(this);
}

void ContentVideoViewOverlay::ScheduleLayout(const gfx::Rect& rect) {}

const base::android::JavaRef<jobject>& ContentVideoViewOverlay::GetJavaSurface()
    const {
  return surface_.j_surface();
}

void ContentVideoViewOverlay::OnSurfaceAvailable(bool success) {
  if (!success) {
    // Notify that the surface won't be available.
    config_.is_failed(this);
    // |this| may be deleted.
    return;
  }

  // Get the surface and notify our client.
  surface_ =
      gpu::GpuSurfaceLookup::GetInstance()->AcquireJavaSurface(surface_id_);

  // If no surface was returned, then fail instead.
  if (surface_.IsEmpty()) {
    config_.is_failed(this);
    // |this| may be deleted.
    return;
  }

  config_.is_ready(this);
}

void ContentVideoViewOverlay::OnSurfaceDestroyed() {
  RunSurfaceDestroyedCallbacks();
  // |this| may be deleted, or deletion might be posted elsewhere.
}

int32_t ContentVideoViewOverlay::GetSurfaceId() {
  return surface_id_;
}

}  // namespace media
