// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PLATFORM_EVENT_LISTENER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PLATFORM_EVENT_LISTENER_H_

namespace blink {

class WebPlatformEventListener {
 public:
  virtual ~WebPlatformEventListener() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_PLATFORM_EVENT_LISTENER_H_
