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

#include "third_party/blink/renderer/core/animation/animation_effect.h"
#include "third_party/blink/renderer/core/animation/timing_calculations.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(AnimationTimingCalculationsTest, ActiveTime) {
  Timing timing;

  // calculateActiveTime(
  //     activeDuration, fillMode, localTime, parentPhase, phase, timing)

  // Before Phase
  timing.start_delay = 10;
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::FORWARDS, 0, AnimationEffect::kPhaseActive,
      AnimationEffect::kPhaseBefore, timing)));
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::NONE, 0, AnimationEffect::kPhaseActive,
      AnimationEffect::kPhaseBefore, timing)));
  EXPECT_EQ(0, CalculateActiveTime(20, Timing::FillMode::BACKWARDS, 0,
                                   AnimationEffect::kPhaseActive,
                                   AnimationEffect::kPhaseBefore, timing));
  EXPECT_EQ(0, CalculateActiveTime(20, Timing::FillMode::BOTH, 0,
                                   AnimationEffect::kPhaseActive,
                                   AnimationEffect::kPhaseBefore, timing));

  // Active Phase
  timing.start_delay = 10;
  // Active, and parent Before
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::NONE, 15, AnimationEffect::kPhaseBefore,
      AnimationEffect::kPhaseActive, timing)));
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::FORWARDS, 15, AnimationEffect::kPhaseBefore,
      AnimationEffect::kPhaseActive, timing)));
  // Active, and parent After
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::NONE, 15, AnimationEffect::kPhaseAfter,
      AnimationEffect::kPhaseActive, timing)));
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      20, Timing::FillMode::BACKWARDS, 15, AnimationEffect::kPhaseAfter,
      AnimationEffect::kPhaseActive, timing)));
  // Active, and parent Active
  EXPECT_EQ(5, CalculateActiveTime(20, Timing::FillMode::FORWARDS, 15,
                                   AnimationEffect::kPhaseActive,
                                   AnimationEffect::kPhaseActive, timing));

  // After Phase
  timing.start_delay = 10;
  EXPECT_EQ(21, CalculateActiveTime(21, Timing::FillMode::FORWARDS, 45,
                                    AnimationEffect::kPhaseActive,
                                    AnimationEffect::kPhaseAfter, timing));
  EXPECT_EQ(21, CalculateActiveTime(21, Timing::FillMode::BOTH, 45,
                                    AnimationEffect::kPhaseActive,
                                    AnimationEffect::kPhaseAfter, timing));
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      21, Timing::FillMode::BACKWARDS, 45, AnimationEffect::kPhaseActive,
      AnimationEffect::kPhaseAfter, timing)));
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      21, Timing::FillMode::NONE, 45, AnimationEffect::kPhaseActive,
      AnimationEffect::kPhaseAfter, timing)));

  // None
  EXPECT_TRUE(IsNull(CalculateActiveTime(
      32, Timing::FillMode::NONE, NullValue(), AnimationEffect::kPhaseNone,
      AnimationEffect::kPhaseNone, timing)));
}

TEST(AnimationTimingCalculationsTest, ScaledActiveTime) {
  Timing timing;

  // calculateScaledActiveTime(activeDuration, activeTime, startOffset, timing)

  // if the active time is null
  EXPECT_TRUE(IsNull(CalculateScaledActiveTime(4, NullValue(), 5, timing)));

  // if the playback rate is negative
  timing.playback_rate = -1;
  EXPECT_EQ(35, CalculateScaledActiveTime(40, 10, 5, timing));

  // otherwise
  timing.playback_rate = 0;
  EXPECT_EQ(5, CalculateScaledActiveTime(40, 10, 5, timing));
  timing.playback_rate = 1;
  EXPECT_EQ(15, CalculateScaledActiveTime(40, 10, 5, timing));

  // infinte activeTime
  timing.playback_rate = 0;
  EXPECT_EQ(0, CalculateScaledActiveTime(
                   std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(), 0, timing));
  timing.playback_rate = 1;
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            CalculateScaledActiveTime(std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::infinity(),
                                      0, timing));
  timing.playback_rate = -1;
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            CalculateScaledActiveTime(std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::infinity(),
                                      0, timing));
}

TEST(AnimationTimingCalculationsTest, IterationTime) {
  Timing timing;

  // calculateIterationTime(
  //     iterationDuration, repeatedDuration, scaledActiveTime, startOffset,
  //     phase, timing)

  // if the scaled active time is null
  EXPECT_TRUE(IsNull(CalculateIterationTime(
      1, 1, NullValue(), 1, AnimationEffect::kPhaseActive, timing)));

  // if (complex-conditions)...
  EXPECT_EQ(12, CalculateIterationTime(12, 12, 12, 0,
                                       AnimationEffect::kPhaseActive, timing));

  // otherwise
  timing.iteration_count = 10;
  EXPECT_EQ(5, CalculateIterationTime(10, 100, 25, 4,
                                      AnimationEffect::kPhaseActive, timing));
  EXPECT_EQ(7, CalculateIterationTime(11, 110, 29, 1,
                                      AnimationEffect::kPhaseActive, timing));
  timing.iteration_start = 1.1;
  EXPECT_EQ(8, CalculateIterationTime(12, 120, 20, 7,
                                      AnimationEffect::kPhaseActive, timing));
}

