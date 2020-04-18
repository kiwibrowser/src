// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_event_converter_evdev.h"

#include <errno.h>
#include <linux/input.h>
#include <stddef.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/posix/eintr_wrapper.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/event_switches.h"
#include "ui/events/ozone/evdev/device_event_dispatcher_evdev.h"
#include "ui/events/ozone/evdev/event_device_test_util.h"
#include "ui/events/ozone/evdev/touch_evdev_types.h"
#include "ui/events/ozone/evdev/touch_filter/false_touch_finder.h"
#include "ui/events/ozone/evdev/touch_filter/touch_filter.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/test/scoped_event_test_tick_clock.h"

namespace ui {

namespace {

const char kTestDevicePath[] = "/dev/input/test-device";

// Returns a fake TimeTicks based on the given microsecond offset.
base::TimeTicks ToTestTimeTicks(int64_t micros) {
  return base::TimeTicks() + base::TimeDelta::FromMicroseconds(micros);
}

void InitPixelTouchscreen(TouchEventConverterEvdev* device) {
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kLinkTouchscreen, &devinfo));
  device->Initialize(devinfo);
}

void InitEloTouchscreen(TouchEventConverterEvdev* device) {
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kElo_TouchSystems_2700, &devinfo));
  device->Initialize(devinfo);
}

enum class GenericEventParamsType { MOUSE_MOVE, MOUSE_BUTTON, TOUCH };

struct GenericEventParams {
  GenericEventParams() {}
  GenericEventParams(const GenericEventParams& other) {
    type = other.type;
    switch (type) {
      case GenericEventParamsType::MOUSE_MOVE:
        mouse_move = other.mouse_move;
        break;
      case GenericEventParamsType::MOUSE_BUTTON:
        mouse_button = other.mouse_button;
        break;
      case GenericEventParamsType::TOUCH:
        touch = other.touch;
        break;
    }
  }
  ~GenericEventParams() {}

  GenericEventParamsType type;
  union {
    MouseMoveEventParams mouse_move;
    MouseButtonEventParams mouse_button;
    TouchEventParams touch;
  };
};

}  // namespace

class MockTouchEventConverterEvdev : public TouchEventConverterEvdev {
 public:
  MockTouchEventConverterEvdev(base::ScopedFD fd,
                               base::FilePath path,
                               const EventDeviceInfo& devinfo,
                               DeviceEventDispatcherEvdev* dispatcher);
  ~MockTouchEventConverterEvdev() override;

  void ConfigureReadMock(struct input_event* queue,
                         long read_this_many,
                         long queue_index);

  // Actually dispatch the event reader code.
  void ReadNow() {
    OnFileCanReadWithoutBlocking(read_pipe_);
    base::RunLoop().RunUntilIdle();
  }

  void SimulateReinitialize(const EventDeviceInfo& devinfo) {
    Initialize(devinfo);
  }

  void Reinitialize() override {}

  FalseTouchFinder* false_touch_finder() { return false_touch_finder_.get(); }

 private:
  int read_pipe_;
  int write_pipe_;

  DISALLOW_COPY_AND_ASSIGN(MockTouchEventConverterEvdev);
};

class MockDeviceEventDispatcherEvdev : public DeviceEventDispatcherEvdev {
 public:
  MockDeviceEventDispatcherEvdev(
      const base::RepeatingCallback<void(const GenericEventParams& params)>&
          callback)
      : callback_(callback) {}
  ~MockDeviceEventDispatcherEvdev() override {}

  // DeviceEventDispatcherEvdev:
  void DispatchKeyEvent(const KeyEventParams& params) override {}
  void DispatchMouseMoveEvent(const MouseMoveEventParams& params) override {
    GenericEventParams generic;
    generic.type = GenericEventParamsType::MOUSE_MOVE;
    generic.mouse_move = params;
    callback_.Run(generic);
  }
  void DispatchMouseButtonEvent(const MouseButtonEventParams& params) override {
    GenericEventParams generic;
    generic.type = GenericEventParamsType::MOUSE_BUTTON;
    generic.mouse_button = params;
    callback_.Run(generic);
  }
  void DispatchMouseWheelEvent(const MouseWheelEventParams& params) override {}
  void DispatchPinchEvent(const PinchEventParams& params) override {}
  void DispatchScrollEvent(const ScrollEventParams& params) override {}
  void DispatchTouchEvent(const TouchEventParams& params) override {
    GenericEventParams generic;
    generic.type = GenericEventParamsType::TOUCH;
    generic.touch = params;
    callback_.Run(generic);
  }

  void DispatchKeyboardDevicesUpdated(
      const std::vector<InputDevice>& devices) override {}
  void DispatchTouchscreenDevicesUpdated(
      const std::vector<TouchscreenDevice>& devices) override {}
  void DispatchMouseDevicesUpdated(
      const std::vector<InputDevice>& devices) override {}
  void DispatchTouchpadDevicesUpdated(
      const std::vector<InputDevice>& devices) override {}
  void DispatchDeviceListsComplete() override {}
  void DispatchStylusStateChanged(StylusState stylus_state) override {}

  // Dispatch Gamepad Event.
  void DispatchGamepadEvent(const GamepadEvent& event) override {}

  void DispatchGamepadDevicesUpdated(
      const std::vector<InputDevice>& devices) override {}

 private:
  base::RepeatingCallback<void(const GenericEventParams& params)> callback_;
};

MockTouchEventConverterEvdev::MockTouchEventConverterEvdev(
    base::ScopedFD fd,
    base::FilePath path,
    const EventDeviceInfo& devinfo,
    DeviceEventDispatcherEvdev* dispatcher)
    : TouchEventConverterEvdev(std::move(fd), path, 1, devinfo, dispatcher) {
  int fds[2];

  if (pipe(fds))
    PLOG(FATAL) << "failed pipe";

  EXPECT_TRUE(base::SetNonBlocking(fds[0]) || base::SetNonBlocking(fds[1]))
    << "failed to set non-blocking: " << strerror(errno);

  read_pipe_ = fds[0];
  write_pipe_ = fds[1];

  events_.resize(ui::kNumTouchEvdevSlots);
  for (size_t i = 0; i < events_.size(); ++i)
    events_[i].slot = i;

  SetEnabled(true);
}

MockTouchEventConverterEvdev::~MockTouchEventConverterEvdev() {
  SetEnabled(false);
}

