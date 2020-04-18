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

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_EXCEPTION_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_EXCEPTION_STATE_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/scoped_persistent.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "v8/include/v8.h"

namespace blink {

typedef int ExceptionCode;
class ScriptPromiseResolver;
class ScriptState;

// ExceptionState is a scope-like class and provides a way to throw an exception
// with an option to cancel it.  An exception message may be auto-generated.
// You can convert an exception to a reject promise.
class CORE_EXPORT ExceptionState {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(ExceptionState);

 public:
  enum ContextType {
    kConstructionContext,
    kExecutionContext,
    kDeletionContext,
    kGetterContext,
    kSetterContext,
    kEnumerationContext,
    kQueryContext,
    kIndexedGetterContext,
    kIndexedSetterContext,
    kIndexedDeletionContext,
    kUnknownContext,  // FIXME: Remove this once we've flipped over to the new
                      // API.
  };

  ExceptionState(v8::Isolate* isolate,
                 ContextType context_type,
                 const char* interface_name,
                 const char* property_name)
      : code_(0),
        context_(context_type),
        property_name_(property_name),
        interface_name_(interface_name),
        isolate_(isolate) {}

  ExceptionState(v8::Isolate* isolate,
                 ContextType context_type,
                 const char* interface_name)
      : ExceptionState(isolate, context_type, interface_name, nullptr) {
#if DCHECK_IS_ON()
    switch (context_) {
      case kConstructionContext:
      case kEnumerationContext:
      case kIndexedGetterContext:
      case kIndexedSetterContext:
      case kIndexedDeletionContext:
        break;
      default:
        NOTREACHED();
    }
#endif  // DCHECK_IS_ON()
  }

  ~ExceptionState() {
    if (!exception_.IsEmpty()) {
      V8ThrowException::ThrowException(isolate_, exception_.NewLocal(isolate_));
    }
  }

  void ThrowDOMException(ExceptionCode, const char* message);
  void ThrowRangeError(const char* message);
  void ThrowSecurityError(const char* sanitized_message,
                          const char* unsanitized_message = nullptr);
  void ThrowTypeError(const char* message);

  virtual void ThrowDOMException(ExceptionCode, const String& message);
  virtual void ThrowRangeError(const String& message);
  virtual void ThrowSecurityError(const String& sanitized_message,
                                  const String& unsanitized_message = String());
  virtual void ThrowTypeError(const String& message);
  virtual void RethrowV8Exception(v8::Local<v8::Value>);

  bool HadException() const { return code_; }
  void ClearException();

  ExceptionCode Code() const { return code_; }
  const String& Message() const { return message_; }
  v8::Local<v8::Value> GetException() {
    DCHECK(!exception_.IsEmpty());
    return exception_.NewLocal(isolate_);
  }

  // This method clears out the exception which |this| has.
  ScriptPromise Reject(ScriptState*);

  // This method clears out the exception which |this| has.
  void Reject(ScriptPromiseResolver*);

  ContextType Context() const { return context_; }
  const char* PropertyName() const { return property_name_; }
  const char* InterfaceName() const { return interface_name_; }

  String AddExceptionContext(const String&) const;

 protected:
  // An ExceptionCode for the case that an exception is rethrown.  In that
  // case, we cannot determine an exception code.
  static const int kRethrownException = kUnknownError;

  void SetException(ExceptionCode, const String&, v8::Local<v8::Value>);

 private:
  ExceptionCode code_;
  ContextType context_;
  String message_;
  const char* property_name_;
  const char* interface_name_;
  // The exception is empty when it was thrown through
  // DummyExceptionStateForTesting.
  ScopedPersistent<v8::Value> exception_;
  v8::Isolate* isolate_;
};

// NonThrowableExceptionState never allow call sites to throw an exception.
// Should be used if an exception must not be thrown.
class CORE_EXPORT NonThrowableExceptionState final : public ExceptionState {
 public:
  NonThrowableExceptionState();
  NonThrowableExceptionState(const char*, int);

  void ThrowDOMException(ExceptionCode, const String& message) override;
  void ThrowTypeError(const String& message) override;
  void ThrowSecurityError(const String& sanitized_message,
                          const String& unsanitized_message) override;
  void ThrowRangeError(const String& message) override;
  void RethrowV8Exception(v8::Local<v8::Value>) override;
  ExceptionState& ReturnThis() { return *this; }

 private:
  const char* file_;
  const int line_;
};

// Syntax sugar for NonThrowableExceptionState.
// This can be used as a default value of an ExceptionState parameter like this:
//
//     Node* removeChild(Node*, ExceptionState& = ASSERT_NO_EXCEPTION);
#if DCHECK_IS_ON()
#define ASSERT_NO_EXCEPTION \
  (::blink::NonThrowableExceptionState(__FILE__, __LINE__).ReturnThis())
#else
#define ASSERT_NO_EXCEPTION \
  (::blink::DummyExceptionStateForTesting().ReturnThis())
#endif

// DummyExceptionStateForTesting ignores all thrown exceptions. You should not
// use DummyExceptionStateForTesting in production code, where you need to
// handle all exceptions properly. If you really need to ignore exceptions in
// production code for some special reason, explicitly call clearException().
class CORE_EXPORT DummyExceptionStateForTesting final : public ExceptionState {
 public:
  DummyExceptionStateForTesting()
      : ExceptionState(nullptr,
                       ExceptionState::kUnknownContext,
                       nullptr,
                       nullptr) {}
  ~DummyExceptionStateForTesting() {
    // Prevent the base class throw an exception.
    if (HadException()) {
      ClearException();
    }
  }
  void ThrowDOMException(ExceptionCode, const String& message) override;
  void ThrowTypeError(const String& message) override;
  void ThrowSecurityError(const String& sanitized_message,
                          const String& unsanitized_message) override;
  void ThrowRangeError(const String& message) override;
  void RethrowV8Exception(v8::Local<v8::Value>) override;
  ExceptionState& ReturnThis() { return *this; }
};

// Syntax sugar for DummyExceptionStateForTesting.
// This can be used as a default value of an ExceptionState parameter like this:
//
//     Node* removeChild(Node*, ExceptionState& = IGNORE_EXCEPTION_FOR_TESTING);
#define IGNORE_EXCEPTION_FOR_TESTING \
  (::blink::DummyExceptionStateForTesting().ReturnThis())

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_EXCEPTION_STATE_H_
