// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/system_tray.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/accessibility/accessibility_controller.h"
#include "ash/app_list/test/app_list_test_helper.h"
#include "ash/ash_view_ids.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_widget.h"
#include "ash/shell.h"
#include "ash/system/message_center/notification_tray.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_test_helper.h"
#include "ash/system/tray/system_tray_bubble.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/test_system_tray_item.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray_drag_controller.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test/ash_test_views_delegate.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_util.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/histogram_tester.h"
#include "ui/base/ui_base_types.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/separator.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {

const char kVisibleRowsHistogramName[] =
    "Ash.SystemMenu.DefaultView.VisibleRows";

class ModalWidgetDelegate : public views::WidgetDelegateView {
 public:
  ModalWidgetDelegate() = default;
  ~ModalWidgetDelegate() override = default;

  ui::ModalType GetModalType() const override { return ui::MODAL_TYPE_SYSTEM; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ModalWidgetDelegate);
};

class KeyEventConsumerView : public views::View {
 public:
  KeyEventConsumerView() : number_of_consumed_key_events_(0) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
  }
  ~KeyEventConsumerView() override = default;

  // Overriden from views::View
  bool SkipDefaultKeyEventProcessing(const ui::KeyEvent& event) override {
    return true;
  }
  void OnKeyEvent(ui::KeyEvent* key_event) override {
    number_of_consumed_key_events_++;
  }

  int number_of_consumed_key_events() { return number_of_consumed_key_events_; }

 private:
  int number_of_consumed_key_events_;

  DISALLOW_COPY_AND_ASSIGN(KeyEventConsumerView);
};

}  // namespace

// TODO(minch): move swiping related tests from SystemTrayTest to
// TraybackgroundView / TrayDragController. crbug.com/745073
class SystemTrayTest : public AshTestBase {
 public:
  SystemTrayTest() = default;
  ~SystemTrayTest() override = default;

  // Swiping on the system tray and ends with finger released. Note, |start| is
  // based on the system tray or system tray bubble's coordinate space.
  void SendGestureEvent(const gfx::Point& start,
                        float delta,
                        bool is_fling,
                        float velocity_y,
                        float scroll_y_hint = -1.0f,
                        bool drag_on_bubble = false) {
    base::TimeTicks timestamp = base::TimeTicks::Now();
    SendScrollStartAndUpdate(start, delta, timestamp, scroll_y_hint,
                             drag_on_bubble);

    ui::GestureEventDetails details =
        is_fling
            ? ui::GestureEventDetails(ui::ET_SCROLL_FLING_START, 0, velocity_y)
            : ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_END);
    ui::GestureEvent event = ui::GestureEvent(start.x(), start.y() + delta,
                                              ui::EF_NONE, timestamp, details);
    DispatchGestureEvent(&event, drag_on_bubble);
  }

  // Swiping on the system tray without releasing the finger.
  void SendScrollStartAndUpdate(const gfx::Point& start,
                                float delta,
                                base::TimeTicks& timestamp,
                                float scroll_y_hint = -1.0f,
                                bool drag_on_bubble = false) {
    ui::GestureEventDetails begin_details(ui::ET_GESTURE_SCROLL_BEGIN, 0,
                                          scroll_y_hint);
    ui::GestureEvent begin_event = ui::GestureEvent(
        start.x(), start.y(), ui::EF_NONE, timestamp, begin_details);
    DispatchGestureEvent(&begin_event, drag_on_bubble);

    ui::GestureEventDetails update_details(ui::ET_GESTURE_SCROLL_UPDATE, 0,
                                           delta);
    timestamp += base::TimeDelta::FromMilliseconds(100);
    ui::GestureEvent update_event = ui::GestureEvent(
        start.x(), start.y() + delta, ui::EF_NONE, timestamp, update_details);
    DispatchGestureEvent(&update_event, drag_on_bubble);
  }

  // Dispatches |event| to target according to |drag_on_bubble|.
  void DispatchGestureEvent(ui::GestureEvent* event, bool drag_on_bubble) {
    SystemTray* system_tray = GetPrimarySystemTray();
    views::View* target = drag_on_bubble
                              ? system_tray->GetSystemBubble()->bubble_view()
                              : static_cast<views::View*>(system_tray);
    ui::Event::DispatcherApi(event).set_target(target);
    target->OnGestureEvent(event);
  }

  // Open the default system tray bubble to get the height of the bubble and
  // then close it.
  float GetSystemBubbleHeight() {
    SystemTray* system_tray = GetPrimarySystemTray();
    system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
    gfx::Rect bounds = GetSystemBubbleBoundsInScreen();
    system_tray->CloseBubble();
    return bounds.height();
  }

  gfx::Rect GetSystemBubbleBoundsInScreen() {
    return GetPrimarySystemTray()
        ->GetSystemBubble()
        ->bubble_view()
        ->GetWidget()
        ->GetWindowBoundsInScreen();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemTrayTest);
};