void MockTouchEventConverterEvdev::ConfigureReadMock(struct input_event* queue,
                                                     long read_this_many,
                                                     long queue_index) {
  int nwrite = HANDLE_EINTR(write(write_pipe_,
                                  queue + queue_index,
                                  sizeof(struct input_event) * read_this_many));
  DCHECK(nwrite ==
         static_cast<int>(sizeof(struct input_event) * read_this_many))
      << "write() failed, errno: " << errno;
}

// Test fixture.
class TouchEventConverterEvdevTest : public testing::Test {
 public:
  TouchEventConverterEvdevTest() {}

  // Overridden from testing::Test:
  void SetUp() override {
    // Set up pipe to satisfy message pump (unused).
    int evdev_io[2];
    if (pipe(evdev_io))
      PLOG(FATAL) << "failed pipe";
    base::ScopedFD events_in(evdev_io[0]);
    events_out_.reset(evdev_io[1]);

    // Device creation happens on a worker thread since it may involve blocking
    // operations. Simulate that by creating it before creating a UI message
    // loop.
    EventDeviceInfo devinfo;
    dispatcher_.reset(new ui::MockDeviceEventDispatcherEvdev(
        base::BindRepeating(&TouchEventConverterEvdevTest::DispatchCallback,
                            base::Unretained(this))));
    device_.reset(new ui::MockTouchEventConverterEvdev(
        std::move(events_in), base::FilePath(kTestDevicePath), devinfo,
        dispatcher_.get()));
    device_->Initialize(devinfo);
    loop_ = new base::MessageLoopForUI;

    test_clock_.reset(new ui::test::ScopedEventTestTickClock());
    ui::DeviceDataManager::CreateInstance();
  }

  void TearDown() override {
    device_.reset();
    delete loop_;
  }

  ui::MockTouchEventConverterEvdev* device() { return device_.get(); }

  unsigned size() { return dispatched_events_.size(); }
  const ui::TouchEventParams& dispatched_touch_event(unsigned index) {
    DCHECK_GT(dispatched_events_.size(), index);
    EXPECT_EQ(GenericEventParamsType::TOUCH, dispatched_events_[index].type);
    return dispatched_events_[index].touch;
  }
  const ui::MouseMoveEventParams& dispatched_mouse_move_event(unsigned index) {
    DCHECK_GT(dispatched_events_.size(), index);
    EXPECT_EQ(GenericEventParamsType::MOUSE_MOVE,
              dispatched_events_[index].type);
    return dispatched_events_[index].mouse_move;
  }
  const ui::MouseButtonEventParams& dispatched_mouse_button_event(
      unsigned index) {
    DCHECK_GT(dispatched_events_.size(), index);
    EXPECT_EQ(GenericEventParamsType::MOUSE_BUTTON,
              dispatched_events_[index].type);
    return dispatched_events_[index].mouse_button;
  }
  void ClearDispatchedEvents() { dispatched_events_.clear(); }

  void DestroyDevice() { device_.reset(); }

  void SetTestNowTime(timeval time) {
    base::TimeTicks ticks = base::TimeTicks() +
                            base::TimeDelta::FromSeconds(time.tv_sec) +
                            base::TimeDelta::FromMicroseconds(time.tv_usec);
    test_clock_->SetNowTicks(ticks);
  }

 private:
  base::MessageLoop* loop_;
  std::unique_ptr<ui::MockTouchEventConverterEvdev> device_;
  std::unique_ptr<ui::MockDeviceEventDispatcherEvdev> dispatcher_;
  std::unique_ptr<ui::test::ScopedEventTestTickClock> test_clock_;

  base::ScopedFD events_out_;

  void DispatchCallback(const GenericEventParams& params) {
    dispatched_events_.push_back(params);
  }
  std::vector<GenericEventParams> dispatched_events_;

  DISALLOW_COPY_AND_ASSIGN(TouchEventConverterEvdevTest);
};

TEST_F(TouchEventConverterEvdevTest, NoEvents) {
  ui::MockTouchEventConverterEvdev* dev = device();
  dev->ConfigureReadMock(NULL, 0, 0);
  EXPECT_EQ(0u, size());
}

TEST_F(TouchEventConverterEvdevTest, TouchMove) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  // Captured from Chromebook Pixel (Link).
  timeval time;
  time = {1427323282, 19203};
  struct input_event mock_kernel_queue_press[] = {
      {time, EV_ABS, ABS_MT_TRACKING_ID, 3},
      {time, EV_ABS, ABS_MT_POSITION_X, 295},
      {time, EV_ABS, ABS_MT_POSITION_Y, 421},
      {time, EV_ABS, ABS_MT_PRESSURE, 34},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 295},
      {time, EV_ABS, ABS_Y, 421},
      {time, EV_ABS, ABS_PRESSURE, 34},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  time = {1427323282, 34693};
  struct input_event mock_kernel_queue_move[] = {
      {time, EV_ABS, ABS_MT_POSITION_X, 312},
      {time, EV_ABS, ABS_MT_POSITION_Y, 432},
      {time, EV_ABS, ABS_MT_PRESSURE, 43},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 100},
      {time, EV_ABS, ABS_X, 312},
      {time, EV_ABS, ABS_Y, 432},
      {time, EV_ABS, ABS_PRESSURE, 43},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  time = {1427323282, 144540};
  struct input_event mock_kernel_queue_release[] = {
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_KEY, BTN_TOUCH, 0},
      {time, EV_ABS, ABS_PRESSURE, 0},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test time to ensure above timestamps are strictly in the past.
  SetTestNowTime(time);

  // Press.
  dev->ConfigureReadMock(mock_kernel_queue_press,
                         arraysize(mock_kernel_queue_press), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());
  ui::TouchEventParams event = dispatched_touch_event(0);
  EXPECT_EQ(ui::ET_TOUCH_PRESSED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1427323282019203), event.timestamp);
  EXPECT_EQ(295, event.location.x());
  EXPECT_EQ(421, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_TOUCH,
            event.pointer_details.pointer_type);
  EXPECT_FLOAT_EQ(58.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.13333334f, event.pointer_details.force);

  // Move.
  dev->ConfigureReadMock(mock_kernel_queue_move,
                         arraysize(mock_kernel_queue_move), 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());
  event = dispatched_touch_event(1);
  EXPECT_EQ(ui::ET_TOUCH_MOVED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1427323282034693), event.timestamp);
  EXPECT_EQ(312, event.location.x());
  EXPECT_EQ(432, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_TOUCH,
            event.pointer_details.pointer_type);
  EXPECT_FLOAT_EQ(50.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.16862745f, event.pointer_details.force);

  // Release.
  dev->ConfigureReadMock(mock_kernel_queue_release,
                         arraysize(mock_kernel_queue_release), 0);
  dev->ReadNow();
  EXPECT_EQ(3u, size());
  event = dispatched_touch_event(2);
  EXPECT_EQ(ui::ET_TOUCH_RELEASED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1427323282144540), event.timestamp);
  EXPECT_EQ(312, event.location.x());
  EXPECT_EQ(432, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_TOUCH,
            event.pointer_details.pointer_type);
  EXPECT_FLOAT_EQ(50.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.16862745f, event.pointer_details.force);
}

