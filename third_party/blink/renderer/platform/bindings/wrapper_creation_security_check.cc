// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/wrapper_creation_security_check.h"

#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"

namespace blink {

WrapperCreationSecurityCheck::SecurityCheckFunction
    WrapperCreationSecurityCheck::security_check_ = nullptr;
WrapperCreationSecurityCheck::RethrowExceptionFunction
    WrapperCreationSecurityCheck::rethrow_exception_ = nullptr;

void WrapperCreationSecurityCheck::SetSecurityCheckFunction(
    SecurityCheckFunction func) {
  DCHECK(!security_check_);
  security_check_ = func;
}

void WrapperCreationSecurityCheck::SetRethrowExceptionFunction(
    RethrowExceptionFunction func) {
  DCHECK(!rethrow_exception_);
  rethrow_exception_ = func;
}

bool WrapperCreationSecurityCheck::VerifyContextAccess(
    v8::Local<v8::Context> creation_context,
    const WrapperTypeInfo* type) {
  return (*security_check_)(creation_context, type);
}

void WrapperCreationSecurityCheck::RethrowCrossContextException(
    v8::Local<v8::Context> creation_context,
    const WrapperTypeInfo* type,
    v8::Local<v8::Value> cross_context_exception) {
  (*rethrow_exception_)(creation_context, type, cross_context_exception);
}

}  // namespace blink
