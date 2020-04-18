// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_device.h"

#include <utility>

namespace device {

FidoDevice::FidoDevice() = default;
FidoDevice::~FidoDevice() = default;

void FidoDevice::SetDeviceInfo(AuthenticatorGetInfoResponse device_info) {
  device_info_ = std::move(device_info);
}

}  // namespace device
