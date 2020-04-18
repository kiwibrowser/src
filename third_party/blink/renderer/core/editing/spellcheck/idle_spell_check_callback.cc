// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/spellcheck/idle_spell_check_callback.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/idle_request_options.h"
#include "third_party/blink/renderer/core/editing/commands/undo_stack.h"
#include "third_party/blink/renderer/core/editing/commands/undo_step.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/iterators/character_iterator.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/spellcheck/cold_mode_spell_check_requester.h"
#include "third_party/blink/renderer/core/editing/spellcheck/hot_mode_spell_check_requester.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_check_requester.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

const int kColdModeTimerIntervalMS = 1000;
const int kConsecutiveColdModeTimerIntervalMS = 200;
const int kHotModeRequestTimeoutMS = 200;
const int kInvalidHandle = -1;
const int kDummyHandleForForcedInvocation = -2;
const double kForcedInvocationDeadlineSeconds = 10;

}  // namespace

IdleSpellCheckCallback::~IdleSpellCheckCallback() = default;

void IdleSpellCheckCallback::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(cold_mode_requester_);
  DocumentShutdownObserver::Trace(visitor);
  ScriptedIdleTaskController::IdleTask::Trace(visitor);
}

IdleSpellCheckCallback* IdleSpellCheckCallback::Create(LocalFrame& frame) {
  return new IdleSpellCheckCallback(frame);
}

IdleSpellCheckCallback::IdleSpellCheckCallback(LocalFrame& frame)
    : state_(State::kInactive),
      idle_callback_handle_(kInvalidHandle),
      frame_(frame),
      last_processed_undo_step_sequence_(0),
      cold_mode_requester_(ColdModeSpellCheckRequester::Create(frame)),
      cold_mode_timer_(frame.GetTaskRunner(TaskType::kInternalDefault),
                       this,
                       &IdleSpellCheckCallback::ColdModeTimerFired) {}

SpellCheckRequester& IdleSpellCheckCallback::GetSpellCheckRequester() const {
  return GetFrame().GetSpellChecker().GetSpellCheckRequester();
}

bool IdleSpellCheckCallback::IsSpellCheckingEnabled() const {
  return GetFrame().GetSpellChecker().IsSpellCheckingEnabled();
}

void IdleSpellCheckCallback::Deactivate() {
  state_ = State::kInactive;
  if (cold_mode_timer_.IsActive())
    cold_mode_timer_.Stop();
  if (idle_callback_handle_ != kInvalidHandle && IsAvailable())
    GetDocument().CancelIdleCallback(idle_callback_handle_);
  idle_callback_handle_ = kInvalidHandle;
}

void IdleSpellCheckCallback::SetNeedsInvocation() {
  if (!IsSpellCheckingEnabled() || !IsAvailable()) {
    Deactivate();
    return;
  }

  if (state_ == State::kHotModeRequested)
    return;

  cold_mode_requester_->ClearProgress();

  if (state_ == State::kColdModeTimerStarted) {
    DCHECK(cold_mode_timer_.IsActive());
    cold_mode_timer_.Stop();
  }

  if (state_ == State::kColdModeRequested) {
    GetDocument().CancelIdleCallback(idle_callback_handle_);
    idle_callback_handle_ = kInvalidHandle;
  }

  IdleRequestOptions options;
  options.setTimeout(kHotModeRequestTimeoutMS);
  idle_callback_handle_ = GetDocument().RequestIdleCallback(this, options);
  state_ = State::kHotModeRequested;
}

void IdleSpellCheckCallback::SetNeedsColdModeInvocation() {
  if (!RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled() ||
      !IsSpellCheckingEnabled()) {
    Deactivate();
    return;
  }

  if (state_ != State::kInactive && state_ != State::kInHotModeInvocation &&
      state_ != State::kInColdModeInvocation)
    return;

  DCHECK(!cold_mode_timer_.IsActive());
  int interval_ms = state_ == State::kInColdModeInvocation
                        ? kConsecutiveColdModeTimerIntervalMS
                        : kColdModeTimerIntervalMS;
  cold_mode_timer_.StartOneShot(interval_ms / 1000.0, FROM_HERE);
  state_ = State::kColdModeTimerStarted;
}

