// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_BUTTON_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_BUTTON_H_

#import <UIKit/UIKit.h>

// This class is a UIButton with an inkview.
// Replaces MDCButton, because MDCButton does a lot of blending, which is a
// problem in the tab switcher where almost the entire screen can be filled
// by buttons.
@interface TabSwitcherButton : UIButton

// Resets the state of the button, in particular the inkview state.
- (void)resetState;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_BUTTON_H_
