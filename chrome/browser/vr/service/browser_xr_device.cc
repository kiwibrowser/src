// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/service/browser_xr_device.h"

#include "chrome/browser/vr/service/vr_display_host.h"
#include "device/vr/vr_device.h"

namespace vr {

BrowserXrDevice::BrowserXrDevice(device::VRDevice* device)
    : device_(device), weak_ptr_factory_(this) {
  device_->SetVRDeviceEventListener(this);
}

BrowserXrDevice::~BrowserXrDevice() {
  device_->SetVRDeviceEventListener(nullptr);
}

void BrowserXrDevice::OnChanged(
    device::mojom::VRDisplayInfoPtr vr_device_info) {
  for (VRDisplayHost* display : displays_) {
    display->OnChanged(vr_device_info.Clone());
  }
}

void BrowserXrDevice::OnExitPresent() {
  if (presenting_display_host_) {
    presenting_display_host_->OnExitPresent();
    presenting_display_host_ = nullptr;
  }
}

void BrowserXrDevice::OnActivate(device::mojom::VRDisplayEventReason reason,
                                 base::OnceCallback<void(bool)> on_handled) {
  if (listening_for_activation_display_host_) {
    listening_for_activation_display_host_->OnActivate(reason,
                                                       std::move(on_handled));
  } else {
    std::move(on_handled).Run(true /* will_not_present */);
  }
}

void BrowserXrDevice::OnDeactivate(device::mojom::VRDisplayEventReason reason) {
  for (VRDisplayHost* display : displays_) {
    display->OnDeactivate(reason);
  }
}

void BrowserXrDevice::OnDisplayHostAdded(VRDisplayHost* display) {
  displays_.insert(display);
}

void BrowserXrDevice::OnDisplayHostRemoved(VRDisplayHost* display) {
  DCHECK(display);
  displays_.erase(display);
  if (display == presenting_display_host_) {
    GetDevice()->ExitPresent();
    DCHECK(presenting_display_host_ == nullptr);
  }
  if (display == listening_for_activation_display_host_) {
    // Not listening for activation.
    listening_for_activation_display_host_ = nullptr;
    GetDevice()->SetListeningForActivate(false);
  }
}

void BrowserXrDevice::ExitPresent(VRDisplayHost* display) {
  if (display == presenting_display_host_) {
    GetDevice()->ExitPresent();
    DCHECK(presenting_display_host_ == nullptr);
  }
}

void BrowserXrDevice::RequestPresent(
    VRDisplayHost* display,
    device::mojom::VRSubmitFrameClientPtr submit_client,
    device::mojom::VRPresentationProviderRequest request,
    device::mojom::VRRequestPresentOptionsPtr present_options,
    device::mojom::VRDisplayHost::RequestPresentCallback callback) {
  device_->RequestPresent(
      std::move(submit_client), std::move(request), std::move(present_options),
      base::BindOnce(&BrowserXrDevice::OnRequestPresentResult,
                     weak_ptr_factory_.GetWeakPtr(), display,
                     std::move(callback)));
}

void BrowserXrDevice::OnRequestPresentResult(
    VRDisplayHost* display,
    device::mojom::VRDisplayHost::RequestPresentCallback callback,
    bool result,
    device::mojom::VRDisplayFrameTransportOptionsPtr transport_options) {
  if (result && (displays_.find(display) != displays_.end())) {
    presenting_display_host_ = display;
    std::move(callback).Run(result, std::move(transport_options));
  } else {
    std::move(callback).Run(false, nullptr);
    if (result) {
      // Stale request completed, so device thinks we are presenting.
      GetDevice()->ExitPresent();
    }
  }
}

void BrowserXrDevice::UpdateListeningForActivate(VRDisplayHost* display) {
  if (display->ListeningForActivate() && display->InFocusedFrame()) {
    bool was_listening = !!listening_for_activation_display_host_;
    listening_for_activation_display_host_ = display;
    if (!was_listening)
      OnListeningForActivate(true);
  } else if (listening_for_activation_display_host_ == display) {
    listening_for_activation_display_host_ = nullptr;
    OnListeningForActivate(false);
  }
}

void BrowserXrDevice::OnListeningForActivate(bool is_listening) {
  device_->SetListeningForActivate(is_listening);
}

}  // namespace vr
