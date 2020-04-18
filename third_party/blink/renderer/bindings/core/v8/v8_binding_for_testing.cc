// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

scoped_refptr<ScriptStateForTesting> ScriptStateForTesting::Create(
    v8::Local<v8::Context> context,
    scoped_refptr<DOMWrapperWorld> world) {
  scoped_refptr<ScriptStateForTesting> script_state =
      base::AdoptRef(new ScriptStateForTesting(context, std::move(world)));
  // This ref() is for keeping this ScriptState alive as long as the v8::Context
  // is alive.  This is deref()ed in the weak callback of the v8::Context.
  script_state->AddRef();
  return script_state;
}

ScriptStateForTesting::ScriptStateForTesting(
    v8::Local<v8::Context> context,
    scoped_refptr<DOMWrapperWorld> world)
    : ScriptState(context, std::move(world)) {}

V8TestingScope::V8TestingScope()
    : holder_(DummyPageHolder::Create()),
      handle_scope_(GetIsolate()),
      context_(GetScriptState()->GetContext()),
      context_scope_(GetContext()),
      try_catch_(GetIsolate()) {
  GetFrame().GetSettings()->SetScriptEnabled(true);
}

ScriptState* V8TestingScope::GetScriptState() const {
  return ToScriptStateForMainWorld(holder_->GetDocument().GetFrame());
}

ExecutionContext* V8TestingScope::GetExecutionContext() const {
  return ExecutionContext::From(GetScriptState());
}

v8::Isolate* V8TestingScope::GetIsolate() const {
  return GetScriptState()->GetIsolate();
}

v8::Local<v8::Context> V8TestingScope::GetContext() const {
  return context_;
}

ExceptionState& V8TestingScope::GetExceptionState() {
  return exception_state_;
}

Page& V8TestingScope::GetPage() {
  return holder_->GetPage();
}

LocalFrame& V8TestingScope::GetFrame() {
  return holder_->GetFrame();
}

Document& V8TestingScope::GetDocument() {
  return holder_->GetDocument();
}

V8TestingScope::~V8TestingScope() {
  // Execute all pending microtasks.
  // The document can be manually shut down here, so we cannot use GetIsolate()
  // which relies on the active document.
  v8::MicrotasksScope::PerformCheckpoint(GetContext()->GetIsolate());

  // TODO(yukishiino): We put this statement here to clear an exception from
  // the isolate.  Otherwise, the leak detector complains.  Really mysterious
  // hack.
  v8::Function::New(GetContext(), nullptr);
}

}  // namespace blink
