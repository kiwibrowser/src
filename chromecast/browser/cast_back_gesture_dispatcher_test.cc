// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_back_gesture_dispatcher.h"

#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// Gmock matchers and actions that we use below.
using testing::AnyOf;
using testing::Eq;
using testing::Return;
using testing::SetArgPointee;
using testing::WithArg;
using testing::_;

namespace chromecast {
namespace shell {

class MockCastContentWindowDelegate : public CastContentWindow::Delegate {
 public:
  ~MockCastContentWindowDelegate() override = default;

  MOCK_METHOD1(CanHandleGesture, bool(GestureType gesture_type));
  MOCK_METHOD1(ConsumeGesture, bool(GestureType gesture_type));
  std::string GetId() override { return "mockContentWindowDelegate"; }
};

// Verify the simple case of a left swipe with the right horizontal leads to
// back.
TEST(CastBackGestureDispatcherTest, VerifySimpleDispatchSuccess) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  EXPECT_CALL(delegate, CanHandleGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  EXPECT_CALL(delegate, ConsumeGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::LEFT);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::LEFT, gfx::Point(5, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(90, 50));
}

// Verify that multiple 'continue' events still only lead to one back
// invocation.
TEST(CastBackGestureDispatcherTest, VerifyOnlySingleDispatch) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  EXPECT_CALL(delegate, CanHandleGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  EXPECT_CALL(delegate, ConsumeGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::LEFT);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::LEFT, gfx::Point(5, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(90, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(105, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(200, 50));
}

// Verify that if the delegate says it doesn't handle back that we won't try to
// ask them to consume it.
TEST(CastBackGestureDispatcherTest, VerifyDelegateDoesNotConsumeUnwanted) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  EXPECT_CALL(delegate, CanHandleGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(false));
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::LEFT);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::LEFT, gfx::Point(5, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(90, 50));
}

// Verify that a not-left gesture doesn't lead to a swipe.
TEST(CastBackGestureDispatcherTest, VerifyNotLeftSwipeIsNotBack) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::TOP);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::TOP, gfx::Point(0, 5));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::TOP,
                                     gfx::Point(0, 90));
}

// Verify that if the gesture doesn't go far enough horizontally that we will
// not consider it a swipe.
TEST(CastBackGestureDispatcherTest, VerifyNotFarEnoughRightIsNotBack) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  EXPECT_CALL(delegate, CanHandleGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::LEFT);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::LEFT, gfx::Point(5, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(70, 50));
}

// Verify that if the gesture ends before going far enough, that's also not a
// swipe.
TEST(CastBackGestureDispatcherTest, VerifyNotFarEnoughRightAndEndIsNotBack) {
  MockCastContentWindowDelegate delegate;
  CastBackGestureDispatcher dispatcher(&delegate);
  EXPECT_CALL(delegate, CanHandleGesture(Eq(GestureType::GO_BACK)))
      .WillOnce(Return(true));
  dispatcher.CanHandleSwipe(CastSideSwipeOrigin::LEFT);
  dispatcher.HandleSideSwipeBegin(CastSideSwipeOrigin::LEFT, gfx::Point(5, 50));
  dispatcher.HandleSideSwipeContinue(CastSideSwipeOrigin::LEFT,
                                     gfx::Point(70, 50));
  dispatcher.HandleSideSwipeEnd(CastSideSwipeOrigin::LEFT, gfx::Point(75, 50));
}

}  // namespace shell
}  // namespace chromecast
