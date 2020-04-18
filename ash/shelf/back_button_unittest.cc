// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/back_button.h"

#include <memory>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shelf/shelf_view.h"
#include "ash/shelf/shelf_view_test_api.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/test/event_generator.h"

namespace ash {

namespace {

class TestTarget : public ui::AcceleratorTarget {
 public:
  TestTarget() = default;
  ~TestTarget() override = default;

  size_t count() const { return count_; }

  // ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override {
    ++count_;
    return true;
  }

  bool CanHandleAccelerators() const override { return true; }

 private:
  size_t count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTarget);
};

}  // namespace

class BackButtonTest : public AshTestBase {
 public:
  BackButtonTest() = default;
  ~BackButtonTest() override = default;

  BackButton* back_button() { return back_button_; }
  ShelfViewTestAPI* test_api() { return test_api_.get(); }

  void SetUp() override {
    AshTestBase::SetUp();
    test_api_ = std::make_unique<ShelfViewTestAPI>(
        GetPrimaryShelf()->GetShelfViewForTesting());
    back_button_ = test_api_->shelf_view()->GetBackButton();

    // Finish all setup tasks. In particular we want to finish the
    // GetSwitchStates post task in (Fake)PowerManagerClient which is triggered
    // by TabletModeController otherwise this will cause tablet mode to exit
    // while we wait for animations in the test.
    RunAllPendingInMessageLoop();
  }

 private:
  BackButton* back_button_ = nullptr;
  std::unique_ptr<ShelfViewTestAPI> test_api_;

  DISALLOW_COPY_AND_ASSIGN(BackButtonTest);
};

// Verify that the back button is visible in tablet mode.
TEST_F(BackButtonTest, Visibility) {
  ASSERT_TRUE(back_button()->layer());
  EXPECT_EQ(0.f, back_button()->layer()->opacity());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_EQ(1.f, back_button()->layer()->opacity());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_EQ(0.f, back_button()->layer()->opacity());
}

// Verify that the back button is visible in tablet mode, if the initial shelf
// alignment is on the left or right.
TEST_F(BackButtonTest, VisibilityWithVerticalShelf) {
  test_api()->shelf_view()->shelf()->SetAlignment(SHELF_ALIGNMENT_LEFT);
  ASSERT_TRUE(back_button()->layer());
  EXPECT_EQ(0.f, back_button()->layer()->opacity());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_EQ(1.f, back_button()->layer()->opacity());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  test_api()->RunMessageLoopUntilAnimationsDone();
  EXPECT_EQ(0.f, back_button()->layer()->opacity());
}

TEST_F(BackButtonTest, BackKeySequenceGenerated) {
  // Enter tablet mode; the back button is not visible in non tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  test_api()->RunMessageLoopUntilAnimationsDone();

  AcceleratorController* controller = Shell::Get()->accelerator_controller();

  // Register an accelerator that looks for back presses.
  ui::Accelerator accelerator_back_press(ui::VKEY_BROWSER_BACK, ui::EF_NONE);
  accelerator_back_press.set_key_state(ui::Accelerator::KeyState::PRESSED);
  TestTarget target_back_press;
  controller->Register({accelerator_back_press}, &target_back_press);

  // Register an accelerator that looks for back releases.
  ui::Accelerator accelerator_back_release(ui::VKEY_BROWSER_BACK, ui::EF_NONE);
  accelerator_back_release.set_key_state(ui::Accelerator::KeyState::RELEASED);
  TestTarget target_back_release;
  controller->Register({accelerator_back_release}, &target_back_release);

  // Verify that by clicking the back button, a back key sequence will be
  // generated.
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(back_button()->GetBoundsInScreen().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_EQ(1u, target_back_press.count());
  EXPECT_EQ(1u, target_back_release.count());
}

}  // namespace ash
