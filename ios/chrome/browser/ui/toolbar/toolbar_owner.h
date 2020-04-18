// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_

#import <UIKit/UIKit.h>

@class ToolbarController;
@protocol ToolbarSnapshotProviding;

@protocol ToolbarOwner<NSObject>

// TODO(crbug.com/781786): Remove this once the TabGrid is enabled.
// Returns the frame of the toolbar.
- (CGRect)toolbarFrame;

// Snapshot provider for the toolbar owner by this class.
@property(nonatomic, strong, readonly) id<ToolbarSnapshotProviding>
    toolbarSnapshotProvider;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_OWNER_H_
