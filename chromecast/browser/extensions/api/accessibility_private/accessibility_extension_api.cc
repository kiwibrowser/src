// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/api/accessibility_private/accessibility_extension_api.h"

#include "chromecast/common/extensions_api/accessibility_private.h"
#include "content/public/browser/browser_accessibility_state.h"

namespace {

const char kErrorNotSupported[] = "This API is not supported on this platform.";

}  // namespace

namespace extensions {
namespace cast {
namespace api {

ExtensionFunction::ResponseAction
AccessibilityPrivateSetNativeAccessibilityEnabledFunction::Run() {
  bool enabled = false;
  EXTENSION_FUNCTION_VALIDATE(args_->GetBoolean(0, &enabled));
  if (enabled) {
    content::BrowserAccessibilityState::GetInstance()->EnableAccessibility();
  } else {
    content::BrowserAccessibilityState::GetInstance()->DisableAccessibility();
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSetFocusRingFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSetFocusRingFunction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSetHighlightsFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSetHighlightsFunction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSetKeyboardListenerFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSetKeyboardListenerFunction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateDarkenScreenFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateDarkenScreenFunction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSetSwitchAccessKeysFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSetSwitchAccessKeysFunction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSetNativeChromeVoxArcSupportForCurrentAppFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSetNativeChromeVoxArcSupportForCurrentAppF"
                "unction";
  return RespondNow(Error(kErrorNotSupported));
}

ExtensionFunction::ResponseAction
AccessibilityPrivateSendSyntheticKeyEventFunction::Run() {
  LOG(ERROR) << "AccessibilityPrivateSendSyntheticKeyEventFunction";
  return RespondNow(Error(kErrorNotSupported));
}

}  // namespace api
}  // namespace cast
}  // namespace extensions
