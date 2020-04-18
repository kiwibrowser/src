// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_API_ACCESSIBILITY_PRIVATE_ACCESSIBILITY_EXTENSION_API_H_
#define CHROMECAST_BROWSER_EXTENSIONS_API_ACCESSIBILITY_PRIVATE_ACCESSIBILITY_EXTENSION_API_H_

#include <string>

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace cast {
namespace api {

// API function that enables or disables web content accessibility support.
class AccessibilityPrivateSetNativeAccessibilityEnabledFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetNativeAccessibilityEnabledFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION(
      "accessibilityPrivate.setNativeAccessibilityEnabled",
      ACCESSIBILITY_PRIVATE_SETNATIVEACCESSIBILITYENABLED)
};

// API function that sets the location of the accessibility focus ring.
class AccessibilityPrivateSetFocusRingFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetFocusRingFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.setFocusRing",
                             ACCESSIBILITY_PRIVATE_SETFOCUSRING)
};

// API function that sets the location of the accessibility highlights.
class AccessibilityPrivateSetHighlightsFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetHighlightsFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.setHighlights",
                             ACCESSIBILITY_PRIVATE_SETHIGHLIGHTS)
};

// API function that sets keyboard capture mode.
class AccessibilityPrivateSetKeyboardListenerFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetKeyboardListenerFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.setKeyboardListener",
                             ACCESSIBILITY_PRIVATE_SETKEYBOARDLISTENER)
};

// API function that darkens or undarkens the screen.
class AccessibilityPrivateDarkenScreenFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateDarkenScreenFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.darkenScreen",
                             ACCESSIBILITY_PRIVATE_DARKENSCREEN)
};

// API function that sets the keys to be captured by Switch Access.
class AccessibilityPrivateSetSwitchAccessKeysFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetSwitchAccessKeysFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.setSwitchAccessKeys",
                             ACCESSIBILITY_PRIVATE_SETSWITCHACCESSKEYS)
};

// API function that sets native ChromeVox ARC support.
class AccessibilityPrivateSetNativeChromeVoxArcSupportForCurrentAppFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSetNativeChromeVoxArcSupportForCurrentAppFunction()
      override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION(
      "accessibilityPrivate.setNativeChromeVoxArcSupportForCurrentApp",
      ACCESSIBILITY_PRIVATE_SETNATIVECHROMEVOXARCSUPPORTFORCURRENTAPP)
};

// API function that injects key events.
class AccessibilityPrivateSendSyntheticKeyEventFunction
    : public UIThreadExtensionFunction {
  ~AccessibilityPrivateSendSyntheticKeyEventFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("accessibilityPrivate.sendSyntheticKeyEvent",
                             ACCESSIBILITY_PRIVATE_SENDSYNTHETICKEYEVENT)
};

}  // namespace api
}  // namespace cast
}  // namespace extensions

#endif  // CHROMECAST_BROWSER_EXTENSIONS_API_ACCESSIBILITY_PRIVATE_ACCESSIBILITY_EXTENSION_API_H_
