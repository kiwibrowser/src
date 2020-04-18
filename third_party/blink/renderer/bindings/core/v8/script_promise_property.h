// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_PROPERTY_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_PROPERTY_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_property_base.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class ExecutionContext;

// ScriptPromiseProperty is a helper for implementing a DOM method or
// attribute whose value is a Promise, and the same Promise must be
// returned each time.
//
// ScriptPromiseProperty does not keep Promises or worlds alive to
// deliver Promise resolution/rejection to them; the Promise
// resolution/rejections are delivered if the holder's wrapper is
// alive. This is achieved by keeping a weak reference from
// ScriptPromiseProperty to the holder's wrapper, and references in
// hidden values from the wrapper to the promise and resolver
// (coincidentally the Resolver and Promise may be the same object,
// but that is an implementation detail of v8.)
//
//                                             ----> Resolver
//                                            /
// ScriptPromiseProperty - - -> Holder Wrapper ----> Promise
//
// To avoid exposing the action of the garbage collector to script,
// you should keep the wrapper alive as long as a promise may be
// settled.
//
// To avoid clobbering hidden values, a holder should only have one
// ScriptPromiseProperty object for a given name at a time. See reset.
template <typename HolderType, typename ResolvedType, typename RejectedType>
class ScriptPromiseProperty : public ScriptPromisePropertyBase {
  WTF_MAKE_NONCOPYABLE(ScriptPromiseProperty);

 public:
  // Creates a ScriptPromiseProperty that will create Promises in
  // the specified ExecutionContext for a property of 'holder'
  // (typically ScriptPromiseProperty should be a member of the
  // property holder).
  //
  // When implementing a ScriptPromiseProperty add the property name
  // to ScriptPromiseProperties.h and pass
  // ScriptPromiseProperty::Foo to create. The name must be unique
  // per kind of holder.
  template <typename PassHolderType>
  ScriptPromiseProperty(ExecutionContext*, PassHolderType, Name);

  ~ScriptPromiseProperty() override = default;

  template <typename PassResolvedType>
  void Resolve(PassResolvedType);

  void ResolveWithUndefined();

  template <typename PassRejectedType>
  void Reject(PassRejectedType);

  // Resets this property by unregistering the Promise property from the
  // holder wrapper. Resets the internal state to Pending and clears the
  // resolved and the rejected values.
  // This method keeps the holder object and the property name.
  void Reset();

  void Trace(blink::Visitor*) override;

 private:
  v8::Local<v8::Object> Holder(v8::Isolate*,
                               v8::Local<v8::Object> creation_context) override;
  v8::Local<v8::Value> ResolvedValue(
      v8::Isolate*,
      v8::Local<v8::Object> creation_context) override;
  v8::Local<v8::Value> RejectedValue(
      v8::Isolate*,
      v8::Local<v8::Object> creation_context) override;

  HolderType holder_;
  ResolvedType resolved_;
  RejectedType rejected_;
  bool resolved_with_undefined_ = false;
};

template <typename HolderType, typename ResolvedType, typename RejectedType>
template <typename PassHolderType>
ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::
    ScriptPromiseProperty(ExecutionContext* execution_context,
                          PassHolderType holder,
                          Name name)
    : ScriptPromisePropertyBase(execution_context, name), holder_(holder) {}

template <typename HolderType, typename ResolvedType, typename RejectedType>
template <typename PassResolvedType>
void ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::Resolve(
    PassResolvedType value) {
  if (GetState() != kPending) {
    NOTREACHED();
    return;
  }
  CHECK(!ScriptForbiddenScope::IsScriptForbidden());
  if (!GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
    return;
  resolved_ = value;
  ResolveOrReject(kResolved);
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
void ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::
    ResolveWithUndefined() {
  if (GetState() != kPending) {
    NOTREACHED();
    return;
  }
  if (!GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
    return;
  resolved_with_undefined_ = true;
  ResolveOrReject(kResolved);
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
template <typename PassRejectedType>
void ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::Reject(
    PassRejectedType value) {
  if (GetState() != kPending) {
    NOTREACHED();
    return;
  }
  if (!GetExecutionContext() || GetExecutionContext()->IsContextDestroyed())
    return;
  rejected_ = value;
  ResolveOrReject(kRejected);
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
v8::Local<v8::Object>
ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::Holder(
    v8::Isolate* isolate,
    v8::Local<v8::Object> creation_context) {
  v8::Local<v8::Value> value = ToV8(holder_, creation_context, isolate);
  if (value.IsEmpty())
    return v8::Local<v8::Object>();
  return value.As<v8::Object>();
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
v8::Local<v8::Value>
ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::ResolvedValue(
    v8::Isolate* isolate,
    v8::Local<v8::Object> creation_context) {
  DCHECK_EQ(GetState(), kResolved);
  if (!resolved_with_undefined_)
    return ToV8(resolved_, creation_context, isolate);
  return v8::Undefined(isolate);
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
v8::Local<v8::Value>
ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::RejectedValue(
    v8::Isolate* isolate,
    v8::Local<v8::Object> creation_context) {
  DCHECK_EQ(GetState(), kRejected);
  return ToV8(rejected_, creation_context, isolate);
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
void ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::Reset() {
  ResetBase();
  resolved_ = ResolvedType();
  rejected_ = RejectedType();
  resolved_with_undefined_ = false;
}

template <typename HolderType, typename ResolvedType, typename RejectedType>
void ScriptPromiseProperty<HolderType, ResolvedType, RejectedType>::Trace(
    Visitor* visitor) {
  TraceIfNeeded<HolderType>::Trace(visitor, holder_);
  TraceIfNeeded<ResolvedType>::Trace(visitor, resolved_);
  TraceIfNeeded<RejectedType>::Trace(visitor, rejected_);
  ScriptPromisePropertyBase::Trace(visitor);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_SCRIPT_PROMISE_PROPERTY_H_
