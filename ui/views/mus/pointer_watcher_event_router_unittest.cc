// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/mus/pointer_watcher_event_router.h"

#include <memory>

#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/events/event.h"
#include "ui/views/mus/mus_client.h"
#include "ui/views/mus/screen_mus.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/test/scoped_views_test_helper.h"

namespace views {
namespace {

class TestPointerWatcher : public PointerWatcher {
 public:
  TestPointerWatcher() {}
  ~TestPointerWatcher() override {}

  ui::PointerEvent* last_event_observed() { return last_event_observed_.get(); }
  gfx::Point last_location_in_screen() { return last_location_in_screen_; }

  void Reset() {
    last_event_observed_.reset();
    last_location_in_screen_ = gfx::Point();
  }

  // PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override {
    last_event_observed_ = std::make_unique<ui::PointerEvent>(event);
    last_location_in_screen_ = location_in_screen;
  }

 private:
  std::unique_ptr<ui::PointerEvent> last_event_observed_;
  gfx::Point last_location_in_screen_;

  DISALLOW_COPY_AND_ASSIGN(TestPointerWatcher);
};

}  // namespace

class PointerWatcherEventRouterTest : public testing::Test {
 public:
  PointerWatcherEventRouterTest() = default;
  ~PointerWatcherEventRouterTest() override = default;

  void OnPointerEventObserved(const ui::PointerEvent& event,
                              int64_t display_id = 0) {
    MusClient::Get()->pointer_watcher_event_router()->OnPointerEventObserved(
        event, display_id, nullptr);
  }

  PointerWatcherEventRouter::EventTypes event_types() const {
    return MusClient::Get()->pointer_watcher_event_router()->event_types_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::UI};
  ScopedViewsTestHelper helper_;

  DISALLOW_COPY_AND_ASSIGN(PointerWatcherEventRouterTest);
};

TEST_F(PointerWatcherEventRouterTest, EventTypes) {
  TestPointerWatcher pointer_watcher1, pointer_watcher2;
  PointerWatcherEventRouter* pointer_watcher_event_router =
      MusClient::Get()->pointer_watcher_event_router();
  aura::WindowTreeClientPrivate test_api(
      MusClient::Get()->window_tree_client());
  EXPECT_FALSE(test_api.HasPointerWatcher());

  // Start with no moves.
  pointer_watcher_event_router->AddPointerWatcher(&pointer_watcher1, false);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::NON_MOVE_EVENTS,
            event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Add moves.
  pointer_watcher_event_router->AddPointerWatcher(&pointer_watcher2, true);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::MOVE_EVENTS, event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Remove no-moves.
  pointer_watcher_event_router->RemovePointerWatcher(&pointer_watcher1);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::MOVE_EVENTS, event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Remove moves.
  pointer_watcher_event_router->RemovePointerWatcher(&pointer_watcher2);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::NONE, event_types());
  EXPECT_FALSE(test_api.HasPointerWatcher());

  // Add moves.
  pointer_watcher_event_router->AddPointerWatcher(&pointer_watcher2, true);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::MOVE_EVENTS, event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Add no moves.
  pointer_watcher_event_router->AddPointerWatcher(&pointer_watcher1, false);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::MOVE_EVENTS, event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Remove moves.
  pointer_watcher_event_router->RemovePointerWatcher(&pointer_watcher2);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::NON_MOVE_EVENTS,
            event_types());
  EXPECT_TRUE(test_api.HasPointerWatcher());

  // Remove no-moves.
  pointer_watcher_event_router->RemovePointerWatcher(&pointer_watcher1);
  EXPECT_EQ(PointerWatcherEventRouter::EventTypes::NONE, event_types());
  EXPECT_FALSE(test_api.HasPointerWatcher());
}

TEST_F(PointerWatcherEventRouterTest, PointerWatcherNoMove) {
  ASSERT_TRUE(MusClient::Get());
  PointerWatcherEventRouter* pointer_watcher_event_router =
      MusClient::Get()->pointer_watcher_event_router();
  ASSERT_TRUE(pointer_watcher_event_router);

  ui::PointerEvent pointer_event_down(
      ui::ET_POINTER_DOWN, gfx::Point(), gfx::Point(), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1),
      base::TimeTicks());
  ui::PointerEvent pointer_event_up(
      ui::ET_POINTER_UP, gfx::Point(), gfx::Point(), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE, 1),
      base::TimeTicks());
  ui::PointerEvent pointer_event_wheel(
      ui::ET_POINTER_WHEEL_CHANGED, gfx::Point(), gfx::Point(), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE, 1),
      base::TimeTicks());
  ui::PointerEvent pointer_event_capture(
      ui::ET_POINTER_CAPTURE_CHANGED, gfx::Point(), gfx::Point(), ui::EF_NONE,
      0, ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE, 1),
      base::TimeTicks());

  // PointerWatchers receive pointer down events.
  TestPointerWatcher watcher1;
  pointer_watcher_event_router->AddPointerWatcher(&watcher1, false);
  OnPointerEventObserved(pointer_event_down);
  EXPECT_EQ(ui::ET_POINTER_DOWN, watcher1.last_event_observed()->type());
  watcher1.Reset();

  // PointerWatchers receive pointer up events.
  OnPointerEventObserved(pointer_event_up);
  EXPECT_EQ(ui::ET_POINTER_UP, watcher1.last_event_observed()->type());
  watcher1.Reset();

  // PointerWatchers receive pointer wheel changed events.
  OnPointerEventObserved(pointer_event_wheel);
  EXPECT_EQ(ui::ET_POINTER_WHEEL_CHANGED,
            watcher1.last_event_observed()->type());
  watcher1.Reset();

  // PointerWatchers receive pointer capture changed events.
  OnPointerEventObserved(pointer_event_capture);
  EXPECT_EQ(ui::ET_POINTER_CAPTURE_CHANGED,
            watcher1.last_event_observed()->type());
  watcher1.Reset();

  // Two PointerWatchers can both receive a single observed event.
  TestPointerWatcher watcher2;
  pointer_watcher_event_router->AddPointerWatcher(&watcher2, false);
  OnPointerEventObserved(pointer_event_down);
  EXPECT_EQ(ui::ET_POINTER_DOWN, watcher1.last_event_observed()->type());
  EXPECT_EQ(ui::ET_POINTER_DOWN, watcher2.last_event_observed()->type());
  watcher1.Reset();
  watcher2.Reset();

  // Removing the first PointerWatcher stops sending events to it.
  pointer_watcher_event_router->RemovePointerWatcher(&watcher1);
  OnPointerEventObserved(pointer_event_down);
  EXPECT_FALSE(watcher1.last_event_observed());
  EXPECT_EQ(ui::ET_POINTER_DOWN, watcher2.last_event_observed()->type());
  watcher1.Reset();
  watcher2.Reset();

  // Removing the last PointerWatcher stops sending events to it.
  pointer_watcher_event_router->RemovePointerWatcher(&watcher2);
  OnPointerEventObserved(pointer_event_down);
  EXPECT_FALSE(watcher1.last_event_observed());
  EXPECT_FALSE(watcher2.last_event_observed());
}

