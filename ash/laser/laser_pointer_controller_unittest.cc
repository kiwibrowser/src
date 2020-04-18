// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/laser/laser_pointer_controller.h"

#include "ash/laser/laser_pointer_controller_test_api.h"
#include "ash/laser/laser_pointer_view.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ui/events/test/event_generator.h"

namespace ash {
namespace {

class LaserPointerControllerTest : public AshTestBase {
 public:
  LaserPointerControllerTest() = default;
  ~LaserPointerControllerTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();
    controller_.reset(new LaserPointerController());
  }

  void TearDown() override {
    // This needs to be called first to remove the event handler before the
    // shell instance gets torn down.
    controller_.reset();
    AshTestBase::TearDown();
  }

 protected:
  std::unique_ptr<LaserPointerController> controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(LaserPointerControllerTest);
};

}  // namespace

// Test to ensure the class responsible for drawing the laser pointer receives
// points from stylus movements as expected.
TEST_F(LaserPointerControllerTest, LaserPointerRenderer) {
  LaserPointerControllerTestApi controller_test_api_(controller_.get());

  // The laser pointer mode only works with stylus.
  GetEventGenerator().EnterPenPointerMode();

  // When disabled the laser pointer should not be showing.
  GetEventGenerator().MoveTouch(gfx::Point(1, 1));
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());

  // Verify that by enabling the mode, the laser pointer should still not be
  // showing.
  controller_test_api_.SetEnabled(true);
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());

  // Verify moving the stylus 4 times will not display the laser pointer.
  GetEventGenerator().MoveTouch(gfx::Point(2, 2));
  GetEventGenerator().MoveTouch(gfx::Point(3, 3));
  GetEventGenerator().MoveTouch(gfx::Point(4, 4));
  GetEventGenerator().MoveTouch(gfx::Point(5, 5));
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());

  // Verify pressing the stylus will show the laser pointer and add a point but
  // will not activate fading out.
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingLaserPointer());
  EXPECT_FALSE(controller_test_api_.IsFadingAway());
  EXPECT_EQ(1, controller_test_api_.laser_points().GetNumberOfPoints());

  // Verify dragging the stylus 2 times will add 2 more points.
  GetEventGenerator().MoveTouch(gfx::Point(6, 6));
  GetEventGenerator().MoveTouch(gfx::Point(7, 7));
  EXPECT_EQ(3, controller_test_api_.laser_points().GetNumberOfPoints());

  // Verify releasing the stylus still shows the laser pointer, which is fading
  // away.
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingLaserPointer());
  EXPECT_TRUE(controller_test_api_.IsFadingAway());

  // Verify that disabling the mode does not display the laser pointer.
  controller_test_api_.SetEnabled(false);
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());
  EXPECT_FALSE(controller_test_api_.IsFadingAway());

  // Verify that disabling the mode while laser pointer is displayed does not
  // display the laser pointer.
  controller_test_api_.SetEnabled(true);
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(6, 6));
  EXPECT_TRUE(controller_test_api_.IsShowingLaserPointer());
  controller_test_api_.SetEnabled(false);
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());

  // Verify that the laser pointer does not add points while disabled.
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(8, 8));
  GetEventGenerator().ReleaseTouch();
  GetEventGenerator().MoveTouch(gfx::Point(9, 9));
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());

  // Verify that the laser pointer does not get shown if points are not coming
  // from the stylus, even when enabled.
  GetEventGenerator().ExitPenPointerMode();
  controller_test_api_.SetEnabled(true);
  GetEventGenerator().PressTouch();
  GetEventGenerator().MoveTouch(gfx::Point(10, 10));
  GetEventGenerator().MoveTouch(gfx::Point(11, 11));
  EXPECT_FALSE(controller_test_api_.IsShowingLaserPointer());
  GetEventGenerator().ReleaseTouch();
}

// Test to ensure the class responsible for drawing the laser pointer handles
// prediction as expected when it receives points from stylus movements.
TEST_F(LaserPointerControllerTest, LaserPointerPrediction) {
  LaserPointerControllerTestApi controller_test_api_(controller_.get());

  controller_test_api_.SetEnabled(true);
  // The laser pointer mode only works with stylus.
  GetEventGenerator().EnterPenPointerMode();
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingLaserPointer());

  EXPECT_EQ(1, controller_test_api_.laser_points().GetNumberOfPoints());
  // Initial press event shouldn't generate any predicted points as there's no
  // history to use for prediction.
  EXPECT_EQ(0,
            controller_test_api_.predicted_laser_points().GetNumberOfPoints());

  // Verify dragging the stylus 3 times will add some predicted points.
  GetEventGenerator().MoveTouch(gfx::Point(10, 10));
  GetEventGenerator().MoveTouch(gfx::Point(20, 20));
  GetEventGenerator().MoveTouch(gfx::Point(30, 30));
  EXPECT_NE(0,
            controller_test_api_.predicted_laser_points().GetNumberOfPoints());
  // Verify predicted points are in the right direction.
  for (const auto& point :
       controller_test_api_.predicted_laser_points().points()) {
    EXPECT_LT(30, point.location.x());
    EXPECT_LT(30, point.location.y());
  }

  // Verify releasing the stylus removes predicted points.
  GetEventGenerator().ReleaseTouch();
  EXPECT_TRUE(controller_test_api_.IsShowingLaserPointer());
  EXPECT_TRUE(controller_test_api_.IsFadingAway());
  EXPECT_EQ(0,
            controller_test_api_.predicted_laser_points().GetNumberOfPoints());
}

}  // namespace ash
