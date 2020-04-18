// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/serial/serial_device_enumerator.h"

#include <memory>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(SerialDeviceEnumeratorTest, GetDevices) {
  // There is no guarantee that a test machine will have a serial device
  // available. The purpose of this test is to ensure that the process of
  // attempting to enumerate devices does not cause a crash.
  auto enumerator = SerialDeviceEnumerator::Create();
  ASSERT_TRUE(enumerator);
  std::vector<mojom::SerialDeviceInfoPtr> devices = enumerator->GetDevices();
}

}  // namespace device
