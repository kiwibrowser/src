// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SERVICE_VR_DISPLAY_HOST_H_
#define CHROME_BROWSER_VR_SERVICE_VR_DISPLAY_HOST_H_

#include <memory>

#include "base/macros.h"

#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/vr_device.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class RenderFrameHost;
}

namespace device {
class VRDisplayImpl;
}  // namespace device

namespace vr {

class BrowserXrDevice;

// The browser-side host for a device::VRDisplayImpl. Controls access to VR
// APIs like poses and presentation.
class VRDisplayHost : public device::mojom::VRDisplayHost {
 public:
  VRDisplayHost(BrowserXrDevice* device,
                content::RenderFrameHost* render_frame_host,
                device::mojom::VRServiceClient* service_client,
                device::mojom::VRDisplayInfoPtr display_info);
  ~VRDisplayHost() override;

  // device::mojom::VRDisplayHost
  void RequestPresent(device::mojom::VRSubmitFrameClientPtr client,
                      device::mojom::VRPresentationProviderRequest request,
                      device::mojom::VRRequestPresentOptionsPtr options,
                      bool triggered_by_displayactive,
                      RequestPresentCallback callback) override;
  void ExitPresent() override;
  void SetListeningForActivate(bool listening);
  void SetInFocusedFrame(bool in_focused_frame);

  // Notifications/calls from BrowserXrDevice:
  void OnChanged(device::mojom::VRDisplayInfoPtr vr_device_info);
  void OnExitPresent();
  void OnBlur();
  void OnFocus();
  void OnActivate(device::mojom::VRDisplayEventReason reason,
                  base::OnceCallback<void(bool)> on_handled);
  void OnDeactivate(device::mojom::VRDisplayEventReason reason);
  bool ListeningForActivate() { return listening_for_activate_; }
  bool InFocusedFrame() { return in_focused_frame_; }

 private:
  void ReportRequestPresent();

  std::unique_ptr<device::VRDisplayImpl> display_;
  BrowserXrDevice* browser_device_ = nullptr;
  bool in_focused_frame_ = false;
  bool listening_for_activate_ = false;

  content::RenderFrameHost* render_frame_host_;
  mojo::Binding<device::mojom::VRDisplayHost> binding_;
  device::mojom::VRDisplayClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(VRDisplayHost);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SERVICE_VR_DISPLAY_HOST_H_
