// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_egtest_util.h"

#import <EarlGrey/EarlGrey.h>

#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace chrome_test_util {

id<GREYMatcher> TabletTabSwitcherOpenButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_TAB_STRIP_ENTER_TAB_SWITCHER);
}

id<GREYMatcher> TabletTabSwitcherCloseButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_TAB_STRIP_LEAVE_TAB_SWITCHER);
}

id<GREYMatcher> TabletTabSwitcherNewTabButton() {
  return grey_allOf(
      ButtonWithAccessibilityLabelId(IDS_IOS_TAB_SWITCHER_CREATE_NEW_TAB),
      grey_sufficientlyVisible(), nil);
}

id<GREYMatcher> TabletTabSwitcherNewIncognitoTabButton() {
  return grey_allOf(ButtonWithAccessibilityLabelId(
                        IDS_IOS_TAB_SWITCHER_CREATE_NEW_INCOGNITO_TAB),
                    grey_sufficientlyVisible(), nil);
}

id<GREYMatcher> TabletTabSwitcherCloseTabButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_TOOLS_MENU_CLOSE_TAB);
}

id<GREYMatcher> TabletTabSwitcherOpenTabsPanelButton() {
  NSString* accessibility_label = l10n_util::GetNSStringWithFixup(
      IDS_IOS_TAB_SWITCHER_HEADER_NON_INCOGNITO_TABS);
  return grey_accessibilityLabel(accessibility_label);
}

id<GREYMatcher> TabletTabSwitcherIncognitoTabsPanelButton() {
  NSString* accessibility_label = l10n_util::GetNSStringWithFixup(
      IDS_IOS_TAB_SWITCHER_HEADER_INCOGNITO_TABS);
  return grey_accessibilityLabel(accessibility_label);
}

id<GREYMatcher> TabletTabSwitcherOtherDevicesPanelButton() {
  NSString* accessibility_label = l10n_util::GetNSStringWithFixup(
      IDS_IOS_TAB_SWITCHER_HEADER_OTHER_DEVICES_TABS);
  return grey_accessibilityLabel(accessibility_label);
}

}  // namespace chrome_test_util
