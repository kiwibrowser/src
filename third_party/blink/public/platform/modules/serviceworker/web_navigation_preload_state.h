// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_NAVIGATION_PRELOAD_STATE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_NAVIGATION_PRELOAD_STATE_H_

#include "third_party/blink/public/platform/web_string.h"

namespace blink {

struct WebNavigationPreloadState {
  WebNavigationPreloadState(bool enabled, const WebString& header_value)
      : enabled(enabled), header_value(header_value) {}

  const bool enabled;
  const WebString header_value;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_SERVICEWORKER_WEB_NAVIGATION_PRELOAD_STATE_H_
