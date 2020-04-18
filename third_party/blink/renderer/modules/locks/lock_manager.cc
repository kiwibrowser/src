// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/locks/lock_manager.h"

#include <algorithm>
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_lock_granted_callback.h"
#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/modules/locks/lock.h"
#include "third_party/blink/renderer/modules/locks/lock_info.h"
#include "third_party/blink/renderer/modules/locks/lock_manager_snapshot.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

constexpr char kRequestAbortedMessage[] = "The request was aborted.";

LockInfo ToLockInfo(const mojom::blink::LockInfoPtr& record) {
  LockInfo info;
  info.setMode(Lock::ModeToString(record->mode));
  info.setName(record->name);
  info.setClientId(record->client_id);
  return info;
}

HeapVector<LockInfo> ToLockInfos(
    const Vector<mojom::blink::LockInfoPtr>& records) {
  HeapVector<LockInfo> out;
  out.ReserveInitialCapacity(records.size());
  for (const auto& record : records)
    out.push_back(ToLockInfo(record));
  return out;
}

}  // namespace

class LockManager::LockRequestImpl final
    : public GarbageCollectedFinalized<LockRequestImpl>,
      public TraceWrapperBase,
      public mojom::blink::LockRequest {
  WTF_MAKE_NONCOPYABLE(LockRequestImpl);
  EAGERLY_FINALIZE();

 public:
  LockRequestImpl(V8LockGrantedCallback* callback,
                  ScriptPromiseResolver* resolver,
                  const String& name,
                  mojom::blink::LockMode mode,
                  mojom::blink::LockRequestRequest request,
                  LockManager* manager)
      : callback_(callback),
        resolver_(resolver),
        name_(name),
        mode_(mode),
        binding_(this, std::move(request)),
        manager_(manager) {}

  ~LockRequestImpl() override = default;

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(resolver_);
    visitor->Trace(manager_);
    visitor->Trace(callback_);
  }

  // Wrapper tracing is needed for callbacks. The reference chain is
  // NavigatorLocksImpl -> LockManager -> LockRequestImpl ->
  // V8LockGrantedCallback.
  void TraceWrappers(ScriptWrappableVisitor* visitor) const override {
    visitor->TraceWrappers(callback_);
  }
  const char* NameInHeapSnapshot() const override {
    return "LockManager::LockRequestImpl";
  }

  // Called to immediately close the pipe which signals the back-end,
  // unblocking further requests, without waiting for GC finalize the object.
  void Cancel() { binding_.Close(); }

  void Abort(const String& reason) override {
    // Abort signal after acquisition should be ignored.
    if (!manager_->IsPendingRequest(this))
      return;

    manager_->RemovePendingRequest(this);
    binding_.Close();

    if (!resolver_->GetScriptState()->ContextIsValid())
      return;

    resolver_->Reject(DOMException::Create(kAbortError, reason));
  }

  void Failed() override {
    manager_->RemovePendingRequest(this);
    binding_.Close();

    ScriptState* script_state = resolver_->GetScriptState();
    if (!script_state->ContextIsValid())
      return;

    // Lock was not granted e.g. because ifAvailable was specified but
    // the lock was not available.
    ScriptState::Scope scope(script_state);
    v8::TryCatch try_catch(script_state->GetIsolate());
    v8::Maybe<ScriptValue> result = callback_->Invoke(nullptr, nullptr);
    if (try_catch.HasCaught()) {
      resolver_->Reject(try_catch.Exception());
    } else if (!result.IsNothing()) {
      resolver_->Resolve(result.FromJust());
    }
  }

  void Granted(mojom::blink::LockHandlePtr handle) override {
    DCHECK(binding_.is_bound());
    DCHECK(handle.is_bound());

    manager_->RemovePendingRequest(this);
    binding_.Close();

    ScriptState* script_state = resolver_->GetScriptState();
    if (!script_state->ContextIsValid()) {
      // If a handle was returned, it will be automatically be released.
      return;
    }

    Lock* lock =
        Lock::Create(script_state, name_, mode_, std::move(handle), manager_);
    manager_->held_locks_.insert(lock);

    ScriptState::Scope scope(script_state);
    v8::TryCatch try_catch(script_state->GetIsolate());
    v8::Maybe<ScriptValue> result = callback_->Invoke(nullptr, lock);
    if (try_catch.HasCaught()) {
      lock->HoldUntil(
          ScriptPromise::Reject(script_state, try_catch.Exception()),
          resolver_);
    } else if (!result.IsNothing()) {
      lock->HoldUntil(ScriptPromise::Cast(script_state, result.FromJust()),
                      resolver_);
    }
  }

 private:
  // Callback passed by script; invoked when the lock is granted.
  TraceWrapperMember<V8LockGrantedCallback> callback_;

  // Rejects if the request was aborted, otherwise resolves/rejects with
  // |callback_|'s result.
  Member<ScriptPromiseResolver> resolver_;

  // Held to stamp the Lock object's |name| property.
  String name_;

  // Held to stamp the Lock object's |mode| property.
  mojom::blink::LockMode mode_;

  mojo::Binding<mojom::blink::LockRequest> binding_;

  // The |manager_| keeps |this| alive until a response comes in and this is
  // registered. If the context is destroyed then |manager_| will dispose of
  // |this| which terminates the request on the service side.
  Member<LockManager> manager_;
};

