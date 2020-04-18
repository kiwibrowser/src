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

#include "third_party/blink/renderer/core/animation/animation.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/animation/animation_clock.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/element_animations.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect_model.h"
#include "third_party/blink/renderer/core/animation/pending_animations.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class AnimationAnimationTest : public RenderingTest {
 public:
  AnimationAnimationTest()
      : RenderingTest(SingleChildLocalFrameClient::Create()) {}

  void SetUp() override {
    RenderingTest::SetUp();
    SetUpWithoutStartingTimeline();
    StartTimeline();
  }

  void SetUpWithoutStartingTimeline() {
    page_holder = DummyPageHolder::Create();
    document = &page_holder->GetDocument();
    document->GetAnimationClock().ResetTimeForTesting();
    timeline = DocumentTimeline::Create(document.Get());
    timeline->ResetForTesting();
    animation = timeline->Play(nullptr);
    animation->setStartTime(0, false);
    animation->setEffect(MakeAnimation());
  }

  void StartTimeline() { SimulateFrame(0); }

  void ResetWithCompositedAnimation() {
    // Get rid of the default animation.
    animation->cancel();

    EnableCompositing();

    SetBodyInnerHTML("<div id='target'></div>");

    // Create a compositable animation; in this case opacity from 1 to 0.
    Timing timing;
    timing.iteration_duration = 30;

    scoped_refptr<StringKeyframe> start_keyframe = StringKeyframe::Create();
    start_keyframe->SetCSSPropertyValue(CSSPropertyOpacity, "1.0",
                                        SecureContextMode::kInsecureContext,
                                        nullptr);
    scoped_refptr<StringKeyframe> end_keyframe = StringKeyframe::Create();
    end_keyframe->SetCSSPropertyValue(CSSPropertyOpacity, "0.0",
                                      SecureContextMode::kInsecureContext,
                                      nullptr);

    StringKeyframeVector keyframes;
    keyframes.push_back(start_keyframe);
    keyframes.push_back(end_keyframe);

    Element* element = GetElementById("target");
    StringKeyframeEffectModel* model =
        StringKeyframeEffectModel::Create(keyframes);
    animation = timeline->Play(KeyframeEffect::Create(element, model, timing));

    // After creating the animation we need to clean the lifecycle so that the
    // animation can be pushed to the compositor.
    UpdateAllLifecyclePhases();

    document->GetAnimationClock().UpdateTime(base::TimeTicks());
    document->GetPendingAnimations().Update(base::nullopt, true);
  }

  KeyframeEffectModelBase* MakeEmptyEffectModel() {
    return StringKeyframeEffectModel::Create(StringKeyframeVector());
  }

  KeyframeEffect* MakeAnimation(double duration = 30,
                                double playback_rate = 1) {
    Timing timing;
    timing.iteration_duration = duration;
    timing.playback_rate = playback_rate;
    return KeyframeEffect::Create(nullptr, MakeEmptyEffectModel(), timing);
  }

  bool SimulateFrame(
      double time,
      base::Optional<CompositorElementIdSet> composited_element_ids =
          base::Optional<CompositorElementIdSet>()) {
    document->GetAnimationClock().UpdateTime(
        base::TimeTicks() + base::TimeDelta::FromSecondsD(time));
    document->GetPendingAnimations().Update(composited_element_ids, false);
    // The timeline does not know about our animation, so we have to explicitly
    // call update().
    return animation->Update(kTimingUpdateForAnimationFrame);
  }

  Persistent<Document> document;
  Persistent<DocumentTimeline> timeline;
  Persistent<Animation> animation;
  std::unique_ptr<DummyPageHolder> page_holder;
};

TEST_F(AnimationAnimationTest, InitialState) {
  SetUpWithoutStartingTimeline();
  animation = timeline->Play(nullptr);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Paused());
  EXPECT_EQ(1, animation->playbackRate());
  EXPECT_FALSE(animation->StartTimeInternal());

  StartTimeline();
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(0, timeline->CurrentTimeInternal());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Paused());
  EXPECT_EQ(1, animation->playbackRate());
  EXPECT_EQ(0, animation->StartTimeInternal());
}

