// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/spellcheck/idle_spell_check_callback.h"

#include "third_party/blink/renderer/core/editing/spellcheck/spell_check_test_base.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

using State = IdleSpellCheckCallback::State;

class IdleSpellCheckCallbackTest : public SpellCheckTestBase {
 protected:
  IdleSpellCheckCallback& IdleChecker() {
    return GetSpellChecker().GetIdleSpellCheckCallback();
  }

  void SetUp() override {
    SpellCheckTestBase::SetUp();

    // The initial cold mode request is on on document startup. This doesn't
    // work in unit test where SpellChecker is enabled after document startup.
    // Post another request here to ensure the activation of cold mode checker.
    IdleChecker().SetNeedsColdModeInvocation();
  }

  void TransitTo(State state) {
    switch (state) {
      case State::kInactive:
        IdleChecker().Deactivate();
        break;
      case State::kHotModeRequested:
        IdleChecker().SetNeedsInvocation();
        break;
      case State::kColdModeTimerStarted:
        DCHECK(RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled());
        break;
      case State::kColdModeRequested:
        DCHECK(RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled());
        IdleChecker().SkipColdModeTimerForTesting();
        break;
      case State::kInHotModeInvocation:
      case State::kInColdModeInvocation:
        NOTREACHED();
    }
  }
};

// Test cases for lifecycle state transitions.

TEST_F(IdleSpellCheckCallbackTest, InitializationWithColdMode) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  EXPECT_EQ(State::kColdModeTimerStarted, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, InitializationWithoutColdMode) {
  if (RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, RequestWhenInactive) {
  TransitTo(State::kInactive);
  IdleChecker().SetNeedsInvocation();
  EXPECT_EQ(State::kHotModeRequested, IdleChecker().GetState());
  EXPECT_NE(-1, IdleChecker().IdleCallbackHandle());
}

TEST_F(IdleSpellCheckCallbackTest, RequestWhenHotModeRequested) {
  TransitTo(State::kHotModeRequested);
  int handle = IdleChecker().IdleCallbackHandle();
  IdleChecker().SetNeedsInvocation();
  EXPECT_EQ(State::kHotModeRequested, IdleChecker().GetState());
  EXPECT_EQ(handle, IdleChecker().IdleCallbackHandle());
  EXPECT_NE(-1, IdleChecker().IdleCallbackHandle());
}

TEST_F(IdleSpellCheckCallbackTest, RequestWhenColdModeTimerStarted) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeTimerStarted);
  IdleChecker().SetNeedsInvocation();
  EXPECT_EQ(State::kHotModeRequested, IdleChecker().GetState());
  EXPECT_NE(-1, IdleChecker().IdleCallbackHandle());
}

TEST_F(IdleSpellCheckCallbackTest, RequestWhenColdModeRequested) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeRequested);
  int handle = IdleChecker().IdleCallbackHandle();
  IdleChecker().SetNeedsInvocation();
  EXPECT_EQ(State::kHotModeRequested, IdleChecker().GetState());
  EXPECT_NE(handle, IdleChecker().IdleCallbackHandle());
  EXPECT_NE(-1, IdleChecker().IdleCallbackHandle());
}

TEST_F(IdleSpellCheckCallbackTest, HotModeTransitToInactive) {
  if (RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kHotModeRequested);
  IdleChecker().ForceInvocationForTesting();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, HotModeTransitToColdMode) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kHotModeRequested);
  IdleChecker().ForceInvocationForTesting();
  EXPECT_EQ(State::kColdModeTimerStarted, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, ColdModeTimerStartedToRequested) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeTimerStarted);
  IdleChecker().SkipColdModeTimerForTesting();
  EXPECT_EQ(State::kColdModeRequested, IdleChecker().GetState());
  EXPECT_NE(-1, IdleChecker().IdleCallbackHandle());
}

TEST_F(IdleSpellCheckCallbackTest, ColdModeStayAtColdMode) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeRequested);
  IdleChecker().SetNeedsMoreColdModeInvocationForTesting();
  IdleChecker().ForceInvocationForTesting();
  EXPECT_EQ(State::kColdModeTimerStarted, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, ColdModeToInactive) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeRequested);
  IdleChecker().ForceInvocationForTesting();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, DetachWhenInactive) {
  TransitTo(State::kInactive);
  GetDocument().Shutdown();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, DetachWhenHotModeRequested) {
  TransitTo(State::kHotModeRequested);
  GetDocument().Shutdown();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, DetachWhenColdModeTimerStarted) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeTimerStarted);
  GetDocument().Shutdown();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

TEST_F(IdleSpellCheckCallbackTest, DetachWhenColdModeRequested) {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled())
    return;

  TransitTo(State::kColdModeRequested);
  GetDocument().Shutdown();
  EXPECT_EQ(State::kInactive, IdleChecker().GetState());
}

}  // namespace blink
