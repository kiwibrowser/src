// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_TEST_FAKE_VR_DEVICE_PROVIDER_H_
#define DEVICE_VR_TEST_FAKE_VR_DEVICE_PROVIDER_H_

#include <vector>

#include "device/vr/vr_device.h"
#include "device/vr/vr_device_provider.h"
#include "device/vr/vr_export.h"

namespace device {

// TODO(mthiesse, crbug.com/769373): Remove DEVICE_VR_EXPORT.
class DEVICE_VR_EXPORT FakeVRDeviceProvider : public VRDeviceProvider {
 public:
  FakeVRDeviceProvider();
  ~FakeVRDeviceProvider() override;

  // Adds devices to the provider with the given device, which will be
  // returned when GetDevices is queried.
  void AddDevice(std::unique_ptr<VRDevice> device);
  void RemoveDevice(unsigned int device_id);

  void Initialize(base::Callback<void(VRDevice*)> add_device_callback,
                  base::Callback<void(VRDevice*)> remove_device_callback,
                  base::OnceClosure initialization_complete) override;
  bool Initialized() override;

 private:
  std::vector<std::unique_ptr<VRDevice>> devices_;
  bool initialized_;
  base::Callback<void(VRDevice*)> add_device_callback_;
  base::Callback<void(VRDevice*)> remove_device_callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeVRDeviceProvider);
};

}  // namespace device

#endif  // DEVICE_VR_TEST_FAKE_VR_DEVICE_PROVIDER_H_