TEST_F(TouchEventConverterEvdevTest, TwoFingerGesture) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  struct input_event mock_kernel_queue_press0[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 684},
    {{0, 0}, EV_ABS, ABS_MT_TOUCH_MAJOR, 3},
    {{0, 0}, EV_ABS, ABS_MT_PRESSURE, 45},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 42},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 51}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  // Setup and discard a press.
  dev->ConfigureReadMock(mock_kernel_queue_press0, 6, 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  struct input_event mock_kernel_queue_move0[] = {
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  // Setup and discard a move.
  dev->ConfigureReadMock(mock_kernel_queue_move0, 2, 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());

  struct input_event mock_kernel_queue_move0press1[] = {
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40}, {{0, 0}, EV_SYN, SYN_REPORT, 0},
    {{0, 0}, EV_ABS, ABS_MT_SLOT, 1}, {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 686},
    {{0, 0}, EV_ABS, ABS_MT_TOUCH_MAJOR, 3},
    {{0, 0}, EV_ABS, ABS_MT_PRESSURE, 45},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 101},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 102}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  // Move on 0, press on 1.
  dev->ConfigureReadMock(mock_kernel_queue_move0press1, 9, 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());
  ui::TouchEventParams ev0 = dispatched_touch_event(2);
  ui::TouchEventParams ev1 = dispatched_touch_event(3);

  // Move
  EXPECT_EQ(ui::ET_TOUCH_MOVED, ev0.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev0.timestamp);
  EXPECT_EQ(40, ev0.location.x());
  EXPECT_EQ(51, ev0.location.y());
  EXPECT_EQ(0, ev0.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev0.pointer_details.force);

  // Press
  EXPECT_EQ(ui::ET_TOUCH_PRESSED, ev1.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev1.timestamp);
  EXPECT_EQ(101, ev1.location.x());
  EXPECT_EQ(102, ev1.location.y());
  EXPECT_EQ(1, ev1.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev1.pointer_details.force);

  // Stationary 0, Moves 1.
  struct input_event mock_kernel_queue_stationary0_move1[] = {
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  dev->ConfigureReadMock(mock_kernel_queue_stationary0_move1, 2, 0);
  dev->ReadNow();
  EXPECT_EQ(5u, size());
  ev1 = dispatched_touch_event(4);

  EXPECT_EQ(ui::ET_TOUCH_MOVED, ev1.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev1.timestamp);
  EXPECT_EQ(40, ev1.location.x());
  EXPECT_EQ(102, ev1.location.y());
  EXPECT_EQ(1, ev1.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev1.pointer_details.force);

  // Move 0, stationary 1.
  struct input_event mock_kernel_queue_move0_stationary1[] = {
    {{0, 0}, EV_ABS, ABS_MT_SLOT, 0}, {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 39},
    {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  dev->ConfigureReadMock(mock_kernel_queue_move0_stationary1, 3, 0);
  dev->ReadNow();
  EXPECT_EQ(6u, size());
  ev0 = dispatched_touch_event(5);

  EXPECT_EQ(ui::ET_TOUCH_MOVED, ev0.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev0.timestamp);
  EXPECT_EQ(39, ev0.location.x());
  EXPECT_EQ(51, ev0.location.y());
  EXPECT_EQ(0, ev0.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev0.pointer_details.force);

  // Release 0, move 1.
  struct input_event mock_kernel_queue_release0_move1[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, -1}, {{0, 0}, EV_ABS, ABS_MT_SLOT, 1},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 38}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };
  dev->ConfigureReadMock(mock_kernel_queue_release0_move1, 4, 0);
  dev->ReadNow();
  EXPECT_EQ(8u, size());
  ev0 = dispatched_touch_event(6);
  ev1 = dispatched_touch_event(7);

  EXPECT_EQ(ui::ET_TOUCH_RELEASED, ev0.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev0.timestamp);
  EXPECT_EQ(39, ev0.location.x());
  EXPECT_EQ(51, ev0.location.y());
  EXPECT_EQ(0, ev0.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev0.pointer_details.force);

  EXPECT_EQ(ui::ET_TOUCH_MOVED, ev1.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev1.timestamp);
  EXPECT_EQ(38, ev1.location.x());
  EXPECT_EQ(102, ev1.location.y());
  EXPECT_EQ(1, ev1.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev1.pointer_details.force);

  // Release 1.
  struct input_event mock_kernel_queue_release1[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, -1}, {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };
  dev->ConfigureReadMock(mock_kernel_queue_release1, 2, 0);
  dev->ReadNow();
  EXPECT_EQ(9u, size());
  ev1 = dispatched_touch_event(8);

  EXPECT_EQ(ui::ET_TOUCH_RELEASED, ev1.type);
  EXPECT_EQ(ToTestTimeTicks(0), ev1.timestamp);
  EXPECT_EQ(38, ev1.location.x());
  EXPECT_EQ(102, ev1.location.y());
  EXPECT_EQ(1, ev1.slot);
  EXPECT_FLOAT_EQ(0.17647059f, ev1.pointer_details.force);
}

