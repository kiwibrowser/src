// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/oculus/oculus_device_provider.h"

#include "device/gamepad/gamepad_data_fetcher_manager.h"
#include "device/vr/oculus/oculus_device.h"
#include "device/vr/oculus/oculus_gamepad_data_fetcher.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"

namespace device {

OculusVRDeviceProvider::OculusVRDeviceProvider() : initialized_(false) {}

OculusVRDeviceProvider::~OculusVRDeviceProvider() {
  device::GamepadDataFetcherManager::GetInstance()->RemoveSourceFactory(
      device::GAMEPAD_SOURCE_OCULUS);

  if (session_)
    ovr_Destroy(session_);
  ovr_Shutdown();
}

void OculusVRDeviceProvider::Initialize(
    base::RepeatingCallback<void(VRDevice*)> add_device_callback,
    base::RepeatingCallback<void(VRDevice*)> remove_device_callback,
    base::OnceClosure initialization_complete) {
  CreateDevice();
  if (device_)
    add_device_callback.Run(device_.get());
  initialized_ = true;
  std::move(initialization_complete).Run();
}

void OculusVRDeviceProvider::CreateDevice() {
  // TODO(billorr): Check for headset presence without starting runtime.
  ovrInitParams initParams = {ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL,
                              0, 0};
  ovrResult result = ovr_Initialize(&initParams);
  if (OVR_FAILURE(result)) {
    return;
  }

  // TODO(792657): luid should be used to handle multi-gpu machines.
  ovrGraphicsLuid luid;
  result = ovr_Create(&session_, &luid);
  if (OVR_FAILURE(result)) {
    return;
  }

  device_ = std::make_unique<OculusDevice>(session_, luid);
  GamepadDataFetcherManager::GetInstance()->AddFactory(
      new OculusGamepadDataFetcher::Factory(device_->GetId(), session_));
}

bool OculusVRDeviceProvider::Initialized() {
  return initialized_;
}

}  // namespace device
