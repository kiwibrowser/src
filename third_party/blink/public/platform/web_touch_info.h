// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_TOUCH_INFO_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_TOUCH_INFO_H_

#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_touch_action.h"

namespace blink {

struct WebTouchInfo {
  WebRect rect;
  WebTouchAction touch_action;
};

}  // namespace blink

#endif