TEST_F(TouchEventConverterEvdevTest, Unsync) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  struct input_event mock_kernel_queue_press0[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 684},
    {{0, 0}, EV_ABS, ABS_MT_TOUCH_MAJOR, 3},
    {{0, 0}, EV_ABS, ABS_MT_PRESSURE, 45},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 42},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 51}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };

  dev->ConfigureReadMock(mock_kernel_queue_press0, 6, 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  // Prepare a move with a drop.
  struct input_event mock_kernel_queue_move0[] = {
    {{0, 0}, EV_SYN, SYN_DROPPED, 0},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };

  // Verify that we didn't receive it/
  dev->ConfigureReadMock(mock_kernel_queue_move0, 3, 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  struct input_event mock_kernel_queue_move1[] = {
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40}, {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };

  // Verify that it re-syncs after a SYN_REPORT.
  dev->ConfigureReadMock(mock_kernel_queue_move1, 2, 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());
}

TEST_F(TouchEventConverterEvdevTest, ShouldResumeExistingContactsOnStart) {
  ui::MockTouchEventConverterEvdev* dev = device();

  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kLinkTouchscreen, &devinfo));

  // Set up an existing contact in slot 0.
  devinfo.SetAbsMtSlot(ABS_MT_TRACKING_ID, 0, 1);
  devinfo.SetAbsMtSlot(ABS_MT_TOUCH_MAJOR, 0, 100);
  devinfo.SetAbsMtSlot(ABS_MT_POSITION_X, 0, 100);
  devinfo.SetAbsMtSlot(ABS_MT_POSITION_Y, 0, 100);
  devinfo.SetAbsMtSlot(ABS_MT_PRESSURE, 0, 128);

  // Initialize the device.
  dev->Initialize(devinfo);

  // Any report should suffice to dispatch the update.. do an empty one.
  struct input_event mock_kernel_queue_empty_report[] = {
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  dev->ConfigureReadMock(mock_kernel_queue_empty_report,
                         arraysize(mock_kernel_queue_empty_report), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  ui::TouchEventParams ev = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev.type);
  EXPECT_EQ(0, ev.slot);
  EXPECT_FLOAT_EQ(50.f, ev.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(50.f, ev.pointer_details.radius_y);
  EXPECT_FLOAT_EQ(0.50196081f, ev.pointer_details.force);
}

TEST_F(TouchEventConverterEvdevTest, ShouldReleaseContactsOnStop) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  // Captured from Chromebook Pixel (Link).
  timeval time;
  time = {1429651083, 686882};
  struct input_event mock_kernel_queue_press[] = {
      {time, EV_ABS, ABS_MT_TRACKING_ID, 0},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  dev->ConfigureReadMock(mock_kernel_queue_press,
                         arraysize(mock_kernel_queue_press), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  ui::TouchEventParams ev1 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1.type);
  EXPECT_EQ(0, ev1.slot);

  DestroyDevice();
  EXPECT_EQ(2u, size());

  ui::TouchEventParams ev2 = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2.type);
  EXPECT_EQ(0, ev2.slot);
}

TEST_F(TouchEventConverterEvdevTest, ShouldRemoveContactsWhenDisabled) {
  ui::MockTouchEventConverterEvdev* dev = device();

  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kLinkTouchscreen, &devinfo));

  // Captured from Chromebook Pixel (Link).
  timeval time;
  time = {1429651083, 686882};
  struct input_event mock_kernel_queue_press[] = {
      {time, EV_ABS, ABS_MT_TRACKING_ID, 0},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  // Initialize the device.
  dev->Initialize(devinfo);

  dev->ConfigureReadMock(mock_kernel_queue_press,
                         arraysize(mock_kernel_queue_press), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());

  ui::TouchEventParams ev1 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1.type);
  EXPECT_EQ(0, ev1.slot);
  EXPECT_EQ(1003, ev1.location.x());
  EXPECT_EQ(749, ev1.location.y());

  // Disable the device (should release the contact).
  dev->SetEnabled(false);
  EXPECT_EQ(2u, size());

  ui::TouchEventParams ev2 = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2.type);
  EXPECT_EQ(0, ev2.slot);

  // Set up the previous contact in slot 0.
  devinfo.SetAbsMtSlot(ABS_MT_TRACKING_ID, 0, 0);
  devinfo.SetAbsMtSlot(ABS_MT_TOUCH_MAJOR, 0, 116);
  devinfo.SetAbsMtSlot(ABS_MT_POSITION_X, 0, 1003);
  devinfo.SetAbsMtSlot(ABS_MT_POSITION_Y, 0, 749);
  devinfo.SetAbsMtSlot(ABS_MT_PRESSURE, 0, 50);

  // Re-enable the device (touch is cancelled, should not come back)
  dev->SimulateReinitialize(devinfo);
  dev->SetEnabled(true);
  EXPECT_EQ(2u, size());

  // Send updates to touch (touch is cancelled, should not come back)
  dev->ConfigureReadMock(mock_kernel_queue_press,
                         arraysize(mock_kernel_queue_press), 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());
}

TEST_F(TouchEventConverterEvdevTest, PalmShouldCancelTouch) {
  ui::MockTouchEventConverterEvdev* dev = device();

  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kLinkWithToolTypeTouchscreen, &devinfo));

  timeval time;
  time = {1429651083, 686882};
  int major_max = devinfo.GetAbsMaximum(ABS_MT_TOUCH_MAJOR);
  struct input_event mock_kernel_queue_max_major[] = {
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 0},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 1},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1103},
      {time, EV_ABS, ABS_MT_POSITION_Y, 649},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, major_max},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  struct input_event mock_kernel_queue_tool_palm[] = {
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 2},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 3},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1103},
      {time, EV_ABS, ABS_MT_POSITION_Y, 649},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_PALM},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  // Initialize the device.
  dev->Initialize(devinfo);

  dev->ConfigureReadMock(mock_kernel_queue_max_major,
                         arraysize(mock_kernel_queue_max_major), 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());

  ui::TouchEventParams ev1_1 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1_1.type);
  EXPECT_EQ(0, ev1_1.slot);
  EXPECT_EQ(1003, ev1_1.location.x());
  EXPECT_EQ(749, ev1_1.location.y());

  ui::TouchEventParams ev1_2 = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1_2.type);
  EXPECT_EQ(1, ev1_2.slot);
  EXPECT_EQ(1103, ev1_2.location.x());
  EXPECT_EQ(649, ev1_2.location.y());

  ui::TouchEventParams ev1_3 = dispatched_touch_event(2);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev1_3.type);
  EXPECT_EQ(0, ev1_3.slot);

  // We expect both touches to be cancelled even though
  // just one reported major at max value.
  ui::TouchEventParams ev1_4 = dispatched_touch_event(3);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev1_4.type);
  EXPECT_EQ(1, ev1_4.slot);

  dev->ConfigureReadMock(mock_kernel_queue_tool_palm,
                         arraysize(mock_kernel_queue_tool_palm), 0);
  dev->ReadNow();
  EXPECT_EQ(8u, size());

  ui::TouchEventParams ev2_1 = dispatched_touch_event(4);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev2_1.type);
  EXPECT_EQ(0, ev2_1.slot);
  EXPECT_EQ(1003, ev2_1.location.x());
  EXPECT_EQ(749, ev2_1.location.y());

  ui::TouchEventParams ev2_2 = dispatched_touch_event(5);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev2_2.type);
  EXPECT_EQ(1, ev2_2.slot);
  EXPECT_EQ(1103, ev2_2.location.x());
  EXPECT_EQ(649, ev2_2.location.y());

  ui::TouchEventParams ev2_3 = dispatched_touch_event(6);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2_3.type);
  EXPECT_EQ(0, ev2_3.slot);

  // We expect both touches to be cancelled even though
  // just one reported MT_TOOL_PALM.
  ui::TouchEventParams ev2_4 = dispatched_touch_event(7);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2_4.type);
  EXPECT_EQ(1, ev2_4.slot);
}