// TODO(minch): Swiping the status area tray or associated bubble in tablet mode
// has been disabled currently, it may be added back in the future.
// http://crbug.com/767679.
// Swiping on the overlap area of shelf and system tray bubble during the
// animation should close the bubble.
TEST_F(SystemTrayTest, DISABLED_SwipingOnShelfDuringAnimation) {
  Shelf* shelf = GetPrimaryShelf();
  SystemTray* system_tray = GetPrimarySystemTray();
  gfx::Point start = system_tray->GetLocalBounds().CenterPoint();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  gfx::Rect shelf_bounds_in_screen =
      shelf->shelf_widget()->GetWindowBoundsInScreen();

  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  gfx::Rect original_bounds = GetSystemBubbleBoundsInScreen();
  system_tray->CloseBubble();

  // Enable animations so that we can make sure that they occur.
  ui::ScopedAnimationDurationScaleMode regular_animations(
      ui::ScopedAnimationDurationScaleMode::NON_ZERO_DURATION);

  ui::test::EventGenerator& generator = GetEventGenerator();
  gfx::Point point_on_shelf_start =
      gfx::Point(original_bounds.x() + 5, shelf_bounds_in_screen.y() + 5);
  gfx::Point point_on_shelf_end(point_on_shelf_start.x(),
                                shelf_bounds_in_screen.bottom());

  // Swiping up exceed one third of the height of the bubble should show the
  // bubble.
  float delta = -original_bounds.height() / 2;
  SendGestureEvent(start, delta, false, 0);
  EXPECT_TRUE(system_tray->HasSystemBubble());
  gfx::Rect current_bounds = GetSystemBubbleBoundsInScreen();

  // Dragging the shelf during up animation should close the bubble.
  if (current_bounds.y() != original_bounds.y()) {
    generator.GestureScrollSequence(point_on_shelf_start, point_on_shelf_end,
                                    base::TimeDelta::FromMilliseconds(100), 5);
    EXPECT_FALSE(system_tray->HasSystemBubble());
  }

  // Fling down on the shelf with a velocity that exceeds |kFlingVelocity|.
  EXPECT_FALSE(system_tray->HasSystemBubble());
  SendGestureEvent(start, delta, true, TrayDragController::kFlingVelocity + 1);
  current_bounds = GetSystemBubbleBoundsInScreen();
  EXPECT_TRUE(system_tray->HasSystemBubble());

  // Dragging the shelf during down animation should close the bubble.
  if (current_bounds.y() != original_bounds.y()) {
    generator.GestureScrollSequence(point_on_shelf_start, point_on_shelf_end,
                                    base::TimeDelta::FromMilliseconds(100), 5);
    EXPECT_FALSE(system_tray->HasSystemBubble());
  }
}

// TODO(minch): Swiping the status area tray or associated bubble in tablet mode
// has been disabled currently, it may be added back in the future.
// http://crbug.com/767679.
// Swiping on the system tray ends with fling event.
TEST_F(SystemTrayTest, DISABLED_FlingOnSystemTray) {
  Shelf* shelf = GetPrimaryShelf();
  SystemTray* system_tray = GetPrimarySystemTray();
  gfx::Point start = system_tray->GetLocalBounds().CenterPoint();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);

  // Fling up on the system tray should show the bubble if the |velocity_y| is
  // larger than |kFlingVelocity| and the dragging amount is larger than one
  // third of the height of the bubble.
  float delta = -GetSystemBubbleHeight();
  SendGestureEvent(start, delta, true,
                   -(TrayDragController::kFlingVelocity + 1));
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Fling up on the system tray should show the bubble if the |velocity_y| is
  // larger than |kFlingVelocity| even the dragging amount is less than one
  // third of the height of the bubble.
  delta /= 4;
  SendGestureEvent(start, delta, true,
                   -(TrayDragController::kFlingVelocity + 1));
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Fling up on the system tray should show the bubble if the |velocity_y| is
  // less than |kFlingVelocity| but the dragging amount if larger than one third
  // of the height of the bubble.
  delta = -GetSystemBubbleHeight();
  SendGestureEvent(start, delta, true,
                   -(TrayDragController::kFlingVelocity - 1));
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Fling up on the system tray should close the bubble if the |velocity_y|
  // is less than |kFlingVelocity| and the dragging amount is less than one
  // third of the height of the bubble.
  delta /= 4;
  SendGestureEvent(start, delta, true,
                   -(TrayDragController::kFlingVelocity - 1));
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Fling down on the system tray should close the bubble if the |velocity_y|
  // is larger than kFLingVelocity.
  SendGestureEvent(start, delta, true, TrayDragController::kFlingVelocity + 1);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Fling down on the system tray should close the bubble if the |velocity_y|
  // is larger than |kFlingVelocity| even the dragging amount is larger than one
  // third of the height of the bubble.
  delta = -GetSystemBubbleHeight();
  SendGestureEvent(start, delta, true, TrayDragController::kFlingVelocity + 1);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Fling down on the system tray should open the bubble if the |velocity_y| is
  // less than |kFlingVelocity| but the dragging amount exceed one third of the
  // height of the bubble.
  SendGestureEvent(start, delta, true, TrayDragController::kFlingVelocity - 1);
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Fling down on the system tray should close the bubble if the |velocity_y|
  // is less than |kFlingVelocity| and the dragging amount is less than one
  // third of the height of the bubble.
  delta /= 4;
  SendGestureEvent(start, delta, true, TrayDragController::kFlingVelocity - 1);
  EXPECT_FALSE(system_tray->HasSystemBubble());
}

