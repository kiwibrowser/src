// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_BINDING_FOR_TESTING_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_BINDING_FOR_TESTING_H_

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "v8/include/v8.h"

namespace blink {

class Document;
class DOMWrapperWorld;
class DummyPageHolder;
class ExecutionContext;
class LocalFrame;
class Page;

class ScriptStateForTesting : public ScriptState {
 public:
  static scoped_refptr<ScriptStateForTesting> Create(
      v8::Local<v8::Context>,
      scoped_refptr<DOMWrapperWorld>);

 private:
  ScriptStateForTesting(v8::Local<v8::Context>, scoped_refptr<DOMWrapperWorld>);
};

class V8TestingScope {
  STACK_ALLOCATED();

 public:
  V8TestingScope();
  ScriptState* GetScriptState() const;
  ExecutionContext* GetExecutionContext() const;
  v8::Isolate* GetIsolate() const;
  v8::Local<v8::Context> GetContext() const;
  ExceptionState& GetExceptionState();
  Page& GetPage();
  LocalFrame& GetFrame();
  Document& GetDocument();
  ~V8TestingScope();

 private:
  std::unique_ptr<DummyPageHolder> holder_;
  v8::HandleScope handle_scope_;
  v8::Local<v8::Context> context_;
  v8::Context::Scope context_scope_;
  v8::TryCatch try_catch_;
  DummyExceptionStateForTesting exception_state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_BINDING_FOR_TESTING_H_
