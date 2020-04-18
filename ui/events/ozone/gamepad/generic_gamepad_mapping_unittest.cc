// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/gamepad/generic_gamepad_mapping.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/ozone/device/device_manager.h"
#include "ui/events/ozone/evdev/event_converter_test_util.h"
#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/evdev/event_device_test_util.h"
#include "ui/events/ozone/evdev/event_device_util.h"
#include "ui/events/ozone/evdev/event_factory_evdev.h"
#include "ui/events/ozone/gamepad/gamepad_event.h"
#include "ui/events/ozone/gamepad/gamepad_observer.h"
#include "ui/events/ozone/gamepad/gamepad_provider_ozone.h"
#include "ui/events/ozone/gamepad/static_gamepad_mapping.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/platform_event_source.h"

namespace ui {

class GenericGamepadMappingTest : public testing::Test {
 public:
  GenericGamepadMappingTest() {}

  void CompareGamepadMapper(const GamepadMapper* l_mapper,
                            const GamepadMapper* r_mapper) {
    bool l_result, r_result;
    GamepadEventType l_mapped_type, r_mapped_type;
    uint16_t l_mapped_code, r_mapped_code;
    for (uint16_t code = BTN_MISC; code < KEY_MAX; code++) {
      l_result = l_mapper->Map(EV_KEY, code, &l_mapped_type, &l_mapped_code);
      r_result = r_mapper->Map(EV_KEY, code, &r_mapped_type, &r_mapped_code);
      EXPECT_EQ(l_result, r_result) << " Current Code: " << code;
      if (l_result) {
        EXPECT_EQ(l_mapped_type, r_mapped_type);
        EXPECT_EQ(r_mapped_code, r_mapped_code);
      }
    }
    for (uint16_t code = ABS_X; code < ABS_MAX; code++) {
      l_result = l_mapper->Map(EV_ABS, code, &l_mapped_type, &l_mapped_code);
      r_result = r_mapper->Map(EV_ABS, code, &r_mapped_type, &r_mapped_code);
      EXPECT_EQ(l_result, r_result);
      if (l_result) {
        EXPECT_EQ(l_mapped_type, r_mapped_type);
        EXPECT_EQ(r_mapped_code, r_mapped_code);
      }
    }
  }

  void TestCompatableWithCapabilities(const DeviceCapabilities& cap) {
    ui::EventDeviceInfo devinfo;
    CapabilitiesToDeviceInfo(cap, &devinfo);
    std::unique_ptr<GamepadMapper> static_mapper(
        GetStaticGamepadMapper(devinfo.vendor_id(), devinfo.product_id()));
    std::unique_ptr<GamepadMapper> generic_mapper =
        BuildGenericGamepadMapper(devinfo);
    CompareGamepadMapper(static_mapper.get(), generic_mapper.get());
  }
};

TEST_F(GenericGamepadMappingTest, XInputGamepad) {
  TestCompatableWithCapabilities(kXboxGamepad);
}

TEST_F(GenericGamepadMappingTest, HJCGamepad) {
  TestCompatableWithCapabilities(kHJCGamepad);
}

}  // namespace ui
