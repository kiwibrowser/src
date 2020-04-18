// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_STATE_H_

#include <memory>

#include "third_party/blink/renderer/platform/bindings/scoped_persistent.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_context_data.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "v8/include/v8.h"

namespace blink {

class DOMWrapperWorld;
class ScriptValue;

// ScriptState is an abstraction class that holds all information about script
// exectuion (e.g., v8::Isolate, v8::Context, DOMWrapperWorld, ExecutionContext
// etc). If you need any info about the script execution, you're expected to
// pass around ScriptState in the code base. ScriptState is in a 1:1
// relationship with v8::Context.
//
// When you need ScriptState, you can add [CallWith=ScriptState] to IDL files
// and pass around ScriptState into a place where you need ScriptState.
//
// In some cases, you need ScriptState in code that doesn't have any JavaScript
// on the stack. Then you can store ScriptState on a C++ object using
// scoped_refptr<ScriptState>.
//
// class SomeObject {
//   void someMethod(ScriptState* scriptState) {
//     script_state_ = scriptState; // Record the ScriptState.
//     ...;
//   }
//
//   void asynchronousMethod() {
//     if (!script_state_->contextIsValid()) {
//       // It's possible that the context is already gone.
//       return;
//     }
//     // Enter the ScriptState.
//     ScriptState::Scope scope(script_state_.get());
//     // Do V8 related things.
//     ToV8(...);
//   }
//   scoped_refptr<ScriptState> script_state_;
// };
//
// You should not store ScriptState on a C++ object that can be accessed
// by multiple worlds. For example, you can store ScriptState on
// ScriptPromiseResolver, ScriptValue etc because they can be accessed from one
// world. However, you cannot store ScriptState on a DOM object that has
// an IDL interface because the DOM object can be accessed from multiple
// worlds. If ScriptState of one world "leak"s to another world, you will
// end up with leaking any JavaScript objects from one Chrome extension
// to another Chrome extension, which is a severe security bug.
//
// Lifetime:
// ScriptState is created when v8::Context is created.
// ScriptState is destroyed when v8::Context is garbage-collected and
// all V8 proxy objects that have references to the ScriptState are destructed.
class PLATFORM_EXPORT ScriptState : public RefCounted<ScriptState> {
  WTF_MAKE_NONCOPYABLE(ScriptState);

 public:
  class Scope {
    STACK_ALLOCATED();

   public:
    // You need to make sure that scriptState->context() is not empty before
    // creating a Scope.
    explicit Scope(ScriptState* script_state)
        : handle_scope_(script_state->GetIsolate()),
          context_(script_state->GetContext()) {
      DCHECK(script_state->ContextIsValid());
      context_->Enter();
    }

    ~Scope() { context_->Exit(); }

   private:
    v8::HandleScope handle_scope_;
    v8::Local<v8::Context> context_;
  };

  static scoped_refptr<ScriptState> Create(v8::Local<v8::Context>,
                                           scoped_refptr<DOMWrapperWorld>);
  virtual ~ScriptState();

  static ScriptState* Current(v8::Isolate* isolate)  // DEPRECATED
  {
    return From(isolate->GetCurrentContext());
  }

  static ScriptState* ForCurrentRealm(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    return From(info.GetIsolate()->GetCurrentContext());
  }

  static ScriptState* ForRelevantRealm(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    return From(info.Holder()->CreationContext());
  }

  static ScriptState* ForRelevantRealm(
      const v8::PropertyCallbackInfo<v8::Value>& info) {
    return From(info.Holder()->CreationContext());
  }

  static ScriptState* ForRelevantRealm(
      const v8::PropertyCallbackInfo<void>& info) {
    return From(info.Holder()->CreationContext());
  }

  static ScriptState* From(v8::Local<v8::Context> context) {
    DCHECK(!context.IsEmpty());
    ScriptState* script_state =
        static_cast<ScriptState*>(context->GetAlignedPointerFromEmbedderData(
            kV8ContextPerContextDataIndex));
    // ScriptState::from() must not be called for a context that does not have
    // valid embedder data in the embedder field.
    SECURITY_CHECK(script_state);
    SECURITY_CHECK(script_state->context_ == context);
    return script_state;
  }

  v8::Isolate* GetIsolate() const { return isolate_; }
  DOMWrapperWorld& World() const { return *world_; }

  // This can return an empty handle if the v8::Context is gone.
  v8::Local<v8::Context> GetContext() const {
    return context_.NewLocal(isolate_);
  }
  bool ContextIsValid() const {
    return !context_.IsEmpty() && per_context_data_;
  }
  void DetachGlobalObject();
  void ClearContext() { return context_.Clear(); }

  V8PerContextData* PerContextData() const { return per_context_data_.get(); }
  void DisposePerContextData();

 protected:
  ScriptState(v8::Local<v8::Context>, scoped_refptr<DOMWrapperWorld>);

 private:
  v8::Isolate* isolate_;
  // This persistent handle is weak.
  ScopedPersistent<v8::Context> context_;

  // This RefPtr doesn't cause a cycle because all persistent handles that
  // DOMWrapperWorld holds are weak.
  scoped_refptr<DOMWrapperWorld> world_;

  // This std::unique_ptr causes a cycle:
  // V8PerContextData --(Persistent)--> v8::Context --(RefPtr)--> ScriptState
  //     --(std::unique_ptr)--> V8PerContextData
  // So you must explicitly clear the std::unique_ptr by calling
  // disposePerContextData() once you no longer need V8PerContextData.
  // Otherwise, the v8::Context will leak.
  std::unique_ptr<V8PerContextData> per_context_data_;
};

// ScriptStateProtectingContext keeps the context associated with the
// ScriptState alive.  You need to call clear() once you no longer need the
// context. Otherwise, the context will leak.
class ScriptStateProtectingContext {
  WTF_MAKE_NONCOPYABLE(ScriptStateProtectingContext);
  USING_FAST_MALLOC(ScriptStateProtectingContext);

 public:
  ScriptStateProtectingContext(ScriptState* script_state)
      : script_state_(script_state) {
    if (script_state_) {
      context_.Set(script_state_->GetIsolate(), script_state_->GetContext());
      context_.Get().AnnotateStrongRetainer(
          "blink::ScriptStateProtectingContext::context_");
    }
  }

  ScriptState* operator->() const { return script_state_.get(); }
  ScriptState* Get() const { return script_state_.get(); }
  void Clear() {
    script_state_ = nullptr;
    context_.Clear();
  }

 private:
  scoped_refptr<ScriptState> script_state_;
  ScopedPersistent<v8::Context> context_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_STATE_H_
