// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/input.h>
#include <wayland-server.h>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/ozone/platform/wayland/fake_server.h"
#include "ui/ozone/platform/wayland/wayland_test.h"
#include "ui/ozone/platform/wayland/wayland_window.h"
#include "ui/ozone/test/mock_platform_window_delegate.h"

using ::testing::SaveArg;
using ::testing::_;

namespace ui {

namespace {

ACTION_P(CloneEvent, ptr) {
  *ptr = Event::Clone(*arg0);
}

}  // namespace

class WaylandTouchTest : public WaylandTest {
 public:
  WaylandTouchTest() {}

  void SetUp() override {
    WaylandTest::SetUp();

    wl_seat_send_capabilities(server_.seat()->resource(),
                              WL_SEAT_CAPABILITY_TOUCH);

    Sync();

    touch_ = server_.seat()->touch();
    ASSERT_TRUE(touch_);
  }

 protected:
  void CheckEventType(ui::EventType event_type, ui::Event* event) {
    ASSERT_TRUE(event);
    ASSERT_TRUE(event->IsTouchEvent());

    auto* key_event = event->AsTouchEvent();
    EXPECT_EQ(event_type, key_event->type());
  }

  wl::MockTouch* touch_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaylandTouchTest);
};

TEST_P(WaylandTouchTest, KeypressAndMotion) {
  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillRepeatedly(CloneEvent(&event));

  wl_touch_send_down(touch_->resource(), 1, 0, surface_->resource(), 0 /* id */,
                     wl_fixed_from_int(50), wl_fixed_from_int(100));

  Sync();
  CheckEventType(ui::ET_TOUCH_PRESSED, event.get());

  wl_touch_send_motion(touch_->resource(), 500, 0 /* id */,
                       wl_fixed_from_int(100), wl_fixed_from_int(100));

  Sync();
  CheckEventType(ui::ET_TOUCH_MOVED, event.get());

  wl_touch_send_up(touch_->resource(), 1, 1000, 0 /* id */);

  Sync();
  CheckEventType(ui::ET_TOUCH_RELEASED, event.get());
}

INSTANTIATE_TEST_CASE_P(XdgVersionV5Test,
                        WaylandTouchTest,
                        ::testing::Values(kXdgShellV5));
INSTANTIATE_TEST_CASE_P(XdgVersionV6Test,
                        WaylandTouchTest,
                        ::testing::Values(kXdgShellV6));

}  // namespace ui
