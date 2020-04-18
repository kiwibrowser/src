/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_throw_dom_exception.h"

namespace blink {

void ExceptionState::ThrowDOMException(ExceptionCode ec, const char* message) {
  ThrowDOMException(ec, String(message));
}

void ExceptionState::ThrowRangeError(const char* message) {
  ThrowRangeError(String(message));
}

void ExceptionState::ThrowSecurityError(const char* sanitized_message,
                                        const char* unsanitized_message) {
  ThrowSecurityError(String(sanitized_message), String(unsanitized_message));
}

void ExceptionState::ThrowTypeError(const char* message) {
  ThrowTypeError(String(message));
}

void ExceptionState::ThrowDOMException(ExceptionCode ec,
                                       const String& message) {
  // SecurityError is thrown via ::throwSecurityError, and _careful_
  // consideration must be given to the data exposed to JavaScript via the
  // 'sanitizedMessage'.
  DCHECK(ec != kSecurityError);

  const String& processed_message = AddExceptionContext(message);
  SetException(
      ec, processed_message,
      V8ThrowDOMException::CreateDOMException(isolate_, ec, processed_message));
}

void ExceptionState::ThrowRangeError(const String& message) {
  SetException(kV8RangeError, message,
               V8ThrowException::CreateRangeError(
                   isolate_, AddExceptionContext(message)));
}

void ExceptionState::ThrowSecurityError(const String& sanitized_message,
                                        const String& unsanitized_message) {
  const String& final_sanitized = AddExceptionContext(sanitized_message);
  const String& final_unsanitized = AddExceptionContext(unsanitized_message);
  SetException(
      kSecurityError, final_sanitized,
      V8ThrowDOMException::CreateDOMException(
          isolate_, kSecurityError, final_sanitized, final_unsanitized));
}

void ExceptionState::ThrowTypeError(const String& message) {
  SetException(kV8TypeError, message,
               V8ThrowException::CreateTypeError(isolate_,
                                                 AddExceptionContext(message)));
}

void ExceptionState::RethrowV8Exception(v8::Local<v8::Value> value) {
  SetException(kRethrownException, String(), value);
}

void ExceptionState::ClearException() {
  code_ = 0;
  message_ = String();
  exception_.Clear();
}

ScriptPromise ExceptionState::Reject(ScriptState* script_state) {
  ScriptPromise promise = ScriptPromise::Reject(script_state, GetException());
  ClearException();
  return promise;
}

void ExceptionState::Reject(ScriptPromiseResolver* resolver) {
  resolver->Reject(GetException());
  ClearException();
}

void ExceptionState::SetException(ExceptionCode ec,
                                  const String& message,
                                  v8::Local<v8::Value> exception) {
  CHECK(ec);

  code_ = ec;
  message_ = message;
  if (exception.IsEmpty()) {
    exception_.Clear();
  } else {
    DCHECK(isolate_);
    exception_.Set(isolate_, exception);
  }
}

String ExceptionState::AddExceptionContext(const String& message) const {
  if (message.IsEmpty())
    return message;

  String processed_message = message;
  if (PropertyName() && InterfaceName() && context_ != kUnknownContext) {
    if (context_ == kDeletionContext)
      processed_message = ExceptionMessages::FailedToDelete(
          PropertyName(), InterfaceName(), message);
    else if (context_ == kExecutionContext)
      processed_message = ExceptionMessages::FailedToExecute(
          PropertyName(), InterfaceName(), message);
    else if (context_ == kGetterContext)
      processed_message = ExceptionMessages::FailedToGet(
          PropertyName(), InterfaceName(), message);
    else if (context_ == kSetterContext)
      processed_message = ExceptionMessages::FailedToSet(
          PropertyName(), InterfaceName(), message);
  } else if (!PropertyName() && InterfaceName()) {
    if (context_ == kConstructionContext)
      processed_message =
          ExceptionMessages::FailedToConstruct(InterfaceName(), message);
    else if (context_ == kEnumerationContext)
      processed_message =
          ExceptionMessages::FailedToEnumerate(InterfaceName(), message);
    else if (context_ == kIndexedDeletionContext)
      processed_message =
          ExceptionMessages::FailedToDeleteIndexed(InterfaceName(), message);
    else if (context_ == kIndexedGetterContext)
      processed_message =
          ExceptionMessages::FailedToGetIndexed(InterfaceName(), message);
    else if (context_ == kIndexedSetterContext)
      processed_message =
          ExceptionMessages::FailedToSetIndexed(InterfaceName(), message);
  }
  return processed_message;
}

NonThrowableExceptionState::NonThrowableExceptionState()
    : ExceptionState(nullptr,
                     ExceptionState::kUnknownContext,
                     nullptr,
                     nullptr),
      file_(""),
      line_(0) {}

NonThrowableExceptionState::NonThrowableExceptionState(const char* file,
                                                       int line)
    : ExceptionState(nullptr,
                     ExceptionState::kUnknownContext,
                     nullptr,
                     nullptr),
      file_(file),
      line_(line) {}

void NonThrowableExceptionState::ThrowDOMException(ExceptionCode ec,
                                                   const String& message) {
  DCHECK_AT(false, file_, line_) << "DOMExeption should not be thrown.";
}

void NonThrowableExceptionState::ThrowRangeError(const String& message) {
  DCHECK_AT(false, file_, line_) << "RangeError should not be thrown.";
}

void NonThrowableExceptionState::ThrowSecurityError(
    const String& sanitized_message,
    const String&) {
  DCHECK_AT(false, file_, line_) << "SecurityError should not be thrown.";
}

void NonThrowableExceptionState::ThrowTypeError(const String& message) {
  DCHECK_AT(false, file_, line_) << "TypeError should not be thrown.";
}

void NonThrowableExceptionState::RethrowV8Exception(v8::Local<v8::Value>) {
  DCHECK_AT(false, file_, line_) << "An exception should not be rethrown.";
}

void DummyExceptionStateForTesting::ThrowDOMException(ExceptionCode ec,
                                                      const String& message) {
  SetException(ec, message, v8::Local<v8::Value>());
}

void DummyExceptionStateForTesting::ThrowRangeError(const String& message) {
  SetException(kV8RangeError, message, v8::Local<v8::Value>());
}

void DummyExceptionStateForTesting::ThrowSecurityError(
    const String& sanitized_message,
    const String&) {
  SetException(kSecurityError, sanitized_message, v8::Local<v8::Value>());
}

void DummyExceptionStateForTesting::ThrowTypeError(const String& message) {
  SetException(kV8TypeError, message, v8::Local<v8::Value>());
}

void DummyExceptionStateForTesting::RethrowV8Exception(v8::Local<v8::Value>) {
  SetException(kRethrownException, String(), v8::Local<v8::Value>());
}

}  // namespace blink
