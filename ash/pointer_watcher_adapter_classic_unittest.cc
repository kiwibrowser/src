// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/test/ash_test_base.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget.h"

namespace ash {

using PointerWatcherAdapterClassicTest = AshTestBase;

enum TestPointerCaptureEvents {
  NONE = 0x01,
  CAPTURE = 0x02,
  WHEEL = 0x04,
  PRESS_OR_RELEASE = 0x08,
  MOVE = 0x10,
  DRAG = 0x20
};

// Records calls to OnPointerEventObserved() in |mouse_wheel_event_count| for a
// mouse wheel event, in |capture_changed_count_| for a mouse capture change
// event and in |pointer_event_count_| for all other pointer events.
class TestPointerWatcher : public views::PointerWatcher {
 public:
  explicit TestPointerWatcher(views::PointerWatcherEventTypes events) {
    ShellPort::Get()->AddPointerWatcher(this, events);
  }
  ~TestPointerWatcher() override {
    ShellPort::Get()->RemovePointerWatcher(this);
  }

  void ClearCounts() {
    pointer_event_count_ = capture_changed_count_ = mouse_wheel_event_count_ =
        move_changed_count_ = drag_changed_count_ = 0;
  }

  int pointer_event_count() const { return pointer_event_count_; }
  int capture_changed_count() const { return capture_changed_count_; }
  int mouse_wheel_event_count() const { return mouse_wheel_event_count_; }
  int move_changed_count() const { return move_changed_count_; }
  int drag_changed_count() const { return drag_changed_count_; }

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override {
    if (event.type() == ui::ET_POINTER_WHEEL_CHANGED) {
      mouse_wheel_event_count_++;
    } else if (event.type() == ui::ET_POINTER_CAPTURE_CHANGED) {
      capture_changed_count_++;
    } else if (event.type() == ui::ET_POINTER_MOVED) {
      // Pointer moved events are drags if they are a touch event.
      if (event.IsTouchPointerEvent()) {
        drag_changed_count_++;
      } else if (event.IsMousePointerEvent()) {
        // Differentiate between a drag and move event.
        if (event.flags() &
            (ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON |
             ui::EF_RIGHT_MOUSE_BUTTON)) {
          drag_changed_count_++;
        } else {
          move_changed_count_++;
        }
      } else {
        move_changed_count_++;
      }
    } else {
      pointer_event_count_++;
    }
  }

 private:
  int pointer_event_count_ = 0;
  int capture_changed_count_ = 0;
  int mouse_wheel_event_count_ = 0;
  int move_changed_count_ = 0;
  int drag_changed_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestPointerWatcher);
};

// Creates three TestPointerWatchers, one that wants moves and one that wants
// drags, and one that does not want either.
class TestHelper {
 public:
  TestHelper()
      : basic_watcher_(views::PointerWatcherEventTypes::BASIC),
        move_watcher_(views::PointerWatcherEventTypes::MOVES),
        drag_watcher_(views::PointerWatcherEventTypes::DRAGS) {}
  ~TestHelper() = default;

  // Used to verify call counts. One ExpectCallCount call should be made after
  // each generated mouse events. |basic_events_bitmask| defines which events
  // the test basic watcher should receive and |move_events_bitamsk| defines
  // which events the test move watcher should receive and |drag_events_bitmask|
  // defines which events the test drag watcher should receive.
  void ExpectCallCount(int basic_events_bitmask,
                       int move_events_bitmask,
                       int drag_events_bitmask) {
    // Compare the expected events in the |basic_events_bitmask| with the actual
    // counts. Basic watcher should never have any move or drag counts.
    EXPECT_EQ(0, basic_watcher_.move_changed_count());
    EXPECT_EQ(0, basic_watcher_.drag_changed_count());
    EXPECT_EQ(basic_events_bitmask & PRESS_OR_RELEASE ? 1 : 0,
              basic_watcher_.pointer_event_count());
    EXPECT_EQ(basic_events_bitmask & CAPTURE ? 1 : 0,
              basic_watcher_.capture_changed_count());
    EXPECT_EQ(basic_events_bitmask & WHEEL ? 1 : 0,
              basic_watcher_.mouse_wheel_event_count());
    // Compare the expected events in the |move_events_bitmask| with the actual
    // counts. Move watcher should never have any drag counts.
    EXPECT_EQ(0, move_watcher_.drag_changed_count());
    EXPECT_EQ(move_events_bitmask & MOVE ? 1 : 0,
              move_watcher_.move_changed_count());
    EXPECT_EQ(move_events_bitmask & PRESS_OR_RELEASE ? 1 : 0,
              move_watcher_.pointer_event_count());
    EXPECT_EQ(move_events_bitmask & CAPTURE ? 1 : 0,
              move_watcher_.capture_changed_count());
    EXPECT_EQ(move_events_bitmask & WHEEL ? 1 : 0,
              move_watcher_.mouse_wheel_event_count());
    // Compare the expected events in the |drag_events_bitmask| with the actual
    // counts.
    EXPECT_EQ(drag_events_bitmask & MOVE ? 1 : 0,
              drag_watcher_.move_changed_count());
    EXPECT_EQ(drag_events_bitmask & DRAG ? 1 : 0,
              drag_watcher_.drag_changed_count());
    EXPECT_EQ(drag_events_bitmask & PRESS_OR_RELEASE ? 1 : 0,
              drag_watcher_.pointer_event_count());
    EXPECT_EQ(drag_events_bitmask & CAPTURE ? 1 : 0,
              drag_watcher_.capture_changed_count());
    EXPECT_EQ(drag_events_bitmask & WHEEL ? 1 : 0,
              drag_watcher_.mouse_wheel_event_count());

    basic_watcher_.ClearCounts();
    move_watcher_.ClearCounts();
    drag_watcher_.ClearCounts();
  }

