// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SERVICE_VR_SERVICE_IMPL_H_
#define CHROME_BROWSER_VR_SERVICE_VR_SERVICE_IMPL_H_

#include <memory>

#include "base/macros.h"

#include "content/public/browser/web_contents_observer.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/vr_device.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace vr {

class VRDisplayHost;
class BrowserXrDevice;

// Browser process representation of a WebVR site session. Instantiated through
// Mojo once the user loads a page containing WebVR.
// It instantiates a VRDisplayImpl for each newly connected VRDisplay and sends
// the display's info to the render process through its connected
// mojom::VRServiceClient.
class VRServiceImpl : public device::mojom::VRService,
                      content::WebContentsObserver {
 public:
  explicit VRServiceImpl(content::RenderFrameHost* render_frame_host);
  ~VRServiceImpl() override;

  static void Create(content::RenderFrameHost* render_frame_host,
                     device::mojom::VRServiceRequest request);

  // device::mojom::VRService implementation
  // Adds this service to the VRDeviceManager.
  void SetClient(device::mojom::VRServiceClientPtr service_client,
                 SetClientCallback callback) override;

  // Tells the renderer that a new VR device is available.
  void ConnectDevice(BrowserXrDevice* device);

  // Tells the renderer that a VR device has gone away.
  void RemoveDevice(BrowserXrDevice* device);

  void InitializationComplete();

 protected:
  // Constructor for tests.
  VRServiceImpl();

  int NumberOfConnectedDisplayHosts() { return displays_.size(); }

 private:
  void SetBinding(mojo::StrongBindingPtr<VRService> binding);

  // device::mojom::VRService implementation
  void SetListeningForActivate(bool listening) override;

  // content::WebContentsObserver implementation
  void OnWebContentsFocused(content::RenderWidgetHost* host) override;
  void OnWebContentsLostFocus(content::RenderWidgetHost* host) override;
  void RenderFrameDeleted(content::RenderFrameHost* host) override;

  void OnWebContentsFocusChanged(content::RenderWidgetHost* host, bool focused);

  std::map<BrowserXrDevice*, std::unique_ptr<VRDisplayHost>> displays_;
  SetClientCallback set_client_callback_;
  device::mojom::VRServiceClientPtr client_;
  content::RenderFrameHost* render_frame_host_;
  mojo::StrongBindingPtr<VRService> binding_;

  DISALLOW_COPY_AND_ASSIGN(VRServiceImpl);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SERVICE_VR_SERVICE_IMPL_H_
