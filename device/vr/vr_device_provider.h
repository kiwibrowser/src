// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_VR_DEVICE_PROVIDER_H
#define DEVICE_VR_VR_DEVICE_PROVIDER_H

#include <vector>

#include "base/callback.h"

namespace device {

class VRDevice;

class VRDeviceProvider {
 public:
  VRDeviceProvider() {}
  virtual ~VRDeviceProvider() {}

  // If the VR API requires initialization that should happen here.
  virtual void Initialize(
      base::Callback<void(VRDevice*)> add_device_callback,
      base::Callback<void(VRDevice*)> remove_device_callback,
      base::OnceClosure initialization_complete) = 0;

  // Returns true if initialization is complete.
  virtual bool Initialized() = 0;
};

}  // namespace device

#endif  // DEVICE_VR_VR_DEVICE_PROVIDER_H