// crbug.com/773087
TEST_F(TouchEventConverterEvdevTest, TrackingIdShouldNotResetCancelByPalm) {
  ui::MockTouchEventConverterEvdev* dev = device();

  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kLinkWithToolTypeTouchscreen, &devinfo));

  timeval time;
  time = {1429651083, 686882};
  int major_max = devinfo.GetAbsMaximum(ABS_MT_TOUCH_MAJOR);
  struct input_event mock_kernel_queue_max_major[] = {
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 0},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 1},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1103},
      {time, EV_ABS, ABS_MT_POSITION_Y, 649},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, major_max},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  // No events will be generated from the following queue. Here is why:
  // The touch in SLOT 0 was identified as a palm in previous events, thus it
  // was cancelled. In the following events, there is new trcking id for slot
  // 0, but there is no MT_TOOL_FINGER and TOUCH_MAJOR saying it's a finger.
  // Thus the slot will continue to be a palm and all events will be cancelled.
  struct input_event mock_kernel_new_touch_without_new_major[] = {
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 3},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 4},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1103},
      {time, EV_ABS, ABS_MT_POSITION_Y, 649},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // This time, there is MT_TOOL_FINGER for slot 0, thus events should be
  // dispatched.
  struct input_event mock_kernel_queue_tool_palm[] = {
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 2},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1003},
      {time, EV_ABS, ABS_MT_POSITION_Y, 749},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_ABS, ABS_MT_SLOT, 1},
      {time, EV_ABS, ABS_MT_TRACKING_ID, 3},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_FINGER},
      {time, EV_ABS, ABS_MT_POSITION_X, 1103},
      {time, EV_ABS, ABS_MT_POSITION_Y, 649},
      {time, EV_ABS, ABS_MT_PRESSURE, 50},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 116},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1003},
      {time, EV_ABS, ABS_Y, 749},
      {time, EV_ABS, ABS_PRESSURE, 50},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_SLOT, 0},
      {time, EV_ABS, ABS_MT_TOOL_TYPE, MT_TOOL_PALM},
      {time, EV_SYN, SYN_REPORT, 0},
      {time, EV_ABS, ABS_MT_TRACKING_ID, -1},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  // Initialize the device.
  dev->Initialize(devinfo);

  dev->ConfigureReadMock(mock_kernel_queue_max_major,
                         arraysize(mock_kernel_queue_max_major), 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());

  ui::TouchEventParams ev1_1 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1_1.type);
  EXPECT_EQ(0, ev1_1.slot);
  EXPECT_EQ(1003, ev1_1.location.x());
  EXPECT_EQ(749, ev1_1.location.y());

  ui::TouchEventParams ev1_2 = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev1_2.type);
  EXPECT_EQ(1, ev1_2.slot);
  EXPECT_EQ(1103, ev1_2.location.x());
  EXPECT_EQ(649, ev1_2.location.y());

  ui::TouchEventParams ev1_3 = dispatched_touch_event(2);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev1_3.type);
  EXPECT_EQ(0, ev1_3.slot);

  // We expect both touches to be cancelled even though
  // just one reported major at max value.
  ui::TouchEventParams ev1_4 = dispatched_touch_event(3);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev1_4.type);
  EXPECT_EQ(1, ev1_4.slot);

  dev->ConfigureReadMock(mock_kernel_new_touch_without_new_major,
                         arraysize(mock_kernel_new_touch_without_new_major), 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());

  dev->ConfigureReadMock(mock_kernel_queue_tool_palm,
                         arraysize(mock_kernel_queue_tool_palm), 0);
  dev->ReadNow();
  EXPECT_EQ(8u, size());

  ui::TouchEventParams ev2_1 = dispatched_touch_event(4);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev2_1.type);
  EXPECT_EQ(0, ev2_1.slot);
  EXPECT_EQ(1003, ev2_1.location.x());
  EXPECT_EQ(749, ev2_1.location.y());

  ui::TouchEventParams ev2_2 = dispatched_touch_event(5);
  EXPECT_EQ(ET_TOUCH_PRESSED, ev2_2.type);
  EXPECT_EQ(1, ev2_2.slot);
  EXPECT_EQ(1103, ev2_2.location.x());
  EXPECT_EQ(649, ev2_2.location.y());

  ui::TouchEventParams ev2_3 = dispatched_touch_event(6);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2_3.type);
  EXPECT_EQ(0, ev2_3.slot);

  // We expect both touches to be cancelled even though
  // just one reported MT_TOOL_PALM.
  ui::TouchEventParams ev2_4 = dispatched_touch_event(7);
  EXPECT_EQ(ET_TOUCH_CANCELLED, ev2_4.type);
  EXPECT_EQ(1, ev2_4.slot);
}

