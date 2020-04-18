// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_GAMEPAD_LISTENER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_GAMEPAD_LISTENER_H_

#include "third_party/blink/public/platform/web_platform_event_listener.h"

namespace device {
class Gamepad;
}

namespace blink {

class WebGamepadListener : public WebPlatformEventListener {
 public:
  virtual void DidConnectGamepad(unsigned index, const device::Gamepad&) = 0;
  virtual void DidDisconnectGamepad(unsigned index, const device::Gamepad&) = 0;

 protected:
  ~WebGamepadListener() override = default;
};

}  // namespace blink

#endif
