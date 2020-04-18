// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OCULUS_DEVICE_H
#define DEVICE_VR_OCULUS_DEVICE_H

#include <memory>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/vr_device_base.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"

namespace device {

class OculusRenderLoop;

class OculusDevice : public VRDeviceBase {
 public:
  explicit OculusDevice(ovrSession session, ovrGraphicsLuid luid);
  ~OculusDevice() override;

  // VRDeviceBase
  void RequestPresent(
      mojom::VRSubmitFrameClientPtr submit_client,
      mojom::VRPresentationProviderRequest request,
      mojom::VRRequestPresentOptionsPtr present_options,
      mojom::VRDisplayHost::RequestPresentCallback callback) override;
  void ExitPresent() override;
  void OnMagicWindowPoseRequest(
      mojom::VRMagicWindowProvider::GetPoseCallback callback) override;

  void OnRequestPresentResult(
      mojom::VRDisplayHost::RequestPresentCallback callback,
      bool result,
      mojom::VRDisplayFrameTransportOptionsPtr transport_options);

 private:
  std::unique_ptr<OculusRenderLoop> render_loop_;
  mojom::VRSubmitFrameClientPtr submit_client_;
  mojom::VRDisplayInfoPtr display_info_;
  ovrSession session_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  base::WeakPtrFactory<OculusDevice> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OculusDevice);
};

}  // namespace device

#endif  // DEVICE_VR_OCULUS_DEVICE_H
