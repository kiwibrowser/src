// Copyright 2016 The Chromium Authors. All rights reserved.
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

class WaylandPointerTest : public WaylandTest {
 public:
  WaylandPointerTest() {}

  void SetUp() override {
    WaylandTest::SetUp();

    wl_seat_send_capabilities(server_.seat()->resource(),
                              WL_SEAT_CAPABILITY_POINTER);

    Sync();

    pointer_ = server_.seat()->pointer();
    ASSERT_TRUE(pointer_);
  }

 protected:
  wl::MockPointer* pointer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaylandPointerTest);
};

TEST_P(WaylandPointerTest, Leave) {
  MockPlatformWindowDelegate other_delegate;
  WaylandWindow other_window(&other_delegate, connection_.get(),
                             gfx::Rect(0, 0, 10, 10));
  gfx::AcceleratedWidget other_widget = gfx::kNullAcceleratedWidget;
  EXPECT_CALL(other_delegate, OnAcceleratedWidgetAvailable(_, _))
      .WillOnce(SaveArg<0>(&other_widget));
  ASSERT_TRUE(other_window.Initialize());
  ASSERT_NE(other_widget, gfx::kNullAcceleratedWidget);

  Sync();

  wl::MockSurface* other_surface =
      server_.GetObject<wl::MockSurface>(other_widget);
  ASSERT_TRUE(other_surface);

  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(), 0, 0);
  wl_pointer_send_leave(pointer_->resource(), 2, surface_->resource());
  wl_pointer_send_enter(pointer_->resource(), 3, other_surface->resource(), 0,
                        0);
  wl_pointer_send_button(pointer_->resource(), 4, 1004, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_PRESSED);
  EXPECT_CALL(delegate_, DispatchEvent(_)).Times(1);

  // Do an extra Sync() here so that we process the second enter event before we
  // destroy |other_window|.
  Sync();
}

ACTION_P(CloneEvent, ptr) {
  *ptr = Event::Clone(*arg0);
}

ACTION_P3(CloneEventAndCheckCapture, window, result, ptr) {
  ASSERT_TRUE(window->HasCapture() == result);
  *ptr = Event::Clone(*arg0);
}

TEST_P(WaylandPointerTest, Motion) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(), 0, 0);
  wl_pointer_send_motion(pointer_->resource(), 1002,
                         wl_fixed_from_double(10.75),
                         wl_fixed_from_double(20.375));

  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillOnce(CloneEvent(&event));

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseEvent());
  auto* mouse_event = event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_MOVED, mouse_event->type());
  EXPECT_EQ(0, mouse_event->button_flags());
  EXPECT_EQ(0, mouse_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(10.75, 20.375), mouse_event->location_f());
  EXPECT_EQ(gfx::PointF(10.75, 20.375), mouse_event->root_location_f());
}

TEST_P(WaylandPointerTest, MotionDragged) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(), 0, 0);
  wl_pointer_send_button(pointer_->resource(), 2, 1002, BTN_MIDDLE,
                         WL_POINTER_BUTTON_STATE_PRESSED);

  Sync();

  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillOnce(CloneEvent(&event));
  wl_pointer_send_motion(pointer_->resource(), 1003, wl_fixed_from_int(400),
                         wl_fixed_from_int(500));

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseEvent());
  auto* mouse_event = event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_DRAGGED, mouse_event->type());
  EXPECT_EQ(EF_MIDDLE_MOUSE_BUTTON, mouse_event->button_flags());
  EXPECT_EQ(0, mouse_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(400, 500), mouse_event->location_f());
  EXPECT_EQ(gfx::PointF(400, 500), mouse_event->root_location_f());
}

TEST_P(WaylandPointerTest, ButtonPressAndCheckCapture) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(),
                        wl_fixed_from_int(200), wl_fixed_from_int(150));
  Sync();

  wl_pointer_send_button(pointer_->resource(), 2, 1002, BTN_RIGHT,
                         WL_POINTER_BUTTON_STATE_PRESSED);
  std::unique_ptr<Event> right_press_event;
  // By the time ET_MOUSE_PRESSED event comes, WaylandWindow must have capture
  // set.
  EXPECT_CALL(delegate_, DispatchEvent(_))
      .WillOnce(
          CloneEventAndCheckCapture(window_.get(), true, &right_press_event));

  Sync();
  ASSERT_TRUE(right_press_event);
  ASSERT_TRUE(right_press_event->IsMouseEvent());
  auto* right_press_mouse_event = right_press_event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_PRESSED, right_press_mouse_event->type());
  EXPECT_EQ(EF_RIGHT_MOUSE_BUTTON, right_press_mouse_event->button_flags());
  EXPECT_EQ(EF_RIGHT_MOUSE_BUTTON,
            right_press_mouse_event->changed_button_flags());

  std::unique_ptr<Event> left_press_event;
  // Ensure capture is still set before DispatchEvent returns.
  EXPECT_CALL(delegate_, DispatchEvent(_))
      .WillOnce(
          CloneEventAndCheckCapture(window_.get(), true, &left_press_event));
  wl_pointer_send_button(pointer_->resource(), 3, 1003, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_PRESSED);

  Sync();

  // Ensure capture is still set after DispatchEvent returns.
  ASSERT_TRUE(window_->HasCapture());

  ASSERT_TRUE(left_press_event);
  ASSERT_TRUE(left_press_event->IsMouseEvent());
  auto* left_press_mouse_event = left_press_event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_PRESSED, left_press_mouse_event->type());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON | EF_RIGHT_MOUSE_BUTTON,
            left_press_mouse_event->button_flags());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON,
            left_press_mouse_event->changed_button_flags());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON,
            left_press_mouse_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(200, 150), left_press_mouse_event->location_f());
  EXPECT_EQ(gfx::PointF(200, 150), left_press_mouse_event->root_location_f());
}

