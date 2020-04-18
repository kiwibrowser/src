// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_DEFAULT_CONTROLLER_WRAPPER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_DEFAULT_CONTROLLER_WRAPPER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/scoped_persistent.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "v8/include/v8.h"

namespace blink {

class CORE_EXPORT ReadableStreamDefaultControllerWrapper final
    : public GarbageCollectedFinalized<ReadableStreamDefaultControllerWrapper> {
 public:
  void Trace(blink::Visitor* visitor) {}

  explicit ReadableStreamDefaultControllerWrapper(ScriptValue controller)
      : script_state_(controller.GetScriptState()),
        js_controller_(controller.GetIsolate(), controller.V8Value()) {
    js_controller_.SetPhantom();
  }

  // Users of the ReadableStreamDefaultControllerWrapper can call this to note
  // that the stream has been canceled and thus they don't anticipate using the
  // ReadableStreamDefaultControllerWrapper anymore.
  // (close/desiredSize/enqueue/error will become no-ops afterward.)
  void NoteHasBeenCanceled() { js_controller_.Clear(); }

  bool IsActive() const { return !js_controller_.IsEmpty(); }

  void Close() {
    ScriptState* script_state = script_state_.get();
    // This will assert that the context is valid; do not call this method when
    // the context is invalidated.
    ScriptState::Scope scope(script_state);
    v8::Isolate* isolate = script_state->GetIsolate();

    v8::Local<v8::Value> controller = js_controller_.NewLocal(isolate);
    if (controller.IsEmpty())
      return;

    v8::Local<v8::Value> args[] = {controller};
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallExtra(
        script_state, "ReadableStreamDefaultControllerClose", args);
    js_controller_.Clear();
    result.ToLocalChecked();
  }

  double DesiredSize() const {
    ScriptState* script_state = script_state_.get();
    // This will assert that the context is valid; do not call this method when
    // the context is invalidated.
    ScriptState::Scope scope(script_state);
    v8::Isolate* isolate = script_state->GetIsolate();

    v8::Local<v8::Value> controller = js_controller_.NewLocal(isolate);
    if (controller.IsEmpty())
      return 0;

    v8::Local<v8::Value> args[] = {controller};
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallExtra(
        script_state, "ReadableStreamDefaultControllerGetDesiredSize", args);

    return result.ToLocalChecked().As<v8::Number>()->Value();
  }

  template <typename ChunkType>
  void Enqueue(ChunkType chunk) const {
    ScriptState* script_state = script_state_.get();
    // This will assert that the context is valid; do not call this method when
    // the context is invalidated.
    ScriptState::Scope scope(script_state);
    v8::Isolate* isolate = script_state->GetIsolate();

    v8::Local<v8::Value> controller = js_controller_.NewLocal(isolate);
    if (controller.IsEmpty())
      return;

    v8::Local<v8::Value> js_chunk = ToV8(chunk, script_state);
    v8::Local<v8::Value> args[] = {controller, js_chunk};
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallExtra(
        script_state, "ReadableStreamDefaultControllerEnqueue", args);
    result.ToLocalChecked();
  }

  template <typename ErrorType>
  void GetError(ErrorType error) {
    ScriptState* script_state = script_state_.get();
    // This will assert that the context is valid; do not call this method when
    // the context is invalidated.
    ScriptState::Scope scope(script_state);
    v8::Isolate* isolate = script_state->GetIsolate();

    v8::Local<v8::Value> controller = js_controller_.NewLocal(isolate);
    if (controller.IsEmpty())
      return;

    v8::Local<v8::Value> js_error = ToV8(error, script_state);
    v8::Local<v8::Value> args[] = {controller, js_error};
    v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallExtra(
        script_state, "ReadableStreamDefaultControllerError", args);
    js_controller_.Clear();
    result.ToLocalChecked();
  }

 private:
  scoped_refptr<ScriptState> script_state_;
  ScopedPersistent<v8::Value> js_controller_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_DEFAULT_CONTROLLER_WRAPPER_H_
