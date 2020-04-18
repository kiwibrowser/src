// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OCULUS_DEVICE_PROVIDER_H
#define DEVICE_VR_OCULUS_DEVICE_PROVIDER_H

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "device/vr/vr_device_provider.h"
#include "device/vr/vr_export.h"

typedef struct ovrHmdStruct* ovrSession;

namespace device {

class OculusDevice;

class DEVICE_VR_EXPORT OculusVRDeviceProvider : public VRDeviceProvider {
 public:
  OculusVRDeviceProvider();
  ~OculusVRDeviceProvider() override;

  void Initialize(
      base::RepeatingCallback<void(VRDevice*)> add_device_callback,
      base::RepeatingCallback<void(VRDevice*)> remove_device_callback,
      base::OnceClosure initialization_complete) override;

  bool Initialized() override;

 private:
  void CreateDevice();

  bool initialized_;
  ovrSession session_ = nullptr;
  std::unique_ptr<OculusDevice> device_;

  DISALLOW_COPY_AND_ASSIGN(OculusVRDeviceProvider);
};

}  // namespace device

#endif  // DEVICE_VR_OCULUS_DEVICE_PROVIDER_H
