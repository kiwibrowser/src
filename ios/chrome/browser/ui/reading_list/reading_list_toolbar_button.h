// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_BUTTON_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_BUTTON_H_

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, ButtonPositioning) { Leading, Centered, Trailing };

// A button class for the buttons in ReadingListToolBar's stackview.  Handles
// the layout alignment of the button and its titleLabel.
@interface ReadingListToolbarButton : UIView

// Initializer.
- (instancetype)initWithText:(NSString*)labelText
                 destructive:(BOOL)isDestructive
                    position:(ButtonPositioning)position;

// Associates a target object and action method with the UIButton.
- (void)addTarget:(id)target
              action:(SEL)action
    forControlEvents:(UIControlEvents)controlEvents;

// Sets the title text of the UIButton.
- (void)setTitle:(NSString*)title;

// Enables or disables the UIButton.
- (void)setEnabled:(BOOL)enabled;

// Gets the titleLabel of the UIButton.
- (UILabel*)titleLabel;

// Sets the maximum width contraint of ReadingListToolbarButton.
- (void)setMaxWidth:(CGFloat)maxWidth;

// The font of the title text.
+ (UIFont*)textFont;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_TOOLBAR_BUTTON_H_