// crbug.com/477695
TEST_F(TouchEventConverterEvdevTest, ShouldUseLeftButtonIfNoTouchButton) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitEloTouchscreen(dev);

  // Captured from Elo TouchSystems 2700.
  timeval time;
  time = {1433965490, 837958};
  struct input_event mock_kernel_queue_press[] = {
      {time, EV_ABS, ABS_X, 3654},
      {time, EV_ABS, ABS_Y, 1054},
      {time, EV_ABS, ABS_MISC, 18},
      {time, EV_SYN, SYN_REPORT, 0},

      {time, EV_MSC, MSC_SCAN, 90001},
      {time, EV_KEY, BTN_LEFT, 1},
      {time, EV_ABS, ABS_Y, 1055},
      {time, EV_ABS, ABS_MISC, 25},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  time = {1433965491, 1953};
  struct input_event mock_kernel_queue_move[] = {
      {time, EV_ABS, ABS_X, 3644},
      {time, EV_ABS, ABS_Y, 1059},
      {time, EV_ABS, ABS_MISC, 36},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  time = {1433965491, 225959};
  struct input_event mock_kernel_queue_release[] = {
      {time, EV_MSC, MSC_SCAN, 90001},
      {time, EV_KEY, BTN_LEFT, 0},
      {time, EV_ABS, ABS_MISC, 0},
      {time, EV_SYN, SYN_REPORT, 0},
  };

  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  // Press.
  dev->ConfigureReadMock(mock_kernel_queue_press,
                         arraysize(mock_kernel_queue_press), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());
  ui::TouchEventParams event = dispatched_touch_event(0);
  EXPECT_EQ(ui::ET_TOUCH_PRESSED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1433965490837958), event.timestamp);
  EXPECT_EQ(3654, event.location.x());
  EXPECT_EQ(1055, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.force);

  // Move.
  dev->ConfigureReadMock(mock_kernel_queue_move,
                         arraysize(mock_kernel_queue_move), 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());
  event = dispatched_touch_event(1);
  EXPECT_EQ(ui::ET_TOUCH_MOVED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1433965491001953), event.timestamp);
  EXPECT_EQ(3644, event.location.x());
  EXPECT_EQ(1059, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.force);

  // Release.
  dev->ConfigureReadMock(mock_kernel_queue_release,
                         arraysize(mock_kernel_queue_release), 0);
  dev->ReadNow();
  EXPECT_EQ(3u, size());
  event = dispatched_touch_event(2);
  EXPECT_EQ(ui::ET_TOUCH_RELEASED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1433965491225959), event.timestamp);
  EXPECT_EQ(3644, event.location.x());
  EXPECT_EQ(1059, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.f, event.pointer_details.force);

  // No dispatch on destruction.
  DestroyDevice();
  EXPECT_EQ(3u, size());
}

// crbug.com/407386
TEST_F(TouchEventConverterEvdevTest,
       DontChangeMultitouchPositionFromLegacyAxes) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  struct input_event mock_kernel_queue[] = {
      {{0, 0}, EV_ABS, ABS_MT_SLOT, 0},
      {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 100},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 999},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 888},
      {{0, 0}, EV_ABS, ABS_MT_PRESSURE, 55},
      {{0, 0}, EV_ABS, ABS_MT_SLOT, 1},
      {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 200},
      {{0, 0}, EV_ABS, ABS_MT_PRESSURE, 44},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 777},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 666},
      {{0, 0}, EV_ABS, ABS_X, 999},
      {{0, 0}, EV_ABS, ABS_Y, 888},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 55},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  // Check that two events are generated.
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();

  const unsigned int kExpectedEventCount = 2;
  EXPECT_EQ(kExpectedEventCount, size());
  if (kExpectedEventCount != size())
    return;

  ui::TouchEventParams ev0 = dispatched_touch_event(0);
  ui::TouchEventParams ev1 = dispatched_touch_event(1);

  EXPECT_EQ(0, ev0.slot);
  EXPECT_EQ(999, ev0.location.x());
  EXPECT_EQ(888, ev0.location.y());
  EXPECT_FLOAT_EQ(0.21568628f, ev0.pointer_details.force);

  EXPECT_EQ(1, ev1.slot);
  EXPECT_EQ(777, ev1.location.x());
  EXPECT_EQ(666, ev1.location.y());
  EXPECT_FLOAT_EQ(0.17254902f, ev1.pointer_details.force);
}

// crbug.com/446939
TEST_F(TouchEventConverterEvdevTest, CheckSlotLimit) {
  ui::MockTouchEventConverterEvdev* dev = device();

  InitPixelTouchscreen(dev);

  struct input_event mock_kernel_queue[] = {
      {{0, 0}, EV_ABS, ABS_MT_SLOT, 0},
      {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 100},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 999},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 888},
      {{0, 0}, EV_ABS, ABS_MT_SLOT, ui::kNumTouchEvdevSlots},
      {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 200},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 777},
      {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 666},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  // Check that one 1 event is generated
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());
}

namespace {

// TouchFilter which:
// - Considers all events of type |noise_event_type| as noise.
// - Keeps track of the events that it receives.
class EventTypeTouchNoiseFilter : public TouchFilter {
 public:
  explicit EventTypeTouchNoiseFilter(EventType noise_event_type)
      : noise_event_type_(noise_event_type) {}
  ~EventTypeTouchNoiseFilter() override {}

  // TouchFilter:
  void Filter(const std::vector<InProgressTouchEvdev>& touches,
              base::TimeTicks time,
              std::bitset<kNumTouchEvdevSlots>* slots_with_noise) override {
    for (const InProgressTouchEvdev& touch : touches) {
      EventType event_type = EventTypeFromTouch(touch);
      ++counts_[event_type];
      if (event_type == noise_event_type_)
        slots_with_noise->set(touch.slot);
    }
  }

  // Returns the number of received events of |type|.
  size_t num_events(EventType type) const {
    std::map<EventType, size_t>::const_iterator it = counts_.find(type);
    return it == counts_.end() ? 0u : it->second;
  }

 private:
  EventType EventTypeFromTouch(const InProgressTouchEvdev& touch) const {
    if (touch.touching)
      return touch.was_touching ? ET_TOUCH_MOVED : ET_TOUCH_PRESSED;
    return touch.was_touching ? ET_TOUCH_RELEASED : ET_UNKNOWN;
  }

  EventType noise_event_type_;
  std::map<EventType, size_t> counts_;

  DISALLOW_COPY_AND_ASSIGN(EventTypeTouchNoiseFilter);
};

}  // namespace

