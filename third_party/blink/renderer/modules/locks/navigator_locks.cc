// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/locks/navigator_locks.h"

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/modules/locks/lock_manager.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

namespace {

template <typename T>
class NavigatorLocksImpl final : public GarbageCollected<NavigatorLocksImpl<T>>,
                                 public Supplement<T>,
                                 public TraceWrapperBase {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorLocksImpl);

 public:
  static const char kSupplementName[];

  static NavigatorLocksImpl& From(T& navigator) {
    NavigatorLocksImpl* supplement = static_cast<NavigatorLocksImpl*>(
        Supplement<T>::template From<NavigatorLocksImpl>(navigator));
    if (!supplement) {
      supplement = new NavigatorLocksImpl(navigator);
      Supplement<T>::ProvideTo(navigator, supplement);
    }
    return *supplement;
  }

  LockManager* GetLockManager(ExecutionContext* context) const {
    if (!lock_manager_ && context) {
      lock_manager_ = new LockManager(context);
    }
    return lock_manager_.Get();
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(lock_manager_);
    Supplement<T>::Trace(visitor);
  }

  // Wrapper tracing is needed for callbacks. The reference chain is
  // NavigatorLocksImpl -> LockManager -> LockRequestImpl ->
  // V8LockGrantedCallback.
  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    visitor->TraceWrappers(lock_manager_);
  }

  const char* NameInHeapSnapshot() const override {
    return "NavigatorLocksImpl";
  }

 private:
  explicit NavigatorLocksImpl(T& navigator) : Supplement<T>(navigator) {}

  mutable TraceWrapperMember<LockManager> lock_manager_;
};

// static
template <typename T>
const char NavigatorLocksImpl<T>::kSupplementName[] = "NavigatorLocksImpl";

}  // namespace

LockManager* NavigatorLocks::locks(ScriptState* script_state,
                                   Navigator& navigator) {
  return NavigatorLocksImpl<Navigator>::From(navigator).GetLockManager(
      ExecutionContext::From(script_state));
}

LockManager* NavigatorLocks::locks(ScriptState* script_state,
                                   WorkerNavigator& navigator) {
  return NavigatorLocksImpl<WorkerNavigator>::From(navigator).GetLockManager(
      ExecutionContext::From(script_state));
}

}  // namespace blink
