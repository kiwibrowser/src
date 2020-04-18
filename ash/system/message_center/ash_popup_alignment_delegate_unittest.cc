// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/message_center/ash_popup_alignment_delegate.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/message_center/public/cpp/message_center_constants.h"

namespace ash {

class AshPopupAlignmentDelegateTest : public AshTestBase {
 public:
  AshPopupAlignmentDelegateTest() = default;
  ~AshPopupAlignmentDelegateTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        keyboard::switches::kEnableVirtualKeyboard);
    AshTestBase::SetUp();
    SetAlignmentDelegate(
        std::make_unique<AshPopupAlignmentDelegate>(GetPrimaryShelf()));
  }

  void TearDown() override {
    alignment_delegate_.reset();
    AshTestBase::TearDown();
  }

 protected:
  enum Position { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, OUTSIDE };

  AshPopupAlignmentDelegate* alignment_delegate() {
    return alignment_delegate_.get();
  }

  void UpdateWorkArea(AshPopupAlignmentDelegate* alignment_delegate,
                      const display::Display& display) {
    alignment_delegate->StartObserving(display::Screen::GetScreen(), display);
    // Update the layout
    alignment_delegate->UpdateWorkArea();
  }

  void SetAlignmentDelegate(
      std::unique_ptr<AshPopupAlignmentDelegate> delegate) {
    if (!delegate.get()) {
      alignment_delegate_.reset();
      return;
    }
    alignment_delegate_ = std::move(delegate);
    UpdateWorkArea(alignment_delegate_.get(),
                   display::Screen::GetScreen()->GetPrimaryDisplay());
  }

  Position GetPositionInDisplay(const gfx::Point& point) {
    const gfx::Rect work_area =
        display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
    const gfx::Point center_point = work_area.CenterPoint();
    if (work_area.x() > point.x() || work_area.y() > point.y() ||
        work_area.right() < point.x() || work_area.bottom() < point.y()) {
      return OUTSIDE;
    }

    if (center_point.x() < point.x())
      return (center_point.y() < point.y()) ? BOTTOM_RIGHT : TOP_RIGHT;
    else
      return (center_point.y() < point.y()) ? BOTTOM_LEFT : TOP_LEFT;
  }

  gfx::Rect GetWorkArea() { return alignment_delegate_->work_area_; }

 private:
  std::unique_ptr<AshPopupAlignmentDelegate> alignment_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AshPopupAlignmentDelegateTest);
};

TEST_F(AshPopupAlignmentDelegateTest, ShelfAlignment) {
  const gfx::Rect toast_size(0, 0, 10, 10);
  UpdateDisplay("600x600");
  gfx::Point toast_point;
  toast_point.set_x(alignment_delegate()->GetToastOriginX(toast_size));
  toast_point.set_y(alignment_delegate()->GetBaseline());
  EXPECT_EQ(BOTTOM_RIGHT, GetPositionInDisplay(toast_point));
  EXPECT_FALSE(alignment_delegate()->IsTopDown());
  EXPECT_FALSE(alignment_delegate()->IsFromLeft());

  GetPrimaryShelf()->SetAlignment(SHELF_ALIGNMENT_RIGHT);
  toast_point.set_x(alignment_delegate()->GetToastOriginX(toast_size));
  toast_point.set_y(alignment_delegate()->GetBaseline());
  EXPECT_EQ(BOTTOM_RIGHT, GetPositionInDisplay(toast_point));
  EXPECT_FALSE(alignment_delegate()->IsTopDown());
  EXPECT_FALSE(alignment_delegate()->IsFromLeft());

  GetPrimaryShelf()->SetAlignment(SHELF_ALIGNMENT_LEFT);
  toast_point.set_x(alignment_delegate()->GetToastOriginX(toast_size));
  toast_point.set_y(alignment_delegate()->GetBaseline());
  EXPECT_EQ(BOTTOM_LEFT, GetPositionInDisplay(toast_point));
  EXPECT_FALSE(alignment_delegate()->IsTopDown());
  EXPECT_TRUE(alignment_delegate()->IsFromLeft());
}

TEST_F(AshPopupAlignmentDelegateTest, LockScreen) {
  const gfx::Rect toast_size(0, 0, 10, 10);

  GetPrimaryShelf()->SetAlignment(SHELF_ALIGNMENT_LEFT);
  gfx::Point toast_point;
  toast_point.set_x(alignment_delegate()->GetToastOriginX(toast_size));
  toast_point.set_y(alignment_delegate()->GetBaseline());
  EXPECT_EQ(BOTTOM_LEFT, GetPositionInDisplay(toast_point));
  EXPECT_FALSE(alignment_delegate()->IsTopDown());
  EXPECT_TRUE(alignment_delegate()->IsFromLeft());

  BlockUserSession(BLOCKED_BY_LOCK_SCREEN);
  toast_point.set_x(alignment_delegate()->GetToastOriginX(toast_size));
  toast_point.set_y(alignment_delegate()->GetBaseline());
  EXPECT_EQ(BOTTOM_RIGHT, GetPositionInDisplay(toast_point));
  EXPECT_FALSE(alignment_delegate()->IsTopDown());
  EXPECT_FALSE(alignment_delegate()->IsFromLeft());
}

