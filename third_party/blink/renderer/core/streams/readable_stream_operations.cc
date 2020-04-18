// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/streams/readable_stream_operations.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/streams/underlying_source_base.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

ScriptValue ReadableStreamOperations::CreateReadableStream(
    ScriptState* script_state,
    UnderlyingSourceBase* underlying_source,
    ScriptValue strategy) {
  ScriptState::Scope scope(script_state);

  v8::Local<v8::Value> js_underlying_source =
      ToV8(underlying_source, script_state);
  v8::Local<v8::Value> js_strategy = strategy.V8Value();
  v8::Local<v8::Value> args[] = {js_underlying_source, js_strategy};
  return ScriptValue(
      script_state,
      V8ScriptRunner::CallExtraOrCrash(
          script_state, "createReadableStreamWithExternalController", args));
}

ScriptValue ReadableStreamOperations::CreateCountQueuingStrategy(
    ScriptState* script_state,
    size_t high_water_mark) {
  ScriptState::Scope scope(script_state);

  v8::Local<v8::Value> args[] = {
      v8::Number::New(script_state->GetIsolate(), high_water_mark)};
  return ScriptValue(
      script_state,
      V8ScriptRunner::CallExtraOrCrash(
          script_state, "createBuiltInCountQueuingStrategy", args));
}

ScriptValue ReadableStreamOperations::GetReader(ScriptState* script_state,
                                                ScriptValue stream,
                                                ExceptionState& es) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::TryCatch block(script_state->GetIsolate());
  v8::Local<v8::Value> args[] = {stream.V8Value()};
  ScriptValue result(
      script_state,
      V8ScriptRunner::CallExtra(script_state,
                                "AcquireReadableStreamDefaultReader", args));
  if (block.HasCaught())
    es.RethrowV8Exception(block.Exception());
  return result;
}

bool ReadableStreamOperations::IsReadableStream(ScriptState* script_state,
                                                ScriptValue value) {
  DCHECK(!value.IsEmpty());

  if (!value.IsObject())
    return false;

  v8::Local<v8::Value> args[] = {value.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state, "IsReadableStream",
                                          args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsDisturbed(ScriptState* script_state,
                                           ScriptValue stream) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamDisturbed", args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsLocked(ScriptState* script_state,
                                        ScriptValue stream) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamLocked", args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsReadable(ScriptState* script_state,
                                          ScriptValue stream) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamReadable", args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsClosed(ScriptState* script_state,
                                        ScriptValue stream) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamClosed", args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsErrored(ScriptState* script_state,
                                         ScriptValue stream) {
  DCHECK(IsReadableStream(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamErrored", args)
      ->ToBoolean()
      ->Value();
}

bool ReadableStreamOperations::IsReadableStreamDefaultReader(
    ScriptState* script_state,
    ScriptValue value) {
  DCHECK(!value.IsEmpty());

  if (!value.IsObject())
    return false;

  v8::Local<v8::Value> args[] = {value.V8Value()};
  return V8ScriptRunner::CallExtraOrCrash(script_state,
                                          "IsReadableStreamDefaultReader", args)
      ->ToBoolean()
      ->Value();
}

ScriptPromise ReadableStreamOperations::DefaultReaderRead(
    ScriptState* script_state,
    ScriptValue reader) {
  DCHECK(IsReadableStreamDefaultReader(script_state, reader));

  v8::Local<v8::Value> args[] = {reader.V8Value()};
  return ScriptPromise::Cast(
      script_state, V8ScriptRunner::CallExtraOrCrash(
                        script_state, "ReadableStreamDefaultReaderRead", args));
}

void ReadableStreamOperations::Tee(ScriptState* script_state,
                                   ScriptValue stream,
                                   ScriptValue* new_stream1,
                                   ScriptValue* new_stream2) {
  DCHECK(IsReadableStream(script_state, stream));
  DCHECK(!IsLocked(script_state, stream));

  v8::Local<v8::Value> args[] = {stream.V8Value()};

  ScriptValue result(script_state,
                     V8ScriptRunner::CallExtraOrCrash(
                         script_state, "ReadableStreamTee", args));
  DCHECK(result.V8Value()->IsArray());
  v8::Local<v8::Array> branches = result.V8Value().As<v8::Array>();
  DCHECK_EQ(2u, branches->Length());

  *new_stream1 = ScriptValue(
      script_state,
      branches->Get(script_state->GetContext(), 0).ToLocalChecked());
  *new_stream2 = ScriptValue(
      script_state,
      branches->Get(script_state->GetContext(), 1).ToLocalChecked());

  DCHECK(IsReadableStream(script_state, *new_stream1));
  DCHECK(IsReadableStream(script_state, *new_stream2));
}

}  // namespace blink