// TODO(minch): Swiping the status area tray or associated bubble in tablet mode
// has been disabled currently, it may be added back in the future.
// http://crbug.com/767679.
// Touch outside the system tray bubble during swiping should close the bubble.
TEST_F(SystemTrayTest, DISABLED_TapOutsideCloseBubble) {
  Shelf* shelf = GetPrimaryShelf();
  SystemTray* system_tray = GetPrimarySystemTray();
  gfx::Point start = system_tray->GetLocalBounds().CenterPoint();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  float delta = -GetSystemBubbleHeight();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  base::TimeTicks timestamp = base::TimeTicks::Now();
  SendScrollStartAndUpdate(start, delta, timestamp);
  EXPECT_TRUE(system_tray->HasSystemBubble());

  ui::test::EventGenerator& generator = GetEventGenerator();
  gfx::Rect bounds = GetSystemBubbleBoundsInScreen();
  gfx::Point point_outside = gfx::Point(bounds.x() - 5, bounds.y() - 5);
  generator.GestureTapAt(point_outside);
  EXPECT_FALSE(system_tray->HasSystemBubble());
}

// TODO(minch): Swiping the status area tray or associated bubble in tablet mode
// has been disabled currently, it may be added back in the future.
// http://crbug.com/767679.
// Swiping on the system tray ends with scroll event.
TEST_F(SystemTrayTest, DISABLED_SwipingOnSystemTray) {
  Shelf* shelf = GetPrimaryShelf();
  SystemTray* system_tray = GetPrimarySystemTray();
  gfx::Point start = system_tray->GetLocalBounds().CenterPoint();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  // Swiping up on the system tray has no effect if it is not in tablet mode.
  float delta = -GetSystemBubbleHeight();
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_FALSE(system_tray->HasSystemBubble());
  SendGestureEvent(start, delta, false, 0);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Swiping up on the system tray should show the system tray bubble if it is
  // in tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  SendGestureEvent(start, delta, false, 0);
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Swiping up less than one third of the bubble's height should not show the
  // bubble.
  delta /= 4;
  SendGestureEvent(start, delta, false, 0);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Swiping up more than one third of the bubble's height should show the
  // bubble.
  delta = -GetSystemBubbleHeight() / 2;
  SendGestureEvent(start, delta, false, 0);
  EXPECT_TRUE(system_tray->HasSystemBubble());
  system_tray->CloseBubble();

  // Swiping up on system tray should not show the system tray bubble if the
  // shelf is left alignment.
  delta = -GetSystemBubbleHeight();
  shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
  SendGestureEvent(start, delta, false, 0);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Swiping up on system tray should not show the system tray bubble if the
  // shelf is right alignment.
  shelf->SetAlignment(SHELF_ALIGNMENT_RIGHT);
  SendGestureEvent(start, delta, false, 0);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Beginning to scroll downward on the shelf should not show the system tray
  // bubble, even if the drag then moves upwards.
  shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
  SendGestureEvent(start, delta, false, 0, 1);
  EXPECT_FALSE(system_tray->HasSystemBubble());
}

// TODO(minch): Swiping the status area tray or associated bubble in tablet mode
// has been disabled currently, it may be added back in the future.
// http://crbug.com/767679.
// Tests for swiping down on an open system tray bubble in order to
// close it.
TEST_F(SystemTrayTest, DISABLED_SwipingOnSystemTrayBubble) {
  Shelf* shelf = GetPrimaryShelf();
  SystemTray* system_tray = GetPrimarySystemTray();
  EXPECT_EQ(SHELF_ALIGNMENT_BOTTOM, shelf->alignment());

  // Beginning to scroll downward and then swiping down more than one third of
  // the bubble's height should close the bubble. This only takes effect in
  // maximize mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  gfx::Rect bounds =
      system_tray->GetSystemBubble()->bubble_view()->GetLocalBounds();
  float delta = bounds.height() / 2;
  gfx::Point start(bounds.x() + 5, bounds.y() + 5);
  SendGestureEvent(start, delta, false, 0, 1, true);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Beginning to scroll upward and then swiping down more than one third of the
  // bubble's height should also close the bubble.
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  SendGestureEvent(start, delta, false, 0, -1, true);
  EXPECT_FALSE(system_tray->HasSystemBubble());

  // Swiping on the bubble has no effect if it is not in maximize mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  SendGestureEvent(start, delta, false, 0, -1, true);
  EXPECT_TRUE(system_tray->HasSystemBubble());
}