class TouchEventConverterEvdevTouchNoiseTest
    : public TouchEventConverterEvdevTest {
 public:
  TouchEventConverterEvdevTouchNoiseTest() {}
  ~TouchEventConverterEvdevTouchNoiseTest() override {}

  // Makes the FalseTouchFinder use |filter| and only |filter| to filter out
  // touch noise. Also removes the edge touch filter.
  void SetTouchNoiseFilter(std::unique_ptr<TouchFilter> filter) {
    FalseTouchFinder* finder = device()->false_touch_finder();
    finder->noise_filters_.clear();
    finder->noise_filters_.push_back(std::move(filter));
    finder->edge_touch_filter_.reset();
  }

  // Returns the first of FalseTouchFinder's filters.
  ui::TouchFilter* first_filter() {
    FalseTouchFinder* finder = device()->false_touch_finder();
    return finder->noise_filters_.empty() ?
        nullptr :
        finder->noise_filters_.begin()->get();
  }

  // TouchEventConverterEvdevTest:
  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kExtraTouchNoiseFiltering);
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEdgeTouchFiltering);
    TouchEventConverterEvdevTest::SetUp();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TouchEventConverterEvdevTouchNoiseTest);
};

// Test that if FalseTouchFinder identifies an event for an in-progress touch as
// noise, that the event is converted to ET_TOUCH_CANCELLED and that all
// subsequent events for the in-progress touch are cancelled.
TEST_F(TouchEventConverterEvdevTouchNoiseTest, TouchNoiseFiltering) {
  struct input_event mock_kernel_queue[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 684},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 41},
    {{0, 0}, EV_SYN, SYN_REPORT, 0},

    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 42},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 43},
    {{0, 0}, EV_SYN, SYN_REPORT, 0},

    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, -1},
    {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };

  MockTouchEventConverterEvdev* dev = device();
  SetTouchNoiseFilter(std::unique_ptr<TouchFilter>(
      new EventTypeTouchNoiseFilter(ET_TOUCH_PRESSED)));
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  ASSERT_EQ(0u, size());

  ClearDispatchedEvents();
  SetTouchNoiseFilter(std::unique_ptr<TouchFilter>(
      new EventTypeTouchNoiseFilter(ET_TOUCH_MOVED)));
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  ASSERT_EQ(2u, size());
  TouchEventParams event0 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, event0.type);
  EXPECT_EQ(40, event0.location.x());
  EXPECT_EQ(41, event0.location.y());
  EXPECT_EQ(ET_TOUCH_CANCELLED, dispatched_touch_event(1).type);

  ClearDispatchedEvents();
  SetTouchNoiseFilter(std::unique_ptr<TouchFilter>(
      new EventTypeTouchNoiseFilter(ET_TOUCH_RELEASED)));
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  ASSERT_EQ(3u, size());
  event0 = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, event0.type);
  EXPECT_EQ(40, event0.location.x());
  EXPECT_EQ(41, event0.location.y());
  TouchEventParams event1 = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_MOVED, event1.type);
  EXPECT_EQ(42, event1.location.x());
  EXPECT_EQ(43, event1.location.y());
  EXPECT_EQ(ET_TOUCH_CANCELLED, dispatched_touch_event(2).type);
}

// Test that TouchEventConverterEvdev keeps sending events to
// FalseTouchFinder after the touch is canceled.
TEST_F(TouchEventConverterEvdevTouchNoiseTest,
       DoNotSendTouchCancelsToFalseTouchFinder) {
  struct input_event mock_kernel_queue[] = {
    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, 684},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 40},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 41},
    {{0, 0}, EV_SYN, SYN_REPORT, 0},

    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 42},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 43},
    {{0, 0}, EV_SYN, SYN_REPORT, 0},

    {{0, 0}, EV_ABS, ABS_MT_POSITION_X, 43},
    {{0, 0}, EV_ABS, ABS_MT_POSITION_Y, 44},
    {{0, 0}, EV_SYN, SYN_REPORT, 0},

    {{0, 0}, EV_ABS, ABS_MT_TRACKING_ID, -1},
    {{0, 0}, EV_SYN, SYN_REPORT, 0}
  };

  MockTouchEventConverterEvdev* dev = device();
  SetTouchNoiseFilter(std::unique_ptr<TouchFilter>(
      new EventTypeTouchNoiseFilter(ET_TOUCH_PRESSED)));
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  ASSERT_EQ(0u, size());

  EventTypeTouchNoiseFilter* filter =
      static_cast<EventTypeTouchNoiseFilter*>(first_filter());
  EXPECT_EQ(1u, filter->num_events(ET_TOUCH_PRESSED));
  EXPECT_EQ(2u, filter->num_events(ET_TOUCH_MOVED));
  EXPECT_EQ(1u, filter->num_events(ET_TOUCH_RELEASED));
}

TEST_F(TouchEventConverterEvdevTest, ActiveStylusTouchAndRelease) {
  ui::MockTouchEventConverterEvdev* dev = device();
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kWilsonBeachActiveStylus, &devinfo));
  dev->Initialize(devinfo);

  struct input_event mock_kernel_queue[]{
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 1},
      {{0, 0}, EV_ABS, ABS_X, 9170},
      {{0, 0}, EV_ABS, ABS_Y, 3658},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOUCH, 1},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 60},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOUCH, 0},
      {{0, 0}, EV_ABS, ABS_X, 9173},
      {{0, 0}, EV_ABS, ABS_Y, 3906},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());

  auto down_event = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, down_event.type);
  EXPECT_EQ(9170, down_event.location.x());
  EXPECT_EQ(3658, down_event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            down_event.pointer_details.pointer_type);
  EXPECT_EQ(60.f / 1024, down_event.pointer_details.force);

  auto up_event = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_RELEASED, up_event.type);
  EXPECT_EQ(9173, up_event.location.x());
  EXPECT_EQ(3906, up_event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            up_event.pointer_details.pointer_type);
  EXPECT_EQ(0.f, up_event.pointer_details.force);
}

