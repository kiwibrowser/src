// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_ERROR_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_ERROR_H_

#include "third_party/blink/public/platform/web_string.h"

namespace blink {

struct WebPushError {
  enum ErrorType {
    kErrorTypeAbort = 0,
    kErrorTypeNetwork,
    kErrorTypeNone,
    kErrorTypeNotAllowed,
    kErrorTypeNotFound,
    kErrorTypeNotSupported,
    kErrorTypeInvalidState,
    kErrorTypeLast = kErrorTypeInvalidState
  };

  WebPushError(ErrorType error_type, const WebString& message)
      : error_type(error_type), message(message) {}

  ErrorType error_type;
  WebString message;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_ERROR_H_
