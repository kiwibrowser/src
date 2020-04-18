// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_NEW_TAB_COMMAND_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_NEW_TAB_COMMAND_H_

#import <UIKit/UIKit.h>

// Command sent to open a new tab, optionally including a point (in UIWindow
// coordinates).
@interface OpenNewTabCommand : NSObject

- (instancetype)initWithIncognito:(BOOL)incognito
                      originPoint:(CGPoint)originPoint
    NS_DESIGNATED_INITIALIZER;

// Mark inherited initializer as unavailable to prevent calling it by mistake.
- (instancetype)init NS_UNAVAILABLE;

// Convenience initializers

// Creates an OpenTabCommand with |incognito| and an |originPoint| of
// CGPointZero.
+ (instancetype)commandWithIncognito:(BOOL)incognito;

// Creates an OpenTabCommand with |incognito| NO and an |originPoint| of
// CGPointZero.
+ (instancetype)command;

// Creates an OpenTabCommand with |incognito| YES and an |originPoint| of
// CGPointZero.
+ (instancetype)incognitoTabCommand;

@property(nonatomic, readonly) BOOL incognito;

@property(nonatomic, readonly) CGPoint originPoint;

// Whether the new tab command was initiated by the user (e.g. by tapping the
// new tab button in the tools menu) or not (e.g. opening a new tab via a
// Javascript action). Defaults to |YES|.
@property(nonatomic, assign, getter=isUserInitiated) BOOL userInitiated;

// Whether the new tab command should also trigger the omnibox to be focused.
@property(nonatomic, assign) BOOL shouldFocusOmnibox;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_OPEN_NEW_TAB_COMMAND_H_
