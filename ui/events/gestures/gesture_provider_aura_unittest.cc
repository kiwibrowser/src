// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gestures/gesture_provider_aura.h"

#include <memory>

#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_utils.h"

namespace ui {

class GestureProviderAuraTest : public testing::Test,
                                public GestureProviderAuraClient {
 public:
  GestureProviderAuraTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  ~GestureProviderAuraTest() override {}

  void OnGestureEvent(GestureConsumer* raw_input_consumer,
                      GestureEvent* event) override {}

  void SetUp() override {
    consumer_.reset(new GestureConsumer());
    provider_.reset(new GestureProviderAura(consumer_.get(), this));
  }

  void TearDown() override { provider_.reset(); }

  GestureProviderAura* provider() { return provider_.get(); }

 private:
  std::unique_ptr<GestureConsumer> consumer_;
  std::unique_ptr<GestureProviderAura> provider_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(GestureProviderAuraTest, IgnoresExtraPressEvents) {
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press1(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&press1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent press2(
      ET_TOUCH_PRESSED, gfx::Point(30, 40), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&press2));
}

TEST_F(GestureProviderAuraTest, IgnoresExtraMoveOrReleaseEvents) {
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press1(
      ET_TOUCH_PRESSED, gfx::Point(10, 10), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&press1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent release1(
      ET_TOUCH_RELEASED, gfx::Point(30, 40), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_TRUE(provider()->OnTouchEvent(&release1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent release2(
      ET_TOUCH_RELEASED, gfx::Point(30, 45), time,
      PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&release1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move1(ET_TOUCH_MOVED, gfx::Point(70, 75), time,
                   PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));
  EXPECT_FALSE(provider()->OnTouchEvent(&move1));
}

TEST_F(GestureProviderAuraTest, IgnoresIdenticalMoveEvents) {
  const float kRadiusX = 20.f;
  const float kRadiusY = 30.f;
  const float kAngle = 0.321f;
  const float kForce = 40.f;
  const int kTouchId0 = 5;
  const int kTouchId1 = 3;

  PointerDetails pointerDetails1(EventPointerType::POINTER_TYPE_TOUCH,
                                 kTouchId0);
  base::TimeTicks time = ui::EventTimeForNow();
  TouchEvent press0_1(ET_TOUCH_PRESSED, gfx::Point(9, 10), time,
                      pointerDetails1);
  EXPECT_TRUE(provider()->OnTouchEvent(&press0_1));

  PointerDetails pointerDetails2(EventPointerType::POINTER_TYPE_TOUCH,
                                 kTouchId1);
  TouchEvent press1_1(ET_TOUCH_PRESSED, gfx::Point(40, 40), time,
                      pointerDetails2);
  EXPECT_TRUE(provider()->OnTouchEvent(&press1_1));

  time += base::TimeDelta::FromMilliseconds(10);
  pointerDetails1 = PointerDetails(EventPointerType::POINTER_TYPE_TOUCH,
                                   kTouchId0, kRadiusX, kRadiusY, kForce);
  TouchEvent move0_1(ET_TOUCH_MOVED, gfx::Point(10, 10), time, pointerDetails1,
                     0, kAngle);
  EXPECT_TRUE(provider()->OnTouchEvent(&move0_1));

  pointerDetails2 = PointerDetails(EventPointerType::POINTER_TYPE_TOUCH,
                                   kTouchId1, kRadiusX, kRadiusY, kForce);
  TouchEvent move1_1(ET_TOUCH_MOVED, gfx::Point(100, 200), time,
                     pointerDetails2, 0, kAngle);
  EXPECT_TRUE(provider()->OnTouchEvent(&move1_1));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move0_2(ET_TOUCH_MOVED, gfx::Point(10, 10), time, pointerDetails1,
                     0, kAngle);
  // Nothing has changed, so ignore the move.
  EXPECT_FALSE(provider()->OnTouchEvent(&move0_2));

  TouchEvent move1_2(ET_TOUCH_MOVED, gfx::Point(100, 200), time,
                     pointerDetails2, 0, kAngle);
  // Nothing has changed, so ignore the move.
  EXPECT_FALSE(provider()->OnTouchEvent(&move1_2));

  time += base::TimeDelta::FromMilliseconds(10);
  TouchEvent move0_3(ET_TOUCH_MOVED, gfx::Point(), time, pointerDetails1, 0,
                     kAngle);
  move0_3.set_location_f(gfx::PointF(70, 75.1f));
  move0_3.set_root_location_f(gfx::PointF(70, 75.1f));
  // Position has changed, so don't ignore the move.
  EXPECT_TRUE(provider()->OnTouchEvent(&move0_3));

  time += base::TimeDelta::FromMilliseconds(10);
  pointerDetails2.radius_y += 1;
  TouchEvent move0_4(ET_TOUCH_MOVED, gfx::Point(), time, pointerDetails2, 0,
                     kAngle);
  move0_4.set_location_f(gfx::PointF(70, 75.1f));
  move0_4.set_root_location_f(gfx::PointF(70, 75.1f));
}

// TODO(jdduke): Test whether event marked as scroll trigger.

}  // namespace ui
