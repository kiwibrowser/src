// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/a11y_util.h"

namespace ui {
namespace a11y_util {

void HideImageFromAccessibilityOrder(NSImageView* view) {
  // This is the minimum change necessary to get VoiceOver to skip the image
  // (instead of reading the word "image"). Accessibility mechanisms in OSX
  // change once in a while, so this may be fragile.
  [[view cell] accessibilitySetOverrideValue:@""
                                forAttribute:NSAccessibilityRoleAttribute];
}

void PlayElementUpdatedSound(id source) {
  NSAccessibilityPostNotificationWithUserInfo(
      source, @"AXPlaySound",
      @{@"AXSoundIdentifier" : @"AXElementUpdatedSound"});
}

}  // namespace a11y_util
}  // namespace ui
