// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_IDLE_SPELL_CHECK_CALLBACK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_IDLE_SPELL_CHECK_CALLBACK_H_

#include "third_party/blink/renderer/core/dom/document_shutdown_observer.h"
#include "third_party/blink/renderer/core/dom/scripted_idle_task_controller.h"
#include "third_party/blink/renderer/core/editing/forward.h"
#include "third_party/blink/renderer/platform/timer.h"

namespace blink {

class ColdModeSpellCheckRequester;
class LocalFrame;
class SpellCheckRequester;

#define FOR_EACH_IDLE_SPELL_CHECK_CALLBACK_STATE(V) \
  V(Inactive)                                       \
  V(HotModeRequested)                               \
  V(InHotModeInvocation)                            \
  V(ColdModeTimerStarted)                           \
  V(ColdModeRequested)                              \
  V(InColdModeInvocation)

// Main class for the implementation of idle time spell checker.
class CORE_EXPORT IdleSpellCheckCallback final
    : public ScriptedIdleTaskController::IdleTask,
      public DocumentShutdownObserver {
  DISALLOW_COPY_AND_ASSIGN(IdleSpellCheckCallback);
  USING_GARBAGE_COLLECTED_MIXIN(IdleSpellCheckCallback);

 public:
  static IdleSpellCheckCallback* Create(LocalFrame&);
  ~IdleSpellCheckCallback() override;

  enum class State {
#define V(state) k##state,
    FOR_EACH_IDLE_SPELL_CHECK_CALLBACK_STATE(V)
#undef V
  };

  State GetState() const { return state_; }

  // Transit to HotModeRequested, if possible. Called by operations that need
  // spell checker to follow up.
  void SetNeedsInvocation();

  // Cleans everything up and makes the callback inactive. Should be called when
  // document is detached or spellchecking is globally disabled.
  void Deactivate();

  void DocumentAttached(Document*);

  // Exposed for testing only.
  SpellCheckRequester& GetSpellCheckRequester() const;
  void ForceInvocationForTesting();
  void SetNeedsMoreColdModeInvocationForTesting();
  void SkipColdModeTimerForTesting();
  int IdleCallbackHandle() const { return idle_callback_handle_; }

  void Trace(blink::Visitor*) override;

 private:
  explicit IdleSpellCheckCallback(LocalFrame&);
  void invoke(IdleDeadline*) override;

  LocalFrame& GetFrame() const { return *frame_; }

  // Returns whether there is an active document to work on.
  bool IsAvailable() const { return LifecycleContext(); }

  // Return the document to work on. Callable only when IsAvailable() is true.
  Document& GetDocument() const {
    DCHECK(IsAvailable());
    return *LifecycleContext();
  }

  // Returns whether spell checking is globally enabled.
  bool IsSpellCheckingEnabled() const;

  // Functions for hot mode.
  void HotModeInvocation(IdleDeadline*);

  // Transit to ColdModeTimerStarted, if possible. Sets up a timer, and requests
  // cold mode invocation if no critical operation occurs before timer firing.
  void SetNeedsColdModeInvocation();

  // Functions for cold mode.
  void ColdModeTimerFired(TimerBase*);
  void ColdModeInvocation(IdleDeadline*);

  // Implements |DocumentShutdownObserver|.
  void ContextDestroyed(Document*) final;

  State state_;
  int idle_callback_handle_;
  const Member<LocalFrame> frame_;
  uint64_t last_processed_undo_step_sequence_;
  const Member<ColdModeSpellCheckRequester> cold_mode_requester_;
  TaskRunnerTimer<IdleSpellCheckCallback> cold_mode_timer_;

  friend class IdleSpellCheckCallbackTest;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_SPELLCHECK_IDLE_SPELL_CHECK_CALLBACK_H_