// Tests that press the search key can toggle the launcher when all the windows
// are minimized after open the system tray bubble in tablet mode.
TEST_F(SystemTrayTest, ToggleAppListAfterOpenSystemTrayBubbleInTabletMode) {
  EXPECT_FALSE(wm::GetActiveWindow());
  SystemTray* system_tray = GetPrimarySystemTray();

  // Open the system tray bubble in tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  EXPECT_FALSE(system_tray->clipping_window_for_test());
  system_tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_FALSE(system_tray->drag_controller());
  EXPECT_FALSE(system_tray->clipping_window_for_test());

  // Convert from tablet mode to clamshell.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  GetAppListTestHelper()->CheckVisibility(false);

  // Press the search key should toggle the launcher.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.PressKey(ui::VKEY_BROWSER_SEARCH, ui::EF_NONE);
  GetAppListTestHelper()->WaitUntilIdle();
  GetAppListTestHelper()->CheckVisibility(true);

  // Press the search key again should still toggle the launcher.
  generator.PressKey(ui::VKEY_BROWSER_SEARCH, ui::EF_NONE);
  GetAppListTestHelper()->WaitUntilIdle();
  GetAppListTestHelper()->CheckVisibility(false);
}

// Verifies only the visible default views are recorded in the
// "Ash.SystemMenu.DefaultView.VisibleItems" histogram.
TEST_F(SystemTrayTest, OnlyVisibleItemsRecorded) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestSystemTrayItem* test_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(test_item));

  base::HistogramTester histogram_tester;

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 1);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 2);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();

  test_item->set_views_are_visible(false);

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 2);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
}

// Verifies a visible UMA_NOT_RECORDED default view is not recorded in the
// "Ash.SystemMenu.DefaultView.VisibleItems" histogram.
TEST_F(SystemTrayTest, NotRecordedtemsAreNotRecorded) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  tray->AddTrayItem(
      std::make_unique<TestSystemTrayItem>(SystemTrayItem::UMA_NOT_RECORDED));

  base::HistogramTester histogram_tester;

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_NOT_RECORDED, 0);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
}

// Verifies null default views are not recorded in the
// "Ash.SystemMenu.DefaultView.VisibleItems" histogram.
TEST_F(SystemTrayTest, NullDefaultViewIsNotRecorded) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  auto test_item = std::make_unique<TestSystemTrayItem>();
  test_item->set_has_views(false);
  tray->AddTrayItem(std::move(test_item));

  base::HistogramTester histogram_tester;

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 0);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
}

// Verifies visible detailed views are not recorded in the
// "Ash.SystemMenu.DefaultView.VisibleItems" histogram.
TEST_F(SystemTrayTest, VisibleDetailedViewsIsNotRecorded) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestSystemTrayItem* test_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(test_item));

  base::HistogramTester histogram_tester;

  tray->ShowDetailedView(test_item, 0, BUBBLE_CREATE_NEW);
  RunAllPendingInMessageLoop();

  histogram_tester.ExpectTotalCount(kVisibleRowsHistogramName, 0);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
}

// Verifies visible default views are not recorded for menu re-shows in the
// "Ash.SystemMenu.DefaultView.VisibleItems" histogram.
TEST_F(SystemTrayTest, VisibleDefaultViewIsNotRecordedOnReshow) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestSystemTrayItem* test_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(test_item));

  base::HistogramTester histogram_tester;

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 1);

  tray->ShowDetailedView(test_item, 0, BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 1);

  tray->ShowDefaultView(BUBBLE_USE_EXISTING, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  histogram_tester.ExpectBucketCount(kVisibleRowsHistogramName,
                                     SystemTrayItem::UMA_TEST, 1);

  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
}

TEST_F(SystemTrayTest, SystemTrayDefaultView) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);

  // Ensure that closing the bubble destroys it.
  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
  ASSERT_FALSE(tray->HasSystemBubble());
}

// Make sure the opening system tray bubble will not deactivate the
// other window. crbug.com/120680.
TEST_F(SystemTrayTest, Activation) {
  SystemTray* tray = GetPrimarySystemTray();
  std::unique_ptr<views::Widget> widget(CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 100, 100)));
  EXPECT_TRUE(widget->IsActive());

  // The window stays active after the bubble opens.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(tray->GetWidget());
  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_TRUE(widget->IsActive());

  // Activating the bubble makes the window lose activation.
  tray->ActivateBubble();
  EXPECT_TRUE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_FALSE(widget->IsActive());

  // Closing the bubble re-activates the window.
  tray->CloseBubble();
  EXPECT_TRUE(widget->IsActive());

  // Opening the bubble with an accelerator activates the bubble because the
  // user will probably navigate with the keyboard.
  Shell::Get()->accelerator_controller()->PerformActionIfEnabled(
      TOGGLE_SYSTEM_TRAY_BUBBLE);
  ASSERT_TRUE(tray->GetWidget());
  EXPECT_TRUE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_FALSE(widget->IsActive());
}