TEST_F(TouchEventConverterEvdevTest, ActiveStylusMotion) {
  ui::MockTouchEventConverterEvdev* dev = device();
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kWilsonBeachActiveStylus, &devinfo));
  dev->Initialize(devinfo);

  struct input_event mock_kernel_queue[]{
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 1},
      {{0, 0}, EV_ABS, ABS_X, 8921},
      {{0, 0}, EV_ABS, ABS_Y, 1072},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOUCH, 1},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 35},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_ABS, ABS_X, 8934},
      {{0, 0}, EV_ABS, ABS_Y, 981},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 184},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_ABS, ABS_X, 8930},
      {{0, 0}, EV_ABS, ABS_Y, 980},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 348},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOUCH, 0},
      {{0, 0}, EV_ABS, ABS_PRESSURE, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());

  ui::TouchEventParams event = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, event.type);
  EXPECT_EQ(8921, event.location.x());
  EXPECT_EQ(1072, event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            event.pointer_details.pointer_type);
  EXPECT_EQ(35.f / 1024, event.pointer_details.force);

  event = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_MOVED, event.type);
  EXPECT_EQ(8934, event.location.x());
  EXPECT_EQ(981, event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            event.pointer_details.pointer_type);
  EXPECT_EQ(184.f / 1024, event.pointer_details.force);

  event = dispatched_touch_event(2);
  EXPECT_EQ(ET_TOUCH_MOVED, event.type);
  EXPECT_EQ(8930, event.location.x());
  EXPECT_EQ(980, event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            event.pointer_details.pointer_type);
  EXPECT_EQ(348.f / 1024, event.pointer_details.force);

  event = dispatched_touch_event(3);
  EXPECT_EQ(ET_TOUCH_RELEASED, event.type);
  EXPECT_EQ(8930, event.location.x());
  EXPECT_EQ(980, event.location.y());
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            event.pointer_details.pointer_type);
  EXPECT_EQ(0.f / 1024, event.pointer_details.force);
}

TEST_F(TouchEventConverterEvdevTest, ActiveStylusBarrelButtonWhileHovering) {
  ui::MockTouchEventConverterEvdev* dev = device();
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kEveStylus, &devinfo));
  dev->Initialize(devinfo);

  struct input_event mock_kernel_queue[]{
      // Hover
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Button pressed
      {{0, 0}, EV_KEY, BTN_STYLUS, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Touching down
      {{0, 0}, EV_KEY, BTN_TOUCH, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Releasing touch
      {{0, 0}, EV_KEY, BTN_TOUCH, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Releasing button
      {{0, 0}, EV_KEY, BTN_STYLUS, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Leaving hover
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(2u, size());

  auto down_event = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, down_event.type);
  EXPECT_TRUE(down_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            down_event.pointer_details.pointer_type);

  auto up_event = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_RELEASED, up_event.type);
  EXPECT_TRUE(down_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            up_event.pointer_details.pointer_type);
}

TEST_F(TouchEventConverterEvdevTest, ActiveStylusBarrelButton) {
  ui::MockTouchEventConverterEvdev* dev = device();
  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kEveStylus, &devinfo));
  dev->Initialize(devinfo);

  struct input_event mock_kernel_queue[]{
      // Hover
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Touching down
      {{0, 0}, EV_KEY, BTN_TOUCH, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Button pressed
      {{0, 0}, EV_KEY, BTN_STYLUS, 1},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Releasing button
      {{0, 0}, EV_KEY, BTN_STYLUS, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Releasing touch
      {{0, 0}, EV_KEY, BTN_TOUCH, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},

      // Leaving hover
      {{0, 0}, EV_KEY, BTN_TOOL_PEN, 0},
      {{0, 0}, EV_SYN, SYN_REPORT, 0},
  };

  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(4u, size());

  auto down_event = dispatched_touch_event(0);
  EXPECT_EQ(ET_TOUCH_PRESSED, down_event.type);
  EXPECT_FALSE(down_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            down_event.pointer_details.pointer_type);

  auto button_down_event = dispatched_touch_event(1);
  EXPECT_EQ(ET_TOUCH_MOVED, button_down_event.type);
  EXPECT_TRUE(button_down_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            button_down_event.pointer_details.pointer_type);

  auto button_up_event = dispatched_touch_event(2);
  EXPECT_EQ(ET_TOUCH_MOVED, button_up_event.type);
  EXPECT_FALSE(button_up_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            button_up_event.pointer_details.pointer_type);

  auto up_event = dispatched_touch_event(3);
  EXPECT_EQ(ET_TOUCH_RELEASED, up_event.type);
  EXPECT_FALSE(down_event.flags & ui::EventFlags::EF_LEFT_MOUSE_BUTTON);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_PEN,
            up_event.pointer_details.pointer_type);
}

// crbug.com/771374
TEST_F(TouchEventConverterEvdevTest, FingerSizeWithResolution) {
  ui::MockTouchEventConverterEvdev* dev = device();

  EventDeviceInfo devinfo;
  EXPECT_TRUE(CapabilitiesToDeviceInfo(kEveTouchScreen, &devinfo));
  dev->Initialize(devinfo);

  // Captured from Pixelbook (Eve).
  timeval time;
  time = {1507226211, 483601};
  struct input_event mock_kernel_queue[] = {
      {time, EV_ABS, ABS_MT_TRACKING_ID, 461},
      {time, EV_ABS, ABS_MT_POSITION_X, 1795},
      {time, EV_ABS, ABS_MT_POSITION_Y, 5559},
      {time, EV_ABS, ABS_MT_PRESSURE, 217},
      {time, EV_ABS, ABS_MT_TOUCH_MAJOR, 14},
      {time, EV_ABS, ABS_MT_TOUCH_MINOR, 14},
      {time, EV_KEY, BTN_TOUCH, 1},
      {time, EV_ABS, ABS_X, 1795},
      {time, EV_ABS, ABS_Y, 5559},
      {time, EV_ABS, ABS_PRESSURE, 217},
      {time, EV_MSC, MSC_TIMESTAMP, 0},
      {time, EV_SYN, SYN_REPORT, 0},
  };
  // Set test now time to ensure above timestamps are in the past.
  SetTestNowTime(time);

  // Finger pressed with major/minor reported.
  dev->ConfigureReadMock(mock_kernel_queue, arraysize(mock_kernel_queue), 0);
  dev->ReadNow();
  EXPECT_EQ(1u, size());
  ui::TouchEventParams event = dispatched_touch_event(0);
  EXPECT_EQ(ui::ET_TOUCH_PRESSED, event.type);
  EXPECT_EQ(ToTestTimeTicks(1507226211483601), event.timestamp);
  EXPECT_EQ(1795, event.location.x());
  EXPECT_EQ(5559, event.location.y());
  EXPECT_EQ(0, event.slot);
  EXPECT_EQ(EventPointerType::POINTER_TYPE_TOUCH,
            event.pointer_details.pointer_type);
  EXPECT_FLOAT_EQ(280.f, event.pointer_details.radius_x);
  EXPECT_FLOAT_EQ(0.8509804f, event.pointer_details.force);
}
}  // namespace ui
