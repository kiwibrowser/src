// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_THROW_DOM_EXCEPTION_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_THROW_DOM_EXCEPTION_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "v8/include/v8.h"

namespace blink {

// Provides utility functions to create and/or throw DOM Exceptions.
class CORE_EXPORT V8ThrowDOMException {
  STATIC_ONLY(V8ThrowDOMException);

 public:
  // Creates and returns an exception object, or returns an empty handle if
  // failed. |unsanitizedMessage| should not be specified unless it's
  // SecurityError.
  static v8::Local<v8::Value> CreateDOMException(
      v8::Isolate*,
      ExceptionCode,
      const String& sanitized_message,
      const String& unsanitized_message = String());

  // Creates and throws an exception object, or does nothing if creation of the
  // DOMException fails. |unsanitizedMessage| should not be specified unless
  // it's SecurityError.
  static void ThrowDOMException(v8::Isolate*,
                                ExceptionCode,
                                const String& sanitized_message,
                                const String& unsanitized_message = String());
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_THROW_DOM_EXCEPTION_H_