// Makes sure that the system tray bubble closes when another window is
// activated, and does not crash regardless of the initial activation state.
// Regression test for crbug.com/704432 .
TEST_F(SystemTrayTest, CloseOnActivation) {
  SystemTray* tray = GetPrimarySystemTray();

  // Show the system bubble.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());

  // Test 1: no crash when there's no active window to begin with.
  EXPECT_FALSE(wm::GetActiveWindow());

  // Showing a new window and activating it will close the system bubble.
  std::unique_ptr<views::Widget> widget(CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 100, 100)));
  EXPECT_TRUE(widget->IsActive());
  EXPECT_FALSE(tray->GetSystemBubble());

  // Show a second widget.
  std::unique_ptr<views::Widget> second_widget(CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 100, 100)));
  EXPECT_TRUE(second_widget->IsActive());

  // Re-show the system bubble.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());

  // Test 2: also no crash when there is a previously active window.
  EXPECT_TRUE(wm::GetActiveWindow());

  // Re-activate the first widget. The system bubble should hide again.
  widget->Activate();
  EXPECT_FALSE(tray->GetSystemBubble());
}

// Opening and closing the bubble should change the coloring of the tray.
TEST_F(SystemTrayTest, SystemTrayColoring) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());
  // At the beginning the tray coloring is not active.
  ASSERT_FALSE(tray->is_active());

  // Showing the system bubble should show the background as active.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(tray->is_active());

  // Closing the system menu should change the coloring back to normal.
  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();
  ASSERT_FALSE(tray->is_active());
}

// Closing the system bubble through an alignment change should change the
// system tray coloring back to normal.
TEST_F(SystemTrayTest, SystemTrayColoringAfterAlignmentChange) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());
  Shelf* shelf = GetPrimaryShelf();
  shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
  // At the beginning the tray coloring is not active.
  ASSERT_FALSE(tray->is_active());

  // Showing the system bubble should show the background as active.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(tray->is_active());

  // Changing the alignment should close the system bubble and change the
  // background color.
  shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
  ASSERT_FALSE(tray->is_active());
  RunAllPendingInMessageLoop();
  // The bubble should already be closed by now.
  ASSERT_FALSE(tray->HasSystemBubble());
}

TEST_F(SystemTrayTest, SystemTrayTestItems) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestSystemTrayItem* test_item = new TestSystemTrayItem();
  TestSystemTrayItem* detailed_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(test_item));
  tray->AddTrayItem(base::WrapUnique(detailed_item));

  // Check items have been added.
  std::vector<SystemTrayItem*> items = tray->GetTrayItems();
  ASSERT_TRUE(base::ContainsValue(items, test_item));
  ASSERT_TRUE(base::ContainsValue(items, detailed_item));

  // Ensure the tray views are created.
  ASSERT_TRUE(test_item->tray_view() != NULL);
  ASSERT_TRUE(detailed_item->tray_view() != NULL);

  // Ensure a default views are created.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(test_item->default_view() != NULL);
  ASSERT_TRUE(detailed_item->default_view() != NULL);

  // Show the detailed view, ensure it's created and the default view destroyed.
  tray->ShowDetailedView(detailed_item, 0, BUBBLE_CREATE_NEW);
  RunAllPendingInMessageLoop();
  ASSERT_TRUE(test_item->default_view() == NULL);
  ASSERT_TRUE(detailed_item->detailed_view() != NULL);

  // Show the default view, ensure it's created and the detailed view destroyed.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  ASSERT_TRUE(test_item->default_view() != NULL);
  ASSERT_TRUE(detailed_item->detailed_view() == NULL);
}

TEST_F(SystemTrayTest, SystemTrayNoViewItems) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  // Verify that no crashes occur on items lacking some views.
  TestSystemTrayItem* no_view_item = new TestSystemTrayItem();
  no_view_item->set_has_views(false);
  tray->AddTrayItem(base::WrapUnique(no_view_item));
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  tray->ShowDetailedView(no_view_item, 0, BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();
}

TEST_F(SystemTrayTest, TrayWidgetAutoResizes) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  // Add an initial tray item so that the tray gets laid out correctly.
  TestSystemTrayItem* initial_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(initial_item));

  gfx::Size initial_size = tray->GetWidget()->GetWindowBoundsInScreen().size();

  TestSystemTrayItem* new_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(new_item));

  gfx::Size new_size = tray->GetWidget()->GetWindowBoundsInScreen().size();

  // Adding the new item should change the size of the tray.
  EXPECT_NE(initial_size.ToString(), new_size.ToString());

  // Hiding the tray view of the new item should also change the size of the
  // tray.
  new_item->tray_view()->SetVisible(false);
  EXPECT_EQ(initial_size.ToString(),
            tray->GetWidget()->GetWindowBoundsInScreen().size().ToString());

  new_item->tray_view()->SetVisible(true);
  EXPECT_EQ(new_size.ToString(),
            tray->GetWidget()->GetWindowBoundsInScreen().size().ToString());
}

