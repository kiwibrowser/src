// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_window.h"

#include <wayland-server-core.h>
#include <xdg-shell-unstable-v5-server-protocol.h>
#include <xdg-shell-unstable-v6-server-protocol.h>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/ozone/platform/wayland/fake_server.h"
#include "ui/ozone/platform/wayland/wayland_test.h"
#include "ui/ozone/test/mock_platform_window_delegate.h"

using ::testing::Eq;
using ::testing::Mock;
using ::testing::SaveArg;
using ::testing::StrEq;
using ::testing::_;

namespace ui {

class WaylandWindowTest : public WaylandTest {
 public:
  WaylandWindowTest()
      : test_mouse_event_(ET_MOUSE_PRESSED,
                          gfx::Point(10, 15),
                          gfx::Point(10, 15),
                          ui::EventTimeStampFromSeconds(123456),
                          EF_LEFT_MOUSE_BUTTON | EF_RIGHT_MOUSE_BUTTON,
                          EF_LEFT_MOUSE_BUTTON) {}

  void SetUp() override {
    WaylandTest::SetUp();

    xdg_surface_ = surface_->xdg_surface();
    ASSERT_TRUE(xdg_surface_);
  }

 protected:
  void SendConfigureEvent(int width,
                          int height,
                          uint32_t serial,
                          struct wl_array* states) {
    if (!xdg_surface_->xdg_toplevel()) {
      xdg_surface_send_configure(xdg_surface_->resource(), width, height,
                                 states, serial);
      return;
    }

    // In xdg_shell_v6, both surfaces send serial configure event and toplevel
    // surfaces send other data like states, heights and widths.
    zxdg_surface_v6_send_configure(xdg_surface_->resource(), serial);
    ASSERT_TRUE(xdg_surface_->xdg_toplevel());
    zxdg_toplevel_v6_send_configure(xdg_surface_->xdg_toplevel()->resource(),
                                    width, height, states);
  }

  // Depending on a shell version, xdg_surface_ or xdg_toplevel surface should
  // get the mock calls. This method decided, which surface to use.
  wl::MockXdgSurface* GetXdgSurface() {
    if (GetParam() == kXdgShellV5)
      return xdg_surface_;
    return xdg_surface_->xdg_toplevel();
  }

  void SetWlArrayWithState(uint32_t state, wl_array* states) {
    uint32_t* s;
    s = static_cast<uint32_t*>(wl_array_add(states, sizeof *s));
    *s = state;
  }

  void InitializeWlArrayWithActivatedState(wl_array* states) {
    wl_array_init(states);
    SetWlArrayWithState(XDG_SURFACE_STATE_ACTIVATED, states);
  }

  wl::MockXdgSurface* xdg_surface_;

  MouseEvent test_mouse_event_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WaylandWindowTest);
};

TEST_P(WaylandWindowTest, SetTitle) {
  EXPECT_CALL(*GetXdgSurface(), SetTitle(StrEq("hello")));
  window_->SetTitle(base::ASCIIToUTF16("hello"));
}

TEST_P(WaylandWindowTest, MaximizeAndRestore) {
  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_MAXIMIZED)));
  SetWlArrayWithState(XDG_SURFACE_STATE_MAXIMIZED, &states);

  EXPECT_CALL(*GetXdgSurface(), SetMaximized());
  window_->Maximize();
  SendConfigureEvent(0, 0, 1, &states);
  Sync();

  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_NORMAL)));
  EXPECT_CALL(*GetXdgSurface(), UnsetMaximized());
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();
}

TEST_P(WaylandWindowTest, Minimize) {
  wl_array states;
  wl_array_init(&states);

  // Initialize to normal first.
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_NORMAL)));
  SendConfigureEvent(0, 0, 1, &states);
  Sync();

  EXPECT_CALL(*GetXdgSurface(), SetMinimized());
  // Wayland compositor doesn't notify clients about minimized state, but rather
  // if a window is not activated. Thus, a WaylandWindow marks itself as being
  // minimized and as soon as a configuration event with not activated state
  // comes, its state is changed to minimized. This EXPECT_CALL ensures this
  // behaviour.
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_MINIMIZED)));
  window_->Minimize();
  // Reinitialize wl_array, which removes previous old states.
  wl_array_init(&states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();

  // Send one additional empty configuration event (which means the surface is
  // not maximized, fullscreen or activated) to ensure, WaylandWindow stays in
  // the same minimized state and doesn't notify its delegate.
  EXPECT_CALL(delegate_, OnWindowStateChanged(_)).Times(0);
  SendConfigureEvent(0, 0, 3, &states);
  Sync();

  // And one last time to ensure the behaviour.
  SendConfigureEvent(0, 0, 4, &states);
  Sync();
}

