// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/gamepad_event_converter_evdev.h"

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
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/ozone/device/device_manager.h"
#include "ui/events/ozone/evdev/event_converter_test_util.h"
#include "ui/events/ozone/evdev/event_device_test_util.h"
#include "ui/events/ozone/evdev/event_factory_evdev.h"
#include "ui/events/ozone/gamepad/gamepad_event.h"
#include "ui/events/ozone/gamepad/gamepad_observer.h"
#include "ui/events/ozone/gamepad/gamepad_provider_ozone.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/test/scoped_event_test_tick_clock.h"

namespace {

const char kTestDevicePath[] = "/dev/input/test-device";

class TestGamepadObserver : public ui::GamepadObserver {
 public:
  TestGamepadObserver() {
    ui::GamepadProviderOzone::GetInstance()->AddGamepadObserver(this);
  }

  ~TestGamepadObserver() override {
    ui::GamepadProviderOzone::GetInstance()->RemoveGamepadObserver(this);
  }
  void OnGamepadEvent(const ui::GamepadEvent& event) override {
    events.push_back(event);
  }

  std::vector<ui::GamepadEvent> events;
};

}  // namespace

namespace ui {
class GamepadEventConverterEvdevTest : public testing::Test {
 public:
  GamepadEventConverterEvdevTest() {}

  // Overriden from testing::Test:
  void SetUp() override {
    device_manager_ = ui::CreateDeviceManagerForTest();
    event_factory_ = ui::CreateEventFactoryEvdevForTest(
        nullptr, device_manager_.get(),
        ui::KeyboardLayoutEngineManager::GetKeyboardLayoutEngine(),
        base::BindRepeating(
            &GamepadEventConverterEvdevTest::DispatchEventForTest,
            base::Unretained(this)));
    dispatcher_ =
        ui::CreateDeviceEventDispatcherEvdevForTest(event_factory_.get());
  }

  std::unique_ptr<ui::GamepadEventConverterEvdev> CreateDevice(
      const ui::DeviceCapabilities& caps) {
    int evdev_io[2];
    if (pipe(evdev_io))
      PLOG(FATAL) << "failed pipe";
    base::ScopedFD events_in(evdev_io[0]);
    base::ScopedFD events_out(evdev_io[1]);

    ui::EventDeviceInfo devinfo;
    CapabilitiesToDeviceInfo(caps, &devinfo);
    return std::make_unique<ui::GamepadEventConverterEvdev>(
        std::move(events_in), base::FilePath(kTestDevicePath), 1, devinfo,
        dispatcher_.get());
  }

 private:
  void DispatchEventForTest(ui::Event* event) {}

  std::unique_ptr<ui::GamepadEventConverterEvdev> gamepad_evdev_;
  std::unique_ptr<ui::DeviceManager> device_manager_;
  std::unique_ptr<ui::EventFactoryEvdev> event_factory_;
  std::unique_ptr<ui::DeviceEventDispatcherEvdev> dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(GamepadEventConverterEvdevTest);
};

struct ExpectedEvent {
  GamepadEventType type;
  uint16_t code;
  double value;
};

// Double value within this range will be considered equal.
const double axis_delta = 0.00001;

TEST_F(GamepadEventConverterEvdevTest, XboxGamepadEvents) {
  TestGamepadObserver observer;
  std::unique_ptr<ui::GamepadEventConverterEvdev> dev =
      CreateDevice(kXboxGamepad);

  struct input_event mock_kernel_queue[] = {
      {{1493076826, 766851}, EV_ABS, 0, 19105},
      {{1493076826, 766851}, EV_SYN, SYN_REPORT},
      {{1493076826, 774849}, EV_ABS, 0, 17931},
      {{1493076826, 774849}, EV_SYN, SYN_REPORT},
      {{1493076826, 782849}, EV_ABS, 0, 17398},
      {{1493076826, 782849}, EV_SYN, SYN_REPORT},
      {{1493076829, 670856}, EV_MSC, 4, 90007},
      {{1493076829, 670856}, EV_KEY, 310, 1},
      {{1493076829, 670856}, EV_SYN, SYN_REPORT},
      {{1493076829, 870828}, EV_MSC, 4, 90007},
      {{1493076829, 870828}, EV_KEY, 310, 0},
      {{1493076829, 870828}, EV_SYN, SYN_REPORT},
      {{1493076830, 974859}, EV_MSC, 4, 90005},
      {{1493076830, 974859}, EV_KEY, 308, 1},
      {{1493076830, 974859}, EV_SYN, SYN_REPORT},
      {{1493076831, 158857}, EV_MSC, 4, 90005},
      {{1493076831, 158857}, EV_KEY, 308, 0},
      {{1493076831, 158857}, EV_SYN, SYN_REPORT},
      {{1493076832, 62859}, EV_MSC, 4, 90002},
      {{1493076832, 62859}, EV_KEY, 305, 1},
      {{1493076832, 62859}, EV_SYN, SYN_REPORT},
      {{1493076832, 206859}, EV_MSC, 4, 90002},
      {{1493076832, 206859}, EV_KEY, 305, 0},
      {{1493076832, 206859}, EV_SYN, SYN_REPORT},
      {{1493076832, 406860}, EV_MSC, 4, 90003},
      {{1493076832, 406860}, EV_KEY, 306, 1},
      {{1493076832, 406860}, EV_SYN, SYN_REPORT},
      {{1493076832, 526871}, EV_MSC, 4, 90003},
      {{1493076832, 526871}, EV_KEY, 306, 0},
      {{1493076832, 526871}, EV_SYN, SYN_REPORT},
      {{1493076832, 750860}, EV_MSC, 4, 90004},
      {{1493076832, 750860}, EV_KEY, 307, 1},
      {{1493076832, 750860}, EV_SYN, SYN_REPORT}};

  // Advance test tick clock so the above events are strictly in the past.
  ui::test::ScopedEventTestTickClock clock;
  clock.SetNowSeconds(1493076833);

  struct ExpectedEvent expected_events[] = {
      {GamepadEventType::AXIS, 0, 0.583062}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::AXIS, 0, 0.547234}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::AXIS, 0, 0.530968}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 4, 1},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 4, 0},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 3, 1},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 3, 0},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 1, 1},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 1, 0},      {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 2, 1},      {GamepadEventType::FRAME, 0, 0}};

  for (unsigned i = 0; i < arraysize(mock_kernel_queue); ++i) {
    dev->ProcessEvent(mock_kernel_queue[i]);
  }

  for (unsigned i = 0; i < observer.events.size(); ++i) {
    EXPECT_EQ(observer.events[i].type(), expected_events[i].type);
    EXPECT_EQ(observer.events[i].code(), expected_events[i].code);
    double d = observer.events[i].value() - expected_events[i].value;
    d = d > 0 ? d : -d;
    EXPECT_LT(d, axis_delta);
  }
}