 private:
  TestPointerWatcher basic_watcher_;
  TestPointerWatcher move_watcher_;
  TestPointerWatcher drag_watcher_;

  DISALLOW_COPY_AND_ASSIGN(TestHelper);
};

TEST_F(PointerWatcherAdapterClassicTest, MouseEvents) {
  // Not relevant for mash.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  TestHelper helper;

  // Move: only the move and drag PointerWatcher should get the event.
  GetEventGenerator().MoveMouseTo(gfx::Point(10, 10));
  helper.ExpectCallCount(NONE, MOVE, MOVE);

  // Press: all.
  GetEventGenerator().PressLeftButton();
  helper.ExpectCallCount(PRESS_OR_RELEASE, PRESS_OR_RELEASE, PRESS_OR_RELEASE);

  // Drag: only drag PointerWatcher should get the event.
  GetEventGenerator().MoveMouseTo(gfx::Point(20, 30));
  helper.ExpectCallCount(NONE, NONE, DRAG);

  // Release: all (aura generates a capture event here).
  GetEventGenerator().ReleaseLeftButton();
  helper.ExpectCallCount(CAPTURE | PRESS_OR_RELEASE, CAPTURE | PRESS_OR_RELEASE,
                         CAPTURE | PRESS_OR_RELEASE);

  // Exit: none.
  GetEventGenerator().SendMouseExit();
  helper.ExpectCallCount(NONE, NONE, NONE);

  // Enter: none.
  ui::MouseEvent enter_event(ui::ET_MOUSE_ENTERED, gfx::Point(), gfx::Point(),
                             ui::EventTimeForNow(), 0, 0);
  GetEventGenerator().Dispatch(&enter_event);
  helper.ExpectCallCount(NONE, NONE, NONE);

  // Wheel: all
  GetEventGenerator().MoveMouseWheel(10, 11);
  helper.ExpectCallCount(WHEEL, WHEEL, WHEEL);

  // Capture: all.
  ui::MouseEvent capture_event(ui::ET_MOUSE_CAPTURE_CHANGED, gfx::Point(),
                               gfx::Point(), ui::EventTimeForNow(), 0, 0);
  GetEventGenerator().Dispatch(&capture_event);
  helper.ExpectCallCount(CAPTURE, CAPTURE, CAPTURE);
}

TEST_F(PointerWatcherAdapterClassicTest, TouchEvents) {
  // Not relevant for mash.
  if (Shell::GetAshConfig() == Config::MASH)
    return;

  TestHelper helper;

  // Press: all.
  const int touch_id = 11;
  GetEventGenerator().PressTouchId(touch_id);
  helper.ExpectCallCount(PRESS_OR_RELEASE, PRESS_OR_RELEASE, PRESS_OR_RELEASE);

  // Drag: only drag.
  GetEventGenerator().MoveTouchId(gfx::Point(20, 30), touch_id);
  helper.ExpectCallCount(NONE, NONE, DRAG);

  // Release: both (contrary to mouse above, touch does not implicitly generate
  // capture).
  GetEventGenerator().ReleaseTouchId(touch_id);
  helper.ExpectCallCount(PRESS_OR_RELEASE, PRESS_OR_RELEASE, PRESS_OR_RELEASE);
}

}  // namespace ash
