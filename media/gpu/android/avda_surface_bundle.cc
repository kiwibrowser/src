// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/avda_surface_bundle.h"

#include "base/threading/sequenced_task_runner_handle.h"
#include "media/base/android/android_overlay.h"

namespace media {

AVDASurfaceBundle::AVDASurfaceBundle()
    : RefCountedDeleteOnSequence<AVDASurfaceBundle>(
          base::SequencedTaskRunnerHandle::Get()),
      weak_factory_(this) {}

AVDASurfaceBundle::AVDASurfaceBundle(std::unique_ptr<AndroidOverlay> overlay)
    : RefCountedDeleteOnSequence<AVDASurfaceBundle>(
          base::SequencedTaskRunnerHandle::Get()),
      overlay(std::move(overlay)),
      weak_factory_(this) {}

AVDASurfaceBundle::AVDASurfaceBundle(scoped_refptr<TextureOwner> texture_owner)
    : RefCountedDeleteOnSequence<AVDASurfaceBundle>(
          base::SequencedTaskRunnerHandle::Get()),
      texture_owner_(std::move(texture_owner)),
      texture_owner_surface(texture_owner_->CreateJavaSurface()),
      weak_factory_(this) {}

AVDASurfaceBundle::~AVDASurfaceBundle() {
  // Explicitly free the surface first, just to be sure that it's deleted before
  // the TextureOwner is.
  texture_owner_surface = gl::ScopedJavaSurface();

  // Also release the back buffers.
  if (texture_owner_) {
    auto task_runner = texture_owner_->task_runner();
    if (task_runner->RunsTasksInCurrentSequence()) {
      texture_owner_->ReleaseBackBuffers();
    } else {
      task_runner->PostTask(
          FROM_HERE, base::BindRepeating(&TextureOwner::ReleaseBackBuffers,
                                         texture_owner_));
    }
  }
}

const base::android::JavaRef<jobject>& AVDASurfaceBundle::GetJavaSurface()
    const {
  if (overlay)
    return overlay->GetJavaSurface();
  else
    return texture_owner_surface.j_surface();
}

AVDASurfaceBundle::ScheduleLayoutCB AVDASurfaceBundle::GetScheduleLayoutCB() {
  return base::BindRepeating(&AVDASurfaceBundle::ScheduleLayout,
                             weak_factory_.GetWeakPtr());
}

void AVDASurfaceBundle::ScheduleLayout(gfx::Rect rect) {
  if (layout_rect_ == rect)
    return;
  layout_rect_ = rect;

  if (overlay)
    overlay->ScheduleLayout(rect);
}

}  // namespace media