TEST_F(GamepadEventConverterEvdevTest, iBuffaloGamepadEvents) {
  TestGamepadObserver observer;
  std::unique_ptr<ui::GamepadEventConverterEvdev> dev =
      CreateDevice(kiBuffaloGamepad);

  struct input_event mock_kernel_queue[] = {
      {{1493141044, 176725}, EV_MSC, 4, 90002},
      {{1493141044, 176725}, EV_KEY, 289, 1},
      {{1493141044, 176725}, EV_SYN, SYN_REPORT},
      {{1493141044, 256722}, EV_MSC, 4, 90002},
      {{1493141044, 256722}, EV_KEY, 289, 0},
      {{1493141044, 256722}, EV_SYN, SYN_REPORT},
      {{1493141044, 400713}, EV_MSC, 4, 90001},
      {{1493141044, 400713}, EV_KEY, 288, 1},
      {{1493141044, 400713}, EV_SYN, SYN_REPORT},
      {{1493141044, 480725}, EV_MSC, 4, 90001},
      {{1493141044, 480725}, EV_KEY, 288, 0},
      {{1493141044, 480725}, EV_SYN, SYN_REPORT},
      {{1493141044, 704716}, EV_MSC, 4, 90003},
      {{1493141044, 704716}, EV_KEY, 290, 1},
      {{1493141044, 704716}, EV_SYN, SYN_REPORT},
      {{1493141044, 768721}, EV_MSC, 4, 90003},
      {{1493141044, 768721}, EV_KEY, 290, 0},
      {{1493141044, 768721}, EV_SYN, SYN_REPORT},
      {{1493141044, 960715}, EV_MSC, 4, 90004},
      {{1493141044, 960715}, EV_KEY, 291, 1},
      {{1493141044, 960715}, EV_SYN, SYN_REPORT},
      {{1493141045, 48714}, EV_MSC, 4, 90004},
      {{1493141045, 48714}, EV_KEY, 291, 0},
      {{1493141045, 48714}, EV_SYN, SYN_REPORT},
      {{1493141046, 520730}, EV_ABS, 1, 255},
      {{1493141046, 520730}, EV_SYN, SYN_REPORT},
      {{1493141046, 648727}, EV_ABS, 1, 128},
      {{1493141046, 648727}, EV_SYN, SYN_REPORT},
      {{1493141046, 848730}, EV_ABS, 1, 0},
      {{1493141046, 848730}, EV_SYN, SYN_REPORT},
      {{1493141046, 992726}, EV_ABS, 1, 128},
      {{1493141046, 992726}, EV_SYN, SYN_REPORT},
      {{1493141047, 224727}, EV_ABS, 0, 0},
      {{1493141047, 224727}, EV_SYN, SYN_REPORT},
      {{1493141047, 344724}, EV_ABS, 0, 128},
      {{1493141047, 344724}, EV_SYN, SYN_REPORT},
      {{1493141047, 552720}, EV_ABS, 0, 255},
      {{1493141047, 552720}, EV_SYN, SYN_REPORT},
      {{1493141047, 696726}, EV_ABS, 0, 128},
      {{1493141047, 696726}, EV_SYN, SYN_REPORT},
      {{1493141047, 920727}, EV_MSC, 4, 90006},
      {{1493141047, 920727}, EV_KEY, 293, 1},
      {{1493141047, 920727}, EV_SYN, SYN_REPORT},
  };

  // Advance test tick clock so the above events are strictly in the past.
  ui::test::ScopedEventTestTickClock clock;
  clock.SetNowSeconds(1493141048);

  struct ExpectedEvent expected_events[] = {
      {GamepadEventType::BUTTON, 1, 1},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 1, 0},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 0, 1},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 0, 0},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 2, 1},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 2, 0},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 3, 1},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 3, 0},  {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 13, 1}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 13, 0}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 12, 1}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 12, 0}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 14, 1}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 14, 0}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 15, 1}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 15, 0}, {GamepadEventType::FRAME, 0, 0},
      {GamepadEventType::BUTTON, 7, 1},  {GamepadEventType::FRAME, 0, 0}};

  for (unsigned i = 0; i < arraysize(mock_kernel_queue); ++i) {
    dev->ProcessEvent(mock_kernel_queue[i]);
  }

  for (unsigned i = 0; i < observer.events.size(); ++i) {
    EXPECT_EQ(observer.events[i].type(), expected_events[i].type);
    EXPECT_EQ(observer.events[i].code(), expected_events[i].code);
    double d = observer.events[i].value() - expected_events[i].value;
    d = d > 0 ? d : -d;
    EXPECT_LT(d, axis_delta);
  }
}
}  // namespace ui