TEST_P(WaylandWindowTest, SetFullscreenAndRestore) {
  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  SetWlArrayWithState(XDG_SURFACE_STATE_FULLSCREEN, &states);

  EXPECT_CALL(*GetXdgSurface(), SetFullscreen());
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_FULLSCREEN)));
  window_->ToggleFullscreen();
  SendConfigureEvent(0, 0, 1, &states);
  Sync();

  EXPECT_CALL(*GetXdgSurface(), UnsetFullscreen());
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_NORMAL)));
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();
}

TEST_P(WaylandWindowTest, SetMaximizedFullscreenAndRestore) {
  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  EXPECT_CALL(*GetXdgSurface(), SetMaximized());
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_MAXIMIZED)));
  window_->Maximize();
  SetWlArrayWithState(XDG_SURFACE_STATE_MAXIMIZED, &states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();

  EXPECT_CALL(*GetXdgSurface(), SetFullscreen());
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_FULLSCREEN)));
  window_->ToggleFullscreen();
  SetWlArrayWithState(XDG_SURFACE_STATE_FULLSCREEN, &states);
  SendConfigureEvent(0, 0, 3, &states);
  Sync();

  EXPECT_CALL(*GetXdgSurface(), UnsetFullscreen());
  EXPECT_CALL(*GetXdgSurface(), UnsetMaximized());
  EXPECT_CALL(delegate_,
              OnWindowStateChanged(Eq(PLATFORM_WINDOW_STATE_NORMAL)));
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 4, &states);
  Sync();
}

TEST_P(WaylandWindowTest, RestoreBoundsAfterMaximize) {
  const gfx::Rect current_bounds = window_->GetBounds();

  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  const gfx::Rect maximized_bounds = gfx::Rect(0, 0, 1024, 768);
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(maximized_bounds)));
  window_->Maximize();
  SetWlArrayWithState(XDG_SURFACE_STATE_MAXIMIZED, &states);
  SendConfigureEvent(maximized_bounds.width(), maximized_bounds.height(), 1,
                     &states);
  Sync();

  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(current_bounds)));
  // Both in XdgV5 and XdgV6, surfaces implement SetWindowGeometry method.
  // Thus, using a toplevel object in XdgV6 case is not right thing. Use a
  // surface here instead.
  EXPECT_CALL(*xdg_surface_, SetWindowGeometry(0, 0, current_bounds.width(),
                                               current_bounds.height()));
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();
}

TEST_P(WaylandWindowTest, RestoreBoundsAfterFullscreen) {
  const gfx::Rect current_bounds = window_->GetBounds();

  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  const gfx::Rect fullscreen_bounds = gfx::Rect(0, 0, 1280, 720);
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(fullscreen_bounds)));
  window_->ToggleFullscreen();
  SetWlArrayWithState(XDG_SURFACE_STATE_FULLSCREEN, &states);
  SendConfigureEvent(fullscreen_bounds.width(), fullscreen_bounds.height(), 1,
                     &states);
  Sync();

  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(current_bounds)));
  // Both in XdgV5 and XdgV6, surfaces implement SetWindowGeometry method.
  // Thus, using a toplevel object in XdgV6 case is not right thing. Use a
  // surface here instead.
  EXPECT_CALL(*xdg_surface_, SetWindowGeometry(0, 0, current_bounds.width(),
                                               current_bounds.height()));
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();
}

