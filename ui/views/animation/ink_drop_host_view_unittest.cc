// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/animation/ink_drop_host_view.h"

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_handler.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/test/ink_drop_host_view_test_api.h"

namespace views {
namespace test {
using InkDropMode = InkDropHostViewTestApi::InkDropMode;

class TestInkDropHostView : public InkDropHostView {
 public:
  TestInkDropHostView() {}

  // Accessors to InkDropHostView internals.
  ui::EventHandler* GetTargetHandler() { return target_handler(); }

  int on_ink_drop_created_count() const { return on_ink_drop_created_count_; }

 protected:
  SkColor GetInkDropBaseColor() const override {
    return gfx::kPlaceholderColor;
  }

  // InkDropHostView:
  void OnInkDropCreated() override { ++on_ink_drop_created_count_; }

 private:
  int on_ink_drop_created_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestInkDropHostView);
};

class InkDropHostViewTest : public testing::Test {
 public:
  InkDropHostViewTest();
  ~InkDropHostViewTest() override;

 protected:
  // Test target.
  TestInkDropHostView host_view_;

  // Provides internal access to |host_view_| test target.
  InkDropHostViewTestApi test_api_;

  std::unique_ptr<base::AutoReset<gfx::Animation::RichAnimationRenderMode>>
      animation_mode_reset_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InkDropHostViewTest);
};

InkDropHostViewTest::InkDropHostViewTest()
    : test_api_(&host_view_),
      animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED)) {}

InkDropHostViewTest::~InkDropHostViewTest() {}

// Verifies the return value of GetInkDropCenterBasedOnLastEvent() for a null
// Event.
TEST_F(InkDropHostViewTest, GetInkDropCenterBasedOnLastEventForNullEvent) {
  host_view_.SetSize(gfx::Size(20, 20));
  test_api_.AnimateInkDrop(InkDropState::ACTION_PENDING, nullptr);
  EXPECT_EQ(gfx::Point(10, 10), test_api_.GetInkDropCenterBasedOnLastEvent());
}

// Verifies the return value of GetInkDropCenterBasedOnLastEvent() for a located
// Event.
TEST_F(InkDropHostViewTest, GetInkDropCenterBasedOnLastEventForLocatedEvent) {
  host_view_.SetSize(gfx::Size(20, 20));

  ui::MouseEvent located_event(ui::ET_MOUSE_PRESSED, gfx::Point(5, 6),
                               gfx::Point(5, 6), ui::EventTimeForNow(),
                               ui::EF_LEFT_MOUSE_BUTTON, 0);

  test_api_.AnimateInkDrop(InkDropState::ACTION_PENDING, &located_event);
  EXPECT_EQ(gfx::Point(5, 6), test_api_.GetInkDropCenterBasedOnLastEvent());
}

TEST_F(InkDropHostViewTest, HasInkDrop) {
  EXPECT_FALSE(test_api_.HasInkDrop());

  test_api_.GetInkDrop();
  EXPECT_TRUE(test_api_.HasInkDrop());

  test_api_.SetInkDropMode(InkDropMode::OFF);
  EXPECT_FALSE(test_api_.HasInkDrop());
}

TEST_F(InkDropHostViewTest, OnInkDropCreatedOnlyNotfiedOnCreation) {
  EXPECT_EQ(0, host_view_.on_ink_drop_created_count());

  test_api_.GetInkDrop();
  EXPECT_EQ(1, host_view_.on_ink_drop_created_count());

  test_api_.GetInkDrop();
  EXPECT_EQ(1, host_view_.on_ink_drop_created_count());

  test_api_.SetInkDropMode(InkDropMode::OFF);
  test_api_.SetInkDropMode(InkDropMode::ON);
  EXPECT_EQ(1, host_view_.on_ink_drop_created_count());

  test_api_.GetInkDrop();
  EXPECT_EQ(2, host_view_.on_ink_drop_created_count());
}

