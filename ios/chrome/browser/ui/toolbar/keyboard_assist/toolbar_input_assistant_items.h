// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_INPUT_ASSISTANT_ITEMS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_INPUT_ASSISTANT_ITEMS_H_

#import <UIKit/UIKIt.h>

@protocol ToolbarAssistiveKeyboardDelegate;

// Returns the leading button groups for the toolbar's inputAssistantItem.
NSArray<UIBarButtonItemGroup*>* ToolbarAssistiveKeyboardLeadingBarButtonGroups(
    id<ToolbarAssistiveKeyboardDelegate> delegate);

// Returns the trailing button groups for the toolbar's inputAssistantItem.
NSArray<UIBarButtonItemGroup*>* ToolbarAssistiveKeyboardTrailingBarButtonGroups(
    id<ToolbarAssistiveKeyboardDelegate> delegate,
    NSArray<NSString*>* buttonTitles);

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_KEYBOARD_ASSIST_TOOLBAR_INPUT_ASSISTANT_ITEMS_H_