TEST_P(WaylandWindowTest, RestoreBoundsAfterMaximizeAndFullscreen) {
  const gfx::Rect current_bounds = window_->GetBounds();

  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  const gfx::Rect maximized_bounds = gfx::Rect(0, 0, 1024, 768);
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(maximized_bounds)));
  window_->Maximize();
  SetWlArrayWithState(XDG_SURFACE_STATE_MAXIMIZED, &states);
  SendConfigureEvent(maximized_bounds.width(), maximized_bounds.height(), 1,
                     &states);
  Sync();

  const gfx::Rect fullscreen_bounds = gfx::Rect(0, 0, 1280, 720);
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(fullscreen_bounds)));
  window_->ToggleFullscreen();
  SetWlArrayWithState(XDG_SURFACE_STATE_FULLSCREEN, &states);
  SendConfigureEvent(fullscreen_bounds.width(), fullscreen_bounds.height(), 2,
                     &states);
  Sync();

  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(maximized_bounds)));
  window_->Maximize();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SetWlArrayWithState(XDG_SURFACE_STATE_MAXIMIZED, &states);
  SendConfigureEvent(maximized_bounds.width(), maximized_bounds.height(), 3,
                     &states);
  Sync();

  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(current_bounds)));
  // Both in XdgV5 and XdgV6, surfaces implement SetWindowGeometry method.
  // Thus, using a toplevel object in XdgV6 case is not right thing. Use a
  // surface here instead.
  EXPECT_CALL(*xdg_surface_, SetWindowGeometry(0, 0, current_bounds.width(),
                                               current_bounds.height()));
  window_->Restore();
  // Reinitialize wl_array, which removes previous old states.
  InitializeWlArrayWithActivatedState(&states);
  SendConfigureEvent(0, 0, 4, &states);
  Sync();
}

TEST_P(WaylandWindowTest, SendsBoundsOnRequest) {
  const gfx::Rect initial_bounds = window_->GetBounds();

  const gfx::Rect new_bounds = gfx::Rect(0, 0, initial_bounds.width() + 10,
                                         initial_bounds.height() + 10);
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(new_bounds)));
  window_->SetBounds(new_bounds);

  wl_array states;
  InitializeWlArrayWithActivatedState(&states);

  // First case is when Wayland sends a configure event with 0,0 height and
  // widht.
  EXPECT_CALL(*xdg_surface_,
              SetWindowGeometry(0, 0, new_bounds.width(), new_bounds.height()))
      .Times(2);
  SendConfigureEvent(0, 0, 2, &states);
  Sync();

  // Second case is when Wayland sends a configure event with 1, 1 height and
  // width. It looks more like a bug in Gnome Shell with Wayland as long as the
  // documentation says it must be set to 0, 0, when wayland requests bounds.
  SendConfigureEvent(0, 0, 3, &states);
  Sync();
}

TEST_P(WaylandWindowTest, CanDispatchMouseEventDefault) {
  EXPECT_FALSE(window_->CanDispatchEvent(&test_mouse_event_));
}

TEST_P(WaylandWindowTest, CanDispatchMouseEventFocus) {
  // set_pointer_focus(true) requires a WaylandPointer.
  wl_seat_send_capabilities(server_.seat()->resource(),
                            WL_SEAT_CAPABILITY_POINTER);
  Sync();
  ASSERT_TRUE(connection_->pointer());
  window_->set_pointer_focus(true);
  EXPECT_TRUE(window_->CanDispatchEvent(&test_mouse_event_));
}

TEST_P(WaylandWindowTest, CanDispatchMouseEventUnfocus) {
  EXPECT_FALSE(window_->has_pointer_focus());
  EXPECT_FALSE(window_->CanDispatchEvent(&test_mouse_event_));
}

ACTION_P(CloneEvent, ptr) {
  *ptr = Event::Clone(*arg0);
}

TEST_P(WaylandWindowTest, DispatchEvent) {
  std::unique_ptr<Event> event;
  EXPECT_CALL(delegate_, DispatchEvent(_)).WillOnce(CloneEvent(&event));
  window_->DispatchEvent(&test_mouse_event_);
  ASSERT_TRUE(event);
  ASSERT_TRUE(event->IsMouseEvent());
  auto* mouse_event = event->AsMouseEvent();
  EXPECT_EQ(mouse_event->location_f(), test_mouse_event_.location_f());
  EXPECT_EQ(mouse_event->root_location_f(),
            test_mouse_event_.root_location_f());
  EXPECT_EQ(mouse_event->time_stamp(), test_mouse_event_.time_stamp());
  EXPECT_EQ(mouse_event->button_flags(), test_mouse_event_.button_flags());
  EXPECT_EQ(mouse_event->changed_button_flags(),
            test_mouse_event_.changed_button_flags());
}

