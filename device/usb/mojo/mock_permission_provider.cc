// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/mojo/mock_permission_provider.h"

#include <stddef.h>
#include <utility>

#include "device/usb/public/mojom/device.mojom.h"

using ::testing::Return;
using ::testing::_;

namespace device {
namespace usb {

MockPermissionProvider::MockPermissionProvider() : weak_factory_(this) {
  ON_CALL(*this, HasDevicePermission(_)).WillByDefault(Return(true));
}

MockPermissionProvider::~MockPermissionProvider() = default;

base::WeakPtr<PermissionProvider> MockPermissionProvider::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace usb
}  // namespace device
