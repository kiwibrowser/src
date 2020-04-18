// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_TITLE_LABEL_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_TITLE_LABEL_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/fade_truncated_label.h"

// A label class for the tab label. Partially implements UIAccessibilityFocus
// protocol to enable proper VoiceOver scrolling behavior. On its target, has
// action for when it receives focus from VoiceOver.
@interface TitleLabel : FadeTruncatedLabel

- (void)addAccessibilityElementFocusedTarget:(id)accessibilityTarget
                                      action:(SEL)accessibilityAction;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_TITLE_LABEL_H_
