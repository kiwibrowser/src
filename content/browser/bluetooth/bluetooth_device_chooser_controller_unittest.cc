// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/bluetooth/bluetooth_device_chooser_controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class BluetoothDeviceChooserControllerTest : public testing::Test {};

TEST_F(BluetoothDeviceChooserControllerTest, CalculateSignalStrengthLevel) {
  EXPECT_EQ(
      0, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-128));
  EXPECT_EQ(
      0, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-80));

  EXPECT_EQ(
      1, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-79));
  EXPECT_EQ(
      1, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-70));

  EXPECT_EQ(
      2, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-69));
  EXPECT_EQ(
      2, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-62));

  EXPECT_EQ(
      3, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-61));
  EXPECT_EQ(
      3, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-53));

  EXPECT_EQ(
      4, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(-52));
  EXPECT_EQ(
      4, BluetoothDeviceChooserController::CalculateSignalStrengthLevel(127));
}

}  // namespace content