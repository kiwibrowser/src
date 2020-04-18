// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SERVICE_BROWSER_XR_DEVICE_H_
#define CHROME_BROWSER_VR_SERVICE_BROWSER_XR_DEVICE_H_

#include "device/vr/vr_device.h"

namespace vr {

class VRDisplayHost;

// This class wraps the VRDevice interface, and registers for events.
// There is one BrowserXrDevice per VRDevice (ie - one per runtime).
// It manages browser-side handling of state, like which VRDisplayHost is
// listening for device activation.
class BrowserXrDevice : public device::VRDeviceEventListener {
 public:
  explicit BrowserXrDevice(device::VRDevice* device);
  ~BrowserXrDevice() override;

  device::VRDevice* GetDevice() { return device_; }

  // device::VRDeviceEventListener
  void OnChanged(device::mojom::VRDisplayInfoPtr vr_device_info) override;
  void OnExitPresent() override;
  void OnActivate(device::mojom::VRDisplayEventReason reason,
                  base::OnceCallback<void(bool)> on_handled) override;
  void OnDeactivate(device::mojom::VRDisplayEventReason reason) override;

  // Methods called by VRDisplayHost to interact with the device.
  void OnDisplayHostAdded(VRDisplayHost* display);
  void OnDisplayHostRemoved(VRDisplayHost* display);
  void ExitPresent(VRDisplayHost* display);
  void RequestPresent(
      VRDisplayHost* display,
      device::mojom::VRSubmitFrameClientPtr submit_client,
      device::mojom::VRPresentationProviderRequest request,
      device::mojom::VRRequestPresentOptionsPtr present_options,
      device::mojom::VRDisplayHost::RequestPresentCallback callback);
  VRDisplayHost* GetPresentingDisplayHost() { return presenting_display_host_; }
  void UpdateListeningForActivate(VRDisplayHost* display);

 private:
  void OnListeningForActivate(bool is_listening);
  void OnRequestPresentResult(
      VRDisplayHost* display,
      device::mojom::VRDisplayHost::RequestPresentCallback callback,
      bool result,
      device::mojom::VRDisplayFrameTransportOptionsPtr transport_options);

  // Not owned by this class, but valid while BrowserXrDevice is alive.
  device::VRDevice* device_;

  std::set<VRDisplayHost*> displays_;
  VRDisplayHost* listening_for_activation_display_host_ = nullptr;
  VRDisplayHost* presenting_display_host_ = nullptr;

  base::WeakPtrFactory<BrowserXrDevice> weak_ptr_factory_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SERVICE_BROWSER_XR_DEVICE_H_