TEST_P(WaylandPointerTest, ButtonReleaseAndCheckCapture) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(),
                        wl_fixed_from_int(50), wl_fixed_from_int(50));
  wl_pointer_send_button(pointer_->resource(), 2, 1002, BTN_BACK,
                         WL_POINTER_BUTTON_STATE_PRESSED);
  wl_pointer_send_button(pointer_->resource(), 3, 1003, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_PRESSED);

  Sync();

  std::unique_ptr<Event> event;
  // Ensure capture is set before DispatchEvent returns.
  EXPECT_CALL(delegate_, DispatchEvent(_))
      .WillOnce(CloneEventAndCheckCapture(window_.get(), true, &event));
  wl_pointer_send_button(pointer_->resource(), 4, 1004, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_RELEASED);

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseEvent());
  auto* mouse_event = event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_RELEASED, mouse_event->type());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON | EF_BACK_MOUSE_BUTTON,
            mouse_event->button_flags());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON, mouse_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(50, 50), mouse_event->location_f());
  EXPECT_EQ(gfx::PointF(50, 50), mouse_event->root_location_f());

  // Ensure capture is still set after DispatchEvent returns.
  ASSERT_TRUE(window_->HasCapture());

  mouse_event = nullptr;
  event.reset();
  // Ensure capture has not been reset before DispatchEvent returns, otherwise
  // the code on top of Ozone (aura and etc), might get a wrong result, when
  // calling HasCapture. If it is false, it can lead to mouse pressed handlers
  // to be never released.
  EXPECT_CALL(delegate_, DispatchEvent(_))
      .WillOnce(CloneEventAndCheckCapture(window_.get(), true, &event));
  wl_pointer_send_button(pointer_->resource(), 5, 1005, BTN_BACK,
                         WL_POINTER_BUTTON_STATE_RELEASED);

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseEvent());
  mouse_event = event->AsMouseEvent();
  EXPECT_EQ(ET_MOUSE_RELEASED, mouse_event->type());
  EXPECT_EQ(EF_BACK_MOUSE_BUTTON, mouse_event->button_flags());
  EXPECT_EQ(EF_BACK_MOUSE_BUTTON, mouse_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(50, 50), mouse_event->location_f());
  EXPECT_EQ(gfx::PointF(50, 50), mouse_event->root_location_f());

  // It is safe to release capture now.
  ASSERT_TRUE(!window_->HasCapture());
}

TEST_P(WaylandPointerTest, AxisVertical) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(),
                        wl_fixed_from_int(0), wl_fixed_from_int(0));
  wl_pointer_send_button(pointer_->resource(), 2, 1002, BTN_RIGHT,
                         WL_POINTER_BUTTON_STATE_PRESSED);

  Sync();

  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillOnce(CloneEvent(&event));
  // Wayland servers typically send a value of 10 per mouse wheel click.
  wl_pointer_send_axis(pointer_->resource(), 1003,
                       WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_int(20));

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseWheelEvent());
  auto* mouse_wheel_event = event->AsMouseWheelEvent();
  EXPECT_EQ(gfx::Vector2d(0, -2 * MouseWheelEvent::kWheelDelta),
            mouse_wheel_event->offset());
  EXPECT_EQ(EF_RIGHT_MOUSE_BUTTON, mouse_wheel_event->button_flags());
  EXPECT_EQ(0, mouse_wheel_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(), mouse_wheel_event->location_f());
  EXPECT_EQ(gfx::PointF(), mouse_wheel_event->root_location_f());
}

TEST_P(WaylandPointerTest, AxisHorizontal) {
  wl_pointer_send_enter(pointer_->resource(), 1, surface_->resource(),
                        wl_fixed_from_int(50), wl_fixed_from_int(75));
  wl_pointer_send_button(pointer_->resource(), 2, 1002, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_PRESSED);

  Sync();

  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillOnce(CloneEvent(&event));
  // Wayland servers typically send a value of 10 per mouse wheel click.
  wl_pointer_send_axis(pointer_->resource(), 1003,
                       WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                       wl_fixed_from_int(10));

  Sync();

  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseWheelEvent());
  auto* mouse_wheel_event = event->AsMouseWheelEvent();
  EXPECT_EQ(gfx::Vector2d(MouseWheelEvent::kWheelDelta, 0),
            mouse_wheel_event->offset());
  EXPECT_EQ(EF_LEFT_MOUSE_BUTTON, mouse_wheel_event->button_flags());
  EXPECT_EQ(0, mouse_wheel_event->changed_button_flags());
  EXPECT_EQ(gfx::PointF(50, 75), mouse_wheel_event->location_f());
  EXPECT_EQ(gfx::PointF(50, 75), mouse_wheel_event->root_location_f());
}

INSTANTIATE_TEST_CASE_P(XdgVersionV5Test,
                        WaylandPointerTest,
                        ::testing::Values(kXdgShellV5));
INSTANTIATE_TEST_CASE_P(XdgVersionV6Test,
                        WaylandPointerTest,
                        ::testing::Values(kXdgShellV6));

}  // namespace ui