// Test is flaky. http://crbug.com/637978
TEST_F(SystemTrayTest, DISABLED_BubbleCreationTypesTest) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  TestSystemTrayItem* test_item = new TestSystemTrayItem();
  tray->AddTrayItem(base::WrapUnique(test_item));

  // Ensure the tray views are created.
  ASSERT_TRUE(test_item->tray_view() != NULL);

  // Show the default view, ensure the notification view is destroyed.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();

  views::Widget* widget = test_item->default_view()->GetWidget();
  gfx::Rect bubble_bounds = widget->GetWindowBoundsInScreen();

  tray->ShowDetailedView(test_item, 0, BUBBLE_USE_EXISTING);
  RunAllPendingInMessageLoop();

  EXPECT_FALSE(test_item->default_view());

  EXPECT_EQ(bubble_bounds.ToString(), test_item->detailed_view()
                                          ->GetWidget()
                                          ->GetWindowBoundsInScreen()
                                          .ToString());
  EXPECT_EQ(widget, test_item->detailed_view()->GetWidget());

  tray->ShowDefaultView(BUBBLE_USE_EXISTING, false /* show_by_click */);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(bubble_bounds.ToString(), test_item->default_view()
                                          ->GetWidget()
                                          ->GetWindowBoundsInScreen()
                                          .ToString());
  EXPECT_EQ(widget, test_item->default_view()->GetWidget());
}

// Tests that the tray view is laid out properly and is fully contained within
// the shelf widget.
TEST_F(SystemTrayTest, TrayBoundsInWidget) {
  Shelf* shelf = GetPrimaryShelf();
  StatusAreaWidget* widget = StatusAreaWidgetTestHelper::GetStatusAreaWidget();
  SystemTray* tray = GetPrimarySystemTray();

  // Test in bottom alignment.
  shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
  gfx::Rect window_bounds = widget->GetWindowBoundsInScreen();
  gfx::Rect tray_bounds = tray->GetBoundsInScreen();
  EXPECT_TRUE(window_bounds.Contains(tray_bounds));

  // Test in locked alignment.
  shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM_LOCKED);
  window_bounds = widget->GetWindowBoundsInScreen();
  tray_bounds = tray->GetBoundsInScreen();
  EXPECT_TRUE(window_bounds.Contains(tray_bounds));

  // Test in the left alignment.
  shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
  window_bounds = widget->GetWindowBoundsInScreen();
  tray_bounds = tray->GetBoundsInScreen();
  // TODO(estade): Re-enable this check. See crbug.com/660928.
  // EXPECT_TRUE(window_bounds.Contains(tray_bounds));

  // Test in the right alignment.
  shelf->SetAlignment(SHELF_ALIGNMENT_LEFT);
  window_bounds = widget->GetWindowBoundsInScreen();
  tray_bounds = tray->GetBoundsInScreen();
  // TODO(estade): Re-enable this check. See crbug.com/660928.
  // EXPECT_TRUE(window_bounds.Contains(tray_bounds));
}

TEST_F(SystemTrayTest, PersistentBubble) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->GetWidget());

  tray->AddTrayItem(std::make_unique<TestSystemTrayItem>());

  std::unique_ptr<views::Widget> widget(CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 100, 100)));

  // Tests for usual default view while activating a window.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  tray->ActivateBubble();
  ASSERT_TRUE(tray->HasSystemBubble());
  widget->Activate();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tray->HasSystemBubble());

  // Clicking outside the bubble should close it.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(tray->HasSystemBubble());
  {
    ui::test::EventGenerator& generator = GetEventGenerator();
    generator.set_current_location(gfx::Point(5, 5));
    generator.ClickLeftButton();
    ASSERT_FALSE(tray->HasSystemBubble());
  }

  // Same tests for persistent default view.
  tray->ShowPersistentDefaultView();
  ASSERT_TRUE(tray->HasSystemBubble());
  widget->Activate();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tray->HasSystemBubble());

  {
    ui::test::EventGenerator& generator = GetEventGenerator();
    generator.set_current_location(gfx::Point(5, 5));
    generator.ClickLeftButton();
    ASSERT_TRUE(tray->HasSystemBubble());
  }

  // Same tests for persistent default view with activation.
  tray->ShowPersistentDefaultView();
  EXPECT_TRUE(tray->HasSystemBubble());
  widget->Activate();
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(tray->HasSystemBubble());

  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.set_current_location(gfx::Point(5, 5));
  generator.ClickLeftButton();
  EXPECT_TRUE(tray->HasSystemBubble());
}

