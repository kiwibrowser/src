// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/vr_display_impl.h"

#include <utility>

#include "base/bind.h"
#include "device/vr/vr_device_base.h"

namespace {
constexpr int kMaxImageHeightOrWidth = 8000;
}  // namespace

namespace device {

VRDisplayImpl::VRDisplayImpl(VRDevice* device,
                             mojom::VRServiceClient* service_client,
                             mojom::VRDisplayInfoPtr display_info,
                             mojom::VRDisplayHostPtr display_host,
                             mojom::VRDisplayClientRequest client_request,
                             bool in_focused_frame)
    : binding_(this),
      device_(static_cast<VRDeviceBase*>(device)),
      in_focused_frame_(in_focused_frame) {
  mojom::VRMagicWindowProviderPtr magic_window_provider;
  binding_.Bind(mojo::MakeRequest(&magic_window_provider));
  service_client->OnDisplayConnected(
      std::move(magic_window_provider), std::move(display_host),
      std::move(client_request), std::move(display_info));
}

VRDisplayImpl::~VRDisplayImpl() = default;

// Gets a pose for magic window sessions.
void VRDisplayImpl::GetPose(GetPoseCallback callback) {
  if (!device_->IsAccessAllowed(this)) {
    std::move(callback).Run(nullptr);
    return;
  }
  device_->GetMagicWindowPose(std::move(callback));
}

// Gets frame image data for AR magic window sessions.
void VRDisplayImpl::GetFrameData(const gfx::Size& frame_size,
                                 display::Display::Rotation rotation,
                                 GetFrameDataCallback callback) {
  if (!device_->IsAccessAllowed(this)) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Check for a valid frame size.
  // While Mojo should handle negative values, we also do not want to allow 0.
  // TODO(https://crbug.com/841062): Reconsider how we check the sizes.
  if (frame_size.width() <= 0 || frame_size.height() <= 0 ||
      frame_size.width() > kMaxImageHeightOrWidth ||
      frame_size.height() > kMaxImageHeightOrWidth) {
    DLOG(ERROR) << "Invalid frame size passed to GetFrameData().";
    std::move(callback).Run(nullptr);
    return;
  }

  device_->GetMagicWindowFrameData(frame_size, rotation, std::move(callback));
}

void VRDisplayImpl::SetListeningForActivate(bool listening) {
  listening_for_activate_ = listening;
  device_->OnListeningForActivateChanged(this);
}

void VRDisplayImpl::SetInFocusedFrame(bool in_focused_frame) {
  in_focused_frame_ = in_focused_frame;
  device_->OnFrameFocusChanged(this);
}

bool VRDisplayImpl::ListeningForActivate() {
  return listening_for_activate_;
}

bool VRDisplayImpl::InFocusedFrame() {
  return in_focused_frame_;
}

}  // namespace device
