// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_UI_BAR_BUTTON_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_UI_BAR_BUTTON_ITEM_H_

#import <UIKit/UIKIt.h>

@protocol ToolbarAssistiveKeyboardDelegate;

// UIBarButtonItem wrapper that calls ToolbarAssistiveKeyboardDelegate's
// |-keyPressed:| when pressed.
@interface ToolbarUIBarButtonItem : UIBarButtonItem

// Default initializer.
- (instancetype)initWithTitle:(NSString*)title
                     delegate:(id<ToolbarAssistiveKeyboardDelegate>)delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_UI_BAR_BUTTON_ITEM_H_
