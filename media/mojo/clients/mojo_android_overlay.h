// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_MOJO_ANDROID_OVERLAY_H_
#define MEDIA_BASE_MOJO_ANDROID_OVERLAY_H_

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "media/base/android/android_overlay.h"
#include "media/mojo/interfaces/android_overlay.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

// AndroidOverlay implementation via mojo.
class MojoAndroidOverlay : public AndroidOverlay,
                           public mojom::AndroidOverlayClient {
 public:
  MojoAndroidOverlay(mojom::AndroidOverlayProviderPtr provider_ptr,
                     AndroidOverlayConfig config,
                     const base::UnguessableToken& routing_token);

  ~MojoAndroidOverlay() override;

  // AndroidOverlay
  void ScheduleLayout(const gfx::Rect& rect) override;
  const base::android::JavaRef<jobject>& GetJavaSurface() const override;

  // mojom::AndroidOverlayClient
  void OnSurfaceReady(uint64_t surface_key) override;
  void OnDestroyed() override;
  void OnPowerEfficientState(bool is_power_efficient) override;

 private:
  AndroidOverlayConfig config_;
  mojom::AndroidOverlayPtr overlay_ptr_;
  mojo::Binding<mojom::AndroidOverlayClient> binding_;
  gl::ScopedJavaSurface surface_;

  // Have we received OnSurfaceReady yet?
  bool received_surface_ = false;

  DISALLOW_COPY_AND_ASSIGN(MojoAndroidOverlay);
};

}  // namespace media

#endif  // MEDIA_BASE_MOJO_ANDROID_OVERLAY_H_