// With a system modal dialog, the bubble should be created with a LOCKED login
// status.
TEST_F(SystemTrayTest, WithSystemModal) {
  // The accessiblity item is created and is visible either way.
  Shell::Get()->accessibility_controller()->SetVirtualKeyboardEnabled(true);
  std::unique_ptr<views::Widget> widget(CreateTestWidget(
      new ModalWidgetDelegate, kShellWindowId_SystemModalContainer,
      gfx::Rect(0, 0, 100, 100)));

  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);

  ASSERT_TRUE(tray->HasSystemBubble());
  const views::View* accessibility =
      tray->GetSystemBubble()->bubble_view()->GetViewByID(
          VIEW_ID_ACCESSIBILITY_TRAY_ITEM);
  ASSERT_TRUE(accessibility);
  EXPECT_TRUE(accessibility->visible());

  // The bluetooth item is disabled in locked mode.
  const views::View* bluetooth =
      tray->GetSystemBubble()->bubble_view()->GetViewByID(
          VIEW_ID_BLUETOOTH_DEFAULT_VIEW);
  ASSERT_TRUE(bluetooth);
  EXPECT_FALSE(bluetooth->enabled());

  // Close the modal dialog.
  widget.reset();

  // System modal is gone. The bluetooth item should be enabled now.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  accessibility = tray->GetSystemBubble()->bubble_view()->GetViewByID(
      VIEW_ID_ACCESSIBILITY_TRAY_ITEM);
  ASSERT_TRUE(accessibility);
  EXPECT_TRUE(accessibility->visible());

  bluetooth = tray->GetSystemBubble()->bubble_view()->GetViewByID(
      VIEW_ID_BLUETOOTH_DEFAULT_VIEW);
  ASSERT_TRUE(bluetooth);
  EXPECT_TRUE(bluetooth->enabled());
}

// Tests that if SetVisible(true) is called while animating to hidden that the
// tray becomes visible, and stops animating to hidden.
TEST_F(SystemTrayTest, SetVisibleDuringHideAnimation) {
  SystemTray* tray = GetPrimarySystemTray();
  ASSERT_TRUE(tray->visible());

  std::unique_ptr<ui::ScopedAnimationDurationScaleMode> animation_duration;
  animation_duration.reset(new ui::ScopedAnimationDurationScaleMode(
      ui::ScopedAnimationDurationScaleMode::SLOW_DURATION));
  tray->SetVisible(false);
  EXPECT_TRUE(tray->visible());
  EXPECT_EQ(0.0f, tray->layer()->GetTargetOpacity());

  tray->SetVisible(true);
  animation_duration.reset();
  tray->layer()->GetAnimator()->StopAnimating();
  EXPECT_TRUE(tray->visible());
  EXPECT_EQ(1.0f, tray->layer()->GetTargetOpacity());
}

TEST_F(SystemTrayTest, SystemTrayHeightWithBubble) {
  SystemTray* tray = GetPrimarySystemTray();
  NotificationTray* notification_tray =
      StatusAreaWidgetTestHelper::GetStatusAreaWidget()->notification_tray();

  // Ensure the initial tray bubble height is zero.
  EXPECT_EQ(0, notification_tray->tray_bubble_height_for_test());

  // Show the default view, ensure the tray bubble height is changed.
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  RunAllPendingInMessageLoop();
  EXPECT_LT(0, notification_tray->tray_bubble_height_for_test());

  // Hide the default view, ensure the tray bubble height is back to zero.
  ASSERT_TRUE(tray->HasSystemBubble());
  tray->CloseBubble();
  RunAllPendingInMessageLoop();

  EXPECT_EQ(0, notification_tray->tray_bubble_height_for_test());
}

TEST_F(SystemTrayTest, SeparatorThickness) {
  EXPECT_EQ(kSeparatorWidth, views::Separator::kThickness);
}

