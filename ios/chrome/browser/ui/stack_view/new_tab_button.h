// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_NEW_TAB_BUTTON_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_NEW_TAB_BUTTON_H_

#import <UIKit/UIKit.h>

// UIButton subclass used for the New Tab button in the phone switcher toolbar
// and the tablet no-tabs toolbar. A NewTabButton has a custom background rect.
@interface NewTabButton : UIButton
// Whether the button opens incognito tabs or not; setting this property changes
// the button's behavior, updating its tag to
// IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB, and updates its appearance.
@property(nonatomic, assign, getter=isIncognito) BOOL incognito;

// If |animated|, animates the transition of the foreground image of the button.
- (void)setIncognito:(BOOL)incognito animated:(BOOL)animated;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_NEW_TAB_BUTTON_H_
