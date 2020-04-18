// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/cast_system_gesture_event_handler.h"

#include <memory>

#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/event_generator_delegate_aura.h"
#include "ui/aura/window.h"
#include "ui/events/test/event_generator.h"
#include "ui/wm/core/default_screen_position_client.h"

namespace chromecast {
namespace test {

namespace {

constexpr base::TimeDelta kTimeDelay = base::TimeDelta::FromMilliseconds(100);
constexpr int kSwipeDistance = 50;
constexpr int kNumSteps = 5;
constexpr gfx::Point kZeroPoint{0, 0};

}  // namespace

class TestEventGeneratorDelegate
    : public aura::test::EventGeneratorDelegateAura {
 public:
  explicit TestEventGeneratorDelegate(aura::Window* root_window)
      : root_window_(root_window) {}
  ~TestEventGeneratorDelegate() override = default;

  // EventGeneratorDelegateAura overrides:
  aura::WindowTreeHost* GetHostAt(const gfx::Point& point) const override {
    return root_window_->GetHost();
  }

  aura::client::ScreenPositionClient* GetScreenPositionClient(
      const aura::Window* window) const override {
    return aura::client::GetScreenPositionClient(root_window_);
  }

 private:
  aura::Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(TestEventGeneratorDelegate);
};

class TestSideSwipeGestureHandler
    : public CastSideSwipeGestureHandlerInterface {
 public:
  TestSideSwipeGestureHandler()
      : begin_swipe_point_(kZeroPoint), end_swipe_point_(kZeroPoint) {}

  ~TestSideSwipeGestureHandler() override = default;

  bool CanHandleSwipe(CastSideSwipeOrigin swipe_origin) override {
    return handle_swipe_;
  }

  void HandleSideSwipeBegin(CastSideSwipeOrigin swipe_origin,
                            const gfx::Point& touch_location) override {
    if (handle_swipe_) {
      begin_swipe_origin_ = swipe_origin;
      begin_swipe_point_ = touch_location;
    }
  }

  void HandleSideSwipeEnd(CastSideSwipeOrigin swipe_origin,
                          const gfx::Point& gesture_event) override {
    end_swipe_origin_ = swipe_origin;
    end_swipe_point_ = gesture_event;
  }

  void SetHandleSwipe(bool handle_swipe) { handle_swipe_ = handle_swipe; }

  CastSideSwipeOrigin begin_swipe_origin() const { return begin_swipe_origin_; }
  gfx::Point begin_swipe_point() const { return begin_swipe_point_; }

  CastSideSwipeOrigin end_swipe_origin() const { return end_swipe_origin_; }
  gfx::Point end_swipe_point() const { return end_swipe_point_; }

 private:
  bool handle_swipe_ = true;

  CastSideSwipeOrigin begin_swipe_origin_ = CastSideSwipeOrigin::NONE;
  gfx::Point begin_swipe_point_;

  CastSideSwipeOrigin end_swipe_origin_ = CastSideSwipeOrigin::NONE;
  gfx::Point end_swipe_point_;
};

// Event sink to check for events that get through (or don't get through) after
// the system gesture handler handles them.
class TestEventHandler : public ui::EventHandler {
 public:
  TestEventHandler() : EventHandler(), num_touch_events_received_(0) {}

  void OnTouchEvent(ui::TouchEvent* event) override {
    num_touch_events_received_++;
  }

  int NumTouchEventsReceived() const { return num_touch_events_received_; }

 private:
  int num_touch_events_received_;
};

class CastSystemGestureEventHandlerTest : public aura::test::AuraTestBase {
 public:
  ~CastSystemGestureEventHandlerTest() override = default;

  void SetUp() override {
    aura::test::AuraTestBase::SetUp();

    screen_position_client_.reset(new wm::DefaultScreenPositionClient());
    aura::client::SetScreenPositionClient(root_window(),
                                          screen_position_client_.get());

    gesture_event_handler_ =
        std::make_unique<CastSystemGestureEventHandler>(root_window());
    gesture_handler_ = std::make_unique<TestSideSwipeGestureHandler>();
    gesture_event_handler_->AddSideSwipeGestureHandler(gesture_handler_.get());
    test_event_handler_ = std::make_unique<TestEventHandler>();
    root_window()->AddPostTargetHandler(test_event_handler_.get());
  }

