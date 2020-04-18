// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CONTENT_SETTING_CALLBACKS_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CONTENT_SETTING_CALLBACKS_H_

#include "third_party/blink/public/platform/web_private_ptr.h"

#if INSIDE_BLINK
#include <memory>
#endif

namespace blink {

class ContentSettingCallbacks;
class WebContentSettingCallbacksPrivate;

class WebContentSettingCallbacks {
 public:
  ~WebContentSettingCallbacks() { Reset(); }
  WebContentSettingCallbacks() = default;
  WebContentSettingCallbacks(const WebContentSettingCallbacks& c) { Assign(c); }
  WebContentSettingCallbacks& operator=(const WebContentSettingCallbacks& c) {
    Assign(c);
    return *this;
  }

  BLINK_PLATFORM_EXPORT void Reset();
  BLINK_PLATFORM_EXPORT void Assign(const WebContentSettingCallbacks&);

#if INSIDE_BLINK
  BLINK_PLATFORM_EXPORT WebContentSettingCallbacks(
      std::unique_ptr<ContentSettingCallbacks>&&);
#endif

  BLINK_PLATFORM_EXPORT void DoAllow();
  BLINK_PLATFORM_EXPORT void DoDeny();

 private:
  WebPrivatePtr<WebContentSettingCallbacksPrivate> private_;
};

}  // namespace blink

#endif
