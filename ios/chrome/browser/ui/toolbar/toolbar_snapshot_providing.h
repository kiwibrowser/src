// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_SNAPSHOT_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_SNAPSHOT_PROVIDER_H_

#import <UIKit/UIKit.h>

// Protocol for an object providing toolbar snapshot.
@protocol ToolbarSnapshotProviding

// Snapshot used by the TabSwitcher.
- (UIView*)snapshotForTabSwitcher;

// Returns a snapshot used by the StackView after resizing the toolbar to
// |frame|.
- (UIView*)snapshotForStackViewWithWidth:(CGFloat)width
                          safeAreaInsets:(UIEdgeInsets)safeAreaInsets;

// Asks the provider for the color of the background of the toolbar. A nil value
// indicates the default color will be used.
- (UIColor*)toolbarBackgroundColor;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_SNAPSHOT_PROVIDER_H_
