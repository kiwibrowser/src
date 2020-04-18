// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SCRIPTED_IDLE_TASK_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SCRIPTED_IDLE_TASK_CONTROLLER_H_

#include "third_party/blink/renderer/bindings/core/v8/v8_idle_request_callback.h"
#include "third_party/blink/renderer/core/dom/idle_deadline.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
namespace internal {
class IdleRequestCallbackWrapper;
}

class ExecutionContext;
class IdleRequestOptions;

class CORE_EXPORT ScriptedIdleTaskController
    : public GarbageCollectedFinalized<ScriptedIdleTaskController>,
      public PausableObject,
      public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(ScriptedIdleTaskController);

 public:
  static ScriptedIdleTaskController* Create(ExecutionContext* context) {
    return new ScriptedIdleTaskController(context);
  }
  ~ScriptedIdleTaskController() override;

  void Trace(blink::Visitor*) override;
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "ScriptedIdleTaskController";
  }

  using CallbackId = int;

  // |IdleTask| is an interface type which generalizes tasks which are invoked
  // on idle. The tasks need to define what to do on idle in |invoke|.
  class IdleTask : public GarbageCollectedFinalized<IdleTask>,
                   public TraceWrapperBase {
   public:
    virtual void Trace(blink::Visitor* visitor) {}
    void TraceWrappers(ScriptWrappableVisitor* visitor) const override {}
    const char* NameInHeapSnapshot() const override { return "IdleTask"; }
    virtual ~IdleTask() = default;
    virtual void invoke(IdleDeadline*) = 0;
  };

  // |V8IdleTask| is the adapter class for the conversion from
  // |V8IdleRequestCallback| to |IdleTask|.
  class V8IdleTask : public IdleTask {
   public:
    static V8IdleTask* Create(V8IdleRequestCallback* callback) {
      return new V8IdleTask(callback);
    }
    ~V8IdleTask() override = default;
    void invoke(IdleDeadline*) override;
    void Trace(blink::Visitor*) override;
    void TraceWrappers(ScriptWrappableVisitor*) const override;

   private:
    explicit V8IdleTask(V8IdleRequestCallback*);
    TraceWrapperMember<V8IdleRequestCallback> callback_;
  };

  int RegisterCallback(IdleTask*, const IdleRequestOptions&);
  void CancelCallback(CallbackId);

  // PausableObject interface.
  void ContextDestroyed(ExecutionContext*) override;
  void Pause() override;
  void Unpause() override;

  void CallbackFired(CallbackId,
                     double deadline_seconds,
                     IdleDeadline::CallbackType);

 private:
  friend class internal::IdleRequestCallbackWrapper;
  explicit ScriptedIdleTaskController(ExecutionContext*);

  void ScheduleCallback(scoped_refptr<internal::IdleRequestCallbackWrapper>,
                        long long timeout_millis);

  int NextCallbackId();

  bool IsValidCallbackId(int id) {
    using Traits = HashTraits<CallbackId>;
    return !Traits::IsDeletedValue(id) &&
           !WTF::IsHashTraitsEmptyValue<Traits, CallbackId>(id);
  }

  void RunCallback(CallbackId,
                   double deadline_seconds,
                   IdleDeadline::CallbackType);

  ThreadScheduler* scheduler_;  // Not owned.
  HeapHashMap<CallbackId, TraceWrapperMember<IdleTask>> idle_tasks_;
  Vector<CallbackId> pending_timeouts_;
  CallbackId next_callback_id_;
  bool paused_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SCRIPTED_IDLE_TASK_CONTROLLER_H_