// Verifies that SetInkDropMode() sets up gesture handling properly.
TEST_F(InkDropHostViewTest, SetInkDropModeGestureHandler) {
  EXPECT_FALSE(test_api_.HasGestureHandler());

  test_api_.SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);
  EXPECT_FALSE(test_api_.HasGestureHandler());

  test_api_.SetInkDropMode(InkDropMode::ON);
  EXPECT_TRUE(test_api_.HasGestureHandler());

  // Enabling gesture handler the second time should just work (no crash).
  test_api_.SetInkDropMode(InkDropMode::ON);
  EXPECT_TRUE(test_api_.HasGestureHandler());

  test_api_.SetInkDropMode(InkDropMode::OFF);
  EXPECT_FALSE(test_api_.HasGestureHandler());
}

// Verifies that ink drops are not shown when the host is disabled.
TEST_F(InkDropHostViewTest,
       GestureEventsDontTriggerInkDropsWhenHostIsDisabled) {
  test_api_.SetInkDropMode(InkDropMode::ON);
  host_view_.SetEnabled(false);

  ui::GestureEvent gesture_event(
      0.f, 0.f, 0, ui::EventTimeForNow(),
      ui::GestureEventDetails(ui::ET_GESTURE_TAP_DOWN));

  host_view_.GetTargetHandler()->OnEvent(&gesture_event);

  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);
}

#if defined(OS_WIN)
TEST_F(InkDropHostViewTest, NoInkDropOnTouchOrGestureEvents) {
  host_view_.SetSize(gfx::Size(20, 20));

  test_api_.SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);

  // Ensure the target ink drop is in the expected state.
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);

  ui::TouchEvent touch_event(
      ui::ET_TOUCH_PRESSED, gfx::Point(5, 6), ui::EventTimeForNow(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1));

  test_api_.AnimateInkDrop(InkDropState::ACTION_PENDING, &touch_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);

  test_api_.AnimateInkDrop(InkDropState::ALTERNATE_ACTION_PENDING,
                           &touch_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);

  ui::GestureEvent gesture_event(5.0f, 6.0f, 0, ui::EventTimeForNow(),
                                 ui::GestureEventDetails(ui::ET_GESTURE_TAP));

  test_api_.AnimateInkDrop(InkDropState::ACTION_PENDING, &gesture_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);

  test_api_.AnimateInkDrop(InkDropState::ALTERNATE_ACTION_PENDING,
                           &gesture_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);
}

TEST_F(InkDropHostViewTest, DismissInkDropOnTouchOrGestureEvents) {
  // TODO(bruthig): Re-enable! For some reason these tests fail on some win
  // trunk builds. See crbug.com/731811.
  if (!gfx::Animation::ShouldRenderRichAnimation())
    return;

  host_view_.SetSize(gfx::Size(20, 20));

  test_api_.SetInkDropMode(InkDropMode::ON_NO_GESTURE_HANDLER);

  // Ensure the target ink drop is in the expected state.
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::HIDDEN);

  ui::MouseEvent mouse_event(ui::ET_MOUSE_PRESSED, gfx::Point(5, 6),
                             gfx::Point(5, 6), ui::EventTimeForNow(),
                             ui::EF_LEFT_MOUSE_BUTTON, 0);

  test_api_.AnimateInkDrop(InkDropState::ACTION_PENDING, &mouse_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::ACTION_PENDING);

  ui::TouchEvent touch_event(
      ui::ET_TOUCH_PRESSED, gfx::Point(5, 6), ui::EventTimeForNow(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1));

  test_api_.AnimateInkDrop(InkDropState::ACTION_TRIGGERED, &touch_event);
  EXPECT_EQ(test_api_.GetInkDrop()->GetTargetInkDropState(),
            InkDropState::ACTION_TRIGGERED);
}
#endif

}  // namespace test
}  // namespace views
