// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SET_SINK_ID_CALLBACKS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SET_SINK_ID_CALLBACKS_H_

#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {

enum class WebSetSinkIdError {
  kNotFound = 1,
  kNotAuthorized,
  kAborted,
  kNotSupported,
  kLast = kNotSupported
};

using WebSetSinkIdCallbacks = WebCallbacks<void, WebSetSinkIdError>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SET_SINK_ID_CALLBACKS_H_
