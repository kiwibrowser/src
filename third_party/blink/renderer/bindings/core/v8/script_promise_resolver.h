// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_RESOLVER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_RESOLVER_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/pausable_object.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/scoped_persistent.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/heap/self_keep_alive.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "v8/include/v8.h"

namespace blink {

// This class wraps v8::Promise::Resolver and provides the following
// functionalities.
//  - A ScriptPromiseResolver retains a ScriptState. A caller
//    can call resolve or reject from outside of a V8 context.
//  - This class is an PausableObject and keeps track of the associated
//    ExecutionContext state. When the ExecutionContext is suspended,
//    resolve or reject will be delayed. When it is stopped, resolve or reject
//    will be ignored.
class CORE_EXPORT ScriptPromiseResolver
    : public GarbageCollectedFinalized<ScriptPromiseResolver>,
      public PausableObject {
  USING_GARBAGE_COLLECTED_MIXIN(ScriptPromiseResolver);
  WTF_MAKE_NONCOPYABLE(ScriptPromiseResolver);

 public:
  static ScriptPromiseResolver* Create(ScriptState* script_state) {
    ScriptPromiseResolver* resolver = new ScriptPromiseResolver(script_state);
    resolver->PauseIfNeeded();
    return resolver;
  }

#if DCHECK_IS_ON()
  // Eagerly finalized so as to ensure valid access to getExecutionContext()
  // from the destructor's assert.
  EAGERLY_FINALIZE();

  ~ScriptPromiseResolver() override {
    // This assertion fails if:
    //  - promise() is called at least once and
    //  - this resolver is destructed before it is resolved, rejected,
    //    detached, the V8 isolate is terminated or the associated
    //    ExecutionContext is stopped.
    DCHECK(state_ == kDetached || !is_promise_called_ ||
           !GetScriptState()->ContextIsValid() || !GetExecutionContext() ||
           GetExecutionContext()->IsContextDestroyed());
  }
#endif

  // Anything that can be passed to toV8 can be passed to this function.
  template <typename T>
  void Resolve(T value) {
    ResolveOrReject(value, kResolving);
  }

  // Anything that can be passed to toV8 can be passed to this function.
  template <typename T>
  void Reject(T value) {
    ResolveOrReject(value, kRejecting);
  }

  void Resolve() { Resolve(ToV8UndefinedGenerator()); }
  void Reject() { Reject(ToV8UndefinedGenerator()); }

  ScriptState* GetScriptState() { return script_state_.get(); }

  // Note that an empty ScriptPromise will be returned after resolve or
  // reject is called.
  ScriptPromise Promise() {
#if DCHECK_IS_ON()
    is_promise_called_ = true;
#endif
    return resolver_.Promise();
  }

  ScriptState* GetScriptState() const { return script_state_.get(); }

  // PausableObject implementation.
  void Pause() override;
  void Unpause() override;
  void ContextDestroyed(ExecutionContext*) override { Detach(); }

  // Calling this function makes the resolver release its internal resources.
  // That means the associated promise will never be resolved or rejected
  // unless it's already been resolved or rejected.
  // Do not call this function unless you truly need the behavior.
  void Detach();

  // Once this function is called this resolver stays alive while the
  // promise is pending and the associated ExecutionContext isn't stopped.
  void KeepAliveWhilePending();

  void Trace(blink::Visitor*) override;

 protected:
  // You need to call suspendIfNeeded after the construction because
  // this is an PausableObject.
  explicit ScriptPromiseResolver(ScriptState*);

 private:
  typedef ScriptPromise::InternalResolver Resolver;
  enum ResolutionState {
    kPending,
    kResolving,
    kRejecting,
    kDetached,
  };

  template <typename T>
  void ResolveOrReject(T value, ResolutionState new_state) {
    if (state_ != kPending || !GetScriptState()->ContextIsValid() ||
        !GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
      return;
    DCHECK(new_state == kResolving || new_state == kRejecting);
    state_ = new_state;

    ScriptState::Scope scope(script_state_.get());

    // Calling ToV8 in a ScriptForbiddenScope will trigger a CHECK and
    // cause a crash. ToV8 just invokes a constructor for wrapper creation,
    // which is safe (no author script can be run). Adding AllowUserAgentScript
    // directly inside createWrapper could cause a perf impact (calling
    // isMainThread() every time a wrapper is created is expensive). Ideally,
    // resolveOrReject shouldn't be called inside a ScriptForbiddenScope.
    {
      ScriptForbiddenScope::AllowUserAgentScript allow_script;
      value_.Set(script_state_->GetIsolate(),
                 ToV8(value, script_state_->GetContext()->Global(),
                      script_state_->GetIsolate()));
    }

    if (GetExecutionContext()->IsContextPaused()) {
      // Retain this object until it is actually resolved or rejected.
      KeepAliveWhilePending();
      return;
    }
    // TODO(esprehn): This is a hack, instead we should CHECK that
    // script is allowed, and v8 should be running the entry hooks below and
    // crashing if script is forbidden. We should then audit all users of
    // ScriptPromiseResolver and the related specs and switch to an async
    // resolve.
    // See: http://crbug.com/663476
    if (ScriptForbiddenScope::IsScriptForbidden()) {
      // Retain this object until it is actually resolved or rejected.
      KeepAliveWhilePending();
      timer_.StartOneShot(TimeDelta(), FROM_HERE);
      return;
    }
    ResolveOrRejectImmediately();
  }

  void ResolveOrRejectImmediately();
  void OnTimerFired(TimerBase*);

  ResolutionState state_;
  const scoped_refptr<ScriptState> script_state_;
  TaskRunnerTimer<ScriptPromiseResolver> timer_;
  Resolver resolver_;
  ScopedPersistent<v8::Value> value_;

  // To support keepAliveWhilePending(), this object needs to keep itself
  // alive while in that state.
  SelfKeepAlive<ScriptPromiseResolver> keep_alive_;

#if DCHECK_IS_ON()
  // True if promise() is called.
  bool is_promise_called_ = false;
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_RESOLVER_H_