TEST_P(WaylandWindowTest, HasCaptureUpdatedOnPointerEvents) {
  wl_seat_send_capabilities(server_.seat()->resource(),
                            WL_SEAT_CAPABILITY_POINTER);

  Sync();

  wl::MockPointer* pointer = server_.seat()->pointer();
  ASSERT_TRUE(pointer);

  wl_pointer_send_enter(pointer->resource(), 1, surface_->resource(), 0, 0);
  Sync();
  EXPECT_FALSE(window_->HasCapture());

  wl_pointer_send_button(pointer->resource(), 2, 1002, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_PRESSED);
  Sync();
  EXPECT_TRUE(window_->HasCapture());

  wl_pointer_send_motion(pointer->resource(), 1003, wl_fixed_from_int(400),
                         wl_fixed_from_int(500));
  Sync();
  EXPECT_TRUE(window_->HasCapture());

  wl_pointer_send_button(pointer->resource(), 4, 1004, BTN_LEFT,
                         WL_POINTER_BUTTON_STATE_RELEASED);
  Sync();
  EXPECT_FALSE(window_->HasCapture());
}

TEST_P(WaylandWindowTest, ConfigureEvent) {
  wl_array states;
  wl_array_init(&states);
  SendConfigureEvent(1000, 1000, 12, &states);
  SendConfigureEvent(1500, 1000, 13, &states);

  // Make sure that the implementation does not call OnBoundsChanged for each
  // configure event if it receives multiple in a row.
  EXPECT_CALL(delegate_, OnBoundsChanged(Eq(gfx::Rect(0, 0, 1500, 1000))));
  // Responding to a configure event, the window geometry in here must respect
  // the sizing negotiations specified by the configure event.
  // |xdg_surface_| must receive the following calls in both xdg_shell_v5 and
  // xdg_shell_v6. Other calls like SetTitle or SetMaximized are recieved by
  // xdg_toplevel in xdg_shell_v6 and by xdg_surface_ in xdg_shell_v5.
  EXPECT_CALL(*xdg_surface_, SetWindowGeometry(0, 0, 1500, 1000)).Times(1);
  EXPECT_CALL(*xdg_surface_, AckConfigure(13));
}

TEST_P(WaylandWindowTest, ConfigureEventWithNulledSize) {
  wl_array states;
  wl_array_init(&states);

  // If Wayland sends configure event with 0 width and 0 size, client should
  // call back with desired sizes. In this case, that's the actual size of
  // the window.
  SendConfigureEvent(0, 0, 14, &states);
  // |xdg_surface_| must receive the following calls in both xdg_shell_v5 and
  // xdg_shell_v6. Other calls like SetTitle or SetMaximized are recieved by
  // xdg_toplevel in xdg_shell_v6 and by xdg_surface_ in xdg_shell_v5.
  EXPECT_CALL(*xdg_surface_, SetWindowGeometry(0, 0, 800, 600));
  EXPECT_CALL(*xdg_surface_, AckConfigure(14));
}

TEST_P(WaylandWindowTest, OnActivationChanged) {
  EXPECT_FALSE(window_->is_active());

  {
    wl_array states;
    InitializeWlArrayWithActivatedState(&states);
    EXPECT_CALL(delegate_, OnActivationChanged(Eq(true)));
    SendConfigureEvent(0, 0, 1, &states);
    Sync();
    EXPECT_TRUE(window_->is_active());
  }

  wl_array states;
  wl_array_init(&states);
  EXPECT_CALL(delegate_, OnActivationChanged(Eq(false)));
  SendConfigureEvent(0, 0, 2, &states);
  Sync();
  EXPECT_FALSE(window_->is_active());
}

TEST_P(WaylandWindowTest, OnAcceleratedWidgetDestroy) {
  EXPECT_CALL(delegate_, OnAcceleratedWidgetDestroying()).Times(1);
  EXPECT_CALL(delegate_, OnAcceleratedWidgetDestroyed()).Times(1);
  window_.reset();
}

INSTANTIATE_TEST_CASE_P(XdgVersionV5Test,
                        WaylandWindowTest,
                        ::testing::Values(kXdgShellV5));
INSTANTIATE_TEST_CASE_P(XdgVersionV6Test,
                        WaylandWindowTest,
                        ::testing::Values(kXdgShellV6));

}  // namespace ui