  void TearDown() override {
    gesture_event_handler_->RemoveSideSwipeGestureHandler(
        gesture_handler_.get());
    gesture_event_handler_.reset();
    gesture_handler_.reset();

    aura::test::AuraTestBase::TearDown();
  }

  ui::test::EventGenerator& GetEventGenerator() {
    if (!event_generator_) {
      event_generator_.reset(new ui::test::EventGenerator(
          new TestEventGeneratorDelegate(root_window())));
    }
    return *event_generator_.get();
  }

  TestSideSwipeGestureHandler& test_gesture_handler() {
    return *gesture_handler_;
  }

  TestEventHandler& test_event_handler() { return *test_event_handler_; }

 private:
  std::unique_ptr<aura::client::ScreenPositionClient> screen_position_client_;
  std::unique_ptr<ui::test::EventGenerator> event_generator_;

  std::unique_ptr<CastSystemGestureEventHandler> gesture_event_handler_;
  std::unique_ptr<TestEventHandler> test_event_handler_;
  std::unique_ptr<TestSideSwipeGestureHandler> gesture_handler_;
};

// Test that initialization works and initial state is clean.
TEST_F(CastSystemGestureEventHandlerTest, Initialization) {
  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().end_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_EQ(0, test_event_handler().NumTouchEventsReceived());
}

// A swipe in the middle of the screen should produce no system gesture.
TEST_F(CastSystemGestureEventHandlerTest, SwipeWithNoSystemGesture) {
  gfx::Point drag_point(root_window()->bounds().width() / 2,
                        root_window()->bounds().height() / 2);
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point - gfx::Vector2d(0, kSwipeDistance),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().end_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_NE(0, test_event_handler().NumTouchEventsReceived());
}

TEST_F(CastSystemGestureEventHandlerTest, SwipeFromLeft) {
  gfx::Point drag_point(0, root_window()->bounds().height() / 2);
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point + gfx::Vector2d(kSwipeDistance, 0),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::LEFT,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::LEFT,
            test_gesture_handler().end_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_EQ(0, test_event_handler().NumTouchEventsReceived());
}

TEST_F(CastSystemGestureEventHandlerTest, SwipeFromRight) {
  gfx::Point drag_point(root_window()->bounds().width(),
                        root_window()->bounds().height() / 2);
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point - gfx::Vector2d(kSwipeDistance, 0),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::RIGHT,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::RIGHT,
            test_gesture_handler().end_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_EQ(0, test_event_handler().NumTouchEventsReceived());
}

TEST_F(CastSystemGestureEventHandlerTest, SwipeFromTop) {
  gfx::Point drag_point(root_window()->bounds().width() / 2, 0);
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point + gfx::Vector2d(0, kSwipeDistance),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::TOP,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::TOP,
            test_gesture_handler().end_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_EQ(0, test_event_handler().NumTouchEventsReceived());
}

TEST_F(CastSystemGestureEventHandlerTest, SwipeFromBottom) {
  gfx::Point drag_point(root_window()->bounds().width() / 2,
                        root_window()->bounds().height());
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point - gfx::Vector2d(0, kSwipeDistance),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::BOTTOM,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::BOTTOM,
            test_gesture_handler().end_swipe_origin());
  EXPECT_NE(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_EQ(0, test_event_handler().NumTouchEventsReceived());
}

// Test that ignoring the gesture at its beginning will make it so the swipe
// is not produced at the end.
TEST_F(CastSystemGestureEventHandlerTest, SwipeUnhandledIgnored) {
  test_gesture_handler().SetHandleSwipe(false);

  gfx::Point drag_point(root_window()->bounds().width() / 2,
                        root_window()->bounds().height());
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.GestureScrollSequence(drag_point,
                                  drag_point - gfx::Vector2d(0, kSwipeDistance),
                                  kTimeDelay, kNumSteps);
  RunAllPendingInMessageLoop();

  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().begin_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().begin_swipe_point());
  EXPECT_EQ(CastSideSwipeOrigin::NONE,
            test_gesture_handler().end_swipe_origin());
  EXPECT_EQ(kZeroPoint, test_gesture_handler().end_swipe_point());
  EXPECT_NE(0, test_event_handler().NumTouchEventsReceived());
}

}  // namespace test
}  // namespace chromecast
