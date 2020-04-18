// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_input_assistant_items.h"

#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_assistive_keyboard_delegate.h"
#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_assistive_keyboard_views.h"
#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_assistive_keyboard_views_utils.h"
#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_ui_bar_button_item.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSArray<UIBarButtonItemGroup*>* ToolbarAssistiveKeyboardLeadingBarButtonGroups(
    id<ToolbarAssistiveKeyboardDelegate> delegate) {
  NSArray<UIButton*>* buttons =
      ToolbarAssistiveKeyboardLeadingButtons(delegate);
  NSMutableArray<UIBarButtonItem*>* barButtonItems =
      [NSMutableArray arrayWithCapacity:[buttons count]];

  for (UIButton* button in buttons) {
    UIBarButtonItem* item = [[UIBarButtonItem alloc] initWithCustomView:button];
    item.accessibilityLabel = button.accessibilityLabel;
    [barButtonItems addObject:item];
  }

  UIBarButtonItemGroup* group =
      [[UIBarButtonItemGroup alloc] initWithBarButtonItems:barButtonItems
                                        representativeItem:nil];
  return @[ group ];
}

NSArray<UIBarButtonItemGroup*>* ToolbarAssistiveKeyboardTrailingBarButtonGroups(
    id<ToolbarAssistiveKeyboardDelegate> delegate,
    NSArray<NSString*>* buttonTitles) {
  NSMutableArray<UIBarButtonItem*>* barButtonItems =
      [NSMutableArray arrayWithCapacity:[buttonTitles count]];
  for (NSString* title in buttonTitles) {
    UIBarButtonItem* item =
        [[ToolbarUIBarButtonItem alloc] initWithTitle:title delegate:delegate];
    [barButtonItems addObject:item];
  }
  UIBarButtonItemGroup* group =
      [[UIBarButtonItemGroup alloc] initWithBarButtonItems:barButtonItems
                                        representativeItem:nil];
  return @[ group ];
}
