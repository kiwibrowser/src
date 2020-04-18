// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_OPERATIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_OPERATIONS_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "v8/include/v8.h"

namespace blink {

class UnderlyingSourceBase;
class ExceptionState;
class ScriptState;

// This class has various methods for ReadableStream[Reader] implemented with
// V8 Extras.
// All methods should be called in an appropriate V8 context. All ScriptValue
// arguments must not be empty.
class CORE_EXPORT ReadableStreamOperations {
  STATIC_ONLY(ReadableStreamOperations);

 public:
  // createReadableStreamWithExternalController
  // If the caller supplies an invalid strategy (e.g. one that returns
  // negative sizes, or doesn't have appropriate properties), this will crash.
  static ScriptValue CreateReadableStream(ScriptState*,
                                          UnderlyingSourceBase*,
                                          ScriptValue strategy);

  // createBuiltInCountQueuingStrategy
  static ScriptValue CreateCountQueuingStrategy(ScriptState*,
                                                size_t high_water_mark);

  // AcquireReadableStreamDefaultReader
  // This function assumes |isReadableStream(stream)|.
  // Returns an empty value and throws an error via the ExceptionState when
  // errored.
  static ScriptValue GetReader(ScriptState*,
                               ScriptValue stream,
                               ExceptionState&);

  // IsReadableStream
  static bool IsReadableStream(ScriptState*, ScriptValue);

  // IsReadableStreamDisturbed
  // This function assumes |isReadableStream(stream)|.
  static bool IsDisturbed(ScriptState*, ScriptValue stream);

  // IsReadableStreamLocked
  // This function assumes |isReadableStream(stream)|.
  static bool IsLocked(ScriptState*, ScriptValue stream);

  // IsReadableStreamReadable
  // This function assumes |isReadableStream(stream)|.
  static bool IsReadable(ScriptState*, ScriptValue stream);

  // IsReadableStreamClosed
  // This function assumes |isReadableStream(stream)|.
  static bool IsClosed(ScriptState*, ScriptValue stream);

  // IsReadableStreamErrored
  // This function assumes |isReadableStream(stream)|.
  static bool IsErrored(ScriptState*, ScriptValue stream);

  // IsReadableStreamDefaultReader
  static bool IsReadableStreamDefaultReader(ScriptState*, ScriptValue);

  // ReadableStreamDefaultReaderRead
  // This function assumes |isReadableStreamDefaultReader(reader)|.
  static ScriptPromise DefaultReaderRead(ScriptState*, ScriptValue reader);

  // ReadableStreamTee
  // This function assumes |isReadableStream(stream)| and |!isLocked(stream)|
  static void Tee(ScriptState*,
                  ScriptValue stream,
                  ScriptValue* new_stream1,
                  ScriptValue* new_stream2);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STREAMS_READABLE_STREAM_OPERATIONS_H_
