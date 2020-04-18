// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_ACCESSORY_VIEW_H_
#define IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_ACCESSORY_VIEW_H_

#import <UIKit/UIKit.h>

@protocol KeyboardAccessoryViewDelegate

// Invoked after the user touches the `accounts` button.
- (void)accountButtonPressed;

// Invoked after the user touches the `credit cards` button.
- (void)cardsButtonPressed;

// Invoked after the user touches the `passwords` button.
- (void)passwordButtonPressed;

// Invoked after the user touches the `previous` button.
- (void)arrowUpPressed;

// Invoked after the user touches the `next` button.
- (void)arrowDownPressed;

// Invoked after the user touches the `cancel` button.
- (void)cancelButtonPressed;

@end

// This view contains the icons to activate "Manual Fill". It is meant to be
// shown above the keyboard on iPhone and above the manual fill view.
@interface KeyboardAccessoryView : UIView

// Unavailable. Use `initWithDelegate:`.
- (instancetype)init NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

// Unavailable. Use `initWithDelegate:`.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

// Instances an object with the desired delegate.
//
// @param delegate The delegate for this object.
// @return A fresh object with the passed delegate.
- (instancetype)initWithDelegate:(id<KeyboardAccessoryViewDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_ACCESSORY_VIEW_H_