TEST_F(AshPopupAlignmentDelegateTest, AutoHide) {
  const gfx::Rect toast_size(0, 0, 10, 10);
  UpdateDisplay("600x600");
  int origin_x = alignment_delegate()->GetToastOriginX(toast_size);
  int baseline = alignment_delegate()->GetBaseline();

  // Create a window, otherwise autohide doesn't work.
  std::unique_ptr<views::Widget> widget = CreateTestWidget(
      nullptr, kShellWindowId_DefaultContainer, gfx::Rect(0, 0, 50, 50));
  Shelf* shelf = GetPrimaryShelf();
  shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_ALWAYS);
  EXPECT_EQ(origin_x, alignment_delegate()->GetToastOriginX(toast_size));
  EXPECT_LT(baseline, alignment_delegate()->GetBaseline());
}

TEST_F(AshPopupAlignmentDelegateTest, DisplayResize) {
  const gfx::Rect toast_size(0, 0, 10, 10);
  UpdateDisplay("600x600");
  int origin_x = alignment_delegate()->GetToastOriginX(toast_size);
  int baseline = alignment_delegate()->GetBaseline();

  UpdateDisplay("800x800");
  EXPECT_LT(origin_x, alignment_delegate()->GetToastOriginX(toast_size));
  EXPECT_LT(baseline, alignment_delegate()->GetBaseline());

  UpdateDisplay("400x400");
  EXPECT_GT(origin_x, alignment_delegate()->GetToastOriginX(toast_size));
  EXPECT_GT(baseline, alignment_delegate()->GetBaseline());
}

TEST_F(AshPopupAlignmentDelegateTest, DockedMode) {
  const gfx::Rect toast_size(0, 0, 10, 10);
  UpdateDisplay("600x600");
  int origin_x = alignment_delegate()->GetToastOriginX(toast_size);
  int baseline = alignment_delegate()->GetBaseline();

  // Emulate the docked mode; enter to an extended mode, then invoke
  // OnNativeDisplaysChanged() with the info for the secondary display only.
  UpdateDisplay("600x600,800x800");

  std::vector<display::ManagedDisplayInfo> new_info;
  new_info.push_back(display_manager()->GetDisplayInfo(
      display_manager()->GetDisplayAt(1u).id()));
  display_manager()->OnNativeDisplaysChanged(new_info);

  EXPECT_LT(origin_x, alignment_delegate()->GetToastOriginX(toast_size));
  EXPECT_LT(baseline, alignment_delegate()->GetBaseline());
}

TEST_F(AshPopupAlignmentDelegateTest, TrayHeight) {
  const gfx::Rect toast_size(0, 0, 10, 10);
  UpdateDisplay("600x600");
  int origin_x = alignment_delegate()->GetToastOriginX(toast_size);
  int baseline = alignment_delegate()->GetBaseline();

  // Simulate the system tray bubble being open.
  const int kTrayHeight = 100;
  alignment_delegate()->SetTrayBubbleHeight(kTrayHeight);

  EXPECT_EQ(origin_x, alignment_delegate()->GetToastOriginX(toast_size));
  EXPECT_EQ(baseline - kTrayHeight - message_center::kMarginBetweenPopups,
            alignment_delegate()->GetBaseline());
}

TEST_F(AshPopupAlignmentDelegateTest, Extended) {
  UpdateDisplay("600x600,800x800");
  SetAlignmentDelegate(
      std::make_unique<AshPopupAlignmentDelegate>(GetPrimaryShelf()));

  display::Display second_display = GetSecondaryDisplay();
  Shelf* second_shelf =
      Shell::GetRootWindowControllerWithDisplayId(second_display.id())->shelf();
  AshPopupAlignmentDelegate for_2nd_display(second_shelf);
  UpdateWorkArea(&for_2nd_display, second_display);
  // Make sure that the toast position on the secondary display is
  // positioned correctly.
  EXPECT_LT(1300, for_2nd_display.GetToastOriginX(gfx::Rect(0, 0, 10, 10)));
  EXPECT_LT(700, for_2nd_display.GetBaseline());
}

TEST_F(AshPopupAlignmentDelegateTest, Unified) {
  display_manager()->SetUnifiedDesktopEnabled(true);

  // Reset the delegate as the primary display's shelf will be destroyed during
  // transition.
  SetAlignmentDelegate(nullptr);

  UpdateDisplay("600x600,800x800");
  SetAlignmentDelegate(
      std::make_unique<AshPopupAlignmentDelegate>(GetPrimaryShelf()));

  EXPECT_GT(600,
            alignment_delegate()->GetToastOriginX(gfx::Rect(0, 0, 10, 10)));
}

// Tests that when the keyboard is showing that notifications appear above it,
// and that they return to normal once the keyboard is gone.
TEST_F(AshPopupAlignmentDelegateTest, KeyboardShowing) {
  ASSERT_TRUE(keyboard::IsKeyboardEnabled());
  ASSERT_TRUE(keyboard::IsKeyboardOverscrollEnabled());

  UpdateDisplay("600x600");
  int baseline = alignment_delegate()->GetBaseline();

  Shelf* shelf = GetPrimaryShelf();
  gfx::Rect keyboard_bounds(0, 300, 600, 300);
  shelf->SetVirtualKeyboardBoundsForTesting(keyboard_bounds);
  int keyboard_baseline = alignment_delegate()->GetBaseline();
  EXPECT_NE(baseline, keyboard_baseline);
  EXPECT_GT(keyboard_bounds.y(), keyboard_baseline);

  shelf->SetVirtualKeyboardBoundsForTesting(gfx::Rect());
  EXPECT_EQ(baseline, alignment_delegate()->GetBaseline());
}

}  // namespace ash