TEST_F(PointerWatcherEventRouterTest, PointerWatcherMove) {
  ASSERT_TRUE(MusClient::Get());
  PointerWatcherEventRouter* pointer_watcher_event_router =
      MusClient::Get()->pointer_watcher_event_router();
  ASSERT_TRUE(pointer_watcher_event_router);

  ui::PointerEvent pointer_event_down(
      ui::ET_POINTER_DOWN, gfx::Point(), gfx::Point(), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1),
      base::TimeTicks());
  ui::PointerEvent pointer_event_move(
      ui::ET_POINTER_MOVED, gfx::Point(), gfx::Point(), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1),
      base::TimeTicks());

  // PointerWatchers receive pointer down events.
  TestPointerWatcher watcher1;
  pointer_watcher_event_router->AddPointerWatcher(&watcher1, true);
  OnPointerEventObserved(pointer_event_down);
  EXPECT_EQ(ui::ET_POINTER_DOWN, watcher1.last_event_observed()->type());
  watcher1.Reset();

  // PointerWatchers receive pointer move events.
  OnPointerEventObserved(pointer_event_move);
  EXPECT_EQ(ui::ET_POINTER_MOVED, watcher1.last_event_observed()->type());
  watcher1.Reset();

  // Two PointerWatchers can both receive a single observed event.
  TestPointerWatcher watcher2;
  pointer_watcher_event_router->AddPointerWatcher(&watcher2, true);
  OnPointerEventObserved(pointer_event_move);
  EXPECT_EQ(ui::ET_POINTER_MOVED, watcher1.last_event_observed()->type());
  EXPECT_EQ(ui::ET_POINTER_MOVED, watcher2.last_event_observed()->type());
  watcher1.Reset();
  watcher2.Reset();

  // Removing the first PointerWatcher stops sending events to it.
  pointer_watcher_event_router->RemovePointerWatcher(&watcher1);
  OnPointerEventObserved(pointer_event_move);
  EXPECT_FALSE(watcher1.last_event_observed());
  EXPECT_EQ(ui::ET_POINTER_MOVED, watcher2.last_event_observed()->type());
  watcher1.Reset();
  watcher2.Reset();

  // Removing the last PointerWatcher stops sending events to it.
  pointer_watcher_event_router->RemovePointerWatcher(&watcher2);
  OnPointerEventObserved(pointer_event_move);
  EXPECT_FALSE(watcher1.last_event_observed());
  EXPECT_FALSE(watcher2.last_event_observed());
}

TEST_F(PointerWatcherEventRouterTest, SecondaryDisplay) {
  PointerWatcherEventRouter* pointer_watcher_event_router =
      MusClient::Get()->pointer_watcher_event_router();
  TestPointerWatcher watcher;
  pointer_watcher_event_router->AddPointerWatcher(&watcher, false);

  ScreenMus* screen = MusClient::Get()->screen();
  const uint64_t kFirstDisplayId = screen->GetPrimaryDisplay().id();

  // The first display is at 0,0.
  ASSERT_TRUE(screen->GetPrimaryDisplay().bounds().origin().IsOrigin());

  // Add a secondary display to the right of the primary.
  const uint64_t kSecondDisplayId = 222;
  screen->display_list().AddDisplay(
      display::Display(kSecondDisplayId, gfx::Rect(800, 0, 640, 480)),
      display::DisplayList::Type::NOT_PRIMARY);

  ui::PointerEvent tap_event(
      ui::ET_POINTER_DOWN, gfx::Point(1, 1), gfx::Point(1, 1), ui::EF_NONE, 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1),
      base::TimeTicks());

  OnPointerEventObserved(tap_event, kFirstDisplayId);
  EXPECT_EQ(gfx::Point(1, 1), watcher.last_location_in_screen());

  OnPointerEventObserved(tap_event, kSecondDisplayId);
  EXPECT_EQ(gfx::Point(801, 1), watcher.last_location_in_screen());

  pointer_watcher_event_router->RemovePointerWatcher(&watcher);
}

}  // namespace views
