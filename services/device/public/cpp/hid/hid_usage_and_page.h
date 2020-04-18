// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_HID_PUBLIC_CPP_HID_USAGE_AND_PAGE_H_
#define DEVICE_HID_PUBLIC_CPP_HID_USAGE_AND_PAGE_H_

#include "services/device/public/mojom/hid.mojom.h"

namespace device {

// Indicates whether this usage is protected by Chrome.
bool IsProtected(const mojom::HidUsageAndPage& hid_usage_and_page);

}  // namespace device

#endif  // DEVICE_HID_PUBLIC_CPP_HID_USAGE_AND_PAGE_H_