void IdleSpellCheckCallback::ColdModeTimerFired(TimerBase*) {
  DCHECK(RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled());
  DCHECK_EQ(State::kColdModeTimerStarted, state_);

  if (!IsSpellCheckingEnabled() || !IsAvailable()) {
    Deactivate();
    return;
  }

  idle_callback_handle_ =
      GetDocument().RequestIdleCallback(this, IdleRequestOptions());
  state_ = State::kColdModeRequested;
}

void IdleSpellCheckCallback::HotModeInvocation(IdleDeadline* deadline) {
  TRACE_EVENT0("blink", "IdleSpellCheckCallback::hotModeInvocation");

  // TODO(xiaochengh): Figure out if this has any performance impact.
  GetDocument().UpdateStyleAndLayout();

  HotModeSpellCheckRequester requester(GetSpellCheckRequester());

  requester.CheckSpellingAt(
      GetFrame().Selection().GetSelectionInDOMTree().Extent());

  const uint64_t watermark = last_processed_undo_step_sequence_;
  for (const UndoStep* step :
       GetFrame().GetEditor().GetUndoStack().UndoSteps()) {
    if (step->SequenceNumber() <= watermark)
      break;
    last_processed_undo_step_sequence_ =
        std::max(step->SequenceNumber(), last_processed_undo_step_sequence_);
    if (deadline->timeRemaining() == 0)
      break;
    // The ending selection stored in undo stack can be invalid, disconnected
    // or have been moved to another document, so we should check its validity
    // before using it.
    if (!step->EndingSelection().IsValidFor(GetDocument()))
      continue;
    requester.CheckSpellingAt(step->EndingSelection().Extent());
  }
}

void IdleSpellCheckCallback::invoke(IdleDeadline* deadline) {
  DCHECK_NE(idle_callback_handle_, kInvalidHandle);
  idle_callback_handle_ = kInvalidHandle;

  if (!IsSpellCheckingEnabled() || !IsAvailable()) {
    Deactivate();
    return;
  }

  if (state_ == State::kHotModeRequested) {
    state_ = State::kInHotModeInvocation;
    HotModeInvocation(deadline);
    SetNeedsColdModeInvocation();
  } else if (state_ == State::kColdModeRequested) {
    DCHECK(RuntimeEnabledFeatures::IdleTimeColdModeSpellCheckingEnabled());
    state_ = State::kInColdModeInvocation;
    cold_mode_requester_->Invoke(deadline);
    if (cold_mode_requester_->FullyChecked())
      state_ = State::kInactive;
    else
      SetNeedsColdModeInvocation();
  } else {
    NOTREACHED();
  }
}

void IdleSpellCheckCallback::DocumentAttached(Document* document) {
  SetContext(document);
}

void IdleSpellCheckCallback::ContextDestroyed(Document*) {
  Deactivate();
}

void IdleSpellCheckCallback::ForceInvocationForTesting() {
  if (!IsSpellCheckingEnabled())
    return;

  IdleDeadline* deadline = IdleDeadline::Create(
      kForcedInvocationDeadlineSeconds + CurrentTimeTicksInSeconds(),
      IdleDeadline::CallbackType::kCalledWhenIdle);

  switch (state_) {
    case State::kColdModeTimerStarted:
      cold_mode_timer_.Stop();
      state_ = State::kColdModeRequested;
      idle_callback_handle_ = kDummyHandleForForcedInvocation;
      invoke(deadline);
      break;
    case State::kHotModeRequested:
    case State::kColdModeRequested:
      GetDocument().CancelIdleCallback(idle_callback_handle_);
      invoke(deadline);
      break;
    case State::kInactive:
    case State::kInHotModeInvocation:
    case State::kInColdModeInvocation:
      NOTREACHED();
  }
}

void IdleSpellCheckCallback::SkipColdModeTimerForTesting() {
  DCHECK(cold_mode_timer_.IsActive());
  cold_mode_timer_.Stop();
  ColdModeTimerFired(&cold_mode_timer_);
}

void IdleSpellCheckCallback::SetNeedsMoreColdModeInvocationForTesting() {
  cold_mode_requester_->SetNeedsMoreInvocationForTesting();
}

}  // namespace blink
