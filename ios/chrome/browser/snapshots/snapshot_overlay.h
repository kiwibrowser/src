// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_OVERLAY_H_
#define IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_OVERLAY_H_

#import <UIKit/UIKit.h>

// Simple object containing all the information needed to display an overlay
// view in a snapshot.
@interface SnapshotOverlay : NSObject

// Initialize SnapshotOverlay with the given |view| and |yOffset|.
- (instancetype)initWithView:(UIView*)view
                     yOffset:(CGFloat)yOffset NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// The overlay view.
@property(nonatomic, strong, readonly) UIView* view;

// Y offset for the overlay view. Used to adjust the y position of |_view|
// within the snapshot.
@property(nonatomic, assign, readonly) CGFloat yOffset;

@end

#endif  // IOS_CHROME_BROWSER_SNAPSHOTS_SNAPSHOT_OVERLAY_H_
