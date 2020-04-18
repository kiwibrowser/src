// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/binding_access_checker.h"

#include "base/strings/stringprintf.h"
#include "gin/converter.h"

namespace extensions {

BindingAccessChecker::BindingAccessChecker(
    const AvailabilityCallback& is_available)
    : is_available_(is_available) {}
BindingAccessChecker::~BindingAccessChecker() {}

bool BindingAccessChecker::HasAccess(v8::Local<v8::Context> context,
                                     const std::string& full_name) const {
  return is_available_.Run(context, full_name);
}

bool BindingAccessChecker::HasAccessOrThrowError(
    v8::Local<v8::Context> context,
    const std::string& full_name) const {
  if (!HasAccess(context, full_name)) {
    context->GetIsolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        context->GetIsolate(),
        base::StringPrintf("'%s' is not available in this context.",
                           full_name.c_str()))));
    return false;
  }

  return true;
}

}  // namespace extensions
