// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/shell/test/earl_grey/shell_matchers.h"

#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/testing/earl_grey/matchers.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/test/earl_grey/web_view_matchers.h"
#import "ios/web/shell/test/app/web_shell_test_util.h"
#import "ios/web/shell/view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

id<GREYMatcher> WebView() {
  return WebViewInWebState(shell_test_util::GetCurrentWebState());
}

id<GREYMatcher> WebViewScrollView() {
  return WebViewScrollView(shell_test_util::GetCurrentWebState());
}

id<GREYMatcher> AddressFieldText(std::string text) {
  MatchesBlock matches = ^BOOL(UIView* view) {
    if (![view isKindOfClass:[UITextField class]]) {
      return NO;
    }
    if (![[view accessibilityLabel]
            isEqualToString:kWebShellAddressFieldAccessibilityLabel]) {
      return NO;
    }
    UITextField* text_field = base::mac::ObjCCastStrict<UITextField>(view);
    NSString* error_message = [NSString
        stringWithFormat:
            @"Address field text did not match. expected: %@, actual: %@",
            base::SysUTF8ToNSString(text), text_field.text];
    GREYAssert(testing::WaitUntilConditionOrTimeout(
                   testing::kWaitForUIElementTimeout,
                   ^{
                     return base::SysNSStringToUTF8(text_field.text) == text;
                   }),
               error_message);
    return YES;
  };

  DescribeToBlock describe = ^(id<GREYDescription> description) {
    [description appendText:@"address field containing "];
    [description appendText:base::SysUTF8ToNSString(text)];
  };

  return [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                              descriptionBlock:describe];
}

id<GREYMatcher> BackButton() {
  return grey_accessibilityLabel(kWebShellBackButtonAccessibilityLabel);
}

id<GREYMatcher> ForwardButton() {
  return grey_accessibilityLabel(kWebShellForwardButtonAccessibilityLabel);
}

id<GREYMatcher> AddressField() {
  return grey_accessibilityLabel(kWebShellAddressFieldAccessibilityLabel);
}

}  // namespace web
