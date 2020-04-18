/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_DOM_WRAPPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_DOM_WRAPPER_H_

#include "third_party/blink/renderer/platform/bindings/dom_data_store.h"
#include "third_party/blink/renderer/platform/bindings/runtime_call_stats.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_creation_security_check.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/compiler.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "v8/include/v8.h"

namespace blink {

struct WrapperTypeInfo;

// Contains utility methods to create wrappers, associate ScriptWrappable
// objects with wrappers and set fields in wrappers.
class V8DOMWrapper {
  STATIC_ONLY(V8DOMWrapper);

 public:
  PLATFORM_EXPORT static v8::Local<v8::Object> CreateWrapper(
      v8::Isolate*,
      v8::Local<v8::Object> creation_context,
      const WrapperTypeInfo*);
  PLATFORM_EXPORT static bool IsWrapper(v8::Isolate*, v8::Local<v8::Value>);

  // Associates the given ScriptWrappable with the given |wrapper| if the
  // ScriptWrappable is not yet associated with any wrapper.  Returns the
  // wrapper already associated or |wrapper| if not yet associated.
  // The caller should always use the returned value rather than |wrapper|.
  PLATFORM_EXPORT WARN_UNUSED_RESULT static v8::Local<v8::Object>
  AssociateObjectWithWrapper(v8::Isolate*,
                             ScriptWrappable*,
                             const WrapperTypeInfo*,
                             v8::Local<v8::Object> wrapper);
  static void SetNativeInfo(v8::Isolate*,
                            v8::Local<v8::Object>,
                            const WrapperTypeInfo*,
                            ScriptWrappable*);
  static void ClearNativeInfo(v8::Isolate*, v8::Local<v8::Object>);

  // hasInternalFieldsSet only checks if the value has the internal fields for
  // wrapper obejct and type, and does not check if it's valid or not.  The
  // value may not be a Blink's wrapper object.  In order to make sure of it,
  // Use isWrapper function instead.
  PLATFORM_EXPORT static bool HasInternalFieldsSet(v8::Local<v8::Value>);
};

inline void V8DOMWrapper::SetNativeInfo(
    v8::Isolate* isolate,
    v8::Local<v8::Object> wrapper,
    const WrapperTypeInfo* wrapper_type_info,
    ScriptWrappable* script_wrappable) {
  DCHECK_GE(wrapper->InternalFieldCount(), 2);
  DCHECK(script_wrappable);
  DCHECK(wrapper_type_info);
  int indices[] = {kV8DOMWrapperObjectIndex, kV8DOMWrapperTypeIndex};
  void* values[] = {script_wrappable,
                    const_cast<WrapperTypeInfo*>(wrapper_type_info)};
  wrapper->SetAlignedPointerInInternalFields(arraysize(indices), indices,
                                             values);
  auto* per_isolate_data = V8PerIsolateData::From(isolate);
  // We notify ScriptWrappableVisitor about the new wrapper association,
  // so the visitor can make sure to trace the association (in case it is
  // currently tracing).  Because of some optimizations, V8 will not
  // necessarily detect wrappers created during its incremental marking.
  per_isolate_data->GetScriptWrappableMarkingVisitor()->RegisterV8Reference(
      std::make_pair(const_cast<WrapperTypeInfo*>(wrapper_type_info),
                     script_wrappable));
}

inline void V8DOMWrapper::ClearNativeInfo(v8::Isolate* isolate,
                                          v8::Local<v8::Object> wrapper) {
  int indices[] = {kV8DOMWrapperObjectIndex, kV8DOMWrapperTypeIndex};
  void* values[] = {nullptr, nullptr};
  wrapper->SetAlignedPointerInInternalFields(arraysize(indices), indices,
                                             values);
}

inline v8::Local<v8::Object> V8DOMWrapper::AssociateObjectWithWrapper(
    v8::Isolate* isolate,
    ScriptWrappable* impl,
    const WrapperTypeInfo* wrapper_type_info,
    v8::Local<v8::Object> wrapper) {
  RUNTIME_CALL_TIMER_SCOPE(
      isolate, RuntimeCallStats::CounterId::kAssociateObjectWithWrapper);
  if (DOMDataStore::SetWrapper(isolate, impl, wrapper_type_info, wrapper)) {
    WrapperTypeInfo::WrapperCreated();
    SetNativeInfo(isolate, wrapper, wrapper_type_info, impl);
    DCHECK(HasInternalFieldsSet(wrapper));
  }
  SECURITY_CHECK(ToScriptWrappable(wrapper) == impl);
  return wrapper;
}

class V8WrapperInstantiationScope {
  STACK_ALLOCATED();

 public:
  V8WrapperInstantiationScope(v8::Local<v8::Object> creation_context,
                              v8::Isolate* isolate,
                              const WrapperTypeInfo* type)
      : did_enter_context_(false),
        context_(isolate->GetCurrentContext()),
        try_catch_(isolate),
        type_(type),
        access_check_failed_(false) {
    // creationContext should not be empty. Because if we have an
    // empty creationContext, we will end up creating
    // a new object in the context currently entered. This is wrong.
    CHECK(!creation_context.IsEmpty());
    v8::Local<v8::Context> context_for_wrapper =
        creation_context->CreationContext();

    // For performance, we enter the context only if the currently running
    // context is different from the context that we are about to enter.
    if (context_for_wrapper == context_)
      return;

    context_ = context_for_wrapper;

    if (!WrapperCreationSecurityCheck::VerifyContextAccess(context_, type_)) {
      DCHECK(try_catch_.HasCaught());
      try_catch_.ReThrow();
      access_check_failed_ = true;
      return;
    }

    did_enter_context_ = true;
    context_->Enter();
  }

  ~V8WrapperInstantiationScope() {
    if (!did_enter_context_) {
      try_catch_.ReThrow();
      return;
    }
    context_->Exit();

    if (access_check_failed_ || !try_catch_.HasCaught())
      return;

    // Any exception caught here is a cross context exception and it may not be
    // safe to directly rethrow the exception in the current context (without
    // converting it). rethrowCrossContextException converts the exception in
    // such a scenario.
    v8::Local<v8::Value> caught_exception = try_catch_.Exception();
    try_catch_.Reset();
    WrapperCreationSecurityCheck::RethrowCrossContextException(
        context_, type_, caught_exception);
    try_catch_.ReThrow();
  }

  v8::Local<v8::Context> GetContext() const { return context_; }
  bool AccessCheckFailed() const { return access_check_failed_; }

 private:
  bool did_enter_context_;
  v8::Local<v8::Context> context_;
  v8::TryCatch try_catch_;
  const WrapperTypeInfo* type_;
  bool access_check_failed_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_DOM_WRAPPER_H_
