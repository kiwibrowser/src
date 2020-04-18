// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_BINDING_FOR_MODULES_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_BINDING_FOR_MODULES_H_

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/modules/webdatabase/sqlite/sql_value.h"

namespace blink {

class IDBKey;
class IDBKeyPath;
class IDBKeyRange;
class IDBValue;
class SerializedScriptValue;
class WebBlobInfo;

// Exposed for unit testing:
MODULES_EXPORT v8::Local<v8::Value> DeserializeIDBValue(
    v8::Isolate*,
    v8::Local<v8::Object> creation_context,
    const IDBValue*);
MODULES_EXPORT bool InjectV8KeyIntoV8Value(v8::Isolate*,
                                           v8::Local<v8::Value> key,
                                           v8::Local<v8::Value>,
                                           const IDBKeyPath&);

// For use by Source/modules/indexeddb (and unit testing):
MODULES_EXPORT bool CanInjectIDBKeyIntoScriptValue(v8::Isolate*,
                                                   const ScriptValue&,
                                                   const IDBKeyPath&);
ScriptValue DeserializeScriptValue(ScriptState*,
                                   SerializedScriptValue*,
                                   const Vector<WebBlobInfo>*,
                                   bool read_wasm_from_stream);

#if DCHECK_IS_ON()
void AssertPrimaryKeyValidOrInjectable(ScriptState*, const IDBValue*);
#endif

template <>
struct NativeValueTraits<SQLValue> {
  static SQLValue NativeValue(v8::Isolate*,
                              v8::Local<v8::Value>,
                              ExceptionState&);
};

template <>
struct NativeValueTraits<std::unique_ptr<IDBKey>> {
  static std::unique_ptr<IDBKey> NativeValue(v8::Isolate*,
                                             v8::Local<v8::Value>,
                                             ExceptionState&);
  MODULES_EXPORT static std::unique_ptr<IDBKey> NativeValue(
      v8::Isolate*,
      v8::Local<v8::Value>,
      ExceptionState&,
      const IDBKeyPath&);
};

template <>
struct NativeValueTraits<IDBKeyRange*> {
  static IDBKeyRange* NativeValue(v8::Isolate*,
                                  v8::Local<v8::Value>,
                                  ExceptionState&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_BINDING_FOR_MODULES_H_
