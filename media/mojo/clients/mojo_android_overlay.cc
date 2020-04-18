// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_android_overlay.h"

#include "gpu/ipc/common/gpu_surface_lookup.h"
#include "services/service_manager/public/cpp/connect.h"

namespace media {

MojoAndroidOverlay::MojoAndroidOverlay(
    mojom::AndroidOverlayProviderPtr provider_ptr,
    AndroidOverlayConfig config,
    const base::UnguessableToken& routing_token)
    : config_(std::move(config)), binding_(this) {
  // Fill in details of |config| into |mojo_config|.  Our caller could do this
  // too, but since we want to retain |config_| anyway, we do it here.
  mojom::AndroidOverlayConfigPtr mojo_config =
      mojom::AndroidOverlayConfig::New();
  mojo_config->routing_token = routing_token;
  mojo_config->rect = config_.rect;
  mojo_config->secure = config_.secure;
  mojo_config->power_efficient = config_.power_efficient;

  mojom::AndroidOverlayClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  provider_ptr->CreateOverlay(mojo::MakeRequest(&overlay_ptr_), std::move(ptr),
                              std::move(mojo_config));
}

MojoAndroidOverlay::~MojoAndroidOverlay() {
  // Dropping |overlay_ptr_| will signal to the implementation that we're done
  // with the surface.  If a synchronous destroy is pending, then it can be
  // allowed to continue.
}

void MojoAndroidOverlay::ScheduleLayout(const gfx::Rect& rect) {
  // If we haven't gotten the surface yet, then ignore this.
  if (!received_surface_)
    return;

  overlay_ptr_->ScheduleLayout(rect);
}

const base::android::JavaRef<jobject>& MojoAndroidOverlay::GetJavaSurface()
    const {
  return surface_.j_surface();
}

void MojoAndroidOverlay::OnSurfaceReady(uint64_t surface_key) {
  received_surface_ = true;

  // Get the surface and notify our client.
  surface_ =
      gpu::GpuSurfaceLookup::GetInstance()->AcquireJavaSurface(surface_key);

  // If no surface was returned, then fail instead.
  if (surface_.IsEmpty()) {
    config_.is_failed(this);
    // |this| may be deleted.
    return;
  }

  config_.is_ready(this);
}

void MojoAndroidOverlay::OnDestroyed() {
  // Note that |overlay_ptr_| might not be bound yet, or we might not have ever
  // gotten a surface.  Regardless, the overlay cannot be used.

  if (!received_surface_)
    config_.is_failed(this);
  else
    RunSurfaceDestroyedCallbacks();

  // Note: we do not delete |overlay_ptr_| here.  Our client must delete us to
  // signal that we should do that, since it still might be in use.
}

void MojoAndroidOverlay::OnPowerEfficientState(bool is_power_efficient) {
  if (config_.power_cb)
    config_.power_cb.Run(this, is_power_efficient);
}

}  // namespace media
