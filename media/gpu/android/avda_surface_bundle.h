// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_AVDA_SURFACE_BUNDLE_H_
#define MEDIA_GPU_ANDROID_AVDA_SURFACE_BUNDLE_H_

#include "base/memory/ref_counted_delete_on_sequence.h"
#include "media/base/android/android_overlay.h"
#include "media/base/surface_manager.h"
#include "media/gpu/android/surface_texture_gl_owner.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gl/android/scoped_java_surface.h"

namespace media {

// AVDASurfaceBundle is a Java surface, and the TextureOwner or Overlay that
// backs it.
//
// Once a MediaCodec is configured with an output surface, the corresponding
// AVDASurfaceBundle should be kept alive as long as the codec to prevent
// crashes due to the codec losing its output surface.
// TODO(watk): Remove AVDA from the name.
struct MEDIA_GPU_EXPORT AVDASurfaceBundle
    : public base::RefCountedDeleteOnSequence<AVDASurfaceBundle> {
 public:
  using ScheduleLayoutCB = base::RepeatingCallback<void(gfx::Rect)>;

  // Create an empty bundle to be manually populated.
  explicit AVDASurfaceBundle();
  explicit AVDASurfaceBundle(std::unique_ptr<AndroidOverlay> overlay);
  explicit AVDASurfaceBundle(scoped_refptr<TextureOwner> texture_owner);

  const base::android::JavaRef<jobject>& GetJavaSurface() const;

  // Returns a callback that can be used to position this overlay.  It must be
  // called on the correct thread for the overlay.  It will not keep a ref to
  // |this|; the cb will do nothing if |this| is destroyed.
  ScheduleLayoutCB GetScheduleLayoutCB();

  // The Overlay or TextureOwner.
  std::unique_ptr<AndroidOverlay> overlay;
  scoped_refptr<TextureOwner> texture_owner_;

  // The Java surface for |texture_owner_|.
  gl::ScopedJavaSurface texture_owner_surface;

 private:
  ~AVDASurfaceBundle();
  friend class base::RefCountedDeleteOnSequence<AVDASurfaceBundle>;
  friend class base::DeleteHelper<AVDASurfaceBundle>;

  void ScheduleLayout(gfx::Rect rect);

  base::WeakPtrFactory<AVDASurfaceBundle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AVDASurfaceBundle);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_AVDA_SURFACE_BUNDLE_H_
