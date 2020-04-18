/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/animation/animation_clock.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class AnimationAnimationClockTest : public testing::Test {
 public:
  AnimationAnimationClockTest() : animation_clock(MockTimeFunction) {}

 protected:
  void SetUp() override {
    mock_time_ = 0;
    animation_clock.ResetTimeForTesting();
  }

  static base::TimeTicks MockTimeFunction() {
    return base::TimeTicks() + base::TimeDelta::FromSecondsD(mock_time_);
  }

  static double mock_time_;
  AnimationClock animation_clock;
};

double AnimationAnimationClockTest::mock_time_;

TEST_F(AnimationAnimationClockTest, TimeIsGreaterThanZeroForUnitTests) {
  AnimationClock clock;
  // unit tests outside core/animation shouldn't need to do anything to get
  // a non-zero currentTime().
  EXPECT_GT(clock.CurrentTime(), 0);
}

TEST_F(AnimationAnimationClockTest, TimeDoesNotChange) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());
  EXPECT_EQ(100, animation_clock.CurrentTime());
}

TEST_F(AnimationAnimationClockTest, TimeAdvancesWhenUpdated) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(200));
  EXPECT_EQ(200, animation_clock.CurrentTime());
}

TEST_F(AnimationAnimationClockTest, TimeAdvancesToTaskTime) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  mock_time_ = 150;
  AnimationClock::NotifyTaskStart();
  EXPECT_GE(animation_clock.CurrentTime(), mock_time_);
}

TEST_F(AnimationAnimationClockTest, TimeAdvancesToTaskTimeOnlyWhenRequired) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  AnimationClock::NotifyTaskStart();
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(125));
  EXPECT_EQ(125, animation_clock.CurrentTime());
}

TEST_F(AnimationAnimationClockTest, UpdateTimeIsMonotonic) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  // Update can't go backwards.
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(50));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  mock_time_ = 50;
  AnimationClock::NotifyTaskStart();
  EXPECT_EQ(100, animation_clock.CurrentTime());

  mock_time_ = 150;
  AnimationClock::NotifyTaskStart();
  EXPECT_GE(animation_clock.CurrentTime(), mock_time_);

  // Update can't go backwards after advance to estimate.
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_GE(animation_clock.CurrentTime(), mock_time_);
}

TEST_F(AnimationAnimationClockTest, CurrentTimeUpdatesTask) {
  animation_clock.UpdateTime(base::TimeTicks() +
                             base::TimeDelta::FromSeconds(100));
  EXPECT_EQ(100, animation_clock.CurrentTime());

  mock_time_ = 100;
  AnimationClock::NotifyTaskStart();
  EXPECT_EQ(100, animation_clock.CurrentTime());

  mock_time_ = 150;
  EXPECT_EQ(100, animation_clock.CurrentTime());
}

}  // namespace blink