TEST(AnimationTimingCalculationsTest, CurrentIteration) {
  Timing timing;

  // calculateCurrentIteration(
  //     iterationDuration, iterationTime, scaledActiveTime, timing)

  // if the scaled active time is null
  EXPECT_TRUE(IsNull(CalculateCurrentIteration(1, 1, NullValue(), timing)));

  // if the scaled active time is zero
  EXPECT_EQ(0, CalculateCurrentIteration(1, 1, 0, timing));

  // if the iteration time equals the iteration duration
  timing.iteration_start = 4;
  timing.iteration_count = 7;
  EXPECT_EQ(10, CalculateCurrentIteration(5, 5, 9, timing));

  // otherwise
  EXPECT_EQ(3, CalculateCurrentIteration(3.2, 3.1, 10, timing));
}

TEST(AnimationTimingCalculationsTest, DirectedTime) {
  Timing timing;

  // calculateDirectedTime(
  //     currentIteration, iterationDuration, iterationTime, timing)

  // if the iteration time is null
  EXPECT_TRUE(IsNull(CalculateDirectedTime(1, 2, NullValue(), timing)));

  // forwards
  EXPECT_EQ(17, CalculateDirectedTime(0, 20, 17, timing));
  EXPECT_EQ(17, CalculateDirectedTime(1, 20, 17, timing));
  timing.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  EXPECT_EQ(17, CalculateDirectedTime(0, 20, 17, timing));
  EXPECT_EQ(17, CalculateDirectedTime(2, 20, 17, timing));
  timing.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;
  EXPECT_EQ(17, CalculateDirectedTime(1, 20, 17, timing));
  EXPECT_EQ(17, CalculateDirectedTime(3, 20, 17, timing));

  // reverse
  timing.direction = Timing::PlaybackDirection::REVERSE;
  EXPECT_EQ(3, CalculateDirectedTime(0, 20, 17, timing));
  EXPECT_EQ(3, CalculateDirectedTime(1, 20, 17, timing));
  timing.direction = Timing::PlaybackDirection::ALTERNATE_NORMAL;
  EXPECT_EQ(3, CalculateDirectedTime(1, 20, 17, timing));
  EXPECT_EQ(3, CalculateDirectedTime(3, 20, 17, timing));
  timing.direction = Timing::PlaybackDirection::ALTERNATE_REVERSE;
  EXPECT_EQ(3, CalculateDirectedTime(0, 20, 17, timing));
  EXPECT_EQ(3, CalculateDirectedTime(2, 20, 17, timing));
}

TEST(AnimationTimingCalculationsTest, TransformedTime) {
  Timing timing;

  // calculateTransformedTime(
  //     currentIteration, iterationDuration, iterationTime, timing)

  // Iteration time is null
  EXPECT_FALSE(CalculateTransformedTime(1, 2, NullValue(), timing).has_value());

  // PlaybackDirectionForwards
  EXPECT_EQ(12, CalculateTransformedTime(0, 20, 12, timing));
  EXPECT_EQ(12, CalculateTransformedTime(1, 20, 12, timing));

  // PlaybackDirectionForwards with timing function
  timing.timing_function =
      StepsTimingFunction::Create(4, StepsTimingFunction::StepPosition::END);
  EXPECT_EQ(10, CalculateTransformedTime(0, 20, 12, timing));
  EXPECT_EQ(10, CalculateTransformedTime(1, 20, 12, timing));

  // PlaybackDirectionReverse
  timing.timing_function = Timing::Defaults().timing_function;
  timing.direction = Timing::PlaybackDirection::REVERSE;
  EXPECT_EQ(8, CalculateTransformedTime(0, 20, 12, timing));
  EXPECT_EQ(8, CalculateTransformedTime(1, 20, 12, timing));

  // PlaybackDirectionReverse with timing function
  timing.timing_function =
      StepsTimingFunction::Create(4, StepsTimingFunction::StepPosition::END);
  EXPECT_EQ(5, CalculateTransformedTime(0, 20, 12, timing));
  EXPECT_EQ(5, CalculateTransformedTime(1, 20, 12, timing));

  // Timing function when directed time is null.
  EXPECT_FALSE(CalculateTransformedTime(1, 2, NullValue(), timing).has_value());

  // Timing function when iterationDuration is infinity
  timing.direction = Timing::PlaybackDirection::NORMAL;
  EXPECT_EQ(0, CalculateTransformedTime(
                   0, std::numeric_limits<double>::infinity(), 0, timing));
  EXPECT_EQ(1, CalculateTransformedTime(
                   0, std::numeric_limits<double>::infinity(), 1, timing));
  timing.direction = Timing::PlaybackDirection::REVERSE;
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            CalculateTransformedTime(0, std::numeric_limits<double>::infinity(),
                                     0, timing));
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            CalculateTransformedTime(0, std::numeric_limits<double>::infinity(),
                                     1, timing));
}

}  // namespace blink