TEST_F(AnimationAnimationTest, CurrentTimeDoesNotSetOutdated) {
  EXPECT_FALSE(animation->Outdated());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Outdated());
  // FIXME: We should split simulateFrame into a version that doesn't update
  // the animation and one that does, as most of the tests don't require
  // update() to be called.
  document->GetAnimationClock().UpdateTime(base::TimeTicks() +
                                           base::TimeDelta::FromSecondsD(10));
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Outdated());
}

TEST_F(AnimationAnimationTest, SetCurrentTime) {
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  animation->SetCurrentTimeInternal(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetCurrentTimeNegative) {
  animation->SetCurrentTimeInternal(-10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(-10, animation->CurrentTimeInternal());
  SimulateFrame(20);
  EXPECT_EQ(10, animation->CurrentTimeInternal());

  animation->setPlaybackRate(-2);
  animation->SetCurrentTimeInternal(-10);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(-10, animation->CurrentTimeInternal());
  SimulateFrame(40);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(-10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest,
       SetCurrentTimeNegativeWithoutSimultaneousPlaybackRateChange) {
  SimulateFrame(20);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  animation->setPlaybackRate(-1);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(30);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  animation->setCurrentTime(-10 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, SetCurrentTimePastContentEnd) {
  animation->setCurrentTime(50 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(50, animation->CurrentTimeInternal());
  SimulateFrame(20);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(50, animation->CurrentTimeInternal());

  animation->setPlaybackRate(-2);
  animation->setCurrentTime(50 * 1000, false);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(50, animation->CurrentTimeInternal());
  SimulateFrame(20);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  SimulateFrame(40);
  EXPECT_EQ(10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetCurrentTimeMax) {
  animation->SetCurrentTimeInternal(std::numeric_limits<double>::max());
  EXPECT_EQ(std::numeric_limits<double>::max(),
            animation->CurrentTimeInternal());
  SimulateFrame(100);
  EXPECT_EQ(std::numeric_limits<double>::max(),
            animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetCurrentTimeSetsStartTime) {
  EXPECT_EQ(0, animation->startTime());
  animation->setCurrentTime(1000, false);
  EXPECT_EQ(-1000, animation->startTime());
  SimulateFrame(1);
  EXPECT_EQ(-1000, animation->startTime());
  EXPECT_EQ(2000, animation->currentTime());
}

TEST_F(AnimationAnimationTest, SetStartTime) {
  SimulateFrame(20);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->StartTimeInternal());
  EXPECT_EQ(20 * 1000, animation->currentTime());
  animation->setStartTime(10 * 1000, false);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(10, animation->StartTimeInternal());
  EXPECT_EQ(10 * 1000, animation->currentTime());
  SimulateFrame(30);
  EXPECT_EQ(10, animation->StartTimeInternal());
  EXPECT_EQ(20 * 1000, animation->currentTime());
  animation->setStartTime(-20 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, SetStartTimeLimitsAnimation) {
  animation->setStartTime(-50 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  animation->setPlaybackRate(-1);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  animation->setStartTime(-100 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_TRUE(animation->Limited());
}

TEST_F(AnimationAnimationTest, SetStartTimeOnLimitedAnimation) {
  SimulateFrame(30);
  animation->setStartTime(-10 * 1000, false);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  animation->SetCurrentTimeInternal(50);
  animation->setStartTime(-40 * 1000, false);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_TRUE(animation->Limited());
}

TEST_F(AnimationAnimationTest, StartTimePauseFinish) {
  NonThrowableExceptionState exception_state;
  animation->pause();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_FALSE(animation->startTime());
  animation->finish(exception_state);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
  EXPECT_EQ(-30000, animation->startTime());
}

TEST_F(AnimationAnimationTest, FinishWhenPaused) {
  NonThrowableExceptionState exception_state;
  animation->pause();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kPaused, animation->PlayStateInternal());
  animation->finish(exception_state);
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, StartTimeFinishPause) {
  NonThrowableExceptionState exception_state;
  animation->finish(exception_state);
  EXPECT_EQ(-30 * 1000, animation->startTime());
  animation->pause();
  EXPECT_FALSE(animation->startTime());
}

TEST_F(AnimationAnimationTest, StartTimeWithZeroPlaybackRate) {
  animation->setPlaybackRate(0);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_FALSE(animation->startTime());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, PausePlay) {
  SimulateFrame(10);
  animation->pause();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_TRUE(animation->Paused());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  SimulateFrame(20);
  EXPECT_EQ(Animation::kPaused, animation->PlayStateInternal());
  animation->play();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(20);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_FALSE(animation->Paused());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  SimulateFrame(30);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, PlayRewindsToStart) {
  animation->SetCurrentTimeInternal(30);
  animation->play();
  EXPECT_EQ(0, animation->CurrentTimeInternal());

  animation->SetCurrentTimeInternal(40);
  animation->play();
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());

  animation->SetCurrentTimeInternal(-10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  animation->play();
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, PlayRewindsToEnd) {
  animation->setPlaybackRate(-1);
  animation->play();
  EXPECT_EQ(30, animation->CurrentTimeInternal());

  animation->SetCurrentTimeInternal(40);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  animation->play();
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());

  animation->SetCurrentTimeInternal(-10);
  animation->play();
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  SimulateFrame(20);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, PlayWithPlaybackRateZeroDoesNotSeek) {
  animation->setPlaybackRate(0);
  animation->play();
  EXPECT_EQ(0, animation->CurrentTimeInternal());

  animation->SetCurrentTimeInternal(40);
  animation->play();
  EXPECT_EQ(40, animation->CurrentTimeInternal());

  animation->SetCurrentTimeInternal(-10);
  animation->play();
  EXPECT_EQ(-10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest,
       PlayAfterPauseWithPlaybackRateZeroUpdatesPlayState) {
  animation->pause();
  animation->setPlaybackRate(0);
  SimulateFrame(1);
  EXPECT_EQ(Animation::kPaused, animation->PlayStateInternal());
  animation->play();
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, Reverse) {
  animation->SetCurrentTimeInternal(10);
  animation->pause();
  animation->reverse();
  EXPECT_FALSE(animation->Paused());
  EXPECT_EQ(-1, animation->playbackRate());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, ReverseDoesNothingWithPlaybackRateZero) {
  animation->SetCurrentTimeInternal(10);
  animation->setPlaybackRate(0);
  animation->pause();
  animation->reverse();
  EXPECT_TRUE(animation->Paused());
  EXPECT_EQ(0, animation->playbackRate());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, ReverseSeeksToStart) {
  animation->SetCurrentTimeInternal(-10);
  animation->setPlaybackRate(-1);
  animation->reverse();
  EXPECT_EQ(0, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, ReverseSeeksToEnd) {
  animation->setCurrentTime(40 * 1000, false);
  animation->reverse();
  EXPECT_EQ(30, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, ReverseBeyondLimit) {
  animation->SetCurrentTimeInternal(40);
  animation->setPlaybackRate(-1);
  animation->reverse();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->CurrentTimeInternal());

  animation->SetCurrentTimeInternal(-10);
  animation->reverse();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(30, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, Finish) {
  NonThrowableExceptionState exception_state;
  animation->finish(exception_state);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());

  animation->setPlaybackRate(-1);
  animation->finish(exception_state);
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, FinishAfterEffectEnd) {
  NonThrowableExceptionState exception_state;
  animation->setCurrentTime(40 * 1000, false);
  animation->finish(exception_state);
  EXPECT_EQ(40, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, FinishBeforeStart) {
  NonThrowableExceptionState exception_state;
  animation->SetCurrentTimeInternal(-10);
  animation->setPlaybackRate(-1);
  animation->finish(exception_state);
  EXPECT_EQ(0, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, FinishDoesNothingWithPlaybackRateZero) {
  DummyExceptionStateForTesting exception_state;
  animation->SetCurrentTimeInternal(10);
  animation->setPlaybackRate(0);
  animation->finish(exception_state);
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  EXPECT_TRUE(exception_state.HadException());
}

TEST_F(AnimationAnimationTest, FinishRaisesException) {
  Timing timing;
  timing.iteration_duration = 1;
  timing.iteration_count = std::numeric_limits<double>::infinity();
  animation->setEffect(
      KeyframeEffect::Create(nullptr, MakeEmptyEffectModel(), timing));
  animation->SetCurrentTimeInternal(10);

  DummyExceptionStateForTesting exception_state;
  animation->finish(exception_state);
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  EXPECT_TRUE(exception_state.HadException());
  EXPECT_EQ(kInvalidStateError, exception_state.Code());
}

TEST_F(AnimationAnimationTest, LimitingAtEffectEnd) {
  SimulateFrame(30);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_TRUE(animation->Limited());
  SimulateFrame(40);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Paused());
}

TEST_F(AnimationAnimationTest, LimitingAtStart) {
  SimulateFrame(30);
  animation->setPlaybackRate(-2);
  SimulateFrame(30);
  SimulateFrame(45);
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_TRUE(animation->Limited());
  SimulateFrame(60);
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  EXPECT_FALSE(animation->Paused());
}

TEST_F(AnimationAnimationTest, LimitingWithNoEffect) {
  animation->setEffect(nullptr);
  EXPECT_TRUE(animation->Limited());
  SimulateFrame(30);
  EXPECT_EQ(0, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetPlaybackRate) {
  animation->setPlaybackRate(2);
  SimulateFrame(0);
  EXPECT_EQ(2, animation->playbackRate());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  SimulateFrame(10);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetPlaybackRateWhilePaused) {
  SimulateFrame(10);
  animation->pause();
  animation->setPlaybackRate(2);
  SimulateFrame(20);
  animation->play();
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  SimulateFrame(20);
  SimulateFrame(25);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetPlaybackRateWhileLimited) {
  SimulateFrame(40);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  animation->setPlaybackRate(2);
  SimulateFrame(50);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
  animation->setPlaybackRate(-2);
  SimulateFrame(50);
  SimulateFrame(60);
  EXPECT_FALSE(animation->Limited());
  EXPECT_EQ(10, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetPlaybackRateZero) {
  SimulateFrame(0);
  SimulateFrame(10);
  animation->setPlaybackRate(0);
  SimulateFrame(10);
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  SimulateFrame(20);
  EXPECT_EQ(10, animation->CurrentTimeInternal());
  animation->SetCurrentTimeInternal(20);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetPlaybackRateMax) {
  animation->setPlaybackRate(std::numeric_limits<double>::max());
  SimulateFrame(0);
  EXPECT_EQ(std::numeric_limits<double>::max(), animation->playbackRate());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  SimulateFrame(1);
  EXPECT_EQ(30, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetEffect) {
  animation = timeline->Play(nullptr);
  animation->setStartTime(0, false);
  AnimationEffect* effect1 = MakeAnimation();
  AnimationEffect* effect2 = MakeAnimation();
  animation->setEffect(effect1);
  EXPECT_EQ(effect1, animation->effect());
  EXPECT_EQ(0, animation->CurrentTimeInternal());
  animation->SetCurrentTimeInternal(15);
  animation->setEffect(effect2);
  EXPECT_EQ(15, animation->CurrentTimeInternal());
  EXPECT_EQ(nullptr, effect1->GetAnimationForTesting());
  EXPECT_EQ(animation, effect2->GetAnimationForTesting());
  EXPECT_EQ(effect2, animation->effect());
}

TEST_F(AnimationAnimationTest, SetEffectLimitsAnimation) {
  animation->SetCurrentTimeInternal(20);
  animation->setEffect(MakeAnimation(10));
  EXPECT_EQ(20, animation->CurrentTimeInternal());
  EXPECT_TRUE(animation->Limited());
  SimulateFrame(10);
  EXPECT_EQ(20, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, SetEffectUnlimitsAnimation) {
  animation->SetCurrentTimeInternal(40);
  animation->setEffect(MakeAnimation(60));
  EXPECT_FALSE(animation->Limited());
  EXPECT_EQ(40, animation->CurrentTimeInternal());
  SimulateFrame(10);
  EXPECT_EQ(50, animation->CurrentTimeInternal());
}

TEST_F(AnimationAnimationTest, EmptyAnimationsDontUpdateEffects) {
  animation = timeline->Play(nullptr);
  animation->Update(kTimingUpdateOnDemand);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());

  SimulateFrame(1234);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest, AnimationsDisassociateFromEffect) {
  AnimationEffect* animation_node = animation->effect();
  Animation* animation2 = timeline->Play(animation_node);
  EXPECT_EQ(nullptr, animation->effect());
  animation->setEffect(animation_node);
  EXPECT_EQ(nullptr, animation2->effect());
}

TEST_F(AnimationAnimationTest, AnimationsReturnTimeToNextEffect) {
  Timing timing;
  timing.start_delay = 1;
  timing.iteration_duration = 1;
  timing.end_delay = 1;
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(nullptr, MakeEmptyEffectModel(), timing);
  animation = timeline->Play(keyframe_effect);
  animation->setStartTime(0, false);

  SimulateFrame(0);
  EXPECT_EQ(1, animation->TimeToEffectChange());

  SimulateFrame(0.5);
  EXPECT_EQ(0.5, animation->TimeToEffectChange());

  SimulateFrame(1);
  EXPECT_EQ(0, animation->TimeToEffectChange());

  SimulateFrame(1.5);
  EXPECT_EQ(0, animation->TimeToEffectChange());

  SimulateFrame(2);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());

  SimulateFrame(3);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());

  animation->SetCurrentTimeInternal(0);
  SimulateFrame(3);
  EXPECT_EQ(1, animation->TimeToEffectChange());

  animation->setPlaybackRate(2);
  SimulateFrame(3);
  EXPECT_EQ(0.5, animation->TimeToEffectChange());

  animation->setPlaybackRate(0);
  animation->Update(kTimingUpdateOnDemand);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());

  animation->SetCurrentTimeInternal(3);
  animation->setPlaybackRate(-1);
  animation->Update(kTimingUpdateOnDemand);
  SimulateFrame(3);
  EXPECT_EQ(1, animation->TimeToEffectChange());

  animation->setPlaybackRate(-2);
  animation->Update(kTimingUpdateOnDemand);
  SimulateFrame(3);
  EXPECT_EQ(0.5, animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest, TimeToNextEffectWhenPaused) {
  EXPECT_EQ(0, animation->TimeToEffectChange());
  animation->pause();
  animation->Update(kTimingUpdateOnDemand);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest, TimeToNextEffectWhenCancelledBeforeStart) {
  EXPECT_EQ(0, animation->TimeToEffectChange());
  animation->SetCurrentTimeInternal(-8);
  animation->setPlaybackRate(2);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  animation->Update(kTimingUpdateOnDemand);
  // This frame will fire the finish event event though no start time has been
  // received from the compositor yet, as cancel() nukes start times.
  SimulateFrame(0);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest,
       TimeToNextEffectWhenCancelledBeforeStartReverse) {
  EXPECT_EQ(0, animation->TimeToEffectChange());
  animation->SetCurrentTimeInternal(9);
  animation->setPlaybackRate(-3);
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  animation->Update(kTimingUpdateOnDemand);
  // This frame will fire the finish event event though no start time has been
  // received from the compositor yet, as cancel() nukes start times.
  SimulateFrame(0);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest, TimeToNextEffectSimpleCancelledBeforeStart) {
  EXPECT_EQ(0, animation->TimeToEffectChange());
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  animation->cancel();
  animation->Update(kTimingUpdateOnDemand);
  // This frame will fire the finish event event though no start time has been
  // received from the compositor yet, as cancel() nukes start times.
  SimulateFrame(0);
  EXPECT_EQ(std::numeric_limits<double>::infinity(),
            animation->TimeToEffectChange());
}

TEST_F(AnimationAnimationTest, AttachedAnimations) {
  Persistent<Element> element = document->CreateElementForBinding("foo");

  Timing timing;
  KeyframeEffect* keyframe_effect =
      KeyframeEffect::Create(element.Get(), MakeEmptyEffectModel(), timing);
  Animation* animation = timeline->Play(keyframe_effect);
  SimulateFrame(0);
  timeline->ServiceAnimations(kTimingUpdateForAnimationFrame);
  EXPECT_EQ(
      1U, element->GetElementAnimations()->Animations().find(animation)->value);

  ThreadState::Current()->CollectAllGarbage();
  EXPECT_TRUE(element->GetElementAnimations()->Animations().IsEmpty());
}

TEST_F(AnimationAnimationTest, HasLowerPriority) {
  Animation* animation1 = timeline->Play(nullptr);
  Animation* animation2 = timeline->Play(nullptr);
  EXPECT_TRUE(Animation::HasLowerPriority(animation1, animation2));
}

TEST_F(AnimationAnimationTest, PlayAfterCancel) {
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  EXPECT_TRUE(std::isnan(animation->currentTime()));
  EXPECT_FALSE(animation->startTime());
  animation->play();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->currentTime());
  EXPECT_FALSE(animation->startTime());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->currentTime());
  EXPECT_EQ(10 * 1000, animation->startTime());
}

TEST_F(AnimationAnimationTest, PlayBackwardsAfterCancel) {
  animation->setPlaybackRate(-1);
  animation->setCurrentTime(15 * 1000, false);
  SimulateFrame(0);
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  EXPECT_TRUE(std::isnan(animation->currentTime()));
  EXPECT_FALSE(animation->startTime());
  animation->play();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(30 * 1000, animation->currentTime());
  EXPECT_FALSE(animation->startTime());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(30 * 1000, animation->currentTime());
  EXPECT_EQ(40 * 1000, animation->startTime());
}

TEST_F(AnimationAnimationTest, ReverseAfterCancel) {
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  EXPECT_TRUE(std::isnan(animation->currentTime()));
  EXPECT_FALSE(animation->startTime());
  animation->reverse();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(30 * 1000, animation->currentTime());
  EXPECT_FALSE(animation->startTime());
  SimulateFrame(10);
  EXPECT_EQ(Animation::kRunning, animation->PlayStateInternal());
  EXPECT_EQ(30 * 1000, animation->currentTime());
  EXPECT_EQ(40 * 1000, animation->startTime());
}

TEST_F(AnimationAnimationTest, FinishAfterCancel) {
  NonThrowableExceptionState exception_state;
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  EXPECT_TRUE(std::isnan(animation->currentTime()));
  EXPECT_FALSE(animation->startTime());
  animation->finish(exception_state);
  EXPECT_EQ(30000, animation->currentTime());
  EXPECT_EQ(-30000, animation->startTime());
  EXPECT_EQ(Animation::kFinished, animation->PlayStateInternal());
}

TEST_F(AnimationAnimationTest, PauseAfterCancel) {
  animation->cancel();
  EXPECT_EQ(Animation::kIdle, animation->PlayStateInternal());
  EXPECT_TRUE(std::isnan(animation->currentTime()));
  EXPECT_FALSE(animation->startTime());
  animation->pause();
  EXPECT_EQ(Animation::kPending, animation->PlayStateInternal());
  EXPECT_EQ(0, animation->currentTime());
  EXPECT_FALSE(animation->startTime());
}

TEST_F(AnimationAnimationTest, NoCompositeWithoutCompositedElementId) {
  ScopedSlimmingPaintV2ForTest enable_s_pv2(true);

  SetBodyInnerHTML(
      "<div id='foo' style='position: relative'></div>"
      "<div id='bar' style='position: relative'></div>");

  LayoutObject* object_composited = GetLayoutObjectByElementId("foo");
  LayoutObject* object_not_composited = GetLayoutObjectByElementId("bar");

  base::Optional<CompositorElementIdSet> composited_element_ids =
      CompositorElementIdSet();
  CompositorElementId expected_compositor_element_id =
      CompositorElementIdFromUniqueObjectId(
          ToLayoutBoxModelObject(object_composited)->UniqueId(),
          CompositorElementIdNamespace::kPrimary);
  composited_element_ids->insert(expected_compositor_element_id);

  Timing timing;
  timing.iteration_duration = 30;
  timing.playback_rate = 1;
  KeyframeEffect* keyframe_effect_composited = KeyframeEffect::Create(
      ToElement(object_composited->GetNode()), MakeEmptyEffectModel(), timing);
  Animation* animation_composited = timeline->Play(keyframe_effect_composited);
  KeyframeEffect* keyframe_effect_not_composited =
      KeyframeEffect::Create(ToElement(object_not_composited->GetNode()),
                             MakeEmptyEffectModel(), timing);
  Animation* animation_not_composited =
      timeline->Play(keyframe_effect_not_composited);

  SimulateFrame(0, composited_element_ids);
  EXPECT_TRUE(
      animation_composited
          ->CheckCanStartAnimationOnCompositorInternal(composited_element_ids)
          .Ok());
  EXPECT_FALSE(
      animation_not_composited
          ->CheckCanStartAnimationOnCompositorInternal(composited_element_ids)
          .Ok());
}

// Regression test for http://crbug.com/819591 . If a compositable animation is
// played and then paused before any start time is set (either blink or
// compositor side), the pausing must still set compositor pending or the pause
// won't be synced.
TEST_F(AnimationAnimationTest, SetCompositorPendingWithUnresolvedStartTimes) {
  ResetWithCompositedAnimation();

  // At this point, the animation exists on both the compositor and blink side,
  // but no start time has arrived on either side. The compositor is currently
  // synced, no update is pending.
  EXPECT_FALSE(animation->CompositorPendingForTesting());

  // However, if we pause the animation then the compositor should still be
  // marked pending. This is required because otherwise the compositor will go
  // ahead and start playing the animation once it receives a start time (e.g.
  // on the next compositor frame).
  animation->pause();

  EXPECT_TRUE(animation->CompositorPendingForTesting());
}

TEST_F(AnimationAnimationTest, PreCommitWithUnresolvedStartTimes) {
  ResetWithCompositedAnimation();

  // At this point, the animation exists on both the compositor and blink side,
  // but no start time has arrived on either side. The compositor is currently
  // synced, no update is pending.
  EXPECT_FALSE(animation->CompositorPendingForTesting());

  // At this point, a call to PreCommit should bail out and tell us to wait for
  // next commit because there are no resolved start times.
  EXPECT_FALSE(animation->PreCommit(0, base::nullopt, true));
}

TEST_F(AnimationAnimationTest, SetKeyframesCausesCompositorPending) {
  ResetWithCompositedAnimation();

  // At this point, the animation exists on both the compositor and blink side,
  // but no start time has arrived on either side. The compositor is currently
  // synced, no update is pending.
  EXPECT_FALSE(animation->CompositorPendingForTesting());

  // Now change the keyframes; this should mark the animation as compositor
  // pending as we need to sync the compositor side.
  scoped_refptr<StringKeyframe> start_keyframe = StringKeyframe::Create();
  start_keyframe->SetCSSPropertyValue(
      CSSPropertyOpacity, "0.0", SecureContextMode::kInsecureContext, nullptr);
  scoped_refptr<StringKeyframe> end_keyframe = StringKeyframe::Create();
  end_keyframe->SetCSSPropertyValue(
      CSSPropertyOpacity, "1.0", SecureContextMode::kInsecureContext, nullptr);

  StringKeyframeVector keyframes;
  keyframes.push_back(start_keyframe);
  keyframes.push_back(end_keyframe);

  ToKeyframeEffect(animation->effect())->SetKeyframes(keyframes);

  EXPECT_TRUE(animation->CompositorPendingForTesting());
}

}  // namespace blink
