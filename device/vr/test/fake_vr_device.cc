// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_vr_device.h"

namespace device {

FakeVRDevice::FakeVRDevice() : VRDeviceBase() {
  SetVRDisplayInfo(InitBasicDevice());
}

FakeVRDevice::~FakeVRDevice() {}

mojom::VRDisplayInfoPtr FakeVRDevice::InitBasicDevice() {
  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = GetId();
  display_info->displayName = "FakeVRDevice";

  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = false;
  display_info->capabilities->hasExternalDisplay = false;
  display_info->capabilities->canPresent = false;

  display_info->leftEye = InitEye(45, -0.03f, 1024);
  display_info->rightEye = InitEye(45, 0.03f, 1024);
  return display_info;
}

mojom::VREyeParametersPtr FakeVRDevice::InitEye(float fov,
                                                float offset,
                                                uint32_t size) {
  mojom::VREyeParametersPtr eye = mojom::VREyeParameters::New();

  eye->fieldOfView = mojom::VRFieldOfView::New();
  eye->fieldOfView->upDegrees = fov;
  eye->fieldOfView->downDegrees = fov;
  eye->fieldOfView->leftDegrees = fov;
  eye->fieldOfView->rightDegrees = fov;

  eye->offset.resize(3);
  eye->offset[0] = offset;
  eye->offset[1] = 0.0f;
  eye->offset[2] = 0.0f;

  eye->renderWidth = size;
  eye->renderHeight = size;

  return eye;
}

void FakeVRDevice::RequestPresent(
    mojom::VRSubmitFrameClientPtr submit_client,
    mojom::VRPresentationProviderRequest request,
    mojom::VRRequestPresentOptionsPtr present_options,
    mojom::VRDisplayHost::RequestPresentCallback callback) {
  SetIsPresenting();
  std::move(callback).Run(true, mojom::VRDisplayFrameTransportOptions::New());
}

void FakeVRDevice::ExitPresent() {
  OnExitPresent();
}

void FakeVRDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  std::move(callback).Run(pose_.Clone());
}

}  // namespace device
