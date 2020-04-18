// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_

#import <UIKit/UIKit.h>

// Protocol for the UI displaying the toolbar.
@protocol ToolbarUI<NSObject>

// The height of the toolbar, not including the safe area inset.
// This should be broadcast using |-broadcastToolbarHeight:|.
@property(nonatomic, readonly) CGFloat toolbarHeight;

@end

// Simple implementation of ToolbarUI that allows readwrite access to broadcast
// properties.
@interface ToolbarUIState : NSObject<ToolbarUI>

// Redefine properties as readwrite.
@property(nonatomic, assign) CGFloat toolbarHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_
