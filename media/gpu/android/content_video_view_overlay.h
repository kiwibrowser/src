// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_H_
#define MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "media/base/android/android_overlay.h"
#include "media/gpu/android/content_video_view_overlay_allocator.h"
#include "ui/gl/android/scoped_java_surface.h"

namespace media {

class ContentVideoViewOverlay
    : public ContentVideoViewOverlayAllocator::Client {
 public:
  // This exists so we can bind construction into a callback returning
  // std::unique_ptr<AndroidOverlay>.
  static std::unique_ptr<AndroidOverlay> Create(int surface_id,
                                                AndroidOverlayConfig config);

  // |config| is ignored except for callbacks.  Callbacks will not be called
  // before this returns.
  ContentVideoViewOverlay(int surface_id, AndroidOverlayConfig config);
  ~ContentVideoViewOverlay() override;

  // AndroidOverlay (via ContentVideoViewOverlayAllocator::Client)
  // ContentVideoView ignores this, unfortunately.
  void ScheduleLayout(const gfx::Rect& rect) override;
  const base::android::JavaRef<jobject>& GetJavaSurface() const override;

  // ContentVideoViewOverlayAllocator::Client
  void OnSurfaceAvailable(bool success) override;
  void OnSurfaceDestroyed() override;
  int32_t GetSurfaceId() override;

 private:
  int surface_id_;
  AndroidOverlayConfig config_;
  gl::ScopedJavaSurface surface_;

  base::WeakPtrFactory<ContentVideoViewOverlay> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(ContentVideoViewOverlay);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_CONTENT_VIDEO_VIEW_OVERLAY_H_
