// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/mojo/device_struct_traits_test.mojom.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/gfx/geometry/size.h"

namespace ui {

namespace {

class DeviceStructTraitsTest : public testing::Test,
                               public mojom::DeviceStructTraitsTest {
 public:
  DeviceStructTraitsTest() {}

 protected:
  mojom::DeviceStructTraitsTestPtr GetTraitsTestProxy() {
    mojom::DeviceStructTraitsTestPtr proxy;
    traits_test_bindings_.AddBinding(this, mojo::MakeRequest(&proxy));
    return proxy;
  }

 private:
  // mojom::DeviceStructTraitsTest:
  void EchoInputDevice(const InputDevice& in,
                       EchoInputDeviceCallback callback) override {
    std::move(callback).Run(in);
  }

  void EchoTouchscreenDevice(const TouchscreenDevice& in,
                             EchoTouchscreenDeviceCallback callback) override {
    std::move(callback).Run(in);
  }

  base::MessageLoop loop_;  // A MessageLoop is needed for mojo IPC to work.
  mojo::BindingSet<mojom::DeviceStructTraitsTest> traits_test_bindings_;

  DISALLOW_COPY_AND_ASSIGN(DeviceStructTraitsTest);
};

}  // namespace

TEST_F(DeviceStructTraitsTest, InputDevice) {
  InputDevice input(15,                     // id
                    INPUT_DEVICE_INTERNAL,  // type
                    "Input Device");        // name
  input.sys_path = base::FilePath::FromUTF8Unsafe("/dev/input/event14");
  input.vendor_id = 1000;
  input.product_id = 2000;

  mojom::DeviceStructTraitsTestPtr proxy = GetTraitsTestProxy();
  InputDevice output;
  proxy->EchoInputDevice(input, &output);

  EXPECT_EQ(input.id, output.id);
  EXPECT_EQ(input.type, output.type);
  EXPECT_EQ(input.name, output.name);
  EXPECT_EQ(input.sys_path, output.sys_path);
  EXPECT_EQ(input.vendor_id, output.vendor_id);
  EXPECT_EQ(input.product_id, output.product_id);
}

TEST_F(DeviceStructTraitsTest, TouchscreenDevice) {
  TouchscreenDevice input(10,                    // id
                          INPUT_DEVICE_UNKNOWN,  // type
                          "Touchscreen Device",  // name
                          gfx::Size(123, 456),   // size
                          3,                     // touch_points
                          true);                 // has_stylus
  // Not setting sys_path intentionally.
  input.vendor_id = 0;
  input.product_id = 0;

  mojom::DeviceStructTraitsTestPtr proxy = GetTraitsTestProxy();
  TouchscreenDevice output;
  proxy->EchoTouchscreenDevice(input, &output);

  EXPECT_EQ(input.id, output.id);
  EXPECT_EQ(input.type, output.type);
  EXPECT_EQ(input.name, output.name);
  EXPECT_EQ(input.sys_path, output.sys_path);
  EXPECT_EQ(input.vendor_id, output.vendor_id);
  EXPECT_EQ(input.product_id, output.product_id);
  EXPECT_EQ(input.size, output.size);
  EXPECT_EQ(input.touch_points, output.touch_points);
  EXPECT_EQ(input.has_stylus, output.has_stylus);
}

}  // namespace ui