LockManager::LockManager(ExecutionContext* context)
    : ContextLifecycleObserver(context) {}

ScriptPromise LockManager::request(ScriptState* script_state,
                                   const String& name,
                                   V8LockGrantedCallback* callback,
                                   ExceptionState& exception_state) {
  return request(script_state, name, LockOptions(), callback, exception_state);
}

ScriptPromise LockManager::request(ScriptState* script_state,
                                   const String& name,
                                   const LockOptions& options,
                                   V8LockGrantedCallback* callback,
                                   ExceptionState& exception_state) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  DCHECK(context->IsContextThread());

  if (!context->GetSecurityOrigin()->CanAccessLocks()) {
    exception_state.ThrowSecurityError(
        "Access to the Locks API is denied in this context.");
    return ScriptPromise();
  }
  if (context->GetSecurityOrigin()->IsLocal()) {
    UseCounter::Count(context, WebFeature::kFileAccessedLocks);
  }

  if (!service_.get()) {
    if (auto* provider = context->GetInterfaceProvider())
      provider->GetInterface(mojo::MakeRequest(&service_));
    if (!service_.get()) {
      exception_state.ThrowTypeError("Service not available.");
      return ScriptPromise();
    }
  }

  mojom::blink::LockMode mode = Lock::StringToMode(options.mode());

  if (options.steal() && options.ifAvailable()) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "The 'steal' and 'ifAvailable' options cannot be used together.");
    return ScriptPromise();
  }

  if (name.StartsWith("-")) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Names cannot start with '-'.");
    return ScriptPromise();
  }

  if (options.steal() && mode != mojom::blink::LockMode::EXCLUSIVE) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "The 'steal' option may only be used with 'exclusive' locks.");
    return ScriptPromise();
  }

  if (options.hasSignal() && options.ifAvailable()) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "The 'signal' and 'ifAvailable' options cannot be used together.");
    return ScriptPromise();
  }

  if (options.hasSignal() && options.steal()) {
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "The 'signal' and 'steal' options cannot be used together.");
    return ScriptPromise();
  }

  if (options.hasSignal() && options.signal()->aborted()) {
    exception_state.ThrowDOMException(kAbortError, kRequestAbortedMessage);
    return ScriptPromise();
  }

  mojom::blink::LockManager::WaitMode wait =
      options.steal()
          ? mojom::blink::LockManager::WaitMode::PREEMPT
          : options.ifAvailable() ? mojom::blink::LockManager::WaitMode::NO_WAIT
                                  : mojom::blink::LockManager::WaitMode::WAIT;

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  mojom::blink::LockRequestPtr request_ptr;
  LockRequestImpl* request = new LockRequestImpl(
      callback, resolver, name, mode, mojo::MakeRequest(&request_ptr), this);
  AddPendingRequest(request);

  if (options.hasSignal()) {
    options.signal()->AddAlgorithm(WTF::Bind(&LockRequestImpl::Abort,
                                             WrapWeakPersistent(request),
                                             String(kRequestAbortedMessage)));
  }

  service_->RequestLock(name, mode, wait, std::move(request_ptr));

  return promise;
}

ScriptPromise LockManager::query(ScriptState* script_state,
                                 ExceptionState& exception_state) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  DCHECK(context->IsContextThread());

  if (!context->GetSecurityOrigin()->CanAccessLocks()) {
    exception_state.ThrowSecurityError(
        "Access to the Locks API is denied in this context.");
    return ScriptPromise();
  }
  if (context->GetSecurityOrigin()->IsLocal()) {
    UseCounter::Count(context, WebFeature::kFileAccessedLocks);
  }

  if (!service_.get()) {
    if (auto* provider = context->GetInterfaceProvider())
      provider->GetInterface(mojo::MakeRequest(&service_));
    if (!service_.get()) {
      exception_state.ThrowTypeError("Service not available.");
      return ScriptPromise();
    }
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  service_->QueryState(WTF::Bind(
      [](ScriptPromiseResolver* resolver,
         Vector<mojom::blink::LockInfoPtr> pending,
         Vector<mojom::blink::LockInfoPtr> held) {
        LockManagerSnapshot snapshot;
        snapshot.setPending(ToLockInfos(pending));
        snapshot.setHeld(ToLockInfos(held));
        resolver->Resolve(snapshot);
      },
      WrapPersistent(resolver)));

  return promise;
}

void LockManager::AddPendingRequest(LockRequestImpl* request) {
  pending_requests_.insert(request);
}

void LockManager::RemovePendingRequest(LockRequestImpl* request) {
  pending_requests_.erase(request);
}

bool LockManager::IsPendingRequest(LockRequestImpl* request) {
  return pending_requests_.Contains(request);
}

void LockManager::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
  visitor->Trace(pending_requests_);
  visitor->Trace(held_locks_);
}

void LockManager::TraceWrappers(ScriptWrappableVisitor* visitor) const {
  for (auto request : pending_requests_)
    visitor->TraceWrappers(request);
  ScriptWrappable::TraceWrappers(visitor);
}

void LockManager::ContextDestroyed(ExecutionContext*) {
  for (auto request : pending_requests_)
    request->Cancel();
  pending_requests_.clear();
  held_locks_.clear();
}

void LockManager::OnLockReleased(Lock* lock) {
  // Lock may be removed by an explicit call and/or when the context is
  // destroyed, so this must be idempotent.
  held_locks_.erase(lock);
}

}  // namespace blink