// System tray is not activated by default. If it is opened by user click on
// system tray, it should be activated when user presses tab key.
TEST_F(SystemTrayTest, KeyboardNavigationWithOtherWindow) {
  std::unique_ptr<views::Widget> widget(CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 100, 100)));
  EXPECT_TRUE(widget->IsActive());

  // Add a view which tries to handle key event by themselves, and focus on it.
  KeyEventConsumerView key_event_consumer_view;
  views::View* root_view = widget->GetContentsView();
  root_view->AddChildView(&key_event_consumer_view);
  key_event_consumer_view.RequestFocus();
  EXPECT_EQ(&key_event_consumer_view,
            key_event_consumer_view.GetFocusManager()->GetFocusedView());

  // Show system tray by performing a gesture tap at tray.
  SystemTray* tray = GetPrimarySystemTray();
  ui::GestureEvent tap(0, 0, 0, base::TimeTicks(),
                       ui::GestureEventDetails(ui::ET_GESTURE_TAP));
  tray->PerformAction(tap);
  ASSERT_TRUE(tray->GetWidget());

  // Confirms that system tray is not activated at this time.
  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_TRUE(widget->IsActive());

  ui::test::EventGenerator& event_generator = GetEventGenerator();
  int number_of_consumed_key_events =
      key_event_consumer_view.number_of_consumed_key_events();

  // Send A key event. Nothing should happen for the tray. Key event is consumed
  // by the tray.
  event_generator.PressKey(ui::VKEY_A, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_A, ui::EF_NONE);

  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_TRUE(widget->IsActive());
  EXPECT_EQ(number_of_consumed_key_events,
            key_event_consumer_view.number_of_consumed_key_events());

  // Send tab key event.
  event_generator.PressKey(ui::VKEY_TAB, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_TAB, ui::EF_NONE);

  // Confirms that system tray is activated.
  EXPECT_TRUE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_FALSE(widget->IsActive());
  EXPECT_EQ(number_of_consumed_key_events,
            key_event_consumer_view.number_of_consumed_key_events());

  // Close system tray and reopen it by not explicit user click.
  tray->CloseBubble();
  ASSERT_FALSE(tray->IsSystemBubbleVisible());
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  ASSERT_TRUE(tray->GetWidget());

  // Confirms that system tray is not activated by tab key event to verify that
  // RerouteEventHandler is not installed in this case.
  event_generator.PressKey(ui::VKEY_TAB, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_TAB, ui::EF_NONE);
  EXPECT_FALSE(tray->GetSystemBubble()->bubble_view()->GetWidget()->IsActive());
  EXPECT_TRUE(widget->IsActive());
}

// When System tray is opened by explicit user click, it passes a key event to
// ViewsDelegate if it's not handled by the tray. It closes the tray if
// ViewsDelegate returns CLOSE_MENU.
TEST_F(SystemTrayTest, AcceleratorController) {
  // Register A key as an accelerator which closes the menu.
  ui::Accelerator accelerator(ui::VKEY_A, ui::EF_NONE);
  AshTestViewsDelegate* views_delegate =
      ash_test_helper()->test_views_delegate();
  views_delegate->set_close_menu_accelerator(accelerator);

  // Show system tray by performing a gesture tap at tray.
  SystemTray* tray = GetPrimarySystemTray();
  ui::GestureEvent tap(0, 0, 0, base::TimeTicks(),
                       ui::GestureEventDetails(ui::ET_GESTURE_TAP));
  tray->PerformAction(tap);
  ASSERT_TRUE(tray->GetWidget());
  ASSERT_TRUE(tray->IsSystemBubbleVisible());

  ui::test::EventGenerator& event_generator = GetEventGenerator();

  // Send B key event and confirms that nothing happens.
  event_generator.PressKey(ui::VKEY_B, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_B, ui::EF_NONE);
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Send A key event and confirms that system tray becomes invisible.
  event_generator.PressKey(ui::VKEY_A, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_A, ui::EF_NONE);
  EXPECT_FALSE(tray->IsSystemBubbleVisible());
}

// When system tray has an active child widget, the child widget should consume
// key event and the tray shouldn't consume key event, i.e. RerouteEventHandler
// in TrayBubbleView should not reroute key events to the tray in this case.
TEST_F(SystemTrayTest, ActiveChildWidget) {
  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, true /* show_by_click */);

  // Create a child widget on system tray and focus it.
  views::Widget* child_widget = views::Widget::CreateWindowWithParent(
      nullptr, tray->GetBubbleView()->GetWidget()->GetNativeView());
  std::unique_ptr<KeyEventConsumerView> consumer_view(
      new KeyEventConsumerView());
  child_widget->GetContentsView()->AddChildView(consumer_view.get());
  child_widget->Show();
  consumer_view->RequestFocus();

  ASSERT_FALSE(tray->GetBubbleView()->GetWidget()->IsActive());
  ASSERT_TRUE(child_widget->IsActive());

  ui::test::EventGenerator& event_generator = GetEventGenerator();

  // Press ESC key and confirm that child widget consumes it. Also confirm that
  // the tray does not consume the key event.
  ASSERT_EQ(0, consumer_view->number_of_consumed_key_events());
  ASSERT_TRUE(tray->IsSystemBubbleVisible());
  event_generator.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  EXPECT_EQ(2, consumer_view->number_of_consumed_key_events());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Hide child widget and press ESC key. Confirm that the tray consumes it and
  // the tray is closed even if the tray is not active.
  child_widget->Hide();
  ASSERT_FALSE(tray->GetBubbleView()->GetWidget()->IsActive());
  event_generator.PressKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  event_generator.ReleaseKey(ui::VKEY_ESCAPE, ui::EF_NONE);
  EXPECT_FALSE(tray->IsSystemBubbleVisible());
}

}  // namespace ash
