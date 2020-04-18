// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_EVENT_LISTENER_PROPERTIES_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_EVENT_LISTENER_PROPERTIES_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

enum class WebEventListenerClass {
  kTouchStartOrMove,  // This value includes "touchstart", "touchmove" and
                      // "pointer" events.
  kMouseWheel,        // This value includes "wheel" and "mousewheel" events.
  kTouchEndOrCancel,  // This value includes "touchend", "touchcancel" events.
};

// Indicates the variety of event listener types for a given
// WebEventListenerClass.
enum class WebEventListenerProperties {
  kNothing,             // This should be "None"; but None #defined in X11's X.h
  kPassive,             // This indicates solely passive listeners.
  kBlocking,            // This indicates solely blocking listeners.
  kBlockingAndPassive,  // This indicates >= 1 blocking listener and >= 1
                        // passive listeners.
};

}  // namespace blink

#endif
